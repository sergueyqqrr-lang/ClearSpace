#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <memory>
#include "BandMapper.h"

/**
    Análisis espectral de una señal mono (main o sidechain, ya
    downmixed) mediante FFT con ventana Hann y overlap 75% (hop =
    fftSize/4). Expone energía por banda perceptual, recalculada cada
    `hopSize` muestras nuevas.

    Importante: este analizador NO hace resíntesis y no está en el
    camino de la señal de salida — solo alimenta al MaskingDetector.
    Por eso su latencia interna (el tamaño de ventana FFT) no añade
    latencia a la señal principal, que se procesa en paralelo con el
    DynamicFilterBank (dominio de tiempo).
*/
class SpectralAnalyzer
{
public:
    void prepare(double sampleRate, int fftOrderIn, int numBands)
    {
        fftOrder = fftOrderIn;
        fftSize = 1 << fftOrder;
        hopSize = fftSize / 4;

        fft = std::make_unique<juce::dsp::FFT>(fftOrder);

        window.resize((size_t) fftSize);
        juce::dsp::WindowingFunction<float>::fillWindowingTables(
            window.data(), (size_t) fftSize,
            juce::dsp::WindowingFunction<float>::hann, false);

        fifo.assign((size_t) fftSize, 0.0f);
        fftWorkBuffer.assign((size_t) fftSize * 2, 0.0f);
        magnitudes.assign((size_t) (fftSize / 2 + 1), 0.0f);

        fifoWriteIndex = 0;
        samplesSinceLastHop = 0;

        bandMapper.prepare(sampleRate, fftSize, numBands);
        bandEnergies.assign((size_t) numBands, 0.0f);
        newDataAvailable = false;
    }

    /** Empuja un bloque mono. Puede llamarse con bloques de cualquier
        tamaño (no necesitan coincidir con el hop interno). */
    void pushBlock(const float* samples, int numSamples)
    {
        for (int i = 0; i < numSamples; ++i)
        {
            fifo[(size_t) fifoWriteIndex] = samples[i];
            fifoWriteIndex = (fifoWriteIndex + 1) % fftSize;

            if (++samplesSinceLastHop >= hopSize)
            {
                samplesSinceLastHop = 0;
                performAnalysis();
            }
        }
    }

    const std::vector<float>& getBandEnergies() const noexcept { return bandEnergies; }
    const BandMapper& getBandMapper() const noexcept { return bandMapper; }
    int getHopSize() const noexcept { return hopSize; }

    /** true si hubo un nuevo análisis desde la última consulta (consume el flag). */
    bool consumeNewDataFlag() noexcept
    {
        const bool v = newDataAvailable;
        newDataAvailable = false;
        return v;
    }

private:
    void performAnalysis()
    {
        // Reordena el ring buffer en orden temporal correcto y aplica la ventana
        for (int i = 0; i < fftSize; ++i)
        {
            const int readIndex = (fifoWriteIndex + i) % fftSize;
            fftWorkBuffer[(size_t) i] = fifo[(size_t) readIndex] * window[(size_t) i];
        }
        std::fill(fftWorkBuffer.begin() + fftSize, fftWorkBuffer.end(), 0.0f);

        fft->performFrequencyOnlyForwardTransform(fftWorkBuffer.data());

        for (int bin = 0; bin < fftSize / 2 + 1; ++bin)
            magnitudes[(size_t) bin] = fftWorkBuffer[(size_t) bin];

        bandMapper.mapToBandEnergies(magnitudes.data(), (int) magnitudes.size(), bandEnergies);
        newDataAvailable = true;
    }

    int fftOrder = 10;
    int fftSize = 1024;
    int hopSize = 256;

    std::unique_ptr<juce::dsp::FFT> fft;
    std::vector<float> window;
    std::vector<float> fifo;
    std::vector<float> fftWorkBuffer;
    std::vector<float> magnitudes;

    int fifoWriteIndex = 0;
    int samplesSinceLastHop = 0;

    BandMapper bandMapper;
    std::vector<float> bandEnergies;
    bool newDataAvailable = false;
};
