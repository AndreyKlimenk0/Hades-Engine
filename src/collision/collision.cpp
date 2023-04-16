#include "collision.h"
#include "../render/model.h"


AABB make_AABB(Triangle_Mesh *mesh)
{
	Vector3 min = { 0.0f, 0.0f, 0.0f };
	Vector3 max = { 0.0f, 0.0f, 0.0f };
	for (u32 i = 0; i < mesh->vertices.count; i++) {
		Vector3 position = mesh->vertices[i].position;
		if (position.x < min.x) {
			min.x = position.x;
		}
		if (position.y < min.y) {
			min.y = position.y;
		}
		if (position.z > min.z) {
			min.z = position.z;
		}
		if (position.x > max.x) {
			max.x = position.x;
		}
		if (position.y > max.y) {
			max.y = position.y;
		}
		if (position.z < max.z) {
			max.z = position.z;
		}
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
