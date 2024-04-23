#include "engine.h"
#include "sys_local.h"
#include "../gui/gui.h"
#include "../win32/test.h"
#include "../win32/win_time.h"
#include "../libs/os/file.h"
#include "../libs/os/event.h"
#include "../libs/mesh_loader.h"

//@Note: Probably it is temporary decision
#include "../libs/png_image.h"
#include "../gui/test_gui.h"

#define DRAW_TEST_GUI 0

static const u32 FONT_SIZE = 11;
static Engine *engine = NULL;

void Engine::init(Win32_Info *_win32_info)
{
	engine = this;
	
	win32_info = *_win32_info;
	init_os_path();

	font_manager.init();

	render_sys.init(this);
	shader_manager.init(&render_sys.gpu_device);
	render_sys.render_2d.init(this);
	//@Note: It will be nice to get rid of input layouts in the future.
	render_sys.init_shader_input_layout(&shader_manager);
	
	
	//gui::init_gui(this, "FiraCode-Regular", 13);
	gui::init_gui(this, "consola", FONT_SIZE);
	
	editor.init(this);

	performance_displayer.init(this);

	game_world.init();
	render_world.init(this);

	String path;
	build_full_path_to_map_file("temp_map.bmap", path);
	if (file_exists(path)) {
		init_from_map();
	}
	render_world.init_meshes();

	file_tracking_sys.add_directory("hlsl", make_member_callback<Shader_Manager>(&shader_manager, &Shader_Manager::reload));

	engine->is_initialized = true;
}

static inline Texture_Idx add_texture(const char *name)
{
	u32 width;
	u32 height;
	u8 *data = NULL;
	String path;
	build_full_path_to_texture_file(name, path);
	load_png_file(path, &data, &width, &height);

	Texture_Idx index = Engine::get_render_world()->render_entity_texture_storage.add_texture(name, width, height, (void *)data);
	DELETE_PTR(data);
	return index;
}

void Engine::init_from_map()
{
	game_world.init_from_file();

	Geometry_Entity *geometry_entity = NULL;
	For(game_world.geometry_entities, geometry_entity) {
		
		char *mesh_name = NULL;
		Mesh_Idx mesh_idx;
		Triangle_Mesh triangle_mesh;
		if (geometry_entity->geometry_type == GEOMETRY_TYPE_BOX) {
			make_box_mesh(&geometry_entity->box, &triangle_mesh);
			mesh_name = format("Box", geometry_entity->box.width, geometry_entity->box.height, geometry_entity->box.depth);
		
		} else if (geometry_entity->geometry_type == GEOMETRY_TYPE_SPHERE) {
			make_sphere_mesh(&geometry_entity->sphere, &triangle_mesh);
			mesh_name = format("Sphere", geometry_entity->sphere.radius, geometry_entity->sphere.slice_count, geometry_entity->sphere.stack_count);
		
		} else if (geometry_entity->geometry_type == GEOMETRY_TYPE_GRID) {
			make_grid_mesh(&geometry_entity->grid, &triangle_mesh);
			mesh_name = format("Grid", geometry_entity->grid.width, geometry_entity->grid.depth, geometry_entity->grid.rows_count, geometry_entity->grid.columns_count);
		
		} else {
			print("Engine::init_from_file: Unknown geometry type.");
		}

		//Render_Entity_Textures render_entity_textures;
		//render_entity_textures.ambient_texture_idx = add_texture("Rock_Mosaic_AO.png");
		//render_entity_textures.normal_texture_idx = add_texture("Rock_Mosaic_NORM.png");
		//render_entity_textures.diffuse_texture_idx = add_texture("Rock_Mosaic_DIFF.png");
		//render_entity_textures.specular_texture_idx = add_texture("Rock_Mosaic_SPEC.png");
		//render_entity_textures.displacement_texture_idx = add_texture("Rock_Mosaic_DISP_alternative.png");

		Render_Entity_Textures render_entity_textures;
		render_entity_textures.ambient_texture_idx = render_world.render_entity_texture_storage.white_texture_idx;
		render_entity_textures.normal_texture_idx = add_texture("toy_box2.png");
		render_entity_textures.diffuse_texture_idx = add_texture("toy_box1.png");
		//render_entity_textures.diffuse_texture_idx = render_world.render_entity_texture_storage.white_texture_idx;;
		render_entity_textures.displacement_texture_idx = add_texture("toy_box3.png");
		render_entity_textures.specular_texture_idx = render_world.render_entity_texture_storage.default_specular_texture_idx;

		render_world.add_mesh(mesh_name, &triangle_mesh, &mesh_idx);
		render_world.add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, get_entity_id(geometry_entity), mesh_idx, &render_entity_textures);
		
		free_string(mesh_name);
	}
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

void Engine::save_to_file()
{
	game_world.save_to_file();
}

void Engine::shutdown()
{
	gui::shutdown();
	save_to_file();
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

