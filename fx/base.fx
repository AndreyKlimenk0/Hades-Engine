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

struct VertexIn {
	float3 PosL  : POSITION;
	float4 Color : COLOR;
};

struct VertexOut {
	float4 PosH  : SV_POSITION;
	float4 Color : COLOR;
};

VertexOut VS(VertexIn vin)
{
	VertexOut vout;
	// Transform to homogeneous clip space.
	vout.PosH = mul(float4(vin.PosL, 1.0f), world_view_projection);
	//vout.PosH = float4(vin.PosL, 1.0f);
	// Just pass vertex color into the pixel shader.
	vout.Color = vin.Color;

	return vout;
}

float4 PS(VertexOut pin) : SV_Target
{
	return pin.Color;
}

technique11 ColorTech {
	pass P0 {
		SetVertexShader(CompileShader(vs_5_0, VS()));
		SetGeometryShader(NULL);
		SetPixelShader(CompileShader(ps_5_0, PS()));
	}
}
