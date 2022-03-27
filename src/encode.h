#ifndef __BLURHASH_ENCODE_H__
#define __BLURHASH_ENCODE_H__

#include <stdint.h>
#include <stdlib.h>
#include <string>
#include <optional>

const std::optional<std::string> blurHashForPixels(size_t xComponents, size_t yComponents, size_t width, size_t height, const uint8_t *rgb);

#endif
