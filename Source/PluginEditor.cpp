#include "PluginEditor.h"

namespace
{
    const juce::Colour kBackgroundTop    { 0xff1e1e22 };
    const juce::Colour kBackgroundBottom { 0xff0d0d0f };
    const juce::Colour kPanelTop         { 0xff232327 };
    const juce::Colour kPanelBottom      { 0xff18181b };
    const juce::Colour kAccent           { 0xff4fd8e0 }; // cian, color de acento
    const juce::Colour kTextPrimary      { 0xffe8e8ea };
    const juce::Colour kTextMuted        { 0xff8a8a90 };
}

//==============================================================================
ClearSpaceEditor::ClearSpaceEditor(ClearSpaceProcessor& p)
    : AudioProcessorEditor(&p), processorRef(p)
{
    setLookAndFeel(&lookAndFeel);

    // ---- Carve: knob grande y central ----
    carveSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    carveSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 80, 20);
    carveSlider.setColour(juce::Slider::rotarySliderFillColourId, kAccent);
    addAndMakeVisible(carveSlider);

    carveLabel.setText("CARVE", juce::dontSendNotification);
    carveLabel.setJustificationType(juce::Justification::centred);
    carveLabel.setColour(juce::Label::textColourId, kTextPrimary);
    carveLabel.setFont(juce::FontOptions(15.0f, juce::Font::bold));
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
    smoothnessSlider.setColour(juce::Slider::rotarySliderFillColourId, kTextMuted.brighter(0.3f));
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
    listenButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2f));
    listenButton.setColour(juce::TextButton::buttonOnColourId, kAccent);
    addAndMakeVisible(listenButton);

    listenAttachment = std::make_unique<ButtonAttachment>(
        processorRef.apvts, ClearSpaceParams::listenID, listenButton);

    // ---- Bypass (ahora en la barra superior, ver mockup original) ----
    bypassButton.setClickingTogglesState(true);
    bypassButton.setColour(juce::TextButton::buttonColourId, juce::Colour(0xff2a2a2f));
    bypassButton.setColour(juce::TextButton::buttonOnColourId, juce::Colours::orange);
    addAndMakeVisible(bypassButton);

    bypassAttachment = std::make_unique<ButtonAttachment>(
        processorRef.apvts, ClearSpaceParams::bypassID, bypassButton);

    // ---- Output gain ----
    outputGainSlider.setSliderStyle(juce::Slider::RotaryHorizontalVerticalDrag);
    outputGainSlider.setTextBoxStyle(juce::Slider::TextBoxBelow, false, 60, 18);
    outputGainSlider.setColour(juce::Slider::rotarySliderFillColourId, kTextMuted.brighter(0.3f));
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

    setSize(680, 560);
    startTimerHz(30);
}

ClearSpaceEditor::~ClearSpaceEditor()
{
    stopTimer();
    setLookAndFeel(nullptr);
}

//==============================================================================
void ClearSpaceEditor::paint(juce::Graphics& g)
{
    auto bounds = getLocalBounds().toFloat();

    // Fondo con gradiente sutil (profundidad en vez de un plano uniforme)
    juce::ColourGradient bgGradient(kBackgroundTop, bounds.getX(), bounds.getY(),
                                     kBackgroundBottom, bounds.getX(), bounds.getBottom(), false);
    g.setGradientFill(bgGradient);
    g.fillAll();

    g.setColour(kTextPrimary);
    g.setFont(juce::FontOptions(19.0f, juce::Font::bold));
    g.drawText("ClearSpace", titleTextArea, juce::Justification::centredLeft);

    // ---- Panel de controles, con relieve (no plano) ----
    juce::ColourGradient panelGradient(kPanelTop, panelBounds.getX(), panelBounds.getY(),
                                        kPanelBottom, panelBounds.getX(), panelBounds.getBottom(), false);
    g.setGradientFill(panelGradient);
    g.fillRoundedRectangle(panelBounds, 10.0f);

    g.setColour(juce::Colours::black.withAlpha(0.5f));
    g.drawRoundedRectangle(panelBounds, 10.0f, 1.5f);
    g.setColour(juce::Colours::white.withAlpha(0.05f));
    g.drawRoundedRectangle(panelBounds.reduced(1.5f), 9.0f, 1.0f);

    // ---- Medidor de reducción de ganancia ----
    g.setColour(kTextMuted);
    g.setFont(juce::FontOptions(10.0f, juce::Font::bold));
    g.drawText("GAIN REDUCTION", grMeterArea.translated(0.0f, -14.0f), juce::Justification::centred);

    g.setColour(juce::Colours::black.withAlpha(0.45f));
    g.fillRoundedRectangle(grMeterArea, 3.0f);

    constexpr float maxGrDb = 18.0f;
    const float fraction = juce::jlimit(0.0f, 1.0f, displayedGainReductionDb / maxGrDb);
    if (fraction > 0.0f)
    {
        auto fillArea = grMeterArea.withWidth(grMeterArea.getWidth() * fraction);
        juce::ColourGradient meterGradient(kAccent, fillArea.getX(), 0.0f,
                                            kAccent.darker(0.3f), fillArea.getRight(), 0.0f, false);
        g.setGradientFill(meterGradient);
        g.fillRoundedRectangle(fillArea, 3.0f);
    }
    g.setColour(juce::Colours::white.withAlpha(0.08f));
    g.drawRoundedRectangle(grMeterArea, 3.0f, 1.0f);
}

void ClearSpaceEditor::resized()
{
    constexpr int margin = 16;
    auto bounds = getLocalBounds().reduced(margin);

    // ---- Barra superior: título + Bypass ----
    // (Bypass vive aquí, aislado de todo lo demás, para que nunca pueda
    // solaparse con otro control por más que cambie el resto del layout.)
    auto titleBar = bounds.removeFromTop(36);
    bypassButton.setBounds(titleBar.removeFromRight(90));
    titleTextArea = titleBar;

    bounds.removeFromTop(10);

    // ---- Spectrum display, ancho completo ----
    spectrumDisplay.setBounds(bounds.removeFromTop(150));

    bounds.removeFromTop(14);

    // ---- Panel de controles: todo lo que queda del alto de la ventana ----
    panelBounds = bounds.toFloat();
    auto panelInner = bounds.reduced(14);

    auto row1 = panelInner.removeFromTop(170); // Mode | Carve | Smoothness
    panelInner.removeFromTop(12);              // separación entre filas
    auto row2 = panelInner;                    // Listen | GR meter | Output (resto del alto)

    // -- Row 1: tres columnas de igual ancho --
    const int colWidth = row1.getWidth() / 3;
    auto modeCol = row1.removeFromLeft(colWidth);
    auto carveCol = row1.removeFromLeft(colWidth);
    auto smoothCol = row1; // resto (absorbe cualquier redondeo de la división)

    auto modeGroup = modeCol.withSizeKeepingCentre(juce::jmin(140, modeCol.getWidth() - 10), 60);
    modeLabel.setBounds(modeGroup.removeFromTop(18));
    modeSelector.setBounds(modeGroup.reduced(6, 2));

    auto carveGroup = carveCol.reduced(8);
    carveLabel.setBounds(carveGroup.removeFromTop(22));
    carveSlider.setBounds(carveGroup);

    auto smoothGroup = smoothCol.withSizeKeepingCentre(juce::jmin(140, smoothCol.getWidth() - 10), 120);
    smoothnessLabel.setBounds(smoothGroup.removeFromTop(18));
    smoothnessSlider.setBounds(smoothGroup.reduced(10));

    // -- Row 2: columnas laterales de ancho fijo, medidor flexible al centro --
    auto listenCol = row2.removeFromLeft(120);
    auto outputCol = row2.removeFromRight(120);
    auto meterCol = row2;

    listenButton.setBounds(listenCol.withSizeKeepingCentre(96, 36));

    auto outGroup = outputCol.withSizeKeepingCentre(90, 78);
    outputGainLabel.setBounds(outGroup.removeFromTop(16));
    outputGainSlider.setBounds(outGroup.reduced(4));

    grMeterArea = meterCol.withSizeKeepingCentre(juce::jmin(meterCol.getWidth() - 20, 260), 14).toFloat();
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
