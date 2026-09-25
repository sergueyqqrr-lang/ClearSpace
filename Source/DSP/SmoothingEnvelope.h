#pragma once

#include <juce_dsp/juce_dsp.h>
#include <vector>
#include <cmath>

/**
    Suaviza temporalmente (attack/release, un polo por banda) el mapa
    de conflicto o la curva de ganancia, para que el efecto sea
    musical y no "bombee" al reaccionar de forma instantánea.

    Opera a la tasa de frames de análisis del SpectralAnalyzer (no a
    sample rate), ya que el conflicto solo se recalcula cada hop.
*/
class SmoothingEnvelope
{
public:
    void prepare(double sampleRate, double hopSizeSeconds, int numBandsIn)
    {
        juce::ignoreUnused(sampleRate);
        frameRate = hopSizeSeconds > 0.0 ? (1.0 / hopSizeSeconds) : 100.0;
        numBands = numBandsIn;
        state.assign((size_t) numBands, 0.0f);
    }

    /** smoothness: 0-1. Mapea a tiempos de attack/release en ms
        (attack más rápido que release, como un compresor musical). */
    void setSmoothness(float smoothness01)
    {
        const float s = std::clamp(smoothness01, 0.0f, 1.0f);
        attackMs = juce::jmap(s, 0.0f, 1.0f, 5.0f, 60.0f);
        releaseMs = juce::jmap(s, 0.0f, 1.0f, 20.0f, 200.0f);
    }

    void process(const std::vector<float>& target, std::vector<float>& outSmoothed)
    {
        outSmoothed.resize((size_t) numBands);

        for (int b = 0; b < numBands; ++b)
        {
            const float t = target[(size_t) b];
            float& s = state[(size_t) b];

            const bool rising = t > s;
            const float timeMs = rising ? attackMs : releaseMs;
            const float coeff = timeConstantToCoeff(timeMs);

            s += coeff * (t - s);
            outSmoothed[(size_t) b] = s;
        }
    }

private:
    float timeConstantToCoeff(float timeMs) const
    {
        const double timeSeconds = (double) timeMs / 1000.0;
        if (timeSeconds <= 0.0)
            return 1.0f;

        const double coeff = 1.0 - std::exp(-1.0 / (timeSeconds * frameRate));
        return (float) juce::jlimit(0.0, 1.0, coeff);
    }

    double frameRate = 172.0; // frames de análisis por segundo (sampleRate / hopSize)
    int numBands = 32;

    float attackMs = 20.0f;
    float releaseMs = 80.0f;

    std::vector<float> state;
};
