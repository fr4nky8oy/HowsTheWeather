/*
  ==============================================================================
    HtwGrain.h
    Individual grain synthesis — ported from MistGrain.h
  ==============================================================================
*/

#pragma once

#include "HtwDSPCommon.h"
#include "HtwAudioBuffer.h"

namespace htw {

class Grain
{
public:
    Grain() = default;

    void init()
    {
        active_ = false;
        envelopePhase_ = 2.0f;
    }

    void start(int32_t preDelay, int32_t bufferSize, int32_t startPos,
               int32_t width, int32_t phaseIncrement, float windowShape,
               float gainL, float gainR, bool highQuality)
    {
        preDelay_ = preDelay;
        width_ = width;
        firstSample_ = (startPos + bufferSize) % bufferSize;
        phaseIncrement_ = phaseIncrement;
        phase_ = 0;
        envelopePhase_ = 0.0f;
        envelopePhaseIncrement_ = 2.0f / static_cast<float>(width);

        if (windowShape >= 0.5f) {
            envelopeSmoothness_ = (windowShape - 0.5f) * 2.0f;
            envelopeSlope_ = 0.0f;
        } else {
            envelopeSmoothness_ = 0.0f;
            envelopeSlope_ = 0.5f / (windowShape + 0.01f);
        }

        active_ = true;
        gainL_ = gainL;
        gainR_ = gainR;
        highQuality_ = highQuality;
    }

    //==============================================================================
    void renderEnvelope(float* destination, size_t count, bool useLUT)
    {
        const float increment = envelopePhaseIncrement_;
        const float smoothness = envelopeSmoothness_;
        const float slopeVal = envelopeSlope_;
        float phase = envelopePhase_;

        while (count--) {
            float gain = phase;
            gain = gain >= 1.0f ? 2.0f - gain : gain;

            if (useLUT) {
                float window = LUT::window(gain);
                gain += smoothness * (window - gain);
            } else {
                gain *= slopeVal;
                if (gain >= 1.0f) gain = 1.0f;
            }

            phase += increment;
            if (phase >= 2.0f) {
                *destination = -1.0f;
                break;
            }
            *destination++ = gain;
        }
        envelopePhase_ = phase;
    }

    //==============================================================================
    void overlapAdd(const AudioBuffer16* buffer, int numChannels,
                    float* out, float* envelope, size_t count)
    {
        if (!active_) return;

        // Handle pre-delay
        while (preDelay_ && count) {
            out += 2;
            --count;
            --preDelay_;
        }

        auto quality = highQuality_
            ? (envelopeSmoothness_ > 0.0f)
            : false;
        renderEnvelope(envelope, count, quality);

        auto readQuality = highQuality_
            ? AudioBuffer16::Quality::High
            : AudioBuffer16::Quality::Medium;

        const int32_t phaseInc = phaseIncrement_;
        const int32_t firstSample = firstSample_;
        const float gL = gainL_;
        const float gR = gainR_;
        int32_t phase = phase_;

        while (count--) {
            int32_t sampleIndex = firstSample + (phase >> 16);
            float gain = *envelope++;

            if (gain == -1.0f) {
                active_ = false;
                break;
            }

            float l = buffer[0].read(sampleIndex, static_cast<uint16_t>(phase & 65535), readQuality) * gain;

            if (numChannels == 1) {
                *out++ += l * gL;
                *out++ += l * gR;
            } else {
                float r = buffer[1].read(sampleIndex, static_cast<uint16_t>(phase & 65535), readQuality) * gain;
                *out++ += l * gL + r * (1.0f - gR);
                *out++ += r * gR + l * (1.0f - gL);
            }
            phase += phaseInc;
        }
        phase_ = phase;
    }

    bool active() const { return active_; }
    void stop() { active_ = false; envelopePhase_ = 2.0f; }

private:
    int32_t firstSample_ = 0;
    int32_t width_ = 0;
    int32_t phase_ = 0;
    int32_t phaseIncrement_ = 0;
    int32_t preDelay_ = 0;

    float envelopeSmoothness_ = 0.0f;
    float envelopeSlope_ = 0.0f;
    float envelopePhase_ = 2.0f;
    float envelopePhaseIncrement_ = 0.0f;

    float gainL_ = 0.0f;
    float gainR_ = 0.0f;

    bool active_ = false;
    bool highQuality_ = true;
};

} // namespace htw
