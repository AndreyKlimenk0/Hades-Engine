#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "../game/world.h"

#include "sys.h"
#include "vars.h"
#include "level.h"
#include "engine.h"
#include "profiling.h"

#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/mesh_loader.h"
#include "../render/render_world.h"
#include "../collision/collision.h"

static Engine *engine = NULL;
static Game_World *game_world = NULL;
static Render_World *render_world = NULL;
static Variable_Service *variable_service = NULL;

static void load_meshes(Array<String> &mesh_names)
{
	begin_profile_task("Load meshes");
	Variable_Service *models_loading = variable_service->find_namespace("models_loading");
	Loading_Models_Options loading_options;
	models_loading->attach("scene_logging", &loading_options.scene_logging);
	models_loading->attach("assimp_logging", &loading_options.assimp_logging);
	models_loading->attach("scaling_value", &loading_options.scaling_value);
	models_loading->attach("use_scaling_value", &loading_options.use_scaling_value);

	for (u32 i = 0; i < mesh_names.count; i++) {
		begin_time_stamp();

		String full_path_to_mesh;
		build_full_path_to_model_file(mesh_names[i], full_path_to_mesh);

		Loading_Models_Info info;
		Array<String> textures;
		Array<Loading_Model *> loaded_models;
		if (load_models_from_file(full_path_to_mesh, loaded_models, textures, &info, &loading_options)) {
			Model_Storage *model_storage = render_world->get_model_storage();
			
			Array<Pair<Loading_Model *, u32>> result;
			String base_file_name;
			extract_base_file_name(mesh_names[i], base_file_name);
			model_storage->pre_load_textures(textures, base_file_name);
			model_storage->add_models(loaded_models, result);

			begin_profile_task("Make entities for models");
			for (u32 j = 0; j < result.count; j++) {
				Pair<Loading_Model *, u32> pair = result[j];
				u32 mesh_idx = pair.second;
				Loading_Model *loaded_model = pair.first;

				AABB mesh_AABB = make_AABB(&loaded_model->mesh);
				assert(loaded_model->instances.count > 0);

				for (u32 k = 0; k < loaded_model->instances.count; k++) {
					Loading_Model::Transformation transformation = loaded_model->instances[k];
					Entity_Id entity_id = game_world->make_entity(transformation.scaling, transformation.rotation, transformation.translation);
					game_world->attach_AABB(entity_id, &mesh_AABB);
					render_world->add_render_entity(entity_id, mesh_idx);
				}
			}
			end_profile_task();
			free_memory(&loaded_models);
			
			print("load_meshes: {} was loaded in game and render world for {}ms", mesh_names[i].c_str(), delta_time_in_milliseconds());
		}
	}
	end_profile_task();
}

void prepare_for_level_loading(Game_World *game_world)
{
	game_world->entities.reset();
	game_world->cameras.reset();
	game_world->lights.reset();
	game_world->geometry_entities.reset();
}

void prepare_for_level_loading(Render_World *render_world)
{
	render_world->cascaded_shadows_list.reset();
	render_world->cascaded_shadows_info_list.reset();
	//render_world->shadow_cascade_ranges.reset();
	render_world->lights.reset();
	render_world->shadows_atlas.reset();
	render_world->render_entity_world_matrices.reset();
	render_world->cascaded_view_projection_matrices.reset();
	
	render_world->game_render_entities.reset();

	Model_Storage *model_storage = &render_world->model_storage;

	model_storage->textures.reset();
	model_storage->render_models.reset();
	model_storage->textures_table.clear();
	model_storage->render_models_table.clear();
}

static void load_level(Array<String> &command_args)
{
	if (!(command_args.is_empty() || command_args.first().is_empty())) {
		String full_path_to_level_file;
		build_full_path_to_level_file(command_args.first(), full_path_to_level_file);
		if (file_exists(full_path_to_level_file)) {
			save_level(engine->current_level_name, game_world, render_world);

			engine->current_level_name = command_args.first();
			
			prepare_for_level_loading(game_world);
			prepare_for_level_loading(render_world);

			load_level(engine->current_level_name, game_world, render_world);
		} else {
			print("load_level: Can not load a level. {} does not exist.", command_args.first());
		}
	} else {
		print("load_level: The command can't get a level name, agruments is not valid.");
	}
}

static void create_level(Array<String> &command_args)
{
	if (!(command_args.is_empty() || command_args.first().is_empty())) {
		save_level(engine->current_level_name, game_world, render_world);
		
		engine->set_current_level_name(command_args.first());
		
		prepare_for_level_loading(game_world);
		prepare_for_level_loading(render_world);

		Entity_Id camera_id = game_world->make_camera(Vector3(0.0f, 20.0f, -250.0f), Vector3(0.0f, 0.0f, -1.0f));
		engine->render_world.set_rendering_view(camera_id);
	} else {
		print("create_level: The command can't get a level name, agruments is not valid.");
	}
}

struct Command {
	String name;
	void (*procedure)(Array<String> &args) = NULL;
};

static Array<Command> commands;

static void add_command(const char *command_name, void (*procedure)(Array<String> &command_args))
{
	assert(command_name);
	assert(strlen(command_name) > 0);
	assert(procedure);

	Command command;
	command.name = command_name;
	command.name.to_lower();
	command.procedure = procedure;
	commands.push(command);
}

void init_commands(Engine *_engine)
{
	engine = _engine;
	game_world = &engine->game_world;
	render_world = &engine->render_world;
	variable_service = &engine->var_service;

	add_command("load mesh", load_meshes);
	add_command("load level", load_level);
	add_command("create level", create_level);
}

void run_command(const char *command_name, Array<String> &command_args)
{
	assert(command_name);

	String temp_command_name = command_name;
	temp_command_name.to_lower();

	bool command_found = false;
	for (u32 i = 0; i < commands.count; i++) {
		if (commands[i].name == temp_command_name) {
			commands[i].procedure(command_args);
			command_found = true;
			break;
		}
	}
	if (!command_found) {
		print("run_command: Command '{}' was not found.", temp_command_name);
	}
}