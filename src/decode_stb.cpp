#include "decode.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_writer.h"

#include <iostream>

int main(int argc, char **argv)
{
    if (argc < 5)
    {
        std::cerr << "Usage: " << argv[0] << " hash width height output_file [punch]\n";
        return 1;
    }

    int width, height, punch = 1;
    std::string_view hash = argv[1];
    width = std::stoi(argv[2]);
    height = std::stoi(argv[3]);
    std::string_view output_file = argv[4];

    const int nChannels = 4;

    if (argc == 6)
        punch = std::stoi(argv[5]);

    auto bytes = decode(hash, width, height, punch, nChannels);

    if (!bytes)
    {
        std::cerr << hash << " is not a valid blurhash, decoding failed.\n";
        return 1;
    }

    if (stbi_write_png(output_file.data(), width, height, nChannels, &(*bytes)[0], nChannels * width) == 0)
    {
        std::cerr << "Failed to write PNG file " << output_file << "\n";
        return 1;
    }

    return 0;
}
