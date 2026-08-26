#ifndef __DEFERRED_LIGHT__
#define __DEFERRED_LIGHT__

#include "BRDF.hlsl"
#include "light.hlsl"
#include "utils.hlsl"
#include "shadows.hlsl"
#include "globals.hlsl"
#include "color_conversion.hlsl"
#include "full_screen_triangle.hlsl"

struct Pass_Data {
    uint albedo_metalness_idx;
    uint normal_idx;
    uint roughness_idx;
    uint depth_idx;
    float4 sky_color;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);
StructuredBuffer<Light> lights : register(t4, space0);

float4 ps_main(Vertex_Out vertex_out) : SV_TARGET
{
    Texture2D<float4> albedo_metalness_texture = ResourceDescriptorHeap[pass_data.albedo_metalness_idx];
    Texture2D<float4> normal_texture = ResourceDescriptorHeap[pass_data.normal_idx];
    Texture2D<float>  roughness_texture = ResourceDescriptorHeap[pass_data.roughness_idx];
    Texture2D<float>  depth_texture = ResourceDescriptorHeap[pass_data.depth_idx];
    
    float  depth = depth_texture.Sample(point_sampler(), vertex_out.uv);
    if (depth == global_info.max_depth) {
        return pass_data.sky_color;
    }
    
    float4 albedo_metalness = albedo_metalness_texture.Sample(point_sampler(), vertex_out.uv);
    float3 normal = normal_texture.Sample(point_sampler(), vertex_out.uv).rgb;
    float  roughness = roughness_texture.Sample(point_sampler(), vertex_out.uv);
    float3 albedo = albedo_metalness.rgb;
    float  metalness = albedo_metalness.a;
    
    float4 clip_space_position = float4(uv_to_ndc(vertex_out.uv), depth, 1.0f);
    clip_space_position = mul(clip_space_position, frame_info.inverse_view_perspective_matrix);
    float3 world_position = clip_space_position.xyz / clip_space_position.w;
    
    float3 view_direction = normalize(frame_info.view_position - world_position);
    
    float3 final_color = { 0.0f, 0.0f, 0.0f};
    float3 light_direction = { 0.0f, 0.0f, 0.0f };
    for (uint i = 0; i < frame_info.light_count; i++) {
        Light light = lights[i];
        switch (light.light_type) {
    		case DIRECTIONAL_LIGHT_TYPE:
    		    light_direction = normalize(-light.direction);
    			break;
    	}
        final_color += cook_torrance_BRDF(normal, view_direction, light_direction, SRGB_to_linear(albedo), roughness, metalness);
    }
    uint shadow_cascade_index;
    float4 shadow_factor = calculate_shadow_factor(world_position, vertex_out.position.xy, normal, shadow_cascade_index);
    final_color *= shadow_factor.xyz;
    final_color = linear_to_SRGB(final_color);
    return float4(final_color, 1.0f);
}

#endif