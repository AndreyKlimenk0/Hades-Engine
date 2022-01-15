cbuffer per_entity{ 
	float4x4 world;
	float4x4 world_view_projection;
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


float4 normalize_rgb(int r, int g, int b)
{
	return float4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}