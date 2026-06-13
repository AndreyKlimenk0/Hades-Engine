#ifndef __DRAW_VERTICES__
#define __DRAW_VERTICES__

#include "globals.hlsl"

struct Pass_Data {
    float4 color;
    float4x4 world_matrix;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

float4 vs_main(float3 position : POSITION) : SV_POSITION
{
	return mul(float4(position, 1.0f), mul(pass_data.world_matrix, mul(frame_info.view_matrix, frame_info.perspective_matrix))); 
}

float4 ps_main(float4 screen_position : SV_POSITION) : SV_TARGET
{
    return pass_data.color;
}

#endif