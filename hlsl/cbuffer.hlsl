#ifndef __CBUFFER__
#define __CBUFFER__

cbuffer Render_2D_Info : register(b0) {
	float4x4 orthographics_matrix;
	float4 primitive_color;
};

cbuffer Primitive_Info {
	float4 color;
	float4x4 world;
	float4x4 world_view_projection;
};

cbuffer per_frame {
	float3 camera_position;
	float3 camera_direction;
};

Texture2D texture_map : register(t0);

SamplerState sampler_anisotropic : register(s0);

#endif