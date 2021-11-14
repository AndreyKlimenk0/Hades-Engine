#include "world.h"
#include "../libs/color.h"
#include "../libs/geometry_generator.h"

#include "../render/directx.h"

World world;

void World::init()
{

	//entity_manager.make_light(Vector3(0.0f, 200.0f, 100.0f), Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f), DIRECTIONAL_LIGHT_TYPE);
	//entity_manager.make_light(Vector3(0.0f, 200.0f, 100.0f), Vector3(0.57735f, -0.57735f, 0.57735f), Vector3(1.0f, 1.0f, 1.0f), DIRECTIONAL_LIGHT_TYPE);
	entity_manager.make_point_light(Vector3(0.0, 100.0f, 0.0f), Vector3(0.1, 0.4, 0.1), 1000.0f);

	entity_manager.make_entity(ENTITY_TYPE_MUTANT, Vector3(-0.5, -1.0, 0.0));

	entity_manager.make_grid(Vector3(0.0, 0.0, 0.0), 5000.0f, 5000.0f, 50, 50);
	entity_manager.make_sphere(Vector3(400, 60.0, 0.0), 100.0f, 100, 100);
}