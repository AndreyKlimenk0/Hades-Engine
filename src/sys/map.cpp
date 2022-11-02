#include <stdio.h>

#include "map.h"
#include "../libs/os/path.h"
#include "../libs/os/file.h"

void save_world_in_map(World *world, const char *map_name)
{
	String path_to_file;
	build_full_path_to_map_file(map_name, path_to_file);

	//HANDLE file = CreateFile(path_to_file, GENERIC_WRITE, 0, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);

	FILE *file = fopen(path_to_file, "wb");

	Entity *entity = NULL;
	For(world->entity_manager.entities, entity) {

	}
}

void read_from_map()
{

}