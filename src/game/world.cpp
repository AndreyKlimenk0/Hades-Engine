#include "world.h"
#include "../libs/color.h"
#include "../libs/geometry_generator.h"
#include "../sys/engine.h"


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

Light *Game_World::make_direction_light(const Vector3 &direction, const Vector3 &color)
{
	Light light;
	light.direction = direction;
	light.color = color;
	light.light_type = DIRECTIONAL_LIGHT_TYPE;
	add_entity(&light);

	return &light;
}

Light *Game_World::make_point_light(const Vector3 &position, const Vector3 &color, float range)
{
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
