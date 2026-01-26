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

#include "../win32/test.h"
#include "../gui/test_gui.h"

#include "sys.h"
#include <windows.h>
#include "../win32/win_helpers.h"

#define DRAW_TEST_GUI 0

static Engine *engine = NULL;

static Font *performance_font = NULL;
//static Render_Primitive_List render_list;

static const String DEFAULT_LEVEL_NAME = "unnamed_level";
static const String LEVEL_EXTENSION = ".hl";

static void init_performance_displaying()
{
	performance_font = engine->font_manager.get_font("consola", 14);
	if (!performance_font) {
		assert(false);
	}
	//Render_Font *render_font = engine->render_sys.render_2d.get_render_font(performance_font);
	//render_list = Render_Primitive_List(&engine->render_sys.render_2d, performance_font, render_font);
}

static void display_performance(s64 fps, s64 frame_time)
{
	char *test = format("Fps", fps);
	char *test2 = format("Frame time {} ms", frame_time);
	u32 text_width = performance_font->get_text_width(test2);

	//s32 x = Render_System::screen_width - text_width - 10;
	//render_list.add_text(x, 5, test);
	//render_list.add_text(x, 20, test2);

	free_string(test);
	free_string(test2);

	//engine->render_sys.render_2d.add_render_primitive_list(&render_list);
}

inline String build_default_level_name()
{
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
	return DEFAULT_LEVEL_NAME + index + LEVEL_EXTENSION;
}

inline void build_default_world(Game_World *game_world, Render_World *render_world)
{
	Array<String> command_args;
	//command_args.push("vampire.fbx");
	//command_args.push("sphere1.gltf");
	//command_args.push("sphere2.gltf");
	//command_args.push("sphere3.gltf");
	//command_args.push("DamagedHelmet.gltf");
	command_args.push("Sponza.gltf");
	run_command("load mesh", command_args);

	Entity_Id entity_id = game_world->make_direction_light(Vector3(0.2f, -1.0f, 0.2f), Color::White.get_rgb());
	//Entity_Id entity_id = game_world->make_direction_light(Vector3(0.5f, -1.0f, 0.5f), Color::White.get_rgb());
	render_world->upload_lights();

	Entity_Id editor_camera_id = game_world->make_camera(Vector3(0.0f, 20.0f, -250.0f), Vector3(0.0f, 0.0f, -1.0f));
	render_world->set_rendering_view(editor_camera_id);
}

void Engine::init(Win32_Window *window)
{
	engine = this;

	init_os_path();
	init_commands(this);

	var_service.load("all.variables");
	global_config.init(&var_service);

	font_manager.init();

	shader_manager.init();

	render_sys.init(window, &var_service);

	gui::init_gui(this);

	game_world.init();
	render_world.init(this);

	current_level_name = global_config.load_level;
	if (!load_level(current_level_name, &game_world, &render_world)) {
		current_level_name = build_default_level_name();
		build_default_world(&game_world, &render_world);
	}
	editor.init(this);

	file_tracking_sys.add_directory("hlsl", make_member_callback<Shader_Manager>(&shader_manager, &Shader_Manager::reload));

	init_performance_displaying();

	engine->is_initialized = true;
}

void Engine::frame()
{
	begin_profile_frame("Frame");

	static s64 fps = 60;
	static s64 frame_time = 1000;

	s64 start_time = milliseconds_counter();
	s64 ticks_counter = cpu_ticks_counter();

	pump_events();
	run_event_loop();

	gui::handle_events();

	editor.handle_events();
	editor.update();
	
	file_tracking_sys.update();
	
	render_world.update();

#if DRAW_TEST_GUI
	draw_test_gui();
#else
	editor.render();
#endif
	render_world.prepare_for_rendering();

	render_sys.render();

	clear_event_queue();

	fps = cpu_ticks_per_second() / (cpu_ticks_counter() - ticks_counter);
	frame_time = milliseconds_counter() - start_time;
	
	end_profile_frame();
}

void Engine::shutdown()
{
	render_sys.flush();

	save_level(current_level_name, &game_world, &render_world);
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
