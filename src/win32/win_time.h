#ifndef WIN_TIME_H
#define WIN_TIME_H

#include <assert.h>
#include <windows.h>
#include "../libs/number_types.h"

inline s64 cpu_ticks_counter()
{
	LARGE_INTEGER ticks;
	if (!QueryPerformanceCounter(&ticks)) {
		assert(false);
		return 0;
	}
	return ticks.QuadPart;
}

inline s64 cpu_ticks_per_second()
{
	LARGE_INTEGER count_ticks_per_second;
	if (!QueryPerformanceFrequency(&count_ticks_per_second)) {
		assert(false);
		return 0;
	}
	return count_ticks_per_second.QuadPart;

}

inline s64 microseconds_counter()
{
	s64 ticks = cpu_ticks_counter();
	s64 ticks_per_second = cpu_ticks_per_second();
	return (1000 * 1000 * ticks) / ticks_per_second;
}

inline s64 milliseconds_counter()
{
	s64 ticks = cpu_ticks_counter();
	s64 ticks_per_second = cpu_ticks_per_second();
	return (1000 * ticks) / ticks_per_second;
}
#endif