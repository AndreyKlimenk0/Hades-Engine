//cbuffer perObject {
//	float4x4 world_view_projection;
//};
//
//struct Vertex_In {
//	float3 position : POSITION;
//	float4 color : COLOR;
//};
//
//struct Vertex_Out {
//	float4 position : SV_POSITION;
//	float4 color : COLOR;
//};
//
//Vertex_Out vs_main(Vertex_In vertex)
//{
//	Vertex_Out result;
//	result.position = mul(float4(vertex.position, 1.0f), world_view_projection);
//	result.color = vertex.color;
//	return result;
//}
//
//float4 ps_main(Vertex_Out pixel) : SV_Target
//{
//	return pixel.color.x + 0.2f;
//}
//
//
//technique11 ColorTech {
//	pass P0 {
//		SetVertexShader(CompileShader(vs_5_0, vs_main()));
//		SetGeometryShader(NULL);
//		SetPixelShader(CompileShader(ps_5_0, ps_main()));
//	}
//}

cbuffer cbPerObject {
	float4x4 world_view_projection;
};

Texture2D texture_map;

SamplerState sampler_anisotropic {
	Filter = ANISOTROPIC;
	MaxAnisotropy = 4;

	AddressU = WRAP;
	AddressV = WRAP;
};

struct VertexIn {
	float3 PosL  : POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

struct VertexOut {
	float4 PosH  : SV_POSITION;
	float3 normal : NORMAL;
	float2 uv : TEXCOORD;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	vout.PosH = mul(float4(vin.PosL, 1.0f), world_view_projection);
	vout.normal = vin.normal;
	vout.uv = vin.uv;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return texture_map.Sample(sampler_anisotropic, pin.uv);
}

technique11 ColorTech {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}
