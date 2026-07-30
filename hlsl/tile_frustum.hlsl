#ifndef __TILE_FRUSTUM__
#define __TILE_FRUSTUM__

#include "globals.hlsl"

struct Pass_Data {
    uint tile_width;
    uint tile_height;
    uint hzb_mip_level;
    uint pad;
    float4x4 cascade_view_projection_matrix;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

static const uint TILE_FRUSTUM_INDICES = 36;

Texture2D<float> hzb_texture : register(t0, space0);

float2 uv_to_ndc(float2 uv)
{
    return float2(uv.x * 2.0f - 1.0f, 1.0f - uv.y * 2.0f);
}

static const float3 points[8] = {
    float3(-1.0, -1.0, 0.0), // 0
    float3( 1.0, -1.0, 0.0), // 1
    float3( 1.0,  1.0, 0.0), // 2
    float3(-1.0,  1.0, 0.0), // 3
    float3(-1.0, -1.0, 1.0), // 4
    float3( 1.0, -1.0, 1.0), // 5
    float3( 1.0,  1.0, 1.0), // 6
    float3(-1.0,  1.0, 1.0)  // 7
};

static const uint indices[36] = {
    0, 2, 1,
    0, 3, 2,
    4, 5, 6,
    4, 6, 7,
    0, 4, 7,
    0, 7, 3,
    1, 2, 6,
    1, 6, 5,
    0, 1, 5,
    0, 5, 4,
    3, 7, 6,
    3, 6, 2
};


float4 vs_main(uint vertex_id : SV_VertexID) : SV_POSITION
{
    uint tile_1d_index = floor(vertex_id / float(TILE_FRUSTUM_INDICES));
    uint2 tile_index = uint2(tile_1d_index % pass_data.tile_width, tile_1d_index / pass_data.tile_width);
    
    float2 half_uv_tile_size = float2(1.0f / (pass_data.tile_width * 2), 1.0f / (pass_data.tile_height * 2));
    float2 uv_tile_center = float2(float(tile_index.x) / float(pass_data.tile_width) + half_uv_tile_size.x, 
                                   float(tile_index.y) / float(pass_data.tile_height) + half_uv_tile_size.y);
    
    uint index = indices[vertex_id % 36];
    float depth = 0.0f;
    if (index > 3) {
        float temp = hzb_texture.SampleLevel(point_sampler(), uv_tile_center, pass_data.hzb_mip_level);
        if (temp < 1.0f) {
             depth = temp;
        }
    }
    float3 position = points[index];
    float x_offset = position.x * half_uv_tile_size.x;
    float y_offset = position.y * half_uv_tile_size.y;
    
    float2 uv_position = uv_tile_center + float2(x_offset, y_offset);
    float3 ndc_position = float3(uv_to_ndc(uv_position), depth);
    
    float4 world_position = mul(float4(ndc_position, 1.0f), frame_info.inverse_view_perspective_matrix);
    world_position /= world_position.w;
    
    return mul(world_position,  pass_data.cascade_view_projection_matrix);
}

#endif