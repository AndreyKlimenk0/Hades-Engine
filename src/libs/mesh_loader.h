#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

#include "str.h"
#include "os/path.h"
#include "ds/array.h"
#include "ds/hash_table.h"
#include "../render/model.h"
#include "../sys/sys_local.h"
#include "../win32/win_types.h"
#include "math/matrix.h"


struct Mesh_Loader {
	struct Mesh_Instance {
		String name;
		Array<Matrix4> transform_matrices;
		Triangle_Mesh mesh;
	};

	Array<Mesh_Instance *> mesh_instances;
	Hash_Table<String, Mesh_Instance *> mesh_instance_table;

	Assimp::Importer importer;
	aiScene* scene;

	bool load(const char *file_name, bool print_info = false);
	void process_nodes(aiNode *node, aiMatrix4x4 parent_transform_matrix);
	void process_mesh(aiMesh *ai_mesh, Triangle_Mesh *mesh);
	void clear();
};

#endif
