#ifndef PROFILING_H
#define PROFILING_H

#include "../libs/number_types.h"

#ifdef VTUNE_PROFILING
#include <ittnotify.h>
__itt_domain *get_default_domain();
__forceinline void begin_profile_frame(const char *name)
{
	__itt_frame_begin_v3(get_default_domain(), NULL);
}
__forceinline void end_profile_frame()
{
	__itt_frame_end_v3(get_default_domain(), NULL);
}
__forceinline void begin_profile_task(const char *task_name)
{
	__itt_task_begin(get_default_domain(), __itt_null, __itt_null, __itt_string_handle_create(task_name));
}
__forceinline void end_profile_task()
{
	__itt_task_end(get_default_domain());
}
#else
__forceinline void begin_profile_frame(const char *name) 
{
}
__forceinline void end_profile_frame() 
{
}
__forceinline void begin_profile_task(const char *name)
{
}
__forceinline void end_profile_task()
{
}
#endif
void begin_time_stamp();
s64 delta_time_in_milliseconds();
s64 delta_time_in_fps();

#endif