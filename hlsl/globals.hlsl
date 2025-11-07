#ifndef __GLOBAL__
#define __GLOBAL__

struct Frame_Info {
	float4x4 view_matrix;
	float4x4 perspective_matrix;
	float4x4 orthographics_matrix;
	float3 view_position;
	float near_plane;
	float3 view_direction;
	float far_plane;
	uint light_count;
    uint3 pad50;
};

ConstantBuffer<Frame_Info> frame_info : register(b0, space10);

Texture2D<float4> textures[] : register(t0, space10);

SamplerState point_sampling : register(s0, space10);
SamplerState linear_sampling : register(s1, space10);

#endif