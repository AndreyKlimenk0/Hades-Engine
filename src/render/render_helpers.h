#ifndef RENDER_HELPERS_H
#define RENDER_HELPERS_H

#include "render_api.h"
#include "../libs/str.h"
#include "../libs/number_types.h"

void fill_texture(void *value, Texture2D *texture2d);
bool is_valid_texture(Texture2D *texture2d, String *error_message = NULL);

struct R24U8 {
	R24U8(u32 r24u8_value);
	R24U8(u32 numerator, u8 typeless_bits);

	u32 numerator;
	u8 typeless_bits;

	u32 get_packed_value();
	float get_unorm_value();
};

const u32 NO_TEXTURE_LOADING_OPTIONS = 0x0;
const u32 TEXTURE_LOADING_OPTION_GENERATE_MIPMAPS = 0x1;

struct Texture_Info {
	u32 width = 0;
	u32 height = 0;
};

bool create_texture2d_from_file(const char *full_path_to_texture_file, Texture2D &texture, u32 loading_flags = NO_TEXTURE_LOADING_OPTIONS);
Texture_Info get_last_create_texture_info();

#endif

