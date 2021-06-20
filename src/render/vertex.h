#ifndef VERTEX_H
#define VERTEX_H

#include <d3d11.h>
#include "../libs/math/vector.h"


struct Vertex_XC {
	Vector3 position;
	Vector4 color;
	Vertex_XC() {}
	Vertex_XC(const Vector3 &position, const Vector4 &color) : position(position), color(color) {}
};

struct Vertex_XUV {
	Vector3 position;
	Vector2 uv;
	Vertex_XUV(const Vector3 &position, const Vector2 &uv) : position(position), uv(uv) {}
};

struct Vertex {
	Vector3 position;
	Vector3 normal;
	Vector2 uv;
	Vertex() {}
	Vertex(Vector3 position, Vector3 normal, Vector2 uv) : position(position), normal(normal), uv(uv) {}
	Vertex(float p1, float p2, float p3, float n1, float n2, float n3, float uv1, float uv2) : Vertex(Vector3(p1, p2, p3), Vector3(n1, n2, n3), Vector2(uv1, uv2)) {}
};

struct Input_Layout {
	~Input_Layout();
	
	static ID3D11InputLayout *vertex_color;
	static ID3D11InputLayout *vertex;
	static ID3D11InputLayout *vertex_xuv;
	static const D3D11_INPUT_ELEMENT_DESC vertex_col_desc[2];
	static const D3D11_INPUT_ELEMENT_DESC vertex_xuv_desc[2];
	static const D3D11_INPUT_ELEMENT_DESC vertex_desc[3];
	
	static void init();
};
#endif