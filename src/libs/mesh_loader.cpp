#include "os/path.h"
#include "os/file.h"
#include "mesh_loader.h"
#include "../sys/sys.h"
#include "../win32/win_time.h"
#include "../libs/structures/hash_table.h"

#include <assimp/scene.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/Logger.hpp>
#include <assimp/LogStream.hpp>
#include <assimp/DefaultLogger.hpp>

static s32 unknown_model_name_count = 0;
static String current_file_name;
static const char *FOUR_SPACES = "    ";
static Loading_Models_Options loading_options;
static Loading_Models_Info loading_info;

struct Assimp_Logger : Assimp::LogStream {
	void write(const char *message)
	{
		::print(message);
	}
};

static void begin_load_models()
{
	unknown_model_name_count = 0;
	current_file_name.free();
	loading_options.scene_logging = false;
	loading_options.assimp_logging = false;
	loading_options.use_scaling_value = false;

	loading_info.model_count = 0;
	loading_info.total_vertex_count = 0;
	loading_info.total_index_count = 0;
}

inline Vector3 to_vector3(aiVector3t<float> &vector)
{
	return Vector3(vector.x, vector.y, vector.z);
}

inline Matrix4 to_matrix4(aiMatrix4x4 &matrix)
{
	return Matrix4(matrix.a1, matrix.a2, matrix.a3, matrix.a4, matrix.b1, matrix.b2, matrix.b3, matrix.b4, matrix.c1, matrix.c2, matrix.c3, matrix.c4, matrix.d1, matrix.d2, matrix.d3, matrix.d4);
}

inline char *get_unique_name(aiMesh *mesh)
{
	return format("{}_{}_{}_{}", mesh->mName.C_Str(), mesh->mNumVertices, mesh->mNumFaces, mesh->mPrimitiveTypes);
}

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

void print_nodes(aiScene *scene, aiNode *node, const aiMatrix4x4 &parent_node_matrix, u32 node_level = 0)
{
	assert(scene);
	assert(node);

	String spaces = "";
	for (u32 i = 0; i < node_level; i++) {
		spaces.append(FOUR_SPACES);
	}

	aiMatrix4x4 transformation_matrix = node->mTransformation * parent_node_matrix;

	aiVector3t<float> scaling;
	aiVector3t<float> rotation;
	aiVector3t<float> position;
	node->mTransformation.Decompose(scaling, rotation, position);

	Vector3 s = to_vector3(scaling);
	Vector3 r = to_vector3(rotation);
	Vector3 p = to_vector3(position);

	transformation_matrix.Decompose(scaling, rotation, position);

	Vector3 s1 = to_vector3(scaling);
	Vector3 r1 = to_vector3(rotation);
	Vector3 p1 = to_vector3(position);

	print("{}aiNode:", spaces);
	print("{}    Name:", spaces, node->mName.C_Str());
	print("{}    Child nodes number:", spaces, node->mNumChildren);
	print("{}    Meshes number:", spaces, node->mNumMeshes);
	
	print("{}    Itself matrix", spaces);
	print("{}    Scaling: {}", spaces, &s);
	print("{}    Rotation: {}", spaces, &r);
	print("{}    Position: {}", spaces, &p);

	print("{}    Inherited matrix", spaces);
	print("{}    Scaling: {}", spaces, &s1);
	print("{}    Rotation: {}", spaces, &r1);
	print("{}    Position: {}", spaces, &p1);

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

				print_texture_info(material, aiTextureType_AMBIENT,      "Ambient textures",      spaces.c_str());
				print_texture_info(material, aiTextureType_EMISSIVE,     "Emissive textures",     spaces.c_str());
				print_texture_info(material, aiTextureType_HEIGHT,       "Height textures",       spaces.c_str());
				print_texture_info(material, aiTextureType_NORMALS,      "Normals textures",      spaces.c_str());
				print_texture_info(material, aiTextureType_SHININESS,    "Shininess textures",    spaces.c_str());
				print_texture_info(material, aiTextureType_DIFFUSE,      "Diffuse textures",      spaces.c_str());
				print_texture_info(material, aiTextureType_SPECULAR,     "Specular textures",	  spaces.c_str());
				print_texture_info(material, aiTextureType_DISPLACEMENT, "Displacement textures", spaces.c_str());
				print_texture_info(material, aiTextureType_REFLECTION,   "Reflection textures",   spaces.c_str());
				print_texture_info(material, aiTextureType_OPACITY,      "Opacity textures",      spaces.c_str());
				
				print_texture_info(material, aiTextureType_BASE_COLOR,        "Base color textures",        spaces.c_str());
				print_texture_info(material, aiTextureType_NORMAL_CAMERA,     "Normal camera textures",     spaces.c_str());
				print_texture_info(material, aiTextureType_EMISSION_COLOR,    "Emission color textures",    spaces.c_str());
				print_texture_info(material, aiTextureType_METALNESS,         "Metalness textures",         spaces.c_str());
				print_texture_info(material, aiTextureType_DIFFUSE_ROUGHNESS, "Roughness textures",         spaces.c_str());
				print_texture_info(material, aiTextureType_AMBIENT_OCCLUSION, "Ambient occlusion textures", spaces.c_str());
			}
		}
	}

	for (u32 i = 0; i < node->mNumChildren; i++) {
		print_nodes(scene, node->mChildren[i], transformation_matrix, node_level + 1);
	}
}

inline void decompose_matrix(aiMatrix4x4 &matrix, Vector3 &s, Vector3 &r, Vector3 &p)
{
	aiVector3t<float> scaling;
	aiVector3t<float> rotation;
	aiVector3t<float> position;
	matrix.Decompose(scaling, rotation, position);

	s = loading_options.use_scaling_value ? Vector3(loading_options.scaling_value, loading_options.scaling_value, loading_options.scaling_value) : to_vector3(scaling);
	r = to_vector3(rotation);
	p = to_vector3(position);
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

inline void process_mesh(aiMesh *ai_mesh, Triangle_Mesh *mesh)
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
		if (ai_mesh->HasTangentsAndBitangents()) {
			vertex.tangent.x = ai_mesh->mTangents[i].x;
			vertex.tangent.y = ai_mesh->mTangents[i].y;
			vertex.tangent.z = ai_mesh->mTangents[i].z;
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

	loading_info.model_count++;
	loading_info.total_vertex_count += mesh->vertices.count;
	loading_info.total_index_count += mesh->indices.count;
}

inline void process_material(aiMaterial *material, Loading_Model *loading_model)
{
	float shininess = 0.0f;
	float shininess_strength = 1.0f;

	material->Get(AI_MATKEY_SHININESS, shininess);
	material->Get(AI_MATKEY_SHININESS_STRENGTH, shininess_strength);

	if (!get_texture_file_name(material, aiTextureType_NORMALS, loading_model->normal_texture_name)) {
		get_texture_file_name(material, aiTextureType_HEIGHT, loading_model->normal_texture_name);
	}
	get_texture_file_name(material, aiTextureType_DIFFUSE, loading_model->diffuse_texture_name);
	get_texture_file_name(material, aiTextureType_SPECULAR, loading_model->specular_texture_name);
	get_texture_file_name(material, aiTextureType_DISPLACEMENT, loading_model->displacement_texture_name);
}

inline void process_nodes(aiScene *scene, aiNode *node, const aiMatrix4x4 &parent_matrix, Array<Loading_Model *> &models, Hash_Table<String, Loading_Model *> &models_cache)
{
	aiMatrix4x4 transform_matrix = node->mTransformation * parent_matrix;

	for (u32 i = 0; i < node->mNumMeshes; i++) {
		u32 mesh_index = node->mMeshes[i];
		
		String mesh_name;
		aiMesh *assimp_mesh = scene->mMeshes[mesh_index];

		if (assimp_mesh->mName.length > 0) {
			mesh_name.move(get_unique_name(assimp_mesh));
		} else {
			char *temp = format("{}_{}_{}_{}_{}", current_file_name, assimp_mesh->mNumVertices, assimp_mesh->mNumFaces, assimp_mesh->mPrimitiveTypes, unknown_model_name_count++);
			mesh_name.move(temp);
			print("process_nodes: A model doesn't have a name. A name was generated for it.");
		}
		
		Loading_Model *loading_model = NULL;
		if (!models_cache.get(mesh_name, loading_model)) {
			loading_model = new Loading_Model(mesh_name, current_file_name);
			process_mesh(assimp_mesh, &loading_model->mesh);
			
			if (scene->HasMaterials()) {
				aiMaterial *material = scene->mMaterials[assimp_mesh->mMaterialIndex];
				process_material(scene->mMaterials[assimp_mesh->mMaterialIndex], loading_model);
			}
			
			models_cache.set(mesh_name, loading_model);
			models.push(loading_model);
		}		
		Loading_Model::Transformation transformation;
		decompose_matrix(transform_matrix, transformation.scaling, transformation.rotation, transformation.translation);
		loading_model->instances.push(transformation);	
	}
	
	for (u32 i = 0; i < node->mNumChildren; i++) {
		process_nodes(scene, node->mChildren[i], transform_matrix, models, models_cache);
	}
}

bool load_models_from_file(const char *full_path_to_model_file, Array<Loading_Model *> &models, Loading_Models_Info *loading_models_info, Loading_Models_Options *options)
{
	s64 start = milliseconds_counter();
	begin_load_models();

	if (options) {
		loading_options = *options;
	}

	String file_name;
	extract_file_name(full_path_to_model_file, file_name);

	print("load: Started to load {}.", file_name);

	if (!file_exists(full_path_to_model_file)) {
		print("load: Failed to load. {} does not exist in model folder.", file_name);
		return false;
	}

	if (loading_options.assimp_logging) {
		Assimp::DefaultLogger::create("", Assimp::Logger::VERBOSE);
		Assimp::DefaultLogger::get()->attachStream(new Assimp_Logger(), Assimp::Logger::Debugging | Assimp::Logger::Info | Assimp::Logger::Err | Assimp::Logger::Warn);
	}
	Assimp::Importer importer;
	aiScene *scene = (aiScene *)importer.ReadFile(full_path_to_model_file, aiProcessPreset_TargetRealtime_Fast | aiProcess_ConvertToLeftHanded);

	bool result = true;
	if (!scene) {
		print("load: Failed to load a scene from {}.", file_name);
		result = false;
	}
	
	if (scene && !scene->mRootNode) {
		print("load: Failed to load a scene from {}.", file_name);
		result = false;
	}
	
	if (result) {
		if (loading_options.scene_logging) {
			print_nodes(scene, scene->mRootNode, aiMatrix4x4());
		}

		current_file_name = file_name;
		models.resize(scene->mNumMeshes);

		Hash_Table<String, Loading_Model *> model_cache;
		process_nodes(scene, scene->mRootNode, aiMatrix4x4(), models, model_cache);

		print("load: {} was successfully loaded. Loading time is {}ms.", file_name, milliseconds_counter() - start);
	}

	if (loading_models_info) {
		*loading_models_info = loading_info;
	}

	Assimp::DefaultLogger::kill();

	return result;
}
