/*
  ==============================================================================
    HtwGranularPlayer.h
    Granular sample player — manages pool of 32 grains
    Ported from MistGranularPlayer.h
  ==============================================================================
*/

#pragma once

#include "HtwDSPCommon.h"
#include "HtwAudioBuffer.h"
#include "HtwGrain.h"
#include "HtwParameters.h"

namespace htw {

class GranularPlayer
{
public:
    GranularPlayer() = default;

    void init(int32_t numChannels, int32_t maxGrains)
    {
        maxNumGrains_ = maxGrains;
        numChannels_ = numChannels;
        gainNormalization_ = 1.0f;
        numGrains_ = 0.0f;
        grainSizeHint_ = 1024.0f;
        grainRatePhasor_ = -1000.0f;

        for (int i = 0; i < kMaxNumGrains; ++i)
            grains_[i].init();
    }

    void stopAllGrains()
    {
        for (int i = 0; i < kMaxNumGrains; ++i)
            grains_[i].stop();
    }

    //==============================================================================
    void play(const AudioBuffer16* buffer,
              const HtwParameters& params,
              float* out, size_t size)
    {
        float overlap = params.granular.overlap;
        overlap = overlap * overlap * overlap;
        float targetNumGrains = static_cast<float>(maxNumGrains_) * overlap;
        float p = targetNumGrains / grainSizeHint_;
        float spaceBetweenGrains = grainSizeHint_ / std::max(targetNumGrains, 0.001f);

        if (params.granular.useDeterministicSeed) {
            p = -1.0f;
        } else {
            grainRatePhasor_ = -1000.0f;
        }

        // Build available grains list
        int32_t numAvailable = 0;
        for (int32_t i = 0; i < maxNumGrains_; ++i) {
            if (!grains_[i].active())
                availableGrains_[numAvailable++] = i;
        }

        // Schedule new grains
        bool seedTrigger = params.trigger;
        for (size_t t = 0; t < size; ++t) {
            grainRatePhasor_ += 1.0f;
            bool seedProbabilistic = Random::getFloat() < p
                                  && targetNumGrains > numGrains_;
            bool seedDeterministic = grainRatePhasor_ >= spaceBetweenGrains;
            bool seed = seedProbabilistic || seedDeterministic || seedTrigger;

            if (numAvailable && seed) {
                --numAvailable;
                int32_t index = availableGrains_[numAvailable];

                scheduleGrain(&grains_[index], params, static_cast<int32_t>(t),
                              buffer->size(), buffer->head() - static_cast<int32_t>(size) + static_cast<int32_t>(t));

                grainRatePhasor_ = 0.0f;
                seedTrigger = false;
            }
        }

        // Clear output
        std::fill(&out[0], &out[size * 2], 0.0f);

        // Overlap-add all active grains
        for (int32_t i = 0; i < maxNumGrains_; ++i) {
            grains_[i].overlapAdd(buffer, numChannels_, out, envelopeBuffer_, size);
        }

        // Compute normalization
        int32_t activeGrains = maxNumGrains_ - numAvailable;
        slope(numGrains_, static_cast<float>(activeGrains), 0.9f, 0.2f);

        float normalization = numGrains_ > 2.0f
            ? fastRsqrt(numGrains_ - 1.0f) : 1.0f;
        float windowGain = 1.0f + 2.0f * params.granular.windowShape;
        windowGain = constrain(windowGain, 1.0f, 2.0f);
        normalization *= crossfade(1.0f, windowGain, params.granular.overlap);

        // Apply normalization
        for (size_t t = 0; t < size; ++t) {
            onePole(gainNormalization_, normalization, 0.01f);
            *out++ *= gainNormalization_;
            *out++ *= gainNormalization_;
        }
    }

private:
    void scheduleGrain(Grain* grain, const HtwParameters& params,
                       int32_t preDelay, int32_t bufferSize, int32_t bufferHead)
    {
        float position = params.position;
        float pitch = params.pitch;
        float windowShape = params.granular.windowShape;
        float grainSize = LUT::grainSize(params.size);
        float pitchRatio = semitonesToRatio(pitch);
        float invPitchRatio = semitonesToRatio(-pitch);
        float pan = 0.5f + params.stereoSpread * (Random::getFloat() - 0.5f);

        float gL, gR;
        if (numChannels_ == 1) {
            gL = LUT::sine(pan);
            gR = LUT::sine(1.0f - pan);
        } else {
            if (pan < 0.5f) {
                gL = 1.0f;
                gR = 2.0f * pan;
            } else {
                gR = 1.0f;
                gL = 2.0f * (1.0f - pan);
            }
        }

        if (pitchRatio > 1.0f)
            grainSize = std::min(grainSize, static_cast<float>(bufferSize) * 0.25f * invPitchRatio);

        float eatenByPlayHead = grainSize * pitchRatio;
        float eatenByRecording = grainSize;
        float available = static_cast<float>(bufferSize) - eatenByPlayHead - eatenByRecording;

        int32_t iSize = static_cast<int32_t>(grainSize) & ~1;
        int32_t start = bufferHead - static_cast<int32_t>(
            position * available + eatenByPlayHead);

        bool isHigh = (numChannels_ == 1) || (Random::getFloat() > 0.25f);

        grain->start(preDelay, bufferSize, start, iSize,
                     static_cast<int32_t>(pitchRatio * 65536.0f),
                     windowShape, gL, gR, isHigh);

        onePole(grainSizeHint_, grainSize, 0.1f);
    }

    int32_t maxNumGrains_ = 32;
    int32_t numChannels_ = 2;

    float numGrains_ = 0.0f;
    float gainNormalization_ = 1.0f;
    float grainSizeHint_ = 1024.0f;
    float grainRatePhasor_ = 0.0f;

    Grain grains_[kMaxNumGrains];
    int32_t availableGrains_[kMaxNumGrains] = {};
    float envelopeBuffer_[kMaxBlockSize] = {};
};

} // namespace htw
