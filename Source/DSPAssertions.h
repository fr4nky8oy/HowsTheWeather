/*
  ==============================================================================

    DSPAssertions.h
    Assertions and validation helpers for audio DSP

    Use these in your audio processing code to catch common issues:
    - NaNs and Infs
    - Denormals
    - Excessive levels
    - Invalid parameters

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <cmath>

//==============================================================================
/** Assertion macros for DSP - only active in Debug builds */

#if JUCE_DEBUG
    #define DSP_ASSERT(condition, message) jassert(condition && message)
#else
    #define DSP_ASSERT(condition, message) ((void)0)
#endif

//==============================================================================
namespace DSPValidation
{
    /** Check if a sample is valid (not NaN or Inf) */
    inline bool isValidSample (float sample)
    {
        return !std::isnan (sample) && !std::isinf (sample);
    }

    /** Check if a sample is a denormal */
    inline bool isDenormal (float sample)
    {
        constexpr float denormalThreshold = 1e-15f;
        return std::abs (sample) > 0.0f && std::abs (sample) < denormalThreshold;
    }

    /** Check if a sample is safe for real-time processing */
    inline bool isSafeSample (float sample)
    {
        return isValidSample (sample) && !isDenormal (sample);
    }

    /** Flush denormals to zero */
    inline float flushDenormal (float sample)
    {
        constexpr float denormalThreshold = 1e-15f;
        return (std::abs (sample) < denormalThreshold) ? 0.0f : sample;
    }

    /** Check if all samples in a buffer are valid */
    inline bool isValidBuffer (const juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (!isValidSample (data[i]))
                    return false;
            }
        }
        return true;
    }

    /** Count denormals in a buffer */
    inline int countDenormals (const juce::AudioBuffer<float>& buffer)
    {
        int count = 0;
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                if (isDenormal (data[i]))
                    ++count;
            }
        }
        return count;
    }

    /** Flush all denormals in a buffer to zero */
    inline void flushDenormals (juce::AudioBuffer<float>& buffer)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float* data = buffer.getWritePointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                data[i] = flushDenormal (data[i]);
            }
        }
    }

    /** Check if a frequency is in audible range */
    inline bool isAudibleFrequency (float frequencyHz)
    {
        return frequencyHz >= 20.0f && frequencyHz <= 20000.0f;
    }

    /** Check if a sample rate is valid */
    inline bool isValidSampleRate (double sampleRate)
    {
        return sampleRate >= 8000.0 && sampleRate <= 384000.0;
    }

    /** Check if a buffer size is reasonable for real-time */
    inline bool isValidBufferSize (int bufferSize)
    {
        return bufferSize > 0 && bufferSize <= 8192;
    }

    /** Calculate RMS level of a buffer */
    inline float calculateRMS (const juce::AudioBuffer<float>& buffer)
    {
        float sumSquares = 0.0f;
        int totalSamples = 0;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer (ch);
            for (int i = 0; i < buffer.getNumSamples(); ++i)
            {
                sumSquares += data[i] * data[i];
            }
            totalSamples += buffer.getNumSamples();
        }

        return std::sqrt (sumSquares / static_cast<float>(totalSamples));
    }

    /** Calculate peak level of a buffer */
    inline float calculatePeak (const juce::AudioBuffer<float>& buffer)
    {
        float peak = 0.0f;

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float channelPeak = buffer.getMagnitude (ch, 0, buffer.getNumSamples());
            peak = std::max (peak, channelPeak);
        }

        return peak;
    }

    /** Check if buffer has excessive DC offset */
    inline bool hasExcessiveDC (const juce::AudioBuffer<float>& buffer, float threshold = 0.1f)
    {
        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            const float* data = buffer.getReadPointer (ch);
            float sum = 0.0f;

            for (int i = 0; i < buffer.getNumSamples(); ++i)
                sum += data[i];

            float dcOffset = sum / static_cast<float>(buffer.getNumSamples());

            if (std::abs (dcOffset) > threshold)
                return true;
        }

        return false;
    }

    /** Assert that a sample is valid in debug builds */
    inline void assertValidSample (float sample, const char* context = "")
    {
        #if JUCE_DEBUG
        if (!isValidSample (sample))
        {
            DBG ("Invalid sample detected" + juce::String (context));
            DBG ("Sample value: " + juce::String (sample));
            jassertfalse;
        }
        #else
        juce::ignoreUnused (sample, context);
        #endif
    }

    /** Assert that a buffer is valid in debug builds */
    inline void assertValidBuffer (const juce::AudioBuffer<float>& buffer, const char* context = "")
    {
        #if JUCE_DEBUG
        if (!isValidBuffer (buffer))
        {
            DBG ("Invalid buffer detected" + juce::String (context));
            jassertfalse;
        }
        #else
        juce::ignoreUnused (buffer, context);
        #endif
    }

    /** Assert that parameters are in valid range */
    inline void assertInRange (float value, float minVal, float maxVal, const char* paramName = "")
    {
        #if JUCE_DEBUG
        if (value < minVal || value > maxVal)
        {
            DBG ("Parameter out of range: " + juce::String (paramName));
            DBG ("Value: " + juce::String (value) +
                 " (expected " + juce::String (minVal) + " to " + juce::String (maxVal) + ")");
            jassertfalse;
        }
        #else
        juce::ignoreUnused (value, minVal, maxVal, paramName);
        #endif
    }

} // namespace DSPValidation

//==============================================================================
/**
    RAII class to measure processing time and warn if it exceeds buffer duration
*/
class ProcessingTimeGuard
{
public:
    ProcessingTimeGuard (double sampleRate, int bufferSize)
        : maxTime ((bufferSize / sampleRate) * 0.9) // 90% of buffer time
    {
        startTime = juce::Time::getMillisecondCounterHiRes();
    }

    ~ProcessingTimeGuard()
    {
        #if JUCE_DEBUG
        double elapsed = (juce::Time::getMillisecondCounterHiRes() - startTime) / 1000.0;

        if (elapsed > maxTime)
        {
            DBG ("⚠️ Processing took too long!");
            DBG ("Elapsed: " + juce::String (elapsed * 1000.0, 2) + " ms");
            DBG ("Maximum: " + juce::String (maxTime * 1000.0, 2) + " ms");
        }
        #endif
    }

private:
    double startTime;
    double maxTime;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ProcessingTimeGuard)
};
