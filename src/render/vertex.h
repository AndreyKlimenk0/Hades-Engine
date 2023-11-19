#ifndef VERTEX_H
#define VERTEX_H

#include <d3d11.h>
#include "../libs/math/vector.h"
#include "../libs/ds/hash_table.h"


struct Vertex_XC {
	Vertex_XC() {}
	Vertex_XC(const Vector2 &position, const Vector4 &color) : position(position), color(color) {}
	
	Vector2 position;
	Vector4 color;
};

struct Vertex_X2UV {
	Vertex_X2UV() {}
	Vertex_X2UV(const Vector2 &position, const Vector2 &uv) : position(position), uv(uv) {}
	
	Vector2 position;
	Vector2 uv;
};

struct Vertex_PNTUV {
	Vertex_PNTUV() {}
	Vertex_PNTUV(Vector3 position, Vector3 normal, Vector3 tangent, Vector2 uv) : position(position), normal(normal), tangent(tangent), uv(uv) {}

	Vector3 position;
	Vector3 normal;
	Vector3 tangent;
	Vector2 uv;
};
#endif