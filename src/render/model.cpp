#include <string.h>

#include "model.h"
#include "../libs/ds/array.h"
#include "../libs/ds/string.h"
#include "../libs/math/vector.h"
#include "../libs/general.h"
#include "../framework/file.h"
#include "../win32/win_types.h"


Vector3 tokens_to_vector3(Array<char *> *tokens)
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
	Array<Vertex_Color> vertices;
	Array<u32> indices;
	Array<Vector3> positions;
	Array<Vector3> normals;
	Array<Vector2> uvs;
	Array<char *> tokens;

	char *buffer = read_entire_file(file_name, "rb");
	assert(buffer != NULL);

	while (1) {
		char *line = get_next_line(&buffer);
		if (!line) {
			break;
		}
		if (line[0] == '#' || line[0] == 'o' || line[0] == '\0') {
			continue;
		}
		//std::vector<char *> *tokens = split(line, " ");
		split(line, " ", &tokens);
		if (!strcmp(tokens[0], "mtllib")) {
			printf("file type mtllib ins't parsed\n");
			continue;
		}
		if (!strcmp(tokens[0], "v")) {
			assert(tokens.count == 4);
			positions.push(tokens_to_vector3(&tokens));

		} else if (!strcmp(tokens[0], "vt")) {
			//assert(tokens->size() == 3);
			uvs.push((Vector2)tokens_to_vector3(&tokens));

		} else if (!strcmp(tokens[0], "vn")) {
			assert(tokens.count == 4);
			normals.push(tokens_to_vector3(&tokens));

		} else if (!strcmp(tokens[0], "f")) {
			for (int i = 1; i < tokens.count; i++) {
				//std::vector<char *> *vertex_indices = split(tokens->at(i), "/");
				Array<char *> vertex_indices;
				split(tokens[i], "/", &vertex_indices);
				int vertex_index  = atoi(vertex_indices[0]);
				int texture_index = atoi(vertex_indices[1]);
				int normal_index  = atoi(vertex_indices[2]);

				Vertex_Color vertex;
				vertex.position = positions[vertex_index - 1];
				vertex.color = Red;
				//vertex.uv = uvs[texture_index - 1];
				//vertex.normal = normals[normal_index - 1];

				vertices.push(vertex);
				indices.push(vertex_index);
			}
		} else {
			printf("This keyworld (%s) isn't supported\n", tokens[0]);
		}
		//DELETE_PTR(line);
	}
	mesh->vertices = new Vertex_Color[vertices.count];
	mesh->indices = new u32[indices.count];
	mesh->vertex_count = vertices.count;
	mesh->index_count = indices.count;
	memcpy(mesh->vertices, vertices.array, vertices.count);
	memcpy(mesh->indices, indices.array, indices.count);
//	DELETE_PTR(buffer);
}