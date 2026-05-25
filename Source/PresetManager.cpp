/*
  ==============================================================================

    PresetManager — Implementation

  ==============================================================================
*/

#include "PresetManager.h"

PresetManager::PresetManager (juce::AudioProcessorValueTreeState& apvts,
                              const juce::String& companyName,
                              const juce::String& pluginName)
    : apvts (apvts)
{
    presetDirectory = juce::File::getSpecialLocation (juce::File::userApplicationDataDirectory)
                          .getChildFile (companyName)
                          .getChildFile (pluginName)
                          .getChildFile ("Presets");

    if (! presetDirectory.exists())
        presetDirectory.createDirectory();
}

//==============================================================================
void PresetManager::savePreset (const juce::String& presetName)
{
    auto state = apvts.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());

    if (xml != nullptr)
    {
        auto file = getPresetFile (presetName);
        xml->writeTo (file);
        currentPresetName = presetName;
    }
}

void PresetManager::loadPreset (const juce::String& presetName)
{
    auto file = getPresetFile (presetName);
    loadPresetFromFile (file);
}

void PresetManager::loadPresetFromFile (const juce::File& file)
{
    if (! file.existsAsFile())
        return;

    auto xml = juce::parseXML (file);

    if (xml != nullptr && xml->hasTagName (apvts.state.getType()))
    {
        apvts.replaceState (juce::ValueTree::fromXml (*xml));
        currentPresetName = file.getFileNameWithoutExtension();

        auto presets = getAllPresets();
        currentPresetIndex = presets.indexOf (currentPresetName);
    }
}

//==============================================================================
void PresetManager::loadNextPreset()
{
    auto presets = getAllPresets();
    if (presets.isEmpty())
        return;

    currentPresetIndex = (currentPresetIndex + 1) % presets.size();
    loadPreset (presets[currentPresetIndex]);
}

void PresetManager::loadPreviousPreset()
{
    auto presets = getAllPresets();
    if (presets.isEmpty())
        return;

    currentPresetIndex = (currentPresetIndex - 1 + presets.size()) % presets.size();
    loadPreset (presets[currentPresetIndex]);
}

//==============================================================================
juce::StringArray PresetManager::getAllPresets() const
{
    juce::StringArray presets;

    for (const auto& file : presetDirectory.findChildFiles (juce::File::findFiles, false, "*.xml"))
        presets.add (file.getFileNameWithoutExtension());

    presets.sort (true);
    return presets;
}

juce::File PresetManager::getPresetDirectory() const
{
    return presetDirectory;
}

juce::File PresetManager::getPresetFile (const juce::String& presetName) const
{
    return presetDirectory.getChildFile (presetName + ".xml");
}
