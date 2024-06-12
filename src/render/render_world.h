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

Render_Entity *find_render_entity(Array<Render_Entity> *render_entities, Entity_Id entity_id, u32 *index = NULL);

struct Mesh_Textures {
	Texture_Idx normal_idx;
	Texture_Idx diffuse_idx;
	Texture_Idx specular_idx;
	Texture_Idx displacement_idx;
};

template <typename T>
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
	Array<T> unified_vertices;
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
	void allocate_gpu_memory();
	bool add_mesh(const char *mesh_name, Mesh<T> *mesh, Mesh_Id *mesh_id);
	bool add_texture(const char *texture_name, Texture_Idx *texture_idx);
	bool update_mesh(Mesh_Id mesh_id, Mesh<T> *mesh);
	Texture_Idx find_texture_or_get_default(String &texture_file_name, Texture_Idx default_texture);

	Mesh_Textures *get_mesh_textures(u32 index);
	Texture2D *get_texture(Texture_Idx texture_idx);
};

template<typename T>
inline Mesh_Textures *Mesh_Storate<T>::get_mesh_textures(u32 index)
{
	return &meshes_textures[index];
}

template<typename T>
inline Texture2D *Mesh_Storate<T>::get_texture(Texture_Idx texture_idx)
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

	Array<Render_Entity> game_render_entities;
	Array<Render_Entity> line_render_entities;
	Array<Render_Entity> vertex_render_entities;
	Array<Color> line_render_entity_colors;
	Array<Color> vertex_render_entity_colors;

	Array<Cascaded_Shadows> cascaded_shadows_list;
	Array<Cascaded_Shadows_Info> cascaded_shadows_info_list;
	Array<Shadow_Cascade_Range> shadow_cascade_ranges;

	Array<Render_Pass *> every_frame_render_passes;

	Mesh_Storate<Vector3> line_meshes;
	Mesh_Storate<Vertex_PNTUV> triangle_meshes;

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
		Outlining_Pass outlining;

		void get_all_passes(Array<Render_Pass *> *render_passes_list);
	} render_passes;

	void init(Engine *engine);
	void init_shadow_rendering();
	void init_render_passes(Shader_Manager *shader_manager);

	void update();
	void update_lights();
	void update_shadows();
	void update_render_entities();

	void add_render_entity(Rendering_Type rendering_type, Entity_Id entity_id, Mesh_Id mesh_id, void *args = NULL);
	bool add_shadow(Light *light);

	u32 delete_render_entity(Entity_Id entity_id);

	void render();

	void set_camera_for_rendering(Entity_Id camera_id);
	void set_camera_for_debuging(Entity_Id camera_info_id);

	bool get_shadow_atls_viewport(Viewport *viewport);
	bool add_mesh(const char *mesh_name, Mesh<Vertex_PNTUV> *mesh, Mesh_Id *mesh_id);
	bool add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Id *mesh_id);

	Vector3 get_light_position(Vector3 light_direction);
};
#endif

