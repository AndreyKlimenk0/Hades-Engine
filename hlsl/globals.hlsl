#ifndef __GLOBAL__
#define __GLOBAL__

// DirectX 11 allows to bind up to 14 constant buffers per pipeline stage. 
// Registers from 0 to 3 reserved for local constant buffers and 
// register from 4 to 13 reserved for global constant buffers.
// If constant buffer register was updated than update constants in hlsl.h file.

cbuffer Render_2D_Info : register(b4) {
	float4x4 orthographics_matrix;
	float4 primitive_color;
};

cbuffer Frame_Info : register(b5) {
	float4x4 view_matrix;
	float4x4 perspective_matrix;
	float4x4 frame_orthographics_matrix;
	float3 camera_position;
	float near_plane;
	float3 camera_direction;
	float far_plane;
	uint light_count;
    uint3 pad50;
};

cbuffer Shadow_Info : register(b6) {
    uint shadow_atlas_size;
    uint shadow_cascade_size;
    uint jittering_sampling_tile_size;
    uint jittering_sampling_filter_size;
    uint jittering_sampling_scaling;
    uint3 pad60;
}

struct Mesh_Instance {
	uint vertex_count;
	uint index_count;
	uint vertex_offset;
	uint index_offset;
};

Texture2D<float4> texture_map : register(t0);
Texture2D<float> shadow_atlas : register(t1);
Texture3D<float2> jittering_samples : register(t2);

Texture2D<float4> ambient_texture : register(t10);
Texture2D<float4> normal_texture : register(t11);
Texture2D<float4> diffuse_texture : register(t12);
Texture2D<float4> specular_texture : register(t13);
Texture2D<float4> displacement_texture : register(t14);

SamplerState point_sampling : register(s0);
SamplerState linear_sampling : register(s1);

#endif