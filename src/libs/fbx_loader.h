#ifndef FBX_LOADER
#define FBX_LOADER

#include <stdio.h>
#include <string.h>

#include "../framework/file.h"
#include "../win32/win_types.h"

enum Property_Value_Type {
	PROPERTY_TYPE_VALUE_BOOL,
	PROPERTY_TYPE_VALUE_S8,
	PROPERTY_TYPE_VALUE_S16,
	PROPERTY_TYPE_VALUE_S32,
	PROPERTY_TYPE_VALUE_S64,
	PROPERTY_TYPE_VALUE_REAL32,
	PROPERTY_TYPE_VALUE_REAL64,
};

struct Property_Value {
	Property_Value_Type type;
	union {
		bool boolean;
		s8  int8;
		s16 int16;
		s32 int32;
		s64 int64;
		float  real32;
		double real64;
	};
};

Property_Value *read_property_value(FILE *file);


struct Fbx_Node {
	Array<Property_Value *> property_values;
	Array<Fbx_Node *> sub_nodes;

	void add_property_value(FILE *file);
	void read(FILE *file);
};

struct Fbx_Binary_File {
	Array<Fbx_Node *> root_nodes;
	
	void read(const char *file_name);
	bool check_title(FILE *file);
};
#endif