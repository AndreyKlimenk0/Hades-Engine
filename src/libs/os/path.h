#ifndef PATH_H
#define PATH_H

#include <assert.h>

#include "../str.h"
#include "../ds/hash_table.h"


struct Path {
	String base_path;
	Hash_Table<String, String> data_dir_paths;

	void init();
	void init_base_path();
	bool build_correct_base_path(Array<String> *splitted_wrong_path, String *base_path);

	void build_full_path_to_texture_file(const char *file_name, String &full_path);
	void build_full_path_to_map_file(const char *file_name, String &full_path);
	void build_full_path_to_editor_file(const char *file_name, String &full_path);
	void build_full_path_to_shader_file(const char *file_name, String &full_path);

	String *build_full_path_to_texture_file(const String *file_name);
	String *build_full_path_to_model_file(const String *file_name);
	String *build_full_path_to_shader_file(const String *file_name);
	String *build_full_path_to_editor_file(const char *file_name);
};

extern Path os_path;

inline void Path::build_full_path_to_texture_file(const char *file_name, String &full_path)
{
	String &value = data_dir_paths["texture"];
	full_path = value + "\\" + file_name;
}

inline void Path::build_full_path_to_map_file(const char *file_name, String &full_path)
{
	String &value = data_dir_paths["maps"];
	full_path = value + "\\" + file_name;
}

inline void Path::build_full_path_to_editor_file(const char * file_name, String & full_path)
{
	String &value = data_dir_paths["editor"];
	full_path = value + "\\" + file_name;
}

inline void Path::build_full_path_to_shader_file(const char * file_name, String & full_path)
{
	String &value = data_dir_paths["shader"];
	full_path = value + "\\" + file_name;
}

inline String *Path::build_full_path_to_texture_file(const String *file_name)
{
	String &value = data_dir_paths["texture"];
	String *result = new String(value + "\\" + *file_name);
	return result;
}

inline String *Path::build_full_path_to_model_file(const String *file_name)
{
	String &value = data_dir_paths["model"];
	String *result = new String(value + "\\" + *file_name);
	return result;
}

inline String *Path::build_full_path_to_shader_file(const String *file_name)
{
	String &value = data_dir_paths["shader"];
	String *result = new String(value + "\\" + *file_name);
	return result;
}

inline String *Path::build_full_path_to_editor_file(const char *file_name)
{
	String &value = data_dir_paths["editor"];
	String *result = new String(value + "\\" + file_name);
	return result;
}
#endif