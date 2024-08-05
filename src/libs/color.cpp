#include <assert.h>

#include "color.h"
#include "utils.h"

Color Color::White = { 1.0f, 1.0f, 1.0f, 1.0f };
Color Color::Black = { 0.0f, 0.0f, 0.0f, 1.0f };
Color Color::Red = { 1.0f, 0.0f, 0.0f, 1.0f };
Color Color::Green = { 0.0f, 1.0f, 0.0f, 1.0f };
Color Color::Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
Color Color::Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
Color Color::Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
Color Color::Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
Color Color::Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
Color Color::LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };

Color::Color(s32 color) : Color(color, color, color)
{
}

Color::Color(const Vector3 &rgb, float a)
{
	assert(in_range(0.0f, 1.0f, rgb.x));
	assert(in_range(0.0f, 1.0f, rgb.y));
	assert(in_range(0.0f, 1.0f, rgb.z));
	assert(in_range(0.0f, 1.0f, a));

	value.x = rgb.x;
	value.y = rgb.y;
	value.z = rgb.z;
	value.w = a;
}

Color::Color(int r, int g, int b, int a)
{
	assert(in_range(0, 255, r));
	assert(in_range(0, 255, g));
	assert(in_range(0, 255, b));
	assert(in_range(0, 255, a));

	value.x = r / 255.0f;
	value.y = g / 255.0f;
	value.z = b / 255.0f;
	value.w = a / 255.0f;
}

u32 Color::get_packed_rgba()
{
	//Change bits order because nvidia gpu uses little endian
	u32 rgba = 0;
	rgba |= u32(value.x * 255.0f);
	rgba |= u32(value.y * 255.0f) << 8;
	rgba |= u32(value.z * 255.0f) << 16;
	rgba |= u32(value.w * 255.0f) << 24;
	return rgba;
}

Vector3 Color::get_rgb()
{
	return to_vector3(value);
}
