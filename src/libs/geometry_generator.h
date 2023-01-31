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

void generate_grid(Grid *grid, Triangle_Mesh *mesh);
void generate_box(Box *box,  Triangle_Mesh *mesh);
void generate_sphere(Sphere *sphere, Triangle_Mesh *mesh);

#endif