#ifndef __FORWARD_LIGHT__
#define __FORWRAD_LIGHT__

#include "light.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"

struct Shadow_Map {
	uint light_view_matrix_idx;
};

cbuffer Pass_Data : register(b2) {
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
StructuredBuffer<Shadow_Map> shadow_maps : register(ps, t6);
StructuredBuffer<float4x4> light_view_matrices : register(ps, t8);

static const uint value = 4;
float2 poissonDisk[value] = {
	float2( -0.94201624, -0.39906216 ),
	float2( 0.94558609, -0.76890725 ),
  	float2( -0.094184101, -0.92938870 ),
  	float2( 0.34495938, 0.29387760 )
};

float calculate_shadow(Light light, float3 normal, float3 world_position, uint light_view_matrix_idx)
{
	float4x4 light_view_matrix = transpose(light_view_matrices[light_view_matrix_idx]);
	
	float4 position_from_light_perspective = mul(float4(world_position, 1.0f), mul(light_view_matrix, direction_light_matrix));

	float2 NDC_coordinates;
	
	NDC_coordinates.x = 0.5f + ((position_from_light_perspective.x / position_from_light_perspective.w) * 0.5f);
	NDC_coordinates.y = 0.5f - ((position_from_light_perspective.y / position_from_light_perspective.w) * 0.5f);

	float currect_depth = position_from_light_perspective.z /  position_from_light_perspective.w;
	
	float bias = max(0.05f * (1.0f - dot(-light.direction, normal)), 0.0005f);

//	for(uint i = 0; i < 4; i++) {
		//float2 temp = float2(NDC_coordinates.x + poissonDisk[i] / 700.0f,NDC_coordinates.y + poissonDisk[i] / 700.0f);
//		float shadow_map_depth = shadow_atlas.Sample(sampler_anisotropic, NDC_coordinates.xy + poissonDisk[i] / 700.0f);
//		if ((currect_depth - bias)  > shadow_map_depth) {
//			shadow_factor -= 0.25f;
//		}
//	}


	float2 texture_size;
	shadow_atlas.GetDimensions(texture_size.x, texture_size.y);

	const float2 texel_size = { 1.0f / texture_size.x, 1.0f / texture_size.y };

	float shadow_factor = 0.0f;
	for(int x = 0; x < 1; x++) {
		for(int y = 0; y < 1; y++) {
			float2 offset = float2(x, y) * texel_size;
			float shadow_map_depth = shadow_atlas.Sample(sampler_anisotropic, NDC_coordinates.xy + offset);
			if ((currect_depth - bias)  < shadow_map_depth) {
				shadow_factor += 1.0f;
			}
		}
	}
	return (shadow_factor / 1.0f);
}

float4 ps_main(Vertex_Out vertex_out) : SV_Target
{
	float4 texel = texture_map.Sample(sampler_anisotropic, vertex_out.uv);

	if (light_count == 0) {
		return texel;
	}
    
    float4 light_factor = calculate_light(vertex_out.world_position, vertex_out.normal, get_material(), lights, light_count);
    
    return texel * light_factor;
}

#endif