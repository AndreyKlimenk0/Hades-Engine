#ifndef __RENDER_2D__
#define __RENDER_2D__

#include "globals.hlsl"
#include "vertex.hlsl"


Vertex_XUV_Out vs_main(Vertex_XUV_In vertex)
{
	Vertex_XUV_Out result;
	result.position = mul(float4(vertex.position, 0.0f, 1.0f), orthographics_matrix);
	result.position.z = 0.0f;
	result.position.w = 1.0f;
	result.uv = vertex.uv;
	return result;
}

float4 ps_main(Vertex_XUV_Out pixel) : SV_TARGET
{
	float4 color = primitive_color * texture_map.Sample(sampler_anisotropic, pixel.uv);
	float alpha = texture_map.Sample(sampler_anisotropic, pixel.uv).a;
	if (color.w < 1.0f) {
		alpha = color.w;
	}
	return float4(color.xyz, alpha);
}

#endif