#include "decode.h"
#include "common.hpp"
#include <math.h>

#include "blurhash_string.hpp"

#include "fast_cos.hpp"

std::optional<std::valarray<uint8_t>> decode(
    std::string_view blurhash,
    int width, int height,
    int punch, int nChannels)
{
    if (auto res = decodeFactors(blurhash, punch))
    {
        auto ret = std::valarray<uint8_t>(255, width * height * nChannels);
        const auto [factors, numX, numY] = *res;

        auto iter = std::begin(ret);

        for (size_t y = 0; y < height; ++y)
        {
            for (size_t x = 0; x < width; ++x)
            {

                Factor color{0,0,0};

                float xm = M_PI * x / width;
                float ym = M_PI * y / height;

                auto factor_iter = std::begin(factors);

                for (size_t j = 0; j < numY; ++j)
                {
                    for (size_t i = 0; i < numX; ++i)
                    {
                        float basis = fast_cos(xm * i) * fast_cos(ym * j);
                        auto &factor = *(factor_iter++);

                        color = color + factor * basis;
                    }
                }
                auto rgb = linearTosRGBFactor(color);
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