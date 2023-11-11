#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "hlsl.h"
#include "model.h"
#include "render_pass.h"
#include "render_system.h"
#include "render_helpers.h"
#include "../libs/ds/array.h"
#include "../libs/math/common.h"
#include "../game/world.h"


struct Render_Pass;
typedef u32 Render_Model_Idx;
typedef u32 Mesh_Idx;

const u32 CASCADE_COUNT = 3;
const u32 SHADOW_ATLAS_SIZE = 8192;
const u32 CASCADE_SIZE = 1024;

const R24U8 DEFAULT_DEPTH_VALUE = R24U8(0xffffff, 0);

struct Gpu_Struct_Buffer {
	Gpu_Buffer gpu_buffer;

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

	Gpu_Struct_Buffer vertex_struct_buffer;
	Gpu_Struct_Buffer index_struct_buffer;
	Gpu_Struct_Buffer mesh_struct_buffer;

	void allocate_gpu_memory();
	bool add_mesh(const char *mesh_name, Mesh<T> *mesh, Mesh_Idx *_mesh_idx);
	bool update_mesh(Mesh_Idx mesh_idx, Mesh<T> *mesh);
};

struct Shadow_Cascade_Range {
	u32 start = 0;
	u32 end = 0;
	u32 previous_range_length = 0;

	u32 get_length();
	Vector3 get_center_point();
};

struct Shadow_Cascade_Ranges {
	Array<Shadow_Cascade_Range> ranges;

	void add_range(u32 start, u32 end);
};

struct Cascaded_Shadow_Map {
	float cascade_width;
	float cascade_height;
	float cascade_depth;
	u32 view_projection_matrix_index;
	Vector3 view_position;
	Viewport viewport;

	void init(float fov, float aspect_ratio, Shadow_Cascade_Range *shadow_cascade_range);
};

struct Cascaded_Shadows {
	Vector3 light_direction;
	Array<Cascaded_Shadow_Map> cascaded_shadow_maps;
};

struct Render_Camera {
	Entity_Id camera_id;
	Entity_Id camera_info_id;
	Matrix4 view_matrix;
	Matrix4 debug_view_matrix;

	void update(Camera *camera, Camera *camera_info);
	bool is_entity_camera_set();
};

enum Rendering_Type {
	RENDERING_TYPE_FORWARD_RENDERING,
	RENDERING_TYPE_LINES_RENDERING,
	RENDERING_TYPE_VERTICES_RENDERING,
};

struct Render_World {
	Game_World *game_world = NULL;
	Render_System *render_sys = NULL;
	
	u32 light_hash;
	u32 cascaded_shadow_map_count = 0;
	u32 jittering_tile_size = 0;
	u32 jittering_filter_size = 0;
	u32 jittering_scaling = 0;

	CB_Frame_Info frame_info;
	Render_Camera render_camera;

	Bounding_Sphere world_bounding_sphere;
	
	Array<Entity_Id> entity_ids; 	//temp code
	
	Array<Matrix4> render_entity_world_matrices;
	Array<Matrix4> light_view_matrices; // is the code necessary ? 
	Array<Matrix4> cascaded_view_projection_matrices;
	
	Array<Render_Entity> forward_rendering_entities;
	Array<Render_Entity> line_rendering_entities;
	Array<Render_Entity> vertex_rendering_entities;
	Array<Color> line_rendering_entity_colors;
	Array<Color> vertex_rendering_entity_colors;
	
	Array<Cascaded_Shadows> cascaded_shadows_list;
	Array<Cascaded_Shadows_Info> cascaded_shadows_info_list;
	Array<Shadow_Cascade_Range> shadow_cascade_ranges;
	
	Array<Render_Pass *> render_passes_array;

	Unified_Mesh_Storate<Vertex_XNUV> triangle_meshes;
	Unified_Mesh_Storate<Vector3> line_meshes;
	
	Texture2D default_texture;
	Texture2D shadow_atlas;
	Texture3D jittering_samples;
	
	Gpu_Buffer frame_info_cbuffer;

	Gpu_Struct_Buffer lights_struct_buffer;
	Gpu_Struct_Buffer cascaded_shadows_info_sb;
	Gpu_Struct_Buffer world_matrices_struct_buffer;
	Gpu_Struct_Buffer cascaded_view_projection_matrices_sb;

	struct Render_Passes {
		Shadows_Pass shadows;
		Draw_Lines_Pass draw_lines;
		Draw_Vertices_Pass draw_vertices;
		Forwar_Light_Pass forward_light;
		Debug_Cascade_Shadows_Pass debug_cascade_shadows;
	} render_passes;

	void init();
	void init_meshes();
	void init_shadow_rendering();
	void init_render_passes();

	void update();
	void update_lights();
	void update_shadows();
	void update_render_entities();

	void add_render_entity(Rendering_Type rendering_type, Entity_Id entity_id, Mesh_Idx mesh_idx, void *args = NULL);
	bool add_shadow(Light *light);

	void render();

	void set_camera_for_rendering(Entity_Id camera_id);
	void set_camera_for_debuging(Entity_Id camera_info_id);
	
	bool get_shadow_atls_viewport(Viewport *viewport);
	bool add_mesh(const char *mesh_name, Mesh<Vertex_XNUV> *mesh, Mesh_Idx *mesh_idx);
	bool add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Idx *mesh_idx);
	
	Render_Entity *find_render_entity(Entity_Id entity_id);
	Vector3 get_light_position(Vector3 light_direction);
};
#endif
