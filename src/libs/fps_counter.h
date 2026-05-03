#ifndef FPS_COUNTER
#define FPS_COUNTER

#include <stdint.h>
#include "number_types.h"

struct FPS_Counter {
	s64 fps = 0;
	s64 average_fps = 0;
	s64 min_fps = INT64_MAX;
	s64 max_fps = INT64_MIN;

	s64 ticks_counter = 0;
	s64 average_fps_per_frames = 60;
	s64 frame_counter = 0;
	s64 fps_accumulator = 0;
	
	s64 start_time = 0;
	s64 frame_time = 0;

	void begin_count();
	void end_count();
};

#endif
