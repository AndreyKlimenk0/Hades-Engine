#ifndef UTILS_H
#define UTILS_H

#include "../win32/win_time.h"
#include "../win32/win_types.h"

inline s64 get_unique_number()
{
	return cpu_ticks_counter();
}
#endif