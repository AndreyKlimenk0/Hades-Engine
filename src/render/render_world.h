#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "hlsl.h"
#include "model.h"
#include "render_pass.h"
#include "render_system.h"
#include "render_helpers.h"
#include "../libs/ds/array.h"
#include "../libs/math/common.h"
#include "../libs/math/matrix.h"
#include "../game/world.h"
#include "../libs/color.h"


struct Render_Pass;
typedef u32 Mesh_Idx;
typedef u32 Texture_Idx;
typedef u32 Render_Entity_Idx;

const u32 CASCADE_COUNT = 3;
const u32 SHADOW_ATLAS_SIZE = 8192;
const u32 CASCADE_SIZE = 1024;

const R24U8 DEFAULT_DEPTH_VALUE = R24U8(0xffffff, 0);

struct Render_Entity_Textures {
	Texture_Idx ambient_texture_idx;
	Texture_Idx normal_texture_idx;
	Texture_Idx diffuse_texture_idx;
	Texture_Idx specular_texture_idx;
	Texture_Idx displacement_texture_idx;
};

struct Render_Entity {
	Entity_Id entity_id;
	u32 world_matrix_idx;
	Texture_Idx ambient_texture_idx;
	Texture_Idx normal_texture_idx;
	Texture_Idx diffuse_texture_idx;
	Texture_Idx specular_texture_idx;
	Texture_Idx displacement_texture_idx;
	Mesh_Idx mesh_idx;
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

struct Render_Entity_Texture_Storage {
	Gpu_Device *gpu_device = NULL;
	Render_Pipeline *render_pipeline = NULL;

	Texture_Idx default_texture_idx;
	Texture_Idx default_specular_texture_idx;
	Texture_Idx white_texture_idx;
	Texture_Idx black_texture_idx;
	Texture_Idx green_texture_idx;

	Array<Texture2D> textures;
	Hash_Table<String, Texture_Idx> texture_table;

	void init(Gpu_Device *_gpu_device, Render_Pipeline *_render_pipeline);
	Texture_Idx add_texture(const char *name, u32 width, u32 height, void *data);
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

	Unified_Mesh_Storate<Vector3> line_meshes;
	Unified_Mesh_Storate<Vertex_PNTUV> triangle_meshes;
	Render_Entity_Texture_Storage render_entity_texture_storage;
	
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
	void init_meshes();
	void init_shadow_rendering();
	void init_render_passes(Shader_Manager *shader_manager);

	void update();
	void update_lights();
	void update_shadows();
	void update_render_entities();

	void add_render_entity(Rendering_Type rendering_type, Entity_Id entity_id, Mesh_Idx mesh_idx, Render_Entity_Textures *render_entity_textures, void *args = NULL);
	u32 delete_render_entity(Entity_Id entity_id);
	bool add_shadow(Light *light);

	void render();

	void set_camera_for_rendering(Entity_Id camera_id);
	void set_camera_for_debuging(Entity_Id camera_info_id);
	
	bool get_shadow_atls_viewport(Viewport *viewport);
	bool add_mesh(const char *mesh_name, Mesh<Vertex_PNTUV> *mesh, Mesh_Idx *mesh_idx);
	bool add_mesh(const char *mesh_name, Mesh<Vector3> *mesh, Mesh_Idx *mesh_idx);
	
	Vector3 get_light_position(Vector3 light_direction);
};
#endif
