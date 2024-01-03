#ifndef __DEPTH_MAP__
#define __DEPTH_MAP__

#include "globals.hlsl"
#include "vertex.hlsl"

cbuffer Pass_Data : register(b0) {
	uint mesh_idx;
	uint world_matrix_idx;
	uint2 pad30;
	float4x4 view_projection_matrix;
}

StructuredBuffer<Mesh_Instance> mesh_instances : register(t2);
StructuredBuffer<uint> unified_index_buffer : register(t4);
StructuredBuffer<Vertex_XNUV> unified_vertex_buffer : register(t5);
StructuredBuffer<float4x4> world_matrices : register(t3);

float4 vs_main(uint vertex_id : SV_VertexID) : SV_POSITION
{
	Mesh_Instance mesh_instance = mesh_instances[mesh_idx];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_XNUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = transpose(world_matrices[world_matrix_idx]);
	float4x4 wvp_matrix = mul(world_matrix, view_projection_matrix);
	return mul(float4(vertex.position, 1.0f), wvp_matrix);
}

#endif