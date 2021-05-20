#include "path.h"


const char DATA_DIR_NAME[] = "data";
const char ENGINE_NAME[] = "hades";
const char GITHUB_ENGINE_NAME[] = "hades-engine";

Path os_path;


void Path::init()
{
	init_base_path();

	char *texture_dir = format("{}\\{}\\{}", base_path, DATA_DIR_NAME, "textures");
	char *shader_dir = format("{}\\{}\\{}", base_path, DATA_DIR_NAME, "shader");
	char *model_dir = format("{}\\{}\\{}", base_path, DATA_DIR_NAME, "models");

	data_dir_paths.set("texture", texture_dir);
	data_dir_paths.set("shader", shader_dir);
	data_dir_paths.set("model", model_dir);

	free_string(texture_dir);
	free_string(shader_dir);
	free_string(model_dir);
}

void Path::init_base_path()
{
	char buffer[512];
	GetCurrentDirectory(512, buffer);
	String current_path = (const char *)&buffer;

	Array<String> dir_names;
	split(&current_path, "\\", &dir_names);

	dir_names.last_item().to_lower();
	if (dir_names.last_item() != ENGINE_NAME || dir_names.last_item() != GITHUB_ENGINE_NAME) {
		bool succeed = build_correct_base_path(&dir_names, &base_path);
		if (!succeed) {
			error("Base path can't be built correct");
		}
	} else {
		base_path = current_path;
	}
}

bool Path::build_correct_base_path(Array<String> *splited_wrong_path, String *base_path)
{
	assert(base_path->data == NULL);

	String *dir = NULL;
	FOR((*splited_wrong_path), dir) {
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
