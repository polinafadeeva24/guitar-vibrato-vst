cmake_minimum_required(VERSION 3.22)
project(GuitarVibrato VERSION 0.1.0)

include(FetchContent)
FetchContent_Declare(JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 8.0.4
    GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(JUCE)

# Плагин собирается как "инструмент" с MIDI-входом и MIDI-выходом:
# так FL Studio позволяет направить его MIDI-выход в другой плагин.
juce_add_plugin(GuitarVibrato
    COMPANY_NAME "Home"
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    COPY_PLUGIN_AFTER_BUILD FALSE
    PLUGIN_MANUFACTURER_CODE Home
    PLUGIN_CODE Gvib
    FORMATS VST3
    PRODUCT_NAME "Guitar Vibrato")

target_sources(GuitarVibrato PRIVATE
    PluginProcessor.cpp
    PluginProcessor.h
    PluginEditor.h)

target_compile_features(GuitarVibrato PRIVATE cxx_std_17)
target_compile_options(GuitarVibrato PRIVATE $<$<CXX_COMPILER_ID:MSVC>:/utf-8>)
target_compile_definitions(GuitarVibrato PUBLIC
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_VST3_CAN_REPLACE_VST2=0)

target_link_libraries(GuitarVibrato
    PRIVATE juce::juce_audio_utils
    PUBLIC juce::juce_recommended_config_flags
           juce::juce_recommended_lto_flags
           juce::juce_recommended_warning_flags)
