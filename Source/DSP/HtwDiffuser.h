/*
  ==============================================================================
    HtwDiffuser.h
    Allpass diffusion network — ported from MistDiffuser.h
  ==============================================================================
*/

#pragma once

#include "FxEngine.h"

namespace htw {

class HtwDiffuser
{
public:
    HtwDiffuser() = default;

    void init()
    {
        engine_.init();
    }

    void process(FloatFrame* inOut, size_t size)
    {
        using Memory = typename E::template Reserve<126,
            typename E::template Reserve<180,
            typename E::template Reserve<269,
            typename E::template Reserve<444,
            typename E::template Reserve<151,
            typename E::template Reserve<205,
            typename E::template Reserve<245,
            typename E::template Reserve<405>>>>>>>>;

        typename E::template DelayLine<Memory, 0> apl1;
        typename E::template DelayLine<Memory, 1> apl2;
        typename E::template DelayLine<Memory, 2> apl3;
        typename E::template DelayLine<Memory, 3> apl4;
        typename E::template DelayLine<Memory, 4> apr1;
        typename E::template DelayLine<Memory, 5> apr2;
        typename E::template DelayLine<Memory, 6> apr3;
        typename E::template DelayLine<Memory, 7> apr4;
        typename E::Context c;

        const float kap = 0.625f;

        while (size--) {
            engine_.start(&c);

            float wet = 0.0f;
            c.read(inOut->l);
            c.readDelay(apl1, -1, kap);
            c.writeAllPass(apl1, -kap);
            c.readDelay(apl2, -1, kap);
            c.writeAllPass(apl2, -kap);
            c.readDelay(apl3, -1, kap);
            c.writeAllPass(apl3, -kap);
            c.readDelay(apl4, -1, kap);
            c.writeAllPass(apl4, -kap);
            c.write(wet, 0.0f);
            inOut->l += amount_ * (wet - inOut->l);

            c.read(inOut->r);
            c.readDelay(apr1, -1, kap);
            c.writeAllPass(apr1, -kap);
            c.readDelay(apr2, -1, kap);
            c.writeAllPass(apr2, -kap);
            c.readDelay(apr3, -1, kap);
            c.writeAllPass(apr3, -kap);
            c.readDelay(apr4, -1, kap);
            c.writeAllPass(apr4, -kap);
            c.write(wet, 0.0f);
            inOut->r += amount_ * (wet - inOut->r);

            ++inOut;
        }
    }

    void setAmount(float amount) { amount_ = amount; }

private:
    using E = FxEngine<2048>;
    E engine_;
    float amount_ = 0.0f;
};

} // namespace htw
