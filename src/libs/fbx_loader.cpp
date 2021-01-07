#include <zlib.h>

#include "fbx_loader.h"
#include "../libs/ds/string.h"
#include "../libs/general.h"


static u32 property_value_type_size(u8 type)
{
	if (type == 'C' || type == 'B') {
		return 1;
	} else if (type == 'Y') {
		return 2;
	} else if (type == 'I') {
		return 4;
	} else if (type == 'F') {
		return 4;
	} else if (type == 'D') {
		return 8;
	} else if (type == 'L') {
		return 8;
	} else {
		return 0;
	}
}

static void read_primitive_type_value(Fbx_Node *node, FILE *file, u8 type)
{
	Fbx_Property *value = new Fbx_Property();
	value->type = PROPERTY_TYPE_VALUE;
	if (type == 'C' || type == 'B') {
		value->value_type = PROPERTY_VALUE_TYPE_BOOL;
		value->value.boolean = (bool)read_u8(file);
	} else if (type == 'Y') {
		value->value_type = PROPERTY_VALUE_TYPE_S16;
		value->value.int16 = read_s16(file);
	} else if (type == 'I') {
		value->value_type = PROPERTY_VALUE_TYPE_S32;
		value->value.int32 = read_s32(file);
	} else if (type == 'L') {
		value->value_type = PROPERTY_VALUE_TYPE_S64;
		value->value.int64 = read_s64(file);
	} else if (type == 'F') {
		value->value_type = PROPERTY_VALUE_TYPE_REAL32;
		value->value.real32 = read_real32(file);
	} else if (type == 'D') {
		value->value_type = PROPERTY_VALUE_TYPE_REAL64;
		value->value.real64 = read_real64(file);
	} else if (type == 'S' || type == 'R') {
		u32 len = read_u32(file);
		value->value_type = PROPERTY_VALUE_TYPE_STRING;
		value->value.string = read_string(file, len);
	} else {
		printf("read_primitive_type_value didn't determine type of value\n");
		assert(false);
	}
	node->properties.push(value);
}

static void read_array_data(Fbx_Node *node, FILE *file, u8 type)
{
	Fbx_Property *array = new Fbx_Property();
	array->type = PROPERTY_TYPE_VALUE_ARRAY;
	
	u32 array_count = read_u32(file);
	u32 encoding = read_u32(file);
	u64 compressed_len = read_u32(file);

	u64 array_byte_len = property_value_type_size(type - ('a' - 'A')) * array_count;
	if (encoding) {
		u8 *decompressed_buffer = new u8[array_byte_len];
		u8 *compressed_buffer = new u8[compressed_len];

		fread(compressed_buffer, sizeof(u8), compressed_len, file);
		int success = uncompress(decompressed_buffer, &array_byte_len, compressed_buffer, compressed_len);
		if (success != Z_OK) {
			printf("Erorr: Fbx property array decompressing failed\n");
		}
		array->copy_data_to_array(decompressed_buffer, array_byte_len, array_count, type);
		
		DELETE_PTR(decompressed_buffer);
		DELETE_PTR(compressed_buffer);
	} else {
		u8 *buffer = new u8[array_byte_len];
		fread(buffer, sizeof(u8), array_byte_len, file);
		array->copy_data_to_array(buffer, array_byte_len, array_count, type);
		
		DELETE_PTR(buffer);
	}
	node->properties.push(array);
}


static void read_property_value(Fbx_Node *node, FILE *file)
{
	u8 type = read_u8(file);
	if (type > 'A' && type < 'Z') {
		read_primitive_type_value(node, file, type);
	} else if (type > 'a' && type < 'z') {
		read_array_data(node, file, type);
	}
}

Fbx_Property::~Fbx_Property()
{
	if (type = PROPERTY_TYPE_VALUE_ARRAY) {
		switch (value_type) {
			case PROPERTY_VALUE_TYPE_BOOL: {
				delete array.boolean;
				break;
			}
			case PROPERTY_VALUE_TYPE_S32: {
				delete array.int32;
				break;
			}
			case PROPERTY_VALUE_TYPE_S64: {
				delete array.int64;
				break;
			}
			case PROPERTY_VALUE_TYPE_REAL32: {
				delete array.real32;
				break;
			}
			case PROPERTY_VALUE_TYPE_REAL64: {
				delete array.real64;
				break;
			}
		}
	} else {
		if (value_type == PROPERTY_VALUE_TYPE_STRING) {
			delete value.string;
		}
	}
}

void Fbx_Property::copy_data_to_array(u8 *data, u32 array_byte_len, u32 array_count, u8 type)
{
	array.count = array_count;
	if (type == 'f') {
		value_type = PROPERTY_VALUE_TYPE_REAL32;
		array.real32 = new float[array_count];
		memcpy(array.real32, data, array_byte_len);
	} else if (type == 'd') {
		value_type = PROPERTY_VALUE_TYPE_REAL64;
		array.real64 = new double[array_count];
		memcpy(array.real64, data, array_byte_len);
	} else if (type == 'i') {
		value_type = PROPERTY_VALUE_TYPE_S32;
		array.int32 = new s32[array_count];
		memcpy(array.int32, data, array_byte_len);
	} else if (type == 'l') {
		value_type = PROPERTY_VALUE_TYPE_S64;
		array.int64 = new s64[array_count];
		memcpy(array.int64, data, array_byte_len);
	} else if (type == 'b') {
		// @Node: this code isn't tested
		value_type = PROPERTY_VALUE_TYPE_BOOL;
		array.boolean = new bool[array_count];
		for (u32 i = 0; i < array_count; i++) {
			array.boolean[i] = (bool)data[i] != 0;
		}
	} else {
		assert(false);
	}
}

Fbx_Node::~Fbx_Node()
{
	if (name) {
		delete name;
	}
}

void Fbx_Node::add_property_value(FILE *file)
{
	read_property_value(this, file);
}

bool Fbx_Node::is_null()
{
	return sub_nodes.count == 0 && properties.count == 0 && strlen(name) == 0;
}

Fbx_Node *find_fbx_node(const Array<Fbx_Node *> *nodes, const char *name)
{
	for (int i = 0; i < nodes->count; i++) {
		Fbx_Node *node = nodes->at(i);
		if (node->is_null()) {
			return NULL;
		}

		if (!strcmp(node->name, name)) {
			return node;
		} else {
			Fbx_Node * searched_node = find_fbx_node(&node->sub_nodes, name);
			if (!searched_node) {
				continue;
			}
			return searched_node;
		}
	}
	return NULL;
}

u32 Fbx_Node::read(FILE *file, u32 start_offset)
{
	u32 bytes = 13;
	u32 end_offset = read_u32(file);
	u32 number_properties = read_u32(file);
	u32 property_list_len = read_u32(file);
	u8 name_len = read_u8(file);
	name = read_string(file, name_len);


	for (u32 i = 0; i < number_properties; i++) {
		add_property_value(file);
	}

	bytes += name_len;
	bytes += property_list_len;

	while (start_offset + bytes < end_offset) {
		Fbx_Node *node = new Fbx_Node();
		bytes += node->read(file, start_offset + bytes);
		sub_nodes.push(node);
	}
	return bytes;
}


void Fbx_Binary_File::read(const char *file_name)
{
	FILE *file;
	if (fopen_s(&file, file_name, "rb")) {
		printf("Fbx_Binary_FIle::read can't open the file by path %s\n", file_name);
		return;
	}

	bool success = check_title(file);
	u32 start_offset = 27;
	while (1) {
		Fbx_Node *node = new Fbx_Node();
		start_offset += node->read(file, start_offset);
		if (node->is_null()) {
			break;
		}
		root_nodes.push(node);
	}

}

bool Fbx_Binary_File::check_title(FILE *file)
{
	bool result = true;
	u32 max_fbx_version = 7400;
	char *magic_string = new char[21];
	static char fbx_title[21] = "Kaydara FBX Binary  ";

	fread(magic_string, sizeof(u8), 21, file);
	if (strcmp(fbx_title, magic_string)) {
		printf("Fbx magic string failed, it can be not fbx file\n");
		result = false;
	}

	if (read_u8(file) != 0x1A) {
		result = false;
	}

	if (read_u8(file) != 0x00) {
		result = false;
	}

	u32 version = read_u32(file);
	if (version > max_fbx_version) {
		printf("Unsupported FBX file version %d, needed file version %d or less\n", version, max_fbx_version);
		result = false;
	}
	DELETE_PTR(magic_string);
	return result;
}


Fbx_Node *Fbx_Binary_File::get_node(const char *name)
{
	return find_fbx_node(&root_nodes, name);
}

#define COPY_VERTICES_TO_MESH_FROM_FBX_PROPERTY_ARRAY(mesh, vertex_property, pointer_name, uv_property, uv_index_property) \
				for (u32 i = 0; i < mesh->vertex_count; i++) { \
					s32 uv_index = uv_index_property->array.int32[i]; \
					mesh->vertices[i].position.x = static_cast<float32>(vertex_property->array. ## pointer_name ##[i * 3]); \
					mesh->vertices[i].position.y = static_cast<float32>(vertex_property->array. ## pointer_name ##[i * 3 + 1]); \
					mesh->vertices[i].position.z = static_cast<float32>(vertex_property->array. ## pointer_name ##[i * 3 + 2]); \
					mesh->vertices[i].uv.x = uv_property->array.real64[uv_index * 2]; \
					mesh->vertices[i].uv.y = uv_property->array.real64[uv_index * 2 + 1]; \
				} \
								
void Fbx_Binary_File::fill_out_mesh(Triangle_Mesh *mesh)
{
	Fbx_Node *vertices = get_node("Vertices");
	Fbx_Node *indices = get_node("PolygonVertexIndex");
	Fbx_Node *normals = get_node("Normals");
	Fbx_Node *uv =  get_node("UV");
	Fbx_Node *uv_indices =  get_node("UVIndex");

	if (!vertices || !indices || !normals || !uv || !uv_indices) {
		printf("Some mesh data from the fbx file for the triangle mesh wasn't found\n");
		return;
	}

	Fbx_Property *vertex_property = vertices->properties[0];
	Fbx_Property *index_property = indices->properties[0];
	Fbx_Property *normal_property = normals->properties[0];
	Fbx_Property *uv_property = uv->properties[0];
	Fbx_Property *uv_index_property = uv_indices->properties[0];
	
	if (vertex_property->type != PROPERTY_TYPE_VALUE_ARRAY || index_property->type != PROPERTY_TYPE_VALUE_ARRAY) {
		printf("Vertex fbx property of index fbx property doesn't appropriate array type\n");
		return;
	}
	
	assert(vertex_property->array.count / 3 != 0);
	mesh->allocate_vertices(vertex_property->array.count / 3);

	switch (vertex_property->value_type) {
		case PROPERTY_VALUE_TYPE_REAL32: {
			COPY_VERTICES_TO_MESH_FROM_FBX_PROPERTY_ARRAY(mesh, vertex_property, real32, uv_property, uv_index_property);
			break;
		}
		case PROPERTY_VALUE_TYPE_REAL64: {
			COPY_VERTICES_TO_MESH_FROM_FBX_PROPERTY_ARRAY(mesh, vertex_property, real64, uv_property, uv_index_property)
			break;
		}
		case PROPERTY_VALUE_TYPE_S32: {
			COPY_VERTICES_TO_MESH_FROM_FBX_PROPERTY_ARRAY(mesh, vertex_property, int32, uv_property, uv_index_property)
			break;
		}
		case PROPERTY_VALUE_TYPE_S64: {
			COPY_VERTICES_TO_MESH_FROM_FBX_PROPERTY_ARRAY(mesh, vertex_property, int64, uv_property, uv_index_property)
			break;
		}
	}


	if (index_property->array.int32[2] < 0) {
		mesh->allocate_indices(index_property->array.count);
		u32 triangle_index_count = mesh->index_count / 3;
		for (int i = 0; i < triangle_index_count; i++) {
			mesh->indices[i * 3]  = index_property->array.int32[i * 3];
			mesh->indices[i * 3 + 1] = index_property->array.int32[i * 3 + 1];
			mesh->indices[i * 3 + 2] = (index_property->array.int32[i * 3 + 2] * -1) - 1;

		}
	} else if (index_property->array.int32[3] < 0) {
		mesh->allocate_indices((index_property->array.count / 4) * 6);
		int k = 0;
		s32 quard_index_count = mesh->index_count / 4;
		for (int i = 0; i < quard_index_count; i++) {
			s32 first_index  = index_property->array.int32[i * 4];
			s32 second_index = index_property->array.int32[i * 4 + 1];
			s32 thrid_index  = index_property->array.int32[i * 4 + 2];
			s32 fourth_index = (index_property->array.int32[i * 4 + 3] * -1) -1;

			if (k < mesh->index_count) {
				mesh->indices[k] = first_index;
				mesh->indices[k + 1] = second_index;
				mesh->indices[k + 2] = thrid_index;

				mesh->indices[k + 3] = thrid_index;
				mesh->indices[k + 4] = fourth_index;
				mesh->indices[k + 5] = first_index;
				k += 6;
			}
		}
	}
}

char *Fbx_Binary_File::get_texture_name()
{
	Fbx_Node *texture_name = find_fbx_node(&root_nodes, "TextureName");
	if (!texture_name) {
		//@Note: here need to be warning message.
		return NULL;
	}
	Fbx_Property *property = texture_name->properties[0];

	if (property->is_property_type_value() && property->is_property_value_type_string()) {
		Array<char *> buffer;
		split(property->value.string, "::", &buffer);
		return _strdup(buffer[1]);
	}
	//@Note: here need to be warning message.
	return NULL;
}
