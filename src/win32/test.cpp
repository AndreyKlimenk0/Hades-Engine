#include "test.h"
#include "../libs/str.h"
#include "../sys/sys.h"
#include "../libs/number_types.h"
#include "../libs/structures/tree.h"

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

void test()
{
}

void update_test()
{
}
