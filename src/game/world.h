#ifndef WORLD_H
#define WORLD_H

#include "entity.h"
#include "../libs/os/camera.h"


struct World {
	Entity_Manager entity_manager;

	void init();
};

#endif