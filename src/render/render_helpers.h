#ifndef RENDER_HELPERS_H
#define RENDER_HELPERS_H

#include <dxgi1_4.h>
#include "../libs/str.h"
#include "../libs/number_types.h"

struct R24U8 {
	R24U8(u32 r24u8_value);
	R24U8(u32 numerator, u8 typeless_bits);

	u32 numerator;
	u8 typeless_bits;

	u32 get_packed_value();
	float get_unorm_value();
};

u32 dxgi_format_size(DXGI_FORMAT format);

#endif

