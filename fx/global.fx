cbuffer per_entity{ 
	float4x4 world_view_projection;
	float4x4 world;
	float3 model_color;
};

cbuffer per_frame {
	float3 camera_position;
	float3 camera_direction;
};

Texture2D texture_map;

SamplerState sampler_anisotropic {
	Filter = ANISOTROPIC;
	MaxAnisotropy = 4;

	AddressU = WRAP;
	AddressV = WRAP;
};