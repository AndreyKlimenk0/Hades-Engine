#include "profiling.h"

#include "../win32/win_time.h"

#ifdef VTUNE_PROFILING
static __itt_domain *domain = NULL;

__itt_domain *get_default_domain()
{
	if (!domain) {
		domain = __itt_domain_create("Profiling");
	}
	return domain;
}
#endif

static s64 time_stamp = 0;

void begin_time_stamp()
{
	time_stamp = milliseconds_counter();
}

s64 delta_time_in_milliseconds()
{
	return milliseconds_counter() - time_stamp;
}

s64 delta_time_in_fps()
{
	return s64();
}

