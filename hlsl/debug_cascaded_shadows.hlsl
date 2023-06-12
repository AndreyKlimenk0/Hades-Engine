#ifndef __DEBUG_CASCADED_SHADOW__
#define __DEBUG_CASCADED_SHADOW__

#include "utils.hlsl"
#include "light.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"

static const uint CASCADES_COLOR_COUNT = 5;
static const float4 cascades_colors[CASCADES_COLOR_COUNT] = {
    normalize_rgb(255, 0, 0), // red
    normalize_rgb(240, 240, 7), // yellow
    normalize_rgb(27, 245, 7), // green
    normalize_rgb(0, 0, 255), // blue
    normalize_rgb(240, 162, 7), // orange
};

cbuffer Shadow_Atlas_Info : register(b4) {
    uint shadow_atlas_width;
    uint shadow_atlas_height;
    uint shadow_cascade_width;
    uint shadow_cascade_height;
    uint shadow_cascade_count;
    uint3 pad40;
}

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
StructuredBuffer<uint> unified_index_buffer : register(t4);
StructuredBuffer<Vertex_XNUV> unified_vertex_buffer : register(t5);
StructuredBuffer<float4x4> world_matrices : register(t3);
StructuredBuffer<float4x4> shadow_cascade_view_projection_matrices : register(t8);
StructuredBuffer<Light> lights : register(t7);

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
	Mesh_Instance mesh_instance = mesh_instances[mesh_id];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_XNUV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = transpose(world_matrices[world_matrix_id]);
	
	Vertex_Out vertex_out;
	vertex_out.position = mul(float4(vertex.position, 1.0f), mul(world_matrix, mul(view_matrix, perspective_matrix))); 
	vertex_out.world_position = (float3)mul(float4(vertex.position, 1.0f), world_matrix);
	vertex_out.normal = normalize(mul(vertex.normal, (float3x3)world_matrix));
	vertex_out.uv = vertex.uv;
	return vertex_out;
}

float4 ps_main(Vertex_Out vertex_out) : SV_TARGET
{
    float shadow_factor = 0.0f;
    uint shadow_cascade_index = 0;
    [unroll(10)] //@Note: Hard code
    for (; shadow_cascade_index < shadow_cascade_count; shadow_cascade_index++) {
        float4x4 shadow_cascade_view_projection_matrix = transpose(shadow_cascade_view_projection_matrices[shadow_cascade_index]);        
        float4 position_from_cascade_perspective = mul(float4(vertex_out.world_position, 1.0f), shadow_cascade_view_projection_matrix);
        float2 cascaded_ndc_coordinates = calculate_ndc_coordinates(position_from_cascade_perspective);
        
        float2 shadow_atlas_ndc_coordinates;
        shadow_atlas_ndc_coordinates.x = ((cascaded_ndc_coordinates.x * shadow_cascade_width) + (shadow_cascade_width * shadow_cascade_index)) / shadow_atlas_width;
        shadow_atlas_ndc_coordinates.y = ((cascaded_ndc_coordinates.y * shadow_cascade_height) + (shadow_cascade_height * shadow_cascade_index)) / shadow_atlas_height;
        
        float current_depth = position_from_cascade_perspective.z / position_from_cascade_perspective.w;
        float shadow_map_depth = shadow_atlas.Sample(sampler_anisotropic, shadow_atlas_ndc_coordinates.xy);
    
        float bias = 0.005f;
        if ((current_depth >= (shadow_map_depth - bias)) && (current_depth <= (shadow_map_depth + bias))) {
            shadow_factor = 0.4f;
            break;
        }
    }
    float4 light_factor = calculate_light(vertex_out.world_position, vertex_out.normal, get_material(), lights, light_count);
    float4 cascade_color = cascades_colors[CASCADES_COLOR_COUNT % shadow_cascade_index];
    return cascade_color * light_factor * shadow_factor;
}

#endif