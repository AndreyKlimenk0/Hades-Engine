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


#define DRAW_TEST_GUI 1

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

void Engine::init_base()
{
	engine = this;
	init_os_path();
	init_commands();
	var_service.load("all.variables");
}

#include <windows.h>
#include "../win32/win_helpers.h"


void Engine::init(Win32_Window *window)
{
	bool windowed = true;
	bool vsync = false;
	s32 back_buffer_count = 3;

	test();

	font_manager.init();

	Variable_Service *rendering_settings = var_service.find_namespace("rendering");
	ATTACH(rendering_settings, vsync);
	ATTACH(rendering_settings, windowed);
	ATTACH(rendering_settings, back_buffer_count);

	shader_manager.init();

	render_sys.init(window, &var_service);

	gui::init_gui(this);
	
	// The editor dependence on render system because it uses the window size for initializing gui.
	editor.init(this);
	
	game_world.init();
	render_world.init(this);

	init_commands();
	Array<String> temp;
	//temp.push("vampire.fbx");
	temp.push("Sponza.gltf");
	//temp.push("Mutant.fbx");
	run_command("load mesh", temp);

	Entity_Id entity_id = game_world.make_direction_light(Vector3(0.2f, -1.0f, 0.2f), Color::White.get_rgb());
	render_world.upload_lights();

	file_tracking_sys.add_directory("hlsl", make_member_callback<Shader_Manager>(&shader_manager, &Shader_Manager::reload));
}

#include "sys.h"

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

	editor.handle_events();
	file_tracking_sys.update();

#if DRAW_TEST_GUI
	draw_test_gui();
#else
	editor.render();
#endif
	render_world.update();

	render_sys.render();

	clear_event_queue();

	fps = cpu_ticks_per_second() / (cpu_ticks_counter() - ticks_counter);
	frame_time = milliseconds_counter() - start_time;

	END_FRAME();
}

void Engine::shutdown()
{
	render_sys.flush();

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
	//save_game_and_render_world_in_level(current_level_name, &game_world, &render_world);
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

//Descriptor_Table_Range::Descriptor_Table_Range()
//{
//	ZeroMemory(&d3d12_descriptr_range, sizeof(D3D12_DESCRIPTOR_RANGE));
//}
//
//Descriptor_Table_Range::Descriptor_Table_Range(D3D12_DESCRIPTOR_RANGE_TYPE range_type, u32 descriptor_count, u32, u32 space, u32 descriptrs_table_offset) : Descriptor_Table_Range()
//{
//	d3d12_descriptr_range.RangeType;
//	d3d12_descriptr_range.NumDescriptors;
//	d3d12_descriptr_range.BaseShaderRegister;
//	d3d12_descriptr_range.RegisterSpace;
//	d3d12_descriptr_range.OffsetInDescriptorsFromTableStart;
//}
//
//Descriptor_Table_Range::~Descriptor_Table_Range()
//{
//}


Variable_Service *Engine::get_variable_service()
{
	return &engine->var_service;
}
