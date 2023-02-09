#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "model.h"
#include "../libs/ds/array.h"
#include "../game/world.h"
#include "../render/model.h"
#include "render_system.h"

typedef u32 Render_Model_Idx;
typedef u32 Mesh_Idx;

struct Render_Entity {
	Entity_Id entity_idx;
	Mesh_Idx mesh_idx;
	u32 world_matrix_idx;
};

struct Frame_Info {
	Matrix4 view_matrix;
	Matrix4 perspective_matrix;
	Vector3 camera_position;
	int pad1;
	Vector3 camera_direction;
	int pad2;
	u32 light_count;
	Vector3 pad3;
};

struct Struct_Buffer {
	u32 count = 0;
	u32 size = 0;
	u32 shader_resource_register = 0;
	
	Gpu_Buffer *gpu_buffer = NULL;
	Shader_Resource shader_resource;

	template <typename T>
	void allocate(u32 elements_count);
	template <typename T>
	void update(Array<T> *array);
	void free();
};

struct Mesh_Instance {
	u32 vertex_count = 0;
	u32 index_count = 0;
	u32 vertex_offset = 0;
	u32 index_offset = 0;
};


struct Shader_Light {
	Vector4 position;
	Vector4 direction;
	Vector4 color;
	u32 light_type;
	float radius;
	float range;
	float pad;
};

template<typename T>
struct Render_Meshes_Data {
	Array<T> unified_vertices;
	Array<u32> unified_indices;
	Array<Mesh_Instance> mesh_instances;
	Array<Render_Entity> render_entities;
	
	Hash_Table<String_Id, Mesh_Idx> mesh_table;

	Struct_Buffer vertex_struct_buffer;
	Struct_Buffer index_struct_buffer;
	Struct_Buffer mesh_instance_struct_buffer;

	void init(u32 vertex_sb_register, u32 index_sb_register, u32 mesh_instance_sb_register);
	Mesh_Idx add_mesh(const char *mesh_name, Array<T> *vertices, Array<u32> *indices);
};

template<typename T>
void Render_Meshes_Data<T>::init(u32 vertex_sb_register, u32 index_sb_register, u32 mesh_instance_sb_register)
{
	vertex_struct_buffer.shader_resource_register = vertex_sb_register;
	index_struct_buffer.shader_resource_register = index_sb_register;
	mesh_instance_struct_buffer.shader_resource_register = mesh_instance_sb_register;

	vertex_struct_buffer.allocate<T>(100000);
	index_struct_buffer.allocate<u32>(100000);
	mesh_instance_struct_buffer.allocate<Mesh_Instance>(100000);
}

template<typename T>
Mesh_Idx Render_Meshes_Data<T>::add_mesh(const char *mesh_name, Array<T> *vertices, Array<u32> *indices)
{
	assert(mesh_name);

	if ((vertices->count == 0) || (indices->count == 0)) {
		print("Render_Meshes_Data::add_mesh: Mesh {} can be added because doesn't have all necessary data.", mesh_name);
		return UINT32_MAX;
	}

	String_Id string_id = fast_hash(mesh_name);

	Mesh_Idx mesh_idx;
	if (mesh_table.get(string_id, &mesh_idx)) {
		return mesh_idx;
	}

	Mesh_Instance mesh_instance;
	mesh_instance.vertex_count = vertices->count;
	mesh_instance.index_count = indices->count;
	mesh_instance.vertex_offset = unified_vertices.count;
	mesh_instance.index_offset = unified_indices.count;

	mesh_idx = mesh_instances.push(mesh_instance);
	merge(&unified_vertices, vertices);
	merge(&unified_indices, indices);

	mesh_instance_struct_buffer.update(&mesh_instances);
	vertex_struct_buffer.update(&unified_vertices);
	index_struct_buffer.update(&unified_indices);

	mesh_table.set(string_id, mesh_idx);
	return mesh_idx;
}


struct Render_World {
	u32 light_hash;

	Frame_Info frame_info;

	Camera camera;

	View_Info *view_info = NULL;
	Gpu_Device *gpu_device = NULL;
	Render_Pipeline *render_pipeline = NULL;
	Game_World *game_world = NULL;
	
	Gpu_Buffer *pass_data_cbuffer = NULL;
	Gpu_Buffer *frame_info_cbuffer = NULL;

	Array<Matrix4> world_matrices;
	
	Array<Vertex_XNUV> unified_vertices;
	Array<u32> unified_indices;
	Array<Mesh_Instance> mesh_instances;

	//temp code
	Array<Entity_Id> entity_ids;
	
	Array<Render_Entity> render_entities;
	
	Hash_Table<String_Id, Mesh_Idx> mesh_table;

	Struct_Buffer vertex_struct_buffer;
	Struct_Buffer index_struct_buffer;
	Struct_Buffer mesh_struct_buffer;
	Struct_Buffer world_matrix_struct_buffer;
	Struct_Buffer light_struct_buffer;

	void init();
	void update();
	void make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx);
	void update_world_matrices();
	void draw_outlines(Array<u32> *entity_ids);
	void draw_bounding_boxs(Array<Entity_Id> *entity_ids);
	void render();
	
	Render_Entity *find_render_entity(Entity_Id entity_id);
	Mesh_Idx add_mesh(const char *mesh_name, Triangle_Mesh *mesh);
};
#endif
