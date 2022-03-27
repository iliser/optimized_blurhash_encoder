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

                float r = 0, g = 0, b = 0;

                float xm = M_PI * x / width;
                float ym = M_PI * y / height;

                auto factor_iter = std::begin(factors);

                for (size_t j = 0; j < numY; ++j)
                {
                    for (size_t i = 0; i < numX; ++i)
                    {
                        float basics = fast_cos(xm * i) * fast_cos(ym * j);
                        auto &factor = *(factor_iter++);

                        r += factor.r * basics;
                        g += factor.g * basics;
                        b += factor.b * basics;
                    }
                }

                *(iter++) = std::clamp(linearTosRGB(r), 0, 255);
                *(iter++) = std::clamp(linearTosRGB(g), 0, 255);
                *(iter++) = std::clamp(linearTosRGB(b), 0, 255);

                if (nChannels == 4)
                    ++iter;
            }
        }
        return ret;
    }
    return std::nullopt;
}