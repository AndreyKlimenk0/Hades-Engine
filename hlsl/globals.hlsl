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

struct Shadow_Info {
    uint shadow_atlas_size;
    uint shadow_cascade_size;
    uint jittering_sampling_tile_size;
    uint jittering_sampling_filter_size;
    uint jittering_sampling_scaling;
    uint3 pad60;
};

ConstantBuffer<Frame_Info> frame_info : register(b0, space10);
ConstantBuffer<Shadow_Info> shadow_info : register(b1, space10);

Texture2D<float4> textures[] : register(t0, space10);
//Texture2D<float> shadow_atlas : register(t1, space10);
//Texture3D<float2> jittering_samples : register(t2, space10);

SamplerState point_sampling : register(s0, space10);
SamplerState linear_sampling : register(s1, space10);

#endif