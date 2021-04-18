#ifndef GEOMETRY_GENERATOR_H
#define GEOMETRY_GENERATOR_H

#include "../render/mesh.h"


void generate_grid(float width, float depth, int m, int n, Triangle_Mesh *mesh);
void generate_box(float width, float height, float depth,  Triangle_Mesh *mesh);

#endif