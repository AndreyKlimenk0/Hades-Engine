#ifndef __BRDF__
#define __BRDF__

static const float PI = 3.14159265f;

float3 fresnel_schlick(float cosTheta, float3 F0)
{
    return F0 + (1.0 - F0) * pow(1.0 - cosTheta, 5.0);
}

float distribution_GGX(float3 N, float3 H, float roughness)
{
    float a      = roughness * roughness;
    float a2     = a * a;
    float NdotH  = saturate(dot(N, H));
    float NdotH2 = NdotH * NdotH;

    float denom = (NdotH2 * (a2 - 1.0) + 1.0);
    return a2 / (PI * denom * denom);
}

float geometry_schlick_GGX(float NdotV, float roughness)
{
    float r = roughness + 1.0;
    float k = (r * r) / 8.0;

    return NdotV / (NdotV * (1.0 - k) + k);
}

float GeometrySmith(float3 N, float3 V, float3 L, float roughness)
{
    float NdotV = saturate(dot(N, V));
    float NdotL = saturate(dot(N, L));
    return geometry_schlick_GGX(NdotV, roughness) * geometry_schlick_GGX(NdotL, roughness);
}

float3 compute_F0(float3 albedo, float metallic)
{
    float3 dielectricF0 = float3(0.04, 0.04, 0.04);
    return lerp(dielectricF0, albedo, metallic);
}

float3 cook_torrance_BRDF(float3 N, float3 V, float3 L, float3 albedo, float roughness, float metallic)
{
    float3 H = normalize(V + L);

    float NdotL = saturate(dot(N, L));
    float NdotV = saturate(dot(N, V));
    float HdotV = saturate(dot(H, V));

    float3 F0 = compute_F0(albedo, metallic);

    float  D = distribution_GGX(N, H, roughness);
    float  G = GeometrySmith(N, V, L, roughness);
    float3 F = fresnel_schlick(HdotV, F0);

    // Specular
    float3 specular = (D * G * F) / max(4.0 * NdotV * NdotL, 0.001);

    // Diffuse (energy conserving)
    float3 kS = F;
    float3 kD = (1.0 - kS) * (1.0 - metallic);

    float3 diffuse = kD * albedo / PI;

    return (diffuse + specular) * NdotL;
}
#endif