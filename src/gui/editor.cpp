#include "editor.h"
#include "../sys/engine.h"
#include "../sys/sys_local.h"
#include "../libs/geometry_helper.h"
#include "../render/render_api.h"
#include "../render/render_system.h"
#include "../render/render_helpers.h"


void Editor_Window::init(Engine *engine)
{
	game_world = &engine->game_world;
	render_world = &engine->render_world;
}

Make_Entity_Window::Make_Entity_Window()
{
	reset_state();
}

Make_Entity_Window::~Make_Entity_Window()
{
	DELETE_PTR(entity_type_helper);
	DELETE_PTR(light_type_helper);
	DELETE_PTR(geometry_type_helper);
}

void Make_Entity_Window::init(Engine *engine)
{
	Editor_Window::init(engine);
	
	set_normal_enum_formatting();
	entity_type_helper = MAKE_ENUM_HELPER(Entity_Type, ENTITY_TYPE_UNKNOWN, ENTITY_TYPE_COMMON, ENTITY_TYPE_LIGHT, ENTITY_TYPE_GEOMETRY);
	entity_type_helper->get_string_enums(&entity_types);

	light_type_helper = MAKE_ENUM_HELPER(Light_Type, SPOT_LIGHT_TYPE, POINT_LIGHT_TYPE, DIRECTIONAL_LIGHT_TYPE);
	light_type_helper->get_string_enums(&light_types);

	geometry_type_helper = MAKE_ENUM_HELPER(Geometry_Type, GEOMETRY_TYPE_BOX, GEOMETRY_TYPE_GRID, GEOMETRY_TYPE_SPHERE);
	geometry_type_helper->get_string_enums(&geometry_types);
}

void Make_Entity_Window::reset_state()
{
	light_index = 0;
	entity_index = 0;
	geometry_index = 0;
	box;
	position = { 0.0f, 0.0f, 0.0f };
	direction = { 0.2f, -1.0f, 0.2f };
	color = { 255.0, 255.0, 255.0 };
}

void Make_Entity_Window::draw()
{
	gui::edit_field("Position", &position);
	gui::list_box(&entity_types, &entity_index);

	Entity_Type type = entity_type_helper->from_string(entity_types[entity_index]);

	if (type == ENTITY_TYPE_LIGHT) {
		gui::list_box(&light_types, &light_index);
		Light_Type light_type = light_type_helper->from_string(light_types[light_index]);

		if (light_type == DIRECTIONAL_LIGHT_TYPE) {
			gui::edit_field("Direction", &direction);
			gui::edit_field("Color", &color, "R", "G", "B");
			if (gui::button("Make")) {
				game_world->make_direction_light(direction, color);
			}
		}

	} else if (type == ENTITY_TYPE_GEOMETRY) {
		gui::list_box(&geometry_types, &geometry_index);
		Geometry_Type geometry_type = geometry_type_helper->from_string(geometry_types[geometry_index]);

		if (geometry_type == GEOMETRY_TYPE_BOX) {
			gui::edit_field("Width", &box.width);
			gui::edit_field("Height", &box.height);
			gui::edit_field("Depth", &box.depth);

			if (gui::button("Make")) {
				Entity_Id entity_id = game_world->make_geometry_entity(position, geometry_type, (void *)&box);

				Triangle_Mesh mesh;
				make_box_mesh(&box, &mesh);

				char *mesh_name = format("Box", box.width, box.height, box.depth);
				Mesh_Idx mesh_idx;
				render_world->add_mesh(mesh_name, &mesh, &mesh_idx);

				render_world->make_render_entity(entity_id, mesh_idx);

				free_string(mesh_name);
			}
		} else if (geometry_type == GEOMETRY_TYPE_SPHERE) {
			gui::edit_field("Radious", &sphere.radius);
			gui::edit_field("Slice count", (s32 *)&sphere.slice_count);
			gui::edit_field("Stack count", (s32 *)&sphere.stack_count);

			if (gui::button("Make")) {
				Entity_Id entity_id = game_world->make_geometry_entity(position, geometry_type, (void *)&sphere);

				Triangle_Mesh mesh;
				make_sphere_mesh(&sphere, &mesh);

				char *mesh_name = format("Sphere", sphere.radius, sphere.slice_count, sphere.stack_count);
				Mesh_Idx mesh_idx;
				render_world->add_mesh(mesh_name, &mesh, &mesh_idx);

				render_world->make_render_entity(entity_id, mesh_idx);

				free_string(mesh_name);
			}
		}
	} else {
		print("Make_Entity_Window::draw: Unknown Entity Type.");
	}
}

Editor::Editor()
{
}

Editor::~Editor()
{
}

void Editor::init()
{
	Engine *engine = Engine::get_instance();
	make_entity_window.init(engine);
	game_world_window.init(engine);
	render_world_window.init(engine);
}

void Editor::render()
{
	gui::begin_frame();
	if (gui::begin_window("Editor")) {

		if (gui::add_tab("Make Entity")) {
			make_entity_window.draw();
		}

		if (gui::add_tab("Game World")) {
			game_world_window.draw();
		}

		if (gui::add_tab("Camera")) {
			Camera *camera = &Engine::get_render_world()->camera;
			gui::text("Camera Type: Free");
			gui::edit_field("Position", &camera->position);
			gui::edit_field("Direction", &camera->target);
		}

		if (gui::add_tab("Render World")) {
			render_world_window.draw();
		}
		gui::end_window();
	}
	gui::end_frame();
}

void Game_World_Window::init(Engine *engine)
{
	Editor_Window::init(engine);
	entity_index = 0;
	window_width_delta = 20;
	world_entities_height = 200;
	entity_info_height = 400;
	window_style = WINDOW_STYLE_DEFAULT & ~WINDOW_WITH_OUTLINES;
	entity_type = ENTITY_TYPE_UNKNOWN;

	world_entities_window_theme.background_color = Color(42, 42, 42);
	world_entities_window_theme.header_color = Color(36, 36, 36);
	world_entities_window_theme.place_between_elements = 0;

	entity_info_window_theme.background_color = Color(42, 42, 42);
	entity_info_window_theme.header_color = Color(36, 36, 36);
	entity_info_window_theme.place_between_elements = 8;

	buttons_theme.color = world_entities_window_theme.background_color;
	buttons_theme.aligment = 0x08;
}

void Game_World_Window::draw()
{
	Size_s32 window_size = gui::get_window_size();
	gui::set_next_window_size(window_size.width - window_width_delta, world_entities_height);
	gui::set_next_theme(&world_entities_window_theme);
	if (gui::begin_child("World Entities", (WINDOW_STYLE_DEFAULT & ~WINDOW_WITH_OUTLINES))) {
		buttons_theme.rect.width = window_size.width - window_width_delta;
		gui::set_theme(&buttons_theme);

		Game_World *game_world = Engine::get_game_world();
		draw_entity_list("Light", game_world->lights.count, ENTITY_TYPE_LIGHT);
		draw_entity_list("Geometry", game_world->geometry_entities.count, ENTITY_TYPE_GEOMETRY);

		gui::reset_button_theme();
		gui::end_child();
	}

	gui::set_next_theme(&entity_info_window_theme);
	gui::set_next_window_size(window_size.width - window_width_delta, entity_info_height);
	if (gui::begin_child("Entity Info", (WINDOW_STYLE_DEFAULT & ~WINDOW_WITH_OUTLINES))) {

		Entity *entity = NULL;
		if (entity_type == ENTITY_TYPE_GEOMETRY) {
			entity = &game_world->geometry_entities[entity_index];
			Geometry_Entity *geometry_entity = &game_world->geometry_entities[entity_index];
			gui::edit_field("Position", &geometry_entity->position);
			if (geometry_entity->geometry_type == GEOMETRY_TYPE_BOX) {
				gui::text("Geometry type: Box");
				gui::edit_field("Width", &geometry_entity->box.width);
				gui::edit_field("Height", &geometry_entity->box.height);
				gui::edit_field("Depth", &geometry_entity->box.depth);
			} else if (geometry_entity->geometry_type == GEOMETRY_TYPE_SPHERE) {
				gui::text("Geometry type: Sphere");
				gui::edit_field("Radius", &geometry_entity->sphere.radius);
				gui::edit_field("Slice Count", (s32 *)&geometry_entity->sphere.slice_count);
				gui::edit_field("Stack Count", (s32 *)&geometry_entity->sphere.stack_count);
			} else if (geometry_entity->geometry_type == GEOMETRY_TYPE_GRID) {
				gui::text("Geometry type: Grid");
				gui::edit_field("Width", &geometry_entity->grid.width);
				gui::edit_field("Depth", &geometry_entity->grid.depth);
				gui::edit_field("Rows count", (s32 *)&geometry_entity->grid.rows_count);
				gui::edit_field("Columns count", (s32 *)&geometry_entity->grid.columns_count);
			}
		} else if (entity_type == ENTITY_TYPE_LIGHT) {
			Light *light = &game_world->lights[entity_index];
			if (light->type == DIRECTIONAL_LIGHT_TYPE) {
				gui::text("Direction Light");
				if (gui::edit_field("Direction", &light->direction) || gui::edit_field("Color", &light->color, "R", "G", "B")) {
					render_world->update_lights();
				}
			}
		}
		if (entity && (entity->bounding_box_type != BOUNDING_BOX_TYPE_UNKNOWN) && (entity_type != DIRECTIONAL_LIGHT_TYPE) && (entity_type != ENTITY_TYPE_UNKNOWN)) {
			if (!draw_AABB_states.key_in_table(entity->idx)) {
				draw_AABB_states[entity->idx] = false;
			}
			bool was_click = gui::radio_button("Draw AABB", &draw_AABB_states[entity->idx]);
			Entity_Id entity_id = Entity_Id(entity_type, entity_index);

			if (was_click && draw_AABB_states[entity->idx]) {
				Render_Entity *render_entity = render_world->find_render_entity(entity_id);

				if (render_entity) {
					char *name = format(&entity->AABB_box.min, &entity->AABB_box.max);
					String_Id string_id = fast_hash(name);

					Mesh_Idx mesh_idx;
					if (!render_world->line_meshes.mesh_table.get(string_id, &mesh_idx)) {
						Line_Mesh AABB_mesh;
						make_AABB_mesh(&entity->AABB_box.min, &entity->AABB_box.max, &AABB_mesh);
						render_world->line_meshes.add_mesh(name, &AABB_mesh, &mesh_idx);
					}
					free_string(name);

					Render_Entity new_render_entity;
					new_render_entity.entity_id = Entity_Id(entity_type, entity_index);
					new_render_entity.world_matrix_idx = render_entity->world_matrix_idx;
					new_render_entity.mesh_idx = mesh_idx;

					render_world->bounding_box_entities.push(new_render_entity);
				}
			} else if (was_click && !draw_AABB_states[entity->idx]) {
				u32 render_entity_index = 0;
				if (find_render_entity(&render_world->bounding_box_entities, entity_id, &render_entity_index)) {
					render_world->bounding_box_entities.remove(render_entity_index);
				}
			}
		}
		gui::end_child();
	}
}

bool Game_World_Window::draw_entity_list(const char *list_name, u32 list_count, Entity_Type type)
{
	for (u32 i = 0; i < list_count; i++) {
		if (gui::button(list_name)) {
			entity_index = i;
			entity_type = type;
			return true;
		}
	}
	return false;
}

void Render_World_Window::init(Engine *engine)
{
	Editor_Window::init(engine);

	Texture_Desc shadows_texture_desc;
	shadows_texture_desc.width = DIRECTION_SHADOW_MAP_WIDTH;
	shadows_texture_desc.height = DIRECTION_SHADOW_MAP_HEIGHT;
	shadows_texture_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
	shadows_texture_desc.mip_levels = 1;
	shadows_texture_desc.usage = RESOURCE_USAGE_DYNAMIC;
	shadows_texture_desc.bind = BIND_SHADER_RESOURCE;
	shadows_texture_desc.cpu_access = CPU_ACCESS_WRITE;

	Engine::get_render_system()->gpu_device.create_texture_2d(&shadows_texture_desc, &shadow_display_texture);

	fill_texture_with_value((void *)&Color::Red, &shadow_display_texture);
}

void Render_World_Window::update()
{
	Texture_Desc texture_desc;
	texture_desc.width = DIRECTION_SHADOW_MAP_WIDTH;
	texture_desc.height = DIRECTION_SHADOW_MAP_HEIGHT;
	texture_desc.format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	texture_desc.mip_levels = 1;
	texture_desc.usage = RESOURCE_USAGE_STAGING;
	texture_desc.bind = 0;
	texture_desc.cpu_access = CPU_ACCESS_READ | CPU_ACCESS_WRITE;

	Texture2D temp_shadow_atlas;

	Engine::get_render_system()->gpu_device.create_texture_2d(&texture_desc, &temp_shadow_atlas, false);
	Engine::get_render_system()->render_pipeline.copy_resource(temp_shadow_atlas, render_world->shadow_atlas);

	u32 *shadow_atlas_pixel = (u32 *)Engine::get_render_system()->render_pipeline.map(temp_shadow_atlas, MAP_TYPE_READ);
	u32 *shadow_dispaly_pixel = (u32 *)Engine::get_render_system()->render_pipeline.map(shadow_display_texture);
	
	for (u32 row = 0; row < shadow_display_texture.height; row++) {
		for (u32 column = 0; column < shadow_display_texture.width; column++) {
			R24U8 depth = R24U8(shadow_atlas_pixel[column]);
			u8 r = u8(depth.numerator >> 24);
			u8 g = u8(depth.numerator >> 16);
			u8 b = u8(depth.numerator >> 15);
			//u8 b = u8(depth.numerator >> 16);
			Color color = Color(b, b, b);
			shadow_dispaly_pixel[column] = color.get_packed_rgba();
		}
		shadow_atlas_pixel += shadow_display_texture.width;
		shadow_dispaly_pixel += shadow_display_texture.width;
	}

	Engine::get_render_system()->render_pipeline.unmap(temp_shadow_atlas);
	Engine::get_render_system()->render_pipeline.unmap(shadow_display_texture);

	temp_shadow_atlas.release();
}

void Render_World_Window::draw()
{
	update();

	static bool state = false;

	gui::button("Shadow atls", &state);

	if (state) {
		gui::set_next_window_size(1000, 800);
		if (gui::begin_window("Shadow atls")) {
			Render_World *render_world = Engine::get_render_world();
			gui::image(&shadow_display_texture, 700, 700);
			gui::end_window();
		}
	}
}
