#ifndef ENTITY_H
#define ENTITY_H

#include <stdlib.h>

#include "../render/model.h"
#include "../libs/ds/array.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../win32/win_types.h"

const u32 MAX_NUMBER_LIGHT_IN_WORLD = 255;

enum Entity_Type {
	ENTITY_TYPE_UNKNOWN,
	ENTITY_TYPE_FLOOR,
	ENTITY_TYPE_MUTANT,
	ENTITY_TYPE_SOLDIER,
	ENTITY_TYPE_LIGHT
};

struct Entity {
	Entity() { type = ENTITY_TYPE_UNKNOWN; }
	int id;
	Entity_Type type;
	Vector3 position;

	Render_Model *model = NULL;

	Matrix4 get_world_matrix();
};

inline Matrix4 Entity::get_world_matrix()
{
	Matrix4 matrix = matrix4_indentity;
	matrix[3] = Vector4(position, 1.0f);
	return matrix;
}

struct Floor : Entity {
	Floor() { type = ENTITY_TYPE_FLOOR; }
};

struct Mutant : Entity {
	Mutant() { type = ENTITY_TYPE_MUTANT; }
};

struct Soldier : Entity {
	Soldier() { type = ENTITY_TYPE_SOLDIER; }
};

enum Light_Type : u32 {
	SPOT_LIGHT_TYPE = 0,
	POINT_LIGHT_TYPE = 1,
	DIRECTIONAL_LIGHT_TYPE = 2,
};

struct Light : Entity {
	Light() { type = ENTITY_TYPE_LIGHT; }
	
	Light_Type light_type;
	
	float range;
	float radius;

	Vector3 color;
	Vector3 direction;
};

struct Entity_Manager {
	int id_count = 0;
	Array<Light *> lights;
	Array<Entity *> entities;

	void add_light(Light *light);
	void add_entity(Entity *entity);
	Entity *make_entity(Entity_Type type, const Vector3 &position);
	Light  *make_light(const Vector3 &position, const Vector3 direction, const Vector3 &color, Light_Type light_type);
	Light  *make_point_light(const Vector3 &position, const Vector3 &color, float range);
	Light  *make_spot_light(const Vector3 &position, const Vector3 &diretion, const Vector3 &color, float radius);
};

inline void Entity_Manager::add_entity(Entity *entity)
{
	entities.push(entity);
}

inline void Entity_Manager::add_light(Light *light)
{
	if (lights.count >= MAX_NUMBER_LIGHT_IN_WORLD) {
		print("In the world already there is max number of lights, this light will not add to the world");
	} else {
		lights.push(light);
		entities.push(light);
	}
}
#endif