#ifndef __SHADOWS__
#define __SHADOWS__

#include "utils.hlsl"
#include "light.hlsl"
#include "vertex.hlsl"
#include "globals.hlsl"

static const float BIAS = 0.005f;

struct Cascaded_Shadows {
    float3 light_direction;
    uint shadow_map_start_index;
    uint shadow_map_end_index;
};

struct Shadow_Atlas {
    uint atlas_size;
    uint cascade_size;
    uint2 pad;
};

struct Jittering_Filter {
    uint tile_size;
    uint filter_size;
    uint scaling;
    uint pad;
};

Texture2D<float> shadow_atlas_texture : register(t0, space2);
Texture3D<float2> jittering_samples : register(t1, space2);

ConstantBuffer<Shadow_Atlas> shadow_atlas : register(b0, space2);
ConstantBuffer<Jittering_Filter> jittering_filter : register(b1, space2);
StructuredBuffer<Cascaded_Shadows> cascaded_shadows_list : register(t2, space2);
StructuredBuffer<float4x4> shadow_cascade_view_projection_matrices : register(t3, space2);

float4 calculate_shadow_factor(float3 world_position, float2 screen_position, float3 normal, out uint cascade_index)
{
    static const uint shadow_cascade_rows = shadow_atlas.atlas_size / shadow_atlas.cascade_size;
    static const uint shadow_cascade_cols = shadow_atlas.atlas_size / shadow_atlas.cascade_size;
    static const float shadow_atlas_texel_size = 1.0f / (float)shadow_atlas.atlas_size;
    
    cascade_index = 0;
    float shadow_factor = 1.0f;
    uint shadow_cascade_index = 0;
    uint cascaded_shadows_stride = 0;    
    uint cascaded_shadows_count = 0;
    cascaded_shadows_list.GetDimensions(cascaded_shadows_count, cascaded_shadows_stride);
    
    for (uint shadows_index = 0; shadows_index < cascaded_shadows_count; shadows_index++) {
        Cascaded_Shadows cascaded_shadows = cascaded_shadows_list[shadows_index];

        float offset = saturate(1.0f - dot(-cascaded_shadows.light_direction, normal));
        world_position = world_position + normal + offset;
    
        shadow_cascade_index = cascaded_shadows.shadow_map_start_index;
        for (; shadow_cascade_index <= cascaded_shadows.shadow_map_end_index; shadow_cascade_index++) {
            float4x4 shadow_cascade_view_projection_matrix = transpose(shadow_cascade_view_projection_matrices[shadow_cascade_index]);        
            float4 position_from_cascade_perspective = mul(float4(world_position, 1.0f), shadow_cascade_view_projection_matrix);
            float3 cascaded_ndc_coordinates = normalize_ndc_coordinates(position_from_cascade_perspective);
    
            float min = 0.1f;
            float max = 0.9f;
            if (in_range(min, max, cascaded_ndc_coordinates.x) && in_range(min, max, cascaded_ndc_coordinates.y) && in_range(min, max, cascaded_ndc_coordinates.z)) {     
                uint shadow_cascade_row_index = shadow_cascade_index % shadow_cascade_rows;
                uint shadow_cascade_col_index = shadow_cascade_index / shadow_cascade_cols;
                float2 shadow_atlas_ndc_coordinates;
                shadow_atlas_ndc_coordinates.x = ((cascaded_ndc_coordinates.x * shadow_atlas.cascade_size) + (shadow_atlas.cascade_size * shadow_cascade_row_index)) / shadow_atlas.atlas_size;
                shadow_atlas_ndc_coordinates.y = ((cascaded_ndc_coordinates.y * shadow_atlas.cascade_size) + (shadow_atlas.cascade_size * shadow_cascade_col_index)) / shadow_atlas.atlas_size;
                
                float current_depth = position_from_cascade_perspective.z / position_from_cascade_perspective.w;
                
                int row_index = (int)fmod(screen_position.x, jittering_filter.tile_size);
                int depth_index = (int)fmod(screen_position.y, jittering_filter.tile_size);
                
                float light_illumination = jittering_filter.filter_size;
                for (uint index = 0; index < jittering_filter.filter_size; index++) {
                    float2 sampling_offset = jittering_samples.Load(uint4(index, row_index, depth_index, 0));
                    sampling_offset *= jittering_filter.scaling;
                    sampling_offset *= shadow_atlas_texel_size;
                    float shadow_map_depth = shadow_atlas_texture.SampleLevel(point_sampling, shadow_atlas_ndc_coordinates.xy + sampling_offset, 0);
                    
                    if ((current_depth - BIAS) > shadow_map_depth) {
                        light_illumination -= 1.0f;
                    }
                }
                light_illumination /= jittering_filter.filter_size;
                
                if ((light_illumination != 0.0f) && (light_illumination != 1.0f)) {
                    
                    uint full_jittering_sampling_filter_size = pow(jittering_filter.filter_size, 2);
                    light_illumination = full_jittering_sampling_filter_size;
                    for (uint index = 0; index < full_jittering_sampling_filter_size; index++) {
                        float2 sampling_offset = jittering_samples.Load(uint4(index, row_index, depth_index, 0));
                        sampling_offset *= jittering_filter.scaling;
                        sampling_offset *= shadow_atlas_texel_size;
                        float shadow_map_depth = shadow_atlas_texture.SampleLevel(point_sampling, shadow_atlas_ndc_coordinates.xy + sampling_offset, 0);
                        
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