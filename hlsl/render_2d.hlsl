#ifndef __RENDER_2D__
#define __RENDER_2D__

#include "cbuffer.hlsl"


struct Vertex_In {
	float2 position : POSITION;
	float4 color : COLOR;
};

struct Vertex_Out {
	float4 position : SV_POSITION;
	float4 color : COLOR;
};

Vertex_Out vs_main(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = mul(float4(vertex.position, 0.0f, 1.0f), orthographics_matrix);
	result.color = vertex.color;
	result.position.z = 0.0f;
	result.position.w = 1.0f;
	return result;
}

float4 ps_main(Vertex_Out pixel) : SV_TARGET
{
	return primitive_color;
}

#endif