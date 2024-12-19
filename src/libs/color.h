#ifndef MCOLOR_H
#define MCOLOR_H

#include "math/vector.h"
#include "number_types.h"

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
	explicit Color(s32 color);
	explicit Color(const Vector3 &rgb, float a = 1.0f);

	operator Vector4();

	u32 get_packed_rgba();
	Vector3 get_rgb();

	void store(float *color);
};

inline Color::operator Vector4()
{
	return value;
}

#endif