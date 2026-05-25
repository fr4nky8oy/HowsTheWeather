/*
  ==============================================================================

    PresetManager — Save/load/browse presets via APVTS state serialization
    Reusable across all plugins built from Franky_Agents_Template

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

class PresetManager
{
public:
    PresetManager (juce::AudioProcessorValueTreeState& apvts,
                   const juce::String& companyName,
                   const juce::String& pluginName);

    //==============================================================================
    void savePreset (const juce::String& presetName);
    void loadPreset (const juce::String& presetName);
    void loadPresetFromFile (const juce::File& file);

    //==============================================================================
    void loadNextPreset();
    void loadPreviousPreset();

    //==============================================================================
    juce::StringArray getAllPresets() const;
    juce::String getCurrentPresetName() const { return currentPresetName; }
    int getCurrentPresetIndex() const { return currentPresetIndex; }

private:
    juce::AudioProcessorValueTreeState& apvts;
    juce::File presetDirectory;
    juce::String currentPresetName { "Default" };
    int currentPresetIndex = 0;

    juce::File getPresetDirectory() const;
    juce::File getPresetFile (const juce::String& presetName) const;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (PresetManager)
};
