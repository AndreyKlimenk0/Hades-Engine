#include <float.h>

#include "collision.h"
#include "../render/model.h"
#include "../libs/math/functions.h"

AABB make_AABB(Triangle_Mesh *mesh)
{
	Vector3 min = { FLT_MAX, FLT_MAX, FLT_MAX };
	Vector3 max = { FLT_MIN, FLT_MIN, FLT_MIN };
;
	for (u32 i = 0; i < mesh->vertices.count; i++) {
		Vector3 position = mesh->vertices[i].position;
		min.x = math::min(min.x, position.x);
		min.y = math::min(min.y, position.y);
		min.z = math::min(min.z, position.z);
		max.x = math::max(max.x, position.x);
		max.y = math::max(max.y, position.y);
		max.z = math::max(max.z, position.z);
	}
	return { min, max };
}

Bounding_Sphere make_bounding_sphere(const Vector3 &position, Triangle_Mesh *mesh)
{
	Bounding_Sphere bounding_sphere;
	bounding_sphere.radious = 0.0f;
	bounding_sphere.postion = position;
	
	for (u32 i = 0; i < mesh->vertices.count; i++) {
		Vector3 position = mesh->vertices[i].position;
		if (position.x > bounding_sphere.radious) {
			bounding_sphere.radious = position.x;
		}
		if (position.y > bounding_sphere.radious) {
			bounding_sphere.radious = position.y;
		}
		if (position.z > bounding_sphere.radious) {
			bounding_sphere.radious = position.z;
		}
	}
	return bounding_sphere;
}
