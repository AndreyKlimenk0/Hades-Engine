#ifndef __CBUFFER__
#define __CBUFFER__


cbuffer Render_2D_Info : register(b0) {
	float4x4 orthographics_matrix;
	float4 primitive_color;
};

cbuffer Entity_Info : register(b1) {
	float4x4 world_matrix;
	float4x4 wvp_matrix;
};

cbuffer World_Info : register(b2) {
	float3 camera_position;
	int pad1;
	float3 camera_direction;
	int pad2;
	uint light_count;
	float3 pad3;
};

Texture2D<float4> texture_map : register(t0);

SamplerState sampler_anisotropic : register(s0);

#endif