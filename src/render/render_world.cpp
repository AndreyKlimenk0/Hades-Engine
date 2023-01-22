#include "render_world.h"
#include "../sys/engine.h"
#include <stdint.h>
#include "../gui/gui.h"


struct Pass_Data {
	u32 mesh_id;
	u32 world_matrix_id;
	u32 pad1;
	u32 pad2;
};

void Render_World::init()
{	
	Render_System *render_sys = Engine::get_render_system();
	
	view_info = &render_sys->view_info;
	gpu_device = &render_sys->gpu_device;
	render_pipeline = &render_sys->render_pipeline;

	light_struct_buffer.shader_resource_register = 6;
	mesh_struct_buffer.shader_resource_register = 2;
	world_matrix_struct_buffer.shader_resource_register = 3;
	index_struct_buffer.shader_resource_register = 4;
	vertex_struct_buffer.shader_resource_register = 5;

	light_struct_buffer.allocate<Hlsl_Light>(100);
	mesh_struct_buffer.allocate<Mesh_Instance>(1000);
	world_matrix_struct_buffer.allocate<Matrix4>(1000);
	index_struct_buffer.allocate<u32>(100000);
	vertex_struct_buffer.allocate<Vertex_XNUV>(100000);

	frame_info_cbuffer = gpu_device->create_constant_buffer((sizeof(Frame_Info)));
	pass_data_cbuffer = gpu_device->create_constant_buffer((sizeof(Pass_Data)));

	Game_World *game_world = Engine::get_game_world();
	Box box;
	Entity_Id entity_id = game_world->make_geometry_entity(Vector3(0.0f, 20.0f, 10.0f), GEOMETRY_TYPE_BOX, (void *)&box);

	Triangle_Mesh mesh;
	generate_box(&box, &mesh);

	char *mesh_name = format(box.width, box.height, box.depth);
	Mesh_Id mesh_id = add_mesh(mesh_name, &mesh);

	make_render_entity(entity_id, mesh_id);

	Triangle_Mesh grid_mesh;
	Grid grid;
	grid.width = 1000.0f;
	grid.depth = 1000.0f;
	grid.rows_count = 100;
	grid.columns_count = 100;
	generate_grid(&grid, &grid_mesh);

	Mesh_Id grid_mesh_id = add_mesh("grid_mesh", &grid_mesh);

	Entity_Id grid_entity_id = game_world->make_geometry_entity(Vector3(0.0f, 0.0f, 00.0f), GEOMETRY_TYPE_GRID, (void *)&grid);
	make_render_entity(grid_entity_id, grid_mesh_id);

	free_string(mesh_name);

	game_world->make_direction_light(Vector3(0.5f, -1.0f, -1.0f), Vector3(1.0f, 1.0f, 1.0f));
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
	frame_info.perspective_matrix = view_info->perspective_matrix;
	frame_info.camera_position = camera.position;
	frame_info.camera_direction = camera.target;

	Game_World *game_world = Engine::get_game_world();
	
	if (light_hash != game_world->light_hash) {
		light_hash = game_world->light_hash;
		
		frame_info.light_count = game_world->lights.count;

		Array<Hlsl_Light> hlsl_lights;
		
		Light *light = NULL;
		For(game_world->lights, light) {
			Hlsl_Light hlsl_light;
			hlsl_light.position = light->position;
			hlsl_light.direction = light->direction;
			hlsl_light.color = light->color;
			hlsl_light.light_type = light->light_type;
			hlsl_light.radius = light->radius;
			hlsl_light.range = light->range;
			hlsl_lights.push(hlsl_light);
		}
		light_struct_buffer.update(&hlsl_lights);
	}
}

Mesh_Id Render_World::add_mesh(const char *mesh_name, Triangle_Mesh *mesh)
{
	assert(mesh_name);
	
	if ((mesh->vertices.count == 0) || (mesh->indices.count == 0)) {
		print("Render_World::add_mesh: Mesh {} can be added because doesn't have all necessary data.");
		return UINT32_MAX;
	}

	String_Id string_id = fast_hash(mesh_name);
	
	Mesh_Id mesh_id;
	if (mesh_table.get(string_id, &mesh_id)) {
		return mesh_id;
	}

	Mesh_Instance mesh_info;
	mesh_info.vertex_count = mesh->vertices.count;
	mesh_info.index_count = mesh->indices.count;
	mesh_info.vertex_offset = unified_vertices.count;
	mesh_info.index_offset = unified_indices.count;

	mesh_id = mesh_instances.push(mesh_info);
	merge(&unified_vertices, &mesh->vertices);
	merge(&unified_indices, &mesh->indices);

	mesh_struct_buffer.update(&mesh_instances);
	vertex_struct_buffer.update(&unified_vertices);
	index_struct_buffer.update(&unified_indices);
	
	mesh_table.set(string_id, mesh_id);
	return mesh_id;
}

void Render_World::make_render_entity(Entity_Id entity_id, Mesh_Id mesh_id)
{
	Render_Entity render_entity;
	render_entity.entity_id = entity_id;
	render_entity.mesh_id = mesh_id;

	Game_World *game_world = Engine::get_game_world();
	Entity *entity = game_world->get_entity(entity_id);

	Matrix4 matrix;
	matrix.translate(&entity->position);
	render_entity.world_matrix_id = world_matrices.push(matrix);

	world_matrix_struct_buffer.update(&world_matrices);
	
	render_entities.push(render_entity);
}

void Render_World::update_world_matrices()
{
	world_matrices.count = 0;
	Game_World *game_world = Engine::get_game_world();
	
	for (u32 index = 0; index < render_entities.count; index++) {
		Entity *entity = game_world->get_entity(render_entities[index].entity_id);
		
		Matrix4 matrix;
		matrix.translate(&entity->position);
		world_matrices.push(matrix);
	}
}

void Render_World::render()
{
	render_pipeline->set_input_layout(NULL);
	render_pipeline->set_primitive(RENDER_PRIMITIVE_TRIANGLES);

	Shader *forward_light = Engine::get_render_system()->get_shader("forward_light");

	render_pipeline->set_vertex_shader(forward_light);
	render_pipeline->set_pixel_shader(forward_light);
	
	render_pipeline->set_vertex_shader_resource(1, frame_info_cbuffer);
	render_pipeline->set_pixel_shader_resource(1, frame_info_cbuffer);
	
	render_pipeline->update_constant_buffer(frame_info_cbuffer, (void *)&frame_info);

	render_pipeline->set_vertex_shader_resource(&mesh_struct_buffer);
	render_pipeline->set_vertex_shader_resource(&world_matrix_struct_buffer);
	render_pipeline->set_vertex_shader_resource(&vertex_struct_buffer);
	render_pipeline->set_vertex_shader_resource(&index_struct_buffer);

	Texture *temp = gpu_device->create_texture_2d(200, 200, NULL, 1);
	u32 *pixel_buffer = create_color_buffer(200, 200, Color(74, 82, 90));
	render_pipeline->update_subresource(temp, (void *)pixel_buffer, temp->get_row_pitch());
	DELETE_PTR(pixel_buffer);
	
	render_pipeline->set_pixel_shader_sampler(Engine::get_render_system()->sampler);
	render_pipeline->set_pixel_shader_resource(temp->shader_resource);
	print("Light register = ", light_struct_buffer.shader_resource_register);
	render_pipeline->set_pixel_shader_resource(&light_struct_buffer);

	Render_Entity *render_entity = NULL;

	For(render_entities, render_entity) {
		Pass_Data pass_data;
		pass_data.mesh_id = render_entity->mesh_id;
		pass_data.world_matrix_id = render_entity->world_matrix_id;

		render_pipeline->update_constant_buffer(pass_data_cbuffer, (void *)&pass_data);
		render_pipeline->set_vertex_shader_resource(2, pass_data_cbuffer);

		render_pipeline->draw(mesh_instances[render_entity->mesh_id].index_count);
	}

	RELEASE_COM(temp->gpu_resource);
	RELEASE_COM(temp->shader_resource);
	DELETE_PTR(temp);
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
	gpu_buffer = gpu_device->create_gpu_buffer(&desc);

	gpu_device->create_shader_resource_for_struct_buffer(gpu_buffer, elements_count, &shader_resource);

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
	if (gpu_buffer) {
		gpu_buffer->free();
	}
	shader_resource.free();
}
