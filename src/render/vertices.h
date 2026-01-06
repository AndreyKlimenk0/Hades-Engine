#ifndef VERTICES_H
#define VERTICES_H

#include "../libs/math/vector.h"

struct Vertex_XC {
	Vertex_XC() {}
	Vertex_XC(const Vector2 &position, const Vector4 &color) : position(position), color(color) {}

	Vector2 position;
	Vector4 color;
};

struct Vertex_P2UV {
	Vertex_P2UV() {}
	Vertex_P2UV(const Vector2 &position, const Vector2 &uv) : position(position), uv(uv) {}

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
