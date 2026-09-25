#include "PluginProcessor.h"
#include "PluginEditor.h"

//==============================================================================
ClearSpaceProcessor::ClearSpaceProcessor()
    : AudioProcessor(BusesProperties()
                          .withInput("Input", juce::AudioChannelSet::stereo(), true)
                          .withOutput("Output", juce::AudioChannelSet::stereo(), true)
                          .withInput("Sidechain", juce::AudioChannelSet::stereo(), false)),
      apvts(*this, nullptr, "PARAMETERS", ClearSpaceParams::createParameterLayout())
{
}

ClearSpaceProcessor::~ClearSpaceProcessor() = default;

//==============================================================================
void ClearSpaceProcessor::prepareToPlay(double sampleRate, int samplesPerBlock)
{
    currentSampleRate = sampleRate;
    currentBlockSize = samplesPerBlock;

    mainAnalyzer.prepare(sampleRate, ClearSpaceConstants::fftOrder, ClearSpaceConstants::numBands);
    sidechainAnalyzer.prepare(sampleRate, ClearSpaceConstants::fftOrder, ClearSpaceConstants::numBands);
    maskingDetector.prepare(ClearSpaceConstants::numBands);

    const double hopSizeSeconds = (double) mainAnalyzer.getHopSize() / sampleRate;
    carveCurveGenerator.prepare(sampleRate, hopSizeSeconds, ClearSpaceConstants::numBands);

    const int numMainChannels = juce::jmax(1, getMainBusNumOutputChannels());
    filterBank.prepare(sampleRate, samplesPerBlock, numMainChannels, mainAnalyzer.getBandMapper());

    dryBuffer.setSize(numMainChannels, samplesPerBlock);
    monoScratch.reserve((size_t) samplesPerBlock);

    currentGainReductionDb = 0.0f;
}

void ClearSpaceProcessor::releaseResources()
{
    filterBank.reset();
}

void ClearSpaceProcessor::downmixToMono(const juce::AudioBuffer<float>& buffer, std::vector<float>& dest)
{
    const int numSamples = buffer.getNumSamples();
    const int numCh = buffer.getNumChannels();

    dest.assign((size_t) numSamples, 0.0f);

    for (int ch = 0; ch < numCh; ++ch)
    {
        const auto* data = buffer.getReadPointer(ch);
        for (int i = 0; i < numSamples; ++i)
            dest[(size_t) i] += data[i];
    }

    const float norm = numCh > 0 ? 1.0f / (float) numCh : 1.0f;
    for (auto& v : dest)
        v *= norm;
}

bool ClearSpaceProcessor::isBusesLayoutSupported(const BusesLayout& layouts) const
{
    // Main in/out deben ser mono o estéreo, y coincidir entre sí.
    const auto mainIn = layouts.getMainInputChannelSet();
    const auto mainOut = layouts.getMainOutputChannelSet();

    if (mainOut != juce::AudioChannelSet::mono() && mainOut != juce::AudioChannelSet::stereo())
        return false;

    if (mainIn != mainOut)
        return false;

    // Sidechain (segundo bus de entrada) es opcional, pero si está
    // presente debe ser mono o estéreo.
    const auto sidechain = layouts.getChannelSet(true, 1);
    if (!sidechain.isDisabled()
        && sidechain != juce::AudioChannelSet::mono()
        && sidechain != juce::AudioChannelSet::stereo())
        return false;

    return true;
}

bool ClearSpaceProcessor::checkSidechainConnected()
{
    return getBus(true, 1) != nullptr && getBus(true, 1)->isEnabled();
}

void ClearSpaceProcessor::processBlock(juce::AudioBuffer<float>& buffer, juce::MidiBuffer&)
{
    juce::ScopedNoDenormals noDenormals;

    const bool bypassed = apvts.getRawParameterValue(ClearSpaceParams::bypassID)->load() > 0.5f;

    sidechainConnected = checkSidechainConnected();

    auto mainBlock = getBusBuffer(buffer, true, 0);

    if (bypassed || !sidechainConnected)
    {
        // Sin sidechain no hay nada que "tallar": pasamos la señal
        // principal intacta. (En una versión posterior, mostrar aviso
        // "No sidechain detected" en la UI en vez de fallar en silencio.)
        currentGainReductionDb = 0.0f;
        return;
    }

    auto sidechainBlock = getBusBuffer(buffer, true, 1);

    // ---- Lectura de parámetros ----
    const float carve = apvts.getRawParameterValue(ClearSpaceParams::carveID)->load() / 100.0f; // 0-1
    const auto mode = static_cast<ClearSpaceParams::Mode>(
        static_cast<int>(apvts.getRawParameterValue(ClearSpaceParams::modeID)->load()));
    const float smoothness = apvts.getRawParameterValue(ClearSpaceParams::smoothnessID)->load() / 100.0f;
    const bool listenMode = apvts.getRawParameterValue(ClearSpaceParams::listenID)->load() > 0.5f;
    const float outputGainDb = apvts.getRawParameterValue(ClearSpaceParams::outputGainID)->load();

    carveCurveGenerator.setSmoothness(smoothness);

    // Guarda una copia dry antes de procesar, solo si Listen está activo
    // (evita el costo de la copia cuando no se necesita).
    if (listenMode)
        dryBuffer.makeCopyOf(mainBlock, true);

    // ---- 1. Análisis: downmix a mono y alimentar los analizadores ----
    downmixToMono(mainBlock, monoScratch);
    mainAnalyzer.pushBlock(monoScratch.data(), (int) monoScratch.size());

    downmixToMono(sidechainBlock, monoScratch);
    sidechainAnalyzer.pushBlock(monoScratch.data(), (int) monoScratch.size());

    // ---- 2-4. Cuando hay un nuevo frame de análisis, recalcular la curva ----
    const bool mainHasNewFrame = mainAnalyzer.consumeNewDataFlag();
    const bool sidechainHasNewFrame = sidechainAnalyzer.consumeNewDataFlag();

    if (mainHasNewFrame || sidechainHasNewFrame)
    {
        const auto& conflict = maskingDetector.computeConflict(
            mainAnalyzer.getBandEnergies(), sidechainAnalyzer.getBandEnergies());

        const auto& gainCurve = carveCurveGenerator.generate(conflict, carve, mode);

        filterBank.updateCoefficients(gainCurve);
        currentGainReductionDb = filterBank.getPeakGainReductionDb();

        // Publica un frame de análisis para el SpectrumDisplay de la UI.
        // pushFrame es no bloqueante: si la UI no ha vaciado el fifo a
        // tiempo, este frame se descarta sin afectar el audio thread.
        SpectrumDataFifo::Frame frame;
        std::copy(mainAnalyzer.getBandEnergies().begin(), mainAnalyzer.getBandEnergies().end(),
                   frame.mainEnergies.begin());
        std::copy(sidechainAnalyzer.getBandEnergies().begin(), sidechainAnalyzer.getBandEnergies().end(),
                   frame.sidechainEnergies.begin());
        std::copy(conflict.begin(), conflict.end(), frame.conflict.begin());
        for (int b = 0; b < ClearSpaceConstants::numBands; ++b)
            frame.bandCentreHz[(size_t) b] = mainAnalyzer.getBandMapper().getBandCentreFrequency(b);
        frame.peakGainReductionDb = currentGainReductionDb.load();

        spectrumDataFifo.pushFrame(frame);
    }

    // ---- 5. Procesamiento en tiempo real de la señal principal ----
    filterBank.process(mainBlock);

    // ---- 6. Modo Listen: reemplaza la señal por la diferencia (lo que se quitó) ----
    if (listenMode)
    {
        for (int ch = 0; ch < mainBlock.getNumChannels(); ++ch)
        {
            auto* processed = mainBlock.getWritePointer(ch);
            const auto* dry = dryBuffer.getReadPointer(ch);
            for (int i = 0; i < mainBlock.getNumSamples(); ++i)
                processed[i] = dry[i] - processed[i];
        }
    }

    const float outputGainLinear = juce::Decibels::decibelsToGain(outputGainDb);
    mainBlock.applyGain(outputGainLinear);
}

//==============================================================================
juce::AudioProcessorEditor* ClearSpaceProcessor::createEditor()
{
    return new ClearSpaceEditor(*this);
}

//==============================================================================
void ClearSpaceProcessor::getStateInformation(juce::MemoryBlock& destData)
{
    if (auto state = apvts.copyState(); state.isValid())
    {
        std::unique_ptr<juce::XmlElement> xml(state.createXml());
        copyXmlToBinary(*xml, destData);
    }
}

void ClearSpaceProcessor::setStateInformation(const void* data, int sizeInBytes)
{
    if (std::unique_ptr<juce::XmlElement> xmlState(getXmlFromBinary(data, sizeInBytes)); xmlState != nullptr)
        if (xmlState->hasTagName(apvts.state.getType()))
            apvts.replaceState(juce::ValueTree::fromXml(*xmlState));
}

//==============================================================================
// Punto de entrada requerido por JUCE
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new ClearSpaceProcessor();
}
