#include <float.h>

#include "collision.h"
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

#include <algorithm>

bool detect_intersection(Ray *ray, AABB *aabb, Vector3 *intersection_point)
{
	float tmin = (aabb->min.x - ray->origin.x) / ray->direction.x;
	float tmax = (aabb->max.x - ray->origin.x) / ray->direction.x;

	if (tmin > tmax) std::swap(tmin, tmax);

	float tymin = (aabb->min.y - ray->origin.y) / ray->direction.y;
	float tymax = (aabb->max.y - ray->origin.y) / ray->direction.y;

	if (tymin > tymax) std::swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	float tzmin = (aabb->min.z - ray->origin.z) / ray->direction.z;
	float tzmax = (aabb->max.z - ray->origin.z) / ray->direction.z;

	if (tzmin > tzmax) std::swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	if (intersection_point) {
		*intersection_point = ray->origin + (Vector3)(ray->direction * tmin);
	}
	return true;
}

bool detect_intersection(float radius, const Vector2 &circle_center, const Vector2 &test_point)
{
	return find_distance(circle_center, test_point) <= radius;
}
