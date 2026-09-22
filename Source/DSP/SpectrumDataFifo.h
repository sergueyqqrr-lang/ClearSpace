#pragma once

#include <juce_core/juce_core.h>
#include <array>
#include <vector>
#include "Constants.h"

/**
    Pasa "frames" de datos de análisis (energía por banda de main y de
    sidechain, mapa de conflicto, y la reducción de ganancia pico)
    desde el audio thread hacia el UI thread, sin locks.

    Se apoya en juce::AbstractFifo (single-producer/single-consumer,
    lock-free): el audio thread empuja un frame cada vez que hay un
    análisis nuevo (~172 veces/seg a 44.1kHz con hop=256). Si la UI no
    ha vaciado el fifo a tiempo, el push simplemente no encuentra
    espacio y el frame se descarta sin bloquear al audio thread — no
    hay problema, a la UI solo le interesa el dato más reciente, no el
    historial completo.
*/
class SpectrumDataFifo
{
public:
    static constexpr int numBands = ClearSpaceConstants::numBands;

    struct Frame
    {
        std::array<float, (size_t) numBands> mainEnergies {};
        std::array<float, (size_t) numBands> sidechainEnergies {};
        std::array<float, (size_t) numBands> conflict {};
        std::array<float, (size_t) numBands> bandCentreHz {}; // para el eje logarítmico de la UI
        float peakGainReductionDb = 0.0f;
    };

    SpectrumDataFifo() : fifo(capacity)
    {
        frames.resize((size_t) capacity);
    }

    /** Audio thread. No bloquea nunca: si no hay espacio libre, descarta el frame. */
    void pushFrame(const Frame& frame)
    {
        int start1, size1, start2, size2;
        fifo.prepareToWrite(1, start1, size1, start2, size2);

        if (size1 > 0)
            frames[(size_t) start1] = frame;
        else if (size2 > 0)
            frames[(size_t) start2] = frame;

        fifo.finishedWrite(size1 + size2);
    }

    /** UI thread. Vacía el fifo y se queda con el frame más reciente.
        Devuelve true si había al menos un frame nuevo disponible. */
    bool popLatestFrame(Frame& outFrame)
    {
        bool gotOne = false;

        while (fifo.getNumReady() > 0)
        {
            int start1, size1, start2, size2;
            fifo.prepareToRead(1, start1, size1, start2, size2);

            if (size1 > 0)
            {
                outFrame = frames[(size_t) start1];
                gotOne = true;
            }
            else if (size2 > 0)
            {
                outFrame = frames[(size_t) start2];
                gotOne = true;
            }

            fifo.finishedRead(size1 + size2);
        }

        return gotOne;
    }

private:
    static constexpr int capacity = 8;
    juce::AbstractFifo fifo;
    std::vector<Frame> frames;
};
