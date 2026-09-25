#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include "BandMapper.h"

/**
    Banco de filtros paramétricos (campana) dinámicos, uno por banda
    perceptual, con ganancia modulada en tiempo real según la curva de
    CarveCurveGenerator.

    Elegido sobre un enfoque FFT/IFFT (Opción B de la arquitectura)
    porque cumple mejor los requisitos de baja latencia y bajo consumo
    de CPU del MVP. Los coeficientes se recalculan a la tasa de hop del
    analizador (no por sample), lo cual es barato y, combinado con el
    smoothing ya aplicado a la curva de ganancia, evita zipper noise
    perceptible.

    Limitación conocida (documentada, no un bug): al ser filtros en
    serie con anchos de banda que pueden solaparse ligeramente entre
    bandas vecinas, no son matemáticamente independientes entre sí como
    lo sería un procesamiento espectral puro. En la práctica, para el
    caso de uso (2-3 zonas de conflicto simultáneas como mucho), esto
    no es audible y es el trade-off correcto para mantener el plugin
    liviano.
*/
class DynamicFilterBank
{
public:
    void prepare(double sampleRateIn, int samplesPerBlock, int numChannels,
                 const BandMapper& bandMapperIn)
    {
        sampleRate = sampleRateIn;
        bandMapper = &bandMapperIn;
        numBands = bandMapper->getNumBands();

        filters.clear();
        filters.resize((size_t) juce::jmax(1, numChannels));

        juce::dsp::ProcessSpec spec {
            sampleRate,
            (juce::uint32) samplesPerBlock,
            (juce::uint32) 1 // un canal por filtro (procesamos canal por canal)
        };

        for (auto& channelFilters : filters)
        {
            channelFilters.clear();
            channelFilters.reserve((size_t) numBands);
            for (int b = 0; b < numBands; ++b)
            {
                auto filter = std::make_unique<juce::dsp::IIR::Filter<float>>();
                filter->prepare(spec);
                channelFilters.push_back(std::move(filter));
            }
        }

        currentGainsDb.assign((size_t) numBands, 0.0f);
        updateCoefficients(currentGainsDb); // arranca neutro (0 dB en todas las bandas)
    }

    void reset()
    {
        for (auto& channelFilters : filters)
            for (auto& f : channelFilters)
                f->reset();
    }

    /** Recalcula los coeficientes de todas las bandas. Se llama a la
        tasa de hop del analizador (decenas de veces por segundo), no
        por bloque de audio ni por sample. */
    void updateCoefficients(const std::vector<float>& gainReductionDb)
    {
        currentGainsDb = gainReductionDb;
        const float nyquistSafe = (float) (sampleRate * 0.49);

        for (int b = 0; b < numBands; ++b)
        {
            const float freq = juce::jlimit(20.0f, nyquistSafe, bandMapper->getBandCentreFrequency(b));
            const float gainDb = currentGainsDb[(size_t) b];
            const float gainFactor = juce::Decibels::decibelsToGain(gainDb);
            constexpr float q = 1.5f; // ancho moderado, aproximado al ancho de banda Bark

            auto coeffs = juce::dsp::IIR::Coefficients<float>::makePeakFilter(sampleRate, freq, q, gainFactor);

            for (auto& channelFilters : filters)
                *(channelFilters[(size_t) b]->coefficients) = *coeffs;
        }
    }

    /** Procesa un buffer multicanal en el dominio de tiempo, canal por
        canal, pasando cada muestra por la cadena de bandas en serie. */
    void process(juce::AudioBuffer<float>& buffer)
    {
        const int numCh = juce::jmin(buffer.getNumChannels(), (int) filters.size());

        for (int ch = 0; ch < numCh; ++ch)
        {
            auto block = juce::dsp::AudioBlock<float>(buffer).getSingleChannelBlock((size_t) ch);
            juce::dsp::ProcessContextReplacing<float> context(block);

            for (int b = 0; b < numBands; ++b)
                filters[(size_t) ch][(size_t) b]->process(context);
        }
    }

    /** Mayor reducción activa entre todas las bandas, para el medidor de la UI. */
    float getPeakGainReductionDb() const
    {
        float minDb = 0.0f;
        for (float g : currentGainsDb)
            minDb = std::min(minDb, g);
        return -minDb;
    }

private:
    double sampleRate = 44100.0;
    int numBands = 32;
    const BandMapper* bandMapper = nullptr;

    // [canal][banda]
    std::vector<std::vector<std::unique_ptr<juce::dsp::IIR::Filter<float>>>> filters;
    std::vector<float> currentGainsDb;
};
