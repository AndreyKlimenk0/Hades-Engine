#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "gpu_data.h"
#include "mesh.h"
#include "render_passes.h"
#include "render_system.h"
#include "render_helpers.h"
#include "../game/world.h"
#include "../libs/str.h"
#include "../libs/color.h"
#include "../libs/number_types.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/math/structures.h"
#include "../libs/structures/array.h"
#include "../libs/structures/hash_table.h"

#include "render_resources.h"

struct Engine;
struct Render_Pass;

const u32 CASCADE_COUNT = 3;
const u32 SHADOW_ATLAS_SIZE = 8192;
const u32 CASCADE_SIZE = 1024;

const R24U8 DEFAULT_DEPTH_VALUE = R24U8(0xffffff, 0);

struct Render_Entity {
	u32 world_matrix_idx;
	u32 mesh_idx;
	Entity_Id entity_id;
};

Matrix4 get_world_matrix(Entity *entity);
Render_Entity *find_render_entity(Array<Render_Entity> *render_entities, Entity_Id entity_id, u32 *index = NULL);

struct GPU_Material {
	u32 normal_idx;
	u32 diffuse_idx;
	u32 specular_idx;
	u32 displacement_idx;
};

struct Mesh_Instance {
	u32 vertex_count = 0;
	u32 index_count = 0;
	u32 vertex_offset = 0;
	u32 index_offset = 0;

	GPU_Material material;
};

struct Render_Model {
	String name;
	String file_name;
	Texture *normal_texture;
	Texture *diffuse_texture;
	Texture *specular_texture;
	Texture *displacement_texture;
	Triangle_Mesh mesh;
};

struct Model_Storage {
	struct Default_Textures {
		Texture *normal;
		Texture *diffuse;
		Texture *specular;
		Texture *displacement;
		Texture *white;
		Texture *black;
		Texture *green;
	};
	Default_Textures default_textures;

	Array<Texture *> textures;
	Array<Render_Model *> render_models;
	Hash_Table<String_Id, Texture *> textures_table;
	Hash_Table<String_Id, Pair<Render_Model *, u32>> render_models_table;

	Buffer *unified_vertex_buffer = NULL;
	Buffer *unified_index_buffer = NULL;
	Buffer *mesh_instance_buffer = NULL;

	void init();
	void release_all_resources();

	void add_models(Array<Loading_Model *> &models, Array<Pair<Loading_Model *, u32>> &result);
	void upload_models_in_gpu();

	Texture *create_texture_from_file(const char *full_path_to_texture);
	Texture *find_texture_or_get_default(String &texture_file_name, String &mesh_file_name, Texture *default_texture);
};

//inline Mesh_Textures *Model_Storage::get_mesh_textures(u32 index)
//{
//	return &meshes_textures[index];
//}

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

struct Rendering_View {
	Entity_Id camera_id;
	Vector3 position;
	Vector3 direction;
	Matrix4 view_matrix;
	Matrix4 inverse_view_matrix;
	
	void update(Game_World *game_world);
	bool is_entity_camera_set();
};

struct Voxel {
	u32 packed_color;
	u32 packed_normal;
	u32 occlusion;
};

struct Voxel_Grid {
	Size_u32 grid_size;
	Size_u32 ceil_size;

	u32 ceil_count();
	Size_u32 total_size();
};

inline u32 Voxel_Grid::ceil_count()
{
	return grid_size.find_area();
}

inline Size_u32 Voxel_Grid::total_size()
{
	return grid_size * ceil_size;
}

struct Light_Info {
	Entity_Id light_id;
	u32 cascade_shadows_index;
	u32 cascaded_shadows_info_index;
	u32 hlsl_light_index;
};

struct Render_World {
	Game_World *game_world = NULL;
	Render_System *render_sys = NULL;

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

	GPU_Frame_Info frame_info;
	Rendering_View rendering_view;

	Bounding_Sphere world_bounding_sphere;

	Array<Matrix4> render_entity_world_matrices;
	Array<Matrix4> light_view_matrices; // is the code necessary ? 
	Array<Matrix4> cascaded_view_projection_matrices;

	Array<Render_Entity> game_render_entities;

	Array<Cascaded_Shadows> cascaded_shadows_list;
	Array<GPU_Cascaded_Shadows_Info> cascaded_shadows_info_list;
	Array<Shadow_Cascade_Range> shadow_cascade_ranges;
	Array<Light_Info> light_info_list;
	Array<GPU_Light> shader_lights;

	Array<Render_Pass *> frame_render_passes;

	Model_Storage model_storage;

	//Texture2D shadow_atlas;
	//Texture3D jittering_samples;

	//Gpu_Buffer frame_info_cbuffer;

	//Gpu_RWStruct_Buffer voxels_sb;
	//Gpu_Struct_Buffer lights_struct_buffer;
	//Gpu_Struct_Buffer cascaded_shadows_info_sb;
	Buffer *world_matrices_buffer = NULL;
	//Gpu_Struct_Buffer cascaded_view_projection_matrices_sb;


	void init(Engine *engine);
	void init_shadow_rendering();
	void release_all_resources();
	void release_render_entities_resources();

	void update();
	void update_shadows();
	void update_render_entities();
	void update_global_illumination();

	void add_render_entity(Entity_Id entity_id, u32 mesh_idx, void *args = NULL);
	bool add_shadow(Light *light);
	void add_light(Entity_Id light_id);
	void update_light(Light *light);

	u32 delete_render_entity(Entity_Id entity_id);

	void render();

	void set_rendering_view(Entity_Id camera_id);

	bool get_shadow_atls_viewport(Viewport *viewport);

	Vector3 get_light_position(Vector3 light_direction);

	Model_Storage *get_model_storage();
};
#endif

