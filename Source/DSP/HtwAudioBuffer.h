/*
  ==============================================================================
    HtwAudioBuffer.h
    Circular audio buffer with freeze and crossfade — 16-bit only
    Ported from MistAudioBuffer.h (Bit8MuLaw removed per ARCHITECTURE.md)
  ==============================================================================
*/

#pragma once

#include "HtwDSPCommon.h"
#include <vector>
#include <algorithm>

namespace htw {

static constexpr int32_t kCrossFadeSize = 256;
static constexpr int32_t kInterpolationTail = 8;

//==============================================================================
// AudioBuffer16 — 16-bit circular buffer for granular processing
//==============================================================================
class AudioBuffer16
{
public:
    AudioBuffer16() = default;

    void init(int32_t bufferSize)
    {
        size_ = bufferSize - kInterpolationTail;
        writeHead_ = 0;
        crossfadeCounter_ = 0;
        storage_.resize(static_cast<size_t>(bufferSize), 0);
        tailBuffer_.resize(kCrossFadeSize, 0);
    }

    void resync(int32_t head)
    {
        writeHead_ = head;
        crossfadeCounter_ = 0;
    }

    //==============================================================================
    void write(float in)
    {
        storage_[writeHead_] = clip16(static_cast<int32_t>(in * 32768.0f));
        if (writeHead_ < kInterpolationTail)
            storage_[writeHead_ + size_] = storage_[writeHead_];

        ++writeHead_;
        if (writeHead_ >= size_)
            writeHead_ = 0;
    }

    //==============================================================================
    void writeFade(const float* in, int32_t count, int32_t stride, bool doWrite)
    {
        if (!doWrite) {
            if (crossfadeCounter_ < kCrossFadeSize) {
                while (count--) {
                    if (crossfadeCounter_ < kCrossFadeSize) {
                        tailBuffer_[crossfadeCounter_++] = clip16(
                            static_cast<int32_t>(*in * 32767.0f));
                        in += stride;
                    }
                }
            }
        } else {
            while (count--) {
                float sample = *in;
                if (crossfadeCounter_) {
                    --crossfadeCounter_;
                    float tailSample = static_cast<float>(
                        tailBuffer_[kCrossFadeSize - crossfadeCounter_]);
                    float gain = static_cast<float>(crossfadeCounter_)
                               / static_cast<float>(kCrossFadeSize);
                    sample += (tailSample / 32768.0f - sample) * gain;
                }
                write(sample);
                in += stride;
            }
        }
    }

    //==============================================================================
    // Read with interpolation
    //==============================================================================
    enum class Quality { Low, Medium, High };

    float readZOH(int32_t integral, uint16_t /*fractional*/) const
    {
        if (integral >= size_) integral -= size_;
        if (integral < 0) integral += size_;
        return static_cast<float>(storage_[integral]) / 32768.0f;
    }

    float readLinear(int32_t integral, uint16_t fractional) const
    {
        if (integral >= size_) integral -= size_;
        if (integral < 0) integral += size_;

        float t = static_cast<float>(fractional) / 65536.0f;
        float x0 = static_cast<float>(storage_[integral]);
        float x1 = static_cast<float>(storage_[integral + 1]);
        return (x0 + (x1 - x0) * t) / 32768.0f;
    }

    float readHermite(int32_t integral, uint16_t fractional) const
    {
        if (integral >= size_) integral -= size_;
        if (integral < 0) integral += size_;

        float t = static_cast<float>(fractional) / 65536.0f;
        float xm1 = static_cast<float>(storage_[integral]);
        float x0  = static_cast<float>(storage_[integral + 1]);
        float x1  = static_cast<float>(storage_[integral + 2]);
        float x2  = static_cast<float>(storage_[integral + 3]);

        const float c = (x1 - xm1) * 0.5f;
        const float v = x0 - x1;
        const float w = c + v;
        const float a = w + v + (x2 - x0) * 0.5f;
        const float bNeg = w + a;
        return ((((a * t) - bNeg) * t + c) * t + x0) / 32768.0f;
    }

    float read(int32_t integral, uint16_t fractional, Quality quality) const
    {
        switch (quality) {
            case Quality::High:   return readHermite(integral, fractional);
            case Quality::Medium: return readLinear(integral, fractional);
            case Quality::Low:
            default:              return readZOH(integral, fractional);
        }
    }

    int32_t size() const { return size_; }
    int32_t head() const { return writeHead_; }

private:
    int32_t size_ = 0;
    int32_t writeHead_ = 0;
    int32_t crossfadeCounter_ = 0;
    std::vector<int16_t> storage_;
    std::vector<int16_t> tailBuffer_;
};

} // namespace htw
