#ifndef __FORWARD_LIGHT__
#define __FORWRAD_LIGHT__

#include "light.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"
#include "shadows.hlsl"

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
	float2 uv : TEXCOORD;
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
	
	Vertex_Out vertex_out;
	vertex_out.position = mul(float4(vertex.position, 1.0f), mul(world_matrix, mul(view_matrix, perspective_matrix))); 
	vertex_out.world_position = mul(float4(vertex.position, 1.0f), world_matrix);
	vertex_out.normal = normalize(mul(vertex.normal, (float3x3)world_matrix));
	vertex_out.uv = vertex.uv;
	return vertex_out;
}

StructuredBuffer<Light> lights : register(t7);

float4 ps_main(Vertex_Out vertex_out) : SV_Target
{
	float4 texel = texture_map.Sample(linear_sampling, vertex_out.uv);
	if (light_count == 0) {
		return texel;
	}
    uint shadow_cascade_index;
    float4 shadow_factor = calculate_shadow_factor(vertex_out.world_position, vertex_out.position.xy, vertex_out.normal, shadow_cascade_index);    
    float4 light_factor = calculate_light(vertex_out.world_position, vertex_out.normal, get_material(), lights, light_count);
    
    return texel * light_factor * shadow_factor;
}

#endif