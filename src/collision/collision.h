#ifndef _COLLISION_H
#define _COLLISION_H

#include "../render/mesh.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../libs/math/structures.h"

enum Boudning_Box_Type {
	BOUNDING_BOX_TYPE_UNKNOWN,
	BOUNDING_BOX_TYPE_AABB,
	BOUNDING_BOX_TYPE_OBB,
	BOUNDING_BOX_TYPE_SPHERE
};

struct AABB {
	Vector3 min;
	Vector3 max;
};

struct Bounding_Sphere {
	float radious;
	Vector3 postion;
};

template <typename T>
inline AABB make_AABB(T *vertices, u32 vertex_count, const Matrix4 &transformation_matrix)
{
	Vector3 min = { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 max = { -FLT_MAX, -FLT_MAX, -FLT_MAX };

	for (u32 i = 0; i < vertex_count; i++) {
		Vector3 position = vertices[i].position * transformation_matrix;
		min.x = math::min(min.x, position.x);
		min.y = math::min(min.y, position.y);
		min.z = math::min(min.z, position.z);
		max.x = math::max(max.x, position.x);
		max.y = math::max(max.y, position.y);
		max.z = math::max(max.z, position.z);
	}
	return { min, max };
}

Bounding_Sphere make_bounding_sphere(const Vector3 &position, Triangle_Mesh *mesh);

bool detect_intersection(Ray *ray, AABB *aabb, Vector3 *intersection_point = NULL);
bool detect_intersection(float radius, const Vector2 &circle_center, const Vector2 &test_point);

#endif

