#ifndef __CASCADED_SHAODW__
#define __CASCADED_SHADOW__

#include "utils.hlsl"

struct Shadow_Atlas_Info {
    uint width;
    uint height;
    uint cascade_width;
    uint cascade_height;
    uint cascade_count;
};

struct Cascaded_Shadow_Calculating_Info {
    float3 vertex_normal;
    float3 light_direction;
    float4x4 shadow_view_projection_matrix
    Shadow_Atlas_Info shadow_atlas_info;
    SamplerState shadow_atlas_sampler;
    Texture2D<float> shadow_atlas_texture;
}

float calculate_cascaded_shadow_factor(Cascaded_Shadow_Calculating_Info calculating_info)
{
    Shadow_Atlas_Info shadow_atlas_info = calculating_info.shadow_atlas_info;
    SamplerState shadow_atlas_sampler = calculating_info.shadow_atlas_sampler;
    Texture2D<float> shadow_atlas_texture = calculating_info.shadow_atlas_texture;
    
    const uint shadow_cascade_rows = shadow_atlas_info.width / shadow_atlas_info.cascade_width;
    const uint shadow_cascade_cols = shadow_atlas_info.height / shadow_atlas_info.cascade_height;
    
    float shadow_factor = 0.0f;
    uint shadow_cascade_index = 0;
    [unroll(10)] //@Note: Hard code
    for (; shadow_cascade_index < shadow_atlas_info.cascade_count; shadow_cascade_index++) {
        float4 position_from_cascade_perspective = mul(float4(vertex_out.world_position, 1.0f), shadow_cascade_view_projection_matrix);
        float2 cascaded_ndc_coordinates = normalize_ndc_coordinates(position_from_cascade_perspective);
        
        uint shadow_cascade_row_index = shadow_cascade_index % shadow_cascade_rows;
        uint shadow_cascade_col_index = shadow_cascade_index / shadow_cascade_cols;
        float2 shadow_atlas_ndc_coordinates;
        shadow_atlas_ndc_coordinates.x = ((cascaded_ndc_coordinates.x * shadow_cascade_width) + (shadow_cascade_width * shadow_cascade_row_index)) / shadow_atlas_width;
        shadow_atlas_ndc_coordinates.y = ((cascaded_ndc_coordinates.y * shadow_cascade_height) + (shadow_cascade_height * shadow_cascade_col_index)) / shadow_atlas_height;
        
        float current_depth = position_from_cascade_perspective.z / position_from_cascade_perspective.w;
        float shadow_map_depth = shadow_atlas_texture.Sample(shadow_atlas_sampler, shadow_atlas_ndc_coordinates.xy);
    
        float3 light_direction = normalize(-light_direction);
  	    float bias = max(0.5f * (1.0f - dot(light_direction, calculating_info.vertex_normal)), 0.005f);
  	    float max_depth = min(shadow_map_depth + bias, 1.0f);
  	    float min_depth = max(shadow_map_depth - bias, 0.0f);
        if (((min_depth <= current_depth) && (current_depth <= max_depth)) && (max_depth != 1.0f)) {
            shadow_factor = 1.0f;
            break;
        }
    }
    return shadow_factor;
}

#endif