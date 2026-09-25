#pragma once

#include <vector>
#include <cmath>
#include <algorithm>

/**
    Agrupa bins de FFT en bandas perceptuales (escala Bark) en vez de
    trabajar bin a bin. Esto reduce el costo computacional del análisis
    y de la corrección posterior, y da resultados más musicales que una
    resolución lineal en Hz (ver sección 3.1 de la arquitectura).
*/
class BandMapper
{
public:
    void prepare(double sampleRate, int fftSize, int numBandsToUse)
    {
        sr = sampleRate;
        fft = fftSize;
        numBands = numBandsToUse;
        buildMapping();
    }

    int getNumBands() const noexcept { return numBands; }

    /** Frecuencia central aproximada de una banda, en Hz. Útil para
        centrar los filtros del DynamicFilterBank. */
    float getBandCentreFrequency(int bandIndex) const noexcept
    {
        return bandCentreHz[(size_t) bandIndex];
    }

    /** Convierte un array de magnitudes de bins FFT (tamaño fftSize/2+1)
        a energía promedio por banda (magnitud^2 promediada). */
    void mapToBandEnergies(const float* magnitudes, int numMagnitudes,
                            std::vector<float>& outBandEnergies) const
    {
        outBandEnergies.assign((size_t) numBands, 0.0f);
        thread_local std::vector<int> counts;
        counts.assign((size_t) numBands, 0);

        for (int bin = 0; bin < numMagnitudes && bin < (int) binToBand.size(); ++bin)
        {
            int band = binToBand[(size_t) bin];
            if (band < 0)
                continue;

            float mag = magnitudes[bin];
            outBandEnergies[(size_t) band] += mag * mag;
            counts[(size_t) band]++;
        }

        for (int b = 0; b < numBands; ++b)
            if (counts[(size_t) b] > 0)
                outBandEnergies[(size_t) b] /= (float) counts[(size_t) b];
    }

private:
    static float hzToBark(float hz)
    {
        return 13.0f * std::atan(0.00076f * hz)
             + 3.5f * std::atan((hz / 7500.0f) * (hz / 7500.0f));
    }

    // Aproximación razonable para fines de UI/filtros; no necesita ser
    // exacta, solo dar centros de banda espaciados perceptualmente.
    static float barkToHzApprox(float bark)
    {
        return 600.0f * std::sinh(bark / 6.0f);
    }

    void buildMapping()
    {
        const int numBins = fft / 2 + 1;
        binToBand.assign((size_t) numBins, -1);
        bandCentreHz.assign((size_t) numBands, 0.0f);

        const float nyquist = (float) (sr * 0.5);
        const float maxBark = hzToBark(nyquist);
        const float barkStep = maxBark / (float) numBands;

        for (int bin = 0; bin < numBins; ++bin)
        {
            const float hz = (float) bin * (float) sr / (float) fft;
            const float bark = hzToBark(hz);
            int band = (int) (bark / barkStep);
            band = std::clamp(band, 0, numBands - 1);
            binToBand[(size_t) bin] = band;
        }

        for (int b = 0; b < numBands; ++b)
        {
            const float barkCentre = ((float) b + 0.5f) * barkStep;
            bandCentreHz[(size_t) b] = std::clamp(barkToHzApprox(barkCentre), 20.0f, nyquist * 0.98f);
        }
    }

    double sr = 44100.0;
    int fft = 1024;
    int numBands = 32;

    std::vector<int> binToBand;
    std::vector<float> bandCentreHz;
};
