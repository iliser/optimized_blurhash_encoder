#include "encode.h"
#include "common.hpp"

#include <valarray>

#include "fast_cos.hpp"
#include "blurhash_string.hpp"

std::valarray<Factor> calculateFactors(
    size_t xComponents, size_t yComponents,
    size_t width, size_t height,
    const uint8_t *rgb)
{
    // allow g++ to unroll the inner loop or do something like this,
    // give x2 speed boost lol
    if (xComponents < 1 || xComponents > 9 || yComponents < 1 || yComponents > 9)throw "";

    std::valarray<Factor> factors(yComponents * xComponents);
    
    for (size_t y = 0; y < height; ++y)
    {
        for (size_t x = 0; x < width; ++x)
        {
            // with loop revert this convert operation execute only once per channel
            // vs yComponents * xComponents times before
            float r = sRGBToLinear(rgb[y * width * 3 + 3 * x + 0]);
            float g = sRGBToLinear(rgb[y * width * 3 + 3 * x + 1]);
            float b = sRGBToLinear(rgb[y * width * 3 + 3 * x + 2]);

            // extract this factor from inner loop give another ~30% boost
            float yf = M_PI * y / height;
            float xf = M_PI * x / width;

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
                    ptr->r += basis * r;
                    ptr->g += basis * g;
                    ptr->b += basis * b;

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

            ptr->r *= scale;
            ptr->g *= scale;
            ptr->b *= scale;

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

    auto factors = calculateFactors(xComponents,yComponents,width,height,rgb);
    return encodeFactors(factors, xComponents, yComponents);
}
