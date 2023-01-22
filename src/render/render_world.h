#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "model.h"
#include "../libs/ds/array.h"
#include "../game/world.h"
#include "../render/model.h"
#include "render_system.h"

typedef u32 Render_Model_Id;
typedef u32 Mesh_Id;

struct Render_Entity {
	Entity_Id entity_id;
	Mesh_Id mesh_id;
	u32 world_matrix_id;
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


struct Hlsl_Light {
	Vector4 position;
	Vector4 direction;
	Vector4 color;
	u32 light_type;
	float radius;
	float range;
	float pad;
};

struct Render_World {

	u32 light_hash;

	Frame_Info frame_info;

	Free_Camera camera;

	View_Info *view_info = NULL;
	Gpu_Device *gpu_device = NULL;
	Render_Pipeline *render_pipeline = NULL;
	
	Gpu_Buffer *pass_data_cbuffer = NULL;
	Gpu_Buffer *frame_info_cbuffer = NULL;

	Array<Matrix4> world_matrices;
	
	Array<Vertex_XNUV> unified_vertices;
	Array<u32> unified_indices;
	Array<Mesh_Instance> mesh_instances;
	
	Array<Render_Entity> render_entities;
	
	Hash_Table<String_Id, Mesh_Id> mesh_table;

	Struct_Buffer vertex_struct_buffer;
	Struct_Buffer index_struct_buffer;
	Struct_Buffer mesh_struct_buffer;
	Struct_Buffer world_matrix_struct_buffer;
	Struct_Buffer light_struct_buffer;

	void init();
	void update();
	void make_render_entity(Entity_Id entity_id, Mesh_Id mesh_id);
	void update_world_matrices();
	void render();
	
	Mesh_Id add_mesh(const char *mesh_name, Triangle_Mesh *mesh);
};
#endif
