#include "fps_counter.h"

#include "math/functions.h"
#include "../win32/win_time.h"

void FPS_Counter::begin_count()
{
	start_time = milliseconds_counter();
	ticks_counter = cpu_ticks_counter();

	if (++frame_counter > average_fps_per_frames) {
		max_fps = max_fps / 2;
		min_fps = min_fps * 2;
	}
}

void FPS_Counter::end_count()
{
	frame_time = milliseconds_counter() - start_time;
	fps = cpu_ticks_per_second() / (cpu_ticks_counter() - ticks_counter);
	fps_accumulator += fps;

	min_fps = math::min(min_fps, fps);
	max_fps = math::max(max_fps, fps);

	if (++frame_counter > average_fps_per_frames) {
		average_fps = fps_accumulator / average_fps_per_frames;
		frame_counter = 0;
		fps_accumulator = 0;
	}
}