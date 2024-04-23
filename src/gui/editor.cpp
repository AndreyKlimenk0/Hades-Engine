#include <assert.h>

#include "editor.h"
#include "../sys/engine.h"
#include "../sys/sys_local.h"

#include "../libs/str.h"
#include "../libs/png_image.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/os/path.h"
#include "../libs/math/vector.h"
#include "../libs/math/vector.h"
#include "../libs/mesh_loader.h"
#include "../libs/geometry_helper.h"

#include "../render/render_api.h"
#include "../render/render_system.h"
#include "../render/render_helpers.h"

#include "../collision/collision.h"

inline bool get_render_pass_index(const char *name, Array<Render_Pass *> &render_passes, u32 *index)
{
	assert(name);
	assert(index);

	for (u32 i = 0; i < render_passes.count; i++) {
		if (render_passes[i]->name == name) {
			*index = i;
			return true;
		}
	}
	return false;
}

inline void calculate_picking_ray(Vector3 &camera_position, Matrix4 &view_matrix, Matrix4 &perspective_matrix, Ray *ray)
{
	Vector2 xy_ndc_point = from_raster_to_screen_space(Mouse_State::x, Mouse_State::y, Render_System::screen_width, Render_System::screen_height);
	Vector4 ndc_point = Vector4(xy_ndc_point.x, xy_ndc_point.y, 1.0f, 1.0f);

	Vector4 mouse_point_in_world = ndc_point * inverse(view_matrix * perspective_matrix);
	mouse_point_in_world /= mouse_point_in_world.w;

	ray->origin = camera_position;
	ray->direction = to_vector3(mouse_point_in_world) - camera_position;
}

struct Ray_Entity_Intersection {
	struct Result {
		Entity_Id entity_id;
		Render_Entity_Idx render_entity_idx;
		Vector3 intersection_point;
	};
	static bool detect_intersection(Ray *picking_ray, Game_World *game_world, Render_World *render_world, Result *result);
};

bool Ray_Entity_Intersection::detect_intersection(Ray *picking_ray, Game_World *game_world, Render_World *render_world, Result *result)
{
	Array<Result> intersected_entities;
	for (u32 i = 0; i < render_world->game_render_entities.count; i++) {
		Entity_Id entity_id = render_world->game_render_entities[i].entity_id;
		Entity *entity = game_world->get_entity(entity_id);

		if (entity->bounding_box_type == BOUNDING_BOX_TYPE_AABB) {
			Result intersection_result;
			if (::detect_intersection(picking_ray, &entity->AABB_box, &intersection_result.intersection_point)) {
				intersection_result.entity_id = entity_id;
				intersection_result.render_entity_idx = i;
				intersected_entities.push(intersection_result);
			}
		}
	}
	if (!intersected_entities.is_empty()) {
		if (intersected_entities.count > 1) {
			u32 most_near_entity_to_camera = 0;
			float most_small_distance = FLT_MAX;
			for (u32 i = 0; i < intersected_entities.count; i++) {
				// A ray origin equals to a camera position.
				float intersection_point_camera_distance = find_distance(&picking_ray->origin, &intersected_entities[i].intersection_point);
				if (intersection_point_camera_distance < most_small_distance) {
					most_near_entity_to_camera = i;
					most_small_distance = intersection_point_camera_distance;
				}
			}
			*result = intersected_entities[most_near_entity_to_camera];
		} else {
			*result = intersected_entities.get_first();
		}
		return true;
	}
	return false;
}

void Editor_Window::init(Engine *engine)
{
	assert(engine);

	editor = &engine->editor;
	game_world = &engine->game_world;
	render_world = &engine->render_world;
	render_system = &engine->render_sys;
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
	entity_type_helper = MAKE_ENUM_HELPER(Entity_Type, ENTITY_TYPE_UNKNOWN, ENTITY_TYPE_ENTITY, ENTITY_TYPE_LIGHT, ENTITY_TYPE_GEOMETRY, ENTITY_TYPE_CAMERA);
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
	position = Vector3(0.0f, 0.0f, 0.0f);
	direction = Vector3(0.2f, -1.0f, 0.2f);
	color = Vector3(255.0, 255.0, 255.0);

	camera_fields.position = Vector3::zero;
	camera_fields.target = Vector3::base_z;
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
				Color normalized_light_color = { (s32)color.x, (s32)color.y, (s32)color.y };
				game_world->make_direction_light(direction, to_vector3(normalized_light_color));
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
				AABB aabb = make_AABB(&mesh);
				game_world->attach_AABB(entity_id, &aabb);

				char *mesh_name = format("Box", box.width, box.height, box.depth);
				Mesh_Idx mesh_idx;
				render_world->add_mesh(mesh_name, &mesh, &mesh_idx);

				Render_Entity_Textures render_entity_textures;
				render_entity_textures.ambient_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				render_entity_textures.normal_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				render_entity_textures.diffuse_texture_idx = render_world->render_entity_texture_storage.default_texture_idx;
				render_entity_textures.specular_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				render_entity_textures.displacement_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;

				render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx, &render_entity_textures);

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

				Render_Entity_Textures render_entity_textures;
				render_entity_textures.ambient_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				render_entity_textures.normal_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				render_entity_textures.diffuse_texture_idx = render_world->render_entity_texture_storage.default_texture_idx;
				render_entity_textures.specular_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				render_entity_textures.displacement_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;

				render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx, &render_entity_textures);

				free_string(mesh_name);
			}
		}
	} else if (type == ENTITY_TYPE_CAMERA) {
		gui::edit_field("Position", &camera_fields.position);
		gui::edit_field("Target", &camera_fields.target);

		if (gui::button("Make")) {
			game_world->make_camera(camera_fields.position, camera_fields.target);
		}
	}
}

void Game_World_Window::init(Engine *engine)
{
	Editor_Window::init(engine);

	window_width_delta = 20;
	world_entities_height = 200;
	entity_info_height = 400;
	window_style = WINDOW_DEFAULT_STYLE & ~WINDOW_OUTLINES;

	world_entities_window_theme.background_color = Color(38);
	world_entities_window_theme.header_color = Color(36);
	world_entities_window_theme.place_between_rects = 0;
	world_entities_window_theme.horizontal_offset_from_sides = 0;
	world_entities_window_theme.vertical_offset_from_sides = 0;

	entity_info_window_theme.background_color = Color(38);
	entity_info_window_theme.header_color = Color(36);
	entity_info_window_theme.place_between_rects = 8;

	buttons_theme.color = world_entities_window_theme.background_color;
	buttons_theme.aligment = LEFT_ALIGNMENT;
}

void Game_World_Window::draw()
{
	Size_s32 editor_window_size = gui::get_window_size();
	gui::set_next_window_size(editor_window_size.width, world_entities_height);
	gui::set_theme(&world_entities_window_theme);
	if (gui::begin_child("World entities", (WINDOW_DEFAULT_STYLE & ~WINDOW_OUTLINES))) {
		buttons_theme.rect.width = gui::get_window_size().width;
		gui::set_theme(&buttons_theme);
		
		draw_entity_list("Camera", game_world->cameras.count, ENTITY_TYPE_CAMERA);
		draw_entity_list("Light", game_world->lights.count, ENTITY_TYPE_LIGHT);
		draw_entity_list("Geometry", game_world->geometry_entities.count, ENTITY_TYPE_GEOMETRY);
		
		gui::reset_button_theme();
		gui::end_child();
	}
	gui::reset_window_theme();
	
	gui::set_theme(&entity_info_window_theme);
	gui::set_next_window_size(editor_window_size.width, entity_info_height);
	if (gui::begin_child("Entity info", (WINDOW_DEFAULT_STYLE & ~WINDOW_OUTLINES))) {
		Entity *entity = game_world->get_entity(editor->picked_entity);
		if (entity) {
			if (entity->type == ENTITY_TYPE_GEOMETRY) {
				Geometry_Entity *geometry_entity = static_cast<Geometry_Entity *>(entity);
				
				Vector3 entity_position = geometry_entity->position;
				gui::edit_field("Position", &entity_position);
				if (entity_position != geometry_entity->position) {
					game_world->place_entity(geometry_entity, entity_position);
				}
				
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
			} else if (entity->type == ENTITY_TYPE_LIGHT) {
				Light *light = static_cast<Light *>(entity);

				if (light->type == DIRECTIONAL_LIGHT_TYPE) {
					gui::text("Direction Light");
					if (gui::edit_field("Direction", &light->direction) || gui::edit_field("Color", &light->color, "R", "G", "B")) {
						render_world->update_lights();
					}
				}
			} else if (entity->type == ENTITY_TYPE_CAMERA) {
				Camera *camera = static_cast<Camera *>(entity);

				gui::edit_field("Position", &camera->position);
				gui::edit_field("Target", &camera->target);

				if (gui::button("To origin")) {
					camera->position = Vector3::zero;
					camera->target = Vector3::base_z;
				}
				if (!draw_frustum_states.key_in_table(camera->idx)) {
					draw_frustum_states[camera->idx] = false;
				}

				bool was_click = gui::radio_button("Draw frustum", &draw_frustum_states[camera->idx]);
				if (was_click && draw_frustum_states[camera->idx] && !find_render_entity(&render_world->line_render_entities, editor->picked_entity)) {
					char *name = format(Render_System::screen_width, Render_System::screen_height, 1000, render_system->view.fov);
					String_Id string_id = fast_hash(name);

					Mesh_Idx mesh_idx;
					if (!render_world->line_meshes.mesh_table.get(string_id, &mesh_idx)) {
						Line_Mesh frustum_mesh;
						make_frustum_mesh(render_system->view.fov, render_system->view.ratio, 1.0f, 1000.0f, &frustum_mesh);
						render_world->add_mesh(name, &frustum_mesh, &mesh_idx);
					}
					free_string(name);

					Render_Entity_Textures render_entity_textures;
					render_entity_textures.ambient_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
					render_entity_textures.normal_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
					render_entity_textures.diffuse_texture_idx = render_world->render_entity_texture_storage.default_texture_idx;
					render_entity_textures.specular_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
					render_entity_textures.displacement_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;

					render_world->add_render_entity(RENDERING_TYPE_LINES_RENDERING, editor->picked_entity, mesh_idx, &render_entity_textures, (void *)&Color::Red);

				} else if (was_click && !draw_frustum_states[camera->idx]) {
					// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
					u32 render_entity_index = 0;
					if (find_render_entity(&render_world->line_render_entities, editor->picked_entity, &render_entity_index)) {
						render_world->line_render_entities.remove(render_entity_index);
					}
				}
			}
		}
		if (entity && (entity->bounding_box_type != BOUNDING_BOX_TYPE_UNKNOWN) && (entity->type != DIRECTIONAL_LIGHT_TYPE) && (entity->type != ENTITY_TYPE_UNKNOWN)) {
			if (!draw_AABB_states.key_in_table(entity->idx)) {
				draw_AABB_states[entity->idx] = false;
			}
			bool was_click = gui::radio_button("Draw AABB", &draw_AABB_states[entity->idx]);
			if (was_click && draw_AABB_states[entity->idx]) {
				Render_Entity *render_entity = find_render_entity(&render_world->game_render_entities, editor->picked_entity);

				if (render_entity) {
					char *name = format(&entity->AABB_box.min, &entity->AABB_box.max);
					String_Id string_id = fast_hash(name);

					Mesh_Idx mesh_idx;
					if (!render_world->line_meshes.mesh_table.get(string_id, &mesh_idx)) {
						Line_Mesh AABB_mesh;
						make_AABB_mesh(&entity->AABB_box.min, &entity->AABB_box.max, &AABB_mesh);
						render_world->add_mesh(name, &AABB_mesh, &mesh_idx);
					}
					free_string(name);

					Render_Entity new_render_entity;
					new_render_entity.entity_id = editor->picked_entity;
					new_render_entity.world_matrix_idx = render_entity->world_matrix_idx;
					new_render_entity.mesh_idx = mesh_idx;

					render_world->line_render_entities.push(new_render_entity);
				}
			} else if (was_click && !draw_AABB_states[entity->idx]) {
				u32 render_entity_index = 0;
				if (find_render_entity(&render_world->line_render_entities, editor->picked_entity, &render_entity_index)) {
					render_world->line_render_entities.remove(render_entity_index);
				}
			}
		}
		gui::end_child();
	}
	gui::reset_window_theme();
}

bool Game_World_Window::draw_entity_list(const char *list_name, u32 list_count, Entity_Type type)
{
	for (u32 i = 0; i < list_count; i++) {
		if (gui::button(list_name)) {
			editor->picked_entity = Entity_Id(type, i);
			return true;
		}
	}
	return false;
}

void Render_World_Window::init(Engine *engine)
{
	Editor_Window::init(engine);
}

void Render_World_Window::update()
{
}

void Render_World_Window::draw()
{
	if (gui::radio_button("Debug cascaded shadows", &debug_cascaded_shadows)) {
		if (render_world->render_passes.forward_light.is_valid && render_world->render_passes.debug_cascade_shadows.is_valid) {
			if (debug_cascaded_shadows) {
				u32 forward_light_index = 0;
				if (get_render_pass_index("Forward_Light", render_world->every_frame_render_passes, &forward_light_index)) {
					render_world->every_frame_render_passes[forward_light_index] = &render_world->render_passes.debug_cascade_shadows;
				} else {
					print("Render_World_Window::draw: Failed turn on cascaded shadows debuging. Forward light pass was not found.");
				}
			} else {
				u32 debug_cascaded_shadows = 0;
				if (get_render_pass_index("Debug_Cascade_Shadows", render_world->every_frame_render_passes, &debug_cascaded_shadows)) {
					render_world->every_frame_render_passes[debug_cascaded_shadows] = &render_world->render_passes.forward_light;
				} else {
					print("Render_World_Window::draw: Failed turn off cascaded shadows debuging. Debug cascaded shadows pass was not found.");
				}
			}
		} else {
			print("Render_World_Window::draw: Cascaded shadows debuging doesn't work because the render passes is not valid.");
		}
	}

    Array<String> strings;
    for (u32 i = 0; i < game_world->cameras.count; i++) {
 		strings.push("Camera");
    }
    static u32 index = 0;
	static u32 prev_index = 1;

    gui::list_box(&strings, &index);
	bool was_update = false;
	if (index != prev_index) {
		Entity_Id camera_id = Entity_Id(ENTITY_TYPE_CAMERA, index);
		editor->editor_camera_id = camera_id;
		render_world->set_camera_for_rendering(camera_id);
		prev_index = index;
		was_update = true;
	}

	static u32 index2 = 0;
	static u32 prev_index2 = 1;

	gui::list_box(&strings, &index2);

	if (index2 != prev_index2) {
		Entity_Id camera_id = Entity_Id(ENTITY_TYPE_CAMERA, index2);
		render_world->set_camera_for_debuging(camera_id);
		prev_index2 = index2;
	} else if (was_update) {
		Entity_Id camera_id = Entity_Id(ENTITY_TYPE_CAMERA, index);
		render_world->set_camera_for_debuging(camera_id);
		prev_index2 = index;
	}
}

void Drop_Down_Entity_Window::init(Engine *engine)
{
	Editor_Window::init(engine);

	window_size = { 220, 150 };

	window_theme.background_color = Color(40, 40, 40);
	window_theme.header_color = Color(36, 36, 36);
	window_theme.place_between_rects = 0;

	buttons_theme.rect.set_size(window_size.width, 25);
	buttons_theme.color = window_theme.background_color;
	buttons_theme.aligment = LEFT_ALIGNMENT;
}

void Drop_Down_Entity_Window::draw()
{
	gui::set_next_window_size(window_size.width, window_size.height);
	gui::set_next_window_pos(mouse_position.x, mouse_position.y);
	gui::set_next_theme(&window_theme);

	if (gui::begin_window("Actions", NO_WINDOW_STYLE)) {
		gui::set_theme(&buttons_theme);

		gui::button("Scale");
		gui::button("Rotate");
		if (gui::button("Move")) {
			set_cursor(CURSOR_TYPE_MOVE);
			editor->draw_drop_down_entity_window = false;
			editor->editor_mode = EDITOR_MODE_MOVE_ENTITY;
		}
		gui::button("Copy");
		if (gui::button("Delete")) {
			game_world->delete_entity(editor->picked_entity);
			u32 render_entity_index = render_world->delete_render_entity(editor->picked_entity);

			render_world->render_passes.outlining.delete_render_entity_index(render_entity_index);

			editor->picked_entity.reset();
			editor->draw_drop_down_entity_window = false;
		}
		gui::reset_button_theme();
		gui::end_window();
	}
}

Editor::Editor()
{
}

Editor::~Editor()
{
}

void Editor::init(Engine *engine)
{
	render_sys = &engine->render_sys;
	game_world = &engine->game_world;
	render_world = &engine->render_world;

	command_window.init(engine);
	make_entity_window.init(engine);
	game_world_window.init(engine);
	render_world_window.init(engine);
	drop_down_entity_window.init(engine);

	if (game_world->cameras.is_empty()) {
		editor_camera_id = game_world->make_camera(Vector3(0.0f, 20.0f, -250.0f), Vector3(0.0f, 0.0f, -1.0f));
		engine->render_world.set_camera_for_rendering(editor_camera_id);
	} else {
		editor_camera_id = get_entity_id(&game_world->cameras.get_first());
		engine->render_world.set_camera_for_rendering(editor_camera_id);
	}

	key_command_bindings.init();
	key_command_bindings.set("move_camera_forward", KEY_W);
	key_command_bindings.set("move_camera_back", KEY_S);
	key_command_bindings.set("move_camera_right", KEY_D);
	key_command_bindings.set("move_camera_left", KEY_A);
	key_command_bindings.set("start_rotate_camera", KEY_LMOUSE);
	key_command_bindings.set("end_rotate_camera", KEY_LMOUSE, false);
	key_command_bindings.set("", KEY_RMOUSE); // Don't want to get annoyiny messages

	key_bindings.bind(KEY_CTRL, KEY_C);
}

void Editor::handle_events()
{
	if (!gui::were_events_handled() && (editor_mode == EDITOR_MODE_COMMON)) {
		key_bindings.handle_events();

		//@Note: In the future here better to use linear allocator.
		Array<Editor_Command> editor_commands;
		Array<Entity_Command *> entity_commands;

		convert_user_input_events_to_edtior_commands(&editor_commands);
		convert_editor_commands_to_entity_commands(&editor_commands, &entity_commands);

		Camera *camera = game_world->get_camera(editor_camera_id);
		camera->handle_commands(&entity_commands);

		free_memory(&entity_commands);
	}
}

struct Moving_Entity {
	bool moving_entity = false;
	float distance; // A distance between a editor camera and an moving entity
	Vector3 ray_entity_intersection_point;
};

void Editor::picking()
{
	Camera *camera = game_world->get_camera(render_world->render_camera.camera_id);
	Ray picking_ray;
	calculate_picking_ray(camera->position, render_world->render_camera.debug_view_matrix, render_sys->view.perspective_matrix, &picking_ray);

	static Moving_Entity moving_entity_info;

	if (was_key_just_pressed(KEY_LMOUSE) && (editor_mode == EDITOR_MODE_MOVE_ENTITY)) {
		Ray_Entity_Intersection::Result intersection_result;
		if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result) && (picked_entity == intersection_result.entity_id)) {
			moving_entity_info.moving_entity = true;
			moving_entity_info.distance = find_distance(&camera->position, &intersection_result.intersection_point);
			moving_entity_info.ray_entity_intersection_point = intersection_result.intersection_point;
		} else {
			editor_mode = EDITOR_MODE_COMMON;
			set_cursor(CURSOR_TYPE_ARROW);
		}
	}

	if (moving_entity_info.moving_entity) {
		Vector3 moved_picking_ray_point = picking_ray.origin + (Vector3)(normalize(picking_ray.direction) * moving_entity_info.distance);
		Vector3 difference = moved_picking_ray_point - moving_entity_info.ray_entity_intersection_point;

		Entity *entity = game_world->get_entity(picked_entity);
		game_world->move_entity(entity, difference);

		moving_entity_info.ray_entity_intersection_point += difference;
	}

	if (was_key_just_released(KEY_LMOUSE) && moving_entity_info.moving_entity) {
		moving_entity_info.moving_entity = false;
	}

	if (!gui::were_events_handled() && was_click(KEY_RMOUSE) && valid_entity_id(picked_entity)) {
		Ray_Entity_Intersection::Result intersection_result;
		if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result)) {
			if (picked_entity == intersection_result.entity_id) {
				draw_drop_down_entity_window = true;
				drop_down_entity_window.mouse_position = { Mouse_State::x, Mouse_State::y };
			}
		}
	}
	if (!gui::were_events_handled() && was_click(KEY_LMOUSE)) {
		Outlining_Pass *outlining_pass = &render_world->render_passes.outlining;
		outlining_pass->reset_render_entity_indices();

		Ray_Entity_Intersection::Result intersection_result;
		if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result)) {
			gui::make_tab_active(game_world_tab_gui_id);
			picked_entity = intersection_result.entity_id;
			outlining_pass->add_render_entity_index(intersection_result.render_entity_idx);
		} else {
			picked_entity.reset();
		}
	}
}

void Editor::update()
{
	if (key_bindings.was_binding_triggered(KEY_CTRL, KEY_C)) {
		if (draw_command_window) {
			draw_command_window = false;
		} else {
			draw_command_window = true;
		}
	}
	if (!gui::were_events_handled() && were_key_events()) {
		if (draw_drop_down_entity_window) {
			draw_drop_down_entity_window = false;
		}
	}
	render_world_window.update();
	picking();
}

void Editor::render()
{
	gui::begin_frame();
	if (gui::begin_window("Editor", WINDOW_DEFAULT_STYLE | WINDOW_TAB_BAR)) {

		if (gui::add_tab("Make Entity")) {
			make_entity_window.draw();
		}

		if (gui::add_tab("Game World")) {
			game_world_window.draw();
		}

		if (gui::add_tab("Render World")) {
			render_world_window.draw();
		}

		gui::end_window();
	}
	if (draw_drop_down_entity_window) {
		drop_down_entity_window.draw();
	}
	if (draw_command_window) {
		command_window.draw();
	}
	gui::end_frame();
}

void Editor::convert_editor_commands_to_entity_commands(Array<Editor_Command> *editor_commands, Array<Entity_Command *> *entity_commands)
{
	static s32 last_x = 0;
	static s32 last_y = 0;
	static bool rotate_camera = false;

	for (u32 i = 0; i < editor_commands->count; i++) {
		Editor_Command &editor_command = editor_commands->get(i);
		String &command = editor_command.command;
		void *additional_info = editor_command.additional_info;

		if (command == "move_camera_forward") {
			Entity_Command_Move *move_command = new Entity_Command_Move;
			move_command->move_direction = MOVE_DIRECTION_FORWARD;
			move_command->distance = editor_settings.camera_speed;
			entity_commands->push(move_command);

		} else if (command == "move_camera_back") {
			Entity_Command_Move *move_command = new Entity_Command_Move;
			move_command->move_direction = MOVE_DIRECTION_BACK;
			move_command->distance = editor_settings.camera_speed;
			entity_commands->push(move_command);

		} else if (command == "move_camera_left") {
			Entity_Command_Move *move_command = new Entity_Command_Move;
			move_command->move_direction = MOVE_DIRECTION_LEFT;
			move_command->distance = editor_settings.camera_speed;
			entity_commands->push(move_command);

		} else if (command == "move_camera_right") {
			Entity_Command_Move *move_command = new Entity_Command_Move;
			move_command->move_direction = MOVE_DIRECTION_RIGHT;
			move_command->distance = editor_settings.camera_speed;
			entity_commands->push(move_command);

		} else if (command == "start_rotate_camera") {
			rotate_camera = true;
			last_x = Mouse_State::x;
			last_y = Mouse_State::y;
			print("Start rotate camera");

		} else if (command == "end_rotate_camera") {
			rotate_camera = false;
			print("End rotate camera");

		} else if (command == "rotate_camera") {
			if (!rotate_camera) {
				continue;
			}
			print("Rotate camera");
			Mouse_Info *mouse_info = (Mouse_Info *)additional_info;
			float x_angle = degrees_to_radians((float)(mouse_info->x - last_x));
			float y_angle = -degrees_to_radians((float)(mouse_info->y - last_y));

			Entity_Command_Rotate *rotate_command = new Entity_Command_Rotate;
			rotate_command->x_angle = x_angle * editor_settings.camera_rotation_speed;
			rotate_command->y_angle = y_angle * editor_settings.camera_rotation_speed;

			entity_commands->push(rotate_command);

			last_x = mouse_info->x;
			last_y = mouse_info->y;
		} else {
			print("Editor::convert_editor_commands_to_entity_commands: For the editor command {} there is no a entity command.", command);
		}
	}
}

void Editor::convert_user_input_events_to_edtior_commands(Array<Editor_Command> *editor_commands)
{
	Queue<Event> *events = get_event_queue();
	for (Queue_Node<Event> *node = events->first; node != NULL; node = node->next) {
		Event *event = &node->item;

		Editor_Command editor_command;
		if (event->type == EVENT_TYPE_KEY) {
			Find_Command_Result result = key_command_bindings.find_command(event->key_info.key, event->key_info.key_state, &editor_command.command);
			if (result == COMMAND_FIND) {
				editor_commands->push(editor_command);

			} else if (result == COMMAND_NOT_FOUND) {
				print("Editor::convert_user_input_events_to_edtior_commands: There is no an editor key command binding for {}.", to_string(event->key_info.key));
			}

		} else if (event->type == EVENT_TYPE_MOUSE) {
			Editor_Command rotate_camera_command;
			editor_command.command = "rotate_camera";
			editor_command.additional_info = (void *)&event->mouse_info;
			editor_commands->push(editor_command);
		}
	}
}

inline void place_in_middle(Rect_s32 *in_element_place, Rect_s32 *placed_element)
{
	placed_element->x = ((in_element_place->width / 2) - (placed_element->width / 2)) + in_element_place->x;
	placed_element->y = ((in_element_place->height / 2) - (placed_element->height / 2)) + in_element_place->y;
}

struct Command {
	String name;

	virtual bool init(const char *command_name);
	virtual void draw_list() = 0;
	virtual void proccess_input(const char *data) = 0;
	virtual void execute(const char *data, void *context) = 0;
};

struct Data_Command : Command {
	String full_path_to_data_directory;
	String relative_path_to_data_directory;

	virtual bool init(const char *command_name, const char *data_directory_name);
	virtual void draw_list() = 0;
	virtual void proccess_input(const char *data) = 0;
};

struct Load_Mesh_Command : Data_Command {
	Array<Gui_List_Line_State> list_line_states;
	
	void draw_list();
	void proccess_input(const char *data);
	void execute(const char *data, void *context);
};

bool Command::init(const char *command_name)
{
	assert(command_name);
	name = command_name;
	return true;
}

bool Data_Command::init(const char *command_name, const char *data_directory_name)
{
	assert(command_name);
	assert(data_directory_name);

	if (Command::init(command_name)) {
		relative_path_to_data_directory = "data/";
		relative_path_to_data_directory.append(data_directory_name);
		return build_full_path_to_data_directory(data_directory_name, full_path_to_data_directory) && directory_exists(full_path_to_data_directory);
	}
	return false;
}

void Load_Mesh_Command::draw_list()
{
	Array<String> model_files;
	get_file_names_from_dir(full_path_to_data_directory, &model_files);
	if (model_files.count > list_line_states.count) {
		list_line_states.reserve(model_files.count);
	}

	Gui_List_Column columns[] = { {"Model", 75 }, { "Relative path", 25 } };
	if (gui::begin_list("Engine Models", columns, 2)) {
		for (u32 i = 0; i < model_files.count; i++) {
			if (gui::begin_line(&list_line_states[i])) {

				if (gui::left_mouse_click(list_line_states[i])) {
					
					execute(model_files[i], Engine::get_instance());
				}

				gui::begin_column("Model");
				gui::add_text(model_files[i], RECT_LEFT_ALIGNMENT);
				gui::end_column();

				gui::begin_column("Relative path");
				gui::add_text(relative_path_to_data_directory, RECT_LEFT_ALIGNMENT);
				gui::end_column();

				gui::end_line();
			}
		}
		gui::end_list();
	}
}

void Load_Mesh_Command::proccess_input(const char *data)
{
}

void Load_Mesh_Command::execute(const char *data, void *context)
{
	Engine *engine = (Engine *)context;
	Render_World *render_world = &engine->render_world;
	Game_World *game_world = &engine->game_world;

	String full_path_to_model;
	build_full_path_to_model_file(data, full_path_to_model);

	if (file_exists(full_path_to_model)) {
		Array<Import_Mesh> meshes;

		Mesh_Loader mesh_loader;
		mesh_loader.load(full_path_to_model, meshes, false, false);

		Import_Mesh *imported_mesh = NULL;
		For(meshes, imported_mesh) {
			Mesh_Idx mesh_idx;
			if (render_world->add_mesh(imported_mesh->mesh.name, &imported_mesh->mesh, &mesh_idx)) {
				Render_Entity_Textures render_entity_textures;
				render_entity_textures.ambient_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				render_entity_textures.normal_texture_idx = load_normal_texture(imported_mesh->normal_texture, this);
				render_entity_textures.diffuse_texture_idx = load_diffuse_texture(imported_mesh->diffuse_texture, this);
				render_entity_textures.specular_texture_idx = load_specular_texture(imported_mesh->specular_texture, this);
				render_entity_textures.displacement_texture_idx = load_displacenet_texture(imported_mesh->displacement_texture, this);

				if (imported_mesh->mesh_instances.count > 0) {
					for (u32 j = 0; j < imported_mesh->mesh_instances.count; j++) {
						Import_Mesh::Transform_Info t = imported_mesh->mesh_instances[j];
						Entity_Id entity_id = game_world->make_entity(t.scaling, t.rotation, t.translation);
						render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx, &render_entity_textures);
					}
				} else {
					Entity_Id entity_id = game_world->make_entity(Vector3::one, Vector3::zero, Vector3::zero);
					render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx, &render_entity_textures);
				}
			}
		}
		print("Finish of creating entities");

	}
}

void Command_Window::init(Engine *engine)
{
	Editor_Window::init(engine);

	String path_to_cmd_icon_image;
	build_full_path_to_editor_file("command-icon.png", path_to_cmd_icon_image);

	u32 width;
	u32 height;
	u8 *data = NULL;
	if (load_png_file(path_to_cmd_icon_image, &data, &width, &height)) {
		Texture2D_Desc texture_desc;
		texture_desc.width = width;
		texture_desc.height = height;
		texture_desc.data = data;
		texture_desc.mip_levels = 1;
		render_system->gpu_device.create_texture_2d(&texture_desc, &cmd_icon_texture);
		render_system->gpu_device.create_shader_resource_view(&texture_desc, &cmd_icon_texture);
		
		DELETE_PTR(data);
	}

	Rect_s32 display;
	display.set_size(Render_System::screen_width, Render_System::screen_height);
	command_window_rect.set_size(600, 500);
	place_in_middle(&display, &command_window_rect);
	command_window_rect.y = 200;


	Load_Mesh_Command *load_mesh_command = new Load_Mesh_Command();
	if (load_mesh_command->init("Load mesh", "models")) {
		commands.push(load_mesh_command);
	} else {
		DELETE_PTR(load_mesh_command);
	}

	command_list_state.reserve(commands.count);
	if (!command_list_state.is_empty()) {
		command_list_state[0] = 0x1;
	}

	command_window_theme.background_color = Color(20);

	command_edit_field_theme.rect.set_size(command_window_rect.width - command_window_theme.horizontal_offset_from_sides * 2, 30);
	command_edit_field_theme.draw_label = false;
	command_edit_field_theme.color = Color(30);

	list_theme.line_height = 30;
	list_theme.column_filter = false;
	list_theme.line_text_offset = command_window_theme.horizontal_offset_from_sides + command_edit_field_theme.text_shift;
	list_theme.window_size.width = command_window_rect.width;
	list_theme.background_color = Color(20);
	list_theme.line_color = Color(20);
}

void Command_Window::draw()
{
	gui::set_next_window_pos(command_window_rect.x, command_window_rect.y);
	gui::set_next_window_size(command_window_rect.width, command_window_rect.height);
	gui::set_theme(&command_window_theme);
	
	if (gui::begin_window("Command window", 0)) {
		
		gui::set_theme(&command_edit_field_theme);
		gui::edit_field("Command field", &command_edit_filed_input_string);
		gui::reset_edit_field_theme();

		Gui_Window_Theme window_theme;
		window_theme.horizontal_offset_from_sides = 0;
		gui::set_theme(&window_theme);

		gui::set_theme(&list_theme);
		gui::make_next_list_active();

		if (!command) {
			Gui_List_Column columns[] = { {"Command", 75 }, { "Key-Binding", 25 } };
			if (gui::begin_list("Commands", columns, 2)) {
				for (u32 i = 0; i < commands.count; i++) {
					if (gui::begin_line(&command_list_state[i])) {

						if (gui::left_mouse_click(command_list_state[i])) {
							command = commands[i];
						}
						gui::begin_column("Command");
						gui::add_image(&cmd_icon_texture, RECT_LEFT_ALIGNMENT);
						gui::add_text(commands[i]->name, RECT_LEFT_ALIGNMENT);
						gui::end_column();

						gui::begin_column("Key-Binding");
						gui::add_text("Ctr + L + M", RECT_LEFT_ALIGNMENT);
						gui::end_column();

						gui::end_line();
					}
				}
				gui::end_list();
			}
		} else {
			command->draw_list();
		}
		
		
		gui::reset_window_theme();
		gui::reset_list_theme();
		
		gui::end_window();
	}
	gui::reset_window_theme();
}
//Array<String> found_commands;
//if (!text.is_empty()) {
//	for (u32 i = 0; i < commands.count; i++) {
//		if (commands[i].find(text.c_str(), 0, false) != -1) {
//			found_commands.push(commands[i]);
//		}
//	}
//} else {
//	found_commands = commands;
//}

