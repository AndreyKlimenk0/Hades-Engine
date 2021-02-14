#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "string.h"


#define MAX_DIGITS_IN_INT 12
#define MAX_DIGITS_IN_LONG_LONG 21


void format_(Array<char *> *array) {}

void split(char *string, const char *characters, Array<char *> *array)
{
	char *next = NULL;
	array->abort();

	char *token = strtok_s(string, characters, &next);
	while (token != NULL) {
		array->push(token);
		token = strtok_s(NULL, characters, &next);
	}
}

void make_format_string(const char *string, Array<char> *buffer, Array<char *> *format_elements)
{
	int index_string = 0;
	while (!format_elements->is_empty() || string[index_string]) {
		if (string[index_string] == '{') {
			char *format_element = format_elements->pop();
			for (int i = 0; i < strlen(format_element); i++) {
				buffer->push(format_element[i]);
			}
			// Jump through '}'
			index_string += 2;
		} else if (index_string >= strlen(string)) {
			// Push remaining elements
			char *tail_string = format_elements->pop();
			buffer->push(',');
			buffer->push(' ');
			for (int i = 0; i < strlen(tail_string); i++) {
				buffer->push(tail_string[i]);
			}
		} else {
			buffer->push(string[index_string++]);
		}
	}
	buffer->push(',');
	buffer->push(' ');
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
	_sprintf_p(str, 64, "%fL", num);
	return str;
}

char *to_string(bool val)
{
	static char _true[] = "true";
	static char _false[] = "false";
	if (val)
		return _true;
	return _false;
	//return val ? _true : _false;
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
