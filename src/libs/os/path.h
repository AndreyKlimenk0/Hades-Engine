#ifndef PATH_H
#define PATH_H

#include "../str.h"

void init_os_path();
void shutdown_os_path();

bool build_full_path_to_data_directory(const char *directory_name, String &full_path);

void build_full_path_to_level_file(const char *file_name, String &full_path);
void build_full_path_to_gui_file(const char *file_name, String &full_path);
void build_full_path_to_texture_file(const char *file_name, String &full_path);
void build_full_path_to_texture_file(const char *file_name, const String &subdirectory, String &full_path);
void build_full_path_to_editor_file(const char *file_name, String &full_path);
void build_full_path_to_shader_file(const char *file_name, String &full_path);
void build_full_path_to_source_shader_file(const char *file_name, String &full_path);
void build_full_path_to_model_file(const char *file_name, String &full_path);

const char *get_base_path();
const char *get_full_path_to_data_directory();

String join_paths(const String &first_path, const String &second_path);

#endif
