/*
  ==============================================================================

    DebugUtils.h
    Debugging utilities for audio plugin development

    These tools help diagnose issues during development:
    - Audio buffer visualization
    - Performance profiling
    - Thread-safe logging
    - Parameter monitoring

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>

//==============================================================================
/**
    Thread-safe logger for audio thread debugging

    Use this instead of DBG() in the audio thread to avoid blocking
*/
class AudioThreadLogger
{
public:
    static AudioThreadLogger& getInstance()
    {
        static AudioThreadLogger instance;
        return instance;
    }

    void log (const juce::String& message)
    {
        // Add to ring buffer
        const juce::ScopedLock sl (lock);
        logBuffer.add (message);

        // Keep only last 100 messages
        while (logBuffer.size() > 100)
            logBuffer.remove (0);
    }

    void printAll()
    {
        const juce::ScopedLock sl (lock);

        DBG ("=== Audio Thread Log ===");
        for (const auto& msg : logBuffer)
            DBG (msg);
        DBG ("========================");
    }

    void clear()
    {
        const juce::ScopedLock sl (lock);
        logBuffer.clear();
    }

private:
    AudioThreadLogger() = default;

    juce::CriticalSection lock;
    juce::StringArray logBuffer;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (AudioThreadLogger)
};

// Macro for easy audio thread logging
#define AUDIO_LOG(message) AudioThreadLogger::getInstance().log(message)

//==============================================================================
/**
    Simple profiler to measure execution time
*/
class SimpleProfiler
{
public:
    SimpleProfiler (const juce::String& name)
        : profilerName (name)
    {
        startTime = juce::Time::getHighResolutionTicks();
    }

    ~SimpleProfiler()
    {
        auto elapsed = juce::Time::getHighResolutionTicks() - startTime;
        auto elapsedMs = juce::Time::highResolutionTicksToSeconds (elapsed) * 1000.0;

        #if JUCE_DEBUG
        DBG (profilerName + ": " + juce::String (elapsedMs, 3) + " ms");
        #endif
    }

private:
    juce::String profilerName;
    juce::int64 startTime;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (SimpleProfiler)
};

// Macro for easy profiling
#define PROFILE_SCOPE(name) SimpleProfiler profiler_##__LINE__(name)
#define PROFILE_FUNCTION() SimpleProfiler profiler_##__LINE__(__FUNCTION__)

//==============================================================================
/**
    Buffer visualizer - prints audio buffer contents for debugging
*/
class BufferVisualizer
{
public:
    static void printBuffer (const juce::AudioBuffer<float>& buffer,
                            const juce::String& label = "Buffer",
                            int maxSamplesToShow = 10)
    {
        #if JUCE_DEBUG
        DBG ("=== " + label + " ===");
        DBG ("Channels: " + juce::String (buffer.getNumChannels()));
        DBG ("Samples: " + juce::String (buffer.getNumSamples()));

        int samplesToShow = juce::jmin (maxSamplesToShow, buffer.getNumSamples());

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            juce::String channelStr = "Ch " + juce::String (ch) + ": [";

            const float* data = buffer.getReadPointer (ch);
            for (int i = 0; i < samplesToShow; ++i)
            {
                channelStr += juce::String (data[i], 4);
                if (i < samplesToShow - 1)
                    channelStr += ", ";
            }

            if (samplesToShow < buffer.getNumSamples())
                channelStr += ", ...";

            channelStr += "]";
            DBG (channelStr);
        }

        DBG ("================");
        #else
        juce::ignoreUnused (buffer, label, maxSamplesToShow);
        #endif
    }

    static void printStats (const juce::AudioBuffer<float>& buffer,
                           const juce::String& label = "Buffer Stats")
    {
        #if JUCE_DEBUG
        DBG ("=== " + label + " ===");

        for (int ch = 0; ch < buffer.getNumChannels(); ++ch)
        {
            float peak = buffer.getMagnitude (ch, 0, buffer.getNumSamples());
            float rms = buffer.getRMSLevel (ch, 0, buffer.getNumSamples());

            DBG ("Channel " + juce::String (ch) + ":");
            DBG ("  Peak: " + juce::String (peak, 6));
            DBG ("  RMS:  " + juce::String (rms, 6));
            DBG ("  dB:   " + juce::String (juce::Decibels::gainToDecibels (rms), 2) + " dB");
        }

        DBG ("================");
        #else
        juce::ignoreUnused (buffer, label);
        #endif
    }
};

//==============================================================================
/**
    Parameter change monitor - logs when parameters change
*/
class ParameterMonitor
{
public:
    void checkParameter (const juce::String& name, float currentValue)
    {
        #if JUCE_DEBUG
        if (!lastValues.contains (name))
        {
            lastValues.set (name, currentValue);
            return;
        }

        float lastValue = lastValues[name];
        if (std::abs (currentValue - lastValue) > 0.001f)
        {
            DBG ("Parameter changed: " + name +
                 " (" + juce::String (lastValue, 3) +
                 " -> " + juce::String (currentValue, 3) + ")");

            lastValues.set (name, currentValue);
        }
        #else
        juce::ignoreUnused (name, currentValue);
        #endif
    }

private:
    juce::HashMap<juce::String, float> lastValues;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (ParameterMonitor)
};

//==============================================================================
/**
    CPU usage monitor for audio processing
*/
class CPUMonitor
{
public:
    void startMeasurement()
    {
        startTime = juce::Time::getHighResolutionTicks();
    }

    void stopMeasurement (double sampleRate, int bufferSize)
    {
        auto elapsed = juce::Time::getHighResolutionTicks() - startTime;
        auto elapsedSeconds = juce::Time::highResolutionTicksToSeconds (elapsed);

        // Calculate what percentage of buffer time was used
        double bufferDuration = bufferSize / sampleRate;
        double cpuUsagePercent = (elapsedSeconds / bufferDuration) * 100.0;

        // Exponential moving average
        averageCPU = 0.95 * averageCPU + 0.05 * cpuUsagePercent;
        peakCPU = juce::jmax (peakCPU, cpuUsagePercent);

        ++measurementCount;

        // Print stats every 100 measurements
        #if JUCE_DEBUG
        if (measurementCount % 100 == 0)
        {
            DBG ("CPU Usage - Average: " + juce::String (averageCPU, 1) + "%, Peak: " +
                 juce::String (peakCPU, 1) + "%");

            // Warn if approaching limit
            if (averageCPU > 80.0)
                DBG ("⚠️  WARNING: High CPU usage!");
        }
        #endif
    }

    double getAverageCPU() const { return averageCPU; }
    double getPeakCPU() const { return peakCPU; }

    void reset()
    {
        averageCPU = 0.0;
        peakCPU = 0.0;
        measurementCount = 0;
    }

private:
    juce::int64 startTime = 0;
    double averageCPU = 0.0;
    double peakCPU = 0.0;
    int measurementCount = 0;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (CPUMonitor)
};

//==============================================================================
/**
    Conditional breakpoint helper
    Use BREAK_IF() to break only when condition is true
*/
#if JUCE_DEBUG
    #define BREAK_IF(condition) if (condition) { jassertfalse; }
#else
    #define BREAK_IF(condition) ((void)0)
#endif

//==============================================================================
/**
    Example usage in AudioProcessor:

    class MyAudioProcessor : public AudioProcessor
    {
    public:
        void processBlock (AudioBuffer<float>& buffer, MidiBuffer&) override
        {
            // Profile this function
            PROFILE_FUNCTION();

            // Measure CPU usage
            cpuMonitor.startMeasurement();

            // Check for invalid samples
            DSPValidation::assertValidBuffer (buffer, "Input");

            // Your processing here...

            // Print buffer stats periodically
            if (++debugCounter % 1000 == 0)
                BufferVisualizer::printStats (buffer, "Output");

            cpuMonitor.stopMeasurement (getSampleRate(), buffer.getNumSamples());
        }

    private:
        CPUMonitor cpuMonitor;
        ParameterMonitor paramMonitor;
        int debugCounter = 0;
    };
*/
