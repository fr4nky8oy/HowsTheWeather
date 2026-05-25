/*
  ==============================================================================

    HowsTheWeather — Audio Processor Implementation
    Built with Franky_Agents_Template multi-agent workflow

  ==============================================================================
*/

#include "PluginProcessor.h"
#include "WebViewEditor.h"
#include "DSPAssertions.h"

//==============================================================================
HowsTheWeatherAudioProcessor::HowsTheWeatherAudioProcessor()
#ifndef JucePlugin_PreferredChannelConfigurations
     : AudioProcessor (BusesProperties()
                     #if ! JucePlugin_IsMidiEffect
                      #if ! JucePlugin_IsSynth
                       .withInput  ("Input",  juce::AudioChannelSet::stereo(), true)
                      #endif
                       .withOutput ("Output", juce::AudioChannelSet::stereo(), true)
                     #endif
                       ),
       parameters (*this, nullptr, "PARAMETERS", createParameterLayout())
#endif
{
    // Cache typed parameter pointers
    positionParam   = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("position"));
    sizeParam       = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("size"));
    densityParam    = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("density"));
    textureParam    = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("texture"));
    pitchParam      = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("pitch"));
    feedbackParam   = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("feedback"));
    dryWetParam     = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("drywet"));
    spreadParam     = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("spread"));
    reverbParam     = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("reverb"));
    freezeParam     = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("freeze"));

    weatherEnableParam    = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("weather_enable"));
    weatherGuiEnableParam = dynamic_cast<juce::AudioParameterBool*>(parameters.getParameter("weather_gui_enable"));
    weatherAmountParam    = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("weather_amount"));
    weatherLatParam       = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("weather_lat"));
    weatherLonParam       = dynamic_cast<juce::AudioParameterFloat*>(parameters.getParameter("weather_lon"));

    jassert(positionParam != nullptr);
    jassert(sizeParam != nullptr);
    jassert(densityParam != nullptr);
    jassert(textureParam != nullptr);
    jassert(pitchParam != nullptr);
    jassert(feedbackParam != nullptr);
    jassert(dryWetParam != nullptr);
    jassert(spreadParam != nullptr);
    jassert(reverbParam != nullptr);
    jassert(freezeParam != nullptr);
    jassert(weatherEnableParam != nullptr);
    jassert(weatherGuiEnableParam != nullptr);
    jassert(weatherAmountParam != nullptr);
    jassert(weatherLatParam != nullptr);
    jassert(weatherLonParam != nullptr);

    // Start weather service
    weatherService_.startService();
}

HowsTheWeatherAudioProcessor::~HowsTheWeatherAudioProcessor()
{
    weatherService_.stopService();
}

//==============================================================================
juce::AudioProcessorValueTreeState::ParameterLayout HowsTheWeatherAudioProcessor::createParameterLayout()
{
    std::vector<std::unique_ptr<juce::RangedAudioParameter>> params;

    // Core Granular Parameters
    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"position", 1}, "Position",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.1f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"size", 1}, "Size",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.93f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"density", 1}, "Density",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.83f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"texture", 1}, "Texture",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.87f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"pitch", 1}, "Pitch",
        juce::NormalisableRange<float>(-24.0f, 24.0f, 0.1f), -4.6f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"feedback", 1}, "Feedback",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.7f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"drywet", 1}, "Dry/Wet",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"spread", 1}, "Spread",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 0.82f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"reverb", 1}, "Reverb",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f, 0.5f), 0.32f));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"freeze", 1}, "Freeze", false));

    // Weather Modulation Parameters
    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"weather_enable", 1}, "Weather Enable", true));

    params.push_back(std::make_unique<juce::AudioParameterBool>(
        juce::ParameterID{"weather_gui_enable", 1}, "Weather GUI Enable", true));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"weather_amount", 1}, "Weather Amount",
        juce::NormalisableRange<float>(0.0f, 1.0f, 0.001f), 1.0f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"weather_lat", 1}, "Latitude",
        juce::NormalisableRange<float>(-90.0f, 90.0f, 0.01f), 51.5f));

    params.push_back(std::make_unique<juce::AudioParameterFloat>(
        juce::ParameterID{"weather_lon", 1}, "Longitude",
        juce::NormalisableRange<float>(-180.0f, 180.0f, 0.01f), -0.1f));

    return { params.begin(), params.end() };
}

//==============================================================================
const juce::String HowsTheWeatherAudioProcessor::getName() const
{
    return JucePlugin_Name;
}

bool HowsTheWeatherAudioProcessor::acceptsMidi() const
{
   #if JucePlugin_WantsMidiInput
    return true;
   #else
    return false;
   #endif
}

bool HowsTheWeatherAudioProcessor::producesMidi() const
{
   #if JucePlugin_ProducesMidiOutput
    return true;
   #else
    return false;
   #endif
}

bool HowsTheWeatherAudioProcessor::isMidiEffect() const
{
   #if JucePlugin_IsMidiEffect
    return true;
   #else
    return false;
   #endif
}

double HowsTheWeatherAudioProcessor::getTailLengthSeconds() const
{
    return 0.0;
}

int HowsTheWeatherAudioProcessor::getNumPrograms()
{
    return 1;
}

int HowsTheWeatherAudioProcessor::getCurrentProgram()
{
    return 0;
}

void HowsTheWeatherAudioProcessor::setCurrentProgram (int index)
{
}

const juce::String HowsTheWeatherAudioProcessor::getProgramName (int index)
{
    return {};
}

void HowsTheWeatherAudioProcessor::changeProgramName (int index, const juce::String& newName)
{
}

//==============================================================================
void HowsTheWeatherAudioProcessor::prepareToPlay (double sampleRate, int samplesPerBlock)
{
    // Reset all smoothers (20ms ramp)
    const double rampTime = 0.02;
    positionSmoother.reset(sampleRate, rampTime);
    sizeSmoother.reset(sampleRate, rampTime);
    densitySmoother.reset(sampleRate, rampTime);
    textureSmoother.reset(sampleRate, rampTime);
    pitchSmoother.reset(sampleRate, rampTime);
    feedbackSmoother.reset(sampleRate, rampTime);
    dryWetSmoother.reset(sampleRate, rampTime);
    spreadSmoother.reset(sampleRate, rampTime);
    reverbSmoother.reset(sampleRate, rampTime);

    // Initialize granular processor
    granularProcessor_.init(static_cast<float>(sampleRate), 32);
}

void HowsTheWeatherAudioProcessor::releaseResources()
{
}

#ifndef JucePlugin_PreferredChannelConfigurations
bool HowsTheWeatherAudioProcessor::isBusesLayoutSupported (const BusesLayout& layouts) const
{
  #if JucePlugin_IsMidiEffect
    juce::ignoreUnused (layouts);
    return true;
  #else
    if (layouts.getMainOutputChannelSet() != juce::AudioChannelSet::mono()
     && layouts.getMainOutputChannelSet() != juce::AudioChannelSet::stereo())
        return false;

   #if ! JucePlugin_IsSynth
    if (layouts.getMainOutputChannelSet() != layouts.getMainInputChannelSet())
        return false;
   #endif

    return true;
  #endif
}
#endif

void HowsTheWeatherAudioProcessor::processBlock (juce::AudioBuffer<float>& buffer, juce::MidiBuffer& midiMessages)
{
    juce::ScopedNoDenormals noDenormals;
    auto totalNumInputChannels  = getTotalNumInputChannels();
    auto totalNumOutputChannels = getTotalNumOutputChannels();

    for (auto i = totalNumInputChannels; i < totalNumOutputChannels; ++i)
        buffer.clear (i, 0, buffer.getNumSamples());

    int numSamples = buffer.getNumSamples();

    // Check for location changes and update weather service
    float currentLat = weatherLatParam->get();
    float currentLon = weatherLonParam->get();
    if (std::abs(currentLat - previousLat_) > 0.01f ||
        std::abs(currentLon - previousLon_) > 0.01f)
    {
        weatherService_.setLocation(currentLat, currentLon);
        previousLat_ = currentLat;
        previousLon_ = currentLon;
    }

    // Read base parameter values
    float positionVal = positionParam->get();
    float sizeVal     = sizeParam->get();
    float densityVal  = densityParam->get();
    float textureVal  = textureParam->get();
    float pitchVal    = pitchParam->get();
    float feedbackVal = feedbackParam->get();
    float dryWetVal   = dryWetParam->get();
    float spreadVal   = spreadParam->get();
    float reverbVal   = reverbParam->get();

    // Apply weather modulation offsets (always active, scaled by amount)
    if (weatherService_.isDataValid())
    {
        float amount = weatherAmountParam->get();

        // Wind → size: (norm - 0.5) * 1.2 * amount → max ±0.6
        sizeVal += (weatherService_.getWindNorm() - 0.5f) * 1.2f * amount;
        sizeVal = juce::jlimit(0.0f, 1.0f, sizeVal);

        // Cloud → density: (norm - 0.5) * 1.2 * amount → max ±0.6
        densityVal += (weatherService_.getCloudNorm() - 0.5f) * 1.2f * amount;
        densityVal = juce::jlimit(0.0f, 1.0f, densityVal);

        // Pressure → pitch: (norm - 0.5) * 24.0 * amount → max ±12 st
        pitchVal += (weatherService_.getPressureNorm() - 0.5f) * 24.0f * amount;
        pitchVal = juce::jlimit(-24.0f, 24.0f, pitchVal);

        // Temperature → position: (norm - 0.5) * 1.2 * amount → max ±0.6
        positionVal += (weatherService_.getTemperatureNorm() - 0.5f) * 1.2f * amount;
        positionVal = juce::jlimit(0.0f, 1.0f, positionVal);

        // Humidity → texture: (norm - 0.5) * 1.2 * amount → max ±0.6
        textureVal += (weatherService_.getHumidityNorm() - 0.5f) * 1.2f * amount;
        textureVal = juce::jlimit(0.0f, 1.0f, textureVal);
    }

    // Set smoother targets (weather offsets included, so they get smoothed automatically)
    positionSmoother.setTargetValue(positionVal);
    sizeSmoother.setTargetValue(sizeVal);
    densitySmoother.setTargetValue(densityVal);
    textureSmoother.setTargetValue(textureVal);
    pitchSmoother.setTargetValue(pitchVal);
    feedbackSmoother.setTargetValue(feedbackVal);
    dryWetSmoother.setTargetValue(dryWetVal);
    spreadSmoother.setTargetValue(spreadVal);
    reverbSmoother.setTargetValue(reverbVal);

    // Advance smoothers by the full block
    int skipCount = numSamples - 1;
    positionSmoother.skip(skipCount);
    sizeSmoother.skip(skipCount);
    densitySmoother.skip(skipCount);
    textureSmoother.skip(skipCount);
    pitchSmoother.skip(skipCount);
    feedbackSmoother.skip(skipCount);
    dryWetSmoother.skip(skipCount);
    spreadSmoother.skip(skipCount);
    reverbSmoother.skip(skipCount);

    // Build DSP parameter struct
    htw::HtwParameters dspParams;
    dspParams.position     = positionSmoother.getCurrentValue();
    dspParams.size         = sizeSmoother.getCurrentValue();
    dspParams.density      = densitySmoother.getCurrentValue();
    dspParams.texture      = textureSmoother.getCurrentValue();
    dspParams.pitch        = pitchSmoother.getCurrentValue();
    dspParams.feedback     = feedbackSmoother.getCurrentValue();
    dspParams.dryWet       = dryWetSmoother.getCurrentValue();
    dspParams.stereoSpread = spreadSmoother.getCurrentValue();
    dspParams.reverb       = reverbSmoother.getCurrentValue();
    dspParams.freeze       = freezeParam->get();

    // Process audio
    if (totalNumInputChannels >= 2) {
        float* leftIn  = buffer.getWritePointer(0);
        float* rightIn = buffer.getWritePointer(1);
        granularProcessor_.process(leftIn, rightIn, leftIn, rightIn,
                                   dspParams, numSamples);
    } else if (totalNumInputChannels == 1) {
        float* monoData = buffer.getWritePointer(0);
        juce::AudioBuffer<float> tempBuffer(2, numSamples);
        tempBuffer.copyFrom(0, 0, monoData, numSamples);
        tempBuffer.copyFrom(1, 0, monoData, numSamples);

        float* left  = tempBuffer.getWritePointer(0);
        float* right = tempBuffer.getWritePointer(1);

        granularProcessor_.process(left, right, left, right,
                                   dspParams, numSamples);

        for (int i = 0; i < numSamples; ++i)
            monoData[i] = (left[i] + right[i]) * 0.5f;
    }

    // Soft-clip output (tanh) to prevent harsh distortion when feedback/reverb/dry-wet stack up
    for (int ch = 0; ch < buffer.getNumChannels(); ++ch) {
        auto* data = buffer.getWritePointer(ch);
        for (int i = 0; i < numSamples; ++i)
            data[i] = std::tanh(data[i]);
    }

    // Debug validation
    DSPValidation::assertValidBuffer(buffer, "processBlock output");
}

//==============================================================================
bool HowsTheWeatherAudioProcessor::hasEditor() const
{
    return true;
}

juce::AudioProcessorEditor* HowsTheWeatherAudioProcessor::createEditor()
{
    return new HowsTheWeatherWebViewEditor (*this);
}

//==============================================================================
void HowsTheWeatherAudioProcessor::getStateInformation (juce::MemoryBlock& destData)
{
    auto state = parameters.copyState();
    std::unique_ptr<juce::XmlElement> xml (state.createXml());
    copyXmlToBinary (*xml, destData);
}

void HowsTheWeatherAudioProcessor::setStateInformation (const void* data, int sizeInBytes)
{
    std::unique_ptr<juce::XmlElement> xml (getXmlFromBinary (data, sizeInBytes));
    if (xml != nullptr && xml->hasTagName (parameters.state.getType()))
        parameters.replaceState (juce::ValueTree::fromXml (*xml));
}

//==============================================================================
juce::AudioProcessor* JUCE_CALLTYPE createPluginFilter()
{
    return new HowsTheWeatherAudioProcessor();
}
