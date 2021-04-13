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
	RELEASE_COM(diffuse_texture);
}



void Model::init_from_file(const char *file_name)
{
	assert(file_name != NULL);
	name = extract_file_name(file_name);
	
	char *file_extension = extract_file_extension(file_name);
	if (!strcmp(file_extension, "fbx")) {

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
		s[i - 1] = static_cast<float>(atof(tokens->at(i)));
		v.x = s[0];
		v.y = s[1];
		v.z = s[2];
	}
	return v;
}

struct Vertex_Buffer {
	Array<Vector3> positions;
	Array<Vector3> normals;
	Array<Vector2> uvs;
};


static void split_obj_face_and_fill_out_vertex(char *face, Vertex *vertex, const Vertex_Buffer *vertex_buffer)
{
	Array<char *> vertex_indices;
	split(face, "/", &vertex_indices);

	int vertex_index = atoi(vertex_indices[0]);
	int texture_index = atoi(vertex_indices[1]);
	int normal_index = atoi(vertex_indices[2]);

	vertex->position = vertex_buffer->positions[vertex_index - 1];
	vertex->uv = vertex_buffer->uvs[texture_index - 1];
	vertex->normal = vertex_buffer->normals[normal_index - 1];
	vertex->uv.y = 1 - vertex->uv.y;
}

static void fill_out_vertex(const Array<char *> &vertex_indices, Vertex *vertex, const Vertex_Buffer *vertex_buffer)
{
	int vertex_index = atoi(vertex_indices[0]);
	int texture_index = atoi(vertex_indices[1]);
	int normal_index = atoi(vertex_indices[2]);

	vertex->position = vertex_buffer->positions[vertex_index - 1];
	vertex->uv = vertex_buffer->uvs[texture_index - 1];
	vertex->normal = vertex_buffer->normals[normal_index - 1];
}


void load_model_from_obj_file(const char *file_name, Triangle_Mesh *mesh)
{
	char *buffer = read_entire_file(file_name);
	assert(buffer);

	Array<Vertex> vertices;
	Array<u32> indices;
	//Array<Vector3> positions;
	//Array<Vector3> normals;
	//Array<Vector2> uvs;
	Vertex_Buffer vertex_buffer;

	int line_count = 0;

	while (1) {
		char *line = get_next_line(&buffer);
		if (!line) {
			break;
		}
		line_count++;
		if (line_count > 106364) {
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
		if (!strcmp(tokens[0], "v")) {
			assert(tokens.count == 4);
			vertex_buffer.positions.push(tokens_to_vector3(&tokens));

		} else if (!strcmp(tokens[0], "vt")) {
			//assert(tokens->size() == 3);
			vertex_buffer.uvs.push((Vector2)*((Vector2 *)&tokens_to_vector3(&tokens)));

		} else if (!strcmp(tokens[0], "vn")) {
			assert(tokens.count == 4);
			vertex_buffer.normals.push(tokens_to_vector3(&tokens));

		} else if (!strcmp(tokens[0], "f")) {
			if ((tokens.count - 1) == 3) {
				for (int i = 1; i < tokens.count; i++) {
					Vertex vertex;
					split_obj_face_and_fill_out_vertex(tokens[i], &vertex, &vertex_buffer);
					vertices.push(vertex);
				}

			} else if ((tokens.count - 1) == 4) {
				Vertex first_vertex;
				split_obj_face_and_fill_out_vertex(tokens[1], &first_vertex, &vertex_buffer);

				Vertex second_vertex;
				split_obj_face_and_fill_out_vertex(tokens[2], &second_vertex, &vertex_buffer);

				Vertex third_vertex;
				split_obj_face_and_fill_out_vertex(tokens[3], &third_vertex, &vertex_buffer);

				Vertex fourth_vertex;
				split_obj_face_and_fill_out_vertex(tokens[4], &fourth_vertex, &vertex_buffer);

				vertices.push(first_vertex);
				vertices.push(second_vertex);
				vertices.push(third_vertex);
				
				vertices.push(first_vertex);
				vertices.push(third_vertex);
				vertices.push(fourth_vertex);

			} else if ((tokens.count - 1) == 7) {

			} else {
				print("load_model_from_obj_file: unknown face length");
			}
		} else {
			print("[line {}] This keyworld {} isn't supported\n", line_count, tokens[0]);
		}
	}
	
	mesh->allocate_vertices(vertices.count);
	Vertex v;
	for (int i = 0; i < vertices.count; i++) {
		mesh->vertices[i].position.x = vertices[i].position.x;
		mesh->vertices[i].position.y = vertices[i].position.y;
		mesh->vertices[i].position.z = vertices[i].position.z;
		
		mesh->vertices[i].normal.x = vertices[i].normal.x;
		mesh->vertices[i].normal.y = vertices[i].normal.y;
		mesh->vertices[i].normal.z = vertices[i].normal.z;
		
		mesh->vertices[i].uv.x = vertices[i].uv.x;
		mesh->vertices[i].uv.y = vertices[i].uv.y;

	}
	
	//mesh->allocate_vertices(vertices.count);
	//memcpy(mesh->vertices, vertices.items, vertices.count);
	//mesh->allocate_indices(indices.count);
	//memcpy(mesh->indices, indices.items, indices.count);
	mesh->is_indexed = false;
}


//for (int i = 1; i < tokens.count; i++) {
//	Array<char *> vertex_indices;
//	split(tokens[i], "/", &vertex_indices);
//	int vertex_index = atoi(vertex_indices[0]);
//	int texture_index = atoi(vertex_indices[1]);
//	int normal_index = atoi(vertex_indices[2]);

//	if (vertex_index > positions.count) {
//		print("Vertex Index is more than position VI {} P {}", vertex_index, positions.count);
//	}

//	if (texture_index > uvs.count) {
//		print("Texture Index is more than uvs");
//	}
//	
//	if (normal_index > normals.count) {
//		print("Vertex Index is more than position");
//	}

//	Vertex vertex;
//	vertex.position = positions[vertex_index - 1];
//	vertex.uv = uvs[texture_index - 1];
//	vertex.normal = normals[normal_index - 1];

//	vertices.push(vertex);
//}