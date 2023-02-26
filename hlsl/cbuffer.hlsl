#ifndef __CBUFFER__
#define __CBUFFER__


cbuffer Render_2D_Info : register(b0) {
	float4x4 orthographics_matrix;
	float4 primitive_color;
};

cbuffer Frame_Info : register(b1) {
	float4x4 view_matrix;
	float4x4 perspective_matrix;
	float3 camera_position;
	int pad1;
	float3 camera_direction;
	int pad2;
	uint light_count;
	float3 pad3;
};

struct Mesh_Instance {
	uint vertex_count;
	uint index_count;
	uint vertex_offset;
	uint index_offset;
};

Texture2D<float4> texture_map : register(t0);

SamplerState sampler_anisotropic : register(s0);

#endif