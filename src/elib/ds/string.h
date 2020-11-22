#ifndef STRING_H
#define STRING_H

#include "array.h"

char *concatenate_c_str(const char *str1, const char *str2);
Array<char *> *split(char *string, const char *characters);

#endif