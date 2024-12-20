#include <assert.h>

#include "engine.h"
#include "commands.h"
#include "profiling.h"
#include "../gui/gui.h"
#include "../sys/level.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/event.h"
#include "../libs/mesh_loader.h"
#include "../win32/win_time.h"

#include "../gui/test_gui.h"

#define DRAW_TEST_GUI 0

static Engine *engine = NULL;

static Font *performance_font = NULL;
static Render_Primitive_List render_list;

static const String DEFAULT_LEVEL_NAME = "unnamed_level";
static const String LEVEL_EXTENSION = ".hl";

static void init_performance_displaying()
{
	performance_font = engine->font_manager.get_font("consola", 14);
	if (!performance_font) {
		assert(false);
	}
	Render_Font *render_font = engine->render_sys.render_2d.get_render_font(performance_font);
	render_list = Render_Primitive_List(&engine->render_sys.render_2d, performance_font, render_font);
}

static void display_performance(s64 fps, s64 frame_time)
{
	char *test = format("Fps", fps);
	char *test2 = format("Frame time {} ms", frame_time);
	u32 text_width = performance_font->get_text_width(test2);

	s32 x = Render_System::screen_width - text_width - 10;
	render_list.add_text(100, 5, test);
	render_list.add_text(180, 5, test2);

	free_string(test);
	free_string(test2);

	engine->render_sys.render_2d.add_render_primitive_list(&render_list);
}

void Engine::init_base()
{
	engine = this;
	init_os_path();
	init_commands();
	var_service.load("all.variables");
}

void Engine::init(Win32_Window *window)
{
	BEGIN_TASK("Initialize engine");

	font_manager.init();
	
	BEGIN_TASK("Initialize render_system");
	render_sys.init(window);
	END_TASK();
	
	shader_manager.init(&render_sys.gpu_device);
	
	render_sys.init_input_layouts(&shader_manager);
	render_sys.render_2d.init(&render_sys, &shader_manager);
	render_sys.render_3d.init(&render_sys, &shader_manager);

	gui::init_gui(this);
	
	editor.init(this);

	game_world.init();
	render_world.init(this);

	current_level_name = DEFAULT_LEVEL_NAME + LEVEL_EXTENSION;
	Variable_Service *system = var_service.find_namespace("system");
	system->attach("load_level", &current_level_name);

	BEGIN_TASK("Load level");
	init_game_and_render_world_from_level(current_level_name, &game_world, &render_world);
	END_TASK();

	file_tracking_sys.add_directory("hlsl", make_member_callback<Shader_Manager>(&shader_manager, &Shader_Manager::reload));

	init_performance_displaying();
	
	engine->is_initialized = true;

	END_TASK()
}

void Engine::frame()
{
	BEGIN_FRAME();

	static s64 fps = 60;
	static s64 frame_time = 1000;

	s64 start_time = milliseconds_counter();
	s64 ticks_counter = cpu_ticks_counter();

	BEGIN_TASK("Update");
	pump_events();
	run_event_loop();

	gui::handle_events();

	editor.handle_events();

	file_tracking_sys.update();

	editor.update();

	render_world.update();

	render_sys.new_frame();

	END_TASK();

	BEGIN_TASK("Render world");
	render_world.render();
	END_TASK();

#if DRAW_TEST_GUI
	draw_test_gui();
#else
	editor.render();
#endif
	display_performance(fps, frame_time);

	BEGIN_TASK("Render system end frame");

	render_sys.end_frame();

	END_TASK();

	clear_event_queue();

	fps = cpu_ticks_per_second() / (cpu_ticks_counter() - ticks_counter);
	frame_time = milliseconds_counter() - start_time;
	
	END_FRAME();
}

void Engine::shutdown()
{
	if (current_level_name.is_empty()) {
		int counter = 0;
		String index = "";
		while (true) {
			String full_path_to_map_file;
			build_full_path_to_level_file(DEFAULT_LEVEL_NAME + index + LEVEL_EXTENSION, full_path_to_map_file);
			if (file_exists(full_path_to_map_file.c_str())) {
				char *str_counter = ::to_string(counter++);
				index = str_counter;
				free_string(str_counter);
				continue;
			}
			break;
		}
		current_level_name = DEFAULT_LEVEL_NAME + index + LEVEL_EXTENSION;
	}
	save_game_and_render_world_in_level(current_level_name, &game_world, &render_world);
	gui::shutdown();
	var_service.shutdown();
}

void Engine::set_current_level_name(const String &level_name)
{
	assert(level_name.len > 0);

	current_level_name = level_name + LEVEL_EXTENSION;
}

bool Engine::initialized()
{
	return engine ? engine->is_initialized : false;
}

void Engine::resize_window(u32 window_width, u32 window_height)
{
	engine->render_sys.resize(window_width, window_height);
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

Variable_Service *Engine::get_variable_service()
{
	return &engine->var_service;
}
