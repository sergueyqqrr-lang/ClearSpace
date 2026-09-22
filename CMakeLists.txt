cmake_minimum_required(VERSION 3.22)

project(ClearSpace VERSION 0.1.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ------------------------------------------------------------------
# JUCE
# ------------------------------------------------------------------
# Opción 1 (recomendada): clona JUCE como submódulo en ./JUCE
#   git submodule add https://github.com/juce-framework/JUCE.git JUCE
# Opción 2: usa FetchContent (descomenta el bloque de abajo y borra
# add_subdirectory(JUCE) si no quieres el submódulo).

if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/JUCE/CMakeLists.txt")
    add_subdirectory(JUCE)
else()
    include(FetchContent)
    FetchContent_Declare(
        JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG 8.0.4
        GIT_SHALLOW TRUE
    )
    FetchContent_MakeAvailable(JUCE)
endif()

# ------------------------------------------------------------------
# Plugin target
# ------------------------------------------------------------------
juce_add_plugin(ClearSpace
    COMPANY_NAME "YourCompany"
    PLUGIN_MANUFACTURER_CODE Ycmp
    PLUGIN_CODE Cspc
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "ClearSpace"
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_CATEGORIES "Fx" "EQ"
    AU_MAIN_TYPE "kAudioUnitType_Effect"
)

juce_generate_juce_header(ClearSpace)

target_sources(ClearSpace
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/Parameters.h
        Source/PluginProcessor.h
        Source/PluginEditor.h
        Source/DSP/Constants.h
        Source/DSP/BandMapper.h
        Source/DSP/SpectralAnalyzer.h
        Source/DSP/MaskingDetector.h
        Source/DSP/SmoothingEnvelope.h
        Source/DSP/CarveCurveGenerator.h
        Source/DSP/DynamicFilterBank.h
        Source/DSP/SpectrumDataFifo.h
        Source/UI/SpectrumDisplay.h
)

target_compile_definitions(ClearSpace
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
)

target_link_libraries(ClearSpace
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
