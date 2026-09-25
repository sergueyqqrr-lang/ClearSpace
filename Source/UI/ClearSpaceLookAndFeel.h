#pragma once

#include <juce_gui_basics/juce_gui_basics.h>

/**
    LookAndFeel personalizado de ClearSpace: knobs y controles con
    apariencia "3D" (bisel metálico, degradado tipo domo, sombra
    proyectada, brillo superior) en vez de los controles planos por
    defecto de JUCE, para transmitir "herramienta precisa y
    profesional" (ver sección 5 del diseño de producto).

    Se aplica una sola vez en ClearSpaceEditor (setLookAndFeel) y
    afecta a todos los Slider/Button/ComboBox del editor.
*/
class ClearSpaceLookAndFeel : public juce::LookAndFeel_V4
{
public:
    ClearSpaceLookAndFeel();

    void drawRotarySlider(juce::Graphics&, int x, int y, int width, int height,
                           float sliderPosProportional, float rotaryStartAngle,
                           float rotaryEndAngle, juce::Slider&) override;

    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour& backgroundColour,
                               bool shouldDrawButtonAsHighlighted, bool shouldDrawButtonAsDown) override;

    void drawComboBox(juce::Graphics&, int width, int height, bool isButtonDown,
                       int buttonX, int buttonY, int buttonW, int buttonH,
                       juce::ComboBox&) override;

    juce::Font getComboBoxFont(juce::ComboBox&) override;
    juce::Font getLabelFont(juce::Label&) override;
};
