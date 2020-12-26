#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include <DirectXMath.h>

#include "base.h"
#include "mesh.h"
#include "../framework/camera.h"
#include "../libs/ds/array.h"

struct Render_World {
	Free_Camera *camera  = NULL;
	Array<Triangle_Mesh *> meshes;
	ID3D11ShaderResourceView *texture = NULL;
	void init(Free_Camera *camera);
	void render_world();
};

void draw_mesh(Triangle_Mesh *mesh, Matrix4 world_view_projection, ID3D11ShaderResourceView *texture);

#endif