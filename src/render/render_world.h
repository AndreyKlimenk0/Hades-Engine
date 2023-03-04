#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "hlsl.h"
#include "model.h"
#include "../libs/ds/array.h"
#include "../game/world.h"
#include "../render/model.h"
#include "render_system.h"

typedef u32 Render_Model_Idx;
typedef u32 Mesh_Idx;

struct Struct_Buffer {
	u32 count = 0;
	u32 size = 0;
	u32 shader_resource_register = 0;
	
	Gpu_Buffer gpu_buffer;
	Shader_Resource_View shader_resource;

	template <typename T>
	void allocate(u32 elements_count);
	template <typename T>
	void update(Array<T> *array);
	void free();
};

struct Render_Entity {
	Entity_Id entity_idx;
	Mesh_Idx mesh_idx;
	u32 world_matrix_idx;
};

struct Mesh_Instance {
	u32 vertex_count = 0;
	u32 index_count = 0;
	u32 vertex_offset = 0;
	u32 index_offset = 0;
};

struct Shadows_Map {
	Game_World *game_world = NULL;
	Gpu_Device *gpu_device = NULL;
	Render_Pipeline *render_pipeline = NULL;

	Texture *texture_map = NULL;
	Depth_Stencil_View *dsv = NULL;

	void init(Render_World *render_world);
	void update();
	void update_map();
};

struct Render_Pass {
	virtual void setup_pipeline_state() = 0;
	virtual void render() = 0;
};

struct Forwar_Light_Pass : Render_Pass {
	void setup_pipeline_state();
	void render();
};

struct Render_World {
	u32 light_hash;

	Frame_Info frame_info;

	Camera camera;

	Game_World *game_world = NULL;

	View_Info *view_info = NULL;
	Gpu_Device *gpu_device = NULL;
	Render_Pipeline *render_pipeline = NULL;
	
	Gpu_Buffer pass_data_cbuffer;
	Gpu_Buffer frame_info_cbuffer;

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
	void update_lights();
	void update_depth_maps();
	void make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx);
	void update_world_matrices();
	void draw_outlines(Array<Entity_Id> *entity_ids);
	void draw_bounding_boxs(Array<Entity_Id> *entity_ids);
	void render();
	
	Render_Entity *find_render_entity(Entity_Id entity_id);
	Mesh_Idx add_mesh(const char *mesh_name, Triangle_Mesh *mesh);
};
#endif
