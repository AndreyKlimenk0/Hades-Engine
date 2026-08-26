#ifndef __FULL_SCREEN_TRIANGLE__
#define __FULL_SCREEN_TRIANGLE__

struct Vertex_Out {
	float4 position : SV_POSITION;
	float2 uv : TEXCOORD;
};

Vertex_Out vs_main(uint vertex_id : SV_VertexID)
{
    Vertex_Out vertex_out;
    vertex_out.position.x = float(vertex_id / 2) * 4.0f - 1.0f;
    vertex_out.position.y = float(vertex_id % 2) * 4.0f - 1.0f;
    vertex_out.position.zw = float2(0.0f, 1.0f);
	vertex_out.uv.x = float(vertex_id / 2) * 2.0f;
	vertex_out.uv.y = 1.0f - float(vertex_id % 2) * 2.0f;
	return vertex_out;
}

#endif