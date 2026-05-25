/*
  ==============================================================================
    FxEngine.h
    Delay-line based FX engine for HowsTheWeather
    Ported from Mist_VST/Clouds FxEngine — used by Reverb and Diffuser
  ==============================================================================
*/

#pragma once

#include "HtwDSPCommon.h"
#include <cstring>

namespace htw {

template<size_t size>
class FxEngine
{
public:
    FxEngine() = default;

    void init()
    {
        std::memset(buffer_, 0, sizeof(buffer_));
        writePtr_ = 0;
    }

    void clear()
    {
        std::memset(buffer_, 0, sizeof(buffer_));
    }

    template<int32_t l, typename T = void>
    struct Reserve
    {
        using Tail = T;
        static constexpr int32_t length = l;
    };

    template<typename Memory, int32_t index>
    struct DelayLine
    {
        static constexpr int32_t length = DelayLine<typename Memory::Tail, index - 1>::length;
        static constexpr int32_t base = DelayLine<Memory, index - 1>::base
                                      + DelayLine<Memory, index - 1>::length + 1;
    };

    template<typename Memory>
    struct DelayLine<Memory, 0>
    {
        static constexpr int32_t length = Memory::length;
        static constexpr int32_t base = 0;
    };

    //==============================================================================
    class Context
    {
        friend class FxEngine;
    public:
        Context() = default;

        void load(float value) { accumulator_ = value; }
        void read(float value, float scale) { accumulator_ += value * scale; }
        void read(float value) { accumulator_ += value; }
        void write(float& value) { value = accumulator_; }
        void write(float& value, float scale) { value = accumulator_; accumulator_ *= scale; }

        template<typename D>
        void write(D&, int32_t offset, float scale)
        {
            int32_t idx;
            if (offset == -1)
                idx = (writePtr_ + D::base + D::length - 1) & MASK;
            else
                idx = (writePtr_ + D::base + offset) & MASK;
            buffer_[idx] = accumulator_;
            accumulator_ *= scale;
        }

        template<typename D>
        void write(D& d, float scale)
        {
            write(d, 0, scale);
        }

        template<typename D>
        void writeAllPass(D& d, int32_t offset, float scale)
        {
            write(d, offset, scale);
            accumulator_ += previousRead_;
        }

        template<typename D>
        void writeAllPass(D& d, float scale)
        {
            writeAllPass(d, 0, scale);
        }

        template<typename D>
        void readDelay(D&, int32_t offset, float scale)
        {
            int32_t idx;
            if (offset == -1)
                idx = (writePtr_ + D::base + D::length - 1) & MASK;
            else
                idx = (writePtr_ + D::base + offset) & MASK;
            float r = buffer_[idx];
            previousRead_ = r;
            accumulator_ += r * scale;
        }

        template<typename D>
        void readDelay(D& d, float scale)
        {
            readDelay(d, 0, scale);
        }

        void lp(float& state, float coefficient)
        {
            state += coefficient * (accumulator_ - state);
            accumulator_ = state;
        }

        void hp(float& state, float coefficient)
        {
            state += coefficient * (accumulator_ - state);
            accumulator_ -= state;
        }

        template<typename D>
        void interpolate(D&, float offset, float scale)
        {
            int32_t integral = static_cast<int32_t>(offset);
            float fractional = offset - static_cast<float>(integral);
            float a = buffer_[(writePtr_ + integral + D::base) & MASK];
            float b = buffer_[(writePtr_ + integral + D::base + 1) & MASK];
            float x = a + (b - a) * fractional;
            previousRead_ = x;
            accumulator_ += x * scale;
        }

        template<typename D>
        void interpolate(D&, float offset, int lfoIndex, float amplitude, float scale)
        {
            offset += amplitude * lfoValue_[lfoIndex];
            int32_t integral = static_cast<int32_t>(offset);
            float fractional = offset - static_cast<float>(integral);
            float a = buffer_[(writePtr_ + integral + D::base) & MASK];
            float b = buffer_[(writePtr_ + integral + D::base + 1) & MASK];
            float x = a + (b - a) * fractional;
            previousRead_ = x;
            accumulator_ += x * scale;
        }

    private:
        float accumulator_ = 0.0f;
        float previousRead_ = 0.0f;
        float lfoValue_[2] = {0.0f, 0.0f};
        float* buffer_ = nullptr;
        int32_t writePtr_ = 0;
    };

    //==============================================================================
    void setLFOFrequency(int index, float frequency)
    {
        lfo_[index].init(frequency * 32.0f);
    }

    void start(Context* c)
    {
        --writePtr_;
        if (writePtr_ < 0)
            writePtr_ += static_cast<int32_t>(size);

        c->accumulator_ = 0.0f;
        c->previousRead_ = 0.0f;
        c->buffer_ = buffer_;
        c->writePtr_ = writePtr_;

        if ((writePtr_ & 31) == 0) {
            c->lfoValue_[0] = lfo_[0].next();
            c->lfoValue_[1] = lfo_[1].next();
        } else {
            c->lfoValue_[0] = lfo_[0].value();
            c->lfoValue_[1] = lfo_[1].value();
        }
    }

private:
    static constexpr int32_t MASK = static_cast<int32_t>(size) - 1;

    int32_t writePtr_ = 0;
    float buffer_[size] = {};
    CosineOscillator lfo_[2];
};

} // namespace htw
