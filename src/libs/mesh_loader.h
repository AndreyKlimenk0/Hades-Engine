#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include "ds/array.h"
#include "ds/hash_table.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "../render/model.h"
#include "../render/render_helpers.h"


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

void init_fbx_lib();
void shutdown_fbx_lib();
bool load_fbx_mesh(const char *full_path_to_file, Array<Import_Mesh> *imported_meshes, bool display_info = false);
bool load_fbx_mesh_texture_info(const char *full_path_file);
#endif
