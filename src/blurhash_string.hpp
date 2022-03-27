#pragma once

#include "common.hpp"

#include <valarray>
#include <algorithm>
#include <ranges>

#include "fast_cos.hpp"

static char characters[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmno"
                           "pqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~";

static std::string intToBase83(int value, int length)
{
    int divisor = std::pow(83, length - 1);

    std::string ret(length, ' ');

    for (auto &c : ret)
    {
        c = characters[(value / divisor) % 83];
        divisor /= 83;
    }
    return ret;
}

static int encodeDC(const Factor &clr)
{
    int roundedR = linearTosRGB(clr.r);
    int roundedG = linearTosRGB(clr.g);
    int roundedB = linearTosRGB(clr.b);
    return (roundedR << 16) + (roundedG << 8) + roundedB;
}

static int encodeAC(const Factor &clr, float maximumValue)
{
    const auto normalize = [=](float v)
    {
        return std::clamp((int)std::floor(signPow(v / maximumValue, 0.5) * 9 + 9.5), 0, 18);
    };

    return normalize(clr.r) * 19 * 19 + normalize(clr.g) * 19 + normalize(clr.b);
}

static std::string encodeBlurhash(
    const std::valarray<Factor> &factors,
    size_t xComponents, size_t yComponents)
{
    int acCount = xComponents * yComponents - 1;

    // write factors count `size`
    std::string ret = intToBase83((xComponents - 1) + (yComponents - 1) * 9, 1);
    const auto acFactors = factors | std::views::drop(1);
    float maximumValue = 1;
    int quantisedMaximumValue = 0;
    if (acCount > 0)
    {

        float actualMaximumValue = std::ranges::max(acFactors, {}, &Factor::max).max();

        quantisedMaximumValue = std::clamp((int)floorf(actualMaximumValue * 166 - 0.5), 0, 82);
        maximumValue = ((float)quantisedMaximumValue + 1) / 166;
    }
    // write maximum value
    ret += intToBase83(quantisedMaximumValue, 1);

    // write initial factors `DC`
    ret += intToBase83(encodeDC(factors[0]), 4);

    // write rest factors `AC`

    for (const auto &val : acFactors)
    {
        ret += intToBase83(encodeAC(val, maximumValue), 2);
    }

    return ret;
};
