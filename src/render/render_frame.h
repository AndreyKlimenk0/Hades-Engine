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

struct Render_Info {
	Matrix4 perspective;
	
};

void draw_mesh(Triangle_Mesh *mesh, Matrix4 world_view_projection);
void draw_entities(Array<Entity *> *entities, Matrix4 *world_view);
void draw_entities(Entity_Manager *entity_manager, Matrix4 &view);

#endif