#ifndef __MESH__
#define __MESH__

struct AABB {
    float3 min;
    float3 max;
};

struct Material {
    uint normal_texture_index;
    uint albedo_texture_index;
    uint roughness_metalic_texture_index;
};

struct Mesh_Instance {
	uint vertex_offset;
	uint index_offset;
	uint material_idx;
	uint bounding_box_idx;
	uint transform_idx;
};

#endif