#ifndef FILE_H
#define FILE_H

#include "../libs/ds/array.h"

char *read_entire_file(const char *name, const char *mode = "r", int *file_size = NULL);
bool get_file_names_from_dir(const char *full_path, Array<char *> *file_names);

#endif