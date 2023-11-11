#ifndef GEOMETRY_GENERATOR_H
#define GEOMETRY_GENERATOR_H

#include "../render/model.h"
#include "../win32/win_types.h"

struct Box {
	float width = 10.0f;
	float height = 10.0f;
	float depth = 10.0f;
};

struct Grid {
	float width;
	float depth;
	u32 rows_count;
	u32 columns_count;
};

struct Sphere {
	float radius = 10.0f;
	u32 slice_count = 100;
	u32 stack_count = 100;
};

void make_grid_mesh(Grid *grid, Triangle_Mesh *mesh);
void make_box_mesh(Box *box,  Triangle_Mesh *mesh);
void make_sphere_mesh(Sphere *sphere, Triangle_Mesh *mesh);
void make_AABB_mesh(Vector3 *min, Vector3 *max, Line_Mesh *mesh);
void make_frustum_mesh(float fov, float aspect_ratio, float near_plane, float far_plane, Line_Mesh *mesh);

#endif