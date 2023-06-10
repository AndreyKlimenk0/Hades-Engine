#ifndef __DEPTH_MAP__
#define __DEPTH_MAP__

#include "cbuffer.hlsl"
#include "vertex.hlsl"

cbuffer Pass_Data : register(b3) {
	uint mesh_id;
	uint world_matrix_id;
	uint light_view_matrix_id;
	uint pad11;
}

cbuffer Shadow_Cascade_Info : register(b4) {
	float4x4 cascade_view_matrix;
	float4x4 cascade_projection_matrix;
}

StructuredBuffer<Mesh_Instance> mesh_instances : register(t2);
StructuredBuffer<float4x4> world_matrices : register(t3);
StructuredBuffer<uint> unified_index_buffer : register(t4);
StructuredBuffer<Vertex_XNUV> unified_vertex_buffer : register(t5);

float4 vs_main(uint vertex_id : SV_VertexID) : SV_POSITION
{
	Mesh_Instance mesh_instance = mesh_instances[mesh_id];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_XNUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = transpose(world_matrices[world_matrix_id]);
	float4x4 wvp_matrix = mul(world_matrix, mul(cascade_view_matrix, cascade_projection_matrix));

	return mul(float4(vertex.position, 1.0f), wvp_matrix);
}

#endif