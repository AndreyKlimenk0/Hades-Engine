#ifndef GPU_STORAGES_H
#define GPU_STORAGES_H

#include "mesh.h"
#include "render_api\render.h"

#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../libs/structures/hash_table.h"
#include "../libs/str.h"

const String DEFAULT_NORMAL_TEXTURE = "default_normal_texture";
const String DEFAULT_ALBEDO_TEXTURE = "default_albedo_texture";
const String DEFAULT_ROUGHNESS_METALIC_TEXTURE = "default_roughness_metalic_texture";

struct Render_System;

struct Texture_Storage {
	Hash_Table<String, Texture *> textures_table;
	Render_System *render_sys = NULL;
	
	void init(Render_System *_render_sys);
	void create_textures(Array<String> &texture_file_names, const char *textures_subdirectory = NULL);
	Texture *find_texture_or_get_default(const char *texture_name, const char *default_texture_name = NULL);
};

struct Material {
	u32 normal_idx;
	u32 albedo_idx;
	u32 roughness_metalic_idx;
};

struct Material_Storage {
	bool upload_data_to_gpu = false;
	Array<Material> materials;
	Buffer *material_buffer = NULL;
	Render_Device *render_device = NULL;

	void init(Render_Device *_render_device);
	void prepare_for_rendering();
	u32 add_material(u32 normal_texture_idx, u32 albedo_texture_idx, u32 roughness_metalic_texture_idx);
};

struct Mesh_Storage_Info {
	u32 vertex_offset = 0;
	u32 index_offset = 0;
	u32 vertex_count = 0;
	u32 index_count = 0;
};

struct Mesh_Storage {
	bool upload_data_to_gpu = false;
	Buffer *unified_vertex_buffer = NULL;
	Buffer *unified_point_buffer = NULL;
	Buffer *unified_index_buffer = NULL;
	Render_Device *render_device = NULL;

	Array<Vertex_PNTUV> unified_vertices;
	Array<Vector3> unified_points;
	Array<u32> unified_indices;

	Hash_Table<String, Mesh_Storage_Info> mesh_table;

	void init(Render_Device *_render_device);
	void prepare_for_rendering();
	
	Vertex_PNTUV *get_base_vertex(Mesh_Storage_Info *mesh_info);
	Mesh_Storage_Info add_mesh(const char *name, Triangle_Mesh *mesh, Array<Vector3> &mesh_points);
};

#endif
