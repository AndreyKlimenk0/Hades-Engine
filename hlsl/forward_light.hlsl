#ifndef __FORWARD_LIGHT__
#define __FORWRAD_LIGHT__

#include "utils.hlsl"
#include "mesh.hlsl"
#include "light.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"
#include "shadows.hlsl"

struct Pass_Data {
	uint mesh_idx;
	uint world_matrix_idx;
	uint pad11;
	uint pad22;
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
StructuredBuffer<Light> lights : register(t4, space0);

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
	Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_idx];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_P3N3T3UV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = transpose(world_matrices[pass_data.world_matrix_idx]);
	
	Vertex_Out vertex_out;
	vertex_out.position = mul(float4(vertex.position, 1.0f), mul(world_matrix, mul(frame_info.view_matrix, frame_info.perspective_matrix))); 
	vertex_out.world_position = mul(float4(vertex.position, 1.0f), world_matrix).xyz;
	vertex_out.normal = mul(vertex.normal, (float3x3)world_matrix);
	vertex_out.tangent = mul(vertex.tangent, (float3x3)world_matrix);
	vertex_out.uv = vertex.uv;
	return vertex_out;
}

float4 ps_main(Vertex_Out vertex_out) : SV_Target
{    
    Material material = mesh_instances[pass_data.mesh_idx].material;
    Texture2D<float4> normal_texture = textures[material.normal_texture_index];
    Texture2D<float4> diffuse_texture = textures[material.diffuse_texture_index];
    Texture2D<float4> specular_texture = textures[material.specular_texture_index];
    
    float3 local_normal = normal_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;
    float3 normal = normal_mapping(local_normal, vertex_out.normal, vertex_out.tangent);
    float3 diffuse = diffuse_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;
    float3 specular = specular_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;

    
    //@Note: hard code
	if (frame_info.light_count == 0) {
		return float4(diffuse, 1.0f);
	}
    uint shadow_cascade_index;
    float4 shadow_factor = calculate_shadow_factor(vertex_out.world_position, vertex_out.position.xy, normal, shadow_cascade_index);       
    float3 light_factor = calculate_light(vertex_out.world_position, frame_info.view_position, normal, diffuse, specular, frame_info.light_count, lights);
    return float4(light_factor, 1.0f) * shadow_factor;
}

#endif