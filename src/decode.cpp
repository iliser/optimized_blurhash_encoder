#include "decode.h"
#include "color_models.hpp"
#include <math.h>
#include <iostream>
#include "blurhash_string.hpp"

#include "fast_cos.hpp"
#include <map>

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


std::optional<std::valarray<uint8_t>> decode(
    std::string_view blurhash,
    int width, int height,
    int punch, int nChannels)
{
    if (auto res = decodeFactors(blurhash, punch))
    {
        auto ret = std::valarray<uint8_t>(255, width * height * nChannels);
        const auto [factors, xComponents, yComponents] = *res;

        auto iter = std::begin(ret);

        auto xf_cos_cache = buildCosCache(xComponents, width);
        auto yf_cos_cache = buildCosCache(yComponents, height);

        for (size_t y = 0; y < height; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {

                LinearColor color{0, 0, 0};

                float xf = M_PI * x / width;
                float yf = M_PI * y / height;

                auto factor_iter = std::begin(factors);

                for (size_t yc = 0; yc < yComponents; ++yc)
                {

                    for (size_t xc = 0; xc < xComponents; ++xc)
                    {
                        float basis = xf_cos_cache[x * xComponents + xc] * yf_cos_cache[y * yComponents + yc];
                        // float basis = fast_cos(xf * xc) * fast_cos(yf * yc);
                        auto &factor = *(factor_iter++);

                        color = color + factor * basis;
                    }
                }
                auto rgb = color.toRGBColor();
                *(iter++) = rgb.r;
                *(iter++) = rgb.g;
                *(iter++) = rgb.b;

                if (nChannels == 4)
                    ++iter;
            }
        }
        return ret;
    }
    return std::nullopt;
}