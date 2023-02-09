#include "render_world.h"
#include "../sys/engine.h"
#include <stdint.h>
#include "../gui/gui.h"


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

void Render_World::init()
{	
	game_world = Engine::get_game_world();
	Render_System *render_sys = Engine::get_render_system();
	
	view_info = &render_sys->view_info;
	gpu_device = &render_sys->gpu_device;
	render_pipeline = &render_sys->render_pipeline;

	light_struct_buffer.shader_resource_register = 6;
	mesh_struct_buffer.shader_resource_register = 2;
	world_matrix_struct_buffer.shader_resource_register = 3;
	index_struct_buffer.shader_resource_register = 4;
	vertex_struct_buffer.shader_resource_register = 5;

	light_struct_buffer.allocate<Shader_Light>(100);
	mesh_struct_buffer.allocate<Mesh_Instance>(1000);
	world_matrix_struct_buffer.allocate<Matrix4>(1000);
	index_struct_buffer.allocate<u32>(100000);
	vertex_struct_buffer.allocate<Vertex_XNUV>(100000);

	frame_info_cbuffer = gpu_device->create_constant_buffer((sizeof(Frame_Info)));
	pass_data_cbuffer = gpu_device->create_constant_buffer((sizeof(Pass_Data)));


	Box box;
	box.depth = 10;
	box.width = 10;
	box.height = 10;
	Entity_Id entity_id = game_world->make_geometry_entity(Vector3(0.0f, 20.0f, 10.0f), GEOMETRY_TYPE_BOX, (void *)&box);
	entity_ids.push(entity_id);
	
	Triangle_Mesh mesh;
	generate_box(&box, &mesh);

	char *mesh_name = format(box.width, box.height, box.depth);
	Mesh_Idx box_mesh_id = add_mesh(mesh_name, &mesh);

	make_render_entity(entity_id, box_mesh_id);
	free_string(mesh_name);

	make_AABB(entity_id, &mesh);

	Triangle_Mesh grid_mesh;
	Grid grid;
	grid.width = 100000.0f;
	grid.depth = 100000.0f;
	grid.rows_count = 10;
	grid.columns_count = 10;
	generate_grid(&grid, &grid_mesh);

	Mesh_Idx grid_mesh_id = add_mesh("grid_mesh", &grid_mesh);

	Entity_Id grid_entity_id = game_world->make_geometry_entity(Vector3(0.0f, 0.0f, 0.0f), GEOMETRY_TYPE_GRID, (void *)&grid);
	make_render_entity(grid_entity_id, grid_mesh_id);

	Sphere sphere;
	Entity_Id sphere_id = game_world->make_geometry_entity(Vector3(20.0, 20.0f, 0.0f), GEOMETRY_TYPE_SPHERE, (void *)&sphere);
	entity_ids.push(sphere_id);
	
	Triangle_Mesh sphere_mesh;
	generate_sphere(&sphere, &sphere_mesh);

	char *sphere_name = format(sphere.radius, sphere.slice_count, sphere.stack_count);

	Mesh_Idx mesh_idx = add_mesh(sphere_name, &sphere_mesh);
	make_AABB(sphere_id, &sphere_mesh);

	make_render_entity(sphere_id, mesh_idx);	

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
}

Render_Entity *Render_World::find_render_entity(Entity_Id entity_id)
{

	Render_Entity *render_entity = NULL;
	For(render_entities, render_entity) {
		if (render_entity->entity_idx == entity_id) {
			return render_entity;
		}
	}
	return NULL;
}

Mesh_Idx Render_World::add_mesh(const char *mesh_name, Triangle_Mesh *mesh)
{
	assert(mesh_name);
	
	if ((mesh->vertices.count == 0) || (mesh->indices.count == 0)) {
		print("Render_World::add_mesh: Mesh {} can be added because doesn't have all necessary data.", mesh_name);
		return UINT32_MAX;
	}

	String_Id string_id = fast_hash(mesh_name);
	
	Mesh_Idx mesh_idx;
	if (mesh_table.get(string_id, &mesh_idx)) {
		return mesh_idx;
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
	return mesh_idx;
}

void Render_World::make_render_entity(Entity_Id entity_idx, Mesh_Idx mesh_idx)
{
	Render_Entity render_entity;
	render_entity.entity_idx = entity_idx;
	render_entity.mesh_idx = mesh_idx;

	Entity *entity = game_world->get_entity(entity_idx);

	Matrix4 matrix;
	matrix.translate(&entity->position);
	render_entity.world_matrix_idx = world_matrices.push(matrix);

	world_matrix_struct_buffer.update(&world_matrices);
	
	render_entities.push(render_entity);
}

void Render_World::update_world_matrices()
{
	world_matrices.count = 0;
	
	for (u32 index = 0; index < render_entities.count; index++) {
		Entity *entity = game_world->get_entity(render_entities[index].entity_idx);
		
		Matrix4 matrix;
		matrix.translate(&entity->position);
		world_matrices.push(matrix);
	}
}

void Render_World::draw_outlines(Array<u32> *entity_ids)
{

}

void make_AABB_mesh(Entity *entity, Array<Vector3> *vertices, Array<u32> *indices)
{
	Vector3 min = entity->AABB_box.min;
	Vector3 max = entity->AABB_box.max;

	// Go  for clockwise.
	// Back cube side.
	vertices->push(min);
	vertices->push(Vector3(min.x, max.y, min.z));
	vertices->push(Vector3(max.x, max.y, min.z));
	vertices->push(Vector3(max.x, min.y, min.z));

	// Front cuve side.
	vertices->push(Vector3(min.x, min.y, max.z));
	vertices->push(Vector3(min.x, max.y, max.z));
	vertices->push(max);
	vertices->push(Vector3(max.x, min.y, max.z));

	indices->push(0);
	indices->push(1);

	indices->push(1);
	indices->push(2);

	indices->push(2);
	indices->push(3);

	indices->push(3);
	indices->push(0);

	indices->push(0 + 4);
	indices->push(1 + 4);

	indices->push(1 + 4);
	indices->push(2 + 4);

	indices->push(2 + 4);
	indices->push(3 + 4);

	indices->push(3 + 4);
	indices->push(0 + 4);

	indices->push(0);
	indices->push(0 + 4);

	indices->push(1);
	indices->push(1 + 4);

	indices->push(2);
	indices->push(2 + 4);

	indices->push(3);
	indices->push(3 + 4);
}

const u32 t0_register = 0;
const u32 t1_register = 1;
const u32 t2_register = 2;
const u32 t3_register = 3;
const u32 t4_register = 4;

const u32 b0_register = 0;
const u32 b1_register = 1;
const u32 b2_register = 2;

void Render_World::draw_bounding_boxs(Array<Entity_Id> *entity_ids)
{
	static Render_Meshes_Data<Vector3> render_data;
	static bool data_initialized = false;
	if (!data_initialized) {
		data_initialized = true;
		render_data.init(t0_register, t1_register, t2_register);
	}

	for (u32 i = 0; i < entity_ids->count; i++) {
		Entity_Id entity_id = entity_ids->get(i);
		Entity *entity = game_world->get_entity(entity_id);

		char *name = format(&entity->AABB_box.min, &entity->AABB_box.max);
		defer(free_string(name));

		if ((entity->bounding_box_type == BOUNDING_BOX_TYPE_AABB) && (!render_data.mesh_table.key_in_table(fast_hash(name)))) {
			Array<Vector3> vertices;
			Array<u32> indices;
			make_AABB_mesh(entity, &vertices, &indices);

			Mesh_Idx mesh_idx = render_data.add_mesh(name, &vertices, &indices);
			Render_Entity *render_entity = find_render_entity(entity_id);
			if (render_entity) {
				Render_Entity new_render_entity;
				new_render_entity.entity_idx = entity_id;
				new_render_entity.mesh_idx = mesh_idx;
				new_render_entity.world_matrix_idx = render_entity->world_matrix_idx;

				render_data.render_entities.push(new_render_entity);
			}
		}
	}

	Shader *shader;
	if (!Engine::get_render_system()->shaders.get("draw_lines", &shader)) {
		loop_print("Can not found shader");
		return;
	}
	
	render_pipeline->set_input_layout(NULL);
	render_pipeline->set_primitive(RENDER_PRIMITIVE_LINES);
	render_pipeline->set_vertex_shader(shader);
	render_pipeline->set_pixel_shader(shader);

	render_pipeline->set_vertex_shader_resource(b1_register, frame_info_cbuffer);
	render_pipeline->set_pixel_shader_resource(b1_register, frame_info_cbuffer);
	
	render_pipeline->update_constant_buffer(frame_info_cbuffer, (void *)&frame_info);
	render_pipeline->set_vertex_shader_resource(&render_data.vertex_struct_buffer);
	render_pipeline->set_vertex_shader_resource(&render_data.index_struct_buffer);
	render_pipeline->set_vertex_shader_resource(&render_data.mesh_instance_struct_buffer);
	render_pipeline->set_vertex_shader_resource(&world_matrix_struct_buffer);

	Render_Entity *render_entity = NULL;
	For(render_data.render_entities, render_entity) {
		Pass_Data pass_data;
		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(pass_data_cbuffer, (void *)&pass_data);
		render_pipeline->draw(render_data.mesh_instances[render_entity->mesh_idx].index_count);
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
	render_pipeline->set_pixel_shader_resource(&light_struct_buffer);

	Render_Entity *render_entity = NULL;
	For(render_entities, render_entity) {
		Pass_Data pass_data;
		pass_data.mesh_idx = render_entity->mesh_idx;
		pass_data.world_matrix_idx = render_entity->world_matrix_idx;

		render_pipeline->update_constant_buffer(pass_data_cbuffer, (void *)&pass_data);
		render_pipeline->set_vertex_shader_resource(2, pass_data_cbuffer);

		render_pipeline->draw(mesh_instances[render_entity->mesh_idx].index_count);
	}

	RELEASE_COM(temp->gpu_resource);
	RELEASE_COM(temp->shader_resource);
	DELETE_PTR(temp);

	draw_bounding_boxs(&entity_ids);
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
