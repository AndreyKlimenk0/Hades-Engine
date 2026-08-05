#ifndef __DEBUG_CASCADED_SHADOW__
#define __DEBUG_CASCADED_SHADOW__

#include "utils.hlsl"
#include "mesh.hlsl"
#include "light.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"
#include "shadows.hlsl"
#include "BRDF.hlsl"
#include "color_conversion.hlsl"

static const uint CASCADES_COLOR_COUNT = 5;
static const float4 cascades_colors[CASCADES_COLOR_COUNT] = {
    normalize_rgb(255, 0, 0),   // red
    normalize_rgb(240, 240, 7), // yellow
    normalize_rgb(27, 245, 7),  // green
    normalize_rgb(0, 0, 255),   // blue
    normalize_rgb(147, 50, 168), // orange
};

struct Pass_Data {
	uint mesh_idx;
	uint3 pad;
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
StructuredBuffer<Material> materials : register(t5, space0);

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
	Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_idx];
	
	uint index = unified_index_buffer[mesh_instance.index_offset + vertex_id];
	Vertex_P3N3T3UV vertex = unified_vertex_buffer[mesh_instance.vertex_offset + index];

	float4x4 world_matrix = world_matrices[mesh_instance.transform_idx];
	
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
	Mesh_Instance mesh_instance = mesh_instances[pass_data.mesh_idx];
    Material material = materials[mesh_instance.material_idx];

    Texture2D<float4> normal_texture = textures[material.normal_texture_index];
    Texture2D<float4> albedo_texture = textures[material.albedo_texture_index];
    Texture2D<float4> roughness_metalic_texture = textures[material.roughness_metalic_texture_index];
    
    float3 local_normal = normal_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;
    //float3 normal = normal_mapping(local_normal, normalize(vertex_out.normal), normalize(vertex_out.tangent));
    float3 normal = normalize(vertex_out.normal);
    float3 roughness_metalic = roughness_metalic_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;
    //float3 albedo = albedo_texture.SampleLevel(linear_sampler(), vertex_out.uv, 0).rgb;
    
    float3 V = normalize(frame_info.view_position - vertex_out.world_position);
    float3 L;
    
    uint shadow_cascade_index = CASCADES_COLOR_COUNT - 1;
    //float shadow_factor = calculate_shadow_factor2(vertex_out.world_position, vertex_out.position.xy, normal, shadow_cascade_index);
    float shadow_factor = calculate_shadow_factor(vertex_out.world_position, vertex_out.position.xy, normal, shadow_cascade_index);
    // if (shadow_factor != 1.0f) {
    //     return float4(shadow_factor, shadow_factor, shadow_factor, 1.0f);
    // }
    
    float3 albedo = cascades_colors[shadow_cascade_index % CASCADES_COLOR_COUNT].xyz;
    //float3 albedo = cascades_colors[0].xyz;
    
    //@Note: hard code
	if (frame_info.light_count == 0) {
		return float4(albedo, 1.0f);
	}
	float3 color = { 0.0f, 0.0f, 0.0f};
    for (uint i = 0; i < frame_info.light_count; i++) {
        Light light = lights[i];
        switch (light.light_type) {
    		case DIRECTIONAL_LIGHT_TYPE:
    		    L = normalize(-light.direction);
    			break;
    	}
        color += cook_torrance_BRDF(normalize(normal), V, L, albedo, roughness_metalic.y, roughness_metalic.z);
        //color += cook_torrance_BRDF(normalize(normal), V, L, SRGB_to_linear(albedo), roughness_metalic.y, roughness_metalic.z);

    }
    color = color * shadow_factor;
    //color = pow(color, 1.0f/ 2.2f);
    color = linear_to_SRGB(color);
    return float4(color, 1.0f);
}
#endif