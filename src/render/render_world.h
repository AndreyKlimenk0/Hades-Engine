#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "hlsl.h"
#include "model.h"
#include "render_system.h"
#include "render_helpers.h"
#include "../libs/ds/array.h"
#include "../libs/math/common.h"
#include "../game/world.h"


struct Render_Pass;
typedef u32 Render_Model_Idx;
typedef u32 Mesh_Idx;

const u32 SHADOW_ATLAS_WIDTH = 8192;
const u32 SHADOW_ATLAS_HEIGHT = 8192;
//const u32 DIRECTION_SHADOW_MAP_WIDTH = 2048;
//const u32 DIRECTION_SHADOW_MAP_HEIGHT = 2048;

const u32 DIRECTION_SHADOW_MAP_WIDTH = 2800;
const u32 DIRECTION_SHADOW_MAP_HEIGHT = 1445;

const R24U8 DEFAULT_DEPTH_VALUE = R24U8(0xffffff, 0);

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

template <typename T>
struct Unified_Mesh_Storate {
	struct Mesh_Instance {
		u32 vertex_count = 0;
		u32 index_count = 0;
		u32 vertex_offset = 0;
		u32 index_offset = 0;
	};

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

struct Shadow_Map {
	u32 light_view_matrix_idx;
};

struct Render_World {
	u32 light_hash;

	Frame_Info frame_info;
	Camera camera;

	Game_World *game_world = NULL;
	Render_System *render_sys = NULL;

	Bounding_Sphere world_bounding_sphere;
	
	//temp code
	Array<Entity_Id> entity_ids;
	
	Array<Matrix4> world_matrices;
	Array<Matrix4> light_view_matrices;
	Array<Render_Entity> render_entities;
	Array<Render_Entity> bounding_box_entities;
	Array<Render_Entity> mesh_outline_entities;
	Array<Shadow_Map> shadow_maps;
	Array<Render_Pass *> render_passes;

	Unified_Mesh_Storate<Vertex_XNUV> triangle_meshes;
	Unified_Mesh_Storate<Vector3> line_meshes;

	struct Light_Projections {
		Matrix4 direction_matrix;
		Matrix4 point_matrix;
		Matrix4 spot_matrix;

		void init();
	} light_projections;
	
	Texture2D default_texture;
	Texture2D shadow_atlas;
	Depth_Stencil_Buffer temp_shadow_storage;
	
	Gpu_Buffer frame_info_cbuffer;
	Gpu_Buffer light_projections_cbuffer;

	Struct_Buffer world_matrix_struct_buffer;
	Struct_Buffer light_view_matrices_struct_buffer;
	Struct_Buffer lights_struct_buffer;
	Struct_Buffer shadow_maps_struct_buffer;

	void init();
	void init_shadow_rendering();
	void init_render_passes();

	void update();
	void update_lights();
	void update_shadow_atlas();
	void update_world_matrices();
	
	void make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx);
	u32 make_shadow(Light *light);

	void render();
	
	Render_Entity *find_render_entity(Entity_Id entity_id);
	bool add_mesh(const char *mesh_name, Mesh<Vertex_XNUV> *mesh, Mesh_Idx *mesh_idx);
	bool add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Idx *mesh_idx);

	Vector3 get_light_position(Vector3 light_direction);
};
#endif
