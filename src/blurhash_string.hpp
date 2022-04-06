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

class BlurhashWriter
{
    std::string buffer;
    std::back_insert_iterator<std::string> iter;

    float max = 1;

    void writeInt(int value, int len)
    {
        int divisor = std::pow(83, len - 1);

        while (len--)
        {
            *(++iter) = characters[(value / divisor) % 83];
            divisor /= 83;
        }
    }
    int encodeDC(const LinearColor &clr) { return linearTosRGBColor(clr); }

    int encodeAC(const LinearColor &clr, float maximumValue)
    {
        const auto normalize = [=](float v)
        {
            return std::clamp((int)std::floor(signPow(v / maximumValue, 0.5) * 9 + 9.5), 0, 18);
        };

        return normalize(clr.r) * 19 * 19 + normalize(clr.g) * 19 + normalize(clr.b);
    }

public:
    BlurhashWriter() : iter(buffer) {}

    void writeSize(size_t w, size_t h) { writeInt((w - 1) + (h - 1) * 9, 1); }
    void wirteMax(float max)
    {

        int qmv = std::clamp((int)floorf(max * 166 - 0.5), 0, 82);
        writeInt(qmv, 1);
        if (max != 0)
            this->max = ((float)qmv + 1) / 166;
    }
    void writeDC(const LinearColor &factor) { writeInt(encodeDC(factor), 4); }
    void writeAC(const LinearColor &factor) { writeInt(encodeAC(factor, max), 2); }

    std::string result() { return buffer; }
};

static std::string
encodeFactors(
    const std::valarray<LinearColor> &factors,
    size_t xComponents, size_t yComponents)
{

    BlurhashWriter writer;
    int acCount = xComponents * yComponents - 1;

    // write factors count `size`
    writer.writeSize(xComponents, yComponents);
    const auto acFactors = factors | std::views::drop(1);

    // write maximum value
    writer.wirteMax(acCount > 0 ? std::ranges::max(acFactors, {}, &LinearColor::max).max() : 0);

    // write initial factors `DC`
    writer.writeDC(factors[0]);

    // write rest factors `AC`

    for (const auto &val : acFactors)
        writer.writeAC(val);

    return writer.result();
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

    LinearColor decodeDC(int value)
    {
        return {
            .r = sRGBToLinear(value >> 16),        // R-component
            .g = sRGBToLinear((value >> 8) & 255), // G-Component
            .b = sRGBToLinear(value & 255),        // B-Component
        };
    }

    LinearColor decodeAC(int value, float maximumValue)
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
    std::optional<LinearColor> readDC()
    {
        if (auto value = readInt(4))
            return decodeDC(*value);
        else
            return std::nullopt;
    }

    std::optional<LinearColor> readAC(float maximumValue)
    {
        if (auto value = readInt(2))
            return decodeAC(*value, maximumValue);
        else
            return std::nullopt;
    }
};

std::optional<std::tuple<std::valarray<LinearColor>, size_t, size_t>>
decodeFactors(std::string_view blurhash, int punch)
{
    try
    {
        if (punch < 1)
            punch = 1;

        BlurhashReader reader{std::string{blurhash}};

        const int hashLength = blurhash.length();

        if (hashLength < 6)
            return std::nullopt;

        auto [numX, numY] = reader.readSize().value();

        // validate blurhash code
        if (hashLength != 4 + 2 * numX * numY)
            return std::nullopt;

        float maxValue = reader.readMaxValue().value();

        std::valarray<LinearColor> factors(numX * numY);

        if (auto value = reader.readDC())
            factors[0] = *value;
        else
            return std::nullopt;

        for (auto &factor : factors | std::views::drop(1))
        {
            factor = reader.readAC(maxValue * punch).value();
        }

        return std::tuple{factors, numX, numY};
    }
    catch (const std::bad_optional_access &ex)
    {
        return std::nullopt;
    }
}
