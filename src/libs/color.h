#ifndef MCOLOR_H
#define MCOLOR_H

#include <d2d1.h>

#include "math/vector.h"
#include "../win32/win_types.h"

struct Color {
	static Color White;
	static Color Black;
	static Color Red;
	static Color Green;
	static Color Blue;
	static Color Yellow;
	static Color Cyan;
	static Color Magenta;
	static Color Silver;
	static Color LightSteelBlue;

	Vector4 value;

	Color() {};
	Color(int r, int g, int b, int a = 255);
	Color(float r, float g, float b, float a = 1.0f) : value(r, g, b, a) {}

	operator D2D1_COLOR_F();
	operator Vector4();
};

inline Color::operator D2D1_COLOR_F()
{
	return D2D1::ColorF(D2D1::ColorF(value.x, value.y, value.z, value.w));
}
inline Color::operator Vector4()
{
	return value;
}
#endif