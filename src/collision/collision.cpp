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

#include <algorithm>

bool detect_intersection(Ray *ray, AABB *aabb, Vector3 *intersection_point)
{
	Vector3 normalize_ray_direction = normalize(ray->direction);

	float tmin = (aabb->min.x - ray->origin.x) / normalize_ray_direction.x;
	float tmax = (aabb->max.x - ray->origin.x) / normalize_ray_direction.x;

	if (tmin > tmax) std::swap(tmin, tmax);

	float tymin = (aabb->min.y - ray->origin.y) / normalize_ray_direction.y;
	float tymax = (aabb->max.y - ray->origin.y) / normalize_ray_direction.y;

	if (tymin > tymax) std::swap(tymin, tymax);

	if ((tmin > tymax) || (tymin > tmax))
		return false;

	if (tymin > tmin)
		tmin = tymin;

	if (tymax < tmax)
		tmax = tymax;

	float tzmin = (aabb->min.z - ray->origin.z) / normalize_ray_direction.z;
	float tzmax = (aabb->max.z - ray->origin.z) / normalize_ray_direction.z;

	if (tzmin > tzmax) std::swap(tzmin, tzmax);

	if ((tmin > tzmax) || (tzmin > tmax))
		return false;

	if (tzmin > tmin)
		tmin = tzmin;

	if (tzmax < tmax)
		tmax = tzmax;

	if (intersection_point) {
		*intersection_point = ray->origin + (Vector3)(normalize_ray_direction * tmin);
	}
	return true;
}
