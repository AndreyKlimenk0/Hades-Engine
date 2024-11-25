#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include "number_types.h"
#include "../render/mesh.h"
#include "structures/array.h"


struct Loading_Models_Info {
	u32 model_count = 0;
	u32 total_vertex_count = 0;
	u32 total_index_count = 0;
};

struct Loading_Models_Options {
	bool scene_logging;
	bool assimp_logging;
	bool use_scaling_value;
	float scaling_value;
};

struct Scene_Loader {
	Scene_Loader();
	~Scene_Loader();

	struct Loading_Options {
		bool scene_logging;
		bool assimp_logging;
		bool use_scaling_value;
		float scaling_value;
	};
	
	struct Loading_Info {
		u32 models_count = 0;
		u32 total_vertex_count = 0;
		u32 total_index_count = 0;
	};

	Array<Loading_Model *> &models;

	void reset();
	bool load_models_from_scene(const char *full_path_to_model_file);
	//bool load_scene(const char *full_path_to_model_file);
};

bool load_models_from_file(const char *full_path_to_model_file, Array<Loading_Model *> &models, Loading_Models_Info *loading_models_info = NULL, Loading_Models_Options *options = NULL);

#endif

