#include "world.h"
#include "../libs/color.h"
#include "../libs/geometry_generator.h"
#include "../sys/engine.h"


static void init_entity(Entity *entity, Entity_Type type, const Vector3 &position)
{
	static u32 entity_count = 0;
	entity->id = entity_count++;
	entity->type = type;
	entity->position = position;
}

void Game_World::add_entity(Entity *entity)
{
	entity->id = id_count++;
	if (entity->type == ENTITY_TYPE_COMMON) {
		entities.push(*entity);
	} else if (entity->type == ENTITY_TYPE_LIGHT) {
		Light *light = static_cast<Light *>(entity);
		
		if (lights.count < MAX_NUMBER_LIGHT_IN_WORLD) {
			lights.push(*light);
		} else {
			print("In the world already there is max number of lights, this light will not add to the world");
		}
	}
}

Entity *Game_World::get_entity(Entity_Id entity_id)
{
	switch (entity_id.type) {
		case ENTITY_TYPE_COMMON:
			return &entities[entity_id.index];
		case ENTITY_TYPE_LIGHT:
			return &lights[entity_id.index];
		case ENTITY_TYPE_GEOMETRY:
			return &geometry_entities[entity_id.index];
	}
	return NULL;
}

Entity_Id Game_World::make_geometry_entity(const Vector3 &position, Geometry_Type geometry_type, void *data)
{
	Geometry_Entity geometry_entity;
	init_entity(&geometry_entity, ENTITY_TYPE_GEOMETRY, position);
	
	geometry_entity.geometry_type = geometry_type;
	
	if (geometry_type == GEOMETRY_TYPE_BOX) {
		geometry_entity.box = *((Box *)data);
	} else if (geometry_type == GEOMETRY_TYPE_GRID) {
		geometry_entity.grid = *((Grid *)data);
	}
	geometry_entities.push(geometry_entity);
	
	return Entity_Id(ENTITY_TYPE_GEOMETRY, geometry_entities.count - 1);
}

Light *Game_World::make_direction_light(const Vector3 &direction, const Vector3 &color)
{
	assert(false);
	Light light;
	light.direction = direction;
	light.color = color;
	light.light_type = DIRECTIONAL_LIGHT_TYPE;
	add_entity(&light);

	return &light;
}

Light *Game_World::make_point_light(const Vector3 &position, const Vector3 &color, float range)
{
	assert(false);
	Light light;
	light.position = position;
	light.color = color;
	light.light_type = POINT_LIGHT_TYPE;
	light.range = range;
	add_entity(&light);
	
	return &light;
}

Light  *Game_World::make_spot_light(const Vector3 &position, const Vector3 &direction, const Vector3 &color, float radius)
{
	assert(false);
	Light light;
	light.position = position;
	light.direction = direction;
	light.color = color;
	light.light_type = SPOT_LIGHT_TYPE;
	light.radius = radius;
	add_entity(&light);

	return &light;
}

void Game_World::init()
{
	render_world = Engine::get_render_world();
}

Entity_Id::Entity_Id()
{
}

Entity_Id::Entity_Id(Entity_Type type, u32 index) : type(type), index(index)
{
}
