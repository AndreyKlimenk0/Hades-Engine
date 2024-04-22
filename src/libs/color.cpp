#include "color.h"

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

Color::Color(int r, int g, int b, int a)
{
	assert(r <= 255 && r >= 0);
	assert(g <= 255 && g >= 0);
	assert(b <= 255 && b >= 0);
	assert(a <= 255 && a >= 0);

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