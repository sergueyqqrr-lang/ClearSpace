#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/SpectrumDisplay.h"
#include "UI/ClearSpaceLookAndFeel.h"

//==============================================================================
class ClearSpaceEditor : public juce::AudioProcessorEditor,
                          private juce::Timer
{
public:
    explicit ClearSpaceEditor(ClearSpaceProcessor&);
    ~ClearSpaceEditor() override;

    void paint(juce::Graphics&) override;
    void resized() override;

private:
    void timerCallback() override;

    ClearSpaceProcessor& processorRef;

    // Declarado primero para que se destruya al final (después de todos
    // los componentes que lo usan mientras existen).
    ClearSpaceLookAndFeel lookAndFeel;

    SpectrumDisplay spectrumDisplay;

    juce::Slider carveSlider;
    juce::Label carveLabel;

    juce::ComboBox modeSelector;
    juce::Label modeLabel;

    juce::Slider smoothnessSlider;
    juce::Label smoothnessLabel;

    juce::TextButton listenButton { "LISTEN" };
    juce::TextButton bypassButton { "BYPASS" };

    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;

    float displayedGainReductionDb = 0.0f;

    // Bounds calculados en resized() y reutilizados en paint(), para que
    // el fondo/panel/medidor siempre coincidan exactamente con el layout
    // real en vez de usar coordenadas fijas que se pueden desincronizar.
    juce::Rectangle<int> titleTextArea;
    juce::Rectangle<float> panelBounds;
    juce::Rectangle<float> grMeterArea;

    using SliderAttachment = juce::AudioProcessorValueTreeState::SliderAttachment;
    using ComboBoxAttachment = juce::AudioProcessorValueTreeState::ComboBoxAttachment;
    using ButtonAttachment = juce::AudioProcessorValueTreeState::ButtonAttachment;

    std::unique_ptr<SliderAttachment> carveAttachment;
    std::unique_ptr<ComboBoxAttachment> modeAttachment;
    std::unique_ptr<SliderAttachment> smoothnessAttachment;
    std::unique_ptr<ButtonAttachment> listenAttachment;
    std::unique_ptr<ButtonAttachment> bypassAttachment;
    std::unique_ptr<SliderAttachment> outputGainAttachment;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ClearSpaceEditor)
};
