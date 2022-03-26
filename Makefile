PROGRAM=blurhash_encoder
DECODER=blurhash_decoder
$(PROGRAM): src/encode_stb.c src/encode.c src/encode.h src/stb_image.h src/common.h
	$(CC) -o $@ src/encode_stb.c src/encode.c -lm -Ofast -march=native -fno-math-errno

$(DECODER): src/decode_stb.c src/decode.c src/decode.h src/stb_writer.h src/common.h
	$(CC) -o $(DECODER) src/decode_stb.c src/decode.c -lm -O3 -ffast-math

.PHONY: clean
clean:
	rm -f $(PROGRAM)
	rm -f $(DECODER)