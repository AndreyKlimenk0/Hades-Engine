#ifndef __SILHOUETTE__
#define __SILHOUETTE__

#include "globals.hlsl"
#include "vertex.hlsl"
#include "mesh.hlsl"

struct Pass_Data {
	uint mesh_id;
	uint world_matrix_id;
	uint mesh_buffer_index;
	uint pad22;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

StructuredBuffer<float4x4> world_matrices : register(t0, space0);
StructuredBuffer<Mesh_Instance> mesh_instances : register(t1, space0);
StructuredBuffer<Vertex_P3N3T3UV> unified_vertex_buffer : register(t2, space0);
StructuredBuffer<uint> unified_index_buffer : register(t3, space0);

float4 vs_main(uint vertex_id : SV_VertexID) : SV_POSITION
{
	Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_id];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_P3N3T3UV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = world_matrices[pass_data.world_matrix_id];
	
	return mul(float4(vertex.position, 1.0f), mul(world_matrix, mul(frame_info.view_matrix, frame_info.perspective_matrix))); 
}

uint ps_main(float4 screen_position : SV_POSITION) : SV_TARGET
{
    return pass_data.mesh_buffer_index;
}
#endif