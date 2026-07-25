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
#include "../render/gpu_storages.h"
#include "../render/render_world.h"
#include "../collision/collision.h"

static Engine *engine = NULL;
static Game_World *game_world = NULL;
static Render_World *render_world = NULL;
static Variable_Service *variable_service = NULL;

static void load_meshes(Array<String> &command_args)
{
	begin_profile_task("Load meshes");

	bool convert_cm_to_m = false;
	Array<String> mesh_names;
	for (u32 i = 0; i < command_args.count; i++) {
		if (command_args[i].find(".") > -1) {
			mesh_names.push(command_args[i]);
		} else if (command_args[i] == "cm_to_m") {
			convert_cm_to_m = true;
		}
	}

	Variable_Service *models_loading = variable_service->find_namespace("models_loading");
	Loading_Models_Options loading_options;
	models_loading->attach("scene_logging", &loading_options.scene_logging);
	models_loading->attach("assimp_logging", &loading_options.assimp_logging);
	models_loading->attach("scaling_value", &loading_options.scaling_value);
	models_loading->attach("use_scaling_value", &loading_options.use_scaling_value);
	loading_options.convert_cm_to_m = convert_cm_to_m;

	for (u32 i = 0; i < mesh_names.count; i++) {
		begin_time_stamp();

		String full_path_to_mesh;
		build_full_path_to_model_file(mesh_names[i], full_path_to_mesh);

		Loading_Models_Info info;
		Array<String> textures;
		Array<Loading_Model *> loaded_models;
		if (load_models_from_file(full_path_to_mesh, loaded_models, textures, &info, &loading_options)) {
			Mesh_Storage *mesh_storage = &engine->mesh_storage;
			Texture_Storage *texture_storage = &engine->texture_storage;
			Material_Storage *material_storage = &engine->material_storage;

			String base_mesh_file_name;
			extract_base_file_name(mesh_names[i], base_mesh_file_name);

			print("load_meshes: Load image files and create textures.");
			texture_storage->create_textures(textures, base_mesh_file_name);

			print("load_meshes: Create game and render entities.");
			for (u32 j = 0; j < loaded_models.count; j++) {
				Loading_Model *loading_model = loaded_models[j];

				Mesh_Storage_Info mesh_info = mesh_storage->add_mesh(loading_model->name, &loading_model->mesh, loading_model->mesh_points);
			
				u32 normal_texture_idx = texture_storage->find_texture_or_get_default(loading_model->normal_texture_name, DEFAULT_NORMAL_TEXTURE)->shader_resource_descriptor()->index();
				u32 albedo_texture_idx = texture_storage->find_texture_or_get_default(loading_model->albedo_texture_name, DEFAULT_ALBEDO_TEXTURE)->shader_resource_descriptor()->index();
				u32 roughness_metalic_idx = texture_storage->find_texture_or_get_default(loading_model->roughness_metalic_texture_name, DEFAULT_ROUGHNESS_METALIC_TEXTURE)->shader_resource_descriptor()->index();

				u32 material_idx = material_storage->add_material(normal_texture_idx, albedo_texture_idx, roughness_metalic_idx);

				for (u32 k = 0; k < loading_model->instances.count; k++) {
					Loading_Model::Transformation transformation = loading_model->instances[k];
					Entity_Id entity_id = game_world->make_entity(transformation.scaling, transformation.rotation, transformation.translation);
					Entity *entity = game_world->get_entity(entity_id);

					AABB bounding_box = make_AABB(mesh_storage->get_base_vertex(&mesh_info), mesh_info.vertex_count, get_world_matrix(entity));

					render_world->add_render_entity(entity_id, bounding_box, material_idx, &mesh_info);
				}
			}
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

		Entity_Id camera_id = game_world->make_perspective_camera(Vector3(0.0f, 20.0f, -250.0f), Vector3(0.0f, 0.0f, -1.0f), engine->global_config.fov, engine->render_sys.window.aspect_ration, engine->global_config.near_plane, engine->global_config.far_plane);
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