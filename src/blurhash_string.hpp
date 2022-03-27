#pragma once

#include "common.hpp"

#include <valarray>
#include <algorithm>
#include <ranges>
#include <optional>

#include "fast_cos.hpp"

static char characters[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmno"
                           "pqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~";

static auto fillCharacterId()
{
    std::array<int, 256> characterId;
    characterId.fill(-1);

    for (const auto id : std::views::iota(size_t(0), sizeof(characters)))
    {
        characterId[characters[id]] = id;
    }
    return characterId;
}
static std::array<int, 256> characterId = fillCharacterId();

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

static std::string encodeFactors(
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

std::optional<int> decodeToInt(std::string_view string, int start, int end)
{
    int value = 0;
    for (int i = start; i < end; i++)
    {
        int index = characterId[string[i]];
        if (index < 0)
            return std::nullopt;
        value = value * 83 + index;
    }
    return value;
}

bool isValidBlurhash(std::string_view blurhash)
{

    const int hashLength = blurhash.length();

    if (hashLength < 6)
        return false;

    auto sizeFlag = decodeToInt(blurhash, 0, 1); // Get size from first character
    if (!sizeFlag)
        return false;
    int numY = (int)floorf(*sizeFlag / 9) + 1;
    int numX = (*sizeFlag % 9) + 1;

    if (hashLength != 4 + 2 * numX * numY)
        return false;
    return true;
}

Factor decodeDC(int value)
{
    return {
        .r = sRGBToLinear(value >> 16),        // R-component
        .g = sRGBToLinear((value >> 8) & 255), // G-Component
        .b = sRGBToLinear(value & 255),        // B-Component
    };
}

Factor decodeAC(int value, float maximumValue)
{
    int quantR = (int)floorf(value / (19 * 19));
    int quantG = (int)floorf(value / 19) % 19;
    int quantB = (int)value % 19;

    return {
        .r = signPow(((float)quantR - 9) / 9, 2.0) * maximumValue,
        .g = signPow(((float)quantG - 9) / 9, 2.0) * maximumValue,
        .b = signPow(((float)quantB - 9) / 9, 2.0) * maximumValue,
    };
}

class BlurhashReader
{
    std::string view;
    typedef std::string::const_iterator iter_type;
    iter_type iter;

    std::optional<int> readInt(size_t len)
    {
        int value = 0;
        while (len--)
        {
            int index = characterId[*iter];
            if (index < 0)
                return std::nullopt;
            value = value * 83 + index;
            ++iter;
        }
        return value;
    }

public:
    BlurhashReader(std::string blurhash) : view(blurhash), iter(view.cbegin()) {}

    std::optional<std::tuple<size_t, size_t>> readSize()
    {
        if (auto sf = readInt(1))
            return std::tuple{(*sf % 9) + 1, (int)floorf(*sf / 9) + 1};

        return std::nullopt;
    }

    std::optional<float> readMaxValue()
    {
        if (auto qmv = readInt(1))
            return ((float)(*qmv + 1)) / 166;

        else
            return std::nullopt;
    }
    std::optional<Factor> readDC()
    {
        if (auto value = readInt(4))
            return decodeDC(*value);
        else
            return std::nullopt;
    }

    std::optional<Factor> readAC(float maximumValue)
    {
        if (auto value = readInt(2))
            return decodeAC(*value, maximumValue);
        else
            return std::nullopt;
    }
};

std::optional<std::tuple<std::valarray<Factor>, size_t, size_t>>
decodeFactors(std::string_view blurhash, int punch)
{
    if (!isValidBlurhash(blurhash))
        return std::nullopt;
    if (punch < 1)
        punch = 1;

    BlurhashReader reader{std::string{blurhash}};

    size_t numX, numY;
    if (auto size = reader.readSize())
        std::tie(numX, numY) = *size;
    else
        return std::nullopt;

    float maxValue;
    if (auto mv = reader.readMaxValue())
        maxValue = *mv;
    else
        return std::nullopt;

    int iter = 0;

    size_t factors_size = numX * numY;
    std::valarray<Factor> factors(factors_size);

    if (auto value = reader.readDC())
        factors[0] = *value;
    else
        return std::nullopt;

    for (auto &factor : factors | std::views::drop(1))
    {
        if (auto value = reader.readAC(maxValue * punch))
            factor = *value;
        else
            return std::nullopt;
    }

    return std::tuple{factors, numX, numY};
}
