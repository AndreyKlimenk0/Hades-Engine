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
			char *string;
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
	void copy_data_to_array(u8 *data, u32 array_byte_len, u32 array_count, u8 type);
	auto get_value_from_array(int index);
};

struct Fbx_Node {
	char *name;
	Array<Fbx_Property *> properties;
	Array<Fbx_Node *> sub_nodes;

	void add_property_value(FILE *file);
	bool is_null();
	u32 read(FILE *file, u32 start_offset);
};


struct Fbx_Binary_File {
	Array<Fbx_Node *> root_nodes;
	
	void read(const char *file_name);
	void fill_out_mesh(Triangle_Mesh *mesh);
	bool check_title(FILE *file);
	Fbx_Node *get_node(const char *name);
};

Fbx_Node *fbx_node_searcher(const Array<Fbx_Node *> *nodes, const char *name);
#endif