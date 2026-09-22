#pragma once
#include <juce_audio_processors/juce_audio_processors.h>

namespace ClearSpaceParams
{
    // ---- Parameter IDs ----
    static constexpr auto carveID       = "carve";
    static constexpr auto modeID        = "mode";
    static constexpr auto smoothnessID  = "smoothness";
    static constexpr auto listenID      = "listen";
    static constexpr auto bypassID      = "bypass";
    static constexpr auto outputGainID  = "outputGain";

    enum class Mode
    {
        Dynamic = 0,
        Static  = 1
    };

    inline juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
    {
        using namespace juce;
        std::vector<std::unique_ptr<RangedAudioParameter>> params;

        // Carve: control principal, 0-100%
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { carveID, 1 },
            "Carve",
            NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            50.0f,
            AudioParameterFloatAttributes().withLabel("%")));

        // Mode: Dynamic / Static
        params.push_back(std::make_unique<AudioParameterChoice>(
            ParameterID { modeID, 1 },
            "Mode",
            StringArray { "Dynamic", "Static" },
            0));

        // Smoothness: mapea a constantes de tiempo attack/release (5-200 ms)
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { smoothnessID, 1 },
            "Smoothness",
            NormalisableRange<float>(0.0f, 100.0f, 0.1f),
            40.0f,
            AudioParameterFloatAttributes().withLabel("%")));

        // Listen: escuchar solo la diferencia (lo que se está quitando)
        params.push_back(std::make_unique<AudioParameterBool>(
            ParameterID { listenID, 1 },
            "Listen",
            false));

        // Bypass
        params.push_back(std::make_unique<AudioParameterBool>(
            ParameterID { bypassID, 1 },
            "Bypass",
            false));

        // Output gain de compensación, +/- 12 dB
        params.push_back(std::make_unique<AudioParameterFloat>(
            ParameterID { outputGainID, 1 },
            "Output Gain",
            NormalisableRange<float>(-12.0f, 12.0f, 0.1f),
            0.0f,
            AudioParameterFloatAttributes().withLabel("dB")));

        return { params.begin(), params.end() };
    }
}
