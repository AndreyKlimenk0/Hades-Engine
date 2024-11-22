#include <assert.h>
#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "../game/world.h"

#include "../sys/sys.h"
#include "../sys/vars.h"
#include "../sys/level.h"
#include "../sys/engine.h"

#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"
#include "../libs/mesh_loader.h"
#include "../render/render_world.h"
#include "../collision/collision.h"

static void load_meshes(Array<String> &mesh_names)
{
	Game_World *game_world = Engine::get_game_world();
	Render_World *render_world = Engine::get_render_world();
	Variable_Service *variable_service = Engine::get_variable_service();
	
	Variable_Service *models_loading = variable_service->find_namespace("models_loading");
	Models_Loading_Options loading_options;
	models_loading->attach("scene_logging", &loading_options.scene_logging);
	models_loading->attach("assimp_logging", &loading_options.assimp_logging);
	models_loading->attach("scaling_value", &loading_options.scaling_value);
	models_loading->attach("use_scaling_value", &loading_options.use_scaling_value);

	for (u32 i = 0; i < mesh_names.count; i++) {
		String full_path_to_mesh;
		build_full_path_to_model_file(mesh_names[i], full_path_to_mesh);

		Array<Loading_Model *> loaded_models;
		if (load_models_from_file(full_path_to_mesh, loaded_models, &loading_options)) {
			render_world->triangle_meshes.loaded_meshes.push(mesh_names[i]);

			Loading_Model *model = NULL;
			For(loaded_models, model) {
				Mesh_Id mesh_id;
				if (render_world->add_triangle_mesh(&model->mesh, &mesh_id)) {
					AABB mesh_AABB = make_AABB(&model->mesh);
					assert(model->instances.count > 0);
					for (u32 j = 0; j < model->instances.count; j++) {
						Loading_Model::Transformation transformation = model->instances[j];
						Entity_Id entity_id = game_world->make_entity(transformation.scaling, transformation.rotation, transformation.translation);
						game_world->attach_AABB(entity_id, &mesh_AABB);
						render_world->add_render_entity(entity_id, mesh_id);
					}
				}
			}
			free_memory(&loaded_models);
		}
	}
}

static void load_level(Array<String> &command_args)
{
	if (!(command_args.is_empty() || command_args.first().is_empty())) {
		String full_path_to_level_file;
		build_full_path_to_level_file(command_args.first(), full_path_to_level_file);
		if (file_exists(full_path_to_level_file)) {
			Engine *engine = Engine::get_instance();
			Game_World *game_world = &engine->game_world;
			Render_World *render_world = &engine->render_world;

			save_game_and_render_world_in_level(engine->current_level_name, game_world, render_world);

			engine->current_level_name = command_args.first();
			game_world->release_all_resources();

			render_world->release_render_entities_resources();
			render_world->triangle_meshes.init(get_current_gpu_device());

			init_game_and_render_world_from_level(engine->current_level_name, game_world, render_world);
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
		Engine *engine = Engine::get_instance();
		Game_World *game_world = &engine->game_world;
		Render_World *render_world = &engine->render_world;
		
		save_game_and_render_world_in_level(engine->current_level_name, game_world, render_world);
		
		engine->set_current_level_name(command_args.first());
		game_world->release_all_resources();
		
		render_world->release_render_entities_resources();
		render_world->triangle_meshes.init(get_current_gpu_device());

		Entity_Id camera_id = game_world->make_camera(Vector3(0.0f, 20.0f, -250.0f), Vector3(0.0f, 0.0f, -1.0f));
		engine->render_world.set_camera_for_rendering(camera_id);
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

void init_commands()
{
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