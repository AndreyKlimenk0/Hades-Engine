#include <stdint.h>

#include "render_world.h"
#include "render_pass.h"

#include "../sys/engine.h"
#include "../gui/gui.h"


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

struct Pass_Data {
	u32 mesh_idx;
	u32 world_matrix_idx;
	u32 pad1;
	u32 pad2;
};

void make_AABB(Entity_Id entity_id, Triangle_Mesh *mesh)
{
	Game_World *game_world = Engine::get_game_world();
	Entity *entity = game_world->get_entity(entity_id);

	Vector3 min = { 0.0f, 0.0f, 0.0f };
	Vector3 max = { 0.0f, 0.0f, 0.0f };
	for (u32 i = 0; i < mesh->vertices.count; i++) {
		Vector3 position = mesh->vertices[i].position;
		if (position.x < min.x) {
			min.x = position.x;
		}
		if (position.y < min.y) {
			min.y = position.y;
		}
		if (position.z > min.z) {
			min.z = position.z;
		}
		if (position.x > max.x) {
			max.x = position.x;
		}
		if (position.y > max.y) {
			max.y = position.y;
		}
		if (position.z < max.z) {
			max.z = position.z;
		}
	}
	entity->bounding_box_type = BOUNDING_BOX_TYPE_AABB;
	entity->AABB_box.min = min;
	entity->AABB_box.max = max;
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

	init_render_passes();
	
	render_sys->gpu_device.create_constant_buffer(sizeof(Frame_Info), &frame_info_cbuffer);

	light_struct_buffer.allocate<Shader_Light>(100);
	world_matrix_struct_buffer.allocate<Matrix4>(1000);

	u32 *pixel_buffer = create_color_buffer(200, 200, Color(74, 82, 90));
	
	Texture_Desc texture_desc;
	texture_desc.width = 200;
	texture_desc.height = 200;
	texture_desc.mip_levels = 1;
	
	render_sys->gpu_device.create_texture_2d(&texture_desc, &default_texture);
	render_sys->render_pipeline.update_subresource(&default_texture, (void *)pixel_buffer, default_texture.get_row_pitch());
	
	DELETE_PTR(pixel_buffer);

	Box box;
	box.depth = 10;
	box.width = 10;
	box.height = 10;
	Entity_Id entity_id = game_world->make_geometry_entity(Vector3(0.0f, 20.0f, 10.0f), GEOMETRY_TYPE_BOX, (void *)&box);
	entity_ids.push(entity_id);
	
	Triangle_Mesh mesh;
	make_box_mesh(&box, &mesh);

	char *mesh_name = format(box.width, box.height, box.depth);
	Mesh_Idx box_mesh_id;
	add_mesh(mesh_name, &mesh, &box_mesh_id);

	make_render_entity(entity_id, box_mesh_id);
	free_string(mesh_name);

	make_AABB(entity_id, &mesh);

	Triangle_Mesh grid_mesh;
	Grid grid;
	grid.width = 100000.0f;
	grid.depth = 100000.0f;
	grid.rows_count = 10;
	grid.columns_count = 10;
	make_grid_mesh(&grid, &grid_mesh);

	Mesh_Idx grid_mesh_id;
	add_mesh("grid_mesh", &grid_mesh, &grid_mesh_id);

	Entity_Id grid_entity_id = game_world->make_geometry_entity(Vector3(0.0f, 0.0f, 0.0f), GEOMETRY_TYPE_GRID, (void *)&grid);
	make_render_entity(grid_entity_id, grid_mesh_id);

	Sphere sphere;
	Entity_Id sphere_id = game_world->make_geometry_entity(Vector3(20.0, 20.0f, 0.0f), GEOMETRY_TYPE_SPHERE, (void *)&sphere);
	entity_ids.push(sphere_id);
	
	Triangle_Mesh sphere_mesh;
	make_sphere_mesh(&sphere, &sphere_mesh);

	char *sphere_name = format(sphere.radius, sphere.slice_count, sphere.stack_count);

	Mesh_Idx mesh_idx;
	add_mesh(sphere_name, &sphere_mesh, &mesh_idx);
	make_AABB(sphere_id, &sphere_mesh);

	make_render_entity(sphere_id, mesh_idx);	

	game_world->make_direction_light(Vector3(0.5f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
}

void Render_World::init_render_passes()
{
	Forwar_Light_Pass *forward_light_pass = new Forwar_Light_Pass((void *)this);
	Draw_Lines_Pass *draw_lines_pass = new Draw_Lines_Pass((void *)this);

	render_passes.push(forward_light_pass);
	render_passes.push(draw_lines_pass);

	Render_System *render_sys = Engine::get_render_system();
	for (u32 i = 0; i < render_passes.count; i++) {
		
		render_passes[i]->init(&render_sys->gpu_device);
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
	frame_info.camera_position = camera.position;
	frame_info.camera_direction = camera.target;

	Game_World *game_world = Engine::get_game_world();
	
	if (light_hash != game_world->light_hash) {
		light_hash = game_world->light_hash;
		update_lights();
		update_depth_maps();
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

void Shadows_Map::setup(Render_World *render_world)
{
	//game_world = render_world->game_world;
	//gpu_device = render_world->gpu_device;
	//render_pipeline = render_world->render_pipeline;
}

void Shadows_Map::update()
{
	u32 directional_light_count = 0;
	Light *light = NULL;
	For(game_world->lights, light) {
		if (light->light_type == DIRECTIONAL_LIGHT_TYPE) {
			directional_light_count += 1;
		}
	}

	//Texture_Desc texture_desc;
	//texture_desc.width = 1024;
	//texture_desc.height = 1024;
	//texture_desc.mip_levels = 1;
	//texture_desc.array_count = directional_light_count;
	//texture_desc.format = DXGI_FORMAT_R32_TYPELESS;
	//texture_desc.bind = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;

	//Shader_Resource_Desc shader_resource_desc;
	//shader_resource_desc.format = DXGI_FORMAT_R32_FLOAT;
	//shader_resource_desc.resource_type = SHADER_RESOURCE_TYPE_TEXTURE_2D_ARRAY;
	//shader_resource_desc.resource.texture_2d_array.count = directional_light_count;
	//shader_resource_desc.resource.texture_2d_array.mip_levels = 1;
	//shader_resource_desc.resource.texture_2d_array.most_detailed_mip = 0;

	//texture_map = gpu_device->create_texture_2d(&texture_desc, &shader_resource_desc);

	//Depth_Stencil_View_Desc depth_stencil_view_desc;
	//depth_stencil_view_desc.format = DXGI_FORMAT_D32_FLOAT;
	//depth_stencil_view_desc.type = DEPTH_STENCIL_VIEW_TYPE_TEXTURE_2D_ARRAY;
	//depth_stencil_view_desc.view.texture_2d_array.mip_slice = 0;
	//depth_stencil_view_desc.view.texture_2d_array.first_array_slice = 0;
	//depth_stencil_view_desc.view.texture_2d_array.array_count = directional_light_count;

	//gpu_device->create_depth_stencil_view(texture_map, &depth_stencil_view_desc, dsv);

	//update_map();
}

void Shadows_Map::update_map()
{
	//Shader *shader;
	//if (!Engine::get_render_system()->shader_table.get("depth_map", &shader)) {
	//	loop_print("Shadows_Map::update_map: Can not found shader");
	//	return;
	//}

	//render_pipeline->set_input_layout(NULL);
	//render_pipeline->set_primitive(RENDER_PRIMITIVE_TRIANGLES);

	//Shader *forward_light = Engine::get_render_system()->get_shader("forward_light");

	//render_pipeline->set_vertex_shader(forward_light);
	//render_pipeline->set_pixel_shader(forward_light);

	//render_pipeline->set_vertex_shader_resource(1, frame_info_cbuffer);
	//render_pipeline->set_pixel_shader_resource(1, frame_info_cbuffer);

	//render_pipeline->update_constant_buffer(frame_info_cbuffer, (void *)&frame_info);

	//render_pipeline->set_vertex_shader_resource(&mesh_struct_buffer);
	//render_pipeline->set_vertex_shader_resource(&world_matrix_struct_buffer);
	//render_pipeline->set_vertex_shader_resource(&vertex_struct_buffer);
	//render_pipeline->set_vertex_shader_resource(&index_struct_buffer);

	//Texture_Desc texture_desc;
	//texture_desc.width = 200;
	//texture_desc.height = 200;
	//texture_desc.mip_levels = 1;
	//Texture2D *temp = gpu_device->create_texture_2d(&texture_desc);
	//u32 *pixel_buffer = create_color_buffer(200, 200, Color(74, 82, 90));
	//render_pipeline->update_subresource(temp, (void *)pixel_buffer, temp->get_row_pitch());
	//DELETE_PTR(pixel_buffer);

	//render_pipeline->set_pixel_shader_sampler(Engine::get_render_system()->sampler);
	//render_pipeline->set_pixel_shader_resource(temp->shader_resource);
	//render_pipeline->set_pixel_shader_resource(&light_struct_buffer);

	//Render_Entity *render_entity = NULL;
	//For(render_entities, render_entity) {
	//	Pass_Data pass_data;
	//	pass_data.mesh_idx = render_entity->mesh_idx;
	//	pass_data.world_matrix_idx = render_entity->world_matrix_idx;

	//	render_pipeline->update_constant_buffer(pass_data_cbuffer, (void *)&pass_data);
	//	render_pipeline->set_vertex_shader_resource(2, pass_data_cbuffer);

	//	render_pipeline->draw(mesh_instances[render_entity->mesh_idx].index_count);
	//}

	//RELEASE_COM(temp->gpu_resource);
	//RELEASE_COM(temp->shader_resource);
	//DELETE_PTR(temp);
}


void Render_World::update_depth_maps()
{
	//u32 directional_light_count = 0;
	//Light *light = NULL;
	//For(game_world->lights, light) {
	//	if (light->light_type == DIRECTIONAL_LIGHT_TYPE) {
	//		directional_light_count += 1;
	//	}
	//}
	//
	//if (depth_maps) {
	//	depth_maps->free();
	//}
	//
	//Texture_Desc texture_desc;
	//texture_desc.width = 1024;
	//texture_desc.height = 1024;
	//texture_desc.mip_levels = 1;
	//texture_desc.array_count = directional_light_count;
	//texture_desc.format = DXGI_FORMAT_R32_TYPELESS;
	//texture_desc.bind = BIND_SHADER_RESOURCE | BIND_DEPTH_STENCIL;
	//
	//Shader_Resource_Desc shader_resource_desc;
	//shader_resource_desc.format = DXGI_FORMAT_R32_FLOAT;
	//shader_resource_desc.resource_type = SHADER_RESOURCE_TYPE_TEXTURE_2D_ARRAY;
	//shader_resource_desc.resource.texture_2d_array.count = directional_light_count;
	//shader_resource_desc.resource.texture_2d_array.mip_levels = 1;
	//shader_resource_desc.resource.texture_2d_array.most_detailed_mip = 0;

	//depth_maps = gpu_device->create_texture_2d(&texture_desc, &shader_resource_desc);

	//Depth_Stencil_View_Desc depth_stencil_view_desc;
	//depth_stencil_view_desc.format = DXGI_FORMAT_D32_FLOAT;
	//depth_stencil_view_desc.type = DEPTH_STENCIL_VIEW_TYPE_TEXTURE_2D_ARRAY;
	//depth_stencil_view_desc.view.texture_2d_array.mip_slice = 0;
	//depth_stencil_view_desc.view.texture_2d_array.first_array_slice = 0;
	//depth_stencil_view_desc.view.texture_2d_array.array_count = directional_light_count;

	//Depth_Stencil_View *depth_stencil_view = NULL;
	//gpu_device->create_depth_stencil_view(depth_maps, &depth_stencil_view_desc, depth_stencil_view);

	//Shader *shader;
	//if (!Engine::get_render_system()->shader_table.get("draw_lines", &shader)) {
	//	loop_print("Can not found shader");
	//	return;
	//}

	//render_pipeline->set_input_layout(NULL);
	//render_pipeline->set_primitive(RENDER_PRIMITIVE_LINES);
	//render_pipeline->set_vertex_shader(shader);
	//render_pipeline->set_pixel_shader(shader);

	//render_pipeline->set_vertex_shader_resource(b1_register, frame_info_cbuffer);
	//render_pipeline->set_pixel_shader_resource(b1_register, frame_info_cbuffer);

	//render_pipeline->update_constant_buffer(frame_info_cbuffer, (void *)&frame_info);
	//render_pipeline->set_vertex_shader_resource(&vertex_struct_buffer);
	//render_pipeline->set_vertex_shader_resource(&index_struct_buffer);
	//render_pipeline->set_vertex_shader_resource(&mesh_struct_buffer);
	//render_pipeline->set_vertex_shader_resource(&world_matrix_struct_buffer);

	//Render_Entity *render_entity = NULL;
	//For(render_entities, render_entity) {
	//	Pass_Data pass_data;
	//	pass_data.mesh_idx = render_entity->mesh_idx;
	//	pass_data.world_matrix_idx = render_entity->world_matrix_idx;

	//	render_pipeline->update_constant_buffer(pass_data_cbuffer, (void *)&pass_data);
	//	//render_pipeline->draw(mesh_instances[render_entity->mesh_idx].index_count);
	//}
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

bool Render_World::add_mesh(const char *mesh_name, Mesh<Vertex_XNUV> *mesh, Mesh_Idx *mesh_idx)
{
	return triangle_meshes.add_mesh(mesh_name, mesh, mesh_idx);
}

bool Render_World::add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Idx *mesh_idx)
{
	return line_meshes.add_mesh(mesh_name, mesh, mesh_idx);
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

	T *buffer = (T *)render_pipeline->map(&gpu_buffer);
	memcpy((void *)buffer, (void *)&array->items[0], sizeof(T) * array->count);
	render_pipeline->unmap(&gpu_buffer);
	count += array->count;
}

void Struct_Buffer::free()
{
	if (!gpu_buffer.is_empty()) {
		gpu_buffer.free();
	}
}