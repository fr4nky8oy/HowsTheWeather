/*
  ==============================================================================
    WeatherService.h
    Background thread + timer for fetching Open-Meteo weather data
    All public getters are atomic — safe to read from audio thread
    Slew-limited interpolation at 2 Hz for smooth parameter transitions
  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>
#include <atomic>

class WeatherService : public juce::Thread,
                       public juce::Timer
{
public:
    WeatherService()
        : juce::Thread("WeatherFetch")
    {
    }

    ~WeatherService() override
    {
        stopTimer();
        signalThreadShouldExit();
        notify();
        stopThread(5000);
    }

    //==============================================================================
    // Call from message thread to start the service
    //==============================================================================
    void startService()
    {
        startThread(juce::Thread::Priority::low);
        startTimerHz(30); // 30 Hz slew interpolation for smooth transitions
    }

    void stopService()
    {
        stopTimer();
        signalThreadShouldExit();
        notify();
        stopThread(5000);
    }

    //==============================================================================
    // Set location — triggers immediate re-fetch
    //==============================================================================
    void setLocation(float lat, float lon)
    {
        latitude_.store(lat, std::memory_order_relaxed);
        longitude_.store(lon, std::memory_order_relaxed);
        locationChanged_.store(true, std::memory_order_release);
        notify(); // wake fetch thread immediately
    }

    //==============================================================================
    // Atomic getters — audio thread safe
    // Returns normalized 0–1 values with slew limiting applied
    //==============================================================================
    float getWindNorm()        const { return windNormSmooth_.load(std::memory_order_relaxed); }
    float getCloudNorm()       const { return cloudNormSmooth_.load(std::memory_order_relaxed); }
    float getPressureNorm()    const { return pressureNormSmooth_.load(std::memory_order_relaxed); }
    float getTemperatureNorm() const { return tempNormSmooth_.load(std::memory_order_relaxed); }
    float getHumidityNorm()    const { return humidityNormSmooth_.load(std::memory_order_relaxed); }
    bool  isDataValid()        const { return dataValid_.load(std::memory_order_relaxed); }

    // Raw values for GUI display
    float getWindSpeed()       const { return rawWind_.load(std::memory_order_relaxed); }
    float getCloudCover()      const { return rawCloud_.load(std::memory_order_relaxed); }
    float getPressure()        const { return rawPressure_.load(std::memory_order_relaxed); }
    float getTemperature()     const { return rawTemp_.load(std::memory_order_relaxed); }
    float getHumidity()        const { return rawHumidity_.load(std::memory_order_relaxed); }
    float getRain()            const { return rawRain_.load(std::memory_order_relaxed); }
    int   getWeatherCode()     const { return rawWeatherCode_.load(std::memory_order_relaxed); }
    float getWindDirection()   const { return rawWindDirection_.load(std::memory_order_relaxed); }

private:
    //==============================================================================
    // Background thread — HTTP fetch every 5 minutes
    //==============================================================================
    void run() override
    {
        while (!threadShouldExit())
        {
            fetchWeatherData();

            // Wait 5 minutes or until notified (location change)
            for (int i = 0; i < 300 && !threadShouldExit(); ++i)
            {
                wait(1000); // 1 second intervals for responsive exit

                if (locationChanged_.load(std::memory_order_acquire))
                {
                    locationChanged_.store(false, std::memory_order_release);
                    break; // re-fetch immediately
                }
            }
        }
    }

    //==============================================================================
    void fetchWeatherData()
    {
        float lat = latitude_.load(std::memory_order_relaxed);
        float lon = longitude_.load(std::memory_order_relaxed);

        juce::String urlStr = juce::String(
            "https://api.open-meteo.com/v1/forecast?latitude=")
            + juce::String(lat, 4)
            + "&longitude=" + juce::String(lon, 4)
            + "&current=temperature_2m,relative_humidity_2m,surface_pressure,wind_speed_10m,wind_direction_10m,cloud_cover,precipitation,weather_code"
            + "&timezone=auto";

        juce::URL url(urlStr);
        auto stream = url.createInputStream(
            juce::URL::InputStreamOptions(juce::URL::ParameterHandling::inAddress)
                .withConnectionTimeoutMs(10000)
                .withResponseHeaders(nullptr)
                .withNumRedirectsToFollow(3));

        if (stream == nullptr)
            return; // Fetch failed — keep previous data or stay invalid

        juce::String response = stream->readEntireStreamAsString();
        if (response.isEmpty())
            return;

        auto result = juce::JSON::parse(response);
        if (result.isVoid())
            return;

        auto* obj = result.getDynamicObject();
        if (obj == nullptr)
            return;

        auto current = obj->getProperty("current");
        if (current.isVoid())
            return;

        auto* currentObj = current.getDynamicObject();
        if (currentObj == nullptr)
            return;

        // Extract raw values
        float wind     = static_cast<float>((double)currentObj->getProperty("wind_speed_10m"));
        float cloud    = static_cast<float>((double)currentObj->getProperty("cloud_cover"));
        float pressure = static_cast<float>((double)currentObj->getProperty("surface_pressure"));
        float temp     = static_cast<float>((double)currentObj->getProperty("temperature_2m"));
        float humidity = static_cast<float>((double)currentObj->getProperty("relative_humidity_2m"));
        float rain     = static_cast<float>((double)currentObj->getProperty("precipitation"));
        int weatherCode = static_cast<int>((double)currentObj->getProperty("weather_code"));
        float windDir  = static_cast<float>((double)currentObj->getProperty("wind_direction_10m"));

        // Store raw values for GUI
        rawWind_.store(wind, std::memory_order_relaxed);
        rawCloud_.store(cloud, std::memory_order_relaxed);
        rawPressure_.store(pressure, std::memory_order_relaxed);
        rawTemp_.store(temp, std::memory_order_relaxed);
        rawHumidity_.store(humidity, std::memory_order_relaxed);
        rawRain_.store(rain, std::memory_order_relaxed);
        rawWeatherCode_.store(weatherCode, std::memory_order_relaxed);
        rawWindDirection_.store(windDir, std::memory_order_relaxed);

        // Normalize to 0–1 (per PARAMETER_SPEC.md)
        float windNorm     = juce::jlimit(0.0f, 1.0f, wind / 60.0f);
        float cloudNorm    = juce::jlimit(0.0f, 1.0f, cloud / 100.0f);
        float pressureNorm = juce::jlimit(0.0f, 1.0f, (pressure - 980.0f) / 60.0f);
        float tempNorm     = juce::jlimit(0.0f, 1.0f, (temp + 20.0f) / 65.0f);
        float humidityNorm = juce::jlimit(0.0f, 1.0f, humidity / 100.0f);

        // Store normalized targets (slew will interpolate toward these)
        windNormTarget_.store(windNorm, std::memory_order_relaxed);
        cloudNormTarget_.store(cloudNorm, std::memory_order_relaxed);
        pressureNormTarget_.store(pressureNorm, std::memory_order_relaxed);
        tempNormTarget_.store(tempNorm, std::memory_order_relaxed);
        humidityNormTarget_.store(humidityNorm, std::memory_order_relaxed);

        dataValid_.store(true, std::memory_order_release);
    }

    //==============================================================================
    // Timer callback — 30 Hz slew interpolation on message thread
    // ~5 second convergence (coefficient 0.007 at 30 Hz)
    //==============================================================================
    void timerCallback() override
    {
        if (!dataValid_.load(std::memory_order_acquire))
            return;

        const float coeff = 0.007f; // ~5s convergence at 30 Hz, smooth transitions

        auto slew = [coeff](std::atomic<float>& smooth, const std::atomic<float>& target) {
            float current = smooth.load(std::memory_order_relaxed);
            float tgt = target.load(std::memory_order_relaxed);
            current += coeff * (tgt - current);
            smooth.store(current, std::memory_order_relaxed);
        };

        slew(windNormSmooth_,     windNormTarget_);
        slew(cloudNormSmooth_,    cloudNormTarget_);
        slew(pressureNormSmooth_, pressureNormTarget_);
        slew(tempNormSmooth_,     tempNormTarget_);
        slew(humidityNormSmooth_, humidityNormTarget_);
    }

    //==============================================================================
    // Location
    std::atomic<float> latitude_  { 51.5f };  // Default: London
    std::atomic<float> longitude_ { -0.1f };
    std::atomic<bool>  locationChanged_ { false };

    // Raw values (for GUI display)
    std::atomic<float> rawWind_     { 0.0f };
    std::atomic<float> rawCloud_    { 0.0f };
    std::atomic<float> rawPressure_ { 1013.0f };
    std::atomic<float> rawTemp_     { 15.0f };
    std::atomic<float> rawHumidity_ { 50.0f };
    std::atomic<float> rawRain_     { 0.0f };
    std::atomic<int>   rawWeatherCode_ { 0 };
    std::atomic<float> rawWindDirection_ { 0.0f };

    // Normalized targets (set by fetch thread)
    std::atomic<float> windNormTarget_     { 0.5f };
    std::atomic<float> cloudNormTarget_    { 0.5f };
    std::atomic<float> pressureNormTarget_ { 0.5f };
    std::atomic<float> tempNormTarget_     { 0.5f };
    std::atomic<float> humidityNormTarget_ { 0.5f };

    // Smoothed normalized values (read by audio thread)
    std::atomic<float> windNormSmooth_     { 0.5f };
    std::atomic<float> cloudNormSmooth_    { 0.5f };
    std::atomic<float> pressureNormSmooth_ { 0.5f };
    std::atomic<float> tempNormSmooth_     { 0.5f };
    std::atomic<float> humidityNormSmooth_ { 0.5f };

    // State
    std::atomic<bool> dataValid_ { false };
};
