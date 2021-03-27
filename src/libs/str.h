#ifndef STRING_H
#define STRING_H

#include "ds/array.h"
#include "../win32/win_local.h"


void format_(Array<char *> *array);
void split(const char *string, const char *characters, Array<char *> *array);

char *get_next_line(char **buffer);
char *concatenate_c_str(const char *str1, const char *str2);

char *to_string(int number, int base = 10);
char *to_string(unsigned int number, int base = 10);
char *to_string(long number, int base = 10);
char *to_string(unsigned long number, int base = 10);
char *to_string(long long number, int base = 10);
char *to_string(float number);
char *to_string(double num);
char *to_string(bool val);
char *to_string(const char *string);

int is_format_string(const char *string);


template <typename First, typename... Args>
void format_(Array<char *> *array, First first, Args... args)
{
	char *temp = to_string(first);
	array->push(temp);
	format_(array, args...);
}

char * __do_formatting(Array<char *> *strings);

template <typename... Args>
char *format(Args... args)
{
	Array<char *> strings;
	format_(&strings, args...);
	return __do_formatting(&strings);
}
#endif