#ifndef __OUTLINING__
#define __OUTLINING__

#include "utils.hlsl"

Texture2D<uint> silhouette_texture : register(t20);
Texture2D<float> silhouette_depth_stencil_texture : register(t21);
Texture2DMS<float> screen_depth_stencil_back_buffer : register(t22);
RWTexture2D<float4> screen_back_buffer : register(u1);

cbuffer Outlining_Info : register(b0) {
    int range;
    int3 pad0;
    float4 outlining_color;
};

[numthreads(32, 32, 1)]
void cs_main(uint3 thread_id : SV_DispatchThreadID)
{
    uint sample_count = 0;
    uint2 temp;
    screen_depth_stencil_back_buffer.GetDimensions(temp.x, temp.y, sample_count);
    
    float result = 0.0f;
    for (uint i = 0; i < sample_count; i++) {
        result = max(result, screen_depth_stencil_back_buffer.Load(thread_id.xy, i));
    }
    if (result == 0.0f) { 
        return; 
    }
    uint index_sample = silhouette_texture[thread_id.xy];
    float depth_sample = silhouette_depth_stencil_texture[thread_id.xy];    
    for (int x = -range; x <= range; x++) {
        for (int y = -range; y <= range; y++) {
            int2 offset = uint2(x, y);
            uint shifted_index_sample = silhouette_texture[thread_id.xy + offset];
            float shifted_depth_sample = silhouette_depth_stencil_texture[thread_id.xy + offset];
            if ((shifted_index_sample != index_sample) && (shifted_depth_sample > depth_sample)) {
                screen_back_buffer[thread_id.xy] = outlining_color;
                break;
            }
        }
    }
}

#endif
