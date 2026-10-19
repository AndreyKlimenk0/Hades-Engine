#ifndef __RAYTRACED_SHADOWS__
#define __RAYTRACED_SHADOWS__

#include "globals.hlsl"
#include "utils.hlsl"
#include "constants.hlsl"

RaytracingAccelerationStructure scene;   // C++ puts built BVH here
struct ShadowPayload {                   // Define a ray payload
    float isVisible;                        // 0: occluded, 1: visible
 };
 
[shader("miss")]                         // Define miss shader #0
void ShadowMiss(inout ShadowPayload pay) 
{
  pay.isVisible = 1.0f;                  // We miss ! Ray unoccluded
}

[shader("anyhit")]                         // Add to hit group #0
void ShadowAnyHit(inout ShadowPayload pay, BuiltInTriangleIntersectionAttributes attrib)
{
   // if ( isTransparent( attrib, PrimitiveIndex() ) )
     // IgnoreHit();                           // Skip transparent hits
}

float ShadowRay( float3 orig, float3 dir, float minT, float maxT ) 
{
    RayDesc ray = { orig, minT, dir, maxT }; // Define our new ray.
    ShadowPayload pay = { 0.0f };            // Assume ray is occluded
    TraceRay( scene,(RAY_FLAG_SKIP_CLOSEST_HIT_SHADER | RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH), 0xFF, 0, 1, 0, ray, pay );
    return pay.isVisible;                    // Return ray payload
}

[shader("raygeneration")]
void cast_shadow_ray()
{
    uint2 pixel_id = DispatchRaysIndex().xy;
    uint2 screen_size = DispatchRaysDimensions().xy;
    
    float2 ndc_position = uv_to_ndc((float2)pixel_id / (float2)screen_size);
    float4 world_position = mul(float4(float3(ndc_position, 0.99f), 1.0f), frame_info.inverse_view_perspective_matrix);
    world_position.xyz /= world_position.w;
    
    ShadowRay(frame_info.view_position, normalize(world_position.xyz - frame_info.view_position), -FLT_MAX, FLT_MAX);
}
#endif