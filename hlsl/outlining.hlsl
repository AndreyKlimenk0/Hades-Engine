#ifndef __OUTLINING__
#define __OUTLINING__

#include "utils.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"

Vertex_XNUV_Out vs_main(Vertex_XNUV_In vertex)
{

	Vertex_XNUV_Out result;
	result.position = mul(float4(vertex.position + (vertex.normal * 4.0), 1.0f), view_matrix * perspective_matrix);
	result.normal = vertex.normal;
	result.uv = vertex.uv;
	return result;
}

float4 ps_main(Vertex_XNUV_Out pixel) : SV_Target
{
	return normalize_rgb(201, 131, 50);
}

#endif
