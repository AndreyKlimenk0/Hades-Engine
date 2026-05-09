#include "test.h"
#include "../libs/str.h"
#include "../sys/sys.h"
#include "../libs/memory/base.h"
#include "../libs/number_types.h"
#include "../libs/structures/tree.h"
#include "../libs/structures/hash_table.h"
#include "../libs/structures/queue.h"

inline u32 pack_RGB(const Vector3 &rgb_value)
{
	u32 r = u32(255.0f * rgb_value.x);
	u32 g = u32(255.0f * rgb_value.y);
	u32 b = u32(255.0f * rgb_value.z);

	u32 result = 0;
	result |= r << 24;
	result |= g << 16;
	result |= b << 8;
	result |= 0xff;
	return result;
}

inline u32 encode_color(const Vector3 &rgb_value)
{
	u32 r = u32(255.0f * rgb_value.x);
	u32 g = u32(255.0f * rgb_value.y);
	u32 b = u32(255.0f * rgb_value.z);

	u32 result = 0;
	result |= r << 16;
	result |= g << 8;
	result |= b;
	return result;
}

struct Shadow_Atlas {
	u32 atlas_size = 8192;
	u32 cascade_size = 1024;
};

Shadow_Atlas shadow_atlas;

Vector2 cascade_ndc_to_atlas_ndc(Vector2 cascade_ndc_coordinates, u32 shadow_cascade_index)
{
	u32 shadow_cascade_rows = shadow_atlas.atlas_size / shadow_atlas.cascade_size;
	u32 shadow_cascade_cols = shadow_atlas.atlas_size / shadow_atlas.cascade_size;
	u32 shadow_cascade_row_index = shadow_cascade_index % shadow_cascade_rows;
	u32 shadow_cascade_col_index = shadow_cascade_index / shadow_cascade_cols;
	Vector2 shadow_atlas_ndc_coordinates;
	shadow_atlas_ndc_coordinates.x = ((cascade_ndc_coordinates.x * (shadow_atlas.cascade_size - 1)) + ((shadow_atlas.cascade_size - 1) * shadow_cascade_row_index)) / shadow_atlas.atlas_size;
	shadow_atlas_ndc_coordinates.y = ((cascade_ndc_coordinates.y * (shadow_atlas.cascade_size - 1)) + ((shadow_atlas.cascade_size - 1) * shadow_cascade_col_index)) / shadow_atlas.atlas_size;
	return shadow_atlas_ndc_coordinates;
}

void update_test()
{
}

void test()
{
}
