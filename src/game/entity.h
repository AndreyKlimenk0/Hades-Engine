#ifndef ENTITY_H
#define ENTITY_H

#include <stdlib.h>

#include "../render/model.h"
#include "../libs/ds/array.h"
#include "../libs/math/vector.h"


enum Entity_Type {
	ENTITY_TYPE_MUTANT,
	ENTITY_TYPE_SOLDIER,
	ENTITY_TYPE_DRAW_VERTEX
};

struct Entity {
	int id;
	Entity_Type type;
	Vector3 position;

	Model *model;
};

struct Mutant :  Entity {
	Mutant() { type = ENTITY_TYPE_MUTANT; }
};

struct Soldier : Entity {
	Soldier() { type = ENTITY_TYPE_SOLDIER; }
};

struct Entity_Manager {
	int id_count = 0;
	Array<Entity *> entities;

	void add_entity(Entity *entity);
	Entity *create_entity(Entity_Type type);
};

inline Entity *Entity_Manager::create_entity(Entity_Type type)
{
	Entity *entity = NULL;

	switch (type) {
		case ENTITY_TYPE_MUTANT: {
			entity = new Mutant();
			break;
		}
		case ENTITY_TYPE_SOLDIER: {
			entity = new Soldier();
			break;
		}
	}

	entity->id = id_count++;
}

inline void Entity_Manager::add_entity(Entity *entity)
{
	entities.push(entity);
}
#endif