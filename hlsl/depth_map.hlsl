#ifndef __DEPTH_MAP__
#define __DEPTH_MAP__

#include "mesh.hlsl"
#include "vertex.hlsl"

struct Pass_Data {
	uint mesh_instance_idx;
	uint3 pad;
	float4x4 view_projection_matrix;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

StructuredBuffer<float4x4> world_matrices : register(t0, space0);
StructuredBuffer<Mesh_Instance> mesh_instances : register(t1, space0);
StructuredBuffer<float3> unified_point_buffer : register(t4, space0);
StructuredBuffer<uint> unified_index_buffer : register(t3, space0);

float4 vs_main(uint vertex_id : SV_VertexID) : SV_POSITION
{
	Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_instance_idx];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	float3 position = unified_point_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = world_matrices[mesh_instance.transform_idx];
	float4x4 wvp_matrix = mul(world_matrix, pass_data.view_projection_matrix);
	return mul(float4(position, 1.0f), wvp_matrix);
}

#endif