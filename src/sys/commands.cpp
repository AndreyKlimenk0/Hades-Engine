#include <stdlib.h>
#include <string.h>

#include "commands.h"
#include "../game/world.h"
#include "../sys/sys.h"
#include "../sys/engine.h"
#include "../libs/str.h"
#include "../libs/os/path.h"
#include "../libs/mesh_loader.h"
#include "../render/render_world.h"

static void load_meshes(Array<String> &mesh_names)
{
	Game_World *game_world = Engine::get_game_world();
	Render_World *render_world = Engine::get_render_world();

	for (u32 i = 0; i < mesh_names.count; i++) {
		String full_path_to_mesh;
		build_full_path_to_model_file(mesh_names[i], full_path_to_mesh);

		Mesh_Loader mesh_loader;
		Array<Import_Mesh> meshes;
		if (mesh_loader.load(full_path_to_mesh, meshes, false, false)) {
			render_world->triangle_meshes.loaded_meshes.push(mesh_names[i]);

			Import_Mesh *imported_mesh = NULL;
			For(meshes, imported_mesh) {
				Mesh_Id mesh_id;
				if (render_world->add_mesh(imported_mesh->mesh.name, &imported_mesh->mesh, &mesh_id)) {
					if (imported_mesh->mesh_instances.count > 0) {
						for (u32 j = 0; j < imported_mesh->mesh_instances.count; j++) {
							Import_Mesh::Transform_Info t = imported_mesh->mesh_instances[j];
							Entity_Id entity_id = game_world->make_entity(t.scaling, t.rotation, t.translation);
							render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_id);
						}
					} else {
						Entity_Id entity_id = game_world->make_entity(Vector3::one, Vector3::zero, Vector3::zero);
						render_world->add_render_entity(RENDERING_TYPE_FORWARD_RENDERING, entity_id, mesh_id);
					}
				}
			}
		}
	}
}

static void create_level(Array<String> &command_args)
{
	//if ((command_args.count == 1) && !command_args.get_first().is_empty()) {

	//} else {
	//	print("create_level: not valid args for the command");
	//}
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
	//add_command("create level", create_level);
	//add_command("load level", load_meshes);
	//add_command("delete level", load_meshes);
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