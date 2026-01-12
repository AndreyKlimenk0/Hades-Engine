#ifndef __RENDER_2D__
#define __RENDER_2D__

#include "globals.hlsl"
#include "vertex.hlsl"

struct Render_2D_Info {
	float4x4 orthographics_matrix;
	float4 primitive_color;
};

struct Vertex_In {
    float2 position : POSITION;
    float2 uv       : TEXCOORD;
};

struct Vertex_Out {
    float4 position : SV_POSITION;
    float2 uv       : TEXCOORD;
};

ConstantBuffer<Render_2D_Info> render_info : register(b0, space0);
Texture2D<float4> texture_map : register(t0, space0);

Vertex_Out vs_main(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = mul(float4(vertex.position, 0.0f, 1.0f), render_info.orthographics_matrix);
	result.position.z = 0.0f;
	result.position.w = 1.0f;
	result.uv = vertex.uv;
	return result;
}

float4 ps_main(Vertex_Out pixel) : SV_TARGET
{
	float4 color = render_info.primitive_color * texture_map.Sample(point_sampler(), pixel.uv);
	float alpha = texture_map.Sample(point_sampler(), pixel.uv).a;
	if (color.w < 1.0f) {
		alpha = color.w;
	}
	return float4(color.xyz, alpha);
}
#endif