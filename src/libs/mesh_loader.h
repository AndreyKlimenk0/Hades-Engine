#ifndef MESH_LOADER_H
#define MESH_LOADER_H

#include "math/vector.h"
#include "structures/array.h"
#include "../render/mesh.h"

struct Mesh_Data {
	Mesh_Data();
	~Mesh_Data();

	struct Transformation {
		Vector3 scaling;
		Vector3 rotation; // stores angles in radians
		Vector3 translation;
	};
	Triangle_Mesh mesh;
	Array<Transformation> mesh_instances;

	Mesh_Data(const Mesh_Data &other);
	Mesh_Data &operator=(const Mesh_Data &other);
};

void setup_3D_file_loading_log(bool print_loading_info, bool print_external_lib_log);
bool load_triangle_mesh(const char *full_path_to_mesh_file, Array<Mesh_Data> &submeshes);

#endif

