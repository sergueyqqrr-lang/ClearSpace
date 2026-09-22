#include "PluginEditor.h"

namespace
{
    const juce::Colour kBackground   { 0xff121214 };
    const juce::Colour kPanel        { 0xff1a1a1e };
    const juce::Colour kAccent       { 0xff4fd8e0 }; // cian, color de acento
    const juce::Colour kTextPrimary  { 0xffe8e8ea };
    const juce::Colour kTextMuted    { 0xff8a8a90 };
}

//==============================================================================
ClearSpaceEditor::ClearSpaceEditor(ClearSpaceProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    // ---- Carve: knob grande y central ----
    carveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    carveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    carveSlider.setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    addAndMakeVisible(carveSlider);

    carveLabel.setText("CARVE", juce::dontSendNotification);
    carveLabel.setJustificationType(juce::Justification::centred);
    carveLabel.setColour(juce::Label::textColourId, kTextPrimary);
    carveLabel.setFont(juce::FontOptions(16.0f, juce::Font::bold));
    addAndMakeVisible(carveLabel);

    carveAttachment = std::make_unique<SliderAttachment>(
        processorRef.apvts, ClearSpaceParams::carveID, carveSlider);

    // ---- Mode selector ----
    modeSelector.addItem("Dynamic", 1);
    modeSelector.addItem("Static", 2);
    addAndMakeVisible(modeSelector);

    modeLabel.setText("MODE", juce::dontSendNotification);
    modeLabel.setJustificationType(juce::Justification::centred);
    modeLabel.setColour(juce::Label::textColourId, kTextMuted);
    modeLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(modeLabel);

    modeAttachment = std::make_unique<ComboBoxAttachment>(
        processorRef.apvts, ClearSpaceParams::modeID, modeSelector);

    // ---- Smoothness ----
    smoothnessSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    smoothnessSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    smoothnessSlider.setColour(juce::Slider::rotarySliderFillColourId, kTextMuted);
    addAndMakeVisible(smoothnessSlider);

    smoothnessLabel.setText("SMOOTHNESS", juce::dontSendNotification);
    smoothnessLabel.setJustificationType(juce::Justification::centred);
    smoothnessLabel.setColour(juce::Label::textColourId, kTextMuted);
    smoothnessLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(smoothnessLabel);

    smoothnessAttachment = std::make_unique<SliderAttachment>(
        processorRef.apvts, ClearSpaceParams::smoothnessID, smoothnessSlider);

    // ---- Listen ----
    listenButton.setClickingTogglesState(true);
    listenButton.setColour(juce::TextButton::buttonOnColourId, kAccent);
    addAndMakeVisible(listenButton);

    listenAttachment = std::make_unique<ButtonAttachment>(
        processorRef.apvts, ClearSpaceParams::listenID, listenButton);

    // ---- Bypass ----
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    addAndMakeVisible(bypassButton);

    bypassAttachment = std::make_unique<ButtonAttachment>(
        processorRef.apvts, ClearSpaceParams::bypassID, bypassButton);

    // ---- Output gain ----
    outputGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    outputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, kTextMuted);
    addAndMakeVisible(outputGainSlider);

    outputGainLabel.setText("OUTPUT", juce::dontSendNotification);
    outputGainLabel.setJustificationType(juce::Justification::centred);
    outputGainLabel.setColour(juce::Label::textColourId, kTextMuted);
    outputGainLabel.setFont(juce::FontOptions(11.0f));
    addAndMakeVisible(outputGainLabel);

    outputGainAttachment = std::make_unique<SliderAttachment>(
        processorRef.apvts, ClearSpaceParams::outputGainID, outputGainSlider);

    // ---- Spectrum display (main / sidechain / conflicto resaltado) ----
    addAndMakeVisible(spectrumDisplay);

    setSize(560, 420);
    startTimerHz(30);
}

ClearSpaceEditor::~ClearSpaceEditor()
{
    stopTimer();
}

//==============================================================================
void ClearSpaceEditor::paint(juce::Graphics& g)
{
    g.fillAll(kBackground);

    g.setColour(kTextPrimary);
    g.setFont(juce::FontOptions(18.0f, juce::Font::bold));
    g.drawText("ClearSpace", 16, 8, 200, 24, juce::Justification::left);

    // Panel de fondo para la sección de controles
    auto bounds = getLocalBounds().toFloat();
    auto controlsArea = bounds.withTop(200.0f).reduced(16.0f, 8.0f);
    g.setColour(kPanel);
    g.fillRoundedRectangle(controlsArea, 8.0f);

    // Medidor de reducción de ganancia (barra horizontal simple)
    auto meterArea = juce::Rectangle<float>(320.0f, 372.0f, 160.0f, 14.0f);
    g.setColour(juce::Colours::black.withAlpha(0.4f));
    g.fillRoundedRectangle(meterArea, 3.0f);

    const float maxGrDb = 12.0f;
    const float fraction = juce::jlimit(0.0f, 1.0f, displayedGainReductionDb / maxGrDb);
    auto fillArea = meterArea.withWidth(meterArea.getWidth() * fraction);
    g.setColour(kAccent);
    g.fillRoundedRectangle(fillArea, 3.0f);

    g.setColour(kTextMuted);
    g.setFont(juce::FontOptions(10.0f));
    g.drawText("GAIN REDUCTION", meterArea.translated(0, -14.0f), juce::Justification::left);
}

void ClearSpaceEditor::resized()
{
    auto area = getLocalBounds().reduced(16);
    area.removeFromTop(28); // espacio para el título

    // Spectrum display arriba, ancho completo
    spectrumDisplay.setBounds(area.removeFromTop(150));

    area.removeFromTop(24); // separación

    // Carve: knob grande centrado
    auto carveArea = area.removeFromLeft(200);
    carveLabel.setBounds(carveArea.removeFromTop(20));
    carveSlider.setBounds(carveArea.reduced(10));

    area.removeFromLeft(12);

    // Columna derecha: mode + smoothness
    auto rightColumn = area.removeFromLeft(160);
    auto modeArea = rightColumn.removeFromTop(50);
    modeLabel.setBounds(modeArea.removeFromTop(16));
    modeSelector.setBounds(modeArea.reduced(4));

    rightColumn.removeFromTop(10);

    auto smoothArea = rightColumn.removeFromTop(110);
    smoothnessLabel.setBounds(smoothArea.removeFromTop(16));
    smoothnessSlider.setBounds(smoothArea.reduced(20));

    area.removeFromLeft(12);

    // Columna final: output gain
    auto outColumn = area;
    auto outArea = outColumn.removeFromTop(110);
    outputGainLabel.setBounds(outArea.removeFromTop(16));
    outputGainSlider.setBounds(outArea.reduced(20));

    // Fila inferior: Listen + Bypass
    auto bottomRow = getLocalBounds().reduced(16).removeFromBottom(40);
    listenButton.setBounds(bottomRow.removeFromLeft(100));
    bottomRow.removeFromLeft(8);
    bypassButton.setBounds(bottomRow.removeFromLeft(100));
}

//==============================================================================
void ClearSpaceEditor::timerCallback()
{
    displayedGainReductionDb = processorRef.getGainReductionValue().load();

    spectrumDisplay.setSidechainConnected(processorRef.isSidechainConnected());

    SpectrumDataFifo::Frame frame;
    if (processorRef.getSpectrumDataFifo().popLatestFrame(frame))
        spectrumDisplay.setLatestFrame(frame);

    repaint();
}
