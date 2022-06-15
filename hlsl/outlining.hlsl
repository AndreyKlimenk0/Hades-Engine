#ifndef __OUTLINING__
#define __OUTLINING__

#include "vertex.hlsl"
#include "cbuffer.hlsl"

float4 normalize_rgb(int r, int g, int b)
{
	return float4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}

Vertex_XNUV_Out vs_main(Vertex_XNUV_In vertex)
{

	Vertex_XNUV_Out result;
	result.position = mul(float4(vertex.position + (vertex.normal * 4.0), 1.0f), wvp_matrix);
	result.normal = vertex.normal;
	result.uv = vertex.uv;
	return result;
}

float4 ps_main(Vertex_XNUV_Out pixel) : SV_Target
{
	return normalize_rgb(201, 131, 50);
}

#endif
