#include <stdint.h>

#include "render_world.h"
#include "render_pass.h"
#include "../collision/collision.h"

#include "../gui/gui.h"
#include "../sys/engine.h"


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

	light_projections.init();

	init_render_passes();
	
	render_sys->gpu_device.create_constant_buffer(sizeof(Frame_Info), &frame_info_cbuffer);

	light_struct_buffer.allocate<Shader_Light>(100);
	world_matrix_struct_buffer.allocate<Matrix4>(1000);
	
	Texture_Desc texture_desc;
	texture_desc.width = 200;
	texture_desc.height = 200;
	texture_desc.mip_levels = 1;
	
	render_sys->gpu_device.create_texture_2d(&texture_desc, &default_texture);
	fill_texture_with_value((void *)&DEFAULT_MESH_COLOR, &default_texture);
}

void Render_World::init_shadow_rendering()
{
	Texture_Desc texture_desc;
	texture_desc.width = DIRECTION_SHADOW_MAP_WIDTH;
	//texture_desc.width = SHADOW_ATLAS_WIDTH;
	//texture_desc.height = SHADOW_ATLAS_HEIGHT;
	texture_desc.height = DIRECTION_SHADOW_MAP_WIDTH;
	texture_desc.format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
	texture_desc.mip_levels = 1;
	
	render_sys->gpu_device.create_texture_2d(&texture_desc, &shadow_atlas);

	fill_texture_with_value((void *)&DEFAULT_DEPTH_VALUE, &shadow_atlas);
	
	Texture_Desc depth_stencil_desc;
	depth_stencil_desc.width = DIRECTION_SHADOW_MAP_WIDTH;
	depth_stencil_desc.height = DIRECTION_SHADOW_MAP_HEIGHT;
	depth_stencil_desc.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	depth_stencil_desc.mip_levels = 1;
	depth_stencil_desc.bind = BIND_DEPTH_STENCIL;

	render_sys->gpu_device.create_depth_stencil_buffer(&depth_stencil_desc, &temp_shadow_storage);

	fill_texture_with_value((void *)&DEFAULT_DEPTH_VALUE, &temp_shadow_storage.texture);
}

void Render_World::init_render_passes()
{
	Forwar_Light_Pass *forward_light_pass = new Forwar_Light_Pass((void *)this);
	Draw_Lines_Pass *draw_lines_pass = new Draw_Lines_Pass((void *)this);
	Shadow_Pass *shadow_pass = new Shadow_Pass((void *)this);

	render_passes.push(forward_light_pass);
	render_passes.push(draw_lines_pass);
	render_passes.push(shadow_pass);

	Render_System *render_sys = Engine::get_render_system();
	for (u32 i = 0; i < render_passes.count; i++) {
		
		render_passes[i]->init(render_sys);
		if (!render_passes[i]->setup_pipeline_state(render_sys)) {
			DELETE_PTR(render_passes[i]);
			render_passes.remove(i);
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
	frame_info.perspective_matrix = render_sys->view_info.perspective_matrix;
	//frame_info.orthographic_matrix = render_sys->view_info.orthogonal_matrix;
	frame_info.orthographic_matrix = render_sys->view_info.perspective_matrix;
	frame_info.camera_position = camera.position;
	frame_info.camera_direction = camera.target;
	frame_info.near_plane = render_sys->view_info.near_plane;
	frame_info.far_plane = render_sys->view_info.far_plane;

	Game_World *game_world = Engine::get_game_world();
	
	if (light_hash != game_world->light_hash) {
		light_hash = game_world->light_hash;
		update_lights();
	}
}

void Render_World::update_lights()
{
	frame_info.light_count = game_world->lights.count;

	Array<Shader_Light> shader_lights;

	Light *light = NULL;
	For(game_world->lights, light) {
		Shader_Light hlsl_light;
		hlsl_light.position = light->position;
		hlsl_light.direction = light->direction;
		hlsl_light.color = light->color;
		hlsl_light.light_type = light->light_type;
		hlsl_light.radius = light->radius;
		hlsl_light.range = light->range;
		shader_lights.push(hlsl_light);
	}
	light_struct_buffer.update(&shader_lights);
}

void Render_World::update_shadow_atlas()
{
	Array<Rect_u32 *> shadow_map_coordinates;
	Shadow_Map *shadow_map = NULL;
	For(shadow_maps, shadow_map) {
		shadow_map_coordinates.push(&shadow_map->coordinates_in_atlas);
	}

	Rect_u32 shadow_atlas_rect;
	pack_rects_in_rect(&shadow_atlas_rect, shadow_map_coordinates);
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

	world_matrix_struct_buffer.update(&world_matrices);
	
	render_entities.push(render_entity);
}

void Render_World::make_shadow(Entity_Id entity_id)
{
	if (entity_id.type != ENTITY_TYPE_LIGHT) {
		print("Render_World::make_shadow: Shadow map can not be created because entity id has not light type.");
		return;
	}
	Light *light = static_cast<Light *>(game_world->get_entity(entity_id));
	
	Vector3 light_diretion = light->direction;
	light_diretion.normalize();
	light_diretion *= -1.0f;
	Vector3 light_position = (world_bounding_sphere.radious * 4.0f) * light_diretion;
	light->position = light_position;

	Shadow_Map shadow_map;
	shadow_map.light_id = entity_id;

	switch (light->light_type) {
		case DIRECTIONAL_LIGHT_TYPE: {
			shadow_map.width = DIRECTION_SHADOW_MAP_WIDTH;
			shadow_map.height = DIRECTION_SHADOW_MAP_HEIGHT;
			shadow_map.coordinates_in_atlas.set_size(shadow_map.width, shadow_map.height);
			//shadow_map.light_view = XMMatrixLookAtLH(light_position, light->direction, Vector3(1.0f, 0.0f, 0.0f));
			shadow_map.light_view = XMMatrixLookAtLH(Vector3(0.0f, 200.0f, 0.0f), Vector3(0.0f, 0.0f, 0.0f), Vector3(1.0f, 0.0f, 0.0f));
			//shadow_map.light_view = XMMatrixLookAtLH(light_position, Vector3(0.0f, 0.0f, 0.0f), Vector3(0.0f, 1.0f, 0.0f));
			break;
		}
		assert(false);
	}
	shadow_maps.push(shadow_map);
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

inline float _get_angle_between_vectors(Vector2 &first_vector, Vector2 &second_vector)
{
	return math::arccos(first_vector.dot(second_vector) / (first_vector.length() * second_vector.length()));
}

void Render_World::render()
{
	render_sys->render_pipeline.update_constant_buffer(&frame_info_cbuffer, (void *)&frame_info);

	render_sys->render_pipeline.set_vertex_shader_resource(1, frame_info_cbuffer);
	render_sys->render_pipeline.set_pixel_shader_resource(1, frame_info_cbuffer);
	
	Render_Pass *render_pass = NULL;
	For(render_passes, render_pass) {
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
void Struct_Buffer::allocate(u32 elements_count)
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

	Shader_Resource_Desc shader_resource_desc;
	shader_resource_desc.format = DXGI_FORMAT_UNKNOWN;
	shader_resource_desc.resource_type = SHADER_RESOURCE_TYPE_BUFFER;
	shader_resource_desc.resource.buffer.element_count = elements_count;
	gpu_device->create_shader_resource_view(&gpu_buffer, &shader_resource_desc, &shader_resource);

	size = elements_count;
}

template<typename T>
void Struct_Buffer::update(Array<T> *array)
{
	if (array->count == 0) {
		return;
	}

	Render_Pipeline *render_pipeline = &Engine::get_render_system()->render_pipeline;
	
	if (array->count > size) {
		free();
		allocate<T>(array->count);
	}

	T *buffer = (T *)render_pipeline->map(gpu_buffer);
	memcpy((void *)buffer, (void *)&array->items[0], sizeof(T) * array->count);
	render_pipeline->unmap(gpu_buffer);
	count += array->count;
}

void Struct_Buffer::free()
{
	if (!gpu_buffer.is_empty()) {
		gpu_buffer.free();
	}
}

void Render_World::Light_Projections::init()
{
	float projection_plane_width = 100.0f;
	float projection_plane_height = 100.0f;
	float near_plane = 0.0f;
	float far_plane = 1000.0f;

	float ratio = (float)1000.0f / (float)1000.0f;
	float fov_y_ratio = XMConvertToRadians(45);
	direction_matrix = XMMatrixPerspectiveFovLH(fov_y_ratio, ratio, 1.0f, far_plane);

	//direction_matrix = XMMatrixOrthographicLH(projection_plane_width, projection_plane_height, near_plane, far_plane);
	direction_matrix = XMMatrixOrthographicOffCenterLH(-projection_plane_width, projection_plane_width, -projection_plane_height, projection_plane_height, near_plane, far_plane);
	point_matrix = XMMatrixOrthographicLH(projection_plane_width, projection_plane_height, near_plane, far_plane);
	spot_matrix = XMMatrixOrthographicLH(projection_plane_width, projection_plane_height, near_plane, far_plane);
}
