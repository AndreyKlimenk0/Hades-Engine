#include <assert.h>

#include "editor.h"
#include "../sys/sys.h"
#include "../sys/engine.h"
#include "../sys/commands.h"

#include "../libs/str.h"
#include "../libs/utils.h"
#include "../libs/geometry.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/os/input.h"
#include "../libs/os/event.h"
#include "../libs/image/png.h"
#include "../libs/math/vector.h"
#include "../libs/math/3dmath.h"
#include "../libs/math/functions.h"
#include "../libs/math/structures.h"

#include "../render/render_world.h"
#include "../render/render_passes.h"
#include "../render/render_system.h"
#include "../render/render_helpers.h"

#include "../collision/collision.h"

static const u32 STR_ENTITY_TYPES_COUNT = 5;
static const char *str_entity_types[STR_ENTITY_TYPES_COUNT] = {
	"Unknown",
	"Entity",
	"Light",
	"Geometry",
	"Camera"
};

static_assert(STR_ENTITY_TYPES_COUNT == ENTITY_TYPES_COUNT, "assert_failed");

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
		//if (render_passes[i]->name == name) {
		//	*index = i;
		//	return true;
		//}
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
					Mesh_Id mesh_id = render_world->game_render_entities[i].mesh_id;
					Mesh_Storate::Mesh_Instance mesh_instance = render_world->triangle_meshes.mesh_instances[mesh_id.instance_idx];

					Vertex_PNTUV *vertices = &render_world->triangle_meshes.unified_vertices[mesh_instance.vertex_offset];
					u32 *indices = &render_world->triangle_meshes.unified_indices[mesh_instance.index_offset];

					Matrix4 entity_world_matrix = get_world_matrix(entity);

					Ray_Trinagle_Intersection_Result ray_mesh_intersection_result;
					if (::detect_intersection(entity_world_matrix, picking_ray, vertices, mesh_instance.vertex_count, indices, mesh_instance.index_count, &ray_mesh_intersection_result)) {
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
				float intersection_point_camera_distance = find_distance(&picking_ray->origin, &intersected_entities[i].intersection_point);
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

void Editor_Window::init(Engine *engine)
{
	assert(engine);

	editor = &engine->editor;
	game_world = &engine->game_world;
	render_world = &engine->render_world;
	render_system = &engine->render_sys;
}

void Editor_Window::open()
{
	window_open = true;
}

void Editor_Window::close()
{
	window_open = false;
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

				Mesh_Id mesh_id;
				render_world->add_triangle_mesh(&mesh, &mesh_id);
				render_world->add_render_entity(entity_id, mesh_id);
			}
		} else if (geometry_type == GEOMETRY_TYPE_SPHERE) {
			gui::edit_field("Radious", &sphere.radius);
			gui::edit_field("Slice count", (s32 *)&sphere.slice_count);
			gui::edit_field("Stack count", (s32 *)&sphere.stack_count);

			if (gui::button("Make")) {
				//Entity_Id entity_id = game_world->make_geometry_entity(position, geometry_type, (void *)&sphere);

				//Triangle_Mesh mesh;
				//make_sphere_mesh(&sphere, &mesh);

				//char *mesh_name = format("Sphere", sphere.radius, sphere.slice_count, sphere.stack_count);
				//Mesh_Idx mesh_idx;
				//render_world->add_mesh(mesh_name, &mesh, &mesh_idx);

				//Render_Entity_Textures render_entity_textures;
				//render_entity_textures.ambient_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				//render_entity_textures.normal_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				//render_entity_textures.diffuse_texture_idx = render_world->render_entity_texture_storage.default_texture_idx;
				//render_entity_textures.specular_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				//render_entity_textures.displacement_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;

				//render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_idx, &render_entity_textures);

				//free_string(mesh_name);
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

	world_entities_window_theme.background_color = Color(40, 40, 40);
	world_entities_window_theme.header_color = Color(36, 36, 36);
	world_entities_window_theme.place_between_rects = 0;
	world_entities_window_theme.horizontal_offset_from_sides = 0;
	world_entities_window_theme.vertical_offset_from_sides = 0;

	entity_info_window_theme.background_color = Color(40, 40, 40);
	entity_info_window_theme.header_color = Color(36, 36, 36);
	entity_info_window_theme.place_between_rects = 8;

	buttons_theme.color = world_entities_window_theme.background_color;
	buttons_theme.aligment = LEFT_ALIGNMENT;
}

void Game_World_Window::draw()
{
	Size_s32 window_size = gui::get_window_size();
	gui::set_next_window_size(window_size.width - window_width_delta, world_entities_height);
	gui::set_theme(&world_entities_window_theme);
	if (gui::begin_child("World entities", (WINDOW_DEFAULT_STYLE & ~WINDOW_OUTLINES))) {
		buttons_theme.rect.width = window_size.width - window_width_delta;
		gui::set_theme(&buttons_theme);

		draw_entity_list("Entity", game_world->entities.count, ENTITY_TYPE_ENTITY);
		draw_entity_list("Camera", game_world->cameras.count, ENTITY_TYPE_CAMERA);
		draw_entity_list("Light", game_world->lights.count, ENTITY_TYPE_LIGHT);
		draw_entity_list("Geometry", game_world->geometry_entities.count, ENTITY_TYPE_GEOMETRY);

		gui::reset_button_theme();
		gui::end_child();
	}
	gui::reset_window_theme();

	gui::set_theme(&entity_info_window_theme);
	gui::set_next_window_size(window_size.width - window_width_delta, entity_info_height);
	if (gui::begin_child("Entity info", (WINDOW_DEFAULT_STYLE & ~WINDOW_OUTLINES))) {
		Entity *entity = game_world->get_entity(editor->picked_entity);
		if (entity) {
			if (entity->type == ENTITY_TYPE_ENTITY) {
				Vector3 entity_position = entity->position;
				gui::edit_field("Scaling", &entity->scaling);
				gui::edit_field("Rotation", &entity->rotation);
				gui::edit_field("Position", &entity->position);

			} else if (entity->type == ENTITY_TYPE_GEOMETRY) {
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
				//if (was_click && draw_frustum_states[camera->idx] && !find_render_entity(&render_world->line_render_entities, editor->picked_entity)) {
				//	char *name = format(Render_System::screen_width, Render_System::screen_height, 1000, render_system->view.fov);
				//	String_Id string_id = fast_hash(name);

				//	Mesh_Idx mesh_idx;
				//	if (!render_world->line_meshes.mesh_table.get(string_id, &mesh_idx)) {
				//		Line_Mesh frustum_mesh;
				//		make_frustum_mesh(render_system->view.fov, render_system->view.ratio, 1.0f, 1000.0f, &frustum_mesh);
				//		render_world->add_mesh(name, &frustum_mesh, &mesh_idx);
				//	}
				//	free_string(name);

				//	Render_Entity_Textures render_entity_textures;
				//	render_entity_textures.ambient_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				//	render_entity_textures.normal_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				//	render_entity_textures.diffuse_texture_idx = render_world->render_entity_texture_storage.default_texture_idx;
				//	render_entity_textures.specular_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;
				//	render_entity_textures.displacement_texture_idx = render_world->render_entity_texture_storage.white_texture_idx;

				//	render_world->add_render_entity(RENDERING_TYPE_LINES_RENDERING, editor->picked_entity, mesh_idx, &render_entity_textures, (void *)&Color::Red);

				//} else if (was_click && !draw_frustum_states[camera->idx]) {
				//	// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
				//	u32 render_entity_index = 0;
				//	if (find_render_entity(&render_world->line_render_entities, editor->picked_entity, &render_entity_index)) {
				//		render_world->line_render_entities.remove(render_entity_index);
				//	}
				//}
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
	
	rendering_types.push("Normal");
	rendering_types.push("Voxel");
}

void Render_World_Window::update()
{
}

inline Vector3 unpack_RGB8(u32 value)
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
	gui::set_theme(&window_theme);

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
	Editor_Window::init(engine);

	displaying_command(MAIN_COMMAND_NAME, display_all_commands);
	current_displaying_command = &displaying_commands.last();

	displaying_command("Load mesh", KEY_CTRL, KEY_L, display_and_get_info_for_load_mesh_command);
	displaying_command("Load level", display_and_get_info_for_load_level_command);
	displaying_command("Create level", NULL);

	Rect_s32 display;
	Size_u32 window_size = Engine::get_render_system()->get_window_size();
	display.set_size(window_size.width, window_size.height);

	command_window_rect.set_size(600, 80);
	command_window_rect_with_additional_info.set_size(600, 500);

	place_in_middle(&display, &command_window_rect);
	command_window_rect.y = 200;

	command_window_theme.background_color = Color(20);
	command_window_theme.vertical_offset_from_sides = 14;
	command_window_theme.horizontal_offset_from_sides = 10;

	command_edit_field_theme.rect.set_size(command_window_rect.width - command_window_theme.horizontal_offset_from_sides * 2, 30);
	command_edit_field_theme.draw_label = false;
	command_edit_field_theme.color = Color(30);
	command_edit_field_theme.rounded_border = 0;

	command_window_rect.height = command_edit_field_theme.rect.height + command_window_theme.vertical_offset_from_sides * 2;

	list_theme.line_height = 30;
	list_theme.column_filter = false;
	list_theme.line_text_offset = command_window_theme.horizontal_offset_from_sides + command_edit_field_theme.text_shift;
	list_theme.window_size.width = command_window_rect_with_additional_info.width;
	list_theme.window_size.height = command_window_rect_with_additional_info.height - command_edit_field_theme.rect.height - command_window_theme.vertical_offset_from_sides;
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
	if (gui::begin_window("Command window", 0)) {

		gui::set_theme(&command_edit_field_theme);
		IF_THEN(window_just_open || active_edit_field, (gui::make_next_ui_element_active(), active_edit_field = false));
		gui::edit_field("Command field", &command_edit_field);
		gui::reset_edit_field_theme();

		Array<String> command_args;
		if (display_additional_info) {
			Gui_Window_Theme window_theme;
			window_theme.horizontal_offset_from_sides = 0;
			gui::set_theme(&window_theme);

			if (current_displaying_command->display_info_and_get_command_args(&command_edit_field, command_args, this)) {
				run_command(current_displaying_command->command_name, command_args);
				command_edit_field.free();
			}
			gui::reset_window_theme();
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

	command_window.init(engine);
	make_entity_window.init(engine);
	game_world_window.init(engine);
	render_world_window.init(engine);
	drop_down_entity_window.init(engine);

	if (game_world->cameras.is_empty()) {
		editor_camera_id = game_world->make_camera(Vector3(0.0f, 20.0f, -250.0f), Vector3(0.0f, 0.0f, -1.0f));
		engine->render_world.set_camera_for_rendering(editor_camera_id);
	} else {
		editor_camera_id = get_entity_id(&game_world->cameras.first());
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

	key_bindings.bind(KEY_CTRL, KEY_C); // Command window keys binding
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
	if (!gui::were_events_handled() && were_key_events()) {
		if (draw_drop_down_entity_window) {
			draw_drop_down_entity_window = false;
		}
	}
	render_world_window.update();
	picking();
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
	//calculate_picking_ray(camera->position, render_world->render_camera.debug_view_matrix, render_sys->view.perspective_matrix, &picking_ray);

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
					draw_drop_down_entity_window = true;
					drop_down_entity_window.mouse_position = Point_s32(Mouse_State::x, Mouse_State::y);
				}
			}
		} else if (was_click(KEY_LMOUSE)) {
			//Outlining_Pass *outlining_pass = &render_world->render_passes.outlining;
			//outlining_pass->reset_render_entity_indices();

			Ray_Entity_Intersection::Result intersection_result;
			if (Ray_Entity_Intersection::detect_intersection(&picking_ray, game_world, render_world, &intersection_result)) {
				gui::make_tab_active(game_world_tab_gui_id);
				picked_entity = intersection_result.entity_id;
				//outlining_pass->add_render_entity_index(intersection_result.render_entity_idx);
			} else {
				picked_entity.reset();
			}
		}
	}
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
		game_world_tab_gui_id = gui::get_last_tab_gui_id();

		if (gui::add_tab("Render World")) {
			render_world_window.draw();
		}

		gui::end_window();
	}
	if (command_window.window_open) {
		command_window.draw();
	}
	if (draw_drop_down_entity_window) {
		drop_down_entity_window.draw();
	}
	gui::end_frame();
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
