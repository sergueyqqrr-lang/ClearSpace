#pragma once

#include <juce_audio_processors/juce_audio_processors.h>
#include "PluginProcessor.h"
#include "UI/SpectrumDisplay.h"

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

    SpectrumDisplay spectrumDisplay;

    juce::Slider carveSlider;
    juce::Label carveLabel;

    juce::ComboBox modeSelector;
    juce::Label modeLabel;

    juce::Slider smoothnessSlider;
    juce::Label smoothnessLabel;

    juce::TextButton listenButton { "Listen" };
    juce::TextButton bypassButton { "Bypass" };

    juce::Slider outputGainSlider;
    juce::Label outputGainLabel;

    // Medidor de reducción de ganancia, dibujado a mano por simplicidad
    float displayedGainReductionDb = 0.0f;

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
