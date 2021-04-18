#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include "../libs/os/camera.h"


struct World {
	Entity_Manager entity_manager;
	Free_Camera *free_camera;

	void init(Free_Camera *camera);
	void draw();
};

#endif