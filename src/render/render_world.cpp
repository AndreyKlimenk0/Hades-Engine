#include <stdint.h>

#include "render_world.h"
#include "../gui/gui.h"
#include "../sys/engine.h"
#include "../collision/collision.h"

const Color DEFAULT_MESH_COLOR = Color(105, 105, 105);

Render_Entity *find_render_entity(Array<Render_Entity> *render_entities, Entity_Id entity_id, u32 *index)
{
	for (u32 i = 0; i < render_entities->count; i++) {
		if (render_entities->get(i).entity_id == entity_id) {
			if (index) {
				*index = i;
			}
			return &render_entities->get(i);
		}
	}
	return NULL;
}

template<typename T>
void Unified_Mesh_Storate<T>::allocate_gpu_memory()
{
	mesh_struct_buffer.allocate<Mesh_Instance>(1000);
	index_struct_buffer.allocate<u32>(100000);
	vertex_struct_buffer.allocate<Vertex_XNUV>(100000);
}

template<typename T>
bool Unified_Mesh_Storate<T>::add_mesh(const char *mesh_name, Mesh<T> *mesh, Mesh_Idx *_mesh_idx)
{
	assert(mesh_name);

	if ((mesh->vertices.count == 0) || (mesh->indices.count == 0)) {
		print("Render_World::add_mesh: Mesh {} can be added because doesn't have all necessary data.", mesh_name);
		return false;
	}

	String_Id string_id = fast_hash(mesh_name);

	Mesh_Idx mesh_idx;
	if (mesh_table.get(string_id, &mesh_idx)) {
		*_mesh_idx = mesh_idx;
		return true;
	}

	Mesh_Instance mesh_info;
	mesh_info.vertex_count = mesh->vertices.count;
	mesh_info.index_count = mesh->indices.count;
	mesh_info.vertex_offset = unified_vertices.count;
	mesh_info.index_offset = unified_indices.count;

	mesh_idx = mesh_instances.push(mesh_info);
	merge(&unified_vertices, &mesh->vertices);
	merge(&unified_indices, &mesh->indices);

	mesh_struct_buffer.update(&mesh_instances);
	vertex_struct_buffer.update(&unified_vertices);
	index_struct_buffer.update(&unified_indices);

	mesh_table.set(string_id, mesh_idx);
	
	*_mesh_idx = mesh_idx;
	return true;
}

void Render_World::init()
{	
	game_world = Engine::get_game_world();
	render_sys = Engine::get_render_system();

	init_shadow_rendering();

	init_render_passes();
	
	render_sys->gpu_device.create_constant_buffer(sizeof(Frame_Info), &frame_info_cbuffer);

	lights_struct_buffer.allocate<Shader_Light>(100);
	world_matrices_struct_buffer.allocate<Matrix4>(100);
	cascaded_view_projection_matrices_sb.allocate<Matrix4>(100);
	
	Texture_Desc texture_desc;
	texture_desc.width = 200;
	texture_desc.height = 200;
	texture_desc.mip_levels = 1;
	
	render_sys->gpu_device.create_texture_2d(&texture_desc, &default_texture);
	render_sys->gpu_device.create_shader_resource_view(&default_texture);
	fill_texture_with_value((void *)&DEFAULT_MESH_COLOR, &default_texture);

	//game_world->make_direction_light(Vector3(0.5f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
	game_world->make_direction_light(Vector3(1.0f, -1.0f, 0.0f), Vector3(1.0f, 1.0f, 1.0f));

	//Mesh_Loader *mesh_loader = Engine::get_mesh_loader();
	//mesh_loader->load("Scene_Demo1.fbx");

	//Mesh_Loader::Mesh_Instance *mesh_instance = NULL;
	//For(mesh_loader->mesh_instances, mesh_instance) {
	//	Mesh_Idx mesh_idx;
	//	if (add_mesh(mesh_instance->name, &mesh_instance->mesh, &mesh_idx)) {
	//		for (u32 j = 0; j < mesh_instance->positions.count; j++) {
	//			Entity_Id entity_id = game_world->make_entity(mesh_instance->positions[j]);
	//			make_render_entity(entity_id, mesh_idx);
	//		}
	//	}
	//}
}

void Render_World::init_shadow_rendering()
{	
	Texture_Desc depth_stencil_desc;
	depth_stencil_desc.width = SHADOW_ATLAS_WIDTH;
	depth_stencil_desc.height = SHADOW_ATLAS_HEIGHT;
	depth_stencil_desc.format = DXGI_FORMAT_R24G8_TYPELESS;
	depth_stencil_desc.mip_levels = 1;
	depth_stencil_desc.bind |= BIND_DEPTH_STENCIL;
	depth_stencil_desc.multisampling = { 1, 0 };

	render_sys->gpu_device.create_texture_2d(&depth_stencil_desc, &shadow_atlas);
	render_sys->gpu_device.create_depth_stencil_view(&shadow_atlas);
	render_sys->gpu_device.create_shader_resource_view(&shadow_atlas);

	fill_texture_with_value((void *)&DEFAULT_DEPTH_VALUE, &shadow_atlas);

	//shadow_cascade_ranges.push({ 0, 15 });
	//shadow_cascade_ranges.push({ 15, 150 });
	//shadow_cascade_ranges.push({ 150, 500 });
	//shadow_cascade_ranges.push({ 500, 1000 });

	//shadow_cascade_ranges.push({ 0, 15 });
	//shadow_cascade_ranges.push({ 15, 100 });
	//shadow_cascade_ranges.push({ 200, 300 });
	//shadow_cascade_ranges.push({ 300, 400 });

	shadow_cascade_ranges.push({ 0, 15 });
	shadow_cascade_ranges.push({ 10, 50 });
	shadow_cascade_ranges.push({ 40, 120 });
	shadow_cascade_ranges.push({ 100, 320 });

	//shadow_cascade_ranges.push({ 0, 50 });
	//shadow_cascade_ranges.push({ 50, 100 });
	//shadow_cascade_ranges.push({ 100, 150 });
	//shadow_cascade_ranges.push({ 150, 200 });
}

void Render_World::init_render_passes()
{
	if (!render_passes.debug_cascade_shadows.init((void *)this, render_sys)) {
		print("Render_World::init_render_passes: Failed to initialize render pass {}.", render_passes.debug_cascade_shadows.name);
	}

	render_passes_array.push(&render_passes.shadows);
	render_passes_array.push(&render_passes.draw_lines);
	render_passes_array.push(&render_passes.forward_light);

	for (u32 i = 0; i < render_passes_array.count; i++) {
		if (!render_passes_array[i]->init((void *)this, render_sys)) {
			print("Render_World::init_render_passes: Failed to initialize render pass {}.", render_passes_array[i]->name);
		}
	}
}

void Render_World::update()
{
	if (!gui::were_events_handled()) {
		Queue<Event> *events = get_event_queue();
		for (Queue_Node<Event> *node = events->first; node != NULL; node = node->next) {
			Event *event = &node->item;

			camera.handle_event(event);
		}
	}

	frame_info.view_matrix = camera.get_view_matrix();
	frame_info.perspective_matrix = render_sys->view.perspective_matrix;
	frame_info.orthographic_matrix = render_sys->view.orthogonal_matrix;
	frame_info.camera_position = camera.position;
	frame_info.camera_direction = camera.target;
	frame_info.near_plane = render_sys->view.near_plane;
	frame_info.far_plane = render_sys->view.far_plane;

	Game_World *game_world = Engine::get_game_world();
	
	if (light_hash != game_world->light_hash) {
		light_hash = game_world->light_hash;
		update_lights();
	}

	update_shadows();
}

void Render_World::update_lights()
{
	frame_info.light_count = game_world->lights.count;

	Array<Shader_Light> shader_lights;

	Light *light = NULL;
	For(game_world->lights, light) {
		Shader_Light hlsl_light;
		hlsl_light.position = light->position;
		auto temp = light->direction;
		temp.normalize();
		hlsl_light.direction = temp;
		hlsl_light.color = light->color;
		hlsl_light.radius = light->radius;
		hlsl_light.range = light->range;
		hlsl_light.light_type = light->light_type;
		hlsl_light.shadow_map_idx = 0;
		make_shadow(light);
		shader_lights.push(hlsl_light);
	}
	lights_struct_buffer.update(&shader_lights);
}

Render_Entity *Render_World::find_render_entity(Entity_Id entity_id)
{
	Render_Entity *render_entity = NULL;
	For(render_entities, render_entity) {
		if (render_entity->entity_id == entity_id) {
			return render_entity;
		}
	}
	return NULL;
}

void Render_World::make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx)
{
	Render_Entity render_entity;
	render_entity.entity_id = entity_id;
	render_entity.mesh_idx = mesh_idx;

	Entity *entity = game_world->get_entity(entity_id);

	Matrix4 matrix;
	matrix.translate(&entity->position);
	render_entity.world_matrix_idx = world_matrices.push(matrix);

	world_matrices_struct_buffer.update(&world_matrices);
	
	render_entities.push(render_entity);
}

void Frustum_Box::Plane::setup(float plane_width, float plane_height, float z_position)
{
	origin_top_left = Vector3(-plane_width, plane_height, z_position);
	origin_top_right = Vector3(plane_width, plane_height, z_position);
	origin_bottom_left = Vector3(-plane_width, -plane_height, z_position);
	origin_bottom_right = Vector3(plane_width, -plane_height, z_position);
}

void Frustum_Box::Plane::transform_plane(Matrix4 *transform_matrix)
{
	//top_left = *transform_matrix * Vector4(origin_top_left, 1.0f);
	//top_right = *transform_matrix * Vector4(origin_top_right, 1.0f);
	//bottom_left = *transform_matrix * Vector4(origin_bottom_left, 1.0f);
	//bottom_right = *transform_matrix * Vector4(origin_bottom_right, 1.0f);

	top_left = XMVector4Transform(Vector4(origin_top_left, 1.0f), *transform_matrix);
	top_right = XMVector4Transform(Vector4(origin_top_right, 1.0f), *transform_matrix);
	bottom_left = XMVector4Transform(Vector4(origin_bottom_left, 1.0f), *transform_matrix);
	bottom_right = XMVector4Transform(Vector4(origin_bottom_right, 1.0f), *transform_matrix);
}

void Frustum_Box::Plane::get_vertices(Array<Vector3> *vertices)
{
	vertices->clear();
	vertices->push(top_left);
	vertices->push(top_right);
	vertices->push(bottom_left);
	vertices->push(bottom_right);
}

void Frustum_Box::calculate_length()
{
	auto r1 = (first_plane.origin_bottom_right - second_plane.origin_top_left);
	auto r2 = (first_plane.origin_bottom_right - second_plane.origin_top_left).length();
	auto s1 = (second_plane.origin_bottom_right - second_plane.origin_top_left);
	auto s2 = (second_plane.origin_bottom_right - second_plane.origin_top_left).length();
	length = (u32)math::max((first_plane.origin_bottom_right - second_plane.origin_top_left).length(), (second_plane.origin_bottom_right - second_plane.origin_top_left).length());
}

#include <limits.h>

void Frustum_Box::update_min_max_values()
{
	Array<Vector3> vertices;
	Array<Vector3> second_plane_vertices;

	first_plane.get_vertices(&vertices);
	second_plane.get_vertices(&second_plane_vertices);

	max_x = std::numeric_limits<float>::lowest();
	max_y = std::numeric_limits<float>::lowest();
	max_z = std::numeric_limits<float>::lowest();

	min_x = std::numeric_limits<float>::max();
	min_y = std::numeric_limits<float>::max();
	min_z = std::numeric_limits<float>::max();

	merge(&vertices, &second_plane_vertices);

	for (u32 i = 0; i < vertices.count; i++) {
		Vector3 vertex = vertices[i];
		max_x = math::max(max_x, vertex.x);
		max_y = math::max(max_y, vertex.y);
		max_z = math::max(max_z, vertex.z);
		min_x = math::min(min_x, vertex.x);
		min_y = math::min(min_y, vertex.y);
		min_z = math::min(min_z, vertex.z);
	}
}

const u32 CASCADE_SHADOW_MAP_SIZE = 1024;

Vector3 Frustum_Box::get_view_position()
{
	//return Vector3((max_x + min_x) / (length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f, (max_y + min_y) / (length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f, min_z);
	float x_result = (max_x + min_x);
	float y_result = (max_y + min_y);
	float x = (length / (float)CASCADE_SHADOW_MAP_SIZE);
	float xx = (length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f;
	float y = (length / (float)CASCADE_SHADOW_MAP_SIZE);
	float yy = (length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f;
	float xxx = x_result / xx;
	float yyy = y_result / yy;
	auto temp = Vector3((max_x + min_x) / ((length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f), (max_y + min_y) / ((length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f), min_z);
	//return Vector3((max_x + min_x) / ((length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f), (max_y + min_y) / ((length / (float)CASCADE_SHADOW_MAP_SIZE) * 2.0f), min_z);
	return Vector3((max_x + min_x) / (2.0f), (max_y + min_y) / (2.0f), min_z);
}

void Cascaded_Shadow::init(float fov, Shadow_Cascade_Range *shadow_cascade_range)
{
	range = *shadow_cascade_range;
	frustum_box.first_plane.setup((float)range.start * fov, (float)range.start * fov, (float)range.start);
	frustum_box.second_plane.setup((float)range.end * fov, (float)range.end * fov, (float)range.end);
	frustum_box.calculate_length();
}

void Cascaded_Shadow::transform(Matrix4 *transform_matrix)
{
	frustum_box.first_plane.transform_plane(transform_matrix);
	frustum_box.second_plane.transform_plane(transform_matrix);
	frustum_box.update_min_max_values();
}

Matrix4 make_rotation_matrix(Vector3 *direction, Vector3 *up_direction = NULL)
{
	assert(direction);

	Vector3 z = *direction;
	z.normalize();

	Vector3 up = { 0.0f, 1.0f, 0.0f };
	if (up_direction) {
		up = *up_direction;
	}
	Vector3 x_axis = up.cross(z);
	x_axis.normalize();
	Vector3 y_axis = z.cross(x_axis);
	y_axis.normalize();

	Matrix4 rotation_matrix;
	rotation_matrix.indentity();
	rotation_matrix[0] = Vector4(x_axis, 0.0f);
	rotation_matrix[1] = Vector4(y_axis, 0.0f);
	rotation_matrix[2] = Vector4(z, 0.0f); // up is Z axis
	return rotation_matrix;
}

Matrix4 Cascaded_Shadow::get_cascade_view_matrix()
{
	//Matrix4 cascade_space_matrix = light_matrix;
	//cascade_space_matrix[3] = Vector4(frustum_box.get_view_position(), 1.0f);
	//cascade_space_matrix = XMMatrixInverse(NULL, cascade_space_matrix);
	//cascade_space_matrix = cascade_space_matrix.inverse();
	//cascade_space_matrix.translate(&frustum_box.get_view_position());
	//return cascade_space_matrix;
	//return XMMatrixLookAtLH(frustum_box.get_view_position(), light_direction, Vector3(0.0f, 1.0f, 0.0f));
	//Matrix4 m;
	//m.indentity();
	//return Engine::get_render_world()->camera.get_view_matrix();
	
	//Matrix4 cascade_space_matrix;
	//cascade_space_matrix.indentity();
	//cascade_space_matrix.translate(&frustum_box.get_view_position());
	//Matrix4 x = Engine::get_render_world()->camera.get_view_matrix();

	Matrix4 x = make_rotation_matrix(&light_direction);
	x[3] = Vector4(frustum_box.get_view_position(), 1.0f);
	Matrix4 result = x.inverse();
	return result;
}

Matrix4 Cascaded_Shadow::get_cascade_projection_matrix()
{
	Matrix4 projection_matrix;
	projection_matrix.indentity();
	projection_matrix[0][0] = 2.0f / frustum_box.length;
	projection_matrix[1][1] = 2.0f / frustum_box.length;
	projection_matrix[2][2] = 1.0f / (frustum_box.max_z - frustum_box.min_z);
	return projection_matrix;
	return XMMatrixOrthographicLH(1024.0f, 1024.0f, 1.0f, 1000.0f);
	//return Engine::get_render_system()->view.perspective_matrix;
}

inline float get_angle_between_vectors(Vector3 *first_vector, Vector3 *second_vector)
{
	return math::arccos(first_vector->dot(*second_vector) / (first_vector->length() * second_vector->length()));
}

//inline bool is_closed_interval_value(float value, float interval_start = 0.0f, float interval_end = 1.0f)
//{
//	assert(interval_end > interval_start);
//	if ((value >= interval_start) && (value <= interval_end)) {
//		return true;
//	}
//	return false;
//}
//
//inline bool is_normalized(Vector3 *vec)
//{
//	assert(vec);
//	bool x = is_closed_interval_value(vec->x);
//	bool y = is_closed_interval_value(vec->y);
//	bool z = is_closed_interval_value(vec->z);
//	return (x && y && z);
//}

bool Render_World::make_shadow(Light *light)
{
	Cascaded_Shadow_Map cascaded_shadow_map;

	light->direction = { 1.0f, -1.0f, 0.0f };
	
	float fov = render_sys->view.fov;
	auto d = light->direction;
	d.negete();
	Matrix4 light_matrix = make_rotation_matrix(&d);

	for (u32 i = 0; i < shadow_cascade_ranges.count; i++) {
		Cascaded_Shadow cascaded_shadow;
		cascaded_shadow.view_projection_matrix_index = cascaded_view_projection_matrices.push(Matrix4());
		cascaded_shadow.light_direction = light->direction;
		cascaded_shadow.light_matrix = light_matrix;
		cascaded_shadow.init(fov, &shadow_cascade_ranges[i]);
		if (!get_shadow_atls_viewport(&cascaded_shadow.viewport)) {
			return false;
		}
		cascaded_shadow_map.cascaded_shadows.push(cascaded_shadow);
		cascaded_shadow_count++;
	}
	cascaded_shadow_maps.push(cascaded_shadow_map);
	return true;
}

void Render_World::update_shadows()
{
	for (u32 i = 0; i < cascaded_shadow_maps.count; i++) {
		for (u32 j = 0; j < cascaded_shadow_maps[i].cascaded_shadows.count; j++) {
			Cascaded_Shadow *cascaded_shadow = &cascaded_shadow_maps[i].cascaded_shadows[j];
			Matrix4 view_matrix = camera.get_view_matrix();
			Matrix4 light_matrix = cascaded_shadow_maps[i].cascaded_shadows[j].light_matrix;
			//Matrix4 light_view_matrix = light_matrix * XMMatrixInverse(NULL, camera.get_view_matrix());
			Matrix4 light_view_matrix = light_matrix * XMMatrixInverse(NULL, camera.get_view_matrix());
			//Matrix4 light_view_matrix = XMMatrixInverse(NULL, camera.get_view_matrix());
			cascaded_shadow_maps[i].cascaded_shadows[j].transform(&light_view_matrix);
			Vector3 v = cascaded_shadow_maps[i].cascaded_shadows[j].frustum_box.get_view_position();
			Matrix4 m = XMMatrixInverse(NULL, camera.get_view_matrix());
			int i = 0;

			cascaded_view_projection_matrices[cascaded_shadow->view_projection_matrix_index] = cascaded_shadow->get_cascade_view_matrix() * cascaded_shadow->get_cascade_projection_matrix();
		}
	}

	cascaded_view_projection_matrices_sb.update(&cascaded_view_projection_matrices);
}

bool Render_World::add_mesh(const char *mesh_name, Mesh<Vertex_XNUV> *mesh, Mesh_Idx *mesh_idx)
{
	Bounding_Sphere temp = make_bounding_sphere(Vector3(0.0f, 0.0f, 0.0f), mesh);
	if (temp.radious > world_bounding_sphere.radious) {
		world_bounding_sphere = temp;
	}
	return triangle_meshes.add_mesh(mesh_name, mesh, mesh_idx);
}

bool Render_World::add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Idx *mesh_idx)
{
	return line_meshes.add_mesh(mesh_name, mesh, mesh_idx);
}

bool Render_World::get_shadow_atls_viewport(Viewport *viewport)
{
	static u32 x = 0;
	static u32 y = 0;
	
	Point_u32 shadow_map_position;
	shadow_map_position.x = CASCADE_WIDTH + x;
	shadow_map_position.y = CASCADE_HEIGHT + y;

	if ((shadow_map_position.x <= SHADOW_ATLAS_WIDTH) && (shadow_map_position.y <= SHADOW_ATLAS_HEIGHT)) {
		viewport->x = x;
		viewport->y = y;
		viewport->width = CASCADE_WIDTH;
		viewport->height = CASCADE_HEIGHT;
		x += CASCADE_WIDTH;
	} else if ((shadow_map_position.x > SHADOW_ATLAS_WIDTH) && (shadow_map_position.y < SHADOW_ATLAS_HEIGHT)) {
		viewport->x = 0;
		viewport->y = y;
		viewport->width = CASCADE_WIDTH;
		viewport->height = CASCADE_HEIGHT;
		x = 0;
		y += CASCADE_HEIGHT;
	} else {
		print("Render_World::get_shadow_atls_view_port: The shadow atlas is out of space.");
		return false;
	}
	return true;
}

void Render_World::render()
{
	render_sys->render_pipeline.update_constant_buffer(&frame_info_cbuffer, (void *)&frame_info);

	render_sys->render_pipeline.set_vertex_shader_resource(1, frame_info_cbuffer);
	render_sys->render_pipeline.set_pixel_shader_resource(1, frame_info_cbuffer);
	
	Render_Pass *render_pass = NULL;
	For(render_passes_array, render_pass) {
		render_pass->render(&render_sys->render_pipeline);
	}
}

void Render_World::update_world_matrices()
{
	world_matrices.count = 0;
	
	for (u32 index = 0; index < render_entities.count; index++) {
		Entity *entity = game_world->get_entity(render_entities[index].entity_id);
		
		Matrix4 matrix;
		matrix.translate(&entity->position);
		world_matrices.push(matrix);
	}
}

template<typename T>
void Gpu_Struct_Buffer::allocate(u32 elements_count)
{	
	Gpu_Buffer_Desc desc;
	desc.usage = RESOURCE_USAGE_DYNAMIC;
	desc.data = NULL;
	desc.data_size = sizeof(T);
	desc.struct_size = sizeof(T);
	desc.data_count = elements_count;
	desc.bind_flags = BIND_SHADER_RESOURCE;
	desc.cpu_access = CPU_ACCESS_WRITE;
	desc.misc_flags = RESOURCE_MISC_BUFFER_STRUCTURED;

	Gpu_Device *gpu_device = &Engine::get_render_system()->gpu_device;
	gpu_device->create_gpu_buffer(&desc, &gpu_buffer);
	gpu_device->create_shader_resource_view(&gpu_buffer);
}

template<typename T>
void Gpu_Struct_Buffer::update(Array<T> *array)
{
	if (array->count == 0) {
		return;
	}

	Render_Pipeline *render_pipeline = &Engine::get_render_system()->render_pipeline;
	
	if (array->count > gpu_buffer.data_count) {
		free();
		allocate<T>(array->count);
	}

	T *buffer = (T *)render_pipeline->map(gpu_buffer);
	memcpy((void *)buffer, (void *)array->items, sizeof(T) * array->count);
	render_pipeline->unmap(gpu_buffer);
}

void Gpu_Struct_Buffer::free()
{
	if (!gpu_buffer.is_empty()) {
		gpu_buffer.free();
	}
}
