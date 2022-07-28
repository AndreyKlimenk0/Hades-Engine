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

enum Entity_Type {
	ENTITY_TYPE_ENTITY,
	ENTITY_TYPE_GRID,
	ENTITY_TYPE_BOX,
	ENTITY_TYPE_SPHERE,
	ENTITY_TYPE_FLOOR,
	ENTITY_TYPE_MUTANT,
	ENTITY_TYPE_SOLDIER,
	ENTITY_TYPE_LIGHT
};

struct Entity {
	Entity() { type = ENTITY_TYPE_ENTITY; }
	u32 id;
	Entity_Type type;
	Vector3 position;

	Matrix4 get_world_matrix();
	void get_world_matrix(Matrix4 &matrix);
};

inline Matrix4 Entity::get_world_matrix()
{
	Matrix4 matrix = matrix4_indentity;
	matrix[3] = Vector4(position, 1.0f);
	return matrix;
}

inline void Entity::get_world_matrix(Matrix4 &matrix)
{
	matrix.indentity();
	matrix[3] = Vector4(position, 1.0f);
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
	u32 id_count = 0;
	Array<Light *> lights;
	Array<Entity *> entities;

	void add_entity(Entity *entity);

	Entity *make_grid(const Vector3 &position, float width, float depth, int m, int n);
	Entity *make_box(const Vector3 &position, float width, float height, float depth);
	Entity *make_sphere(const Vector3 &position, float radius, u32 slice_count, u32 stack_count);
	
	Light  *make_spot_light(const Vector3 &position, const Vector3 &diretion, const Vector3 &color, float radius);
	Light  *make_point_light(const Vector3 &position, const Vector3 &color, float range);
	Light  *make_direction_light(const Vector3 &direction, const Vector3 &color);

	Mutant *make_mutant(const Vector3 &position);
};

struct Render_Entity {
	u32 stencil_ref_value = 0;
	Stencil_Test *stencil_test = NULL;

	Entity *entity = NULL;
	Render_Model *render_model = NULL;
	
	void (*call_before_drawing_entity)(Render_Entity *render_entity) = NULL;
	void (*call_after_drawn_entity)(Render_Entity *render_entity) = NULL;
};

void make_render_entity(Entity *entity, const char *model_name);
void make_render_entity(Entity *entity, Render_Model *render_model);

struct Shadow_Map {
	Light *light_entity;
	Texture *depth_map;
};

Shadow_Map *make_shadow_map(Light *light);

struct World {
	Entity_Manager entity_manager;
	Array<Render_Entity *> render_entities;
	Array<Shadow_Map *> shadows_map;

	void init();
	void init_from_map(const char *map_name);
};

extern World world;
#endif