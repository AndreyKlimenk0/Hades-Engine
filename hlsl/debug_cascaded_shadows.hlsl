#ifndef __DEBUG_CASCADED_SHADOW__
#define __DEBUG_CASCADED_SHADOW__

#include "utils.hlsl"
#include "light.hlsl"
#include "vertex.hlsl"
#include "shadows.hlsl"
#include "globals.hlsl"

static const uint CASCADES_COLOR_COUNT = 5;
static const float4 cascades_colors[CASCADES_COLOR_COUNT] = {
    normalize_rgb(255, 0, 0),   // red
    normalize_rgb(240, 240, 7), // yellow
    normalize_rgb(27, 245, 7),  // green
    normalize_rgb(0, 0, 255),   // blue
    normalize_rgb(240, 162, 7), // orange
};

cbuffer Pass_Data : register(b0) {
	uint mesh_id;
	uint world_matrix_id;
	uint pad11;
	uint pad22;
}

struct Vertex_Out {
	float4 position : SV_POSITION;
	float3 world_position : POSITION;
	float3 normal : NORMAL;
	float3 tangent : TANGENT;
	float2 uv : TEXCOORD;
};

StructuredBuffer<Mesh_Instance> mesh_instances : register(t2);
StructuredBuffer<uint> unified_index_buffer : register(t4);
StructuredBuffer<Vertex_XNUV> unified_vertex_buffer : register(t5);
StructuredBuffer<float4x4> world_matrices : register(t3);
StructuredBuffer<Light> lights : register(t7);

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
	Mesh_Instance mesh_instance = mesh_instances[mesh_id];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_XNUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = transpose(world_matrices[world_matrix_id]);
	
	Vertex_Out vertex_out;
	vertex_out.position = mul(float4(vertex.position, 1.0f), mul(world_matrix, mul(view_matrix, perspective_matrix))); 
	vertex_out.world_position = mul(float4(vertex.position, 1.0f), world_matrix).xyz;
	vertex_out.normal = mul(vertex.normal, (float3x3)world_matrix);
	vertex_out.tangent = mul(vertex.tangent, (float3x3)world_matrix);
	vertex_out.uv = vertex.uv;
	return vertex_out;
}

float4 ps_main(Vertex_Out vertex_out) : SV_TARGET
{
    Material material;
    material.ambient = ambient_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    material.diffuse = diffuse_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    material.specular = specular_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    float3 temp = displacement_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    
    float3 normal_sample = normal_texture.Sample(linear_sampling, vertex_out.uv).rgb;
    float3 normal = normal_mapping(normal_sample, vertex_out.normal, vertex_out.tangent);
    
    uint shadow_cascade_index;
    float4 shadow_factor = calculate_shadow_factor(vertex_out.world_position, vertex_out.position.xy, normal, shadow_cascade_index);
    float3 light_factor = calculate_light(vertex_out.world_position, normal, material, light_count, lights);
    float4 cascade_color = cascades_colors[shadow_cascade_index % CASCADES_COLOR_COUNT];
    return cascade_color * float4(light_factor, 1.0f) * shadow_factor;
}

#endif