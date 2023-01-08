#ifndef WORLD_H
#define WORLD_H

#include <stdlib.h>

#include "../libs/os/camera.h"
#include "../libs/ds/array.h"
#include "../libs/math/vector.h"
#include "../libs/math/matrix.h"
#include "../win32/win_types.h"
#include "../render/model.h"


const u32 MAX_NUMBER_LIGHT_IN_WORLD = 255;

typedef u32 Entity_Id;
struct Render_World;

enum Entity_Type {
	ENTITY_TYPE_COMMON,
	ENTITY_TYPE_LIGHT
};

struct Entity {
	Entity() { type = ENTITY_TYPE_COMMON; }
	u32 id;
	Entity_Type type;
	Vector3 position;
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

struct Game_World {
	u32 id_count = 0;
	Render_World *render_world = NULL;
	
	Array<Light> lights;
	Array<Entity> entities;

	void init();
	void add_entity(Entity *entity);
	
	Light  *make_spot_light(const Vector3 &position, const Vector3 &diretion, const Vector3 &color, float radius);
	Light  *make_point_light(const Vector3 &position, const Vector3 &color, float range);
	Light  *make_direction_light(const Vector3 &direction, const Vector3 &color);

};
#endif