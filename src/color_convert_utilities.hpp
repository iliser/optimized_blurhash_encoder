#pragma once 

#include <cmath>
#include <array>

static inline float sRGBToLinearValue(int value)
{
    float v = (float)value / 255;
    if (v <= 0.04045)
        return v / 12.92;
    else
        return std::pow((v + 0.055) / 1.055, 2.4);
}

// convert linear color component to rgb
static inline uint8_t linearTosRGB(float value)
{
    float v = fmaxf(0, fminf(1, value));
    int ret = (v <= 0.0031308) ? v * 12.92 * 255 + 0.5 : (1.055 * powf(v, 1 / 2.4) - 0.055) * 255 + 0.5;
    return std::clamp<int>(ret, 0, 255);
}

// initialize srgb_to_linear_color_table for speed up conversion
static auto _initArray()
{
    std::array<float, 256> array;
    for (size_t i = 0; i < 256; ++i)
    {
        array[i] = sRGBToLinearValue(i);
    }
    return array;
}

static std::array<float, 256> srgb_to_linear_table = _initArray();

static inline float sRGBToLinear(int value)
{
    return srgb_to_linear_table[value];
}

