#include "world.h"
#include "../libs/color.h"
#include "../libs/geometry_helper.h"
#include "../sys/engine.h"


inline void init_entity(Entity *entity, Entity_Type type, const Vector3 &position)
{
	entity->type = type;
	entity->scaling = Vector3(1.0f, 1.0f, 1.0f);
	entity->rotation = Vector3(0.0f, 0.0f, 0.0f);
	entity->position = position;
}

inline void init_entity(Entity *entity, Entity_Type type, const Vector3 &scaling, const Vector3 &rotation, const Vector3 position)
{
	entity->type = type;
	entity->scaling = scaling;
	entity->rotation = rotation;
	entity->position = position;
}

Entity *Game_World::get_entity(Entity_Id entity_id)
{
	switch (entity_id.type) {
		case ENTITY_TYPE_ENTITY:
			return &entities[entity_id.index];
		case ENTITY_TYPE_LIGHT:
			return &lights[entity_id.index];
		case ENTITY_TYPE_GEOMETRY:
			return &geometry_entities[entity_id.index];
		case ENTITY_TYPE_CAMERA:
			return get_camera(entity_id);
	}
	return NULL;
}

Camera *Game_World::get_camera(Entity_Id entity_id)
{
	if ((entity_id.type == ENTITY_TYPE_CAMERA) && (entity_id.index < cameras.count)) {
		return &cameras[entity_id.index];
	}
	print("Game_World::get_camera: Failed to get a camera. The entity id not valied.");
	return NULL;
}

Entity_Id Game_World::make_entity(const Vector3 &position)
{
	Entity entity;
	init_entity(&entity, ENTITY_TYPE_ENTITY, position);
	entity.idx = entities.count;
	entities.push(entity);
	return get_entity_id(&entity);
}

Entity_Id Game_World::make_entity(const Vector3 &scaling, const Vector3 &rotation, const Vector3 &position)
{
	Entity entity;
	init_entity(&entity, ENTITY_TYPE_ENTITY, scaling, rotation, position);
	entity.idx = entities.count;
	entities.push(entity);
	return get_entity_id(&entity);
}

Entity_Id Game_World::make_camera(const Vector3 &position, const Vector3 &target)
{
	Camera camera;
	init_entity(&camera, ENTITY_TYPE_CAMERA, position);
	camera.target = target;
	camera.idx = cameras.push(camera);
	return get_entity_id(&camera);
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
	file.read(&cameras);
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
	file.write(&cameras);
}

void Game_World::attach_AABB(Entity_Id entity_id, AABB *bounding_box)
{
	Entity *entity = get_entity(entity_id);
	if (entity) {
		entity->bounding_box_type = BOUNDING_BOX_TYPE_AABB;
		entity->AABB_box.min = bounding_box->min + entity->position;
		entity->AABB_box.max = bounding_box->max + entity->position;
	} else {
		print("Game_World::attach_AABB: Failed to set AABB for a entity. The entity was not found.");
	}
}

void Game_World::move_entity(Entity *entity, const Vector3 &displacement)
{
	entity->position += displacement;
	if (entity->bounding_box_type == BOUNDING_BOX_TYPE_AABB) {
		entity->AABB_box.min += displacement;
		entity->AABB_box.max += displacement;
	}
}

void Game_World::place_entity(Entity *entity, const Vector3 &position)
{
	if (entity->bounding_box_type == BOUNDING_BOX_TYPE_AABB) {
		entity->AABB_box.min -= entity->position;
		entity->AABB_box.max -= entity->position;
		entity->position = position;
		entity->AABB_box.min += position;
		entity->AABB_box.max += position;
	}
}

Entity_Id::Entity_Id()
{
	type = ENTITY_TYPE_UNKNOWN;
	index = UINT32_MAX;
}

Entity_Id::Entity_Id(Entity_Type type, u32 index) : type(type), index(index)
{
}

void Entity_Id::reset()
{
	type = ENTITY_TYPE_UNKNOWN;
	index = UINT32_MAX;
}

void Camera::handle_commands(Array<Entity_Command *> *entity_commands)
{
	Entity_Command *entity_command = NULL;

	For((*entity_commands), entity_command) {
		switch (entity_command->type) {
			case ENTITY_COMMAND_MOVE: {
				Entity_Command_Move *move_command = static_cast<Entity_Command_Move *>(entity_command);
				switch (move_command->move_direction) {
					case MOVE_DIRECTION_FORWARD: {
						Vector3 target_direction = (target - position);
						Vector3 move_distance = normalize(&target_direction) * move_command->distance;
						position += move_distance;
						target += move_distance;
						break;
					}
					case MOVE_DIRECTION_BACK: {
						Vector3 target_direction = (target - position);
						Vector3 move_distance = normalize(&target_direction) * move_command->distance;
						position -= move_distance;
						target -= move_distance;
						break;
					}
					case MOVE_DIRECTION_LEFT: {
						break;
					}
					case MOVE_DIRECTION_RIGHT: {
						break;
					}
					case MOVE_DIRECTION_UP: {
						position.z += move_command->distance;
						target.z += move_command->distance;
						break;
					}
					case MOVE_DIRECTION_DOWN: {
						position.z -= move_command->distance;
						target.z -= move_command->distance;
						break;
					}
				}
				break;
			}
			case ENTITY_COMMAND_ROTATE: {
				Entity_Command_Rotate *rotate_command = static_cast<Entity_Command_Rotate *>(entity_command);

				Matrix4 rotation_matrix = rotate_about_x(rotate_command->y_angle) * rotate_about_y(rotate_command->x_angle);
				//@Note: Why I just don't normalize target vector ?
				Vector3 target_direction = target - position;
				Vector3 normalized_target = normalize(&target_direction);
				target = (normalized_target * rotation_matrix) + position;
				break;
			}
		}
	}
}

bool operator==(const Entity_Id &first, const Entity_Id &second)
{
	if ((first.type == second.type) && (first.index == second.index)) {
		return true;
	}
	return false;
}

bool operator!=(const Entity_Id &first, const Entity_Id &second)
{
	if ((first.type != second.type) && (first.index != second.index)) {
		return true;
	}
	return false;
}
