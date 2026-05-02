#ifndef __UTILS__
#define __UTILS__

const static float3x3 identity_matrix3x3 =
{
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
};

// The function converts coordinates from [-1, 1] to [0, 1]
float3 clip_to_uv_coordinates(float4 transformed_vertex_position)
{
    float3 ndc_coordinates;
    ndc_coordinates.x = 0.5f + ((transformed_vertex_position.x / transformed_vertex_position.w) * 0.5f);
	ndc_coordinates.y = 0.5f - ((transformed_vertex_position.y / transformed_vertex_position.w) * 0.5f);
	ndc_coordinates.z = transformed_vertex_position.z / transformed_vertex_position.w;
	return ndc_coordinates;
}

float3x3 get_TBN_matrix(float3 tangent, float3 vertex_normal)
{
    float3x3 TBN = identity_matrix3x3;
    TBN[0] = tangent;
    TBN[1] = cross(vertex_normal, tangent);
    TBN[2] = vertex_normal;
    return TBN;
}

float3 normal_mapping(float3 normal_sample, float3 vertex_normal, float3 tangent)
{
    float3 uncompress_normal = (normal_sample * 2.0f) - 1.0f;
    float3x3 TBN = identity_matrix3x3;
    TBN[0] = tangent;
    TBN[1] = cross(vertex_normal, tangent);
    TBN[2] = vertex_normal;
    return mul(uncompress_normal, TBN);
}

float4 normalize_rgb(int r, int g, int b)
{
	return float4(r / 255.0f, g / 255.0f, b / 255.0f, 1.0f);
}

float4 unpack_rgba(uint encoded)
{
    float4 decoded;
    decoded.x = (0xFF000000u & encoded) >> 24;
    decoded.y = (0x00FF0000u & encoded) >> 16;
    decoded.z = (0x0000FF00u & encoded) >> 8;
    decoded.w = (0x000000FFu & encoded);
    decoded /= 255.0;
    return decoded;
}

template<typename T>
inline bool saturated(T a) 
{ 
    return all(a == saturate(a));
}

template<typename T>
float max2(T vec)
{
    return max(vec.x, vec.y);
}

template<typename T>
float max4(T vec)
{
    return max(max(vec.x, vec.y), max(vec.z, vec.w));
}
#endif