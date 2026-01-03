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

#include "../render/render_world.h"
#include "../render/render_passes.h"
#include "../render/render_system.h"

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
	Vector2 xy_ndc_point = from_raster_to_ndc_coordinates(Mouse_State::x, Mouse_State::y, 0, 0);
	//Vector2 xy_ndc_point = from_raster_to_ndc_coordinates(Mouse_State::x, Mouse_State::y, Render_System::screen_width, Render_System::screen_height);
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
		//Render_Entity_Idx render_entity_idx;
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
						//intersection_result.render_entity_idx = i;
						intersected_entities.push(intersection_result);
					}
				} else {
					//Mesh_Id mesh_id = render_world->game_render_entities[i].mesh_id;
					//Model_Storage::Mesh_Instance mesh_instance = render_world->model_storage.mesh_instances[mesh_id.instance_idx];

					//Vertex_PNTUV *vertices = &render_world->model_storage.unified_vertices[mesh_instance.vertex_offset];
					//u32 *indices = &render_world->model_storage.unified_indices[mesh_instance.index_offset];

					//Matrix4 entity_world_matrix = get_world_matrix(entity);

					//Ray_Trinagle_Intersection_Result ray_mesh_intersection_result;
					//if (::detect_intersection(entity_world_matrix, picking_ray, vertices, mesh_instance.vertex_count, indices, mesh_instance.index_count, &ray_mesh_intersection_result)) {
					//	intersection_result.entity_id = entity_id;
					//	intersection_result.render_entity_idx = i;
					//	intersection_result.intersection_point = ray_mesh_intersection_result.intersection_point;
					//	intersected_entities.push(intersection_result);
					//}
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
	gui::open_window(name);
}

void Editor_Window::close()
{
	window_open = false;
	gui::close_window(name);
}

void Editor_Window::set_position(s32 x, s32 y)
{
	window_rect.set(x, y);
}

void Editor_Window::set_size(s32 width, s32 height)
{
	window_rect.set_size(width, height);
}

void Top_Right_Window::init(const char *_name, Engine *engine)
{
	Editor_Window::init(_name, engine);

	window_theme.rounded_border = 10;
	window_theme.header_height = 25;
	window_theme.background_color = Color(24);
	window_theme.outlines_width = 2.0f;
}

void Entity_Window::init(Engine *engine)
{
	Top_Right_Window::init("Entity window", engine);
	set_size(400, 600);
	Rect_s32 screen_rect = get_display_screen_rect();
	place_rect_on_top_right(&screen_rect, &window_rect);
}

void Entity_Window::display_sun_earth(u32 earth_radius, u32 sun_radius, u32 orbit_radius, const Point_s32 &position, Light *light, Render_Primitive_List *render_list)
{
	Vector2 mouse_position = Vector2((float)Mouse_State::x, (float)Mouse_State::y);

	//render_list->add_circle(position.x, position.y, earth_radius, Color(121, 121, 121));
	//render_list->add_outline_circle(position.x, position.y, orbit_radius, 2.0f, Color(51, 77, 128));

	Vector2 sun_position = normalize(Vector2(light->direction.x, light->direction.z));
	sun_position *= (float)orbit_radius;
	sun_position.y *= -1.0f;
	sun_position += position.to_vector2();

	//render_list->add_circle((s32)sun_position.x, (s32)sun_position.y, sun_radius, Color(121, 121, 121));

	static bool update_light_direction = false;
	if (was_key_just_pressed(KEY_LMOUSE) && detect_intersection((float)sun_radius, sun_position, mouse_position)) {
		update_light_direction = true;
	}
	if (update_light_direction && was_key_just_released(KEY_LMOUSE)) {
		update_light_direction = false;
	}
	if (update_light_direction) {
		Vector2 new_light_direction = normalize(mouse_position - position.to_vector2());
		new_light_direction.y *= -1.0f;

		//game_world->update_light_direction(light, Vector3(vec2.x, light->direction.y, vec2.y));
		light->direction.x = new_light_direction.x;
		light->direction.z = new_light_direction.y;
		render_world->upload_lights();
	}
}

void Entity_Window::display_light(Light *light)
{
	if (light->light_type == DIRECTIONAL_LIGHT_TYPE) {
		if (gui::edit_field("Direction", &light->direction)) {
			game_world->update_light_direction(light, light->direction);
			render_world->upload_lights();
		}
		gui::edit_field("Color", &light->color);

		Render_Primitive_List *render_list = gui::get_render_primitive_list();
		u32 middle = window_rect.x + window_rect.width / 2;
		display_sun_earth(70, 20, 100, Point_s32(middle, 220), light, render_list);

		//render_list->add_circle(window_rect.x + 100, 100, 70, Color(121, 121, 121));
		//render_list->add_outline_circle(window_rect.x + 100, 100, 100, 2.0f, Color(51, 77, 128));
	}
}

void Entity_Window::draw()
{
	static Entity_Id prev_entity_id;
	static Vector3 scaling;
	static Vector3 rotation;
	static Vector3 position;
	static Vector3 direction;

	Entity *entity = game_world->get_entity(editor->picked_entity);
	String str_entity_id;
	if (entity) {
		str_entity_id = to_string(get_entity_id(entity));
		if (editor->picked_entity != prev_entity_id) {
			scaling = entity->scaling;
			rotation = entity->rotation;
			position = entity->position;
			if (entity->type == ENTITY_TYPE_LIGHT) {
				Light *light = static_cast<Light *>(entity);
				direction = light->direction;
			}
		}
	}
	window_theme.header_text = str_entity_id.c_str();
	gui::set_theme(&window_theme);
	gui::set_next_window_pos(window_rect.x, window_rect.y);
	gui::set_next_window_size(window_rect.width, window_rect.height);
	if (gui::begin_window(name, WINDOW_HEADER)) {
		if (entity) {
			if (entity->type == ENTITY_TYPE_LIGHT) {
				Light *light = static_cast<Light *>(entity);
				display_light(light);
			} else {
				if (gui::edit_field("Scaling", &scaling)) {
					entity->scaling = scaling;
				}
				gui::edit_field("Rotation", &rotation);
				if (gui::edit_field("Position", &position)) {
					game_world->place_entity(entity, position);
				}
				if ((entity->type != ENTITY_TYPE_CAMERA) || (entity->type == ENTITY_TYPE_LIGHT)) {
					static bool temp;
					gui::radio_button("Draw bounding box", &temp);
				}
			}
		}
		if (entity && (entity->bounding_box_type != BOUNDING_BOX_TYPE_UNKNOWN) && (entity->type != DIRECTIONAL_LIGHT_TYPE) && (entity->type != ENTITY_TYPE_UNKNOWN)) {
			//if (!draw_AABB_states.key_in_table(entity->idx)) {
			//	draw_AABB_states[entity->idx] = false;
			//}
			//static Entity *last_picked_entity = NULL;
			//bool was_click = gui::radio_button("Draw AABB", &draw_AABB_states[entity->idx]);
			//if (was_click && draw_AABB_states[entity->idx]) {
			//	if (entity) {
			//		last_picked_entity = entity;
			//		Line_Mesh AABB_mesh;
			//		make_AABB_mesh(&entity->AABB_box.min, &entity->AABB_box.max, &AABB_mesh);
			//		render_system->render_3d.set_mesh(&AABB_mesh);
			//	}
			//} else if (was_click && !draw_AABB_states[entity->idx]) {
			//	render_system->render_3d.reset_mesh();
			//	last_picked_entity = NULL;
			//}
			//if (draw_AABB_states[entity->idx] && last_picked_entity) {
			//	render_system->render_3d.draw_lines(last_picked_entity->position, Color::Red);
			//}
		}
		gui::end_child();
	}
	gui::reset_window_theme();
}

void Entity_Tree_Window::init(Engine *engine)
{
	Top_Right_Window::init("Entity list", engine);
	set_size(400, 600);
	Rect_s32 screen_rect = get_display_screen_rect();
	place_rect_on_top_right(&screen_rect, &window_rect);

	window_theme.horizontal_padding = 0;
	window_theme.vertical_padding = 0;

	tree_theme.background_color = Color(24);
	tree_theme.tree_node_color = Color(24);
	tree_theme.window_size.width = window_rect.width - window_theme.horizontal_padding * 2;
	tree_theme.window_size.height = window_rect.height - 35;
}

template <typename T>
void Entity_Tree_Window::draw_entity_list(Array<T> &entity_list, const char *name)
{
	u32 r = (value & 0xff000000) >> 24;
	u32 g = (value & 0x00ff0000) >> 16;
	u32 b = (value & 0x0000ff00) >> 8;
	return Vector3((float)r, (float)g, (float)b);
	//return Vector3(float(value & 0xff000000), float(value & 0x00ff0000), float(value & 0x0000ff00));
}

void Render_World_Window::draw()
{
	//static u32 render_type_index = 0;
	//static u32 prev_render_type_index = 0;
	//
	//gui::list_box(&rendering_types, &render_type_index);
	//if (render_type_index != prev_render_type_index) {
	//	if (rendering_types[render_type_index] == "Normal") {
	//		render_world->frame_render_passes.clear();
	//		render_world->frame_render_passes.push(&render_world->render_passes.shadows);
	//		render_world->frame_render_passes.push(&render_world->render_passes.forward_light);
	//	
	//	} else if (rendering_types[render_type_index] == "Voxel") {
	//		render_world->frame_render_passes.clear();
	//		render_world->frame_render_passes.push(&render_world->render_passes.voxelization);
	//	}
	//	prev_render_type_index = render_type_index;
	//}

	//if (gui::radio_button("Debug cascaded shadows", &debug_cascaded_shadows)) {
	//	if (render_world->render_passes.forward_light.is_valid && render_world->render_passes.debug_cascade_shadows.is_valid) {
	//		if (debug_cascaded_shadows) {
	//			u32 forward_light_index = 0;
	//			if (get_render_pass_index("Forward_Light", render_world->frame_render_passes, &forward_light_index)) {
	//				render_world->frame_render_passes[forward_light_index] = &render_world->render_passes.debug_cascade_shadows;
	//			} else {
	//				print("Render_World_Window::draw: Failed turn on cascaded shadows debuging. Forward light pass was not found.");
	//			}
	//		} else {
	//			u32 debug_cascaded_shadows = 0;
	//			if (get_render_pass_index("Debug_Cascade_Shadows", render_world->frame_render_passes, &debug_cascaded_shadows)) {
	//				render_world->frame_render_passes[debug_cascaded_shadows] = &render_world->render_passes.forward_light;
	//			} else {
	//				print("Render_World_Window::draw: Failed turn off cascaded shadows debuging. Debug cascaded shadows pass was not found.");
	//			}
	//		}
	//	} else {
	//		print("Render_World_Window::draw: Cascaded shadows debuging doesn't work because the render passes is not valid.");
	//	}
	//}

	//if (gui::radio_button("Dispaly voxel world", &display_voxel_world)) {
	//	if (display_voxel_world) {
	//		render_world->frame_render_passes.remove(0);
	//		render_world->frame_render_passes.remove(0);
	//	} else {
	//		render_world->frame_render_passes.clear();
	//		render_world->frame_render_passes.push(&render_world->render_passes.shadows);
	//		render_world->frame_render_passes.push(&render_world->render_passes.forward_light);
	//		render_world->frame_render_passes.push(&render_world->render_passes.voxelization);
	//	}
	//}
	//if (display_voxel_world) {
	//	Array<Voxel> voxels;
	//	voxels.reserve(render_world->voxel_grid.ceil_count());
	//	memset((void *)voxels.items, 0, voxels.get_size());
	//	render_world->voxels_sb.read(&voxels);

	//	if (!voxels.is_empty()) {
	//		auto matrix = make_look_to_matrix(render_world->voxel_grid_center, Vector3::base_z);
	//		Size_f32 s = render_world->voxel_grid.ceil_size;
	//		Box box = { s.width, s.height, s.depth };
	//		Triangle_Mesh tri_mesh;
	//		make_box_mesh(&box, &tri_mesh);

	//		Vertex_Mesh mesh;
	//		mesh.vertices.reserve(tri_mesh.vertices.count);
	//		mesh.indices.reserve(tri_mesh.indices.count);

	//		for (u32 i = 0; i < tri_mesh.vertices.count; i++) {
	//			mesh.vertices[i] = tri_mesh.vertices[i].position;
	//		}
	//		mesh.indices = tri_mesh.indices;

	//		render_system->render_3d.set_mesh(&mesh);

	//		Size_s32 voxel_grid_size = (Size_s32)render_world->voxel_grid.grid_size / 2;

	//		for (u32 i = 0; i < voxels.count; i++) {
	//			if (voxels[i].occlusion == 0) {
	//				continue;
	//			}
	//			Point_s32 index = (Point_s32)convert_1d_to_3d_index(i, render_world->voxel_grid.grid_size.height, render_world->voxel_grid.grid_size.depth);
	//			index = index - Point_s32(voxel_grid_size);

	//			Point_s32 ceil_size = Point_s32((Size_s32)render_world->voxel_grid.ceil_size);

	//			Vector3 offset = ((index * ceil_size) + (ceil_size / 2)).to_vector3();
	//			Vector3 voxel_center = (offset)*inverse(&matrix);

	//			u32 packed_color = voxels[i].packed_color;
	//			Vector3 values = unpack_RGB8(voxels[i].packed_color);
	//			values /= 255.0f;
	//			Color color = Color(values.x, values.y, values.z);
	//			render_system->render_3d.draw_triangles(voxel_center, color);
	//		}
	//		render_system->render_3d.reset_mesh();
	//	}
	//	render_world->voxels_sb.reset<Voxel>();
	//}

	//gui::radio_button("Dispaly Voxel Grid", &display_voxel_grid);
	//gui::radio_button("Display voxel grid bounds", &display_voxel_grid_bounds);

	//if (display_voxel_grid_bounds) {
	//	Vector3 max = render_world->voxel_grid.total_size().to_vector3() * 0.5f;
	//	Vector3 min = -max;

	//	Size_f32 size = render_world->voxel_grid.total_size();
	//	Box box = { size.width, size.height, size.depth };
	//	Triangle_Mesh tri_mesh;
	//	make_box_mesh(&box, &tri_mesh);

	//	Line_Mesh mesh;
	//	//mesh.vertices.reserve(tri_mesh.vertices.count);
	//	//mesh.indices.reserve(tri_mesh.indices.count);
	//	//for (int i = 0; i < tri_mesh.vertices.count; i++) {
	//	//	mesh.vertices[i] = tri_mesh.vertices[i].position;
	//	//}
	//	////mesh.indices = tri_mesh.indices;
	//	//for (int i = 0, j = tri_mesh.indices.count - 1; i < tri_mesh.indices.count; i++, j--) {
	//	//	mesh.indices[i] = tri_mesh.indices[j];
	//	//}
	//	make_AABB_mesh(&min, &max, &mesh);
	//	
	//	render_system->render_3d.set_mesh(&mesh);
	//	render_system->render_3d.draw_lines(render_world->voxel_grid_center, Color(Color::Green.get_rgb(), 0.3f));
	//	render_system->render_3d.reset_mesh();

	//	Line_Mesh camera_AABB;
	//	Vector3 temp_max = { 5.0f, 5.0f, 5.0f };
	//	Vector3 temp_min = -temp_max;
	//	make_AABB_mesh(&temp_min, &temp_max, &camera_AABB);
	//	
	//	auto view_pos = render_world->voxel_grid_center;
	//	view_pos.z -= max.z;

	//	render_system->render_3d.set_mesh(&camera_AABB);
	//	render_system->render_3d.draw_lines(view_pos, Color::Green);
	//	render_system->render_3d.reset_mesh();
	//}

	//if (display_voxel_grid) {
	//	Array<Voxel> voxels;
	//	voxels.reserve(render_world->voxel_grid.ceil_count());
	//	memset((void *)voxels.items, 0, voxels.get_size());
	//	render_world->voxels_sb.read(&voxels);

	//	if (!voxels.is_empty()) {
	//		auto matrix = make_look_to_matrix(render_world->voxel_grid_center, Vector3::base_z);
	//		Vector3 max = render_world->voxel_grid.ceil_size.to_vector3() * 0.5f;
	//		Vector3 min = -max;

	//		Line_Mesh mesh;
	//		make_AABB_mesh(&min, &max, &mesh);

	//		render_system->render_3d.set_mesh(&mesh);
	//		
	//		Size_s32 voxel_grid_size = (Size_s32)render_world->voxel_grid.grid_size / 2;

	//		for (u32 i = 0; i < voxels.count; i++) {
	//			if (voxels[i].occlusion == 0) {
	//				continue;
	//			}
	//			Point_s32 index = (Point_s32)convert_1d_to_3d_index(i, render_world->voxel_grid.grid_size.height, render_world->voxel_grid.grid_size.depth);
	//			index = index - Point_s32(voxel_grid_size);

	//			Point_s32 ceil_size = Point_s32((Size_s32)render_world->voxel_grid.ceil_size);

	//			Vector3 offset = ((index * ceil_size) + (ceil_size / 2)).to_vector3();
	//			Vector3 voxel_center = (offset) * inverse(&matrix);

	//			render_system->render_3d.draw_lines(voxel_center, Color::Red);
	//		}
	//		render_system->render_3d.reset_mesh();
	//	}
	//	render_world->voxels_sb.reset<Voxel>();
	//}

	//Array<String> strings;
	//for (u32 i = 0; i < game_world->cameras.count; i++) {
	//	strings.push("Camera");
	//}
	//static u32 index = 0;
	//static u32 prev_index = 1;

	//gui::list_box(&strings, &index);
	//bool was_update = false;
	//if (index != prev_index) {
	//	Entity_Id camera_id = Entity_Id(ENTITY_TYPE_CAMERA, index);
	//	editor->editor_camera_id = camera_id;
	//	render_world->set_camera_for_rendering(camera_id);
	//	prev_index = index;
	//	was_update = true;
	//}

	//static u32 index2 = 0;
	//static u32 prev_index2 = 1;

	//gui::list_box(&strings, &index2);

	//if (index2 != prev_index2) {
	//	Entity_Id camera_id = Entity_Id(ENTITY_TYPE_CAMERA, index2);
	//	render_world->set_camera_for_debuging(camera_id);
	//	prev_index2 = index2;
	//} else if (was_update) {
	//	Entity_Id camera_id = Entity_Id(ENTITY_TYPE_CAMERA, index);
	//	render_world->set_camera_for_debuging(camera_id);
	//	prev_index2 = index;
	//}
	if (gui::begin_tree_node(name)) {
		for (u32 i = 0; i < entity_list.count; i++) {
			T *entity = &entity_list[i];
			Entity_Id entity_id = get_entity_id(entity);
			String str_entity_id = to_string(entity_id);
			if (gui::begin_tree_node(str_entity_id, GUI_TREE_NODE_FINAL)) {
				if (gui::element_clicked(KEY_LMOUSE) || gui::element_double_clicked(KEY_LMOUSE)) {
					if ((entity_id.type != ENTITY_TYPE_LIGHT) && (entity_id.type != ENTITY_TYPE_CAMERA)) {
						//Outlining_Pass *outlining_pass = &render_world->render_passes.outlining;
						//outlining_pass->reset_render_entity_indices();
						u32 index = 0;
						Render_Entity *render_entity = find_render_entity(&render_world->game_render_entities, entity_id, &index);
						if (render_entity) {
							editor->picked_entity = entity_id;
							//outlining_pass->add_render_entity_index(index);
						}
					} else {
						editor->picked_entity = entity_id;
					}
				}
				if (gui::element_double_clicked(KEY_LMOUSE)) {
					editor->open_or_close_right_window(&editor->entity_window);
				}
				gui::end_tree_node();
			}
		}
		gui::end_tree_node();
	}
}

void Entity_Tree_Window::draw()
{
	gui::set_theme(&window_theme);
	gui::set_next_window_pos(window_rect.x, window_rect.y);
	gui::set_next_window_size(window_rect.width, window_rect.height);
	if (gui::begin_window(name, WINDOW_HEADER)) {
		gui::set_theme(&tree_theme);
		if (gui::begin_tree("Entities tree")) {
			draw_entity_list(game_world->lights, "Lights");
			draw_entity_list(game_world->cameras, "Cameras");
			draw_entity_list(game_world->entities, "Entities");
			gui::end_tree();
		}
		gui::reset_tree_theme();

			//render_world->render_passes.outlining.delete_render_entity_index(render_entity_index);

			editor->picked_entity.reset();
			editor->draw_drop_down_entity_window = false;
		}
		gui::reset_button_theme();
		gui::end_window();
	}
	gui::reset_window_theme();
}

static s32 draw_two_columns_list(const char *list_name, Array<Gui_List_Line_State> &list_line_states, Array<Pair<String, String>> &list)
{
	s32 line_index = -1;
	Gui_List_Column columns[] = { {"First column", 75 }, { "Second column", 25 } };
	if (gui::begin_list(list_name, columns, 2)) {
		for (u32 i = 0; i < list.count; i++) {
			if (gui::begin_line(&list_line_states[i])) {
				if (gui::left_mouse_click(list_line_states[i]) || gui::enter_key_click(list_line_states[i])) {
					line_index = i;
				}
				gui::begin_column("First column");
				gui::add_text(list[i].first, RECT_LEFT_ALIGNMENT);
				gui::end_column();

				gui::begin_column("Second column");
				gui::add_text(list[i].second, RECT_LEFT_ALIGNMENT);
				gui::end_column();

				gui::end_line();
			}
		}
		gui::end_list();
	}
	return line_index;
}

inline s32 list_line_selected(Array<Gui_List_Line_State> &list_line_states)
{
	for (s32 i = 0; i < (s32)list_line_states.count; i++) {
		if (gui::selected(list_line_states[i])) {
			return i;
		}
	}
	return -1;
}

inline void select_line(u32 line_index, Array<Gui_List_Line_State> &list_line_states)
{
	if (list_line_states.count > line_index) {
		memset((void *)list_line_states.items, 0, sizeof(Gui_List_Line_State) * list_line_states.count);
		list_line_states[line_index] = 0x1;
	}
}

static bool display_and_get_info_for_load_mesh_command(String *edit_field, Array<String> &command_args, void *context)
{
	assert(edit_field);
	assert(context);

	String full_path_to_data_directory;
	build_full_path_to_data_directory("models", full_path_to_data_directory);

	Array<String> not_matched_files;
	get_file_names_from_dir(full_path_to_data_directory, &not_matched_files);

	Array<String> matched_files;
	if (!edit_field->is_empty()) {
		for (u32 i = 0; i < not_matched_files.count; i++) {
			if (not_matched_files[i].find(edit_field->c_str(), 0, false) != -1) {
				matched_files.push(not_matched_files[i]);
			}
		}
	} else {
		matched_files = not_matched_files;
	}
	Command_Window *command_window = (Command_Window *)context;

	if (command_window->list_line_states.is_empty()) {
		command_window->list_line_states.reserve(matched_files.count);
		select_line(0, command_window->list_line_states);
	} else if ((command_window->list_line_states.count > matched_files.count) && !matched_files.is_empty()) {
		s32 line_index = list_line_selected(command_window->list_line_states);
		if ((line_index > -1) && (line_index > ((s32)matched_files.count - 1))) {
			select_line((matched_files.count - 1), command_window->list_line_states);
		}
	}

	Array<Pair<String, String>> mesh_path_list;
	for (u32 i = 0; i < matched_files.count; i++) {
		mesh_path_list.push({ matched_files[i], "data/models" });
	}

	bool result = false;
	gui::set_theme(&command_window->list_theme);
	gui::make_next_list_active();
	s32 line_index = draw_two_columns_list("meshes list", command_window->list_line_states, mesh_path_list);
	if (line_index >= 0) {
		command_args.push(mesh_path_list[line_index].first);
		result = true;
	}
	gui::reset_list_theme();
	return result;
}

static bool display_and_get_info_for_load_level_command(String *edit_field, Array<String> &command_args, void *context)
{
	assert(edit_field);
	assert(context);

	String full_path_to_data_directory;
	build_full_path_to_data_directory("levels", full_path_to_data_directory);

	Array<String> not_matched_files;
	get_file_names_from_dir(full_path_to_data_directory, &not_matched_files);

	Array<String> matched_files;
	if (!edit_field->is_empty()) {
		for (u32 i = 0; i < not_matched_files.count; i++) {
			if (not_matched_files[i].find(edit_field->c_str(), 0, false) != -1) {
				matched_files.push(not_matched_files[i]);
			}
		}
	} else {
		matched_files = not_matched_files;
	}
	Command_Window *command_window = (Command_Window *)context;

	if (command_window->list_line_states.is_empty()) {
		command_window->list_line_states.reserve(matched_files.count);
		select_line(0, command_window->list_line_states);
	} else if ((command_window->list_line_states.count > matched_files.count) && !matched_files.is_empty()) {
		s32 line_index = list_line_selected(command_window->list_line_states);
		if ((line_index > -1) && (line_index > ((s32)matched_files.count - 1))) {
			select_line((matched_files.count - 1), command_window->list_line_states);
		}
	}

	Array<Pair<String, String>> mesh_path_list;
	for (u32 i = 0; i < matched_files.count; i++) {
		mesh_path_list.push({ matched_files[i], "data/levels" });
	}

	bool result = false;
	gui::set_theme(&command_window->list_theme);
	gui::make_next_list_active();
	s32 line_index = draw_two_columns_list("level list", command_window->list_line_states, mesh_path_list);
	if (line_index >= 0) {
		command_args.push(mesh_path_list[line_index].first);
		result = true;
	}
	gui::reset_list_theme();
	return result;
}

static bool display_all_commands(String *edit_field, Array<String> &command_args, void *context)
{
	Command_Window *command_window = (Command_Window *)context;

	Array<Pair<String, String>> list;
	if (!edit_field->is_empty()) {
		for (u32 i = 1; i < command_window->displaying_commands.count; i++) {
			if (command_window->displaying_commands[i].command_name.find(edit_field->c_str(), 0, false) != -1) {
				list.push({ command_window->displaying_commands[i].command_name, command_window->displaying_commands[i].str_key_binding });
			}
		}
	} else {
		for (u32 i = 1; i < command_window->displaying_commands.count; i++) {
			list.push({ command_window->displaying_commands[i].command_name, command_window->displaying_commands[i].str_key_binding });
		}
	}
	gui::set_theme(&command_window->list_theme);
	gui::make_next_list_active();
	s32 line_index = draw_two_columns_list("command list", command_window->list_line_states, list);
	if (line_index >= 0) {
		String &command_name = list[line_index].first;
		for (u32 i = 0; i < command_window->displaying_commands.count; i++) {
			if (command_name == command_window->displaying_commands[i].command_name) {
				command_window->current_displaying_command = &command_window->displaying_commands[i];
				command_window->active_edit_field = true;
				command_window->list_line_states.clear();
				edit_field->free();
				break;
			}
		}
	}
	gui::reset_list_theme();
	return false;
}

static const char *MAIN_COMMAND_NAME = "Display all commands";

Command_Window::Command_Window()
{
}

Command_Window::~Command_Window()
{
}

void Command_Window::init(Engine *engine)
{
	Editor_Window::init("Command window", engine);

	displaying_command(MAIN_COMMAND_NAME, display_all_commands);
	current_displaying_command = &displaying_commands.last();

	displaying_command("Load mesh", KEY_CTRL, KEY_L, display_and_get_info_for_load_mesh_command);
	displaying_command("Load level", display_and_get_info_for_load_level_command);
	displaying_command("Create level", NULL);

	Rect_s32 display;
	//display.set_size(Render_System::screen_width, Render_System::screen_height);

	command_window_rect.set_size(600, 80);
	command_window_rect_with_additional_info.set_size(600, 500);

	place_in_middle(&display, &command_window_rect);
	command_window_rect.y = 200;

	command_window_theme.background_color = Color(20);
	command_window_theme.vertical_padding = 14;
	command_window_theme.horizontal_padding = 10;

	command_edit_field_theme.rect.set_size(command_window_rect.width - command_window_theme.horizontal_padding * 2, 30);
	command_edit_field_theme.draw_label = false;
	command_edit_field_theme.color = Color(30);
	command_edit_field_theme.rounded_border = 0;

	command_window_rect.height = command_edit_field_theme.rect.height + command_window_theme.vertical_padding * 2;

	list_theme.line_height = 30;
	list_theme.column_filter = false;
	list_theme.line_text_offset = command_window_theme.horizontal_padding + command_edit_field_theme.text_shift;
	list_theme.window_size.width = command_window_rect_with_additional_info.width;
	list_theme.window_size.height = command_window_rect_with_additional_info.height - command_edit_field_theme.rect.height - command_window_theme.vertical_padding;
	list_theme.background_color = Color(20);
	list_theme.line_color = Color(20);

	list_line_states.reserve(displaying_commands.count);
	select_line(0, list_line_states);
}

void Command_Window::open()
{
	Editor_Window::open();
	window_just_open = true;
}

void Command_Window::close()
{
	Editor_Window::close();
	window_just_open = false;
	command_edit_field.free();
}

void Command_Window::displaying_command(const char *command_name, bool(*display_info_and_get_command_args)(String *edit_field, Array<String> &command_args, void *context))
{
	assert(command_name);

	Displaying_Command command_displaying_info;
	command_displaying_info.command_name = command_name;
	command_displaying_info.str_key_binding = "";
	command_displaying_info.display_info_and_get_command_args = display_info_and_get_command_args;

	displaying_commands.push(command_displaying_info);
}

void Command_Window::displaying_command(const char *command_name, Key modified_key, Key second_key, bool(*display_info_and_get_command_args)(String *edit_field, Array<String> &command_args, void *context))
{
	assert(command_name);

	Key_Binding key_binding = { modified_key, second_key };
	char *str_key_binding = to_string(&key_binding);

	Displaying_Command command_displaying_info;
	command_displaying_info.command_name = command_name;
	command_displaying_info.str_key_binding = str_key_binding;
	command_displaying_info.display_info_and_get_command_args = display_info_and_get_command_args;

	displaying_commands.push(command_displaying_info);
	command_key_bindings.push({ &displaying_commands.last(), key_binding });

	free_string(str_key_binding);
}

#define IF_THEN(exp, code) if (exp) { code; };

void Command_Window::draw()
{
	assert(current_displaying_command);

	IF_THEN(!window_open, return);

	if (was_click(KEY_ESC)) {
		if (current_displaying_command->command_name == MAIN_COMMAND_NAME) {
			close();
		} else {
			current_displaying_command = &displaying_commands.first();
			list_line_states.reserve(displaying_commands.count);
			select_line(0, list_line_states);
		}
		command_edit_field.free();
	}
	bool display_additional_info = current_displaying_command->display_info_and_get_command_args != NULL;
	Rect_s32 window_rect = display_additional_info ? command_window_rect_with_additional_info : command_window_rect;

	gui::set_next_window_pos(command_window_rect.x, command_window_rect.y);
	gui::set_next_window_size(window_rect.width, window_rect.height);
	gui::set_theme(&command_window_theme);

	IF_THEN(window_just_open, gui::make_next_ui_element_active());
	if (gui::begin_window(name, 0)) {

		gui::set_theme(&command_edit_field_theme);
		IF_THEN(window_just_open || active_edit_field, (gui::make_next_ui_element_active(), active_edit_field = false));
		gui::edit_field("Command field", &command_edit_field);
		gui::reset_edit_field_theme();

		Array<String> command_args;
		if (display_additional_info) {
			gui::set_window_padding(0);
			if (current_displaying_command->display_info_and_get_command_args(&command_edit_field, command_args, this)) {
				run_command(current_displaying_command->command_name, command_args);
				command_edit_field.free();
			}
			gui::set_window_padding(command_window_theme.horizontal_padding);
		} else {
			if (was_click(KEY_ENTER)) {
				command_args.push(command_edit_field);
				run_command(current_displaying_command->command_name, command_args);
			}
		}
		gui::end_window();
	}
	IF_THEN(window_just_open, window_just_open = false);
	gui::reset_window_theme();
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

	windows.push(&entities_window);
	windows.push(&entity_window);
	windows.push(&command_window);

	top_right_windows.push(&entity_window);
	top_right_windows.push(&entities_window);

	for (u32 i = 0; i < windows.count; i++) {
		windows[i]->init(engine);
	}

	for (u32 i = 0; i < top_right_windows.count; i++) {
		top_right_windows[i]->close();
	}

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

	init_left_bar();
}

void Editor::init_left_bar()
{
	left_bar.window_theme.rects_padding = 1;
	left_bar.window_theme.horizontal_padding = 0;
	left_bar.window_theme.vertical_padding = 0;
	left_bar.window_theme.background_color = Color(0, 0, 0, 0);

	left_bar.button_theme.hover_color = Color(48);
	left_bar.button_theme.color = Color(40);
	left_bar.button_theme.button_size = { 42, 42 };
	left_bar.button_theme.rect_rounding = 0;

	//left_bar.images.adding.init_from_file("icons8-add-30.png", "editor");
	//left_bar.images.entity.init_from_file("entity2.png", "editor");
	//left_bar.images.entities.init_from_file("entities.png", "editor");
	//left_bar.images.rendering.init_from_file("rendering.png", "editor");
}

void Editor::handle_events()
{
	key_bindings.handle_events();
	if (!gui::were_events_handled() && (editor_mode == EDITOR_MODE_COMMON)) {
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
	if (key_bindings.was_binding_triggered(KEY_CTRL, KEY_C)) {
		if (command_window.window_open) {
			command_window.close();
		} else {
			command_window.open();
		}
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
	//calculate_picking_ray(camera->position, render_world->render_camera.debug_view_matrix, render_sys->view.perspective_matrix, &picking_ray);

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

	if (!gui::were_events_handled()) {
		if (was_click(KEY_RMOUSE) && valid_entity_id(picked_entity)) {
			Ray_Entity_Intersection::Result intersection_result;
			if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result)) {
				if (picked_entity == intersection_result.entity_id) {
					gui::open_menu("Actions on entity");
					mouse_position = Point_s32(Mouse_State::x, Mouse_State::y);
				}
			}
		} else if (was_click(KEY_LMOUSE)) {
			//Outlining_Pass *outlining_pass = &render_world->render_passes.outlining;
			//outlining_pass->reset_render_entity_indices();

			Ray_Entity_Intersection::Result intersection_result;
			if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result)) {
				//gui::make_tab_active(game_world_tab_gui_id);
				picked_entity = intersection_result.entity_id;
				//outlining_pass->add_render_entity_index(intersection_result.render_entity_idx);
			} else {
				picked_entity.reset();
			}
		}
	}
}

void Editor::render_menus()
{
	gui::set_next_window_pos(53, 20);
	if (gui::begin_menu("Adding entity")) {
		if (gui::menu_item("Direction light")) {
			//game_world->make_direction_light(Vector3(1.0, -0.5, 1.0), Color::White.get_rgb());
			Entity_Id entity_id = game_world->make_direction_light(Vector3(0.2f, -1.0f, 0.2f), Color::White.get_rgb());
			render_world->upload_lights();
		}
		if (gui::menu_item("Point light")) {
		}
		gui::segment();
		if (gui::menu_item("Box")) {
		}
		if (gui::menu_item("Sphere")) {
		}
		if (gui::menu_item("Plane")) {
		}
		gui::end_menu();
	}

	gui::set_next_window_pos(mouse_position.x, mouse_position.y);
	if (gui::begin_menu("Actions on entity")) {
		if (gui::menu_item("Scale")) {
		}
		if (gui::menu_item("Rotate")) {
		}
		if (gui::menu_item("Translate")) {
			set_cursor(CURSOR_TYPE_MOVE);
			editor_mode = EDITOR_MODE_MOVE_ENTITY;
		}
		gui::segment();
		if (gui::menu_item("Copy")) {

		}
		if (gui::menu_item("Delete")) {
			game_world->delete_entity(picked_entity);
			u32 render_entity_index = render_world->delete_render_entity(picked_entity);
			//render_world->render_passes.outlining.delete_render_entity_index(render_entity_index);
		}
		gui::end_menu();
	}
}

void Editor::render_left_bar()
{
	gui::set_theme(&left_bar.window_theme);
	gui::set_next_window_pos(10, 20);
	gui::set_next_window_size(50, 270);
	if (gui::begin_window("Top bar", NO_WINDOW_STYLE)) {
		gui::set_theme(&left_bar.button_theme);

		if (gui::image_button(&left_bar.images.adding)) {
			gui::open_menu("Adding entity");
		}
		if (gui::image_button(&left_bar.images.entity)) {
			open_or_close_right_window(&entity_window);
		}
		if (gui::image_button(&left_bar.images.entities)) {
			open_or_close_right_window(&entities_window);
		}
		if (gui::image_button(&left_bar.images.rendering)) {
		}

		gui::reset_image_button_theme();
	}
	gui::reset_window_theme();
}

void Editor::render()
{
	gui::begin_frame();
	render_menus();
	render_left_bar();
	for (u32 i = 0; i < windows.count; i++) {
		windows[i]->draw();
	}
	gui::end_frame();
}

void Editor::open_or_close_right_window(Editor_Window *window)
{
	Editor_Window *top_right_window = NULL;
	For(top_right_windows, top_right_window)
	{
		if (top_right_window != window) {
			top_right_window->close();
		}
	}
	if (!window->window_open) {
		window->open();
	} else {
		window->close();
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