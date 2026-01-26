#ifndef __MESH__
#define __MESH__

struct Material {
    uint normal_texture_index;
    uint albedo_texture_index;
    uint roughness_metalic_texture_index;
};

struct Mesh_Instance {
	uint vertex_count;
	uint index_count;
	uint vertex_offset;
	uint index_offset;
	Material material;
};
#endif