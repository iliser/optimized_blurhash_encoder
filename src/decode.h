#pragma once

#include <string_view>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>

#include <optional>
#include <valarray>

/*
	decode : Returns the pixel array of the result image given the blurhash string,
	Parameters : 
		blurhash : A string representing the blurhash to be decoded.
		width : Width of the resulting image
		height : Height of the resulting image
		punch : The factor to improve the contrast, default = 1
		nChannels : Number of channels in the resulting image array, 3 = RGB, 4 = RGBA
	Returns : A pointer to memory region where pixels are stored in (H, W, C) format
*/
std::optional<std::valarray<uint8_t>> decode(std::string_view blurhash, int width, int height, int punch, int nChannels);
