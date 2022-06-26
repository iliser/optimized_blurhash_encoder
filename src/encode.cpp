#include "encode.h"
#include "color_models.hpp"

#include <valarray>
#include <iostream>

#include "fast_cos.hpp"
#include "blurhash_string.hpp"

inline auto buildCosCache(size_t components, size_t size)
{
    std::valarray<float> cache(size * components);

    for (size_t i = 0; i < size; ++i)
    {
        float xf = M_PI * i / size;
        for (size_t c = 0; c < components; ++c)
        {
            cache[i * components + c] = cosf(xf * c);
        }
    }
    return cache;
}

std::valarray<LinearColor> calculateFactors(
    size_t xComponents, size_t yComponents,
    size_t width, size_t height,
    const uint8_t *rgb)
{
    if (xComponents < 1 || xComponents > 9 || yComponents < 1 || yComponents > 9)
        throw "";

    std::valarray<LinearColor> factors(yComponents * xComponents);

    auto rgb_ptr = rgb;

    auto xf_cos_cache = buildCosCache(xComponents, width);
    auto yf_cos_cache = buildCosCache(yComponents, height);

    size_t samplingFactor = 4;

    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            LinearColor color = LinearColor::fromRGBPtr(rgb_ptr);
            rgb_ptr += 3;

            auto ptr = std::begin(factors);
            for (size_t yc = 0; yc < yComponents; ++yc)
            {
                for (size_t xc = 0; xc < xComponents; ++xc)
                {
                    float basis = xf_cos_cache[x * xComponents + xc] * yf_cos_cache[y * yComponents + yc];

                    *ptr = *ptr + color * basis;

                    ++ptr;
                }
            }
        }
    }

    // normalization step, before optimization it placed in the end of
    // multiplyBasisFunction
    auto ptr = std::begin(factors);
    for (size_t yc = 0; yc < yComponents; yc++)
    {
        for (size_t xc = 0; xc < xComponents; xc++)
        {
            float normalisation = (xc == 0 && yc == 0) ? 1 : 2;
            float scale = normalisation / (width * height);

            *ptr = *ptr * scale;

            ++ptr;
        }
    }

    return factors;
}

const std::optional<std::string> blurHashForPixels(
    size_t xComponents, size_t yComponents,
    size_t width, size_t height,
    const uint8_t *rgb)
{
    // calculate factors
    if (xComponents < 1 || xComponents > 9 || yComponents < 1 || yComponents > 9)
        return std::nullopt;

    auto factors = calculateFactors(xComponents, yComponents, width, height, rgb);
    return encodeFactors(factors, xComponents, yComponents);
}
