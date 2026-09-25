#pragma once

#include <vector>
#include <algorithm>
#include <cmath>

/**
    Calcula, banda por banda, cuánto masking está causando el
    sidechain sobre la señal principal (ver sección 3.1 de la
    arquitectura).

    Lógica: el conflicto en una banda es alto solo cuando
      (a) ambas señales tienen energía relevante ahí (solapamiento), Y
      (b) el sidechain domina o iguala al main en esa banda.
    Si el main ya es más fuerte que el sidechain en una banda, no
    tiene sentido "tallarlo" — el masking real lo sufre el elemento
    más débil, no el más fuerte.

    Trabaja en términos relativos (normalizado por el pico de cada
    señal en el frame actual), no en energía absoluta, para responder
    de forma consistente sin importar el nivel de entrada.
*/
class MaskingDetector
{
public:
    void prepare(int numBandsIn)
    {
        numBands = numBandsIn;
        conflict.assign((size_t) numBands, 0.0f);
    }

    const std::vector<float>& computeConflict(const std::vector<float>& mainEnergies,
                                               const std::vector<float>& sidechainEnergies)
    {
        const float mainMax = maxOrFloor(mainEnergies);
        const float scMax = maxOrFloor(sidechainEnergies);

        for (int b = 0; b < numBands; ++b)
        {
            const float mainE = mainEnergies[(size_t) b];
            const float scE = sidechainEnergies[(size_t) b];

            if (mainE < noiseFloorLinear && scE < noiseFloorLinear)
            {
                conflict[(size_t) b] = 0.0f;
                continue;
            }

            const float mainNorm = mainE / mainMax;
            const float scNorm = scE / scMax;

            const float total = mainNorm + scNorm;
            const float dominance = total > 1.0e-9f ? (scNorm / total) : 0.0f;
            const float overlap = std::min(mainNorm, scNorm);

            // Solo cuenta cuando el sidechain domina (>0.5); por debajo,
            // el peso cae a 0 (el main ya "gana" esa banda).
            const float dominanceWeight = std::clamp((dominance - 0.5f) * 2.0f, 0.0f, 1.0f);

            conflict[(size_t) b] = overlap * dominanceWeight;
        }

        return conflict;
    }

private:
    static float maxOrFloor(const std::vector<float>& v)
    {
        float m = 1.0e-9f;
        for (float x : v)
            m = std::max(m, x);
        return m;
    }

    int numBands = 32;
    std::vector<float> conflict;
    const float noiseFloorLinear = 1.0e-6f; // umbral mínimo, evita reaccionar al ruido de piso
};
