#include "fbx_loader.h"

static Property_Value *read_array_data(FILE *file, u8 type)
{
	return new Property_Value();
}

static Property_Value *read_primitive_type_value(FILE *file, u8 type)
{
	Property_Value *value = new Property_Value();
	if (type == 'C') {
		value->type = PROPERTY_TYPE_VALUE_BOOL;
		value->boolean = (bool)read_u8(file);
	} else if (type == 'Y') {
		value->type = PROPERTY_TYPE_VALUE_S16;
		value->int16 = read_s16(file);
	} else if (type == 'I') {
		value->type = PROPERTY_TYPE_VALUE_S32;
		value->int32 = read_s32(file);
	} else if (type == 'L') {
		value->type = PROPERTY_TYPE_VALUE_S64;
		value->int64 = read_s64(file);
	} else if (type == 'F') {
		value->type = PROPERTY_TYPE_VALUE_REAL32;
		value->real32 = read_real32(file);
	} else if (type == 'D') {
		value->type = PROPERTY_TYPE_VALUE_REAL64;
		value->real64 = read_real64(file);
	} else {
		printf("read_primitive_type_value didn't determine type of value\n");
		return NULL;
	}
	return value;
}


Property_Value *read_property_value(FILE *file)
{
	u8 type = read_u8(file);
	if (type > 'A' && type < 'Z') {
		return read_primitive_type_value(file, type);
	} else if (type > 'a' && type < 'z') {
		return read_array_data(file, type);
	}
	return NULL;
}

void Fbx_Node::add_property_value(FILE *file)
{
	Property_Value *value = read_property_value(file);
	if (value) {
		property_values.push(value);
		printf("Value %d\n", value->int32);
	} else {
		printf("Warning: property value wasn't read\n");
	}
}


void Fbx_Node::read(FILE *file)
{
	u32 end_offset = read_u32(file);
	u32 number_properties = read_u32(file);
	u32 property_list_len = read_u32(file);
	u8 name_len = read_u8(file);
	char *name = read_string(file, name_len);

	for (u32 i = 0; i < number_properties; i++) {
		add_property_value(file);
	}

	u32 end_offset2 = read_u32(file);
	u32 number_properties2 = read_u32(file);
	u32 property_list_len2 = read_u32(file);
	u8 name_len2 = read_u8(file);
	char *name2 = read_string(file, name_len2);

	for (u32 i = 0; i < number_properties2; i++) {
		add_property_value(file);
	}

}


void Fbx_Binary_File::read(const char *file_name)
{
	FILE *file;
	if (fopen_s(&file, file_name, "rb")) {
		printf("Fbx_Binary_FIle::read can't open the file by path %s\n", file_name);
		return;
	}
	
	bool success = check_title(file);

	//while (1) {
	Fbx_Node *node = new Fbx_Node();
	node->read(file);
	//}

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
