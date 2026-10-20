#ifndef __DOWNSAMPLE_HZB_H__
#define __DOWNSAMPLE_HZB_H__

#include "globals.hlsl"
#include "utils.hlsl"

struct Pass_Data {
    uint number_mip_levels;
    float2 texel_size;
    uint pad;
};

Texture2D<float2> source_mip : register(t0, space0); // Store min/x max/y depth
RWTexture2D<float2> output_mip0 : register(u0, space0);
RWTexture2D<float2> output_mip1 : register(u1, space0);
RWTexture2D<float2> output_mip2 : register(u2, space0);
RWTexture2D<float2> output_mip3 : register(u3, space0);

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

groupshared float depth_min_storage[64];
groupshared float depth_max_storage[64];

void store_depth(uint index, float min_depth, float max_depth)
{
    depth_min_storage[index] = min_depth;
    depth_max_storage[index] = max_depth;
}

float2 load_depth(uint index)
{
    return float2(depth_min_storage[index], depth_max_storage[index]);
}

[numthreads(8, 8, 1)]
void cs_main(uint group_index : SV_GroupIndex, uint3 thread_id : SV_DispatchThreadID)
{
    float min_depth;
    float max_depth;
    float2 uv = (float2(thread_id.xy) + 0.5f) * pass_data.texel_size;
    
    float4 result = source_mip.Gather(point_clamp_sampler(), uv);
    min_depth = min4(result);
    max_depth = max4(result);
    output_mip0[thread_id.xy] = float2(min_depth, max_depth);
    
    if (pass_data.number_mip_levels == 1) {
        return;
    }
    
    store_depth(group_index, min_depth, max_depth);
    GroupMemoryBarrierWithGroupSync();
    
    if ((group_index & 0x9) == 0) {
        float2 depth1 = load_depth(group_index + 0x1);
        float2 depth2 = load_depth(group_index + 0x8);
        float2 depth3 = load_depth(group_index + 0x9);
        
        min_depth = min4(float4(min_depth, depth1.x, depth2.x, depth3.x));
        max_depth = max4(float4(max_depth, depth1.y, depth2.y, depth3.y));
        output_mip1[thread_id.xy / 2] = float2(min_depth, max_depth);
        store_depth(group_index, min_depth, max_depth);
    }
    
    if (pass_data.number_mip_levels == 2) {
        return;
    }

    GroupMemoryBarrierWithGroupSync();
    
    if ((group_index & 0x1B) == 0) {
        float2 depth1 = load_depth(group_index + 0x2);
        float2 depth2 = load_depth(group_index + 0x10);
        float2 depth3 = load_depth(group_index + 0x12);
        
        min_depth = min4(float4(min_depth, depth1.x, depth2.x, depth3.x));
        max_depth = max4(float4(max_depth, depth1.y, depth2.y, depth3.y));
        output_mip2[thread_id.xy / 4] = float2(min_depth, max_depth);
        store_depth(group_index, min_depth, max_depth);
    }
    
    if (pass_data.number_mip_levels == 3) {
        return;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (group_index == 0) {
        float2 depth1 = load_depth(group_index + 0x4);
        float2 depth2 = load_depth(group_index + 0x20);
        float2 depth3 = load_depth(group_index + 0x24);
        
        min_depth = min4(float4(min_depth, depth1.x, depth2.x, depth3.x));
        max_depth = max4(float4(max_depth, depth1.y, depth2.y, depth3.y));
        output_mip3[thread_id.xy / 8] = float2(min_depth, max_depth);
    }
}
#endif