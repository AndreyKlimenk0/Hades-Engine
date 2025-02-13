#ifndef __DRAW_BOX__

#include "mesh.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"

struct Pass_Data {
    uint world_matrix_idx;
    uint mesh_idx;
    uint2 pad;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

StructuredBuffer<float4x4> world_matrices : register(t0, space0);
StructuredBuffer<Mesh_Instance> mesh_instances : register(t1, space0);
StructuredBuffer<Vertex_P3NTUV> unified_vertex_buffer : register(t2, space0);
StructuredBuffer<uint> unified_index_buffer : register(t3, space0);

struct Vertex_Out {
	float4 position : SV_POSITION;
	float3 world_position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
	Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_idx];
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_P3NTUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];
	
	float4x4 world_matrix = transpose(world_matrices[pass_data.world_matrix_idx]);
	
	Vertex_Out vertex_out;
	vertex_out.position = mul(float4(vertex.position, 1.0f), mul(world_matrix, mul(frame_info.view_matrix, frame_info.perspective_matrix))); 
	vertex_out.world_position = mul(float4(vertex.position, 1.0f), world_matrix).xyz;
	vertex_out.normal = mul(vertex.normal, (float3x3)world_matrix);
	vertex_out.tangent = mul(vertex.tangent, (float3x3)world_matrix);
	vertex_out.uv = vertex.uv;
	return vertex_out;
}

float4 ps_main(Vertex_Out output) : SV_TARGET
{
    Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_idx];
    Texture2D<float4> tex = textures[mesh_instance.material.diffuse_texture_index];
    return tex.Sample(linear_sampling, output.uv);
}

#endif