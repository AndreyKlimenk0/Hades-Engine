#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "hlsl.h"
#include "model.h"
#include "../libs/ds/array.h"
#include "../game/world.h"
#include "../render/model.h"
#include "render_system.h"

struct Render_Pass;
typedef u32 Render_Model_Idx;
typedef u32 Mesh_Idx;

struct Struct_Buffer {
	u32 count = 0;
	u32 size = 0;
	
	Gpu_Buffer gpu_buffer;
	Shader_Resource_View shader_resource;

	template <typename T>
	void allocate(u32 elements_count);
	template <typename T>
	void update(Array<T> *array);
	void free();
};

struct Render_Entity {
	Entity_Id entity_id;
	Mesh_Idx mesh_idx;
	u32 world_matrix_idx;
};

Render_Entity *find_render_entity(Array<Render_Entity> *render_entities, Entity_Id entity_id, u32 *index = NULL);

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

	Texture2D *texture_map = NULL;
	Depth_Stencil_View *dsv = NULL;

	void setup(Render_World *render_world);
	void update();
	void update_map();
};

template <typename T>
struct Unified_Mesh_Storate {
	Array<T> unified_vertices;
	Array<u32> unified_indices;
	Array<Mesh_Instance> mesh_instances;
	Hash_Table<String_Id, Mesh_Idx> mesh_table;

	Struct_Buffer vertex_struct_buffer;
	Struct_Buffer index_struct_buffer;
	Struct_Buffer mesh_struct_buffer;

	void allocate_gpu_memory();
	bool add_mesh(const char *mesh_name, Mesh<T> *mesh, Mesh_Idx *_mesh_idx);
};

struct Render_World {
	u32 light_hash;

	Frame_Info frame_info;
	Camera camera;

	Game_World *game_world = NULL;
	Render_System *render_sys = NULL;
	
	Array<Matrix4> world_matrices;

	//temp code
	Array<Entity_Id> entity_ids;
	
	Array<Render_Entity> render_entities;
	Array<Render_Entity> bounding_box_entities;
	Array<Render_Entity> mesh_outline_entities;
	
	Array<Render_Pass *> render_passes;

	Unified_Mesh_Storate<Vertex_XNUV> triangle_meshes;
	Unified_Mesh_Storate<Vector3> line_meshes;
	
	Gpu_Buffer frame_info_cbuffer;
	Texture2D default_texture;

	Struct_Buffer world_matrix_struct_buffer;
	Struct_Buffer light_struct_buffer;

	void init();
	void init_render_passes();

	void update();
	void update_lights();
	void update_depth_maps();
	void update_world_matrices();
	
	void make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx);

	void render();
	
	Render_Entity *find_render_entity(Entity_Id entity_id);
	bool add_mesh(const char *mesh_name, Mesh<Vertex_XNUV> *mesh, Mesh_Idx *mesh_idx);
	bool add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Idx *mesh_idx);
};
#endif
