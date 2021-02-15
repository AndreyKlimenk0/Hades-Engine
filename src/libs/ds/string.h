#ifndef STRING_H
#define STRING_H

#include "array.h"
#include <windows.h>

void format_(Array<char *> *array);
void split(char *string, const char *characters, Array<char *> *array);
void format_string(const char *format_string, Array<char> *formatting_string, Array<char *> *vars);

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

void concatenate_string_with_format_string(const char *string, Array<char> *formatting_string);

//#define COPY_STRING_TO_CHAR_ARRAY(string, char_array);


template <typename... Args>
char *format(Args... args)
{
	Array<char>   formatting_string;
	Array<char *> strings;
	Array<char *> vars_buffer;

	format_(&strings, args...);

	for (int i = 0; i < strings.count; i++) {
		char *string = strings[i];
		int result = is_format_string(string);
		if (result) {
			assert(strings.count >= result);
			int var_index = i + 1;
			for (int j = 0; j < result; j++, i++) {
				vars_buffer.push(strings[var_index++]);
			}
			format_string(string, &formatting_string, &vars_buffer);
			vars_buffer.clear();
		} else {
			concatenate_string_with_format_string(string, &formatting_string);
		}
	}
	formatting_string.push('\0');
	return _strdup(formatting_string.items);
}

#endif