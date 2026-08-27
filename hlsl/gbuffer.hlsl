#ifndef __GBUFFER__
#define __GBUFFER__

#include "utils.hlsl"
#include "mesh.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"

struct Pass_Data {
	uint mesh_instance_idx;
	uint pad0;
	uint pad1;
	uint pad2;
};

struct Vertex_Out {
	float4 position : SV_POSITION;
	float3 world_position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

StructuredBuffer<float4x4> world_matrices : register(t0, space0);
StructuredBuffer<Mesh_Instance> mesh_instances : register(t1, space0);
StructuredBuffer<Vertex_P3N3T3UV> unified_vertex_buffer : register(t2, space0);
StructuredBuffer<uint> unified_index_buffer : register(t3, space0);
StructuredBuffer<Material> materials : register(t5, space0);

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
	Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_instance_idx];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_P3N3T3UV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = world_matrices[mesh_instance.transform_idx];
	
	Vertex_Out vertex_out;
	// Use view_perspective_matrix instead of view_matrix * perspective_matrix because
	// the latter causes z fighting. The same matrix as in the Depth Pass should be used here.
	// A multiplied matrix on GPU is not equal to the multplied matrix on CPU.
	vertex_out.position = mul(float4(vertex.position, 1.0f), mul(world_matrix, frame_info.view_perspective_matrix)); 
	vertex_out.world_position = mul(float4(vertex.position, 1.0f), world_matrix).xyz;
	vertex_out.normal = mul(vertex.normal, (float3x3)world_matrix);
	vertex_out.tangent = mul(vertex.tangent, (float3x3)world_matrix);
	vertex_out.uv = vertex.uv;
	return vertex_out;
}

struct GBuffer {
    float4 albedo_metalness : SV_TARGET0;
    float4 normal           : SV_TARGET1;
    float  roughness        : SV_TARGET2;
};

GBuffer ps_main(Vertex_Out vertex_out)
{
    Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_instance_idx];
    Material material = materials[mesh_instance.material_idx];

    Texture2D<float4> normal_texture = textures[material.normal_texture_index];
    Texture2D<float4> albedo_texture = textures[material.albedo_texture_index];
    Texture2D<float4> roughness_metalic_texture = textures[material.roughness_metalic_texture_index];
    
    float3 normal = normal_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;
    normal = normal_mapping(normal, vertex_out.normal, vertex_out.tangent);
    float3 albedo = albedo_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;
    float3 roughness_metalic = roughness_metalic_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;
    
    GBuffer gbuffer;
    gbuffer.albedo_metalness = float4(albedo, roughness_metalic.b);
    gbuffer.normal = float4(normal, 1.0f);
    //gbuffer.roughness = roughness_metalic.g;
    gbuffer.roughness = vertex_out.position.z;
    return gbuffer;
}

#endif