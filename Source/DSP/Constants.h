#pragma once

namespace ClearSpaceConstants
{
    /** Número de bandas perceptuales usado en todo el pipeline:
        BandMapper, MaskingDetector, CarveCurveGenerator,
        DynamicFilterBank y el frame compartido con la UI
        (SpectrumDataFifo). Centralizado aquí para que nunca se
        desincronicen entre sí. */
    static constexpr int numBands = 32;

    /** Orden de FFT usado por SpectralAnalyzer (fftSize = 1 << fftOrder). */
    static constexpr int fftOrder = 10; // 1024 puntos
}
