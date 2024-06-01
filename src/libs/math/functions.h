#ifndef MATH_FUNCTIONS_H
#define MATH_FUNCTIONS_H

#include <assert.h>
#include <math.h>

#include "constants.h"
#include "../../win32/win_types.h"

struct Vector2;
struct Vector3;
struct Matrix4;

#ifdef max
	#undef max
#endif

#ifdef min
	#undef min
#endif

namespace math {
	template <typename T>
	inline T clamp(T value, T min, T max)
	{
		return value < min ? min : (value > max ? max : value);
	}
	template <typename T>
	inline T max(T x, T y)
	{
		return (x > y) ? x : y;
	}

	template <typename T>
	inline T min(T x, T y)
	{
		return (x > y) ? y : x;
	}

	template <typename T>
	inline T abs(T value)
	{
		return (value > 0) ? value : -value;
	}

	inline float arccos(float value)
	{
		return (float)::acos((double)value);
	}

	inline float cos(float value)
	{
		return (float)::cos((double)value);
	}

	inline float sin(float value)
	{
		return (float)::sin((double)value);
	}

	inline float tan(float value)
	{
		return (float)::tan((double)value);
	}

	template< typename T>
	inline T pow2(T value)
	{
		return (T)::pow((double)value, 2);
	}

	template< typename T>
	inline T sqrt(T value)
	{
		return (T)::sqrt((double)value);
	}

	template< typename T>
	inline T ceil(T value)
	{
		return (T)::ceil((double)value);
	}
};

inline float radians_to_degrees(float radians)
{
	return radians * (180.0f / PI);
}

inline float degrees_to_radians(float degrees)
{
	return (degrees * PI) / 180.0f;
}

inline void invert(bool *value)
{
	assert(value);
	*value = *value ? false : true;
}

Vector2 from_raster_to_screen_space(u32 x, u32 y, u32 screen_width, u32 screen_height);
Matrix4 make_rotation_matrix(Vector3 *direction, Vector3 *up_direction = NULL);

#endif