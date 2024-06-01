#include <assert.h>

#include "map.h"
#include "sys_local.h"
#include "../libs/mesh_loader.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/ds/array.h"
#include "../libs/ds/hash_table.h"
#include "../libs/math/structures.h"

inline bool operator==(const Mesh_Id &first, const Mesh_Id &second)
{
	return (first.textures_idx == second.textures_idx) && (first.instance_idx == second.instance_idx);
}

inline u32 hash(Mesh_Id mesh_id, int table_count, int attempt)
{
	char *str1 = ::to_string(mesh_id.textures_idx);
	char *str2 = ::to_string(mesh_id.instance_idx);
	String result = String(str1) + String(str2);
	u32 h = ::hash(result.c_str(), table_count, attempt);
	free_string(str1);
	free_string(str2);
	return h;
}

inline void load_game_entities(File *map_file, Game_World *game_world)
{
	assert(map_file);
	assert(game_world);

	map_file->read(&game_world->entities);
	map_file->read(&game_world->lights);
	map_file->read(&game_world->geometry_entities);
	map_file->read(&game_world->cameras);
}

inline void load_saved_meshes(File *map_file, Render_World *render_world)
{
	Array<String> mesh_names;
	Array<u8> unified_strings;
	map_file->read(&unified_strings);

	String mesh_name;
	for (u32 i = 0; i < unified_strings.count; i++) {
		if ((unified_strings[i] == '\0') && !mesh_name.is_empty()) {
			mesh_names.push(mesh_name);
			mesh_name.free();
			continue;
		}
		mesh_name.append(unified_strings[i]);
	}

	Mesh_Loader mesh_loader;
	for (u32 i = 0; i < mesh_names.count; i++) {
		String full_path_to_mesh_file;
		build_full_path_to_model_file(mesh_names[i].c_str(), full_path_to_mesh_file);

		Array<Import_Mesh> meshes;
		if (mesh_loader.load(full_path_to_mesh_file, meshes, false, false)) {
			render_world->triangle_meshes.loaded_meshes.push(mesh_names[i].c_str());
			Import_Mesh *imported_mesh = NULL;
			For(meshes, imported_mesh) {
				Mesh_Id mesh_id;
				render_world->add_mesh(imported_mesh->mesh.name, &imported_mesh->mesh, &mesh_id);
			}
		}

	}
}

inline void make_render_entities(File *map_file, Game_World *game_world, Render_World *render_world)
{
	assert(map_file);
	assert(game_world);
	assert(render_world);

	Array<Pair<Entity_Id, String_Id>> map_render_entities;
	map_file->read(&map_render_entities);

	for (u32 i = 0; i < map_render_entities.count; i++) {
		Pair<Entity_Id, String_Id> *entity = &map_render_entities[i];
		if (game_world->get_entity(entity->first)) {
			Mesh_Id mesh_id;
			if (render_world->triangle_meshes.mesh_table.get(entity->second, &mesh_id)) {
				render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity->first, mesh_id);
			}
		}
	}
}

inline void save_game_entities(File *map_file, Game_World *game_world)
{
	assert(map_file);
	assert(game_world);

	map_file->write(&game_world->entities);
	map_file->write(&game_world->lights);
	map_file->write(&game_world->geometry_entities);
	map_file->write(&game_world->cameras);
}

inline void save_render_entities(File *map_file, Render_World *render_world)
{
	assert(map_file);
	assert(render_world);

	if ((render_world->triangle_meshes.mesh_table.count == 0) || (render_world->game_render_entities.is_empty())) {
		print("save_render_entitties: Render entities can be saved in a map file. There is no meshes or render entities in the render world.");
		return;
	}

	Hash_Table<Mesh_Id, String_Id> table;
	for (u32 i = 0; i < render_world->triangle_meshes.mesh_table.count; i++) {
		Hash_Node<String_Id, Mesh_Id> *node = render_world->triangle_meshes.mesh_table.get_node(i);
		table.set(node->value, node->key);
	}

	Array<Pair<Entity_Id, String_Id>> map_render_entities;

	Render_Entity *render_entity = NULL;
	For(render_world->game_render_entities, render_entity) {
		String_Id mesh_name;
		if (table.get(render_entity->mesh_id, &mesh_name)) {
			Pair<Entity_Id, String_Id> map_render_entity;
			map_render_entity.first = render_entity->entity_id;
			map_render_entity.second = mesh_name;
			map_render_entities.push(map_render_entity);
		}
	}
	map_file->write(&map_render_entities);
}

inline void save_loaded_mesh_names(File *map_file, Array<String> &mesh_names)
{
	assert(map_file);

	Array<u8> unified_strings;
	for (u32 i = 0; i < mesh_names.count; i++) {
		String *mesh_name = &mesh_names[i];
		if (!mesh_name->is_empty()) {
			for (u32 j = 0; j < mesh_name->len; j++) {
				unified_strings.push(mesh_name->data[j]);
			}
			unified_strings.push('\0');
		}
	}
	map_file->write(&unified_strings);
}

void init_game_and_render_world_from_map(const char *map_name, Game_World *game_world, Render_World *render_world)
{
	String full_path_to_map_file;
	build_full_path_to_map_file(map_name, full_path_to_map_file);

	File map_file;
	if (map_file.open(full_path_to_map_file, FILE_MODE_READ, FILE_OPEN_EXISTING)) {
		load_game_entities(&map_file, game_world);
		load_saved_meshes(&map_file, render_world);
		make_render_entities(&map_file, game_world, render_world);
	}
}

void save_game_and_render_world_in_map(const char *map_name, Game_World *game_world, Render_World *render_world)
{
	assert(map_name);

	String full_path_to_map_file;
	build_full_path_to_map_file(map_name, full_path_to_map_file);

	File map_file;
	if (map_file.open(full_path_to_map_file, FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
		save_game_entities(&map_file, game_world);
		save_loaded_mesh_names(&map_file, render_world->triangle_meshes.loaded_meshes);
		save_render_entities(&map_file, render_world);
	}
}