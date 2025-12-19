#include <assert.h>
#include "render_base.h"

u64 Buffer_Desc::size()
{
	return count * stride;
}

u64 Texture_Desc::size()
{
	assert(format != DXGI_FORMAT_UNKNOWN);
	assert(dimension != TEXTURE_DIMENSION_UNKNOWN);

	u64 result = 0;
	if (dimension == TEXTURE_DIMENSION_1D) {
		assert(width > 0);
		result = width * dxgi_format_size(format);
	} else if (dimension == TEXTURE_DIMENSION_2D) {
		assert(width > 0);
		assert(height > 0);
		result = width * height * dxgi_format_size(format);
	} else if (dimension == TEXTURE_DIMENSION_3D) {
		assert(width > 0);
		assert(height > 0);
		assert(depth > 0);
		result = width * height * depth * dxgi_format_size(format);
	}
	return result;
}