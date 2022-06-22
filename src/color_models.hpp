#pragma once

#include <math.h>
#include <algorithm>

#include "color_convert_utilities.hpp"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static inline float signPow(float value, float exp)
{
    return copysignf(powf(fabsf(value), exp), value);
}

struct RGBColor
{
    uint8_t r, g, b;

    // operator int() remove implicit type conversion
    int toInt() const
    {
        return (r << 16) + (g << 8) + b;
    }
};

struct LinearColor 
{
    float r, g, b;

    inline float max() const
    {
        return std::max({r, g, b});
    }

    bool operator<(const LinearColor &o) const
    {
        return max() < o.max();
    }

    LinearColor operator+(const LinearColor &o) const
    {
        return {r + o.r, g + o.g, b + o.b};
    }

    LinearColor operator*(float m) const
    {
        return {r * m, g * m, b * m};
    }

    inline static LinearColor fromRGBPtr(const uint8_t values[3])
    {
        return {
            srgb_to_linear_table[values[0]],
            srgb_to_linear_table[values[1]],
            srgb_to_linear_table[values[2]],
        };
    }

    inline RGBColor toRGBColor() const
    {
        return {
            linearTosRGB(this->r),
            linearTosRGB(this->g),
            linearTosRGB(this->b),
        };
    }
};

LinearColor linearColorFromRGBPtr(const uint8_t values[3])
    {
        return {
            srgb_to_linear_table[values[0]],
            srgb_to_linear_table[values[1]],
            srgb_to_linear_table[values[2]],
        };
    }
