#ifndef __GLOBAL__
#define __GLOBAL__

struct Global_Info {
    uint anisotropic_sampler_idx;
    uint linear_sampler_idx;
    uint point_sampler_idx;
    uint point_clamp_sampler_idx;
};

struct Frame_Info {
	float4x4 view_matrix;
	float4x4 freeze_view_matrix;
	float4x4 perspective_matrix;
	float4x4 orthographics_matrix;
	// Left, Right, Bottom, Top, Near, Far
	float4 frustum_planes[6]; 
	float3 view_position;
	float near_plane;
	float3 view_direction;
	float far_plane;
	uint light_count;
    uint3 pad50;
};

ConstantBuffer<Global_Info> global_info : register(b0, space10);
ConstantBuffer<Frame_Info> frame_info : register(b1, space10);

Texture2D<float4> textures[] : register(t0, space10);
SamplerState samplers[] : register(s0, space10);

SamplerState anisotropic_sampler() 
{ 
    return samplers[global_info.anisotropic_sampler_idx]; 
}

SamplerState linear_sampler() 
{ 
    return samplers[global_info.linear_sampler_idx]; 
}

SamplerState point_sampler() 
{ 
    return samplers[global_info.point_sampler_idx]; 
}

SamplerState point_clamp_sampler()
{
    return samplers[global_info.point_clamp_sampler_idx];
}
#endif