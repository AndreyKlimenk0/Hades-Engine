#include "editor.h"
#include "../sys/engine.h"
#include "../sys/sys_local.h"
#include "../libs/str.h"
#include "../libs/os/event.h"
#include "../libs/math/common.h"
#include "../libs/geometry_helper.h"
#include "../render/render_api.h"
#include "../render/render_system.h"
#include "../render/render_helpers.h"


void Editor_Window::init(Engine *engine)
{
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
				AABB aabb = make_AABB(&mesh);
				//@Note: don't like set_entity_AABB
				game_world->set_entity_AABB(entity_id, &aabb);

				char *mesh_name = format("Box", box.width, box.height, box.depth);
				Mesh_Idx mesh_idx;
				render_world->add_mesh(mesh_name, &mesh, &mesh_idx);

				render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx);

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

				render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx);

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

Editor::Editor()
{
}

Editor::~Editor()
{
}

void Editor::init(Engine *engine)
{
	game_world = &engine->game_world;

	make_entity_window.init(engine);
	game_world_window.init(engine);
	render_world_window.init(engine);

	if (game_world->cameras.is_empty()) {
		editor_camera_id = game_world->make_camera(Vector3(0.0f, 20.0f, -250.0f), Vector3(0.0f, 0.0f, -1.0f));
		engine->render_world.set_camera_for_rendering(editor_camera_id);
	}
	else {
		editor_camera_id = get_entity_id(&game_world->cameras.first_item());
		engine->render_world.set_camera_for_rendering(editor_camera_id);
	}

	key_command_bindings.init();
	key_command_bindings.set("move_camera_forward", KEY_W);
	key_command_bindings.set("move_camera_back",    KEY_S);
	key_command_bindings.set("move_camera_right",   KEY_D);
	key_command_bindings.set("move_camera_left",    KEY_A);
	key_command_bindings.set("start_rotate_camera", KEY_LMOUSE);
	key_command_bindings.set("end_rotate_camera",   KEY_LMOUSE, false);
	
	//Mesh_Loader *mesh_loader = Engine::get_mesh_loader();
	//mesh_loader->load("camera.fbx");
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
			last_x = Mouse_Input::x;
			last_y = Mouse_Input::y;
		
		} else if (command == "end_rotate_camera") {
			rotate_camera = false;

		} else if (command == "rotate_camera") {
			if (!rotate_camera) {
				continue;
			}
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
			Find_Command_Result result = key_command_bindings.find_command(event->key_info.key, event->key_info.is_pressed, &editor_command.command);
			if (result == COMMAND_FIND) {
				editor_commands->push(editor_command);
			
			} else if (result == COMMAND_NOT_FOUND) {
				print("Editor::convert_user_input_events_to_edtior_commands: There is no an editor key binding for {}.", to_string(event->key_info.key));
			}
		
		} else if (event->type == EVENT_TYPE_MOUSE) {
			Editor_Command rotate_camera_command;
			editor_command.command = "rotate_camera";
			editor_command.additional_info = (void *)&event->mouse_info;
			editor_commands->push(editor_command);
		}
	}
}

void Editor::handle_events()
{
	if (!gui::were_events_handled()) {
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

void Editor::update()
{
	render_world_window.update();
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
	if (gui::begin_child("World entities", (WINDOW_STYLE_DEFAULT & ~WINDOW_WITH_OUTLINES))) {
		buttons_theme.rect.width = window_size.width - window_width_delta;
		gui::set_theme(&buttons_theme);

		draw_entity_list("Camera", game_world->cameras.count, ENTITY_TYPE_CAMERA);
		draw_entity_list("Light", game_world->lights.count, ENTITY_TYPE_LIGHT);
		draw_entity_list("Geometry", game_world->geometry_entities.count, ENTITY_TYPE_GEOMETRY);

		gui::reset_button_theme();
		gui::end_child();
	}
	
	gui::set_next_theme(&entity_info_window_theme);
	gui::set_next_window_size(window_size.width - window_width_delta, entity_info_height);
	if (gui::begin_child("Entity info", (WINDOW_STYLE_DEFAULT & ~WINDOW_WITH_OUTLINES))) {
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
		} else if (entity_type == ENTITY_TYPE_CAMERA) {
			Camera *camera = &game_world->cameras[entity_index];
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
			Entity_Id entity_id = Entity_Id(entity_type, entity_index);
			
			if (was_click && draw_frustum_states[camera->idx] && !find_render_entity(&render_world->line_rendering_entities, entity_id)) {
				char *name = format(Render_System::screen_width, Render_System::screen_height, 1000, render_system->view.fov);
				String_Id string_id = fast_hash(name);

				Mesh_Idx mesh_idx;
				if (!render_world->line_meshes.mesh_table.get(string_id, &mesh_idx)) {
					Line_Mesh frustum_mesh;
					make_frustum_mesh(render_system->view.fov, render_system->view.ratio, 1.0f, 1000.0f, &frustum_mesh);
					render_world->add_mesh(name, &frustum_mesh, &mesh_idx);
				}
				free_string(name);
				render_world->add_render_entity(RENDERING_TYPE_LINES_RENDERING, entity_id, mesh_idx, (void *)&Color::Red);

			} else if (was_click && !draw_frustum_states[camera->idx]) {
				// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				u32 render_entity_index = 0;
				if (find_render_entity(&render_world->line_rendering_entities, entity_id, &render_entity_index)) {
					render_world->line_rendering_entities.remove(render_entity_index);
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
						render_world->add_mesh(name, &AABB_mesh, &mesh_idx);
					}
					free_string(name);

					Render_Entity new_render_entity;
					new_render_entity.entity_id = Entity_Id(entity_type, entity_index);
					new_render_entity.world_matrix_idx = render_entity->world_matrix_idx;
					new_render_entity.mesh_idx = mesh_idx;

					render_world->line_rendering_entities.push(new_render_entity);
				}
			} else if (was_click && !draw_AABB_states[entity->idx]) {
				u32 render_entity_index = 0;
				if (find_render_entity(&render_world->line_rendering_entities, entity_id, &render_entity_index)) {
					render_world->line_rendering_entities.remove(render_entity_index);
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

 // Texture_Desc shadows_texture_desc;
 // shadows_texture_desc.width = DIRECTION_SHADOW_MAP_WIDTH;
 // shadows_texture_desc.height = DIRECTION_SHADOW_MAP_HEIGHT;
 // shadows_texture_desc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
 // shadows_texture_desc.mip_levels = 1;
 // shadows_texture_desc.usage = RESOURCE_USAGE_DYNAMIC;
 // shadows_texture_desc.bind = BIND_SHADER_RESOURCE;
 // shadows_texture_desc.cpu_access = CPU_ACCESS_WRITE;

 // Engine::get_render_system()->gpu_device.create_texture_2d(&shadows_texture_desc, &shadow_display_texture);

 // fill_texture_with_value((void *)&Color::Red, &shadow_display_texture);
}


Matrix4 make_rotation_matrix_v2(Vector3 *direction, Vector3 *up_direction = NULL)
{
	assert(direction);

	Vector3 z_axis = normalize(direction);
	Vector3 up = { 0.0f, 1.0f, 0.0f };
	if (up_direction) {
		up = *up_direction;
	}
	Vector3 x_axis = normalize(&cross(&up, &z_axis));
	Vector3 y_axis = normalize(&cross(&z_axis, &x_axis));

	Matrix4 rotation_matrix = make_identity_matrix();
	rotation_matrix.set_row_0(Vector4(x_axis, 0.0f));
	rotation_matrix.set_row_1(Vector4(y_axis, 0.0f));
	rotation_matrix.set_row_2(Vector4(z_axis, 0.0f));
	return rotation_matrix;
}

void Render_World_Window::update()
{
 // Texture_Desc texture_desc;
 // texture_desc.width = DIRECTION_SHADOW_MAP_WIDTH;
 // texture_desc.height = DIRECTION_SHADOW_MAP_HEIGHT;
 // texture_desc.format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
 // texture_desc.mip_levels = 1;
 // texture_desc.usage = RESOURCE_USAGE_STAGING;
 // texture_desc.bind = 0;
 // texture_desc.cpu_access = CPU_ACCESS_READ | CPU_ACCESS_WRITE;

 // Texture2D temp_shadow_atlas;

	//Engine::get_render_system()->gpu_device.create_texture_2d(&texture_desc, &temp_shadow_atlas, false);
	//Engine::get_render_system()->render_pipeline.copy_resource(temp_shadow_atlas, render_world->shadow_atlas);

	//u32 *shadow_atlas_pixel = (u32 *)Engine::get_render_system()->render_pipeline.map(temp_shadow_atlas, MAP_TYPE_READ);
	//u32 *shadow_dispaly_pixel = (u32 *)Engine::get_render_system()->render_pipeline.map(shadow_display_texture);
	//
	//for (u32 row = 0; row < shadow_display_texture.height; row++) {
	//	for (u32 column = 0; column < shadow_display_texture.width; column++) {
	//		R24U8 depth = R24U8(shadow_atlas_pixel[column]);
	//		u8 r = u8(depth.numerator >> 24);
	//		u8 g = u8(depth.numerator >> 16);
	//		u8 b = u8(depth.numerator >> 8);
	//		//u8 b = u8(depth.numerator >> 16);
	//		Color color = Color(g, g, g);
	//		shadow_dispaly_pixel[column] = color.get_packed_rgba();
	//	}
	//	shadow_atlas_pixel += shadow_display_texture.width;
	//	shadow_dispaly_pixel += shadow_display_texture.width;
	//}

	//Engine::get_render_system()->render_pipeline.unmap(temp_shadow_atlas);
	//Engine::get_render_system()->render_pipeline.unmap(shadow_display_texture);

	//temp_shadow_atlas.release();

	if (show_cascaded_shadow_frustums) {
		for (u32 i = 0; i < frustum_entity_ids.count; i++) {
			//Draw_Cascade_Info draw_cascade_info = frustum_entity_ids[i];
			//Cascaded_Shadow *cascaded_shadow = &render_world->cascaded_shadow_maps[draw_cascade_info.cascaded_shadow_map_index].cascaded_shadows[draw_cascade_info.shadow_cascade_index];
			//Matrix3 orientation = make_rotation_matrix_v2(&cascaded_shadow->light_direction).to_matrix3();
			//Vector3 view_position = cascaded_shadow->range.get_center_point() * inverse(&render_world->render_camera.debug_view_matrix);
			//
			//Entity *entity = game_world->get_entity(draw_cascade_info.entity_id);
			//entity->position = view_position;
			//entity->orientation = orientation;
		}
	}
}

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

static const u32 CASCADES_COLOR_COUNT = 5;
static const Color cascade_colors[CASCADES_COLOR_COUNT] = {
	Color(255, 0, 0, 125), // red
	Color(240, 240, 7, 125), // yellow
	Color(27, 245, 7, 125), // green
	Color(0, 0, 255, 125), // blue
	Color(240, 162, 7, 125), // orange
};

#include <cmath>

void Render_World_Window::draw()
{
	//Cascaded_Shadow_Map *cascade_shadow_map = NULL;
	//For(Engine::get_render_world()->cascaded_shadow_maps, cascade_shadow_map) {
	//	Cascaded_Shadow *shadow_cascade = NULL;
	//	For(cascade_shadow_map->cascaded_shadows, shadow_cascade) {
	//		Matrix4 m = shadow_cascade->get_cascade_view_matrix();
	//		Vector3 position = m[3];
	//		char *text = format("Position", &position);
	//		gui::text(text);
	//		free_string(text);
	//	}
	//}
	if (gui::radio_button("Show cascaded shadow frustums", &show_cascaded_shadow_frustums)) {
		//if (show_cascaded_shadow_frustums) {
		//	for (u32 i = 0; i < render_world->cascaded_shadow_maps.count; i++) {
		//		for (u32 j = 0; j < render_world->cascaded_shadow_maps[i].cascaded_shadows.count; j++) {
		//			Cascaded_Shadow *cascaded_shadow = &render_world->cascaded_shadow_maps[i].cascaded_shadows[j];

		//			//Vector3 view_position = cascaded_shadow->range.get_center_point() * inverse(&render_world->render_camera.debug_view_matrix);
		//			//float radius = cascaded_shadow->cascade_width / 2.0f;
		//			//float texel_per_unit = CASCADE_WIDTH / (radius * 2.0f);

		//			//Matrix4 scalar = make_scale_matrix(texel_per_unit);
		//			//Matrix4 look_at = make_look_at_matrix(Vector3::zero, negate(&cascaded_shadow->light_direction)) * scalar;
		//			//Matrix4 inverse_look_at = inverse(&look_at);

		//			//view_position = view_position * look_at;
		//			//view_position.x = std::floor(view_position.x);
		//			//view_position.y = std::floor(view_position.y);
		//			//view_position = view_position * inverse_look_at;
		//			//
		//			//Matrix3 orientation = make_rotation_matrix_v2(&cascaded_shadow->light_direction).to_matrix3();
		//			//Entity_Id entity_id = game_world->make_entity(view_position, orientation);
		//			//
		//			//Draw_Cascade_Info draw_cascade_info;
		//			//draw_cascade_info.entity_id = entity_id;
		//			//draw_cascade_info.cascaded_shadow_map_index = i;
		//			//draw_cascade_info.shadow_cascade_index = j;
		//			//
		//			//frustum_entity_ids.push(draw_cascade_info);

		//			//Box frustum_box;
		//			//frustum_box.width = cascaded_shadow->cascade_width;
		//			//frustum_box.height = cascaded_shadow->cascade_width;
		//			//frustum_box.depth = cascaded_shadow->cascade_width;
		//			//
		//			//Triangle_Mesh frustum_box_mesh;
		//			//make_box_mesh(&frustum_box, &frustum_box_mesh);

		//			//char *frustum_box_name = format("Frustum_Box_{}_{}_{}", frustum_box.width, frustum_box.height, frustum_box.depth);
		//			//
		//			//Mesh_Idx mesh_idx;
		//			//Color r = Color::Red;
		//			//r.value.w = 0.5f;
		//			//if (render_world->add_mesh(frustum_box_name, &frustum_box_mesh, &mesh_idx)) {
		//			//	u32 color_index = j % CASCADES_COLOR_COUNT;
		//			//	render_world->add_render_entity(RENDERING_TYPE_VERTICES_RENDERING, entity_id, mesh_idx, (void *)&cascade_colors[color_index]);
		//			//}
		//			//free_string(frustum_box_name);
		//		}
		//	}
		//} else {
		//	for (u32 i = 0; i < frustum_entity_ids.count; i++) {
		//		Draw_Cascade_Info draw_cascade_info = frustum_entity_ids[i];
		//		Entity *entity = game_world->get_entity(draw_cascade_info.entity_id);
		//	}
		//}
	}

	if (gui::radio_button("Debug cascaded shadows", &debug_cascaded_shadows)) {
		if (render_world->render_passes.forward_light.initialized && render_world->render_passes.debug_cascade_shadows.initialized) {
			if (debug_cascaded_shadows) {
				u32 forward_light_index = 0;
				if (get_render_pass_index("Forward_Light", render_world->render_passes_array, &forward_light_index)) {
					render_world->render_passes_array[forward_light_index] = &render_world->render_passes.debug_cascade_shadows;
				} else {
					print("Render_World_Window::draw: Failed turn on cascaded shadows debuging. Forward light pass was not found.");
				}
			} else {
				u32 debug_cascaded_shadows = 0;
				if (get_render_pass_index("Debug_Cascade_Shadows", render_world->render_passes_array, &debug_cascaded_shadows)) {
					render_world->render_passes_array[debug_cascaded_shadows] = &render_world->render_passes.forward_light;
				} else {
					print("Render_World_Window::draw: Failed turn off cascaded shadows debuging. Debug cascaded shadows pass was not found.");
				}
			}
		} else {
			print("Render_World_Window::draw: Cascaded shadows debuging doesn't work because the render passes was not initialized.");
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