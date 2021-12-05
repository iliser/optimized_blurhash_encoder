#include "encode.h"
#include "common.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static float *multiplyBasisFunction(int xComponent, int yComponent, int width, int height, float *rgb, size_t bytesPerRow);
static char *encode_int(int value, int length, char *destination);

static int encodeDC(float r, float g, float b);
static int encodeAC(float r, float g, float b, float maximumValue);


const char *blurHashForPixels(int xComponents, int yComponents, int width, int height, uint8_t *rgb, size_t bytesPerRow) {
	static char buffer[2 + 4 + (9 * 9 - 1) * 2 + 1];

	if(xComponents < 1 || xComponents > 9) return NULL;
	if(yComponents < 1 || yComponents > 9) return NULL;

	float factors[yComponents][xComponents][3];
	memset(factors, 0, sizeof(factors));
	

	// all code bellow is just reordering of loop and operation

	// assembly factor pixel information 
	float r,g,b;
	for(int y = 0; y < height; ++y) {
		for(int x = 0; x < width; ++x) {
			// with loop revert this convert operation execute only once per channel vs yComponents * xComponents times before
			r = sRGBToLinear(rgb[3 * x + 0 + y * bytesPerRow]);
			g = sRGBToLinear(rgb[3 * x + 1 + y * bytesPerRow]);
			b = sRGBToLinear(rgb[3 * x + 2 + y * bytesPerRow]);
			
			// extract this factor from inner loop give another ~30% boost
			float yf = M_PI * y / height;
			float xf = M_PI * x / width;

			// order of internal loop does't care cause of `cos` is `high cost` operation
			for(int yc = 0; yc < yComponents; ++yc) {
				for(int xc = 0; xc < xComponents; ++xc) {
					float basis = cosf(xf * xc) * cosf(yf * yc );

					factors[yc][xc][0] += basis * r;
					factors[yc][xc][1] += basis * g;
					factors[yc][xc][2] += basis * b;
				}
			}
		}
	}

	// normalization step, before optimization it placed in the end of multiplyBasisFunction
	for(int yc = 0; yc < yComponents; yc++) {
		for(int xc = 0; xc < xComponents; xc++) {
			float normalisation = (xc == 0 && yc == 0) ? 1 : 2;
			float scale = normalisation / (width * height);
			factors[yc][xc][0] *= scale;
			factors[yc][xc][1] *= scale;
			factors[yc][xc][2] *= scale;
		}
	}


	float *dc = factors[0][0];
	float *ac = dc + 3;
	int acCount = xComponents * yComponents - 1;
	char *ptr = buffer;

	int sizeFlag = (xComponents - 1) + (yComponents - 1) * 9;
	ptr = encode_int(sizeFlag, 1, ptr);

	float maximumValue;
	if(acCount > 0) {
		float actualMaximumValue = 0;
		for(int i = 0; i < acCount * 3; i++) {
			actualMaximumValue = fmaxf(fabsf(ac[i]), actualMaximumValue);
		}

		int quantisedMaximumValue = fmaxf(0, fminf(82, floorf(actualMaximumValue * 166 - 0.5)));
		maximumValue = ((float)quantisedMaximumValue + 1) / 166;
		ptr = encode_int(quantisedMaximumValue, 1, ptr);
	} else {
		maximumValue = 1;
		ptr = encode_int(0, 1, ptr);
	}

	ptr = encode_int(encodeDC(dc[0], dc[1], dc[2]), 4, ptr);

	for(int i = 0; i < acCount; i++) {
		ptr = encode_int(encodeAC(ac[i * 3 + 0], ac[i * 3 + 1], ac[i * 3 + 2], maximumValue), 2, ptr);
	}

	*ptr = 0;

	return buffer;
}


static int encodeDC(float r, float g, float b) {
	int roundedR = linearTosRGB(r);
	int roundedG = linearTosRGB(g);
	int roundedB = linearTosRGB(b);
	return (roundedR << 16) + (roundedG << 8) + roundedB;
}

static int encodeAC(float r, float g, float b, float maximumValue) {
	int quantR = fmaxf(0, fminf(18, floorf(signPow(r / maximumValue, 0.5) * 9 + 9.5)));
	int quantG = fmaxf(0, fminf(18, floorf(signPow(g / maximumValue, 0.5) * 9 + 9.5)));
	int quantB = fmaxf(0, fminf(18, floorf(signPow(b / maximumValue, 0.5) * 9 + 9.5)));

	return quantR * 19 * 19 + quantG * 19 + quantB;
}

static char characters[83]="0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz#$%*+,-.:;=?@[]^_{|}~";

static char *encode_int(int value, int length, char *destination) {
	int divisor = 1;
	for(int i = 0; i < length - 1; i++) divisor *= 83;

	for(int i = 0; i < length; i++) {
		int digit = (value / divisor) % 83;
		divisor /= 83;
		*destination++ = characters[digit];
	}
	return destination;
}
