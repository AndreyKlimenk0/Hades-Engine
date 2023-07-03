#include "world.h"
#include "../libs/color.h"
#include "../libs/geometry_helper.h"
#include "../sys/engine.h"


inline void init_entity(Entity *entity, Entity_Type type, const Vector3 &position)
{
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

Entity_Id Game_World::make_entity(const Vector3 &position)
{
	Entity entity;
	init_entity(&entity, ENTITY_TYPE_COMMON, position);
	entity.idx = entities.count;
	entities.push(entity);
	return get_entity_id(&entity);
}

Entity_Id Game_World::make_geometry_entity(const Vector3 &position, Geometry_Type geometry_type, void *data)
{
	Geometry_Entity geometry_entity;
	init_entity(&geometry_entity, ENTITY_TYPE_GEOMETRY, position);
	
	geometry_entity.geometry_type = geometry_type;
	geometry_entity.idx = geometry_entities.count;
	
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
	return get_entity_id(&geometry_entity);
}

#define UPDATE_LIGHT_HASH() light_hash += (u32)light.light_type + 1

Entity_Id Game_World::make_direction_light(const Vector3 &direction, const Vector3 &color)
{
	Light light;
	init_entity(&light, ENTITY_TYPE_LIGHT, Vector3(0.0f, 100.0f, 0.0f));
	light.direction = direction;
	light.color = color;
	light.light_type = DIRECTIONAL_LIGHT_TYPE;;
	light.idx = lights.count;

	UPDATE_LIGHT_HASH();

	lights.push(light);
	return get_entity_id(&light);
}

Entity_Id Game_World::make_point_light(const Vector3 &position, const Vector3 &color, float range)
{
	Light light;
	init_entity(&light, ENTITY_TYPE_LIGHT, position);
	light.color = color;
	light.light_type = POINT_LIGHT_TYPE;
	light.range = range;
	light.idx = lights.count;

	UPDATE_LIGHT_HASH();

	lights.push(light);
	return get_entity_id(&light);
}

Entity_Id Game_World::make_spot_light(const Vector3 &position, const Vector3 &direction, const Vector3 &color, float radius)
{
	Light light;
	init_entity(&light, ENTITY_TYPE_LIGHT, position);
	light.direction = direction;
	light.color = color;
	light.light_type = SPOT_LIGHT_TYPE;
	light.radius = radius;
	light.idx = lights.count;

	UPDATE_LIGHT_HASH();

	lights.push(light);
	return get_entity_id(&light);
}

void Game_World::init()
{
}

void Game_World::init_from_file()
{
	String full_path_to_map_file;
	build_full_path_to_map_file("temp_map.bmap", full_path_to_map_file);

	File file;
	if (!file.open(full_path_to_map_file, FILE_MODE_READ, FILE_OPEN_EXISTING)) {
		print("Game_World::init_from_file: Failed to init game world from temp_map.bmap file.");
		return;
	}

	file.read(&light_hash);
	file.read(&entities);
	file.read(&lights);
	file.read(&geometry_entities);
}

void Game_World::save_to_file()
{
	String full_path_to_map_file;
	build_full_path_to_map_file("temp_map.bmap", full_path_to_map_file);

	File file;
	if (!file.open(full_path_to_map_file, FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
		print("Game_World::save_to_file: Failed to save game world to temp_map.bmap file.");
		return;
	}
	file.write(&light_hash);
	file.write(&entities);
	file.write(&lights);
	file.write(&geometry_entities);
}

void Game_World::set_entity_AABB(Entity_Id entity_id, AABB *bounding_box)
{
	Entity *entity = get_entity(entity_id);
	if (entity) {
		entity->bounding_box_type = BOUNDING_BOX_TYPE_AABB;
		entity->AABB_box = *bounding_box;
		return;
	}
	print("Game_World::set_entity_AABB: Failed to set AABB for a entity. The entity was not found.");
}

Entity_Id::Entity_Id()
{
}

Entity_Id::Entity_Id(Entity_Type type, u32 index) : type(type), index(index)
{
}

