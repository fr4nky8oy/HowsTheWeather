cmake_minimum_required(VERSION 3.22)

project(HowsTheWeather VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# JUCE path configuration
set(JUCE_PATH "/Users/frankyredente/JUCE" CACHE PATH "Path to JUCE framework")

# Add JUCE
add_subdirectory(${JUCE_PATH} ${CMAKE_BINARY_DIR}/JUCE)

# Plugin formats - enable all by default
option(BUILD_VST3 "Build VST3 plugin" ON)
option(BUILD_AU "Build AU plugin" ON)
option(BUILD_STANDALONE "Build standalone application" ON)

# Set plugin formats based on options
set(PLUGIN_FORMATS "")
if(BUILD_VST3)
    list(APPEND PLUGIN_FORMATS VST3)
endif()
if(BUILD_AU AND APPLE)
    list(APPEND PLUGIN_FORMATS AU)
endif()
if(BUILD_STANDALONE)
    list(APPEND PLUGIN_FORMATS Standalone)
endif()

# Plugin configuration
juce_add_plugin(${PROJECT_NAME}
    COMPANY_NAME "Franky"
    PLUGIN_MANUFACTURER_CODE XA97
    PLUGIN_CODE XD6E
    FORMATS ${PLUGIN_FORMATS}
    PRODUCT_NAME "HowsTheWeather"

    # Plugin characteristics — set by create_agent_plugin.sh
    IS_SYNTH FALSE
    NEEDS_MIDI_INPUT FALSE
    NEEDS_MIDI_OUTPUT FALSE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS FALSE

    # Copy plugin to system folders after build
    COPY_PLUGIN_AFTER_BUILD TRUE
    VST3_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/VST3"
    AU_COPY_DIR "$ENV{HOME}/Library/Audio/Plug-Ins/Components"

    # WebView support (set to TRUE if GUI Mode = WebView)
    NEEDS_WEB_BROWSER TRUE
)

# Source files — Build agent updates this list as DSP/GUI agents add files
target_sources(${PROJECT_NAME}
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginEditor.cpp
        Source/WebViewEditor.cpp
        Source/PresetManager.cpp
)

# Compile definitions
target_compile_definitions(${PROJECT_NAME}
    PUBLIC
        JUCE_WEB_BROWSER=1
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_REPORT_APP_USAGE=0
)

# ============================================================================
# WebView UI Resources (only for WebView GUI mode)
# Uncomment the following block if GUI Mode = WebView
# ============================================================================
juce_add_binary_data(${PROJECT_NAME}_UIResources
    SOURCES
        Source/ui/public/index.html
        Source/ui/public/js/cobe.js
        Source/ui/public/js/juce/index.js
        Source/ui/public/js/juce/check_native_interop.js
)
# ============================================================================

# Link JUCE modules
target_link_libraries(${PROJECT_NAME}
    PRIVATE
        ${PROJECT_NAME}_UIResources
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_plugin_client
        juce::juce_audio_processors
        juce::juce_audio_utils
        juce::juce_core
        juce::juce_data_structures
        juce::juce_dsp
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

# Generate JuceHeader.h (MUST come after target_link_libraries)
juce_generate_juce_header(${PROJECT_NAME})

# Architecture configuration
if(APPLE)
    # Default to host architecture for local builds
    if(NOT DEFINED CMAKE_OSX_ARCHITECTURES)
        set(CMAKE_OSX_ARCHITECTURES "${CMAKE_SYSTEM_PROCESSOR}" CACHE STRING "Build architectures for Mac OS X" FORCE)
    endif()

    # For CI builds with v* tags, build universal binary
    if(DEFINED ENV{CI} AND DEFINED ENV{GITHUB_REF} AND "$ENV{GITHUB_REF}" MATCHES "refs/tags/v.*")
        set(CMAKE_OSX_ARCHITECTURES "arm64;x86_64" CACHE STRING "Build universal binary for release" FORCE)
    endif()
endif()

# Enable testing
enable_testing()

# Add tests subdirectory if it exists
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/tests")
    add_subdirectory(tests)
endif()
