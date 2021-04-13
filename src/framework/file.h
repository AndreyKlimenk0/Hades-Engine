#ifndef FILE_H
#define FILE_H

#include "../win32/win_types.h"
#include "../libs/str.h"
#include "../libs/ds/array.h"
#include "../libs/ds/hash_table.h"


bool get_file_names_from_dir(const char *full_path, Array<char *> *file_names);

u8  read_u8(FILE *file);
u16 read_u16(FILE *file);
u32 read_u32(FILE *file);
u64 read_u64(FILE *file);

s8  read_s8(FILE *file);
s16 read_s16(FILE *file);
s32 read_s32(FILE *file);
s64 read_s64(FILE *file);

float read_real32(FILE *file);
double read_real64(FILE *file);

char *read_string(FILE *file, int len);
char *read_entire_file(const char *name, const char *mode = "r", int *file_size = NULL);
char *extract_file_name(const char *file_path);
char *extract_file_extension(const char *file_path);

struct Path {
	String base_path;
	Hash_Table<String, String> data_dir_paths;

	void init();
	void init_base_path();
	bool build_correct_base_path(Array<String> *splitted_wrong_path, String *base_path);
	
	String *build_full_path_to_texture_file(const String *file_name);
	String *build_full_path_to_model_file(const String *file_name);
	String *build_full_path_to_shader_file(const String *file_name);
};

extern Path os_path;

inline String *Path::build_full_path_to_texture_file(const String *file_name)
{
	String value;
	data_dir_paths.get("texture", value);
	String *result = new String(value + "\\" + *file_name);
	return result;
}

inline String *Path::build_full_path_to_model_file(const String *file_name)
{
	String value;
	data_dir_paths.get("model", value);
	String *result = new String(value + "\\" + *file_name);
	return result;
}

inline String *Path::build_full_path_to_shader_file(const String *file_name)
{
	String value;
	data_dir_paths.get("shader", value);
	String *result = new String(value + "\\" + *file_name);
	return result;
}
#endif