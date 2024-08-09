#include "os/path.h"
#include "os/file.h"
#include "mesh_loader.h"
#include "../sys/sys.h"

#include <assimp/postprocess.h>
#include <assimp/Logger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

struct Assimp_Logger : Assimp::LogStream {
	void write(const char *message)
	{
		::print(message);
	}
};

Import_Mesh::Import_Mesh(const Import_Mesh &other)
{
	*this = other;
}

Import_Mesh &Import_Mesh::operator=(const Import_Mesh &other)
{
	if (this != &other) {
		mesh_instances = other.mesh_instances;
		mesh.name = other.mesh.name;
		mesh.vertices = other.mesh.vertices;
		mesh.indices = other.mesh.indices;

		mesh.normal_texture_name = other.mesh.normal_texture_name;
		mesh.diffuse_texture_name = other.mesh.diffuse_texture_name;
		mesh.specular_texture_name = other.mesh.specular_texture_name;
		mesh.displacement_texture_name = other.mesh.displacement_texture_name;
	}
	return *this;
}

static const char *FOUR_SPACES = "    ";

inline void print_texture_info(aiMaterial *material, aiTextureType texture_type, const char *texture_label, const char *spaces)
{
	assert(material);

	u32 texture_count = material->GetTextureCount(texture_type);
	print("{}            {}:", spaces, texture_label);
	print("{}                Count: {}", spaces, texture_count);
	for (u32 i = 0; i < texture_count; i++) {
		aiString path;
		material->GetTexture(texture_type, i, &path);

		String texture_file_name;
		extract_file_name(path.C_Str(), texture_file_name);
		print("{}                File name: {}", spaces, texture_file_name);
	}
}

inline Vector3 to_vector3(aiVector3t<float> &vector)
{
	return Vector3(vector.x, vector.y, vector.z);
}

inline Matrix4 to_matrix4(aiMatrix4x4 &matrix)
{
	return Matrix4(matrix.a1, matrix.a2, matrix.a3, matrix.a4, matrix.b1, matrix.b2, matrix.b3, matrix.b4, matrix.c1, matrix.c2, matrix.c3, matrix.c4, matrix.d1, matrix.d2, matrix.d3, matrix.d4);
}

void print_nodes(aiScene *scene, aiNode *node, u32 node_level = 0)
{
	assert(scene);
	assert(node);

	String spaces = "";
	for (u32 i = 0; i < node_level; i++) {
		spaces.append(FOUR_SPACES);
	}

	aiVector3t<float> scaling;
	aiVector3t<float> rotation;
	aiVector3t<float> position;
	node->mTransformation.Decompose(scaling, rotation, position);

	Vector3 s = to_vector3(scaling);
	Vector3 r = to_vector3(rotation);
	Vector3 p = to_vector3(position);

	print("{}aiNode:", spaces);
	print("{}    Name:", spaces, node->mName.C_Str());
	print("{}    Child nodes number:", spaces, node->mNumChildren);
	print("{}    Meshes number:", spaces, node->mNumMeshes);
	print("{}    Scaling: {}", spaces, &s);
	print("{}    Rotation: {}", spaces, &r);
	print("{}    Position: {}", spaces, &p);

	if (node->mNumMeshes > 0) {
		for (u32 i = 0; i < node->mNumMeshes; i++) {
			u32 mesh_index = node->mMeshes[i];;
			aiMesh *mesh = scene->mMeshes[mesh_index];
			print("{}    Mesh: {}", spaces, mesh->mName.C_Str());
			print("{}        Vertices number:", spaces, mesh->mNumVertices);
			if (scene->HasMaterials()) {
				print("{}        Material info:", spaces);
				aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
				aiString name;
				name.Append("Unknown material name");
				material->Get(AI_MATKEY_NAME, name);
				print("{}            Name: {}", spaces, name.C_Str());

				float shininess = 0.0f;
				material->Get(AI_MATKEY_SHININESS, shininess);
				print("{}            Shininess: {}", spaces, shininess);

				float shininess_strength = 1.0f;
				material->Get(AI_MATKEY_SHININESS_STRENGTH, shininess_strength);
				print("{}            Shininess strength: {}", spaces, shininess_strength);

				print_texture_info(material, aiTextureType_HEIGHT, "Height textures", spaces.c_str());
				print_texture_info(material, aiTextureType_NORMALS, "Normals textures", spaces.c_str());
				print_texture_info(material, aiTextureType_DIFFUSE, "Diffuse textures", spaces.c_str());
				print_texture_info(material, aiTextureType_SPECULAR, "Specular textures", spaces.c_str());
				print_texture_info(material, aiTextureType_DISPLACEMENT, "Displacement textures", spaces.c_str());
			}
		}
	}

	for (u32 i = 0; i < node->mNumChildren; i++) {
		print_nodes(scene, node->mChildren[i], node_level + 1);
	}
}

bool Mesh_Loader::load(const char *full_path_to_mesh_file, Array<Import_Mesh> &meshes, bool print_info, bool print_assimp_log)
{
	String file_name;
	extract_base_file_name(full_path_to_mesh_file, file_name);

	print("Mesh_Loader::load: Started to load {}.", file_name);

	if (!file_exists(full_path_to_mesh_file)) {
		print("Mesh_Loader::load: Failed to load. {} does not exist in model folder.", file_name);
		return false;
	}

	if (print_assimp_log) {
		Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
		Assimp::DefaultLogger::get()->attachStream(new Assimp_Logger(), Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn);
		defer(Assimp::DefaultLogger::kill());
	}

	scene = (aiScene *)importer.ReadFile(full_path_to_mesh_file, aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded);
	if (!scene) {
		print("Mesh_Loader::load: Failed to load a scene from {}.", file_name);
		return false;
	}

	if (!scene->mRootNode) {
		print("Mesh_Loader::load: Failed to load a scene from {}.", file_name);
		return false;
	}

	if (print_info) {
		print_nodes(scene, scene->mRootNode);
	}
	Hash_Table<u32, aiMesh *> mesh_cache;
	process_nodes(scene->mRootNode, meshes, mesh_cache);

	print("Mesh_Loader::load: {} was load successfully.", file_name);
	return true;
}

inline void decompose_matrix(aiMatrix4x4 &matrix, Vector3 &s, Vector3 &r, Vector3 &p)
{
	aiVector3t<float> scaling;
	aiVector3t<float> rotation;
	aiVector3t<float> position;
	matrix.Decompose(scaling, rotation, position);

	s = to_vector3(scaling);
	r = to_vector3(rotation);
	p = to_vector3(position);
}

void Mesh_Loader::process_nodes(aiNode *node, Array<Import_Mesh> &meshes, Hash_Table<u32, aiMesh *> &mesh_cache)
{
	for (u32 i = 0; i < node->mNumMeshes; i++) {
		u32 mesh_index = node->mMeshes[i];
		if (scene->mMeshes[mesh_index]->mName.length > 0) {

			aiMesh *mesh = NULL;
			if (!mesh_cache.get(mesh_index, &mesh)) {
				mesh = scene->mMeshes[mesh_index];

				u32 import_mesh_index = meshes.push(Import_Mesh());
				Import_Mesh *temp = &meshes[import_mesh_index];
				temp->mesh.name = mesh->mName.C_Str();

				Import_Mesh::Transform_Info transformation_matrix;
				decompose_matrix(node->mTransformation, transformation_matrix.scaling, transformation_matrix.rotation, transformation_matrix.translation);
				temp->mesh_instances.push(transformation_matrix);

				process_mesh(mesh, &temp->mesh);
				if (scene->HasMaterials()) {
					aiMaterial *material = scene->mMaterials[mesh->mMaterialIndex];
					process_material(material, &meshes.last());
				}
			}
		}
	}
	for (u32 i = 0; i < node->mNumChildren; i++) {
		process_nodes(node->mChildren[i], meshes, mesh_cache);
	}
}

inline bool get_texture_file_name(aiMaterial *material, aiTextureType texture_type, String &texture_file_name)
{
	u32 texture_count = material->GetTextureCount(texture_type);
	if (texture_count > 0) {
		aiString path_to_texture_file;
		material->GetTexture(texture_type, 0, &path_to_texture_file);
		extract_file_name(path_to_texture_file.C_Str(), texture_file_name);
		return true;
	}
	return false;
}

void Mesh_Loader::process_material(aiMaterial *material, Import_Mesh *import_mesh)
{
	float shininess = 0.0f;
	float shininess_strength = 1.0f;

	material->Get(AI_MATKEY_SHININESS, shininess);
	material->Get(AI_MATKEY_SHININESS_STRENGTH, shininess_strength);

	if (!get_texture_file_name(material, aiTextureType_NORMALS, import_mesh->mesh.normal_texture_name)) {
		get_texture_file_name(material, aiTextureType_HEIGHT, import_mesh->mesh.normal_texture_name);
	}
	get_texture_file_name(material, aiTextureType_DIFFUSE, import_mesh->mesh.diffuse_texture_name);
	get_texture_file_name(material, aiTextureType_SPECULAR, import_mesh->mesh.specular_texture_name);
	get_texture_file_name(material, aiTextureType_DISPLACEMENT, import_mesh->mesh.displacement_texture_name);
}

void Mesh_Loader::process_mesh(aiMesh *ai_mesh, Triangle_Mesh *mesh)
{
	for (u32 i = 0; i < ai_mesh->mNumVertices; i++) {
		Vertex_PNTUV vertex;
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
		vertex.tangent.x = ai_mesh->mTangents[i].x;
		vertex.tangent.y = ai_mesh->mTangents[i].y;
		vertex.tangent.z = ai_mesh->mTangents[i].z;

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