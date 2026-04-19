cmake_minimum_required(VERSION 3.22)

project(Drummr VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_OSX_DEPLOYMENT_TARGET "11.0" CACHE STRING "Minimum macOS")

include(FetchContent)
FetchContent_Declare(
    JUCE
    GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
    GIT_TAG        8.0.4
    GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(JUCE)

juce_add_gui_app(Drummr
    PRODUCT_NAME  "Drummr"
    BUNDLE_ID     "com.angihe.drummr"
    COMPANY_NAME  "angihe"
)

juce_generate_juce_header(Drummr)

target_sources(Drummr PRIVATE
    Source/Main.cpp
    Source/MainComponent.cpp
    Source/MainComponent.h
    Source/DrumSynth.cpp
    Source/DrumSynth.h
    Source/DrumInput.cpp
    Source/DrumInput.h
    Source/MidiPlayer.cpp
    Source/MidiPlayer.h
    Source/PitchedSynth.cpp
    Source/PitchedSynth.h
    Source/DrumNotesView.cpp
    Source/DrumNotesView.h
)

target_compile_definitions(Drummr PRIVATE
    JUCE_WEB_BROWSER=0
    JUCE_USE_CURL=0
    JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Drummr,JUCE_PRODUCT_NAME>"
    JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Drummr,JUCE_VERSION>"
)

target_link_libraries(Drummr PRIVATE
    juce::juce_gui_extra
    juce::juce_audio_utils
    juce::juce_audio_devices
    juce::juce_audio_formats
    juce::juce_audio_processors
    juce::juce_audio_basics
    juce::juce_core
    juce::juce_events
    juce::juce_graphics
    juce::juce_data_structures
    juce::juce_recommended_config_flags
    juce::juce_recommended_lto_flags
    juce::juce_recommended_warning_flags
)
