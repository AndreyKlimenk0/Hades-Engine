#include <assert.h>
#include <stdlib.h>

#include "editor.h"
#include "../sys/sys.h"
#include "../sys/utils.h"
#include "../sys/engine.h"
#include "../sys/commands.h"

#include "../libs/str.h"
#include "../libs/utils.h"
#include "../libs/geometry.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/math/vector.h"
#include "../libs/math/3dmath.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"

#include "../render/helpers.h"
#include "../render/render_world.h"
#include "../render/render_passes.h"
#include "../render/render_system.h"
#include "../render/render_api/render.h"

#include "../collision/collision.h"

static const u32 STR_ENTITY_TYPES_COUNT = 5;
static const String str_entity_types[STR_ENTITY_TYPES_COUNT] = {
	"Unknown",
	"Entity",
	"Light",
	"Geometry",
	"Camera"
};

inline Rect_s32 get_display_screen_rect()
{
	Size_s32 window_size = static_cast<Size_s32>(Engine::get_render_system()->get_window_size());
	return Rect_s32(window_size);
}

inline void place_rect_on_top_right(Rect_s32 *src, Rect_s32 *dest)
{
	assert(src);
	assert(dest);
	assert(src->width >= dest->width);

	dest->x = src->width - dest->width;
	dest->y = 0;
}

static String to_string(Entity_Id entity_id)
{
	char *entity_index = to_string(entity_id.index);
	defer(free_string(entity_index));
	return str_entity_types[(u32)entity_id.type] + "#" + entity_index;
}

inline void place_in_middle(Rect_s32 *in_element_place, Rect_s32 *placed_element)
{
	placed_element->x = ((in_element_place->width / 2) - (placed_element->width / 2)) + in_element_place->x;
	placed_element->y = ((in_element_place->height / 2) - (placed_element->height / 2)) + in_element_place->y;
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

inline void calculate_picking_ray(Vector3 &camera_position, Matrix4 &view_matrix, Matrix4 &perspective_matrix, Ray *ray)
{
	Size_u32 window_size = Engine::get_render_system()->get_window_size();
	Vector2 xy_ndc_point = from_raster_to_ndc_coordinates(Mouse_State::x, Mouse_State::y, window_size.width, window_size.height);
	Vector4 ndc_point = Vector4(xy_ndc_point.x, xy_ndc_point.y, 1.0f, 1.0f);

	Vector4 mouse_point_in_world = ndc_point * inverse(view_matrix * perspective_matrix);
	mouse_point_in_world /= mouse_point_in_world.w;

	*ray = Ray(camera_position, to_vector3(mouse_point_in_world) - camera_position);
}

inline float find_triangle_area(const Vector3 &a, const Vector3 &b, const Vector3 &c)
{
	Vector3 area = cross(b - a, c - a); //  The magnitude of the cross-product can be interpreted as the area of the parallelogram
	return length(area) * 0.5f;
}

struct Ray_Trinagle_Intersection_Result {
	Vector3 a;
	Vector3 b;
	Vector3 c;
	Vector3 intersection_point;
};

static bool detect_intersection(Matrix4 &entity_world_matrix, Ray *picking_ray, Vertex_PNTUV *vertices, u32 vertex_count, u32 *indices, u32 index_count, Ray_Trinagle_Intersection_Result *intersection_result = NULL)
{
	assert(picking_ray);
	assert(vertices);
	assert(indices);
	assert(index_count % 3 == 0);

	for (u32 index = 0, i = 0; i < (index_count / 3); index += 3, i++) {
		Vector3 a = vertices[indices[index + 0]].position * entity_world_matrix;
		Vector3 b = vertices[indices[index + 1]].position * entity_world_matrix;
		Vector3 c = vertices[indices[index + 2]].position * entity_world_matrix;
		Vector3 plane_normal = normalize(vertices[indices[index]].normal * entity_world_matrix.to_matrix3());

		float result = dot(picking_ray->direction, plane_normal);
		if (result < 0.0f) {
			Vector3 plane_origin = (a + b + c) / 3.0f;
			float ray_length = dot(plane_origin - picking_ray->origin, plane_normal) / result;
			if (ray_length > 0.0f) {
				Vector3 ray_plane_intersection_point = picking_ray->origin + Vector3(picking_ray->direction * ray_length);

				float triangle_area = find_triangle_area(a, b, c);
				float u = find_triangle_area(c, a, ray_plane_intersection_point) / triangle_area;
				float v = find_triangle_area(a, b, ray_plane_intersection_point) / triangle_area;
				float w = find_triangle_area(b, c, ray_plane_intersection_point) / triangle_area;

				if (in_range(0.0f, 1.1f, u + v + w)) {
					if (result) {
						intersection_result->a = a;
						intersection_result->b = b;
						intersection_result->c = c;
						intersection_result->intersection_point = ray_plane_intersection_point;
					}
					return true;
				}
			}
		}
	}
	return false;
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

		if (entity->bounding_box_type == BOUNDING_BOX_TYPE_UNKNOWN) {
			continue;
		}

		if (entity->bounding_box_type == BOUNDING_BOX_TYPE_AABB) {
			Result intersection_result;
			if (::detect_intersection(picking_ray, &entity->AABB_box, &intersection_result.intersection_point)) {
				if (entity->type == ENTITY_TYPE_GEOMETRY) {
					Geometry_Entity *geometry_entity = static_cast<Geometry_Entity *>(entity);
					if (geometry_entity->geometry_type == GEOMETRY_TYPE_BOX) {
						intersection_result.entity_id = entity_id;
						intersection_result.render_entity_idx = i;
						intersected_entities.push(intersection_result);
					}
				} else {
					Mesh_Idx mesh_id = render_world->game_render_entities[i].mesh_idx;
					Render_Model *render_model = render_world->model_storage.render_models[mesh_id];

					Vertex_PNTUV *vertices = render_model->mesh.vertices.items;
					u32 *indices = render_model->mesh.indices.items;

					Matrix4 entity_world_matrix = get_world_matrix(entity);

					Ray_Trinagle_Intersection_Result ray_mesh_intersection_result;
					if (::detect_intersection(entity_world_matrix, picking_ray, vertices, render_model->mesh.vertex_count(), indices, render_model->mesh.index_count(), &ray_mesh_intersection_result)) {
						intersection_result.entity_id = entity_id;
						intersection_result.render_entity_idx = i;
						intersection_result.intersection_point = ray_mesh_intersection_result.intersection_point;
						intersected_entities.push(intersection_result);
					}
				}
			}
		}
	}

	if (!intersected_entities.is_empty()) {
		if (intersected_entities.count > 1) {
			u32 most_near_entity_to_camera = 0;
			float most_small_distance = FLT_MAX;
			for (u32 i = 0; i < intersected_entities.count; i++) {
				// A ray origin equals to a camera position.
				float intersection_point_camera_distance = find_distance(picking_ray->origin, intersected_entities[i].intersection_point);
				if (intersection_point_camera_distance < most_small_distance) {
					most_near_entity_to_camera = i;
					most_small_distance = intersection_point_camera_distance;
				}
			}
			*result = intersected_entities[most_near_entity_to_camera];
		} else {
			*result = intersected_entities.first();
		}
		return true;
	}
	return false;
}

Editor_Window::Editor_Window()
{
}

Editor_Window::~Editor_Window()
{
}

void Editor_Window::init(Engine *engine)
{
	editor = &engine->editor;
	game_world = &engine->game_world;
	render_world = &engine->render_world;
	render_system = &engine->render_sys;
}

void Editor_Window::init(const char *_name, Engine *engine)
{
	assert(engine);

	name = _name;
	Editor_Window::init(engine);
}

void Editor_Window::open()
{
	window_open = true;
	//gui::open_window(name);
}

void Editor_Window::close()
{
	window_open = false;
	//gui::close_window(name);
}

void Editor_Window::set_absolute_position(s32 x, s32 y)
{
	window_rect.set(x, y);
}

void Editor_Window::set_size(s32 width, s32 height)
{
	window_rect.set_size(width, height);
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

	if (game_world->cameras.is_empty()) {
		editor_camera_id = game_world->make_camera(Vector3(0.0f, 20.0f, -250.0f), Vector3(0.0f, 0.0f, -1.0f));
		engine->render_world.set_rendering_view(editor_camera_id);
	} else {
		editor_camera_id = get_entity_id(&game_world->cameras.first());
		engine->render_world.set_rendering_view(editor_camera_id);
	}

	key_command_bindings.init();
	key_command_bindings.set("move_camera_forward", KEY_W);
	key_command_bindings.set("move_camera_back", KEY_S);
	key_command_bindings.set("move_camera_right", KEY_D);
	key_command_bindings.set("move_camera_left", KEY_A);
	key_command_bindings.set("start_rotate_camera", KEY_LMOUSE);
	key_command_bindings.set("end_rotate_camera", KEY_LMOUSE, false);
	key_command_bindings.set("", KEY_RMOUSE); // Don't want to get annoyiny messages

	key_bindings.bind(KEY_CTRL, KEY_C); // Command window keys binding
}

void Editor::handle_events()
{
	key_bindings.handle_events();
	//if (!gui::were_events_handled() && (editor_mode == EDITOR_MODE_COMMON)) {
	//	//@Note: In the future here better to use linear allocator.
	//	Array<Editor_Command> editor_commands;
	//	Array<Entity_Command *> entity_commands;

	//	convert_user_input_events_to_edtior_commands(&editor_commands);
	//	convert_editor_commands_to_entity_commands(&editor_commands, &entity_commands);

	//	Camera *camera = game_world->get_camera(editor_camera_id);
	//	camera->handle_commands(&entity_commands);

	//	free_memory(&entity_commands);
	//}
}

void Editor::update()
{
	if (key_bindings.was_binding_triggered(KEY_CTRL, KEY_C)) {
	}
	picking();
}

struct Moving_Entity {
	bool moving_entity = false;
	float distance; // A distance between the editor camera and a moving entity
	Vector3 ray_entity_intersection_point;
};

void Editor::picking()
{
	Camera *camera = game_world->get_camera(render_world->rendering_view.camera_id);
	Ray picking_ray;
	calculate_picking_ray(camera->position, render_world->rendering_view.view_matrix, render_sys->window_view_plane.perspective_matrix, &picking_ray);

	static Moving_Entity moving_entity_info;

	if (was_key_just_pressed(KEY_LMOUSE) && (editor_mode == EDITOR_MODE_MOVE_ENTITY)) {
		Ray_Entity_Intersection::Result intersection_result;
		if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result) && (picked_entity == intersection_result.entity_id)) {
			moving_entity_info.moving_entity = true;
			moving_entity_info.distance = find_distance(camera->position, intersection_result.intersection_point);
			moving_entity_info.ray_entity_intersection_point = intersection_result.intersection_point;
		} else {
			editor_mode = EDITOR_MODE_COMMON;
			set_cursor(CURSOR_TYPE_ARROW);
		}
	}

	if (moving_entity_info.moving_entity) {
		Vector3 moved_picking_ray_point = picking_ray.origin + (Vector3)(picking_ray.direction * moving_entity_info.distance);
		Vector3 difference = moved_picking_ray_point - moving_entity_info.ray_entity_intersection_point;

		Entity *entity = game_world->get_entity(picked_entity);
		game_world->move_entity(entity, difference);

		moving_entity_info.ray_entity_intersection_point += difference;
	}

	if (was_key_just_released(KEY_LMOUSE) && moving_entity_info.moving_entity) {
		moving_entity_info.moving_entity = false;
	}

	//if (!gui::were_events_handled()) {
	//	if (was_click(KEY_RMOUSE) && valid_entity_id(picked_entity)) {
	//		Ray_Entity_Intersection::Result intersection_result;
	//		if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result)) {
	//			if (picked_entity == intersection_result.entity_id) {
	//				gui::open_menu("Actions on entity");
	//				mouse_position = Point_s32(Mouse_State::x, Mouse_State::y);
	//			}
	//		}
	//	} else if (was_click(KEY_LMOUSE)) {
	//		Silhouette_Pass *silhouette_pass = &render_sys->passes.silhouette_pass;
	//		silhouette_pass->reset_render_entity_indices();

	//		Ray_Entity_Intersection::Result intersection_result;
	//		if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result)) {
	//			//gui::make_tab_active(game_world_tab_gui_id);
	//			picked_entity = intersection_result.entity_id;
	//			silhouette_pass->add_render_entity_index(intersection_result.render_entity_idx);
	//		} else {
	//			picked_entity.reset();
	//		}
	//	}
	//}
}

void topbar_check_button(const char *text, u32 *flags, u32 value)
{
	begin_ui_element("Top Bar Button %s", text);
	set_alignment(ALIGNMENT_CENTER);
	set_size(fixed_size(100), fixed_size(18));

	if (ui_element_clicked()) {
		*flags = value;
	}

	if (*flags & value) {
		set_background_color(Color(99));
	} else {
		if (ui_element_hovered()) {
			set_background_color(Color(50));
		} else {
			set_background_color(Color(44));
		}
	}
	
	set_rounding(4);
	ui_text_element(text);

	end_ui_element();
}

void Editor::render()
{
	begin_frame();

	set_space(0);

	begin_ui_element("Top Bar");
	set_layout(ROW_LAYOUT);
	set_alignment(ALIGNMENT_LEFT | ALIGNMENT_BOTTOM);
	set_size(grow_size(), fixed_size(28));
	//set_relative_position(0, 0);
	set_background_color(Color(33));
	set_padding(Padding(10, 0, 0, 4));

	static bool draw_world_panel = false;
	static bool draw_world_panel1 = false;
	static bool draw_world_panel2 = false;

	static u32 flags = 0;
	
	topbar_check_button("World", &flags, 0x2);
	topbar_check_button("Rendering", &flags, 0x4);
	topbar_check_button("View", &flags, 0x8);
	
	end_ui_element();

	begin_ui_element("Main View");
	set_size(grow_size(), grow_size());
	set_background_color(Color::Red);

	set_layout(ROW_LAYOUT);
	set_alignment(ALIGNMENT_RIGHT | ALIGNMENT_VERTICAL_CENTER);
	
	begin_ui_element("Right Side Window");
	set_size(fixed_size(400), grow_size());
	set_background_color(Color::Black);
	end_ui_element();
	
	end_ui_element();

	end_frame();
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
			}

		} else if (event->type == EVENT_TYPE_MOUSE) {
			Editor_Command rotate_camera_command;
			editor_command.command = "rotate_camera";
			editor_command.additional_info = (void *)&event->mouse_info;
			editor_commands->push(editor_command);
		}
	}
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