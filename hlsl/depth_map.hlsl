#ifndef __DEPTH_MAP__
#define __DEPTH_MAP__

#include "cbuffer.hlsl"
#include "vertex.hlsl"


cbuffer Pass_Data : register(b2) {
	uint mesh_id;
	uint world_matrix_id;
	uint pad11;
	uint pad22;
}

struct Vertex_Out {
	float4 screen_position : SV_POSITION;
	float3 world_position : POSITION; //@Note: May be better pass only z
};

StructuredBuffer<Mesh_Instance> mesh_instances : register(t2);
StructuredBuffer<float4x4> world_matrices : register(t3);
StructuredBuffer<uint> unified_index_buffer : register(t4);
StructuredBuffer<Vertex_XNUV> unified_vertex_buffer : register(t5);

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
	Mesh_Instance mesh_instance = mesh_instances[mesh_id];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_XNUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = transpose(world_matrices[world_matrix_id]);
	float4x4 wvp_matrix = mul(mul(world_matrix, view_matrix), frame_orthographics_matrix);

	Vertex_Out vertex_out;
	vertex_out.screen_position = mul(float4(vertex.position, 1.0f), wvp_matrix);
	vertex_out.world_position = mul(float4(vertex.position, 1.0f), world_matrix).xyz;
	return vertex_out;
}
#endif