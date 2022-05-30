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
};

struct Rect_f32 {
	Rect_f32() {}
	Rect_f32(float x, float y, float width, float height) : x(x), y(y), width(width), height(height) {}
	
	float x = 0.0f;
	float y = 0.0f;
	float width = 0.0f;
	float height = 0.0f;
};
#endif