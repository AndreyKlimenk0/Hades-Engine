#include <assert.h>

#include "engine.h"
#include "commands.h"
#include "profiling.h"
#include "../sys/level.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/event.h"
#include "../libs/mesh_loader.h"
#include "../win32/win_time.h"

#include "../win32/test.h"

#include "sys.h"
#include <windows.h>
#include "../win32/win_helpers.h"

static Engine *engine = NULL;

static const String DEFAULT_LEVEL_NAME = "unnamed_level";
static const String LEVEL_EXTENSION = ".hl";

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
	//command_args.push("cm_to_m");
	//command_args.push("sphere1.gltf");
	//command_args.push("sphere2.gltf");
	//command_args.push("sphere3.gltf");
	////command_args.push("DamagedHelmet.gltf");
	//command_args.push("Sponza.gltf");
	//command_args.push("testAABB1.gltf");
	command_args.push("occlusion_culling_scene.gltf");
	//command_args.push("test_shadows.gltf");
	//command_args.push("Scene_Demo.gltf");
	run_command("load mesh", command_args);

	//Entity_Id camera_id = game_world->make_perspective_camera(Vector3(0.0f, 3.0f, -14.0f), Vector3(0.0f, 0.0f, 1.0f), engine->global_config.fov, engine->render_sys.window.aspect_ration, engine->global_config.near_plane, engine->global_config.far_plane);
	Entity_Id camera_id = game_world->make_perspective_camera(Vector3(0.0f, 3.0f, -14.0f), Vector3(0.0f, 0.2f, 1.0f), engine->global_config.fov, engine->render_sys.window.aspect_ration, engine->global_config.near_plane, engine->global_config.far_plane);
	render_world->set_rendering_view(camera_id);

	Entity_Id entity_id = game_world->make_direction_light(Vector3(0.2f, -1.0f, 0.2f), Color::White.get_rgb());
	//Entity_Id entity_id = game_world->make_direction_light(Vector3(0.0f, -1.0f, 0.4f), Color::White.get_rgb());
	//Entity_Id entity_id = game_world->make_direction_light(Vector3(0.5f, -1.0f, 0.5f), Color::White.get_rgb());
	render_world->upload_lights();

	//Entity_Id editor_camera_id = game_world->make_camera(Vector3(-5.0f, 10.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
	//Entity_Id editor_camera_id = game_world->make_camera(Vector3(0.0f, 20.0f, -5.0f), Vector3(1.0f, 0.0f, 0.0f));
}

void Engine::init(Win32_Window *window)
{
	engine = this;

	init_os_path();
	init_commands(this);

	var_service.load("all.variables");
	global_config.init(&var_service);

	shader_manager.init();

	render_sys.init(window, &var_service);
	ui_storage.init(render_sys.render_device);

	game_world.init();
	render_world.init(this);

	current_level_name = global_config.load_level;
	if (!load_level(current_level_name, &game_world, &render_world)) {
		current_level_name = build_default_level_name();
		build_default_world(&game_world, &render_world);
	}
	editor.init(this);

	file_tracking_sys.add_directory("hlsl", make_member_callback<Shader_Manager>(&shader_manager, &Shader_Manager::reload));

	engine->is_initialized = true;
}

void Engine::frame()
{
	begin_profile_frame("Frame");
	fps_counter.begin_count();

	pump_events();
	run_event_loop();

	editor.handle_events();
	editor.update();
	editor.render();

	file_tracking_sys.update();
	
	render_world.update();
	render_world.prepare_for_rendering();
	ui_storage.prepare_for_rendering();

	render_sys.render();

	clear_event_queue();

	fps_counter.end_count();
	end_profile_frame();
}

void Engine::shutdown()
{
	render_sys.flush();

	//save_level(current_level_name, &game_world, &render_world);
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

Variable_Service *Engine::get_variable_service()
{
	return &engine->var_service;
}

Global_Config *Engine::get_global_config()
{
	return nullptr;
}
