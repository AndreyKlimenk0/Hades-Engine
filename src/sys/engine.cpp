#include "../libs/str.h"
#include "map.h"
#include "engine.h"
#include "sys_local.h"
#include "commands.h"
#include "../gui/gui.h"
#include "../win32/test.h"
#include "../win32/win_time.h"
#include "../libs/os/file.h"
#include "../libs/os/event.h"
#include "../libs/mesh_loader.h"
#include "../libs/os/path.h"

//@Note: Probably it is temporary decision
#include "../libs/png_image.h"
#include "../gui/test_gui.h"

#define DRAW_TEST_GUI 1

static const u32 FONT_SIZE = 11;
static Engine *engine = NULL;

void Engine::init(Win32_Info *_win32_info)
{
	engine = this;
	
	win32_info = *_win32_info;
	init_os_path();

	init_commands();

	font_manager.init();

	render_sys.init(this);
	shader_manager.init(&render_sys.gpu_device);
	render_sys.render_2d.init(this);
	//@Note: It will be nice to get rid of input layouts in the future.
	render_sys.init_shader_input_layout(&shader_manager);
	
	
	gui::init_gui(this, "FiraCode-Regular", 12);
	//gui::init_gui(this, "consola", FONT_SIZE);
	
	editor.init(this);

	performance_displayer.init(this);

	game_world.init();
	render_world.init(this);

	current_map = "unnamed_map.hmap";
	init_game_and_render_world_from_map("unnamed_map.hmap", &game_world, &render_world);
	
	file_tracking_sys.add_directory("hlsl", make_member_callback<Shader_Manager>(&shader_manager, &Shader_Manager::reload));

	engine->is_initialized = true;
}

void Engine::init_from_map()
{
}

void Engine::frame()
{
	s64 start_time = milliseconds_counter();
	s64 ticks_counter = cpu_ticks_counter();

	pump_events();
	run_event_loop();

	gui::handle_events();

	editor.handle_events();

	file_tracking_sys.update();
	
	editor.update();

	render_world.update();

	render_sys.new_frame();

	render_world.render();

#if DRAW_TEST_GUI
	draw_test_gui();
#else
	editor.render();
#endif
	performance_displayer.display();

	render_sys.end_frame();

	clear_event_queue();

	fps = cpu_ticks_per_second() / (cpu_ticks_counter() - ticks_counter);
	frame_time = milliseconds_counter() - start_time;
}

void Engine::shutdown()
{
	if (current_map.is_empty()) {
		int counter = 0;
		String map_name = "unnamed_map";
		String map_extension = ".hmap";
		String index = "";
		while (true) {
			String full_path_to_map_file;
			build_full_path_to_map_file(map_name + index + map_extension, full_path_to_map_file);
			if (file_exists(full_path_to_map_file.c_str())) {
				char *str_counter = ::to_string(counter++);
				index = str_counter;
				free_string(str_counter);
				continue;
			}
			break;
		}
		current_map = map_name + index + map_extension;
	}
	save_game_and_render_world_in_map(current_map, &game_world, &render_world);
	gui::shutdown();
}

bool Engine::initialized()
{
	if (engine) {
		return engine->is_initialized;
	}
	return false;
}

void Engine::resize_window(u32 window_width, u32 window_height)
{
	engine->render_sys.resize(window_width, window_height);
}

Win32_Info *Engine::get_win32_info()
{
	return &engine->win32_info;
}

Engine *Engine::get_instance()
{
	return engine;
}

Game_World *Engine::get_game_world()
{
	return &engine->game_world;
}

Render_World *Engine::get_render_world()
{
	return &engine->render_world;
}

Render_System *Engine::get_render_system()
{
	return &engine->render_sys;
}

Font_Manager *Engine::get_font_manager()
{
	return &engine->font_manager;
}

void Engine::Performance_Displayer::init(Engine *_engine)
{
	engine = _engine;
	font = engine->font_manager.get_font("consola", 14);
	if (!font) {
		print("Engine::Performance_Displayer::init: Failed to get font.");
		return;
	}
	Render_Font *render_font = engine->render_sys.render_2d.get_render_font(font);
	render_list = Render_Primitive_List(&engine->render_sys.render_2d, font, render_font);
}

void Engine::Performance_Displayer::display()
{
	char *test = format("Fps", engine->fps);
	char *test2 = format("Frame time {} ms", engine->frame_time);
	u32 text_width = font->get_text_width(test2);

	s32 x = engine->win32_info.window_width - text_width - 10;
	render_list.add_text(x, 5, test);
	render_list.add_text(x, 20, test2);
	
	free_string(test);
	free_string(test2);

	engine->render_sys.render_2d.add_render_primitive_list(&render_list);
}

