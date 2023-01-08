#ifndef RENDER_WORLD_H
#define RENDER_WORLD_H

#include "model.h"
#include "../libs/ds/array.h"
#include "../game/world.h"
#include "../render/model.h"

typedef u32 Render_Model_Id;

struct Render_Entity {
	Entity_Id entity_id;
	Render_Model_Id render_model_id;
};

struct Render_World {
	Array<Render_Entity> render_entities;
	
	void add_new_entity(Entity_Id entity_id, const char *model_name);
};
#endif