#ifndef WIN_TIME_H
#define WIN_TIME_H

#include <windows.h>
#include "win_types.h"
#include "../sys/sys_local.h"
#include "../libs/os/file.h"


inline s64 cpu_ticks_counter()
{
	LARGE_INTEGER ticks;
	if (!QueryPerformanceCounter(&ticks)) {
		print("Can't get the information about tichs counter");
		return 0;
	}
	return ticks.QuadPart;
}

inline s64 cpu_ticks_per_second()
{
	LARGE_INTEGER count_ticks_per_second;
	if (!QueryPerformanceFrequency(&count_ticks_per_second)) {
		print("Can't get the information about count ticks per second");
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

#define START_PROFILE s64 _start_point = milliseconds_counter()
#define END_PROFILE print("Profiler: File [{}]; Function [{}]; Time elapsed {} ms.", extract_file_name((const char *)__FILE__), (const char *)__FUNCTION__, milliseconds_counter() - _start_point)

#endif