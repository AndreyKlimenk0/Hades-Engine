#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include <DirectXMath.h>

#include "base.h"
#include "mesh.h"
#include "../framework/camera.h"
#include "../libs/ds/array.h"
#include "../ui/element.h"

struct Render_World {
	Free_Camera *camera  = NULL;
	Array<Triangle_Mesh *> meshes;

	void init(Free_Camera *camera);
	void render_world(Editor *editor);
};

void draw_mesh(Triangle_Mesh *mesh, Matrix4 world_view_projection);

#endif