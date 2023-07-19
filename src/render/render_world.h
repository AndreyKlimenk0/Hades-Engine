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
const u32 SHADOW_ATLAS_WIDTH = 8192;
const u32 SHADOW_ATLAS_HEIGHT = 8192;
const u32 CASCADE_WIDTH = 1024;
const u32 CASCADE_HEIGHT = 1024;

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
};

struct Frustum_Box {
	struct Plane {
		Vector3 origin_top_left;
		Vector3 origin_top_right;
		Vector3 origin_bottom_left;
		Vector3 origin_bottom_right;

		Vector3 top_left;
		Vector3 top_right;
		Vector3 bottom_left;
		Vector3 bottom_right;

		void setup(float plane_width, float plane_height, float z_position);
		void transform_plane(Matrix4 *transform_matrix);
		void get_vertices(Array<Vector3> *vertices);
	};
	u32 len = 0;
	float max_x;
	float max_y;
	float max_z;
	float min_x;
	float min_y;
	float min_z;

	Plane near_plane;
	Plane far_plane;

	void calculate_length();
	void update_min_max_values();
	Vector3 get_view_position();
};

struct Shadow_Cascade_Range {
	u32 start = 0;
	u32 end = 0;
};

struct Cascaded_Shadow {
	u32 matrix_index;
	u32 view_projection_matrix_index;
	Shadow_Cascade_Range range;
	Vector3 light_direction;
	Matrix4 light_matrix;
	Viewport viewport;
	Frustum_Box frustum_box;

	void init(float fov, Shadow_Cascade_Range *shadow_cascade_range);
	void transform(Matrix4 *transform_matrix);
	Matrix4 get_cascade_view_matrix();
	Matrix4 get_cascade_projection_matrix();
};

struct Cascaded_Shadow_Map {
	Array<Cascaded_Shadow> cascaded_shadows;
};

struct Shadow_Cascade_Info {
	Matrix4 shadow_cascade_view_matrix;
	Matrix4 shadow_cascade_projection_matrix;
};

struct Render_Camera {
	Entity_Id camera_id;
	Matrix4 view_matrix;

	void update(Camera *camera);
	bool is_entity_camera_set();
};

struct Render_World {
	u32 light_hash;
	u32 cascaded_shadow_count = 0;

	Frame_Info frame_info;
	Render_Camera render_camera;

	Game_World *game_world = NULL;
	Render_System *render_sys = NULL;

	Bounding_Sphere world_bounding_sphere;
	
	Array<Entity_Id> entity_ids; 	//temp code
	
	Array<Matrix4> world_matrices;
	Array<Matrix4> light_view_matrices; // is the code necessary ? 
	Array<Matrix4> projection_light_matrices; // is the code necessary ?
	Array<Matrix4> cascaded_view_projection_matrices;
	
	Array<Render_Entity> render_entities;
	Array<Render_Entity> bounding_box_entities;
	Array<Render_Entity> mesh_outline_entities;

	Array<Cascaded_Shadow_Map> cascaded_shadow_maps;
	Array<Shadow_Cascade_Range> shadow_cascade_ranges;
	
	Array<Render_Pass *> render_passes_array;

	Unified_Mesh_Storate<Vertex_XNUV> triangle_meshes;
	Unified_Mesh_Storate<Vector3> line_meshes;
	
	Texture2D default_texture;
	Texture2D shadow_atlas;
	
	Gpu_Buffer frame_info_cbuffer;

	Gpu_Struct_Buffer lights_struct_buffer;
	Gpu_Struct_Buffer world_matrices_struct_buffer;
	Gpu_Struct_Buffer cascaded_view_projection_matrices_sb;

	struct Render_Passes {
		Shadows_Pass shadows;
		Draw_Lines_Pass draw_lines;
		Forwar_Light_Pass forward_light;
		Debug_Cascade_Shadows_Pass debug_cascade_shadows;
	} render_passes;

	void init();
	void init_shadow_rendering();
	void init_render_passes();

	void update();
	void update_lights();
	void update_shadows();
	void update_render_entities();
	
	void make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx);
	void make_line_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx);
	void make_render_entity(Entity_Id entity_id, Mesh_Idx mesh_idx, Matrix4 *matrix);
	bool make_shadow(Light *light);

	void render();

	void set_camera_for_rendering(Entity_Id camera_id);
	
	bool get_shadow_atls_viewport(Viewport *viewport);
	bool add_mesh(const char *mesh_name, Mesh<Vertex_XNUV> *mesh, Mesh_Idx *mesh_idx);
	bool add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Idx *mesh_idx);
	
	Render_Entity *find_render_entity(Entity_Id entity_id);
	Vector3 get_light_position(Vector3 light_direction);
};
#endif
