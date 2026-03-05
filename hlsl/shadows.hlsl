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

static const float2 poissonDisk[24] = {
	float2(0.5713538f, 0.7814451f),
	float2(0.2306823f, 0.6228884f),
	float2(0.1000122f, 0.9680607f),
	float2(0.947788f, 0.2773731f),
	float2(0.2837818f, 0.303393f),
	float2(0.6001099f, 0.4147638f),
	float2(-0.2314563f, 0.5434746f),
	float2(-0.08173513f, 0.0796717f),
	float2(-0.4692954f, 0.8651238f),
	float2(0.2768489f, -0.3682062f),
	float2(-0.5900795f, 0.3607553f),
	float2(-0.1010569f, -0.5284956f),
	float2(-0.4741178f, -0.2713854f),
	float2(0.4067073f, -0.00782522f),
	float2(-0.4603044f, 0.0511527f),
	float2(0.9820454f, -0.1295522f),
	float2(0.8187376f, -0.4105208f),
	float2(-0.8115796f, -0.106716f),
	float2(-0.4698426f, -0.6179109f),
	float2(-0.8402727f, -0.4400948f),
	float2(-0.2302377f, -0.879307f),
	float2(0.2748472f, -0.708988f),
	float2(-0.7874522f, 0.6162704f),
	float2(-0.9310728f, 0.3289311f)
};
#define TOTAL_SAMPLES 20

float poisson_disk_shadow_sampling(float4 shadow_perspective_position, float2 shadow_uv, float bias)
{	
    static const float shadow_atlas_texel_size = 1.0f / (float)shadow_atlas.atlas_size;
	// Get the current depth stored in the shadow map
	float4 samples[TOTAL_SAMPLES]; 

	float shadow = TOTAL_SAMPLES;
	float sampleDiscSize = 1.7f;
	float2 pixelSize = float2(shadow_atlas_texel_size, shadow_atlas_texel_size) * sampleDiscSize;

	// Sample the texture at various offsets
	float current_depth = shadow_perspective_position.z / shadow_perspective_position.w;

	[unroll]
	for (int i = 0; i < TOTAL_SAMPLES; i++)
	{
		//samples[i] = tex2D(smp, texCoord + poissonDisk[i] * pixelSize).r > ourdepth;
		float shadow_map_depth = shadow_atlas_texture.SampleLevel(point_sampler(), shadow_uv + poissonDisk[i] * pixelSize, 0);
		if ((current_depth - bias) > shadow_map_depth) {
		    shadow -= 1.0f;
		}
	}

	shadow /= (TOTAL_SAMPLES + 1);
	return shadow;
}

float2 cascade_uv_to_shadow_atlas_uv(float2 cascade_ndc_coordinates, uint shadow_cascade_index)
{
    uint shadow_cascade_rows = shadow_atlas.atlas_size / shadow_atlas.cascade_size;
    uint shadow_cascade_cols = shadow_atlas.atlas_size / shadow_atlas.cascade_size;
    uint shadow_cascade_row_index = shadow_cascade_index % shadow_cascade_rows;
    uint shadow_cascade_col_index = shadow_cascade_index / shadow_cascade_cols;
    float2 shadow_atlas_uv_coordinates;
    shadow_atlas_uv_coordinates.x = ((cascade_ndc_coordinates.x * (shadow_atlas.cascade_size - 1)) + (shadow_atlas.cascade_size * shadow_cascade_row_index)) / shadow_atlas.atlas_size;
    shadow_atlas_uv_coordinates.y = ((cascade_ndc_coordinates.y * (shadow_atlas.cascade_size - 1)) + (shadow_atlas.cascade_size * shadow_cascade_col_index)) / shadow_atlas.atlas_size;
    return shadow_atlas_uv_coordinates;
}

float jittering_shadow_sampling(float4 shadow_perspective_position, float2 shadow_uv, float4 cascade_uv_borders, float2 screen_position, float bias, uint cascade_index, uint cascade_count)
{
    float cascade_scaling_factor = (float)jittering_filter.scaling / (float)cascade_count;

    static const float shadow_atlas_texel_size = 1.0f / (float)shadow_atlas.atlas_size;
    float current_depth = shadow_perspective_position.z / shadow_perspective_position.w;

    int row_index = (int)fmod(screen_position.x, jittering_filter.tile_size);
    int depth_index = (int)fmod(screen_position.y, jittering_filter.tile_size);
    
    float light_illumination = jittering_filter.filter_size;
    for (uint index = 0; index < jittering_filter.filter_size; index++) {
        float2 sampling_offset = jittering_samples.Load(uint4(index, row_index, depth_index, 0));
        // A larger cascade makes samples more distributed, which causes softer and weaker shadows. 
        // To fix this, decrease the scaling value by the cascade index factor.
        sampling_offset *= (float)jittering_filter.scaling - cascade_scaling_factor * (float)cascade_index;
        sampling_offset *= shadow_atlas_texel_size;
        float2 clamp_uv = clamp(shadow_uv.xy + sampling_offset, cascade_uv_borders.xy, cascade_uv_borders.zw);
        float shadow_map_depth = shadow_atlas_texture.SampleLevel(point_sampler(), clamp_uv, 0);
        
        if ((current_depth - bias) > shadow_map_depth) {
            light_illumination -= 1.0f;
        }
    }
    light_illumination /= jittering_filter.filter_size;
    
    if ((light_illumination != 0.0f) && (light_illumination != 1.0f)) {
        uint full_jittering_sampling_filter_size = pow(jittering_filter.filter_size, 2);
        light_illumination = full_jittering_sampling_filter_size;
        for (uint index = 0; index < full_jittering_sampling_filter_size; index++) {
            float2 sampling_offset = jittering_samples.Load(uint4(index, row_index, depth_index, 0));
            sampling_offset *= (float)jittering_filter.scaling - cascade_scaling_factor * (float)cascade_index;
            sampling_offset *= shadow_atlas_texel_size;
            float2 clamp_uv = clamp(shadow_uv.xy + sampling_offset, cascade_uv_borders.xy, cascade_uv_borders.zw);
            float shadow_map_depth = shadow_atlas_texture.SampleLevel(point_sampler(), clamp_uv, 0);

            if ((current_depth - bias) > shadow_map_depth) {
                light_illumination -= 1.0f;
            }
        }
        light_illumination /= full_jittering_sampling_filter_size;
    }
   return light_illumination;
}

#define JITTERING_SHADOW_SAMPLING
//#define POISSON_DISC_SHADOW_SAMPLING
//#define HARD_SHADOW_SAMPLING

float4 shadow_cascade_uv_borders(uint cascade_index)
{
    float4 uv;
    uv.xy = cascade_uv_to_shadow_atlas_uv(float2(0.0f, 0.0f), cascade_index);
    uv.zw = cascade_uv_to_shadow_atlas_uv(float2(1.0f, 1.0f), cascade_index);
    return uv;
}

float shadow_sampling(float4 shadow_perspective_position, float2 shadow_uv, float2 screen_position, float bias, uint cascade_index, uint cascade_count)
{
    float shadow_factor = 1.0f;
    float4 cascade_uv_borders = shadow_cascade_uv_borders(cascade_index);
#ifdef JITTERING_SHADOW_SAMPLING
    shadow_factor = jittering_shadow_sampling(shadow_perspective_position, shadow_uv, cascade_uv_borders, screen_position, bias, cascade_index, cascade_count);
#endif
#ifdef POISSON_DISC_SHADOW_SAMPLING
    shadow_factor = poisson_disk_shadow_sampling(shadow_perspective_position, shadow_uv, bias);
#endif
#ifdef HARD_SHADOW_SAMPLING
    float current_depth = shadow_perspective_position.z / shadow_perspective_position.w;
    float shadow_map_depth = shadow_atlas_texture.SampleLevel(point_sampler(), shadow_uv, 0);
    if ((current_depth - bias) > shadow_map_depth) {
        shadow_factor = 0.0f;
    }
#endif
    return shadow_factor;
}

static const float2 bias_table[4] = {
    {0.0003f, 0.0005f},
    {0.0005f, 0.00099f},
    {0.0009f, 0.003f},
    {0.005f, 0.009f},
};

float calculate_shadow_factor(float3 world_position, float2 screen_position, float3 normal, out uint cascade_index)
{
    static const float shadow_atlas_texel_size = 1.0f / (float)shadow_atlas.atlas_size;

    uint shadow_cascade_index = 0;
    uint cascaded_shadows_stride = 0;    
    uint cascaded_shadows_count = 0;
    float shadow_factor = 1.0f;
    cascaded_shadows_list.GetDimensions(cascaded_shadows_count, cascaded_shadows_stride);
    
    for (uint shadows_index = 0; shadows_index < cascaded_shadows_count; shadows_index++) {
        Cascaded_Shadows cascaded_shadows = cascaded_shadows_list[shadows_index];
        //world_position = world_position + normal;
        
        shadow_cascade_index = cascaded_shadows.shadow_map_start_index;
        uint cascade_local_index = 0;
        for (; shadow_cascade_index <= cascaded_shadows.shadow_map_end_index; shadow_cascade_index++) {
            uint cascade_count = cascaded_shadows.shadow_map_end_index + 1 - cascaded_shadows.shadow_map_start_index;
            float4 position_from_cascade_perspective = mul(float4(world_position, 1.0f), shadow_cascade_view_projection_matrices[shadow_cascade_index]);
            float3 cascaded_uv_coordinates = clip_to_uv_coordinates(position_from_cascade_perspective);

            if (saturated(cascaded_uv_coordinates)) {
                float2 shadow_atlas_uv_coordinates = cascade_uv_to_shadow_atlas_uv(cascaded_uv_coordinates.xy, shadow_cascade_index);
                float max_bias = bias_table[cascade_local_index].y;
                float min_bias = bias_table[cascade_local_index].x;
                float bias = max(max_bias * (1.0 - dot(-cascaded_shadows.light_direction, normal)), min_bias);
                //float bias = 0.0f;
                float first_shadow_sample = shadow_sampling(position_from_cascade_perspective, shadow_atlas_uv_coordinates, screen_position, bias, cascade_local_index, cascade_count);
                
                cascade_index = shadow_cascade_index;
                float blending_threshold = max2(abs(position_from_cascade_perspective.xy / position_from_cascade_perspective.w));
                if ((blending_threshold >= 0.8f) && (shadow_cascade_index < cascaded_shadows.shadow_map_end_index)) {
                     position_from_cascade_perspective = mul(float4(world_position, 1.0f), shadow_cascade_view_projection_matrices[shadow_cascade_index + 1]);
                     cascaded_uv_coordinates = clip_to_uv_coordinates(position_from_cascade_perspective);
                     shadow_atlas_uv_coordinates = cascade_uv_to_shadow_atlas_uv(cascaded_uv_coordinates.xy, shadow_cascade_index + 1);
                     max_bias = bias_table[cascade_local_index + 1].y;
                     min_bias = bias_table[cascade_local_index + 1].x;
                     bias = max(max_bias * (1.0 - dot(-cascaded_shadows.light_direction, normal)), min_bias);
                     float second_shadow_sample = shadow_sampling(position_from_cascade_perspective, shadow_atlas_uv_coordinates, screen_position, bias, cascade_local_index + 1, cascade_count);
                     
                     float x = saturate(blending_threshold - 0.8f) * 5.0f;
                     shadow_factor = lerp(first_shadow_sample, second_shadow_sample, x);
                     break;
                }
                shadow_factor = first_shadow_sample;
                break;
            }
            cascade_local_index++;
        }
    }
    return shadow_factor;
}
#endif