#include "encode.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <optional>
#include <stdio.h>
#include <iostream>

const std::optional<std::string>
blurHashForFile(size_t xComponents, size_t yComponents, std::string_view filename)
{
    int width, height, channels;
    unsigned char *data = stbi_load(filename.data(), &width, &height, &channels, 3);
    if (!data)
        return std::nullopt;

    const auto hash = blurHashForPixels(xComponents, yComponents, width, height, data);

    stbi_image_free(data);

    return hash;
}

int main(int argc, const char **argv)
{
    if (argc != 4)
    {
        std::cout << "Usage: " << argv[0] << " x_components y_components imagefile\n";
        return 1;
    }

    int xComponents = std::stoi(argv[1]);
    int yComponents = std::stoi(argv[2]);
    if (xComponents < 1 || xComponents > 8 || yComponents < 1 ||
        yComponents > 8)
    {
        std::cerr << "Component counts must be between 1 and 8.\n";
        return 1;
    }

    const auto hash = blurHashForFile(xComponents, yComponents, argv[3]);
    if (!hash)
    {
        std::cerr << "Failed to load image file \"" << argv[3] << "\".\n";
        return 1;
    }

    std::cout << *hash << '\n';

    return 0;
}
