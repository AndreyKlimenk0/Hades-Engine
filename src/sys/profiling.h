#ifndef PROFILING_H
#define PROFILING_H

#include "../libs/number_types.h"

#ifdef VTUNE_PROFILING
#include <ittnotify.h>
__itt_domain *get_default_domain();
#define BEGIN_TASK(task_name) __itt_task_begin(get_default_domain(), __itt_null, __itt_null, __itt_string_handle_create(task_name))
#define END_TASK() __itt_task_end(get_default_domain());
#define BEGIN_FRAME() __itt_frame_begin_v3(get_default_domain(), NULL)
#define END_FRAME() 	__itt_frame_end_v3(get_default_domain(), NULL);
#else
#define BEGIN_TASK(task_name)
#define END_TASK()
#define BEGIN_FRAME()
#define END_FRAME()
#endif

void begin_time_stamp();
s64 delta_time_in_milliseconds();
s64 delta_time_in_fps();

#endif