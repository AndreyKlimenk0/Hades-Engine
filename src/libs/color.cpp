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