#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "str.h"


#define MAX_DIGITS_IN_INT 12
#define MAX_DIGITS_IN_LONG_LONG 21

#define bytes_of(type, count) (sizeof(type) * count)

#define COPY_STRING_TO_CHAR_ARRAY(string, char_array) {\
	int s_len = strlen(string); \
	while ((char_array->size - char_array->count) < s_len) { \
		char_array->resize();\
	} \
	char *symbol = &char_array->at(char_array->count); \
	memcpy(symbol, string, bytes_of(char, s_len));\
	char_array->count += s_len; \
}

void format_(Array<char *> *array) {}

void split(const char *string, const char *characters, Array<char *> *array)
{
	// string copy is needed so that string char array don't point on the same memory location and don't free it 
	char *string_copy = _strdup(string);
	char *next = NULL;
	array->clear();

	char *token = strtok_s(string_copy, characters, &next);
	while (token != NULL) {
		array->push(token);
		token = strtok_s(NULL, characters, &next);
	}
}

static void format_string(const char *format_string, Array<char> *formatting_string, Array<char *> *vars)
{
	assert(format_string);
	assert(formatting_string);
	assert(vars);

	int var_index = 0;
	const char *f_string = format_string;

	while (*f_string) {
		if (*f_string == '}') {
			f_string++;
			continue;
		}
		if (*f_string == '{') {
			COPY_STRING_TO_CHAR_ARRAY(vars->items[var_index], formatting_string);
			var_index++;
		} else {
			formatting_string->push(*f_string);
		}
		f_string++;
	}
}

char * __do_formatting(Array<char *> *strings)
{
	Array<char>   formatting_string;
	Array<char *> vars_buffer;

	for (int i = 0; i < strings->count; i++) {
		char *string = strings->at(i);
		int result = is_format_string(string);
		if (result) {
			assert(strings->count >= result);
			int var_index = i + 1;
			for (int j = 0; j < result; j++, i++) {
				vars_buffer.push(strings->at(var_index++));
			}
			format_string(string, &formatting_string, &vars_buffer);
			vars_buffer.clear();
			formatting_string.push(' ');
		} else {
			// Append string in buffer without needs for formatting
			COPY_STRING_TO_CHAR_ARRAY(string, ((Array<char> *)&formatting_string));
			formatting_string.push(' ');
		}
	}
	// Rewrite last not needed space
	formatting_string[formatting_string.count - 1] = '\0';
	return _strdup(formatting_string.items);
}

char *concatenate_c_str(const char *str1, const char *str2)
{
	int str1_len = strlen(str1);
	int str2_len = strlen(str2);
	int len = str1_len + str2_len;
	char *new_string = new char[len + 1];

	memcpy(new_string, str1, str1_len);
	memcpy(new_string + (sizeof(char) * str1_len), str2, str2_len);
	new_string[len] = '\0';

	return new_string;
}

char *get_next_line(char **buffer)
{
	char *t = *buffer;
	if (!*t) return NULL;
	char *s = t;

	while (*t && (*t != '\n') && (*t != '\r')) t++;

	char *end = t;
	if (*t) {
		end++;
		if (*t == '\r') {
			if (*end = '\n') end++;
		}
		*t = '\0';
	}

	*buffer = end;
	return s;
}

char *to_string(int number, int base)
{
	char *str = new char[MAX_DIGITS_IN_INT];
	memset(str, 0, MAX_DIGITS_IN_INT);
	_itoa_s(number, str, MAX_DIGITS_IN_INT, base);
	return str;
}

char *to_string(unsigned int number, int base)
{
	char *str = new char[MAX_DIGITS_IN_INT];
	memset(str, 0, MAX_DIGITS_IN_INT);
	_ultoa_s((unsigned long)number, str, MAX_DIGITS_IN_INT, base);
	return str;
}

char *to_string(long number, int base)
{
	char *str = new char[MAX_DIGITS_IN_INT];
	memset(str, 0, MAX_DIGITS_IN_INT);
	_ltoa_s(number, str, MAX_DIGITS_IN_INT, base);
	return str;
}

char *to_string(unsigned long number, int base)
{
	char *str = new char[MAX_DIGITS_IN_INT];
	memset(str, 0, MAX_DIGITS_IN_INT);
	_ultoa_s(number, str, MAX_DIGITS_IN_INT, base);
	return str;
}

char *to_string(long long number, int base)
{
	char *str = new char[21];
	memset(str, 0, 21);
	_i64toa_s(number, str, 21, base);
	return str;
}

char *to_string(float number)
{
	char *str = new char[64];
	memset(str, 0, 64);
	_sprintf_p(str, 64, "%f", number);
	return str;
}

char *to_string(double num)
{
	char *str = new char[64];
	memset(str, 0, 64);
	_sprintf_p(str, 64, "%lf", num);
	return str;
}

char *to_string(bool val)
{
	static char _true[] = "true";
	static char _false[] = "false";
	return val ? _true : _false;
}

char *to_string(const char *string)
{
	return const_cast<char *>(string);
}

// Check the string has format braces if it has return number of braces 
// if not return 0 and it means that this string is not format string 
int is_format_string(const char *string)
{
	int count = 0;
	const char *str = string;

	while (*str) {
		if (*str == '{' && *(str + 1) == '}') {
			count++;
		}
		str++;
	}
	return count;
}
