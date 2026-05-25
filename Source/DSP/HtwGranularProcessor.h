/*
  ==============================================================================
    HtwGranularProcessor.h
    Main granular processing engine for HowsTheWeather
    Ported from MistGranularProcessor.h — granular mode only
    No mode switching, no 8-bit buffer, no PitchShifter/LoopingDelay/Spectral
  ==============================================================================
*/

#pragma once

#include "HtwDSPCommon.h"
#include "HtwParameters.h"
#include "HtwAudioBuffer.h"
#include "HtwGranularPlayer.h"
#include "HtwReverb.h"
#include "HtwDiffuser.h"

namespace htw {

class GranularProcessor
{
public:
    GranularProcessor() = default;

    //==============================================================================
    void init(float hostSampleRate, int maxGrains)
    {
        hostSampleRate_ = hostSampleRate;
        maxGrains_ = maxGrains;
        numChannels_ = 2;
        resetBuffers_ = true;
        freezeLp_ = 0.0f;
        dryWet_ = 0.0f;

        for (int i = 0; i < 2; ++i)
            fbFilter_[i].init();

        reverb_.init(hostSampleRate);
        diffuser_.init();

        prepareBuffers();
    }

    //==============================================================================
    void process(float* leftIn, float* rightIn,
                 float* leftOut, float* rightOut,
                 const HtwParameters& params,
                 int numSamples)
    {
        if (resetBuffers_) {
            prepareBuffers();
        }

        int remaining = numSamples;
        int offset = 0;

        while (remaining > 0) {
            int blockSize = std::min(remaining, kMaxBlockSize);
            processBlock(leftIn + offset, rightIn + offset,
                        leftOut + offset, rightOut + offset,
                        params, blockSize);
            offset += blockSize;
            remaining -= blockSize;
        }
    }

private:
    //==============================================================================
    void prepareBuffers()
    {
        int bufferSamples = static_cast<int>(hostSampleRate_ * 4.0f);
        buffer16_[0].init(bufferSamples);
        buffer16_[1].init(bufferSamples);

        player_.init(numChannels_, maxGrains_);

        reverb_.init(hostSampleRate_);
        diffuser_.init();

        std::memset(fb_, 0, sizeof(fb_));

        dryWet_ = 0.0f;
        freezeLp_ = 0.0f;

        for (int i = 0; i < 2; ++i)
            fbFilter_[i].init();

        resetBuffers_ = false;
    }

    //==============================================================================
    void processBlock(float* leftIn, float* rightIn,
                      float* leftOut, float* rightOut,
                      const HtwParameters& params,
                      int blockSize)
    {
        FloatFrame in[kMaxBlockSize];
        FloatFrame out[kMaxBlockSize];

        for (int i = 0; i < blockSize; ++i) {
            in[i].l = leftIn[i];
            in[i].r = rightIn[i];
        }

        // Feedback with HP filtering
        onePole(freezeLp_, params.freeze ? 1.0f : 0.0f, 0.0005f);
        float feedback = params.feedback;
        float cutoff = (20.0f + 100.0f * feedback * feedback) / hostSampleRate_;
        fbFilter_[0].setFQ(cutoff, 1.0f);
        fbFilter_[1].copyFrom(fbFilter_[0]);
        fbFilter_[0].processHighPass(&fb_[0].l, static_cast<size_t>(blockSize), 2);
        fbFilter_[1].processHighPass(&fb_[0].r, static_cast<size_t>(blockSize), 2);

        float fbGain = feedback * (1.0f - freezeLp_);
        for (int i = 0; i < blockSize; ++i) {
            in[i].l += fbGain * (softLimit(fbGain * 1.4f * fb_[i].l + in[i].l) - in[i].l);
            in[i].r += fbGain * (softLimit(fbGain * 1.4f * fb_[i].r + in[i].r) - in[i].r);
        }

        // Write to buffer and process grains
        writeToBuffer(in, static_cast<size_t>(blockSize), !params.freeze);

        // Derive granular parameters
        HtwParameters derivedParams = params;
        derivedParams.granular.useDeterministicSeed = params.density < 0.5f;
        if (params.density >= 0.53f)
            derivedParams.granular.overlap = (params.density - 0.53f) * 2.12f;
        else if (params.density <= 0.47f)
            derivedParams.granular.overlap = (0.47f - params.density) * 2.12f;
        else
            derivedParams.granular.overlap = 0.0f;

        derivedParams.granular.windowShape = params.texture < 0.75f
            ? params.texture * 1.333f : 1.0f;

        player_.play(buffer16_, derivedParams, &out[0].l, static_cast<size_t>(blockSize));

        // Diffusion (texture-driven)
        float texture = params.texture;
        float diffusion = (texture > 0.75f) ? (texture - 0.75f) * 4.0f : 0.0f;
        diffuser_.setAmount(diffusion);
        diffuser_.process(out, static_cast<size_t>(blockSize));

        // Save feedback buffer (before reverb)
        std::copy(&out[0], &out[blockSize], &fb_[0]);

        // Reverb
        float reverbAmount = params.reverb * 0.95f;
        reverbAmount += feedback * (2.0f - feedback) * freezeLp_;
        reverbAmount = constrain(reverbAmount, 0.0f, 1.0f);

        reverb_.setAmount(reverbAmount * 0.54f);
        reverb_.setDiffusion(0.7f);
        reverb_.setTime(0.35f + 0.63f * reverbAmount);
        reverb_.setInputGain(0.2f);
        reverb_.setLP(0.6f + 0.37f * feedback);
        reverb_.process(out, static_cast<size_t>(blockSize));

        // Dry/wet mix with equal-power crossfade + soft limiter
        ParameterInterpolator dryWetMod(&dryWet_, params.dryWet, static_cast<size_t>(blockSize));
        for (int i = 0; i < blockSize; ++i) {
            float dw = dryWetMod.next();
            float fadeIn = LUT::xfadeIn(dw);
            float fadeOut = LUT::xfadeOut(dw);
            float wetL = std::tanh(out[i].l) * fadeIn;
            float wetR = std::tanh(out[i].r) * fadeIn;
            leftOut[i]  = leftIn[i] * fadeOut + wetL;
            rightOut[i] = rightIn[i] * fadeOut + wetR;
        }
    }

    //==============================================================================
    void writeToBuffer(const FloatFrame* input, size_t size, bool doWrite)
    {
        const float* inputSamples = &input[0].l;
        for (int ch = 0; ch < numChannels_; ++ch) {
            buffer16_[ch].writeFade(inputSamples + ch,
                static_cast<int32_t>(size), 2, doWrite);
        }
    }

    //==============================================================================
    float hostSampleRate_ = 44100.0f;
    int maxGrains_ = 32;
    int numChannels_ = 2;
    bool resetBuffers_ = true;

    float freezeLp_ = 0.0f;
    float dryWet_ = 0.0f;

    // Audio buffers (stereo, 16-bit only)
    AudioBuffer16 buffer16_[2];

    // Feedback buffer
    FloatFrame fb_[kMaxBlockSize] = {};

    // DSP modules
    GranularPlayer player_;
    HtwReverb reverb_;
    HtwDiffuser diffuser_;

    // Filters
    SVF fbFilter_[2];
};

} // namespace htw
