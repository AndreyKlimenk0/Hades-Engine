#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include "math/vector.h"
#include "structures/array.h"
#include "../render/mesh.h"

#define DISALLOW_COPY_AND_ASSIGN(Type_Name)	\
    Type_Name(const Type_Name &other) = delete;       \
	Type_Name &operator=(const Type_Name &other) = delete  \

struct Loading_Model {
	Loading_Model();
	~Loading_Model();

	struct Transformation {
		Vector3 scaling;
		Vector3 rotation; // stores angles in radians
		Vector3 translation;
	};
	Triangle_Mesh mesh;
	Array<Transformation> instances;

	DISALLOW_COPY_AND_ASSIGN(Loading_Model);
};

struct Models_Loading_Options {
	bool scene_logging;
	bool assimp_logging;
	bool use_scaling_value;
	float scaling_value;
};

bool load_models_from_file(const char *full_path_to_model_file, Array<Loading_Model *> &models, Models_Loading_Options *options = NULL);

#endif

