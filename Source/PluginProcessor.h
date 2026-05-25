/*
  ==============================================================================

    HowsTheWeather — Audio Processor
    Built with Franky_Agents_Template multi-agent workflow

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include "DSP/HtwGranularProcessor.h"
#include "DSP/WeatherService.h"
#include "PresetManager.h"

class HowsTheWeatherAudioProcessor : public juce::AudioProcessor
{
public:
    //==============================================================================
    HowsTheWeatherAudioProcessor();
    ~HowsTheWeatherAudioProcessor() override;

    //==============================================================================
    void prepareToPlay (double sampleRate, int samplesPerBlock) override;
    void releaseResources() override;

   #ifndef JucePlugin_PreferredChannelConfigurations
    bool isBusesLayoutSupported (const BusesLayout& layouts) const override;
   #endif

    void processBlock (juce::AudioBuffer<float>&, juce::MidiBuffer&) override;

    //==============================================================================
    juce::AudioProcessorEditor* createEditor() override;
    bool hasEditor() const override;

    //==============================================================================
    const juce::String getName() const override;

    bool acceptsMidi() const override;
    bool producesMidi() const override;
    bool isMidiEffect() const override;
    double getTailLengthSeconds() const override;

    //==============================================================================
    int getNumPrograms() override;
    int getCurrentProgram() override;
    void setCurrentProgram (int index) override;
    const juce::String getProgramName (int index) override;
    void changeProgramName (int index, const juce::String& newName) override;

    //==============================================================================
    void getStateInformation (juce::MemoryBlock& destData) override;
    void setStateInformation (const void* data, int sizeInBytes) override;

    //==============================================================================
    juce::AudioProcessorValueTreeState::ParameterLayout createParameterLayout();

    // Public APVTS — GUI accesses parameters via attachments on this object
    juce::AudioProcessorValueTreeState parameters;

    // Public accessor for WeatherService — GUI reads weather data for display
    WeatherService& getWeatherService() { return weatherService_; }

    // Public accessor for PresetManager — GUI wires save/load/next/prev
    PresetManager& getPresetManager() { return presetManager_; }

private:
    //==============================================================================
    // Cached parameter pointers (set once in constructor)
    //==============================================================================
    juce::AudioParameterFloat* positionParam   = nullptr;
    juce::AudioParameterFloat* sizeParam       = nullptr;
    juce::AudioParameterFloat* densityParam    = nullptr;
    juce::AudioParameterFloat* textureParam    = nullptr;
    juce::AudioParameterFloat* pitchParam      = nullptr;
    juce::AudioParameterFloat* feedbackParam   = nullptr;
    juce::AudioParameterFloat* dryWetParam     = nullptr;
    juce::AudioParameterFloat* spreadParam     = nullptr;
    juce::AudioParameterFloat* reverbParam     = nullptr;
    juce::AudioParameterBool*  freezeParam     = nullptr;

    juce::AudioParameterBool*  weatherEnableParam    = nullptr;
    juce::AudioParameterBool*  weatherGuiEnableParam = nullptr;
    juce::AudioParameterFloat* weatherAmountParam    = nullptr;
    juce::AudioParameterFloat* weatherLatParam       = nullptr;
    juce::AudioParameterFloat* weatherLonParam       = nullptr;

    //==============================================================================
    // SmoothedValues for continuously-varying parameters
    //==============================================================================
    juce::SmoothedValue<float> positionSmoother;
    juce::SmoothedValue<float> sizeSmoother;
    juce::SmoothedValue<float> densitySmoother;
    juce::SmoothedValue<float> textureSmoother;
    juce::SmoothedValue<float> pitchSmoother;
    juce::SmoothedValue<float> feedbackSmoother;
    juce::SmoothedValue<float> dryWetSmoother;
    juce::SmoothedValue<float> spreadSmoother;
    juce::SmoothedValue<float> reverbSmoother;

    //==============================================================================
    // DSP engine
    //==============================================================================
    htw::GranularProcessor granularProcessor_;

    //==============================================================================
    // Weather service
    //==============================================================================
    WeatherService weatherService_;
    float previousLat_ = 51.5f;
    float previousLon_ = -0.1f;

    //==============================================================================
    // Preset manager
    //==============================================================================
    PresetManager presetManager_ { parameters, "Franky", "HowsTheWeather" };

    //==============================================================================
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (HowsTheWeatherAudioProcessor)
};
