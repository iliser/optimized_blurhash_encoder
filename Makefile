PROGRAM=blurhash_encoder
DECODER=blurhash_decoder

$(PROGRAM): src/encode_stb.cpp src/encode.cpp src/encode.h src/stb_image.h src/color_models.hpp src/blurhash_string.hpp
	$(CXX) -o $@ src/encode_stb.cpp src/encode.cpp -lm -Ofast -march=native -std=c++20

$(DECODER): src/decode_stb.cpp src/decode.cpp src/decode.h src/stb_writer.h src/color_models.hpp
	$(CXX) -o $(DECODER) src/decode_stb.cpp src/decode.cpp -lm -Ofast -march=native -std=c++20

all: clean $(PROGRAM) $(DECODER)

.PHONY: clean
clean:
	rm -f $(PROGRAM)
	rm -f $(DECODER)