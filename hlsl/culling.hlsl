#ifndef __CULLING__
#define __CULLING__

#include "mesh.hlsl"
#include "utils.hlsl"
#include "globals.hlsl"

struct Render_Entity {
    uint mesh_idx;
    uint world_matrix_idx;
    uint pad11;
	uint pad22;
};

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
StructuredBuffer<Render_Entity> render_entities : register(t3, space0);
StructuredBuffer<IndirectCommand> mesh_draw_commands : register(t4, space0);
AppendStructuredBuffer<IndirectCommand> culled_mesh_draw_commands : register(u0, space0);

bool within(float3 x, float3 y, float3 z)
{
    return all(x <= y) && all(y <= z);
}

// bool frustum_culled(float3 min, float3 max)
// {
//     float3 box_corners[] = {
//             float3(min.x, min.y, min.z),
//             float3(max.x, min.y, min.z),
//             float3(min.x, max.y, min.z),
//             float3(max.x, max.y, min.z),

//             float3(min.x, min.y, max.z),
//             float3(max.x, min.y, max.z),
//             float3(min.x, max.y, max.z),
//             float3(max.x, max.y, max.z),
//     };
    
//     for (uint i = 0; i < 6; i++) {
//         uint x = 0;
//         x += dot(frame_info.frustum_planes[i], float4(box_corners[0], 1.0f)) < 0.0f ? 1.0f : 0.0f;
//         x += dot(frame_info.frustum_planes[i], float4(box_corners[1], 1.0f)) < 0.0f ? 1.0f : 0.0f;
//         x += dot(frame_info.frustum_planes[i], float4(box_corners[2], 1.0f)) < 0.0f ? 1.0f : 0.0f;
//         x += dot(frame_info.frustum_planes[i], float4(box_corners[3], 1.0f)) < 0.0f ? 1.0f : 0.0f;
//         x += dot(frame_info.frustum_planes[i], float4(box_corners[4], 1.0f)) < 0.0f ? 1.0f : 0.0f;
//         x += dot(frame_info.frustum_planes[i], float4(box_corners[5], 1.0f)) < 0.0f ? 1.0f : 0.0f;
//         x += dot(frame_info.frustum_planes[i], float4(box_corners[6], 1.0f)) < 0.0f ? 1.0f : 0.0f;
//         x += dot(frame_info.frustum_planes[i], float4(box_corners[7], 1.0f)) < 0.0f ? 1.0f : 0.0f;
//         if (x == 8) {
//             return false;
//         }
//     }
//     return true;
// }

// [numthreads(128, 1, 1)]
// void cs_main(uint3 thread_id : SV_DispatchThreadId)
// {
//     uint index = thread_id.x;
//     if (index < pass_data.draw_command_count) {
//         Render_Entity render_entity = render_entities[index];
//         float4x4 world_matrix = world_matrices[render_entity.world_matrix_idx];
//         float3 position = world_matrix[3].xyz;
//         Mesh_Instance mesh_instance = mesh_instances[render_entity.mesh_idx];

//         AABB bounding_box = mesh_instance.bounding_box;
//         float3 max = bounding_box.max + position;
//         float3 min = bounding_box.min + position;
        
//         if (frustum_culled(min, max)) {
//             culled_mesh_draw_commands.Append(mesh_draw_commands[index]);
//         }
//     }
// }
bool frustum_culled(float3 min, float3 max)
{
    float4x4 view_perspective_matrix = mul(frame_info.freeze_view_matrix, frame_info.perspective_matrix);
    
    float3 box_corners[] = {
            float3(min.x, min.y, min.z),
            float3(max.x, min.y, min.z),
            float3(min.x, max.y, min.z),
            float3(max.x, max.y, min.z),

            float3(min.x, min.y, max.z),
            float3(max.x, min.y, max.z),
            float3(min.x, max.y, max.z),
            float3(max.x, max.y, max.z),
    };
    
    for (uint i = 0; i < 8; i++) {
        float4 result = mul(float4(box_corners[i], 1.0f), view_perspective_matrix);
        float3 x = clip_to_uv_coordinates(result);
        if (saturated(x)) {
            return false;
        }
    }
    return true;
}

[numthreads(128, 1, 1)]
void cs_main(uint3 thread_id : SV_DispatchThreadId)
{
    uint index = thread_id.x;
    if (index < pass_data.draw_command_count) {
        Render_Entity render_entity = render_entities[index];
        float4x4 world_matrix = world_matrices[render_entity.world_matrix_idx];
        float3 position = world_matrix[3].xyz;
        Mesh_Instance mesh_instance = mesh_instances[render_entity.mesh_idx];

        AABB bounding_box = mesh_instance.bounding_box;
        float3 max = bounding_box.max + position;
        float3 min = bounding_box.min + position;
        
        if (!frustum_culled(min, max)) {
            culled_mesh_draw_commands.Append(mesh_draw_commands[index]);
        }
    }
    //culled_mesh_draw_commands.Append(mesh_draw_commands[index]);
}

#endif