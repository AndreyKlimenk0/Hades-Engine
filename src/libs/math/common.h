#ifndef COMMON_MATH_H
#define COMMON_MATH_H

#include "../../win32/win_types.h"

//float Pi = 3.1415926535f;
//
//float clamp(float value, float min, float max)
//{
//	return value < min ? min : (value > max ? max : value);
//}

struct Size_u32 {
	Size_u32() {}
	Size_u32(u32 width, u32 height) : width(width), height(height) {}
	
	u32 width = 0;
	u32 height = 0;
};

struct Rect_u32 {
	Rect_u32() {}
	Rect_u32(u32 width, u32 height) : width(width), height(height) {}
	Rect_u32(u32 x, u32 y, u32 width, u32 height) : x(x), y(y), width(width), height(height) {}
	Rect_u32(Size_u32 &size) : width(size.width), height(size.height) {}
	
	u32 x = 0;
	u32 y = 0;
	u32 width = 0;
	u32 height = 0;

	void set(Size_u32 &size);
	void set(u32 _x, u32 _y);
	void set_wh(u32 _width, u32 _height);

	u32 right();
	u32 bottom();
};

inline void Rect_u32::set(Size_u32 &size)
{
	width = size.width;
	height = size.height;
}

inline void Rect_u32::set(u32 _x, u32 _y)
{
	x = _x;
	y = _y;
}

inline u32 Rect_u32::right()
{
	return x + width;
}

inline u32 Rect_u32::bottom()
{
	return y + height;
}

inline void Rect_u32::set_wh(u32 _width, u32 _height)
{
	width = _width;
	height = _height;
}

struct Rect_f32 {
	Rect_f32() {}
	Rect_f32(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
	
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
};
#endif