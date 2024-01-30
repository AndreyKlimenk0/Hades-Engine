#include <assert.h>

#include "vector.h"
#include "functions.h"

Vector2 from_raster_to_screen_space(u32 x, u32 y, u32 screen_width, u32 screen_height)
{
	assert(screen_width > 0);
	assert(screen_height > 0);

	float ndc_x = (((float)x / (float)screen_width) * 2.0f) - 1.0f;
	float ndc_y = 1.0f - (((float)y / (float)screen_height) * 2.0f);
	return Vector2(ndc_x, ndc_y);
}
