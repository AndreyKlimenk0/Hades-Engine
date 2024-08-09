#ifndef __DRAW_VERTICES__
#define __DRAW_VERTICES__

#include "globals.hlsl"

cbuffer Draw_Info : register(b0) {
    float3 mesh_color;
    float mesh_transparency;
	float4x4 world_matrix;
}

struct Vertex_In {
    float3 position : POSITION;
};

struct Vertex_Out {
    float4 position : SV_POSITION;
};

Vertex_Out vs_main(Vertex_In vertex_in)
{
    Vertex_Out vertex_out;
	vertex_out.position = mul(float4(vertex_in.position, 1.0f), mul(world_matrix, mul(view_matrix, perspective_matrix))); 
	return vertex_out;
}

float4 ps_main(float4 screen_position : SV_POSITION) : SV_TARGET
{
    return float4(mesh_color, mesh_transparency);
}

#endif