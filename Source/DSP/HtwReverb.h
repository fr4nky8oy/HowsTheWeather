/*
  ==============================================================================
    HtwReverb.h
    Dattorro plate reverb — ported from MistReverb.h
    Griesinger topology: 4 AP diffusers -> loop of 2x(2AP+1Delay)
  ==============================================================================
*/

#pragma once

#include "FxEngine.h"

namespace htw {

class HtwReverb
{
public:
    HtwReverb() = default;

    void init(float sampleRate)
    {
        sampleRate_ = sampleRate;
        engine_.init();
        engine_.setLFOFrequency(0, 0.5f / sampleRate);
        engine_.setLFOFrequency(1, 0.3f / sampleRate);
        lp_ = 0.7f;
        diffusion_ = 0.625f;
        lpDecay1_ = 0.0f;
        lpDecay2_ = 0.0f;
    }

    void process(FloatFrame* inOut, size_t size)
    {
        using Memory = typename E::template Reserve<113,
            typename E::template Reserve<162,
            typename E::template Reserve<241,
            typename E::template Reserve<399,
            typename E::template Reserve<1653,
            typename E::template Reserve<2038,
            typename E::template Reserve<3411,
            typename E::template Reserve<1913,
            typename E::template Reserve<1663,
            typename E::template Reserve<4782>>>>>>>>>>;

        typename E::template DelayLine<Memory, 0> ap1;
        typename E::template DelayLine<Memory, 1> ap2;
        typename E::template DelayLine<Memory, 2> ap3;
        typename E::template DelayLine<Memory, 3> ap4;
        typename E::template DelayLine<Memory, 4> dap1a;
        typename E::template DelayLine<Memory, 5> dap1b;
        typename E::template DelayLine<Memory, 6> del1;
        typename E::template DelayLine<Memory, 7> dap2a;
        typename E::template DelayLine<Memory, 8> dap2b;
        typename E::template DelayLine<Memory, 9> del2;
        typename E::Context c;

        const float kap = diffusion_;
        const float klp = lp_;
        const float krt = reverbTime_;
        const float amt = amount_;
        const float gain = inputGain_;

        float lp1 = lpDecay1_;
        float lp2 = lpDecay2_;

        while (size--) {
            float wet;
            float apout = 0.0f;
            engine_.start(&c);

            c.interpolate(ap1, 10.0f, 0, 60.0f, 1.0f);
            c.write(ap1, 100, 0.0f);

            c.read(inOut->l + inOut->r, gain);

            c.readDelay(ap1, -1, kap);
            c.writeAllPass(ap1, -kap);
            c.readDelay(ap2, -1, kap);
            c.writeAllPass(ap2, -kap);
            c.readDelay(ap3, -1, kap);
            c.writeAllPass(ap3, -kap);
            c.readDelay(ap4, -1, kap);
            c.writeAllPass(ap4, -kap);
            c.write(apout);

            c.load(apout);
            c.interpolate(del2, 4680.0f, 1, 100.0f, krt);
            c.lp(lp1, klp);
            c.readDelay(dap1a, -1, -kap);
            c.writeAllPass(dap1a, kap);
            c.readDelay(dap1b, -1, kap);
            c.writeAllPass(dap1b, -kap);
            c.write(del1, 2.0f);
            c.write(wet, 0.0f);

            inOut->l += (wet - inOut->l) * amt;

            c.load(apout);
            c.readDelay(del1, -1, krt);
            c.lp(lp2, klp);
            c.readDelay(dap2a, -1, kap);
            c.writeAllPass(dap2a, -kap);
            c.readDelay(dap2b, -1, -kap);
            c.writeAllPass(dap2b, kap);
            c.write(del2, 2.0f);
            c.write(wet, 0.0f);

            inOut->r += (wet - inOut->r) * amt;

            ++inOut;
        }

        lpDecay1_ = lp1;
        lpDecay2_ = lp2;
    }

    void setAmount(float amount) { amount_ = amount; }
    void setInputGain(float gain) { inputGain_ = gain; }
    void setTime(float time) { reverbTime_ = time; }
    void setDiffusion(float diffusion) { diffusion_ = diffusion; }
    void setLP(float lp) { lp_ = lp; }

private:
    using E = FxEngine<16384>;
    E engine_;

    float sampleRate_ = 32000.0f;
    float amount_ = 0.0f;
    float inputGain_ = 0.2f;
    float reverbTime_ = 0.5f;
    float diffusion_ = 0.625f;
    float lp_ = 0.7f;
    float lpDecay1_ = 0.0f;
    float lpDecay2_ = 0.0f;
};

} // namespace htw
