#ifndef __UTILS__
#define __UTILS__

const static float3x3 identity_matrix3x3 =
{
    { 1, 0, 0 },
    { 0, 1, 0 },
    { 0, 0, 1 },
};

float3 calculate_ndc_coordinates(float4 transformed_vertex_position)
{
    float3 ndc_coordinates;
    ndc_coordinates.x = 0.5f + ((transformed_vertex_position.x / transformed_vertex_position.w) * 0.5f);
	ndc_coordinates.y = 0.5f - ((transformed_vertex_position.y / transformed_vertex_position.w) * 0.5f);
	ndc_coordinates.z = 0.5f - ((transformed_vertex_position.z / transformed_vertex_position.w) * 0.5f);
	return ndc_coordinates;
}

float3 normal_sample_to_world_space(float3 normal_sample, float3 vertex_normal, float3 tangent)
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
        
#endif