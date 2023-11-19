#ifndef __SHADOWS__
#define __SHADOWS__

#include "utils.hlsl"
#include "light.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"

static const float BIAS = 0.0005f;

struct Cascaded_Shadows_Info {
    float3 light_direction;
    uint shadow_map_start_index;
    uint shadow_map_end_index;
};


StructuredBuffer<float4x4> shadow_cascade_view_projection_matrices : register(t8);
StructuredBuffer<Cascaded_Shadows_Info> cascaded_shadows_info_buffer : register(t9);

static bool is_in_range(float value, float min, float max)
{
    bool result = false;
    if ((min <= value) && (value <= max)) {
        result = true;
    }
    return result;
}

float4 calculate_shadow_factor(float3 world_position, float2 screen_position, float3 normal, out uint cascade_index)
{
    static const uint shadow_cascade_rows = shadow_atlas_size / shadow_cascade_size;
    static const uint shadow_cascade_cols = shadow_atlas_size / shadow_cascade_size;
    static const float shadow_atlas_texel_size = 1.0f / (float)shadow_atlas_size;
    
    cascade_index = 0;
    float shadow_factor = 1.0f;
    uint shadow_cascade_index = 0;
    uint cascaded_shadows_info_stride = 0;    
    uint cascaded_shadows_info_count = 0;
    cascaded_shadows_info_buffer.GetDimensions(cascaded_shadows_info_count, cascaded_shadows_info_stride);
    
    for (uint shadows_index = 0; shadows_index < cascaded_shadows_info_count; shadows_index++) {
        Cascaded_Shadows_Info cascaded_shadows_info = cascaded_shadows_info_buffer[shadows_index];

        float offset = saturate(1.0f - dot(-cascaded_shadows_info.light_direction, normal));
        world_position = world_position + normal + offset;
    
        shadow_cascade_index = cascaded_shadows_info.shadow_map_start_index;
        for (; shadow_cascade_index <= cascaded_shadows_info.shadow_map_end_index; shadow_cascade_index++) {
            float4x4 shadow_cascade_view_projection_matrix = transpose(shadow_cascade_view_projection_matrices[shadow_cascade_index]);        
            float4 position_from_cascade_perspective = mul(float4(world_position, 1.0f), shadow_cascade_view_projection_matrix);
            float3 cascaded_ndc_coordinates = calculate_ndc_coordinates(position_from_cascade_perspective);
    
            float min = 0.1f;
            float max = 0.9f;
            if (is_in_range(cascaded_ndc_coordinates.x, min, max) && is_in_range(cascaded_ndc_coordinates.y, min, max) && is_in_range(cascaded_ndc_coordinates.z, min, max)) {     
                uint shadow_cascade_row_index = shadow_cascade_index % shadow_cascade_rows;
                uint shadow_cascade_col_index = shadow_cascade_index / shadow_cascade_cols;
                float2 shadow_atlas_ndc_coordinates;
                shadow_atlas_ndc_coordinates.x = ((cascaded_ndc_coordinates.x * shadow_cascade_size) + (shadow_cascade_size * shadow_cascade_row_index)) / shadow_atlas_size;
                shadow_atlas_ndc_coordinates.y = ((cascaded_ndc_coordinates.y * shadow_cascade_size) + (shadow_cascade_size * shadow_cascade_col_index)) / shadow_atlas_size;
                
                float current_depth = position_from_cascade_perspective.z / position_from_cascade_perspective.w;
                
                int row_index = (int)fmod(screen_position.x, jittering_sampling_tile_size);
                int depth_index = (int)fmod(screen_position.y, jittering_sampling_tile_size);
                
                float light_illumination = jittering_sampling_filter_size;
                for (uint index = 0; index < jittering_sampling_filter_size; index++) {
                    float2 sampling_offset = jittering_samples.Load(uint4(index, row_index, depth_index, 0));
                    sampling_offset *= jittering_sampling_scaling;
                    sampling_offset *= shadow_atlas_texel_size;
                    float shadow_map_depth = shadow_atlas.SampleLevel(point_sampling, shadow_atlas_ndc_coordinates.xy + sampling_offset, 0);
                    
                    if ((current_depth - BIAS) > shadow_map_depth) {
                        light_illumination -= 1.0f;
                    }
                }
                light_illumination /= jittering_sampling_filter_size;
                
                if ((light_illumination != 0.0f) && (light_illumination != 1.0f)) {
                    
                    uint full_jittering_sampling_filter_size = pow(jittering_sampling_filter_size, 2);
                    light_illumination = full_jittering_sampling_filter_size;
                    for (uint index = 0; index < full_jittering_sampling_filter_size; index++) {
                        float2 sampling_offset = jittering_samples.Load(uint4(index, row_index, depth_index, 0));
                        sampling_offset *= jittering_sampling_scaling;
                        sampling_offset *= shadow_atlas_texel_size;
                        float shadow_map_depth = shadow_atlas.SampleLevel(point_sampling, shadow_atlas_ndc_coordinates.xy + sampling_offset, 0);
                        
                        if ((current_depth - BIAS) > shadow_map_depth) {
                            light_illumination -= 1.0f;
                        }
                    }
                    light_illumination /= full_jittering_sampling_filter_size;
                }
                shadow_factor = light_illumination;
                break;
            }
        }
    }
    cascade_index = shadow_cascade_index;
    return float4(shadow_factor, shadow_factor, shadow_factor, shadow_factor);
}

#endif