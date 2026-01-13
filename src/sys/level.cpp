#include <assert.h>

#include "sys.h"
#include "vars.h"
#include "level.h"
#include "engine.h"
#include "profiling.h"

#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/mesh_loader.h"
#include "../libs/math/structures.h"
#include "../libs/structures/array.h"
#include "../libs/structures/hash_table.h"

#include "../render/render_world.h"

inline u32 hash(Mesh_Idx mesh_idx, int table_count, int attempt)
{
	char *str = ::to_string(mesh_idx);
	String result = String(str);
	u32 h = ::hash(result.c_str(), table_count, attempt);
	free_string(str);
	return h;
}

inline void load_game_entities(File *level_file, Game_World *game_world)
{
	assert(level_file);
	assert(game_world);

	level_file->read(&game_world->entities);
	level_file->read(&game_world->lights);
	level_file->read(&game_world->geometry_entities);
	level_file->read(&game_world->cameras);
}

inline void load_saved_meshes(File *level_file, Render_World *render_world)
{
	Array<String> mesh_names;
	Array<u8> unified_strings;
	level_file->read(&unified_strings);

	String mesh_name;
	for (u32 i = 0; i < unified_strings.count; i++) {
		if ((unified_strings[i] == '\0') && !mesh_name.is_empty()) {
			mesh_names.push(mesh_name);
			mesh_name.free();
			continue;
		}
		mesh_name.append(unified_strings[i]);
	}

	Variable_Service *variable_service = Engine::get_variable_service();
	Variable_Service *models_loading = variable_service->find_namespace("models_loading");

	Loading_Models_Options loading_options;
	models_loading->attach("scene_logging", &loading_options.scene_logging);
	models_loading->attach("assimp_logging", &loading_options.assimp_logging);
	models_loading->attach("scaling_value", &loading_options.scaling_value);
	models_loading->attach("use_scaling_value", &loading_options.use_scaling_value);

	for (u32 i = 0; i < mesh_names.count; i++) {
		String full_path_to_mesh_file;
		build_full_path_to_model_file(mesh_names[i].c_str(), full_path_to_mesh_file);

		Loading_Models_Info info;
		Array<Loading_Model *> loaded_models;
		if (load_models_from_file(full_path_to_mesh_file, loaded_models, &info, &loading_options)) {
			begin_time_stamp();
			Model_Storage *model_storage = render_world->get_model_storage();
			
			Array<Pair<Loading_Model *, Mesh_Idx>> result;
			model_storage->add_models(loaded_models, result);

			free_memory(&loaded_models);

			print("load_saved_meshes: {} was loaded in render world for {}ms", mesh_names[i].c_str(), delta_time_in_milliseconds());
		}
	}
}

inline void init_render_world(File *level_file, Game_World *game_world, Render_World *render_world)
{
	assert(level_file);
	assert(game_world);
	assert(render_world);

	Array<Pair<Entity_Id, String_Id>> level_render_entities;
	level_file->read(&level_render_entities);

	for (u32 i = 0; i < level_render_entities.count; i++) {
		Pair<Entity_Id, String_Id> *entity = &level_render_entities[i];
		if (game_world->get_entity(entity->first)) {
			Pair<Render_Model *, u32> temp;
			if (render_world->model_storage.render_models_table.get(entity->second, &temp)) {
				render_world->add_render_entity(entity->first, temp.second);
			}
		}
	}
	render_world->upload_lights();
}

inline void save_game_entities(File *level_file, Game_World *game_world)
{
	assert(level_file);
	assert(game_world);

	level_file->write(&game_world->entities);
	level_file->write(&game_world->lights);
	level_file->write(&game_world->geometry_entities);
	level_file->write(&game_world->cameras);
}

inline void save_render_entities(File *level_file, Render_World *render_world)
{
	assert(level_file);
	assert(render_world);

	if ((render_world->model_storage.render_models_table.count == 0) || (render_world->game_render_entities.is_empty())) {
		print("save_render_entitties: Render entities can be saved in a level file. There is no meshes or render entities in the render world.");
		return;
	}

	Hash_Table<Mesh_Idx, String_Id> table;
	for (u32 i = 0; i < render_world->model_storage.render_models_table.count; i++) {
		Hash_Node<String_Id, Pair<Render_Model *, u32>> *node = render_world->model_storage.render_models_table.get_node(i);
		table.set(node->value.second, node->key);
	}

	Array<Pair<Entity_Id, String_Id>> level_render_entities;

	Render_Entity *render_entity = NULL;
	For(render_world->game_render_entities, render_entity) {
		String_Id mesh_name;
		if (table.get(render_entity->mesh_idx, &mesh_name)) {
			Pair<Entity_Id, String_Id> level_render_entity;
			level_render_entity.first = render_entity->entity_id;
			level_render_entity.second = mesh_name;
			level_render_entities.push(level_render_entity);
		}
	}
	level_file->write(&level_render_entities);
}

inline void save_loaded_mesh_names(File *level_file, Array<Render_Model *> &render_models)
{
	assert(level_file);

	Array<u8> unified_strings;
	for (u32 i = 0; i < render_models.count; i++) {
		String *mesh_name = &render_models[i]->file_name;
		if (!mesh_name->is_empty()) {
			for (u32 j = 0; j < mesh_name->len; j++) {
				unified_strings.push(mesh_name->data[j]);
			}
			unified_strings.push('\0');
		}
	}
	level_file->write(&unified_strings);
}

bool load_level(const char *level_name, Game_World *game_world, Render_World *render_world)
{
	String full_path_to_level_file;
	build_full_path_to_level_file(level_name, full_path_to_level_file);

	File level_file;
	if (level_file.open(full_path_to_level_file, FILE_MODE_READ, FILE_OPEN_EXISTING)) {
		load_game_entities(&level_file, game_world);
		load_saved_meshes(&level_file, render_world);
		init_render_world(&level_file, game_world, render_world);
		return true;
	}
	return false;
}

void save_level(const char *level_name, Game_World *game_world, Render_World *render_world)
{
	assert(level_name);

	String full_path_to_level_file;
	build_full_path_to_level_file(level_name, full_path_to_level_file);

	File level_file;
	if (level_file.open(full_path_to_level_file, FILE_MODE_WRITE, FILE_CREATE_ALWAYS)) {
		save_game_entities(&level_file, game_world);
		save_loaded_mesh_names(&level_file, render_world->model_storage.render_models);
		save_render_entities(&level_file, render_world);
	}
}