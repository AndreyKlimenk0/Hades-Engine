#include "mesh_loader.h"
#include "os/file.h"

const char *four_spaces = "    ";

inline void print_nodes(aiScene *scene, aiNode *node, u32 node_level = 0)
{
	String spaces;
	for (u32 i = 0; i < node_level; i++) {
		spaces.append(four_spaces);
	}

	if (spaces.is_empty()) {
		print("Node name: ", node->mName.C_Str());
		print("Node child count:", node->mNumChildren);
		print("Node mesh count: ", node->mNumMeshes);
		for (u32 i = 0; i < node->mNumMeshes; i++) {
			u32 mesh_index = node->mMeshes[i];
			print("    Node mesh name:", scene->mMeshes[mesh_index]->mName.C_Str());
		}
	} else {
		print(spaces, "Node name: ", node->mName.C_Str());
		print(spaces, "Node child count:", node->mNumChildren);
		print(spaces, "Node mesh count: ", node->mNumMeshes);
		for (u32 i = 0; i < node->mNumMeshes; i++) {
			u32 mesh_index = node->mMeshes[i];
			print(spaces, "    Node mesh name:", scene->mMeshes[mesh_index]->mName.C_Str());
		}
	}

	node_level += 1;
	for (u32 i = 0; i < node->mNumChildren; i++) {
		print_nodes(scene, node->mChildren[i], node_level);
	}
}

bool Mesh_Loader::load(const char *file_name, bool print_info)
{
	print("Mesh_Loader::load: Started to load {}.", file_name);
	clear();

	String path;
	build_full_path_to_model_file(file_name, path);

	if (!file_exists(path)) {
		print("Mesh_Loader::load: Failed to load. {} does not exist in model folder.", file_name);
		return false;
	}

	scene = (aiScene *)importer.ReadFile(path, aiProcess_CalcTangentSpace | aiProcess_Triangulate | aiProcess_JoinIdenticalVertices |
										 aiProcess_SortByPType | aiProcess_MakeLeftHanded | aiProcess_FlipUVs);
	if (!scene) {
		print("Mesh_Loader::load: Failed to load a scene from {}.", file_name);
		return false;
	}
	if (print_info) {
		print_nodes(scene, scene->mRootNode);
	}

	process_nodes(scene->mRootNode);
	print("Mesh_Loader::load: {} was load successfully.", file_name);
	return true;
}

void Mesh_Loader::process_nodes(aiNode *node)
{
	Array<String> processed_meshes;
	for (u32 i = 0; i < node->mNumMeshes; i++) {
		u32 mesh_index = node->mMeshes[i];
		const char *mesh_name = scene->mMeshes[mesh_index]->mName.C_Str();

		if (processed_meshes.find(mesh_name)) {
			continue;
		}

		processed_meshes.push(mesh_name);

		Mesh_Instance *mesh_instance = NULL;
		if (!mesh_instance_table.get(mesh_name, &mesh_instance)) {
			mesh_instance = new Mesh_Instance();
			mesh_instance->name = mesh_name;
			process_mesh(scene->mMeshes[mesh_index], &mesh_instance->mesh);

			mesh_instance_table.set(mesh_name, mesh_instance);
			mesh_instances.push(mesh_instance);
		}
		aiVector3t<float> scaling;
		aiVector3t<float> rotation;
		aiVector3t<float> position;
		node->mTransformation.Decompose(scaling, rotation, position);

		mesh_instance->positions.push(Vector3(position.x, position.y, position.z));
	}

	for (u32 i = 0; i < node->mNumChildren; i++) {
		process_nodes(node->mChildren[i]);
	}
}

void Mesh_Loader::process_mesh(aiMesh *ai_mesh, Triangle_Mesh *mesh)
{
	mesh->vertices.clear();
	mesh->indices.clear();

	for (u32 i = 0; i < ai_mesh->mNumVertices; i++) {
		Vertex_XNUV vertex;

		vertex.position.x = ai_mesh->mVertices[i].x;
		vertex.position.y = ai_mesh->mVertices[i].y;
		vertex.position.z = ai_mesh->mVertices[i].z;

		if (ai_mesh->HasTextureCoords(0)) {
			vertex.uv.x = (float)ai_mesh->mTextureCoords[0][i].x;
			vertex.uv.y = (float)ai_mesh->mTextureCoords[0][i].y;
		}

		if (ai_mesh->HasNormals()) {
			vertex.normal.x = ai_mesh->mNormals[i].x;
			vertex.normal.y = ai_mesh->mNormals[i].y;
			vertex.normal.z = ai_mesh->mNormals[i].z;
		}

		mesh->vertices.push(vertex);
	}

	for (u32 i = 0; i < ai_mesh->mNumFaces; i++) {
		aiFace face = ai_mesh->mFaces[i];

		for (auto j = 0u; j < face.mNumIndices; j++) {
			mesh->indices.push(face.mIndices[j]);
		}
	}
}

void Mesh_Loader::clear()
{
	mesh_instances.clear();
	mesh_instance_table.clear();
}
