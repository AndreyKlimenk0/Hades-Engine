#ifndef __DEFERRED_LIGHT__
#define __DEFERRED_LIGHT__

#include "BRDF.hlsl"
#include "light.hlsl"
#include "utils.hlsl"
#include "shadows.hlsl"
#include "globals.hlsl"
#include "constants.hlsl"
#include "color_conversion.hlsl"
#include "full_screen_triangle.hlsl"

struct Pass_Data {
    uint albedo_metalness_idx;
    uint normal_idx;
    uint roughness_idx;
    uint depth_idx;
    uint hzb_idx;
    uint3 pad;
    float4 sky_color;
};

ConstantBuffer<Pass_Data> pass_data : register(b0, space0);
StructuredBuffer<Light> lights : register(t4, space0);

static const uint rayMarchIterationsMax = 60; // primary ray march step count (higher will find more in distance, but slower)
static const float rayMarchStepIncrease = 0.45f; // primary ray march step increase (higher will travel more distance, but can miss details)
static const uint rayMarchFineIterationsMax = 2; // binary step count (higher is nicer but slower)
static const float rayMarchTolerance = 0.000002; // early exit factor for binary search (smaller is nicer but slower)
static const float rayMarchLevelIncrement = 0.3; // level increment based on ray travel distance and roughness (higher values improves performance, but traces at lower resolution)

// samplePos where ray march left of
float3 BinarySearch(Texture2D<float2> hzb, float3 samplePos, float3 V, float level)
{
    for (uint i = 0; i < rayMarchFineIterationsMax; i++)
    {
        float sampleDepth = hzb.SampleLevel(point_sampler(), samplePos.xy, level).r;

        if (abs(samplePos.z - sampleDepth) < rayMarchTolerance)
        {
            return samplePos;
        }

        if (samplePos.z >= sampleDepth)
        {
            samplePos += V;
        }

        V *= 0.5f;
        samplePos -= V;
    }

    return samplePos;
}

// P and V in screen space [0, 1] x [0, 1]
float3 RayMarch(Texture2D<float2> hzb, float3 P, float3 V, out bool validHit)
{
    float3 samplePos = P + V;

    float sampleDepth = 0;
    float level = 1;

    uint iterations = 0;
    while (iterations <= rayMarchIterationsMax)
    {
        if (any(samplePos.xy < 0.0) || any(samplePos.xy > 1.0))
        {
            validHit = false;
            return samplePos;
        }

        samplePos += V;

        sampleDepth = hzb.SampleLevel(point_sampler(), samplePos.xy, level).r;

        if (sampleDepth > samplePos.z)
        {
            samplePos = BinarySearch(hzb, samplePos, V, level);
            break;
        }

        V *= rayMarchStepIncrease;
        level += rayMarchLevelIncrement;

        iterations++;
    }

    validHit = (iterations <= rayMarchIterationsMax);
    return float3(samplePos.xy, sampleDepth);
}

static const float HiZTraceMostDetailedLevel = 0.0;
static const float HiZTraceIterationsMax = 64;

void InitialAdvanceRay(float3 origin, float3 direction, float2 currentMipResolution, float2 currentMipResolution_rcp, float2 floorOffset, float2 uvOffset, out float3 position, out float tCurrent)
{
    float2 offset = float2(0, 0);
    // float2 offset = (direction.xy * 20.0f));
    float2 currentMipPosition = currentMipResolution * origin.xy + offset;

    // Intersect ray with the half box that is pointing away from the ray origin.
    float2 xyPlane = floor(currentMipPosition) + floorOffset;
    xyPlane = xyPlane * currentMipResolution_rcp + uvOffset;

    // o + d * t = p' => t = (p' - o) / d
    float2 t = (xyPlane - origin.xy) / direction.xy;
    tCurrent = min(t.x, t.y);
    position = origin + tCurrent * direction;
}

bool AdvanceRay(float3 origin, float3 direction, float2 currentMipPosition, float2 currentMipResolution_rcp, float2 floorOffset, float2 uvOffset, float surfaceZ, inout float3 position, inout float tCurrent)
{
    // Create boundary planes
    float2 xyPlane = floor(currentMipPosition) + floorOffset;
    xyPlane = xyPlane * currentMipResolution_rcp + uvOffset;
    float3 boundaryPlanes = float3(xyPlane, surfaceZ);

    // Intersect ray with the half box that is pointing away from the ray origin.
    // o + d * t = p' => t = (p' - o) / d
    float3 t = (boundaryPlanes - origin) / direction;

    // Prevent using z plane when shooting out of the depth buffer.
    t.z = direction.z > 0 ? t.z : FLT_MAX;

    // Choose nearest intersection with a boundary.
    float tMin = min(min(t.x, t.y), t.z);

    // Larger z means closer to the camera.
    bool aboveSurface = surfaceZ > position.z;

    // Decide whether we are able to advance the ray until we hit the xy boundaries or if we had to clamp it at the surface.
    // We use the asuint comparison to avoid NaN / Inf logic, also we actually care about bitwise equality here to see if t_min is the t.z we fed into the min3 above.
    bool skippedTile = asuint(tMin) != asuint(t.z) && aboveSurface;

    // Make sure to only advance the ray if we're still above the surface.
    tCurrent = aboveSurface ? tMin : tCurrent;

    // Advance ray
    position = origin + tCurrent * direction;

    return skippedTile;
}

float2 GetMipResolution(float2 screenDimensions, int mipLevel)
{
    return screenDimensions * pow(0.5, mipLevel);
}

// Based on: https://github.com/GPUOpen-Effects/FidelityFX-SSSR/tree/master
// Requires origin and direction of the ray to be in screen space [0, 1] x [0, 1]
float3 HierarchicalRaymarch(Texture2D<float2> hzb, float3 origin, float3 direction, float2 screenSize, out bool validHit)
{
    // Start on mip with highest detail.
    int currentMip = HiZTraceMostDetailedLevel;

    // Could recompute these every iteration, but it's faster to hoist them out and update them.
    float2 currentMipResolution = GetMipResolution(screenSize, currentMip);
    float2 currentMipResolution_rcp = rcp(currentMipResolution);

    // Offset to the bounding boxes uv space to intersect the ray with the center of the next pixel.
    // This means we ever so slightly over shoot into the next region. 
    float2 uvOffset = 0.005 * exp2(HiZTraceMostDetailedLevel) / screenSize;
    uvOffset = select(direction.xy < 0, -uvOffset, uvOffset);

    // Offset applied depending on current mip resolution to move the boundary to the left/right upper/lower border depending on ray direction.
    float2 floorOffset = select(direction.xy < 0, 0, 1);

    // Initially advance ray to avoid immediate self intersections.
    float tCurrent;
    float3 position;
    InitialAdvanceRay(origin, direction, currentMipResolution, currentMipResolution_rcp, floorOffset, uvOffset, position, tCurrent);
    //InitialAdvanceRay(position, direction, currentMipResolution, currentMipResolution_rcp, floorOffset, uvOffset, position, tCurrent);
    //InitialAdvanceRay(position, direction, currentMipResolution, currentMipResolution_rcp, floorOffset, uvOffset, position, tCurrent);

    int i = 0;
    while (i < HiZTraceIterationsMax && currentMip >= HiZTraceMostDetailedLevel)
    {
        if (any(position.xy < 0.0) || any(position.xy > 1.0))
        {
            validHit = false;
            return position;
        }

        float2 currentMipPosition = currentMipResolution * position.xy;
        float surfaceZ = hzb.Load(int3(currentMipPosition, currentMip)).r;

        bool skippedTile = AdvanceRay(origin, direction, currentMipPosition, currentMipResolution_rcp, floorOffset, uvOffset, surfaceZ, position, tCurrent);

        currentMip += skippedTile ? 1 : -1;
        currentMipResolution *= skippedTile ? 0.5 : 2;
        currentMipResolution_rcp *= skippedTile ? 2 : 0.5;

        i++;
    }

    validHit = (i <= HiZTraceIterationsMax);

    return position;
}

float FFX_SSSR_LoadDepth(float2 uv, uint miplevel)
{
    Texture2D<float2> hzb = ResourceDescriptorHeap[pass_data.hzb_idx];
    //return hzb.SampleLevel(point_sampler(), uv, miplevel).r;
    return hzb.Load(int3(uv, miplevel)).r;
}

float3 FFX_SSSR_LoadWorldSpaceNormal(float2 uv)
{
    Texture2D<float4> normal_texture = ResourceDescriptorHeap[pass_data.normal_idx];
    // return normal_texture.Sample(point_sampler(), uv).rgb;
    return normal_texture.Load(int3(uv, 0)).rgb;
}

float3 FFX_SSSR_ScreenSpaceToViewSpace(float3 uv)
{
    uv.xy = uv_to_ndc(uv.xy);
    float4 result = mul(float4(uv, 1.0f), frame_info.inverse_perspective_matrix);
    result.xyz /= result.w;
    return result.xyz;
}

float FFX_SSSR_ValidateHit(float3 hit, float2 uv, float3 world_space_ray_direction, float2 screen_size, float2 depth_buffer_size, float depth_buffer_thickness) {
    // Reject hits outside the view frustum
    if (any(hit.xy < 0) || any(hit.xy > 1)) {
        return 0;
    }

    // Reject the hit if we didnt advance the ray significantly to avoid immediate self reflection
    float2 manhattan_dist = abs(hit.xy - uv);
    if(all(manhattan_dist < (2 / screen_size))) {
        return 0;
    }

    // Don't lookup radiance from the background.
    int2 texel_coords = int2(depth_buffer_size * hit.xy);
    float surface_z = FFX_SSSR_LoadDepth(texel_coords / 2, 1);
#ifdef FFX_SSSR_INVERTED_DEPTH_RANGE
    if (surface_z == 0.0) {
#else
    if (surface_z == 1.0) {
#endif
        return 0;
    }

    // We check if we hit the surface from the back, these should be rejected.
    float3 hit_normal = FFX_SSSR_LoadWorldSpaceNormal(texel_coords);
    if (dot(hit_normal, world_space_ray_direction) > 0) {
        return 0;
    }

    float3 view_space_surface = FFX_SSSR_ScreenSpaceToViewSpace(float3(hit.xy, surface_z));
    float3 view_space_hit = FFX_SSSR_ScreenSpaceToViewSpace(hit);
    float distance = length(view_space_surface - view_space_hit);

    // Fade out hits near the screen borders
    float2 fov = 0.05 * float2(screen_size.y / screen_size.x, 1);
    float2 border = smoothstep(0, fov, hit.xy) * (1 - smoothstep(1 - fov, 1, hit.xy));
    float vignette = border.x * border.y;

    // We accept all hits that are within a reasonable minimum distance below the surface.
    // Add constant in linear space to avoid growing of the reflections toward the reflected objects.
    float confidence = 1 - smoothstep(0, depth_buffer_thickness, distance);
    confidence *= confidence;

    return vignette * confidence;
}

float3 intersect_plane(float3 o, float3 d, float t)
{
    return o + d * t;
}

float3 move_to_cell_boundery(float3 o, float3 d, float2 cell_index, float2 cell_count, float2 cross_step, float2 cross_offset)
{
    float2 next_cell = cell_index + cross_step;
    next_cell /= cell_count;
    next_cell += cross_offset;
    float2 delta = (next_cell - o.xy) / d.xy;
    float t = min2(delta);
    float3 ray = o + d * t;
    //ray.xy += (delta.x < delta.y) ? float2(cross_offset.x, 0.0) : float2(0.0, cross_offset.y);
    return ray;
}

float2 get_cell(float3 ray, float2 cell_count)
{
    return floor(ray.xy * cell_count);
}

float2 get_cell_count(Texture2D<float2> hzb, uint level)
{
    uint3 size;
    hzb.GetDimensions(level, size.x, size.y, size.z);
    return size.xy;
    //return float2(1900, 980) / size.xy;
}

// float3 hiz_trace(Texture2D<float> hzb, float3 pss0, float3 vss, float3 o, float3 d, out bool hit)
// {
//     float3 ray = pss0;
//     int iterations = 0;
//     int level = 2;
//     int max_hzb_level = global_info.hzb_width_height_mips.z;
    
//     float2 cross_step = float2(vss.x >= 0.0f ? 1.0f : -1.0f, vss.y >= 0.0f ? 1.0f : -1.0f);
//     float2 cross_offset = float2(cross_step.xy * (1.0f / global_info.hzb_width_height_mips.xy));
//     cross_step.xy = saturate(cross_step.xy);
    
//     float2 cell_count = get_cell_count(hzb, level);
//     float2 cell = get_cell(ray, cell_count);
//     ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
    
//     while ((level >= 2 )&& (iterations < 64)) {
//     // while ((level < (global_info.hzb_width_height_mips.z - 2)) && (level < max_hzb_level) && (iterations < 64)) {
//         if (any(ray.xy < 0.0f) || any(ray.xy > 1.0f)) { 
//             ray = pss0;
//             hit = false;
//             return ray;
//         }
//         float min_z = hzb.SampleLevel(point_sampler(), ray.xy, level);
        
//         float2 cell_count = get_cell_count(hzb, level);
//         float2 old_cell = get_cell(ray, cell_count);
//         float3 temp_ray = o + d * max(ray.z, min_z);
        
//         float2 new_cell = get_cell(temp_ray, cell_count);
        
//         if (any(new_cell != old_cell)) {
//             temp_ray = move_to_cell_boundery(o, d, old_cell, cell_count, cross_step, cross_offset);
//             level = min(level + 2, global_info.hzb_width_height_mips.z - 1);
//         }
//         // } else if (ray.z < min_z) {
//         //     temp_ray = move_to_cell_boundery(o, d, old_cell, cell_count, cross_step, cross_offset);
//         //     level = min(level + 2, global_info.hzb_width_height_mips.z - 2);
//         // }
//         ray = temp_ray;
//         level--;
//         iterations++;
//     }
//     hit  = iterations < 64;
//     return ray;
// }

float3 move_to_cell_boundery_z(float3 o, float3 d, float2 cell_index, float2 cell_count, float2 cross_step, float2 cross_offset, float z)
{
    float2 next_cell = cell_index + cross_step;
    next_cell /= cell_count;
    next_cell += cross_offset;
    float3 delta = (float3(next_cell, z) - o) / d;
    delta.z = delta.z > 0.0f ? delta.z : FLT_MAX;
    float t = min(min2(delta.xy), delta.z);
    //float t = min(min2(delta.xy), delta.z);
    float3 ray = o + d * t;
    //ray.xy += (delta.x < delta.y) ? float2(cross_offset.x, 0.0) : float2(0.0, cross_offset.y);
    return ray;
}

float3 hiz_trace(Texture2D<float2> hzb, float3 pss0, float3 vss, float3 o, float3 d, out bool hit)
{
    float3 ray = pss0;
    int iterations = 0;
    int level = 0;
    int max_hzb_level = global_info.hzb_width_height_mips.z;
    
    float2 cross_step = float2(vss.x >= 0.0f ? 1.0f : -1.0f, vss.y >= 0.0f ? 1.0f : -1.0f);
    float2 cross_offset = 0.005f * float2(cross_step.xy * (1.0f / global_info.hzb_width_height_mips.xy));
    cross_step.xy = saturate(cross_step.xy);
    
        float2 cell_count = get_cell_count(hzb, level);
    float2 cell = get_cell(ray, cell_count);
    ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
    cell_count = get_cell_count(hzb, level);
    cell = get_cell(ray, cell_count);
    ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
    cell_count = get_cell_count(hzb, level);
    cell = get_cell(ray, cell_count);
    ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
        cell_count = get_cell_count(hzb, level);
    cell = get_cell(ray, cell_count);
    ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
        cell_count = get_cell_count(hzb, level);
    cell = get_cell(ray, cell_count);
    ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
    
    int max_iterations = 124;
    while ((level > -1 ) && (iterations < max_iterations)) {
        if (any(ray.xy < 0.0f) || any(ray.xy > 1.0f)) { 
            ray = pss0;
            hit = false;
            return ray;
        }
        float min_z = hzb.SampleLevel(point_clamp_sampler(), ray.xy, level).r;
        
        float2 cell_count = get_cell_count(hzb, level);
        float2 cell = get_cell(ray, cell_count);
        float3 temp = move_to_cell_boundery_z(o, d, cell, cell_count, cross_step, cross_offset, min_z);
        
        if (ray.z < min_z) {
            level = min(level + 1, max_hzb_level - 1);
            ray = temp;
        } else if ((level == 0) && (iterations == 0) && (ray.z > min_z)) {
            level++;
            ray = temp;
        } else {
            level--;
        }
        iterations++;
    }
    hit  = iterations < max_iterations;
    return ray;
}

// float3 hiz_trace(Texture2D<float2> hzb, float3 pss0, float3 vss, float3 o, float3 d, out bool hit)
// {
//     float3 ray = pss0;
//     int iterations = 0;
//     int level = 0;
//     int max_hzb_level = global_info.hzb_width_height_mips.z;
    
//     float2 cross_step = float2(vss.x >= 0.0f ? 1.0f : -1.0f, vss.y >= 0.0f ? 1.0f : -1.0f);
//     float2 cross_offset = 0.005f * float2(cross_step.xy * (1.0f / global_info.hzb_width_height_mips.xy));
//     cross_step.xy = saturate(cross_step.xy);
    
//     float2 cell_count = get_cell_count(hzb, level);
//     float2 cell = get_cell(ray, cell_count);
//     ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
//     cell_count = get_cell_count(hzb, level);
//     cell = get_cell(ray, cell_count);
//     ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
//     cell_count = get_cell_count(hzb, level);
//     cell = get_cell(ray, cell_count);
//     ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
    
//     int max_iterations = 124;
//     while ((level > -1 ) && (iterations < max_iterations)) {
//         if (any(ray.xy < 0.0f) || any(ray.xy > 1.0f)) { 
//             ray = pss0;
//             hit = false;
//             return ray;
//         }
//         float2 cell_count = get_cell_count(hzb, level);
//         float2 cell = get_cell(ray, cell_count);
//         float3 temp = ray;
//         ray = move_to_cell_boundery(o, d, cell, cell_count, cross_step, cross_offset);
        
//         float min_z = hzb.SampleLevel(point_border_sampler(), ray.xy, level).r;
//         //float x = abs(ray.z - min_z);
//         if (ray.z < min_z) {
//             level = min(level + 1, max_hzb_level - 1);
//         } else {
//             ray = temp;
//             level--;
//         }
//         iterations++;
//     }
//     hit  = iterations < max_iterations;
//     return ray;
// }

// float3 linear_ssr(Texture2D<float2> depth_buffer, float3 origin, float3 direction, out bool hit)
// {
    
// }


float3 linear_ssr(
    Texture2D<float2> depth_buffer,
    float3 origin,
    float3 direction,
    out bool hit)
{
    hit = false;

    const int MAX_STEPS = 2048;
    const float THICKNESS = 0.001f;

    const float2 screen_size = float2(1900.0f, 980.0f);

    uint depth_width, depth_height;
    depth_buffer.GetDimensions(depth_width, depth_height);

    float2 depth_size = float2(depth_width, depth_height);

    // origin: UV + depth [0, 1]
    // direction: projected_endpoint - origin

    float2 dir_pixels = direction.xy * screen_size;

    float step_count = max(
        abs(dir_pixels.x),
        abs(dir_pixels.y)
    );

    if (step_count < 1e-6f)
        return origin;

    float step_size = 1.0f / step_count;

    float3 ray = origin;

    float previous_delta = 0.0f;
    bool previous_valid = false;

    [loop]
    for (int i = 0; i < MAX_STEPS; ++i)
    {
        float t = (i + 1) * step_size;

        if (t > 1.0f)
            break;

        ray = origin + direction * t;

        if (any(ray.xy < 0.0f) ||
            any(ray.xy >= 1.0f) ||
            ray.z < 0.0f ||
            ray.z > 1.0f)
        {
            break;
        }

        // Convert normalized UV into depth texture coordinates.
        int2 pixel = int2(ray.xy * depth_size);

        float scene_depth =
            depth_buffer.Load(int3(pixel, 0)).x;

        // Standard Z: near = 0, far = 1.
        if (scene_depth >= 1.0f)
        {
            previous_valid = false;
            continue;
        }

        float delta = ray.z - scene_depth;

        if (previous_valid &&
            previous_delta < 0.0f &&
            delta >= 0.0f &&
            delta < THICKNESS)
        {
            hit = true;
            return ray;
        }

        previous_delta = delta;
        previous_valid = true;
    }

    return ray;
}

float4 ps_main(Vertex_Out vertex_out) : SV_TARGET
{
    Texture2D<float4> albedo_metalness_texture = ResourceDescriptorHeap[pass_data.albedo_metalness_idx];
    Texture2D<float4> normal_texture = ResourceDescriptorHeap[pass_data.normal_idx];
    Texture2D<float>  roughness_texture = ResourceDescriptorHeap[pass_data.roughness_idx];
    Texture2D<float>  depth_buffer = ResourceDescriptorHeap[pass_data.depth_idx];
    Texture2D<float2>  hzb = ResourceDescriptorHeap[pass_data.hzb_idx];
    
    float depth = depth_buffer.Sample(point_sampler(), vertex_out.uv);
    if (depth == global_info.max_depth) {
        return pass_data.sky_color;
    }
    
    float4 albedo_metalness = albedo_metalness_texture.Sample(point_sampler(), vertex_out.uv);
    float3 normal = normal_texture.Sample(point_sampler(), vertex_out.uv).rgb;
    float  roughness = roughness_texture.Sample(point_sampler(), vertex_out.uv);
    float3 albedo = albedo_metalness.rgb;
    float  metalness = albedo_metalness.a;
    
    float4 clip_space_position = float4(uv_to_ndc(vertex_out.uv), depth, 1.0f);
    clip_space_position = mul(clip_space_position, frame_info.inverse_view_perspective_matrix);
    float3 world_position = clip_space_position.xyz / clip_space_position.w;
    
    float3 view_direction = normalize(frame_info.view_position - world_position);
    float3 normal_normal = normal;
    normal = float3(0, 1, 0);
    
    float3 pss0 = float3(vertex_out.uv, depth);
    
    float3 vv = normalize(world_position - frame_info.view_position);
    float4 pcs = mul(float4(world_position + reflect(vv, normal), 1.0f), frame_info.view_perspective_matrix);
    pcs /= pcs.w;

    float3 pss1 = float3(ndc_to_uv(pcs.xyz), pcs.z);
    
    //float3 vss = pss1 - pss0;
    float3 vss = pss1 - pss0;
    float3 d = vss.xyz / vss.z;
    float3 o = pss0 + d * -pss0.z;
    
    //float3 ray = hi_z_trace(hzb, pss0, vss);
    bool hit = false;
    float3 ray = HierarchicalRaymarch(hzb, pss0, vss, float2(1024, 512), hit);
    //float3 ray = HierarchicalRaymarch(hzb, pss0, vss, float2(1900, 980), hit);
    // float3 ray = RayMarch(hzb, pss0, vss, hit);
    //float3 ray = pss0;
    //float3 ray = linear_ssr(hzb, pss0, vss, hit);
    
    // float3 ray = hiz_trace(hzb, pss0, vss, pss0, vss, hit);
    //float3 ray = hiz_trace(hzb, pss0, vss, o, d, hit);
    if (!hit) { ray = pss0; }
    float distance = length(abs(ray - pss0));
    if (hit && (distance < 0.025f)) {
        ray = pss0;
    }
    
    // float confidence = hit ? FFX_SSSR_ValidateHit(ray, vertex_out.uv, vss, float2(1900, 980), 0.9f) : 0;
    // float confidence = hit ? FFX_SSSR_ValidateHit(ray, vertex_out.uv, vss, float2(1900, 980), float2(1024, 512), 0.015f) : 0;
    // if (confidence <= 0) {
        // ray = pss0;
    // }
    
    // float2 screen_size = float2(1024, 512);
    // float2 manhattan_dist = abs(ray.xy - vertex_out.uv);
    // if(all(manhattan_dist < (2 / screen_size))) {
    //     ray = pss0;
    // }
    // float3 hit_normal = normal_texture.Sample(point_sampler(), ray.xy).rgb;
    // float3 reflection_ray = reflect(vv, normal);
    // if (dot(reflection_ray, hit_normal) > 0.0f) {
    //     ray = pss0;
    // }
    
    float3 final_color = { 0, 0, 0 };
    if (dot(normal, normal_normal) < 0.8f) {
        ray = pss0;
    }
    float3 light_direction = { 0.0f, 0.0f, 0.0f };
    for (uint i = 0; i < frame_info.light_count; i++) {
        Light light = lights[i];
        switch (light.light_type) {
            case DIRECTIONAL_LIGHT_TYPE:
                light_direction = normalize(-light.direction);
                break;
        }
        final_color += cook_torrance_BRDF(normal, view_direction, light_direction, SRGB_to_linear(albedo), roughness, metalness);
    }
    final_color = SRGB_to_linear(albedo_metalness_texture.Sample(point_sampler(), ray.xy).rgb);
    final_color = linear_to_SRGB(final_color);
    if (dot(normal, normal_normal) >= 0.8f) {
        //final_color.x += 0.5f;
    }
    return float4(final_color, 1.0f);
}

#endif