#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include <juce_dsp/juce_dsp.h>
#include "Parameters.h"
#include "DSP/Constants.h"
#include "DSP/SpectralAnalyzer.h"
#include "DSP/MaskingDetector.h"
#include "DSP/CarveCurveGenerator.h"
#include "DSP/DynamicFilterBank.h"
#include "DSP/SpectrumDataFifo.h"

//==============================================================================
/**
    ClearSpaceProcessor
    ---------------------------------------------------------------
    Orquesta el flujo completo: recibe main + sidechain, analiza
    ambos con SpectralAnalyzer/BandMapper, calcula el conflicto con
    MaskingDetector, genera la curva de reducción con
    CarveCurveGenerator, y la aplica en tiempo real sobre la señal
    principal con DynamicFilterBank. Publica cada frame de análisis a
    través de SpectrumDataFifo para que la UI lo dibuje sin locks.
*/
class ClearSpaceProcessor : public juce::AudioProcessor
{
public:
    ClearSpaceProcessor();
    ~ClearSpaceProcessor() override;

    //==============================================================
    void prepareToPlay(double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;
    bool isBusesLayoutSupported(const BusesLayout& layouts) const override;
    void processBlock(juce::AudioBuffer<float>&, juce::MidiBuffer&) override;
    using AudioProcessor::processBlock; // evita warning por ocultar el overload de double (no lo usamos)

    //==============================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override { return true; }

    //==============================================================
    const juce::String getName() const override { return JucePlugin_Name; }
    bool acceptsMidi() const override { return false; }
    bool producesMidi() const override { return false; }
    bool isMidiEffect() const override { return false; }
    double getTailLengthSeconds() const override { return 0.0; }

    //==============================================================
    int getNumPrograms() override { return 1; }
    int getCurrentProgram() override { return 0; }
    void setCurrentProgram(int) override {}
    const juce::String getProgramName(int) override { return {}; }
    void changeProgramName(int, const juce::String&) override {}

    //==============================================================
    void getStateInformation(juce::MemoryBlock& destData) override;
    void setStateInformation(const void* data, int sizeInBytes) override;

    //==============================================================
    // Acceso para la UI
    juce::AudioProcessorValueTreeState apvts;

    /** Reducción de ganancia actual en dB, para el medidor de la UI.
        Actualizado en el audio thread, leído desde el UI thread
        (atomic, lectura aproximada — suficiente para un medidor). */
    std::atomic<float>& getGainReductionValue() { return currentGainReductionDb; }

    /** true si hay una señal de sidechain conectada y activa. */
    bool isSidechainConnected() const noexcept { return sidechainConnected; }

    /** Fuente de datos de análisis para el SpectrumDisplay de la UI
        (lock-free, seguro de leer desde el UI thread). */
    SpectrumDataFifo& getSpectrumDataFifo() noexcept { return spectrumDataFifo; }

private:
    //==============================================================
    bool checkSidechainConnected();
    static void downmixToMono(const juce::AudioBuffer<float>& buffer, std::vector<float>& dest);

    SpectralAnalyzer mainAnalyzer;
    SpectralAnalyzer sidechainAnalyzer;
    MaskingDetector maskingDetector;
    CarveCurveGenerator carveCurveGenerator;
    DynamicFilterBank filterBank;
    SpectrumDataFifo spectrumDataFifo;

    juce::AudioBuffer<float> dryBuffer;   // copia pre-proceso, para el modo Listen
    std::vector<float> monoScratch;       // downmix temporal para el analizador

    std::atomic<float> currentGainReductionDb { 0.0f };
    bool sidechainConnected = false;

    double currentSampleRate = 44100.0;
    int currentBlockSize = 512;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClearSpaceProcessor)
};
