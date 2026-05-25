/*
  ==============================================================================
    HtwDSPCommon.h
    Common DSP utilities for HowsTheWeather
    Ported from MistDSPCommon.h — stripped of mu-law and PlaybackMode
  ==============================================================================
*/

#pragma once

#include <cmath>
#include <cstdint>
#include <cstring>
#include <algorithm>
#include <random>

namespace htw {

//==============================================================================
// Constants
//==============================================================================
static constexpr int kMaxBlockSize = 32;
static constexpr int kMaxNumGrains = 32;

//==============================================================================
// Math helpers
//==============================================================================
inline float onePole(float& state, float input, float coefficient)
{
    state += coefficient * (input - state);
    return state;
}

inline float constrain(float value, float minVal, float maxVal)
{
    return std::max(minVal, std::min(maxVal, value));
}

inline float semitonesToRatio(float semitones)
{
    return std::pow(2.0f, semitones / 12.0f);
}

inline float softLimit(float x)
{
    return x * (27.0f + x * x) / (27.0f + 9.0f * x * x);
}

inline float softClip(float x)
{
    if (x > 1.0f) return 1.0f;
    if (x < -1.0f) return -1.0f;
    return 1.5f * x - 0.5f * x * x * x;
}

inline int16_t clip16(int32_t x)
{
    if (x > 32767) return 32767;
    if (x < -32768) return -32768;
    return static_cast<int16_t>(x);
}

inline float fastRsqrt(float x)
{
    float xhalf = 0.5f * x;
    int32_t i;
    std::memcpy(&i, &x, sizeof(float));
    i = 0x5f3759df - (i >> 1);
    std::memcpy(&x, &i, sizeof(float));
    x = x * (1.5f - xhalf * x * x);
    return x;
}

inline float crossfade(float a, float b, float mix)
{
    return a + (b - a) * mix;
}

//==============================================================================
// Slope
//==============================================================================
inline void slope(float& state, float target, float positiveRate, float negativeRate)
{
    if (target > state)
        state += positiveRate * (target - state);
    else
        state += negativeRate * (target - state);
}

//==============================================================================
// Parameter interpolator
//==============================================================================
class ParameterInterpolator
{
public:
    ParameterInterpolator(float* state, float newValue, size_t size)
    {
        state_ = state;
        value_ = *state;
        increment_ = (newValue - *state) / static_cast<float>(size);
    }

    ~ParameterInterpolator()
    {
        *state_ = value_;
    }

    inline float next()
    {
        value_ += increment_;
        return value_;
    }

private:
    float* state_ = nullptr;
    float value_ = 0.0f;
    float increment_ = 0.0f;
};

//==============================================================================
// Cosine oscillator (LFO)
//==============================================================================
class CosineOscillator
{
public:
    void init(float frequency)
    {
        iir_coefficient_ = 2.0f * std::cos(2.0f * 3.14159265358979f * frequency);
        initial_amplitude_ = iir_coefficient_ * 0.25f;
        start();
    }

    void start()
    {
        y1_ = initial_amplitude_;
        y0_ = 0.5f;
    }

    inline float value() const { return y1_ + 0.5f; }

    inline float next()
    {
        float temp = y0_;
        y0_ = iir_coefficient_ * y0_ - y1_;
        y1_ = temp;
        return temp + 0.5f;
    }

private:
    float y1_ = 0.0f;
    float y0_ = 0.0f;
    float iir_coefficient_ = 0.0f;
    float initial_amplitude_ = 0.0f;
};

//==============================================================================
// Random number generation
//==============================================================================
class Random
{
public:
    static float getFloat()
    {
        static std::mt19937 rng(42);
        static std::uniform_real_distribution<float> dist(0.0f, 1.0f);
        return dist(rng);
    }

    static uint32_t getWord()
    {
        static std::mt19937 rng(42);
        return rng();
    }
};

//==============================================================================
// Stereo frame
//==============================================================================
struct FloatFrame
{
    float l = 0.0f;
    float r = 0.0f;
};

//==============================================================================
// SVF (State Variable Filter)
//==============================================================================
class SVF
{
public:
    void init()
    {
        state1_ = 0.0f;
        state2_ = 0.0f;
        frequency_ = 0.01f;
        resonance_ = 1.0f;
    }

    void setFQ(float frequency, float resonance)
    {
        frequency_ = 2.0f * std::sin(3.14159265358979f * std::min(frequency, 0.499f));
        resonance_ = 1.0f / resonance;
    }

    void copyFrom(const SVF& other)
    {
        frequency_ = other.frequency_;
        resonance_ = other.resonance_;
    }

    void processHighPass(float* data, size_t size, int stride)
    {
        for (size_t i = 0; i < size; ++i) {
            float input = data[i * stride];
            float hp = input - state1_ * resonance_ - state2_;
            float bp = hp * frequency_ + state1_;
            float lp = bp * frequency_ + state2_;
            state1_ = bp;
            state2_ = lp;
            data[i * stride] = hp;
        }
    }

    void processLowPass(float* data, size_t size, int stride)
    {
        for (size_t i = 0; i < size; ++i) {
            float input = data[i * stride];
            float hp = input - state1_ * resonance_ - state2_;
            float bp = hp * frequency_ + state1_;
            float lp = bp * frequency_ + state2_;
            state1_ = bp;
            state2_ = lp;
            data[i * stride] = lp;
        }
    }

private:
    float state1_ = 0.0f;
    float state2_ = 0.0f;
    float frequency_ = 0.01f;
    float resonance_ = 1.0f;
};

//==============================================================================
// Lookup tables
//==============================================================================
namespace LUT {

inline float grainSize(float normalizedSize)
{
    return 320.0f * std::pow(200.0f, normalizedSize);
}

inline float window(float phase)
{
    return 0.5f * (1.0f - std::cos(2.0f * 3.14159265358979f * phase));
}

inline float sine(float phase)
{
    return std::sin(phase * 3.14159265358979f * 0.5f);
}

inline float xfadeIn(float mix)
{
    return std::sin(mix * 3.14159265358979f * 0.5f);
}

inline float xfadeOut(float mix)
{
    return std::cos(mix * 3.14159265358979f * 0.5f);
}

} // namespace LUT

} // namespace htw
