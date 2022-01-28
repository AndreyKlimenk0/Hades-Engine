#ifndef __DRAW_TEXTURE__
#define __DRAW_TEXTURE__


#include "vertex.hlsl"
#include "cbuffer.hlsl"

Vertex_XNUV_Out vs_main(Vertex_XNUV_In vertex)
{
	Vertex_XNUV_Out result;
	result.position = float4(vertex.position, 1.0f);
	result.normal = vertex.normal;
	result.uv = vertex.uv;
	return result;
}

float4 ps_main(Vertex_XNUV_Out pixel) : SV_Target
{
	float4 tex = texture_map.Sample(sampler_anisotropic, pixel.uv);
	if (tex.a < 1.0f) {
		discard;
	}
	return tex;
}

#endif