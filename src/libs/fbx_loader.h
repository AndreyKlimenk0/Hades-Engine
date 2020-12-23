#ifndef FBX_LOADER
#define FBX_LOADER

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "../render/mesh.h"
#include "../framework/file.h"
#include "../win32/win_types.h"

enum Property_Value_Type {
	PROPERTY_VALUE_TYPE_BOOL,
	PROPERTY_VALUE_TYPE_S8,
	PROPERTY_VALUE_TYPE_S16,
	PROPERTY_VALUE_TYPE_S32,
	PROPERTY_VALUE_TYPE_S64,
	PROPERTY_VALUE_TYPE_REAL32,
	PROPERTY_VALUE_TYPE_REAL64,
	PROPERTY_VALUE_TYPE_STRING
};

enum Fbx_Property_Type {
	PROPERTY_TYPE_VALUE,
	PROPERTY_TYPE_VALUE_ARRAY
};

struct Fbx_Property {

	Fbx_Property_Type type;
	Property_Value_Type value_type;
	union {
		union {
			bool boolean;
			s8  int8;
			s16 int16;
			s32 int32;
			s64 int64;
			float  real32;
			double real64;
			char *string = NULL;
		} value;
		struct {
			union {
				bool *boolean;
				s32 *int32;
				s64 *int64;
				float  *real32;
				double *real64;
			};
			u32 count;
		} array;
	};
	Fbx_Property() {};
	~Fbx_Property();
	void copy_data_to_array(u8 *data, u32 array_byte_len, u32 array_count, u8 type);
	bool is_property_type_value() { return type == PROPERTY_TYPE_VALUE; }
	bool is_property_type_value_array() { return type == PROPERTY_TYPE_VALUE_ARRAY; }
	bool is_property_value_type_string() { return value_type == PROPERTY_VALUE_TYPE_STRING; }
};

struct Fbx_Node {
	char *name = NULL;
	Array<Fbx_Property *> properties;
	Array<Fbx_Node *> sub_nodes;

	~Fbx_Node();
	void add_property_value(FILE *file);
	bool is_null();
	u32 read(FILE *file, u32 start_offset);
};


struct Fbx_Binary_File {
	Array<Fbx_Node *> root_nodes;
	
	void read(const char *file_name);
	void fill_out_mesh(Triangle_Mesh *mesh);
	bool check_title(FILE *file);
	char *get_texture_name();
	Fbx_Node *get_node(const char *name);
};

Fbx_Node *find_fbx_node(const Array<Fbx_Node *> *nodes, const char *name);
#endif