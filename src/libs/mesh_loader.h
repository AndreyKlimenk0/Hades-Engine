#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include "str.h"
#include "../render/mesh.h"
#include "math/vector.h"
#include "structures/array.h"
#include "structures/hash_table.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>

struct Import_Mesh {
	struct Transform_Info {
		Vector3 scaling;
		Vector3 rotation; // stores angles in radians
		Vector3 translation;
	};
	Array<Transform_Info> mesh_instances;
	Triangle_Mesh mesh;

	Import_Mesh() = default;
	Import_Mesh(const Import_Mesh &other);
	Import_Mesh &operator=(const Import_Mesh &other);
};

struct Mesh_Loader {
	Assimp::Importer importer;
	aiScene *scene = NULL;

	bool load(const char *full_path_to_mesh_file, Array<Import_Mesh> &meshes, bool print_info, bool print_assimp_log);
	void process_mesh(aiMesh *ai_mesh, Triangle_Mesh *mesh);
	void process_nodes(aiNode *node, Array<Import_Mesh> &meshes, Hash_Table<u32, aiMesh *> &mesh_cache);
	void process_material(aiMaterial *material, Import_Mesh *import_mesh);
};

#endif

