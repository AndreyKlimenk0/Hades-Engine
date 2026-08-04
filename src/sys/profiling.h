#ifndef PROFILING_H
#define PROFILING_H

#include "../libs/number_types.h"

#ifdef VTUNE_PROFILING
//#include <ittnotify.h>
//__itt_domain *get_default_domain();

#define USE_PIX
#include <d3d12.h>
#include <Windows.h>
#include <pix3.h>

__forceinline void begin_profile_frame(const char *name)
{
	//__itt_frame_begin_v3(get_default_domain(), NULL);
	static u8 color_index = 0;
	PIXBeginEvent(PIX_COLOR_INDEX(++color_index), name);
}
__forceinline void end_profile_frame()
{
	//__itt_frame_end_v3(get_default_domain(), NULL);
	PIXEndEvent();
}
__forceinline void begin_profile_task(const char *task_name)
{
	static u8 color_index = 0;
	PIXBeginEvent(PIX_COLOR_INDEX(++color_index), task_name);
	//__itt_task_begin(get_default_domain(), __itt_null, __itt_null, __itt_string_handle_create(task_name));
}
__forceinline void end_profile_task()
{
	PIXEndEvent();
	//__itt_task_end(get_default_domain());
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
#endif