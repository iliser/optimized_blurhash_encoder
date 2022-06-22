#include "encode.h"
#include "color_models.hpp"

#include <valarray>

#include "fast_cos.hpp"
#include "blurhash_string.hpp"

std::valarray<LinearColor> calculateFactors(
    size_t xComponents, size_t yComponents,
    size_t width, size_t height,
    const uint8_t *rgb)
{
    // allow g++ to unroll the inner loop or do something like this,
    // give x2 speed boost lol
    if (xComponents < 1 || xComponents > 9 || yComponents < 1 || yComponents > 9)
        throw "";

    std::valarray<LinearColor> factors(yComponents * xComponents);
    auto rgb_ptr = rgb;

    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            // with loop revert this convert operation execute only once per channel
            // vs yComponents * xComponents times before
            LinearColor color = LinearColor::fromRGBPtr(rgb_ptr);
            rgb_ptr += 3;

            // extract this factor from inner loop give another ~30% boost
            float xf = M_PI * x / width;
            float yf = M_PI * y / height;

            // order of internal loop does't care cause of `cos` is `high cost`
            // operation

            auto ptr = std::begin(factors);

            for (size_t yc = 0; yc < yComponents; ++yc)
            {
                for (size_t xc = 0; xc < xComponents; ++xc)
                {
                    float basis = fast_cos(xf * xc) * fast_cos(yf * yc);

                    // access through ptr cause access from factors[id(xc,yc,c)] 2 times
                    // slower cause of arithmetics
                    *ptr = *ptr + color * basis;
                    // ptr->g += basis * g;
                    // ptr->b += basis * b;

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
