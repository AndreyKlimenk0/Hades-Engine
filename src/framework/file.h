#ifndef FILE_H
#define FILE_H

#include "../elib/ds/array.h"

char *read_entire_file(const char *name, const char *mode = "r");
Array<char *> *get_file_names_from_dir(const char *full_path);

#endif