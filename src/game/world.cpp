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
		} else {
			print("In the world already there is max number of lights, this light will not add to the world");
		}
	}
}

Light *Entity_Manager::make_direction_light(const Vector3 &direction, const Vector3 &color)
{
	Light *light = new Light;
	light->direction = direction;
	light->color = color;
	light->light_type = DIRECTIONAL_LIGHT_TYPE;
	add_entity(light);

	return light;
}

Light *Entity_Manager::make_point_light(const Vector3 &position, const Vector3 &color, float range)
{
	Light *light = new Light;
	light->position = position;
	light->color = color;
	light->light_type = POINT_LIGHT_TYPE;
	light->range = range;
	add_entity(light);
	
	return light;
}

Light  *Entity_Manager::make_spot_light(const Vector3 &position, const Vector3 &direction, const Vector3 &color, float radius)
{
	Light *light = new Light;
	light->position = position;
	light->direction = direction;
	light->color = color;
	light->light_type = SPOT_LIGHT_TYPE;
	light->radius = radius;
	add_entity(light);

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

	Entity *entity = new Entity;
	entity->position = position;
	add_entity(entity);
	make_render_entity(entity, render_model);
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

	Entity *entity = new Entity;
	entity->position = position;
	add_entity(entity);
	make_render_entity(entity, render_model);
	return entity;
}

Mutant *Entity_Manager::make_mutant(const Vector3 &position)
{
	Mutant *mutant = new Mutant();
	mutant->position = position;
	add_entity(mutant);
	make_render_entity(mutant, "mutant.fbx");
	return mutant;
}

void make_render_entity(Entity *entity, const char *model_name)
{
	Render_Entity *render_entity = new Render_Entity();
	render_entity->entity = entity;
	render_entity->render_model = model_manager.get_render_model(model_name);

	world.render_entities.push(render_entity);
}

void make_render_entity(Entity *entity, Render_Model *render_model)
{
	Render_Entity *render_entity = new Render_Entity();
	render_entity->entity = entity;
	render_entity->render_model = render_model;

	world.render_entities.push(render_entity);
}

Shadow_Map * make_shadow_map(Light * light)
{
	Shadow_Map *shadow_map = new Shadow_Map();
	shadow_map->light_entity = light;
	shadow_map->depth_map = texture_manager.create_texture(1024, 1024, DXGI_FORMAT_D32_FLOAT);
	return shadow_map;
}

void World::init()
{

	//entity_manager.make_light(Vector3(0.0f, 200.0f, 100.0f), Vector3(1.0f, 1.0f, 1.0f), Vector3(1.0f, 1.0f, 1.0f), DIRECTIONAL_LIGHT_TYPE);
	//entity_manager.make_light(Vector3(0.0f, 200.0f, 100.0f), Vector3(0.57735f, -0.57735f, 0.57735f), Vector3(1.0f, 1.0f, 1.0f), DIRECTIONAL_LIGHT_TYPE);
	//entity_manager.make_point_light(Vector3(0.0, 100.0f, 0.0f), Vector3(0.1, 0.4, 0.1), 1000.0f);
	entity_manager.make_direction_light(Vector3(0.0f, -1.0f, -0.5f), Vector3(1.0f, 1.0f, 1.0f));

	entity_manager.make_mutant(Vector3(-0.5, -1.0, 0.0));

	entity_manager.make_sphere(Vector3(400, 100.0, 0.0), 100.0f, 100, 100);
	entity_manager.make_grid(Vector3(0.0, 0.0, 0.0), 5000.0f, 5000.0f, 50, 50);
}
