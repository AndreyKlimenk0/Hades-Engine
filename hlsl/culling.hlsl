#ifndef __CULLING__
#define __CULLING__

#include "mesh.hlsl"
#include "utils.hlsl"
#include "globals.hlsl"
#include "constants.hlsl"

struct IndirectCommand {
    uint2 cbvAddress;
    uint4 drawArguments;
};

struct Pass_Data {
    uint draw_command_count;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);

Texture2D<float> hzb_texture : register(t0, space0);

StructuredBuffer<float4x4> world_matrices : register(t1, space0);

StructuredBuffer<Mesh_Instance> mesh_instances : register(t2, space0);
StructuredBuffer<IndirectCommand> mesh_draw_commands : register(t4, space0);
StructuredBuffer<AABB> bounding_boxes : register(t5, space0);
AppendStructuredBuffer<IndirectCommand> culled_mesh_draw_commands : register(u0, space0);

bool frustum_culled(float3 min_AABB, float3 max_AABB)
{
    float4x4 view_perspective_matrix = mul(frame_info.freeze_view_matrix, frame_info.perspective_matrix);
    float3 bounding_box_corners[] = {
            float3(min_AABB.x, min_AABB.y, min_AABB.z),
            float3(max_AABB.x, min_AABB.y, min_AABB.z),
            float3(min_AABB.x, max_AABB.y, min_AABB.z),
            float3(max_AABB.x, max_AABB.y, min_AABB.z),
            float3(min_AABB.x, min_AABB.y, max_AABB.z),
            float3(max_AABB.x, min_AABB.y, max_AABB.z),
            float3(min_AABB.x, max_AABB.y, max_AABB.z),
            float3(max_AABB.x, max_AABB.y, max_AABB.z),
    };
    for (uint i = 0; i < 8; i++) {
        float3 uv_position = clip_to_uv_coordinates(mul(float4(bounding_box_corners[i], 1.0f), view_perspective_matrix));
        if (saturated(uv_position)) {
            return false;
        }
    }
    return true;
}

void world_aabb_to_ndc(inout float3 min_AABB, inout float3 max_AABB)
{
    float4x4 view_perspective_matrix = mul(frame_info.freeze_view_matrix, frame_info.perspective_matrix);
    float3 ndc_min_AABB = float3(FLT_MAX, FLT_MAX, FLT_MAX);
    float3 ndc_max_AABB = float3(-FLT_MAX, -FLT_MAX, -FLT_MAX);
    float3 bounding_box_corners[] = {
            float3(min_AABB.x, min_AABB.y, min_AABB.z),
            float3(max_AABB.x, min_AABB.y, min_AABB.z),
            float3(min_AABB.x, max_AABB.y, min_AABB.z),
            float3(max_AABB.x, max_AABB.y, min_AABB.z),
            float3(min_AABB.x, min_AABB.y, max_AABB.z),
            float3(max_AABB.x, min_AABB.y, max_AABB.z),
            float3(min_AABB.x, max_AABB.y, max_AABB.z),
            float3(max_AABB.x, max_AABB.y, max_AABB.z),
    };
    for (uint i = 0; i < 8; i++) {
        float4 clip_space_position = mul(float4(bounding_box_corners[i], 1.0f), view_perspective_matrix);
        float3 ndc_position = clip_space_position.xyz / clip_space_position.w;
        ndc_min_AABB = min(ndc_position, ndc_min_AABB);
        ndc_max_AABB = max(ndc_position, ndc_max_AABB);
    }
    min_AABB = ndc_min_AABB;
    max_AABB = ndc_max_AABB;
}

float2 ndc_to_uv(float3 ndc_position)
{
    return ndc_position.xy * float2(0.5f, -0.5f) + 0.5f;
}

bool occlusion_culled(float3 min_AABB, float3 max_AABB)
{
    world_aabb_to_ndc(min_AABB, max_AABB);
    float2 uv_min_AABB = ndc_to_uv(min_AABB);
    float2 uv_max_AABB = ndc_to_uv(max_AABB);
    
    float2 min_HZB_pixel = uv_min_AABB * global_info.hzb_width_height_mips.xy;
    float2 max_HZB_pixel = uv_max_AABB * global_info.hzb_width_height_mips.xy;
    
    float mip_level = ceil(log2(max2(max_HZB_pixel - min_HZB_pixel)));
    float mip_scale = rcp(exp2(mip_level));
    float2 min_mip = min_HZB_pixel * mip_scale;
    float2 max_mip = max_HZB_pixel * mip_scale;
    if (all(floor(min_mip) == floor(max_mip))) {
        mip_level -= 1;
    }
    
    if (mip_level < global_info.hzb_width_height_mips.z) {
        float4 depths;
        depths.x = hzb_texture.SampleLevel(point_clamp_sampler(), uv_min_AABB, mip_level);
        depths.y = hzb_texture.SampleLevel(point_clamp_sampler(), uv_max_AABB, mip_level);
        depths.w = hzb_texture.SampleLevel(point_clamp_sampler(), float2(uv_min_AABB.x, uv_max_AABB.y), mip_level);
        depths.z = hzb_texture.SampleLevel(point_clamp_sampler(), float2(uv_max_AABB.x, uv_min_AABB.y), mip_level);
        
        float max_depth = max4(depths);
        return min_AABB.z > max_depth;
    }
    return false;
}

[numthreads(128, 1, 1)]
void cs_main(uint3 thread_id : SV_DispatchThreadId)
{
    uint index = thread_id.x;
    if (index < pass_data.draw_command_count) {
        Mesh_Instance mesh_instance = mesh_instances[index];
        AABB bounding_box = bounding_boxes[mesh_instance.bounding_box_idx];
        float3 max_AABB = bounding_box.max;
        float3 min_AABB = bounding_box.min;
        
        if (!frustum_culled(min_AABB, max_AABB) && !occlusion_culled(min_AABB, max_AABB)) {
            culled_mesh_draw_commands.Append(mesh_draw_commands[index]);
        }
    }
}

#endif