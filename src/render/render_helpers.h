#ifndef RENDER_HELPERS_H
#define RENDER_HELPERS_H

#include "render_api.h"
#include "../libs/str.h"
#include "../libs/math/vector.h"
#include "../win32/win_types.h"

enum Texture_Type {
	TEXTURE_TYPE_NORMAL,
	TEXTURE_TYPE_DIFFUSE,
	TEXTURE_TYPE_SPECULAR,
};

struct Texture_File_Info {
	Texture_Type type;
	String name;
};

typedef Vector2 Pad2;
typedef Vector3 Pad3;
typedef Vector4 Pad4;

bool is_valid_texture(Texture2D *texture2d, String *error_message = NULL);
void fill_texture_with_value(void *value, Texture2D *texture2d);

struct R24U8 {
	R24U8(u32 r24u8_value);
	R24U8(u32 numerator, u8 typeless_bits);

	u32 numerator;
	u8 typeless_bits;

	u32 get_packed_value();
	float get_unorm_value();
};

u32 *r8_to_rgba32(u8 *data, u32 width, u32 height);
#endif
