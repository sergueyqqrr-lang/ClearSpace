#pragma once

#include <juce_gui_basics/juce_gui_basics.h>
#include <algorithm>
#include <cmath>
#include "../DSP/SpectrumDataFifo.h"

/**
    Visualización del análisis espectral en tiempo real:
      - Línea de la señal principal (sólida).
      - Línea de la señal de sidechain (sutil, semitransparente).
      - Bandas resaltadas donde se está detectando y corrigiendo
        masking (proporcional a la intensidad del conflicto).
      - Eje de frecuencia en escala logarítmica real, con grid y
        etiquetas (50, 100, 200, 500, 1k, 2k, 5k, 10k, 20k), acotado al
        rango real de las bandas analizadas a este sample rate.

    Recibe los datos ya calculados por el audio thread a través de
    SpectrumDataFifo — este componente solo dibuja, no hace ningún
    análisis por su cuenta.
*/
class SpectrumDisplay : public juce::Component
{
public:
    void setLatestFrame(const SpectrumDataFifo::Frame& frame)
    {
        latestFrame = frame;
        hasReceivedData = true;
        repaint();
    }

    void setSidechainConnected(bool connected) noexcept
    {
        if (sidechainConnected != connected)
        {
            sidechainConnected = connected;
            if (!connected)
                hasReceivedData = false;
            repaint();
        }
    }

    void paint(juce::Graphics& g) override
    {
        auto bounds = getLocalBounds().toFloat();

        g.setColour(juce::Colour(0xff1a1a1e));
        g.fillRoundedRectangle(bounds, 6.0f);
        g.setColour(juce::Colours::grey.withAlpha(0.4f));
        g.drawRoundedRectangle(bounds.reduced(0.5f), 6.0f, 1.0f);

        if (!sidechainConnected)
        {
            drawCentredMessage(g, bounds, "Sin sidechain conectado");
            return;
        }

        if (!hasReceivedData)
        {
            drawCentredMessage(g, bounds, "Analizando...");
            return;
        }

        drawSpectrum(g, bounds.reduced(6.0f));
    }

private:
    static void drawCentredMessage(juce::Graphics& g, juce::Rectangle<float> bounds, const juce::String& text)
    {
        g.setColour(juce::Colours::grey.withAlpha(0.6f));
        g.setFont(juce::FontOptions(13.0f));
        g.drawText(text, bounds, juce::Justification::centred);
    }

    /** Número de bandas del frame, para referencia local. */
    static constexpr int numBands = SpectrumDataFifo::numBands;

    void drawSpectrum(juce::Graphics& g, juce::Rectangle<float> outerArea)
    {
        constexpr float labelStripHeight = 14.0f;
        auto plotArea = outerArea;
        auto labelArea = plotArea.removeFromBottom(labelStripHeight);

        const float minHz = latestFrame.bandCentreHz.front();
        const float maxHz = latestFrame.bandCentreHz.back();

        // Normaliza contra el pico del frame actual (entre ambas
        // señales) para que la visualización se adapte al nivel de
        // entrada en vez de asumir una escala fija de dBFS.
        float maxEnergy = 1.0e-9f;
        for (int b = 0; b < numBands; ++b)
        {
            maxEnergy = std::max(maxEnergy, latestFrame.mainEnergies[(size_t) b]);
            maxEnergy = std::max(maxEnergy, latestFrame.sidechainEnergies[(size_t) b]);
        }

        constexpr float floorDb = -50.0f;
        auto energyToHeight01 = [maxEnergy, floorDb](float energy) -> float
        {
            float db = 10.0f * std::log10(std::max(energy, 1.0e-12f) / maxEnergy);
            db = std::clamp(db, floorDb, 0.0f);
            return (db - floorDb) / -floorDb; // 0..1
        };

        // Posición X (log-frecuencia) de cada banda, y los bordes entre
        // bandas contiguas (punto medio en X), usados para las zonas de
        // conflicto resaltadas — así cada resaltado ocupa exactamente el
        // ancho real de su banda en la escala logarítmica, no un ancho
        // uniforme artificial.
        std::array<float, (size_t) numBands> bandX {};
        for (int b = 0; b < numBands; ++b)
            bandX[(size_t) b] = freqToX(latestFrame.bandCentreHz[(size_t) b], minHz, maxHz, plotArea.getWidth());

        std::array<float, (size_t) (numBands + 1)> bandEdgesX {};
        bandEdgesX[0] = 0.0f;
        bandEdgesX[(size_t) numBands] = plotArea.getWidth();
        for (int b = 1; b < numBands; ++b)
            bandEdgesX[(size_t) b] = 0.5f * (bandX[(size_t) (b - 1)] + bandX[(size_t) b]);

        drawFrequencyGrid(g, plotArea, labelArea, minHz, maxHz);

        // ---- Zonas de conflicto resaltadas (detrás de todo) ----
        for (int b = 0; b < numBands; ++b)
        {
            const float conflict = latestFrame.conflict[(size_t) b];
            if (conflict <= 0.05f)
                continue;

            const float left = plotArea.getX() + bandEdgesX[(size_t) b];
            const float right = plotArea.getX() + bandEdgesX[(size_t) (b + 1)];

            auto bandRect = juce::Rectangle<float>(left, plotArea.getY(), right - left, plotArea.getHeight());

            g.setColour(kAccentColour.withAlpha(std::clamp(conflict * 0.4f, 0.05f, 0.35f)));
            g.fillRect(bandRect);
        }

        // ---- Sidechain: línea sutil ----
        g.setColour(juce::Colours::grey.withAlpha(0.5f));
        g.strokePath(buildPath(latestFrame.sidechainEnergies, bandX, plotArea, energyToHeight01),
                     juce::PathStrokeType(1.4f));

        // ---- Main: línea sólida, en primer plano ----
        g.setColour(juce::Colour(0xffe8e8ea));
        g.strokePath(buildPath(latestFrame.mainEnergies, bandX, plotArea, energyToHeight01),
                     juce::PathStrokeType(2.0f));
    }

    /** Mapea una frecuencia (Hz) a una posición X en escala logarítmica
        dentro de un ancho dado, acotada a [minHz, maxHz]. */
    static float freqToX(float hz, float minHz, float maxHz, float widthPx)
    {
        if (maxHz <= minHz)
            return 0.0f;

        const float logMin = std::log10(minHz);
        const float logMax = std::log10(maxHz);
        const float logHz = std::log10(std::clamp(hz, minHz, maxHz));
        return widthPx * (logHz - logMin) / (logMax - logMin);
    }

    /** Dibuja las líneas de rejilla verticales y sus etiquetas de
        frecuencia (50, 100, 200, 500, 1k, 2k, 5k, 10k, 20k...),
        omitiendo las que caen fuera del rango [minHz, maxHz] que
        cubren las bandas analizadas a este sample rate. */
    /** Formatea una etiqueta de frecuencia legible: "500", "1k", "2k",
        "10k"... sin decimales sobrantes cuando el valor es entero. */
    static juce::String formatFrequencyLabel(float hz)
    {
        if (hz < 1000.0f)
            return juce::String((int) hz);

        const float k = hz / 1000.0f;
        const bool isWhole = std::abs(k - std::round(k)) < 0.01f;
        return (isWhole ? juce::String((int) std::round(k)) : juce::String(k, 1)) + "k";
    }

    static void drawFrequencyGrid(juce::Graphics& g, juce::Rectangle<float> plotArea,
                                   juce::Rectangle<float> labelArea, float minHz, float maxHz)
    {
        static const std::array<float, 10> ticks { 50.0f, 100.0f, 200.0f, 500.0f, 1000.0f,
                                                     2000.0f, 5000.0f, 10000.0f, 20000.0f, 40000.0f };

        g.setFont(juce::FontOptions(9.5f));

        for (float hz : ticks)
        {
            if (hz < minHz || hz > maxHz)
                continue;

            const float x = plotArea.getX() + freqToX(hz, minHz, maxHz, plotArea.getWidth());

            g.setColour(juce::Colours::white.withAlpha(0.08f));
            g.drawVerticalLine((int) std::round(x), plotArea.getY(), plotArea.getBottom());

            g.setColour(juce::Colours::grey.withAlpha(0.7f));
            const juce::String label = formatFrequencyLabel(hz);

            constexpr float labelWidth = 28.0f;
            g.drawText(label,
                       juce::Rectangle<float>(x - labelWidth * 0.5f, labelArea.getY(), labelWidth, labelArea.getHeight()),
                       juce::Justification::centred);
        }
    }

    template <typename EnergyToHeightFn>
    static juce::Path buildPath(const std::array<float, (size_t) numBands>& energies,
                                 const std::array<float, (size_t) numBands>& bandX,
                                 juce::Rectangle<float> plotArea,
                                 EnergyToHeightFn energyToHeight01)
    {
        juce::Path path;

        for (int b = 0; b < numBands; ++b)
        {
            const float h01 = energyToHeight01(energies[(size_t) b]);
            const float x = plotArea.getX() + bandX[(size_t) b];
            const float y = plotArea.getBottom() - h01 * plotArea.getHeight();

            if (b == 0)
                path.startNewSubPath(x, y);
            else
                path.lineTo(x, y);
        }

        return path;
    }

    static inline const juce::Colour kAccentColour { 0xff4fd8e0 };

    SpectrumDataFifo::Frame latestFrame {};
    bool hasReceivedData = false;
    bool sidechainConnected = false;
};
