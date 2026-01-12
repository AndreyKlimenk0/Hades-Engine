#ifndef __OUTLINING__
#define __OUTLINING__

#include "utils.hlsl"

struct Pass_Data {
    int range;
    int3 pad0;
    float4 outlining_color;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

Texture2D<uint> silhouette_texture : register(t0, space0);
Texture2D<float> silhouette_depth : register(t1, space0);
Texture2D<float> back_buffer_depth : register(t2, space0);
RWTexture2D<float4> outlining : register(u0, space0);

[numthreads(32, 32, 1)]
void cs_main(uint3 thread_id : SV_DispatchThreadID)
{
    outlining[thread_id.xy] = float4(0.0f, 0.0f, 0.0f, 0.0f);

    // UI elements are drawn with zero depth. Prevent outlining over them.
    float result = back_buffer_depth[thread_id.xy];
    if (result == 0.0f) { 
        return; 
    }
    uint index_sample = silhouette_texture[thread_id.xy];
    float depth_sample = silhouette_depth[thread_id.xy];    
    for (int x = -pass_data.range; x <= pass_data.range; x++) {
        for (int y = -pass_data.range; y <= pass_data.range; y++) {
            int2 offset = uint2(x, y);
            uint shifted_index_sample = silhouette_texture[thread_id.xy + offset];
            float shifted_depth_sample = silhouette_depth[thread_id.xy + offset];
            if ((shifted_index_sample != index_sample) && (depth_sample > shifted_depth_sample)) {
                outlining[thread_id.xy] = pass_data.outlining_color;
                break;
            }
        }
    }
}

#endif
