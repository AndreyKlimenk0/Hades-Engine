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
	bool convert_cm_to_m;
	bool scene_logging;
	bool assimp_logging;
	bool use_scaling_value;
	float scaling_value;
};

struct Loading_Model {
	Loading_Model();
	Loading_Model(const String &name, const String &file_name);
	~Loading_Model();

	struct Transformation {
		Vector3 scaling = Vector3::one;
		Vector3 rotation = Vector3::zero; // stores angles in radians
		Vector3 translation = Vector3::zero;
	};

	String name;
	String file_name;

	String normal_texture_name;
	String albedo_texture_name;
	String roughness_metalic_texture_name;

	Triangle_Mesh mesh;
	Array<Transformation> instances;

	const char *get_name();
	String get_pretty_name();
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

bool load_models_from_file(const char *full_path_to_model_file, Array<Loading_Model *> &models, Array<String> &textures, Loading_Models_Info *loading_models_info = NULL, Loading_Models_Options *options = NULL);
#endif