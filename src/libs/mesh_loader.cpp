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

#include <assimp/LogStream.hpp>
#include <assimp/Logger.hpp>
#include <assimp/DefaultLogger.hpp>

class myStream : public Assimp::LogStream {
public:
	// Write womethink using your own functionality
	void write(const char* message) {
		//::print("Assimp log: ", message);
	}
};

bool Mesh_Loader::load(const char *file_name, bool print_info)
{

	Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
	// Select the kinds of messages you want to receive on this log stream
	const unsigned int severity = Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn;

	// Attaching it to the default logger
	auto temp = new myStream();
	Assimp::DefaultLogger::get()->attachStream(temp, severity);

	Assimp::DefaultLogger::get()->info("this is my info-call");

	print("Mesh_Loader::load: Started to load {}.", file_name);

	String path;
	build_full_path_to_model_file(file_name, path);

	if (!file_exists(path)) {
		print("Mesh_Loader::load: Failed to load. {} does not exist in model folder.", file_name);
		return false;
	}
	auto postProcessSteps = (aiPostProcessSteps)(
		aiProcess_Triangulate |
		aiProcess_SortByPType |
		//aiProcess_CalcTangentSpace |
		//aiProcess_FlipUVs |
		//aiProcess_JoinIdenticalVertices |
		aiProcess_ConvertToLeftHanded);

	scene = (aiScene *)importer.ReadFile(path, postProcessSteps);
	if (!scene) {
		print("Mesh_Loader::load: Failed to load a scene from {}.", file_name);
		return false;
	}
	if (print_info) {
		print_nodes(scene, scene->mRootNode);
	}

	auto matrix = aiMatrix4x4(
		1.0f, 0.0f, 0.0f, 0.0f,
		0.0f, 1.0f, 0.0f, 0.0f,
		0.0f, 0.0f, 1.0f, 0.0f,
		0.0f, 0.0f, 0.0f, 1.0f);

	process_nodes(scene->mRootNode, matrix);
	print("Mesh_Loader::load: {} was load successfully.", file_name);
	return true;
}

void Mesh_Loader::process_nodes(aiNode *node, aiMatrix4x4 parent_transform_matrix)
{
	Array<String> processed_meshes;
	for (u32 i = 0; i < node->mNumMeshes; i++) {
		u32 mesh_index = node->mMeshes[i];
		const char *mesh_name = scene->mMeshes[mesh_index]->mName.C_Str();

		Mesh_Instance *mesh_instance = NULL;
		if (!mesh_instance_table.get(mesh_name, &mesh_instance)) {
			mesh_instance = new Mesh_Instance();
			mesh_instance->name = mesh_name;
			process_mesh(scene->mMeshes[mesh_index], &mesh_instance->mesh);

			mesh_instance_table.set(mesh_name, mesh_instance);
			mesh_instances.push(mesh_instance);
		}
		aiMatrix4x4 temp = parent_transform_matrix * node->mTransformation;
		Matrix4 transform_matrix = {
			temp.a1, temp.a2, temp.a3, temp.a4,
			temp.b1, temp.b2, temp.b3, temp.b4,
			temp.c1, temp.c2, temp.c3, temp.c4,
			temp.d1, temp.d2, temp.d3, temp.d4,
		};
		transform_matrix = transpose(&transform_matrix);

		mesh_instance->transform_matrices.push(make_identity_matrix());
	}

	aiMatrix4x4 transform_matrix = node->mTransformation * parent_transform_matrix;

	for (u32 i = 0; i < node->mNumChildren; i++) {
		process_nodes(node->mChildren[i], transform_matrix);
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

		assert(face.mNumIndices == 3);
		for (u32 j = 0; j < face.mNumIndices; j++) {
			mesh->indices.push(face.mIndices[j]);
		}
	}
}

void Mesh_Loader::clear()
{
	mesh_instances.clear();
	mesh_instance_table.clear();
}
