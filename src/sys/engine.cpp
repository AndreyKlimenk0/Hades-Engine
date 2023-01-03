#include "engine.h"
#include "../gui/gui.h"
#include "../win32/test.h"
#include "../win32/win_time.h"

static Engine *engine = NULL;

void Engine::init(Win32_Info *_win32_info)
{
	engine = this;
	
	win32_info = *_win32_info;
	init_os_path();

	font.init(11);

	render_sys.init(&win32_info, &font);
	gui::init_gui(&render_sys.render_2d, &win32_info, &font, &render_sys.gpu_device);
}

void Engine::frame()
{
	s64 start_time = milliseconds_counter();
	s64 ticks_counter = cpu_ticks_counter();

	pump_events();
	run_event_loop();

	render_sys.new_frame();
	render_sys.render_frame();
	render_sys.end_frame();

	clear_event_queue();

	s64 elapsed_ticks = cpu_ticks_counter() - ticks_counter;
	fps = cpu_ticks_per_second() / elapsed_ticks;
	frame_time = milliseconds_counter() - start_time;
}

void Engine::shutdown()
{
}

Event_Handler *Engine::get_event_handler()
{
	return nullptr;
}

u32 get_window_width()
{
	return engine->win32_info.window_width;
}

u32 get_window_height()
{
	return engine->win32_info.window_height;
}
