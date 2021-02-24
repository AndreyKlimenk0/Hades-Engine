#include <string.h>
#include <d3dx11.h>

#include "base.h"
#include "mesh.h"
#include "model.h"
#include "../framework/file.h"
#include "../sys/sys_local.h"
#include "../libs/fbx_loader.h"
#include "../libs/str.h"

#include "../libs/ds/array.h"
#include "../libs/math/vector.h"


Model::~Model()
{
	DELETE_PTR(name);
	RELEASE_COM(texture);
}

void load_texture(const char *name, ID3D11ShaderResourceView *texture)
{
	char texture_dir[] = "data\\texture\\";
	char *texture_path = build_full_path(concatenate_c_str(texture_dir, name));
	HR(D3DX11CreateShaderResourceViewFromFile(direct3d.device, texture_path, NULL, NULL, &texture, NULL));
	DELETE_PTR(texture_path);
}

void Model::init_from_file(const char *file_name)
{
	assert(file_name != NULL);
	name = extract_file_name(file_name);
	
	char *file_extension = extract_file_extension(file_name);
	if (!strcmp(file_extension, "fbx")) {
		Fbx_Binary_File fbx_file;
		fbx_file.read(file_name);
		fbx_file.fill_out_mesh(&mesh);
		fbx_file.get_texture_name();
	} else {
		printf("Model::init_from_file: %s is unkown model type, now only supports fbx file type\n", file_extension);
		return;
	}
}

inline Vector3 tokens_to_vector3(Array<char *> *tokens)
{
	Vector3 v;
	float s[3];
	for (int i = 1; i < tokens->count; i++) {
		s[i - 1] = (float)atof(tokens->at(i));
		v.x = s[0];
		v.y = s[1];
		v.z = s[2];
	}
	return v;
}

void load_model_from_obj_file(const char *file_name, Triangle_Mesh *mesh)
{
	char *buffer = read_entire_file(file_name);
	Array<Vector3> positions;
	Array<Vector3> normals;
	Array<Vector2> uvs;

	while (1) {
		char *line = get_next_line(&buffer);
		if (!line) {
			break;
		}
		if (line[0] == '#' || line[0] == 'o' || line[0] == '\0') {
			continue;
		}
		Array<char *> tokens;
		split(line, " ", &tokens);
		if (!strcmp(tokens[0], "mtllib")) {
			printf("file tpe mtllib ins't parsed\n");
			continue;
		}
		Vertex *vertex = new Vertex();
		if (!strcmp(tokens[0], "v")) {
			assert(tokens.count == 4);
			positions.push(tokens_to_vector3(&tokens));

		} else if (!strcmp(tokens[0], "vt")) {
			//assert(tokens->size() == 3);
			uvs.push((Vector2)*((Vector2 *)&tokens_to_vector3(&tokens)));

		} else if (!strcmp(tokens[0], "vn")) {
			assert(tokens.count == 4);
			normals.push(tokens_to_vector3(&tokens));

		} else if (!strcmp(tokens[0], "f")) {
			for (int i = 1; i < tokens.count; i++) {
				Array<char *> vertex_indices;
				split(tokens[i], "/", &vertex_indices);
				int vertex_index = atoi(vertex_indices[0]);
				int texture_index = atoi(vertex_indices[1]);
				int normal_index = atoi(vertex_indices[2]);

				Vertex vertex;
				vertex.position = positions[vertex_index - 1];
				vertex.uv = uvs[texture_index - 1];
				vertex.normal = normals[normal_index - 1];
				
				//mesh->allocate_vertices(vertex_index - 1);

				vertices->push_back(vertex);
				indices->push_back(vertex_index);
			}
		} else {
			printf("This keyworld (%s) isn't supported\n", tokens->at(0));
		}
	}

}