#ifndef __DOWNSAMPLE_HZB_H__
#define __DOWNSAMPLE_HZB_H__

#include "globals.hlsl"
#include "utils.hlsl"

struct Pass_Data {
    uint number_mip_levels;
    float2 texel_size;
    uint pad;
};

Texture2D<float> source_mip : register(t0, space0);
RWTexture2D<float> output_mip0 : register(u0, space0);
RWTexture2D<float> output_mip1 : register(u1, space0);
RWTexture2D<float> output_mip2 : register(u2, space0);
RWTexture2D<float> output_mip3 : register(u3, space0);

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

groupshared float depth_storage[64];

void store_depth(uint index, float depth)
{
    depth_storage[index] = depth;
}

float load_depth(uint index)
{
    return depth_storage[index];
}

[numthreads(8, 8, 1)]
void cs_main(uint group_index : SV_GroupIndex, uint3 thread_id : SV_DispatchThreadID)
{
    float depth0;
    float2 uv = (float2(thread_id.xy) + 1.0f) * pass_data.texel_size;
    
    float4 result = source_mip.Gather(point_clamp_sampler(), uv);
    depth0 = max4(result);
    output_mip0[thread_id.xy] = depth0;
    
    if (pass_data.number_mip_levels == 1) {
        return;
    }
    
    store_depth(group_index, depth0);
    GroupMemoryBarrierWithGroupSync();
    
    if ((group_index & 0x9) == 0) {
        float depth1 = load_depth(group_index + 0x1);
        float depth2 = load_depth(group_index + 0x8);
        float depth3 = load_depth(group_index + 0x9);
        
        depth0 = max4(float4(depth0, depth1, depth2, depth3));
        output_mip1[thread_id.xy / 2] = depth0;
        store_depth(group_index, depth0);
    }
    
    if (pass_data.number_mip_levels == 2) {
        return;
    }

    GroupMemoryBarrierWithGroupSync();
    
    if ((group_index & 0x1B) == 0) {
        float depth1 = load_depth(group_index + 0x2);
        float depth2 = load_depth(group_index + 0x10);
        float depth3 = load_depth(group_index + 0x12);
        
        depth0 = max4(float4(depth0, depth1, depth2, depth3));
        output_mip2[thread_id.xy / 4] = depth0;
        store_depth(group_index, depth0);
    }
    
    if (pass_data.number_mip_levels == 3) {
        return;
    }
    
    GroupMemoryBarrierWithGroupSync();
    
    if (group_index == 0) {
        float depth1 = load_depth(group_index + 0x4);
        float depth2 = load_depth(group_index + 0x20);
        float depth3 = load_depth(group_index + 0x24);
        
        depth0 = max4(float4(depth0, depth1, depth2, depth3));
        output_mip3[thread_id.xy / 8] = depth0;
    }
}
#endif