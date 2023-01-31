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
	} else if (geometry_type == GEOMETRY_TYPE_SPHERE) {
		geometry_entity.sphere = *((Sphere *)data);
	} else {
		print("Game_World::make_geometry_entity: Was passed not existing Geometry Type argument.");
		return Entity_Id();
	}
	geometry_entities.push(geometry_entity);
	
	return Entity_Id(ENTITY_TYPE_GEOMETRY, geometry_entities.count - 1);
}

#define UPDATE_LIGHT_HASH() light_hash += (u32)light.light_type + 1

Light *Game_World::make_direction_light(const Vector3 &direction, const Vector3 &color)
{
	Light light;
	init_entity(&light, ENTITY_TYPE_LIGHT, Vector3(0.0f, 0.0f, 0.0f));
	light.direction = direction;
	light.color = color;
	light.light_type = DIRECTIONAL_LIGHT_TYPE;
	lights.push(light);

	UPDATE_LIGHT_HASH();

	return &light;
}

Light *Game_World::make_point_light(const Vector3 &position, const Vector3 &color, float range)
{
	Light light;
	init_entity(&light, ENTITY_TYPE_LIGHT, position);
	light.color = color;
	light.light_type = POINT_LIGHT_TYPE;
	light.range = range;
	lights.push(light);

	UPDATE_LIGHT_HASH();

	return &light;
}

Light  *Game_World::make_spot_light(const Vector3 &position, const Vector3 &direction, const Vector3 &color, float radius)
{
	Light light;
	init_entity(&light, ENTITY_TYPE_LIGHT, position);
	light.direction = direction;
	light.color = color;
	light.light_type = SPOT_LIGHT_TYPE;
	light.radius = radius;
	lights.push(light);

	UPDATE_LIGHT_HASH();

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
