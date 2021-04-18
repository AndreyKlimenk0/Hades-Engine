cbuffer perObject {
	float4x4 world_view_projection;
};

Texture2D texture_map;

SamplerState sampler_anisotropic {
	Filter = ANISOTROPIC;
	MaxAnisotropy = 4;

	AddressU = WRAP;
	AddressV = WRAP;
};

struct Vertex_In {
	float3 position : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct Vertex_Out {
	float4 position : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

Vertex_Out vs_main(Vertex_In vertex)
{
	Vertex_Out result;
	result.position = mul(float4(vertex.position, 1.0f), world_view_projection);
	result.normal = vertex.normal;
	result.uv = vertex.uv;
	return result;
}

float4 ps_main(Vertex_Out pixel) : SV_Target
{
	return texture_map.Sample(sampler_anisotropic, pixel.uv);
}

technique11 ColorTech {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, vs_main()));
		SetPixelShader(CompileShader(ps_5_0, ps_main()));
	}
}

