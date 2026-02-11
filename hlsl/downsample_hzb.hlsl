#ifndef __DOWNSAMPLE_HZB_H__
#define __DOWNSAMPLE_HZB_H__

#include "globals.hlsl"

struct Pass_Data {
    uint src_mip_level;
    uint pad;
    float2 texel_size;
};

Texture2D<float> SrcMip : register(t0, space0);
RWTexture2D<float> DstMip : register(u0, space0);

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

[numthreads(8, 8, 1)]
void cs_main(uint3 thread_id : SV_DispatchThreadID)
{
    // float2 offset = texel_size;
    // float2 uv = (float2(thread_id.xy) + 0.5f) * texel_size;
    // float sample0 = src_mip.SampleLevel(point_sampler(), uv, src_mip_level);
    // float sample1 = src_mip.SampleLevel(point_sampler(), uv + float2(offset, 0.0f), src_mip_level);
    // float sample2 = src_mip.SampleLevel(point_sampler(), uv + float2(0.0f, offset), src_mip_level);
    // float sample3 = src_mip.SampleLevel(point_sampler(), uv + float2(offset, offset), src_mip_level);
    
    // float max_depth = max(max(sample0, sample1), max(sample2, sample3));
    // dst_mip[thread_id.xy] = max_depth;
    
    float2 UV1 = pass_data.texel_size * (thread_id.xy + float2(0.25, 0.25));
    float2 Off = pass_data.texel_size * 0.5;
            
    float2 offset1 = float2(0.0, 0.0);
    float2 offset2 = float2(Off.x, 0.0);
    float2 offset3 = float2(0.0, Off.y);
    float2 offset4 = float2(Off.x, Off.y);
            
    float Src = max(max(max(SrcMip.SampleLevel(point_sampler(), UV1 + offset1, pass_data.src_mip_level),
            max(SrcMip.SampleLevel(point_sampler(), UV1 + offset2, pass_data.src_mip_level),
                max(SrcMip.SampleLevel(point_sampler(), UV1 + offset3, pass_data.src_mip_level),
                    SrcMip.SampleLevel(point_sampler(), UV1 + offset4, pass_data.src_mip_level)))),
            max(SrcMip.SampleLevel(point_sampler(), UV1 + offset1 + float2(Off.x, 0.0), pass_data.src_mip_level),
                max(SrcMip.SampleLevel(point_sampler(), UV1 + offset2 + float2(Off.x, 0.0), pass_data.src_mip_level),
                    max(SrcMip.SampleLevel(point_sampler(), UV1 + offset3 + float2(Off.x, 0.0), pass_data.src_mip_level),
                        SrcMip.SampleLevel(point_sampler(), UV1 + offset4 + float2(Off.x, 0.0), pass_data.src_mip_level))))),
            
          max(max(SrcMip.SampleLevel(point_sampler(), UV1 + offset1 + float2(0.0, Off.y), pass_data.src_mip_level),
            max(SrcMip.SampleLevel(point_sampler(), UV1 + offset2 + float2(0.0, Off.y), pass_data.src_mip_level),
                max(SrcMip.SampleLevel(point_sampler(), UV1 + offset3 + float2(0.0, Off.y), pass_data.src_mip_level),
                    SrcMip.SampleLevel(point_sampler(), UV1 + offset4 + float2(0.0, Off.y), pass_data.src_mip_level)))),
            max(SrcMip.SampleLevel(point_sampler(), UV1 + offset1 + float2(Off.x, Off.y), pass_data.src_mip_level),
                max(SrcMip.SampleLevel(point_sampler(), UV1 + offset2 + float2(Off.x, Off.y), pass_data.src_mip_level),
                    max(SrcMip.SampleLevel(point_sampler(), UV1 + offset3 + float2(Off.x, Off.y), pass_data.src_mip_level),
                        SrcMip.SampleLevel(point_sampler(), UV1 + offset4 + float2(Off.x, Off.y), pass_data.src_mip_level))))));
     DstMip[thread_id.xy] = Src;
}
#endif