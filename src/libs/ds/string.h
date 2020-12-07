#ifndef STRING_H
#define STRING_H

#include "array.h"

char *concatenate_c_str(const char *str1, const char *str2);
void split(char *string, const char *characters, Array<char *> *array);
char *get_next_line(char **buffer);
#endif