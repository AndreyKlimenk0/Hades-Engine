#ifndef GEOMETRY_GENERATOR_H
#define GEOMETRY_GENERATOR_H

#include "../render/mesh.h"

void generate_grid(int x, int z, Triangle_Mesh *mesh);
void generate_box(Triangle_Mesh *mesh);

#endif