#ifndef __DRAW_LINES__
#define __DRAW_LINES__

#include "globals.hlsl"

StructuredBuffer<float3> unified_vertices : register(t0);
StructuredBuffer<uint> unified_indices : register(t1);
StructuredBuffer<Mesh_Instance> mesh_instances : register(t2);
StructuredBuffer<float4x4> world_matrices : register(t3);

cbuffer Pass_Data : register(b2) {
	uint mesh_id;
	uint world_matrix_id;
	uint pad11;
	uint pad22;
}

float4 vs_main(uint vertex_id : SV_VertexID) : SV_POSITION
{
	Mesh_Instance mesh_instance = mesh_instances[mesh_id];
	uint index = unified_indices[mesh_instance.index_offset + vertex_id];
	float3 position = unified_vertices[mesh_instance.vertex_offset + index];
	
	float4x4 world_matrix = transpose(world_matrices[world_matrix_id]);
	float4x4 result = mul(mul(world_matrix, view_matrix), perspective_matrix);
	return mul(float4(position, 1.0f), result);
}

float4 ps_main(float4 position : SV_POSITION) : SV_TARGET
{
	return float4(1.0f, 0.0f, 0.0f, 1.0f);
}

#endif