#ifndef WORLD_H
#define WORLD_H

#include <stdlib.h>
#include <stdint.h>

#include "../libs/geometry.h"
#include "../libs/math/vector.h"
#include "../libs/number_types.h"
#include "../libs/structures/array.h"
#include "../collision/collision.h"


enum Entity_Type : u32 {
	ENTITY_TYPE_UNKNOWN = 0,
	ENTITY_TYPE_ENTITY,
	ENTITY_TYPE_LIGHT,
	ENTITY_TYPE_GEOMETRY,
	ENTITY_TYPE_CAMERA,
};

struct Entity_Id {
	Entity_Id();
	Entity_Id(Entity_Type type, u32 index);

	Entity_Type type;
	u32 index;

	void reset();
};

bool operator==(const Entity_Id &first, const Entity_Id &second);
bool operator!=(const Entity_Id &first, const Entity_Id &second);

struct Entity {
	Entity() { type = ENTITY_TYPE_ENTITY; bounding_box_type = BOUNDING_BOX_TYPE_UNKNOWN; }
	u32 idx;
	Entity_Type type;

	Vector3 scaling;
	Vector3 rotation;
	Vector3 position;

	//@Note: Why is this here ?
	Boudning_Box_Type bounding_box_type;
	AABB AABB_box;
};

inline Entity_Id get_entity_id(Entity *entity)
{
	//@Note: Should entity_id field be in the Entity struct ?
	return Entity_Id(entity->type, entity->idx);
}

inline bool valid_entity_id(Entity_Id entity_id)
{
	return ((entity_id.type == ENTITY_TYPE_UNKNOWN) && (entity_id.index == UINT32_MAX)) ? false : true;
}

enum Geometry_Type {
	GEOMETRY_TYPE_BOX,
	GEOMETRY_TYPE_GRID,
	GEOMETRY_TYPE_SPHERE
};

struct Geometry_Entity : Entity {
	Geometry_Entity() { type = ENTITY_TYPE_GEOMETRY; }
	Geometry_Type geometry_type;
	union {
		Box box;
		Grid grid;
		Sphere sphere;
	};
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

enum Entity_Command_Type {
	ENTITY_COMMAND_NONE,
	ENTITY_COMMAND_MOVE,
	ENTITY_COMMAND_ROTATE,
};

struct Entity_Command {
	Entity_Command() { type = ENTITY_COMMAND_NONE; }
	Entity_Command_Type type;
};

enum Move_Direction {
	MOVE_DIRECTION_FORWARD,
	MOVE_DIRECTION_BACK,
	MOVE_DIRECTION_LEFT,
	MOVE_DIRECTION_RIGHT,
	MOVE_DIRECTION_UP,
	MOVE_DIRECTION_DOWN,
};

struct Entity_Command_Move : Entity_Command {
	Entity_Command_Move() { type = ENTITY_COMMAND_MOVE; }
	Move_Direction move_direction;
	float distance;
};

struct Entity_Command_Rotate : Entity_Command {
	Entity_Command_Rotate() { type = ENTITY_COMMAND_ROTATE; }
	float x_angle = 0.0f;
	float y_angle = 0.0f;
	float z_angle = 0.0f;
};

struct Camera : Entity {
	Vector3 up;
	Vector3 target;

	void handle_commands(Array<Entity_Command *> *entity_commands);
};

struct Group {
	String name;
	AABB AABB_box;
	Boudning_Box_Type bounding_box_type;
	Array<Entity> entities;
};

struct Game_World {

	Array<Entity> entities;
	Array<Camera> cameras;
	Array<Light> lights;
	Array<Geometry_Entity> geometry_entities;

	void init();
	void release_all_resources();

	void delete_entity(Entity_Id entity_id);

	void attach_AABB(Entity_Id entity_id, AABB *bounding_box);
	void move_entity(Entity *entity, const Vector3 &displacement);
	void place_entity(Entity *entity, const Vector3 &position);
	void update_light_direction(Light *light, const Vector3 &direction);

	Entity *get_entity(Entity_Id entity_id);
	Camera *get_camera(Entity_Id entity_id);

	Entity_Id make_entity(const Vector3 &position);
	Entity_Id make_entity(const Vector3 &scaling, const Vector3 &rotation, const Vector3 &position);

	Entity_Id make_camera(const Vector3 &position, const Vector3 &target);

	Entity_Id make_geometry_entity(const Vector3 &position, Geometry_Type geometry_type, void *data);

	Entity_Id make_spot_light(const Vector3 &position, const Vector3 &diretion, const Vector3 &color, float radius);
	Entity_Id make_point_light(const Vector3 &position, const Vector3 &color, float range);
	Entity_Id make_direction_light(const Vector3 &direction, const Vector3 &color);

};
#endif
