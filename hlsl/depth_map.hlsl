#ifndef __DEPTH_MAP__
#define __DEPTH_MAP__

#include "mesh.hlsl"
#include "vertex.hlsl"

struct Pass_Data {
	uint mesh_idx;
	uint world_matrix_idx;
	uint2 pad30;
	float4x4 view_projection_matrix;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

StructuredBuffer<float4x4> world_matrices : register(t0, space0);
StructuredBuffer<Mesh_Instance> mesh_instances : register(t1, space0);
StructuredBuffer<Vertex_P3NTUV> unified_vertex_buffer : register(t2, space0);
StructuredBuffer<uint> unified_index_buffer : register(t3, space0);

float4 vs_main(uint vertex_id : SV_VertexID) : SV_POSITION
{
	Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_idx];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_P3NTUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = transpose(world_matrices[pass_data.world_matrix_idx]);
	float4x4 wvp_matrix = mul(world_matrix, pass_data.view_projection_matrix);
	return mul(float4(vertex.position, 1.0f), wvp_matrix);
}

#endif