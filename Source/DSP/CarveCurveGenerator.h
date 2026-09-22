#pragma once

#include <vector>
#include "../Parameters.h"
#include "SmoothingEnvelope.h"

/**
    Traduce el mapa de conflicto (0-1 por banda) en una curva de
    reducción de ganancia objetivo, en dB (valores <= 0), según el
    parámetro Carve y el modo de trabajo.

    - Dynamic: sigue el conflicto instante a instante (ya suavizado
      por SmoothingEnvelope) — solo reduce cuando hay masking real en
      ese momento.
    - Static: usa un promedio de mucho más largo plazo del conflicto
      en vez del valor instantáneo, dando un corte fijo y suave en la
      zona de mayor conflicto histórico, tal como se describe en la
      arquitectura (3.1 / sección "Static").
*/
class CarveCurveGenerator
{
public:
    static constexpr float kMaxReductionDb = 18.0f;

    void prepare(double sampleRate, double hopSizeSeconds, int numBandsIn)
    {
        numBands = numBandsIn;
        envelope.prepare(sampleRate, hopSizeSeconds, numBands);
        staticAverage.assign((size_t) numBands, 0.0f);
        gainReductionDb.assign((size_t) numBands, 0.0f);
    }

    void setSmoothness(float smoothness01) { envelope.setSmoothness(smoothness01); }

    const std::vector<float>& generate(const std::vector<float>& conflict,
                                        float carveAmount01,
                                        ClearSpaceParams::Mode mode)
    {
        target.resize((size_t) numBands);

        if (mode == ClearSpaceParams::Mode::Dynamic)
        {
            target = conflict;
        }
        else // Static: promedio muy lento, casi un "perfil" del conflicto
        {
            constexpr float staticAvgCoeff = 0.002f;
            for (int b = 0; b < numBands; ++b)
                staticAverage[(size_t) b] += staticAvgCoeff * (conflict[(size_t) b] - staticAverage[(size_t) b]);

            target = staticAverage;
        }

        envelope.process(target, smoothed);

        const float carve = std::clamp(carveAmount01, 0.0f, 1.0f);
        for (int b = 0; b < numBands; ++b)
            gainReductionDb[(size_t) b] = -smoothed[(size_t) b] * carve * kMaxReductionDb;

        return gainReductionDb;
    }

private:
    int numBands = 32;
    SmoothingEnvelope envelope;

    std::vector<float> target;
    std::vector<float> smoothed;
    std::vector<float> staticAverage;
    std::vector<float> gainReductionDb;
};
