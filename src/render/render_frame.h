#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include <DirectXMath.h>

#include "base.h"
#include "mesh.h"

#include "../libs/ds/array.h"
#include "../libs/os/camera.h"

#include "../game/entity.h"

struct Render_World {
	Free_Camera *camera  = NULL;
	Array<Triangle_Mesh *> meshes;
	Array<Entity *> entities;

	void init(Free_Camera *camera);
	void render_world();
};

void draw_mesh(Triangle_Mesh *mesh, Matrix4 world_view_projection);

#endif