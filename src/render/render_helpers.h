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

#endif

