#include <stdio.h>

#include "png.h"
#include "spng.h"
#include "../str.h"
#include "../os/file.h"
#include "../../sys/sys.h"

bool load_png_file(const char *path_to_file, u8 **png_image_buffer, u32 *width, u32 *height)
{
	FILE *png_file = fopen(path_to_file, "rb");
	if (!png_file) {
		String file_name;
		extract_base_file_name(path_to_file, file_name);
		print("Can not open PNG file with name {}", file_name);
		return false;
	}

	spng_ctx *ctx = spng_ctx_new(0);
	spng_set_crc_action(ctx, SPNG_CRC_USE, SPNG_CRC_USE);

	spng_set_png_file(ctx, png_file);

	struct spng_ihdr ihdr;
	int result = spng_get_ihdr(ctx, &ihdr);

	if (result) {
		print("spng_get_ihdr() error: {}", spng_strerror(result));
		spng_ctx_free(ctx);
		fclose(png_file);
		return false;
	}

	*width = ihdr.width;
	*height = ihdr.height;

	if (*png_image_buffer) {
		delete *png_image_buffer;
	}

	int size = ihdr.width * ihdr.width * 4;

	*png_image_buffer = new u8[size];


	result = spng_decode_chunks(ctx);
	if (result) {
		print("spng_decode_chunks() error: {}", spng_strerror(result));
		delete *png_image_buffer;
		spng_ctx_free(ctx);
		fclose(png_file);
		return false;
	}

	result = spng_decode_image(ctx, (void *)*png_image_buffer, size, SPNG_FMT_RGBA8, 0);
	if (result) {
		print("spng_decode_image() error: {}", spng_strerror(result));
		delete *png_image_buffer;
		spng_ctx_free(ctx);
		fclose(png_file);
		return false;
	}

	spng_ctx_free(ctx);
	fclose(png_file);
	return true;
}