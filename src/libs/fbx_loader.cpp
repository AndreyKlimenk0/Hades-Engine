#include <zlib.h>

#include "fbx_loader.h"
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

Fbx_Node *fbx_node_searcher(const Array<Fbx_Node *> *nodes, const char *name)
{
	for (int i = 0; i < nodes->count; i++) {
		Fbx_Node *node = nodes->at(i);
		if (node->is_null()) {
			return NULL;
		}

		if (!strcmp(node->name, name)) {
			return node;
		} else {
			Fbx_Node * searched_node = fbx_node_searcher(&node->sub_nodes, name);
			if (!searched_node) {
				continue;
			}
			return searched_node;
		}
	}
	return NULL;
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

void Fbx_Node::add_property_value(FILE *file)
{
	read_property_value(this, file);
}

bool Fbx_Node::is_null()
{
	return sub_nodes.count == 0 && properties.count == 0 && strlen(name) == 0;
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
	u32 max_fbx_version = 7400;
	static char fbx_title[21] = "Kaydara FBX Binary  ";
	char *magic_string = new char[21];

	fread(magic_string, sizeof(u8), 21, file);
	if (strcmp(fbx_title, magic_string)) {
		printf("Fbx magic string failed, it can be not fbx file");
		return false;
	}

	if (read_u8(file) != 0x1A) {
		return false;
	}

	if (read_u8(file) != 0x00) {
		return false;
	}

	u32 version = read_u32(file);
	if (version > max_fbx_version) {
		printf("Unsupported FBX file version %d, needed file version %d or less", version, max_fbx_version);
		return false;
	}
	return true;
}


Fbx_Node *Fbx_Binary_File::get_node(const char *name)
{
	return fbx_node_searcher(&root_nodes, name);
}

auto Fbx_Property::get_value_from_array(int index)
{
	assert(type != PROPERTY_TYPE_VALUE);
	switch(value_type) 
}

//#define RETURN_NEEDED_FBX_PROPETY_ARRAY_FIED(property)
#define COPY_VERTICES_TO_MESH_FROM_FBX_PROPERTY_AND_CAST_TO_FLOAT32(type, mesh, property) \
				for (int i = 0; i < mesh->vertex_count; i++) { \
					mesh->vertices[i].position.x = static_cast<type>(vertex_property->array.real32[i]); \
					mesh->vertices[i].position.y = vertex_property->array.real32[i]; \
					mesh->vertices[i].position.z = vertex_property->array.real32[i]; \
				} \

void Fbx_Binary_File::fill_out_mesh(Triangle_Mesh *mesh)
{
	Fbx_Node *vertices = get_node("Vertices");
	Fbx_Node *indices = get_node("PolygonVertexIndex");

	if (!vertices || !indices) {
		printf("Vertex or index data from the fbx file for the triangle mesh wasn't foundn");
		return;
	}

	Fbx_Property *vertex_property = vertices->properties[0];
	Fbx_Property *index_property = indices->properties[0];
	
	if (vertex_property->type != PROPERTY_TYPE_VALUE_ARRAY || index_property->type != PROPERTY_TYPE_VALUE_ARRAY) {
		printf("Vertex fbx property of index fbx property doesn't appropriate array type\n");
	}

	assert(vertex_property->array.count / 3 != 0);
	mesh->allocate_vertices(vertex_property->array.count / 3);
	mesh->allocate_indices(index_property->array.count);

	mesh->indices = (u32 *)index_property->array.int32;
	index_property->array.int32 = NULL;
	index_property->array.count = 0;

	switch (vertex_property->value_type) {
		case PROPERTY_VALUE_TYPE_REAL32: {
			for (int i = 0; i < mesh->vertex_count; i++) {
				mesh->vertices[i].position.x = vertex_property->array.real32[i * 1];
				mesh->vertices[i].position.y = vertex_property->array.real32[i * 2];
				mesh->vertices[i].position.z = vertex_property->array.real32[i * 3];
				mesh->vertices[i].color = Yellow;
			}
			break;
		}
		case PROPERTY_VALUE_TYPE_REAL64: {
			for (int i = 0; i < mesh->vertex_count; i++) {
				mesh->vertices[i].position.x = static_cast<float32>(vertex_property->array.real64[i * 3]);
				mesh->vertices[i].position.y = static_cast<float32>(vertex_property->array.real64[i * 3 + 1]);
				mesh->vertices[i].position.z = static_cast<float32>(vertex_property->array.real64[i * 3 + 2]);
				mesh->vertices[i].color = Yellow;
			}
			break;
		}
		case PROPERTY_VALUE_TYPE_S32: {
			for (int i = 0; i < mesh->vertex_count; i++) {
				mesh->vertices[i].position.x = static_cast<float32>(vertex_property->array.int32[i * 1]);
				mesh->vertices[i].position.y = static_cast<float32>(vertex_property->array.int32[i * 2]);
				mesh->vertices[i].position.z = static_cast<float32>(vertex_property->array.int32[i * 3]);
				mesh->vertices[i].color = Yellow;
			}
			break;
		}
		case PROPERTY_VALUE_TYPE_S64: {
			for (int i = 0; i < mesh->vertex_count; i++) {
				mesh->vertices[i].position.x = static_cast<float32>(vertex_property->array.int64[i * 1]);
				mesh->vertices[i].position.y = static_cast<float32>(vertex_property->array.int64[i * 2]);
				mesh->vertices[i].position.z = static_cast<float32>(vertex_property->array.int64[i * 3]);
				mesh->vertices[i].color = Yellow;
			}
			break;
		}
	}

}
