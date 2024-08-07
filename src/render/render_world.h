#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "hlsl.h"
#include "mesh.h"
#include "render_passes.h"
#include "render_system.h"
#include "render_helpers.h"
#include "../game/world.h"
#include "../libs/color.h"
#include "../libs/number_types.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/structures/array.h"

struct Engine;
struct Render_Pass;
typedef u32 Texture_Idx;
typedef u32 Render_Entity_Idx;

const u32 CASCADE_COUNT = 3;
const u32 SHADOW_ATLAS_SIZE = 8192;
const u32 CASCADE_SIZE = 1024;

const R24U8 DEFAULT_DEPTH_VALUE = R24U8(0xffffff, 0);

struct Mesh_Id {
	u32 textures_idx;
	u32 instance_idx;
};

struct Render_Entity {
	u32 world_matrix_idx;
	Mesh_Id mesh_id;
	Entity_Id entity_id;
};

Matrix4 get_world_matrix(Entity *entity);
Render_Entity *find_render_entity(Array<Render_Entity> *render_entities, Entity_Id entity_id, u32 *index = NULL);

struct Mesh_Textures {
	Texture_Idx normal_idx;
	Texture_Idx diffuse_idx;
	Texture_Idx specular_idx;
	Texture_Idx displacement_idx;
};

struct Mesh_Storate {
	struct Mesh_Instance {
		u32 vertex_count = 0;
		u32 index_count = 0;
		u32 vertex_offset = 0;
		u32 index_offset = 0;
	};

	struct Default_Textures {
		Texture_Idx normal;
		Texture_Idx diffuse;
		Texture_Idx specular;
		Texture_Idx displacement;
		Texture_Idx white;
		Texture_Idx black;
		Texture_Idx green;
	};

	Default_Textures default_textures;

	Array<Vertex_PNTUV> unified_vertices;
	Array<u32> unified_indices;
	Array<Texture2D> textures;
	Array<Mesh_Instance> mesh_instances;
	Array<Mesh_Textures> meshes_textures;
	Array<String> loaded_meshes;

	Hash_Table<String_Id, Mesh_Id> mesh_table;
	Hash_Table<String_Id, Texture_Idx> texture_table;

	Gpu_Struct_Buffer vertex_struct_buffer;
	Gpu_Struct_Buffer index_struct_buffer;
	Gpu_Struct_Buffer mesh_struct_buffer;

	void init(Gpu_Device *gpu_device);
	void release_all_resources();

	void allocate_gpu_memory();
	bool add_mesh(const char *mesh_name, Triangle_Mesh *triangle_mesh, Mesh_Id *mesh_id);
	bool add_texture(const char *texture_name, Texture_Idx *texture_idx);
	bool update_mesh(Mesh_Id mesh_id, Triangle_Mesh *triangle_mesh);
	Texture_Idx find_texture_or_get_default(String &texture_file_name, Texture_Idx default_texture);

	Mesh_Textures *get_mesh_textures(u32 index);
	Texture2D *get_texture(Texture_Idx texture_idx);
};

inline Mesh_Textures *Mesh_Storate::get_mesh_textures(u32 index)
{
	return &meshes_textures[index];
}

inline Texture2D *Mesh_Storate::get_texture(Texture_Idx texture_idx)
{
	return &textures[texture_idx];
}

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
	Matrix4 view_projection_matrix;

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
	Matrix4 inverse_view_matrix;
	Matrix4 view_pespective_matrix;
	Matrix4 inverse_view_pespective_matrix;

	Matrix4 debug_view_matrix;

	void update(Camera *camera, Camera *camera_info);
	bool is_entity_camera_set();
};

struct Voxel {
	u32 encoded_color;
	u32 occlusion;
};

struct Voxel_Grid {
	u32 width;
	u32 height;
	u32 depth;
	u32 ceil_width;
	u32 ceil_height;
	u32 ceil_depth;

	u32 total_width() { return width * ceil_width; }
	u32 total_height() { return height * ceil_height; }
	u32 total_depth() { return depth * ceil_depth;}
};

struct Render_World {
	Game_World *game_world = NULL;
	Render_System *render_sys = NULL;

	u32 light_hash;
	u32 cascaded_shadow_map_count = 0;
	u32 jittering_tile_size = 0;
	u32 jittering_filter_size = 0;
	u32 jittering_scaling = 0;

	Matrix4 voxel_matrix;
	Voxel_Grid voxel_grid;
	Vector3 voxel_grid_center;
	
	Matrix4 left_to_right_voxel_view_matrix;
	Matrix4 top_to_down_voxel_view_matrix;
	Matrix4 back_to_front_voxel_view_matrix;

	CB_Frame_Info frame_info;
	Render_Camera render_camera;

	Bounding_Sphere world_bounding_sphere;

	Array<Matrix4> render_entity_world_matrices;
	Array<Matrix4> light_view_matrices; // is the code necessary ? 
	Array<Matrix4> cascaded_view_projection_matrices;

	Array<Render_Entity> game_render_entities;

	Array<Cascaded_Shadows> cascaded_shadows_list;
	Array<Cascaded_Shadows_Info> cascaded_shadows_info_list;
	Array<Shadow_Cascade_Range> shadow_cascade_ranges;

	Array<Render_Pass *> frame_render_passes;

	Mesh_Storate triangle_meshes;

	Texture2D shadow_atlas;
	Texture3D jittering_samples;

	Gpu_Buffer frame_info_cbuffer;

	Gpu_RWStruct_Buffer voxels_sb;
	Gpu_Struct_Buffer lights_struct_buffer;
	Gpu_Struct_Buffer cascaded_shadows_info_sb;
	Gpu_Struct_Buffer world_matrices_struct_buffer;
	Gpu_Struct_Buffer cascaded_view_projection_matrices_sb;

	struct Render_Passes {
		Shadows_Pass shadows;
		Forwar_Light_Pass forward_light;
		Debug_Cascade_Shadows_Pass debug_cascade_shadows;
		Outlining_Pass outlining;
		Voxelization voxelization;

		void get_all_passes(Array<Render_Pass *> *render_passes_list);
	} render_passes;

	void init(Engine *engine);
	void init_shadow_rendering();
	void init_render_passes(Shader_Manager *shader_manager);
	void release_all_resources();
	void release_render_entities_resources();

	void update();
	void update_lights();
	void update_shadows();
	void update_render_entities();
	void update_global_illumination();

	void add_render_entity(Entity_Id entity_id, Mesh_Id mesh_id, void *args = NULL);
	bool add_shadow(Light *light);

	u32 delete_render_entity(Entity_Id entity_id);

	void render();

	void set_camera_for_rendering(Entity_Id camera_id);
	void set_camera_for_debuging(Entity_Id camera_info_id);

	bool get_shadow_atls_viewport(Viewport *viewport);
	bool add_triangle_mesh(const char *mesh_name, Triangle_Mesh *triangle_mesh, Mesh_Id *mesh_id);

	Vector3 get_light_position(Vector3 light_direction);
};
#endif

