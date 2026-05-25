/*
  ==============================================================================
    HtwParameters.h
    Internal DSP parameter struct for HowsTheWeather
    Fed from APVTS each processBlock, consumed by all DSP modules
    No PlaybackMode enum — always Granular
  ==============================================================================
*/

#pragma once

namespace htw {

struct HtwParameters
{
    float position = 0.5f;
    float size = 0.5f;
    float pitch = 0.0f;        // semitones
    float density = 0.5f;
    float texture = 0.5f;
    float dryWet = 0.5f;
    float stereoSpread = 0.5f;
    float feedback = 0.0f;
    float reverb = 0.0f;

    bool freeze = false;
    bool trigger = false;

    // Derived parameters for granular mode (computed in processBlock)
    struct Granular
    {
        float overlap = 0.0f;
        float windowShape = 0.5f;
        bool useDeterministicSeed = false;
    } granular;
};

} // namespace htw
