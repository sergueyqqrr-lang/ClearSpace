#include "ClearSpaceLookAndFeel.h"

namespace
{
    const juce::Colour kKnobLight  { 0xff4a4a52 };
    const juce::Colour kKnobDark   { 0xff1c1c1f };
    const juce::Colour kBezelEdge  { 0xff2a2a2f };
    const juce::Colour kTrackDark  { 0xff2a2a2f };
    const juce::Colour kPointer    { 0xfff0f0f2 };
    const juce::Colour kShadow     { 0xff000000 };
}

ClearSpaceLookAndFeel::ClearSpaceLookAndFeel()
{
    setColour(juce::Slider::textBoxTextColourId, juce::Colour(0xffe8e8ea));
    setColour(juce::Slider::textBoxBackgroundColourId, juce::Colours::transparentBlack);
    setColour(juce::Slider::textBoxOutlineColourId, juce::Colours::transparentBlack);

    setColour(juce::ComboBox::backgroundColourId, juce::Colour(0xff232327));
    setColour(juce::ComboBox::outlineColourId, juce::Colours::transparentBlack);
    setColour(juce::ComboBox::textColourId, juce::Colour(0xffe8e8ea));

    setColour(juce::PopupMenu::backgroundColourId, juce::Colour(0xff232327));
    setColour(juce::PopupMenu::textColourId, juce::Colour(0xffe8e8ea));
    setColour(juce::PopupMenu::highlightedBackgroundColourId, juce::Colour(0xff4fd8e0));
    setColour(juce::PopupMenu::highlightedTextColourId, juce::Colour(0xff0d0d0f));

    setColour(juce::TextButton::textColourOnId, juce::Colour(0xff0d0d0f));
    setColour(juce::TextButton::textColourOffId, juce::Colour(0xffe8e8ea));
}

//==============================================================================
void ClearSpaceLookAndFeel::drawRotarySlider(juce::Graphics& g, int x, int y, int width, int height,
                                              float sliderPos, float rotaryStartAngle, float rotaryEndAngle,
                                              juce::Slider& slider)
{
    auto bounds = juce::Rectangle<float>((float) x, (float) y, (float) width, (float) height);
    const float diameter = juce::jmin(bounds.getWidth(), bounds.getHeight()) - 4.0f;
    auto knobBounds = bounds.withSizeKeepingCentre(diameter, diameter);
    const float radius = diameter * 0.5f;
    const auto centre = knobBounds.getCentre();

    const float angle = rotaryStartAngle + sliderPos * (rotaryEndAngle - rotaryStartAngle);
    const float trackThickness = juce::jmax(3.0f, radius * 0.14f);
    const float trackRadius = radius - trackThickness * 0.5f - 2.0f;

    // ---- 1. Track de fondo (arco completo, apagado) ----
    juce::Path backgroundTrack;
    backgroundTrack.addCentredArc(centre.x, centre.y, trackRadius, trackRadius,
                                   0.0f, rotaryStartAngle, rotaryEndAngle, true);
    g.setColour(kTrackDark);
    g.strokePath(backgroundTrack,
                 juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));

    // ---- 2. Track de valor (color de acento del slider) ----
    const auto valueColour = slider.findColour(juce::Slider::rotarySliderFillColourId);
    if (angle > rotaryStartAngle + 0.001f)
    {
        juce::Path valueTrack;
        valueTrack.addCentredArc(centre.x, centre.y, trackRadius, trackRadius,
                                  0.0f, rotaryStartAngle, angle, true);
        g.setColour(valueColour);
        g.strokePath(valueTrack,
                     juce::PathStrokeType(trackThickness, juce::PathStrokeType::curved, juce::PathStrokeType::rounded));
    }

    // ---- 3. Sombra proyectada debajo del cuerpo (simula elevación) ----
    auto bodyBounds = knobBounds.reduced(trackThickness + 4.0f);
    g.setColour(kShadow.withAlpha(0.35f));
    g.fillEllipse(bodyBounds.translated(0.0f, 2.5f));

    // ---- 4. Cuerpo del knob: gradiente radial simulando una esfera/domo ----
    juce::ColourGradient bodyGradient(kKnobLight,
                                       bodyBounds.getX() + bodyBounds.getWidth() * 0.32f,
                                       bodyBounds.getY() + bodyBounds.getHeight() * 0.26f,
                                       kKnobDark,
                                       bodyBounds.getCentreX(), bodyBounds.getBottom(),
                                       true);
    g.setGradientFill(bodyGradient);
    g.fillEllipse(bodyBounds);

    g.setColour(kBezelEdge.withAlpha(0.7f));
    g.drawEllipse(bodyBounds, 1.0f);

    // ---- 5. Highlight especular (arriba-izquierda), da sensación de luz real ----
    auto highlightBounds = bodyBounds.reduced(bodyBounds.getWidth() * 0.22f)
                                      .translated(-bodyBounds.getWidth() * 0.06f, -bodyBounds.getHeight() * 0.14f);
    juce::ColourGradient highlightGradient(juce::Colours::white.withAlpha(0.20f),
                                            highlightBounds.getCentreX(), highlightBounds.getY(),
                                            juce::Colours::white.withAlpha(0.0f),
                                            highlightBounds.getCentreX(), highlightBounds.getBottom(),
                                            false);
    g.setGradientFill(highlightGradient);
    g.fillEllipse(highlightBounds);

    // ---- 6. Puntero que indica la posición actual ----
    const float pointerLength = bodyBounds.getHeight() * 0.36f;
    const float pointerThickness = juce::jmax(2.5f, bodyBounds.getHeight() * 0.05f);

    juce::Path pointer;
    pointer.addRoundedRectangle(-pointerThickness * 0.5f, -pointerLength,
                                 pointerThickness, pointerLength * 0.85f, pointerThickness * 0.5f);

    // Sombra sutil del puntero, dibujada primero (debajo)
    g.setColour(kShadow.withAlpha(0.3f));
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre.x + 1.0f, centre.y + 1.0f));

    // Puntero real, encima
    g.setColour(kPointer);
    g.fillPath(pointer, juce::AffineTransform::rotation(angle).translated(centre));
}

//==============================================================================
void ClearSpaceLookAndFeel::drawButtonBackground(juce::Graphics& g, juce::Button& button,
                                                  const juce::Colour& backgroundColour,
                                                  bool shouldDrawButtonAsHighlighted,
                                                  bool shouldDrawButtonAsDown)
{
    auto bounds = button.getLocalBounds().toFloat().reduced(1.0f);
    const float cornerRadius = juce::jmin(8.0f, bounds.getHeight() * 0.28f);

    const bool isToggledOn = button.getToggleState();
    const bool pressedLook = shouldDrawButtonAsDown || isToggledOn;

    // Sombra proyectada solo cuando el botón está "levantado" (no presionado),
    // para reforzar la sensación de profundidad.
    if (!pressedLook)
    {
        g.setColour(juce::Colours::black.withAlpha(0.35f));
        g.fillRoundedRectangle(bounds.translated(0.0f, 1.5f), cornerRadius);
    }

    const auto baseColour = isToggledOn ? button.findColour(juce::TextButton::buttonOnColourId)
                                         : backgroundColour;

    // Gradiente vertical: cara "levantada" (claro arriba) o "hundida" (oscuro arriba)
    const juce::Colour top = pressedLook ? baseColour.darker(0.35f) : baseColour.brighter(0.18f);
    const juce::Colour bottom = pressedLook ? baseColour.brighter(0.05f) : baseColour.darker(0.35f);

    juce::ColourGradient gradient(top, bounds.getX(), bounds.getY(),
                                   bottom, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);

    // Borde biselado: línea clara arriba/costados, más tenue si está presionado
    g.setColour(juce::Colours::white.withAlpha(pressedLook ? 0.06f : 0.16f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

    if (shouldDrawButtonAsHighlighted && !pressedLook)
    {
        g.setColour(juce::Colours::white.withAlpha(0.06f));
        g.fillRoundedRectangle(bounds, cornerRadius);
    }
}

//==============================================================================
void ClearSpaceLookAndFeel::drawComboBox(juce::Graphics& g, int width, int height, bool /*isButtonDown*/,
                                          int /*buttonX*/, int /*buttonY*/, int /*buttonW*/, int /*buttonH*/,
                                          juce::ComboBox& box)
{
    auto bounds = juce::Rectangle<float>(0.0f, 0.0f, (float) width, (float) height).reduced(1.0f);
    const float cornerRadius = 6.0f;

    g.setColour(juce::Colours::black.withAlpha(0.3f));
    g.fillRoundedRectangle(bounds.translated(0.0f, 1.0f), cornerRadius);

    const auto baseColour = box.findColour(juce::ComboBox::backgroundColourId);
    juce::ColourGradient gradient(baseColour.brighter(0.08f), bounds.getX(), bounds.getY(),
                                   baseColour.darker(0.25f), bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(gradient);
    g.fillRoundedRectangle(bounds, cornerRadius);

    g.setColour(juce::Colours::white.withAlpha(0.12f));
    g.drawRoundedRectangle(bounds.reduced(0.5f), cornerRadius, 1.0f);

    // Flecha del dropdown
    auto arrowZone = juce::Rectangle<float>((float) width - 24.0f, 0.0f, 20.0f, (float) height);
    const auto c = arrowZone.getCentre();
    constexpr float arrowSize = 5.0f;

    juce::Path arrow;
    arrow.addTriangle(c.x - arrowSize, c.y - arrowSize * 0.5f,
                       c.x + arrowSize, c.y - arrowSize * 0.5f,
                       c.x, c.y + arrowSize * 0.6f);
    g.setColour(juce::Colour(0xff8a8a90));
    g.fillPath(arrow);
}

//==============================================================================
juce::Font ClearSpaceLookAndFeel::getComboBoxFont(juce::ComboBox&)
{
    return juce::FontOptions(13.0f);
}

juce::Font ClearSpaceLookAndFeel::getLabelFont(juce::Label&)
{
    return juce::FontOptions(12.0f);
}
