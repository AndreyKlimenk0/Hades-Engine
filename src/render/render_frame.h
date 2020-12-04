#ifndef RENDER_FRAME_H
#define RENDER_FRAME_H

#include <DirectXMath.h>

#include "base.h"
#include "mesh.h"
#include "../framework/camera.h"
#include "../libs/ds/array.h"

struct Render_World {
	Direct3D *direct3d = NULL;
	Win32_State *win32 = NULL;
	Free_Camera *camera  = NULL;
	Array<Triangle_Mesh *> meshes;
	
	void init(Direct3D *direct3d, Win32_State *win32, Free_Camera *camera);
	void render_world();
};

void draw_mesh(Direct3D *direct3d, Triangle_Mesh *mesh, Matrix4 world_view_projection);

#endif