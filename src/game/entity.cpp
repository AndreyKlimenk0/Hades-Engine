#include "entity.h"

Entity *Entity_Manager::make_entity(Entity_Type type, const Vector3 &position)
{
	Entity *entity = NULL;

	switch (type) {
	case ENTITY_TYPE_MUTANT:
	{
		entity = new Mutant();
		break;
	}
	case ENTITY_TYPE_SOLDIER:
	{
		entity = new Soldier();
		break;
	}
	case ENTITY_TYPE_LIGHT:
	{
		entity = new Light();
		break;
	}
	}

	entity->id = id_count++;
	entity->position = position;
	return entity;
}

Light *Entity_Manager::make_light(const Vector3 &position, const Vector3 direction, const Vector3 &color, Light_Type light_type)
{
	Light *light = (Light *)make_entity(ENTITY_TYPE_LIGHT, position);
	light->direction = direction;
	light->color = color;
	light->light_type = light_type;

	add_light(light);

	return light;
}

Light *Entity_Manager::make_point_light(const Vector3 &position, const Vector3 &color, float range)
{
	Light *light = (Light *)make_entity(ENTITY_TYPE_LIGHT, position);
	light->color = color;
	light->light_type = POINT_LIGHT_TYPE;
	light->range = range;

	add_light(light);
	
	return light;
}

Light  *Entity_Manager::make_spot_light(const Vector3 &position, const Vector3 &direction, const Vector3 &color, float radius)
{
	Light *light = (Light *)make_entity(ENTITY_TYPE_LIGHT, position);
	light->direction = direction;
	light->color = color;
	light->light_type = SPOT_LIGHT_TYPE;
	light->radius = radius;

	add_light(light);

	return light;
}