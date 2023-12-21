#include <assert.h>

#include "path.h"
#include "../ds/array.h"
#include "../ds/hash_table.h"

const char DATA_DIR_NAME[] = "data";
const char ENGINE_NAME[] = "hades";
const char GITHUB_ENGINE_NAME[] = "hades-engine";


struct Os_Path {
	String base_path;
	//@Note: May be better to git rid of this hash table. Just hard code
	Hash_Table<String, String> data_dir_paths;
};

static Os_Path os_path;

static bool build_correct_base_path(Array<String> *splitted_wrong_path, String *base_path)
{
	assert(base_path->data == NULL);

	String *dir = NULL;
	For((*splitted_wrong_path), dir) {
		dir->to_lower();
		if (*dir == "hades" || *dir == "hades-engine") {
			base_path->append(dir);
			return true;
		} else {
			base_path->append(dir);
			base_path->append("\\");
		}
	}
	return false;
}

static void init_base_path()
{
	char buffer[512];
	GetCurrentDirectory(512, buffer);
	String current_path = (const char *)&buffer;

	Array<String> dir_names;
	split(&current_path, "\\", &dir_names);

	dir_names.last_item().to_lower();
	if (dir_names.last_item() != ENGINE_NAME || dir_names.last_item() != GITHUB_ENGINE_NAME) {
		bool succeed = build_correct_base_path(&dir_names, &os_path.base_path);
		if (!succeed) {
			error("Base path can't be built correct");
		}
	} else {
		os_path.base_path = current_path;
	}
}

void init_os_path()
{
	init_base_path();

	char *texture_dir = format("{}\\{}\\{}", os_path.base_path, DATA_DIR_NAME, "textures");
	char *shader_dir = format("{}\\{}\\{}", os_path.base_path, DATA_DIR_NAME, "shader");
	char *model_dir = format("{}\\{}\\{}", os_path.base_path, DATA_DIR_NAME, "models");
	char *editor_dir = format("{}\\{}\\{}", os_path.base_path, DATA_DIR_NAME, "editor");
	char *maps_dir = format("{}\\{}\\{}", os_path.base_path, DATA_DIR_NAME, "maps");
	char *gui_dir = format("{}\\{}\\{}", os_path.base_path, DATA_DIR_NAME, "gui");

	os_path.data_dir_paths.set("texture", texture_dir);
	os_path.data_dir_paths.set("shader", shader_dir);
	os_path.data_dir_paths.set("model", model_dir);
	os_path.data_dir_paths.set("editor", editor_dir);
	os_path.data_dir_paths.set("maps", maps_dir);
	os_path.data_dir_paths.set("gui", gui_dir);

	free_string(texture_dir);
	free_string(shader_dir);
	free_string(model_dir);
	free_string(editor_dir);
	free_string(maps_dir);
	free_string(gui_dir);
}

void shutdown_os_path()
{
	os_path.base_path.free();
	os_path.data_dir_paths.clear();
}

void get_path_to_data_dir(const char *dir_name, String &full_path)
{
	os_path.data_dir_paths.get(dir_name, full_path);
}

void build_full_path_to_map_file(const char *file_name, String &full_path)
{
	String &value = os_path.data_dir_paths["maps"];
	full_path = value + "\\" + file_name;
}

void build_full_path_to_gui_file(const char *file_name, String &full_path)
{
	String &value = os_path.data_dir_paths["gui"];
	full_path = value + "\\" + file_name;
}

void build_full_path_to_texture_file(const char *file_name, String &full_path)
{
	String &value = os_path.data_dir_paths["texture"];
	full_path = value + "\\" + file_name;
}

void build_full_path_to_editor_file(const char *file_name, String &full_path)
{
	String &value = os_path.data_dir_paths["editor"];
	full_path = value + "\\" + file_name;
}

void build_full_path_to_shader_file(const char *file_name, String &full_path)
{
	String &value = os_path.data_dir_paths["shader"];
	full_path = value + "\\" + file_name;
}

void build_full_path_to_model_file(const char * file_name, String & full_path)
{
	String &value = os_path.data_dir_paths["model"];
	full_path = value + "\\" + file_name;
}

const char *get_base_path()
{
	return os_path.base_path.c_str();
}
