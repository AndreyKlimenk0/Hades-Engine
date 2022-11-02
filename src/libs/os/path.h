#ifndef PATH_H
#define PATH_H

#include <assert.h>

#include "../str.h"
#include "../ds/hash_table.h"



void init_os_path();
void shutdown_os_path();

void get_path_to_data_dir(const char *dir_name, String &full_path);

void build_full_path_to_map_file(const char *file_name, String &full_path);
void build_full_path_to_gui_file(const char *file_name, String &full_path);
void build_full_path_to_texture_file(const char *file_name, String &full_path);
void build_full_path_to_editor_file(const char *file_name, String &full_path);
void build_full_path_to_shader_file(const char *file_name, String &full_path);
void build_full_path_to_model_file(const char *file_name, String &full_path);

#endif