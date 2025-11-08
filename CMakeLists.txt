cmake_minimum_required(VERSION 3.15)

project(GranularVerbDelay VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# macOS specific settings for Sequoia (macOS 15) compatibility
if(APPLE)
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.13" CACHE STRING "Minimum macOS deployment version")
    # Optional: Set specific architecture (arm64 for M1/M2/M3, x86_64 for Intel, or "arm64;x86_64" for Universal)
    # set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "macOS architectures")
endif()

# Add JUCE as a subdirectory (you'll need to clone JUCE into this directory)
# Or use FetchContent to download it automatically
include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG 7.0.9
)
FetchContent_MakeAvailable(JUCE)

# Platform-specific plugin formats
if(APPLE)
    set(PLUGIN_FORMATS AU VST3 Standalone)
elseif(WIN32)
    set(PLUGIN_FORMATS VST3 Standalone)
else()
    set(PLUGIN_FORMATS VST3 Standalone)
endif()

# Add your plugin target
juce_add_plugin(GranularVerbDelay
    COMPANY_NAME "GranularAudio"
    COMPANY_WEBSITE "https://github.com/ifeelvoid/granularverbdelay"
    COMPANY_EMAIL "support@granularaudio.dev"
    BUNDLE_ID "dev.granularaudio.GranularVerbDelay"

    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE
    COPY_PLUGIN_AFTER_BUILD TRUE

    PLUGIN_MANUFACTURER_CODE Grau
    PLUGIN_CODE GvDl

    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "Granular Verb Delay"

    # macOS specific settings
    AU_MAIN_TYPE kAudioUnitType_Effect

    # VST3 category
    VST3_CATEGORIES "Fx" "Delay" "Reverb"

    # Description
    DESCRIPTION "Granular delay and reverb effect with stereo width control"

    # For macOS code signing (optional but reduces warnings)
    # Set to empty string or "ad-hoc" for local builds
    # Set to your Developer ID for distribution
    # HARDENED_RUNTIME_ENABLED TRUE
    # HARDENED_RUNTIME_OPTIONS "com.apple.security.device.audio-input"
)

# Add source files
target_sources(GranularVerbDelay
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
)

# Link required JUCE modules
target_compile_definitions(GranularVerbDelay
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
)

target_link_libraries(GranularVerbDelay
    PRIVATE
        juce::juce_audio_utils
        juce::juce_dsp
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)
