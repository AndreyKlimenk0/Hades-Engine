#include "world.h"
#include "../libs/color.h"
#include "../libs/geometry_generator.h"

#include "../render/directx.h"

World world;


void Entity_Manager::add_entity(Entity *entity)
{
	entity->id = id_count++;
	entities.push(entity);
	
	if (entity->type == ENTITY_TYPE_LIGHT) {
		Light *light = static_cast<Light *>(entity);
		
		if (lights.count < MAX_NUMBER_LIGHT_IN_WORLD) {
			lights.push(light);
			entities.push(light);
		} else {
			print("In the world already there is max number of lights, this light will not add to the world");
		}
	}
}

Entity *Entity_Manager::make_entity(Entity_Type type)
{
	Entity *entity = NULL;

	switch (type) {
		case ENTITY_TYPE_MUTANT: {
			entity = new Mutant();
			entity->model = model_manager.get_render_model("mutant.fbx");
			break;
		}
		case ENTITY_TYPE_SOLDIER: {
			entity = new Soldier();
			break;
		}
		case ENTITY_TYPE_LIGHT: {
			entity = new Light();
			break;
		}
		case ENTITY_TYPE_UNKNOWN: {
			entity = new Entity();
			break;
		}
	}
	
	add_entity(entity);
	return entity;
}

Entity *Entity_Manager::make_entity(Entity_Type entity_type, const Vector3 &position)
{
	Entity *entity = make_entity(entity_type);
	entity->position = position;
	return entity;
}

Light *Entity_Manager::make_direction_light(const Vector3 &direction, const Vector3 &color)
{
	Light *light = (Light *)make_entity(ENTITY_TYPE_LIGHT);
	light->direction = direction;
	light->color = color;
	light->light_type = DIRECTIONAL_LIGHT_TYPE;

	return light;
}

Light *Entity_Manager::make_point_light(const Vector3 &position, const Vector3 &color, float range)
{
	Light *light = (Light *)make_entity(ENTITY_TYPE_LIGHT, position);
	light->color = color;
	light->light_type = POINT_LIGHT_TYPE;
	light->range = range;
	
	return light;
}

Light  *Entity_Manager::make_spot_light(const Vector3 &position, const Vector3 &direction, const Vector3 &color, float radius)
{
	Light *light = (Light *)make_entity(ENTITY_TYPE_LIGHT, position);
	light->direction = direction;
	light->color = color;
	light->light_type = SPOT_LIGHT_TYPE;
	light->radius = radius;

	return light;
}

Entity *Entity_Manager::make_grid(const Vector3 &position, float width, float depth, int m, int n)
{
	String model_name = String("grid_") + String((int)width) + String((int)depth) + String(m) + String(n);
	Render_Model *render_model = model_manager.make_render_model(model_name);
	Render_Mesh *render_mesh = render_model->get_render_mesh();
	render_mesh->material.specular = Vector4(0.1f, 0.1f, 0.1f, 4.0f);
	render_mesh->diffuse_texture = texture_manager.get_texture("floor.png");

	generate_grid(width, depth, m, n, render_model->get_triangle_mesh());

	Entity *entity = make_entity(ENTITY_TYPE_UNKNOWN, position);
	entity->model = render_model;
	return entity;
}

Entity *Entity_Manager::make_box(const Vector3 &position, float width, float height, float depth)
{
	return NULL;
}

Entity *Entity_Manager::make_sphere(const Vector3 &position, float radius, u32 slice_count, u32 stack_count)
{
	String model_name = String("sphere_") + String((int)radius) + String((int)slice_count) + String((int)stack_count);
	Render_Model *render_model = model_manager.make_render_model(model_name);

	generate_sphere(radius, slice_count, stack_count, render_model->get_triangle_mesh());

	Entity *entity = make_entity(ENTITY_TYPE_UNKNOWN, position);
	entity->model = render_model;
	return entity;
}

void World::init()
{

	//entity_manager.make_light(Vector3(0.0f, 200.0f, 100.0f), Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f), DIRECTIONAL_LIGHT_TYPE);
	//entity_manager.make_light(Vector3(0.0f, 200.0f, 100.0f), Vector3(0.57735f, -0.57735f, 0.57735f), Vector3(1.0f, 1.0f, 1.0f), DIRECTIONAL_LIGHT_TYPE);
	entity_manager.make_point_light(Vector3(0.0, 100.0f, 0.0f), Vector3(0.1, 0.4, 0.1), 1000.0f);

	entity_manager.make_entity(ENTITY_TYPE_MUTANT, Vector3(-0.5, -1.0, 0.0));

	entity_manager.make_grid(Vector3(0.0, 0.0, 0.0), 5000.0f, 5000.0f, 50, 50);
	entity_manager.make_sphere(Vector3(400, 60.0, 0.0), 100.0f, 100, 100);
}