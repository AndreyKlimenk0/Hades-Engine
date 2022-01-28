#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include "str.h"
#include "../sys/sys_local.h"


#define MAX_DIGITS_IN_INT 12
#define MAX_DIGITS_IN_LONG_LONG 21

#define bytes_of(type, count) (sizeof(type) * count)

#define COPY_STRING_TO_CHAR_ARRAY(string, char_array) {\
	int s_len = strlen(string); \
	while ((char_array->size - char_array->count) < s_len) { \
		char_array->resize(char_array->size * 2);\
	} \
	char *symbol = &char_array->at(char_array->count); \
	memcpy(symbol, string, bytes_of(char, s_len));\
	char_array->count += s_len; \
}

void format_(Array<char *> *array) {}

void split(const char *string, const char *characters, Array<char *> *array)
{
	// string copy is needed so that char array don't point on the same memory location and don't free it 
	char *string_copy = _strdup(string);
	char *next = NULL;
	array->clear();

	char *token = strtok_s(string_copy, characters, &next);
	while (token != NULL) {
		array->push(token);
		token = strtok_s(NULL, characters, &next);
	}
}

bool split(String *string, const char *symbols, Array<String> *array)
{
	int curr_pos = 0;
	int prev_pos = 0;
	int	len = strlen(symbols);

	while ((curr_pos = string->find_text(symbols, curr_pos)) != -1) {
		array->push(String(string->data, prev_pos, curr_pos));
		prev_pos = curr_pos + len;
		curr_pos += len;
	}

	if ((prev_pos == 0) && (curr_pos == -1)) {
		return false;
	}

	if (prev_pos != string->len) {
		array->push(String(*string, prev_pos, string->len));
	}

	return true;
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
		}
		else {
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
		}
		else {
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

char *to_string(s64 number, int base)
{
	char *str = new char[21];
	memset(str, 0, 21);
	_i64toa_s(number, str, 21, base);
	return str;
}

char *to_string(u64 number, int base)
{
	char *str = new char[21];
	memset(str, 0, 21);
	_ui64toa_s(number, str, 21, base);
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

char *to_string(char c)
{
	char *str = new char[2];
	str[0] = c;
	str[1] = '\0';
	return str;
}

char *to_string(String &string)
{
	char *str = new char[string.len + 1];
	memcpy(str, string.data, string.len + 1);
	return str;
}

char *to_string(String *string)
{
	assert(string->len != 0);

	char *str = new char[string->len + 1];
	memcpy(str, string->data, string->len + 1);
	return str;
}

char *to_string(Vector3 *vector)
{
	return format("{} {} {}", vector->x, vector->y, vector->z);
}

// Check the string has format braces if it has return number of braces 
// if not return 0 and it means that this string is not format string 
int is_format_string(const char *string)
{
	assert(string);

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

String::~String()
{
	DELETE_ARRAY(data);
}

String::String(int number)
{
	char *num = to_string(number);
	*this = num;
	DELETE_PTR(num);
}

String::String(float number)
{
	char *num = to_string(number);
	*this = num;
	DELETE_PTR(num);
}

String::String(const char *string)
{
	assert(string != NULL);
	assert(string[0] != '\0');

	if (string == data) {
		return;
	}

	allocate_and_copy_string(string);
}

String::String(const char *string, int start, int end)
{
	int l = end - start;
	const char *c = string;
	c += start;

	data = new char[l + 1];
	memcpy(data, c, sizeof(char) * l);
	data[l] = '\0';
}

String::String(const String &string, int start, int end)
{
	int l = end - start;
	const char *c = string.data;
	c += start;

	data = new char[l + 1];
	memcpy(data, c, sizeof(char) * l);
	data[l] = '\0';
}

String::String(const String *other)
{
	DELETE_ARRAY(data);
	allocate_and_copy_string(other->data);
}

String::String(const String &other)
{
	DELETE_ARRAY(data);
	allocate_and_copy_string(other.data);
}

String &String::operator=(const char *string)
{
	assert(string != NULL);
	assert(string[0] != '\0');

	if (string == data) {
		return *this;
	}

	DELETE_ARRAY(data);
	allocate_and_copy_string(string);
	return *this;
}

String &String::operator=(const String &other)
{
	if (!other.data) {
		return *this;
	}

	if (this == &other) {
		return *this;;
	}

	DELETE_ARRAY(data);
	allocate_and_copy_string(other.data);
	return *this;
}

void String::free()
{
	DELETE_ARRAY(data);
	len = 0;
}

void String::print()
{
	::print(data);
}

void String::to_lower()
{
	assert(len > 0);

	for (int i = 0; i < len; i++) {
		if (isupper(data[i])) {
			data[i] += ('a' - 'A');
		}
	}
}

void String::pop_char()
{
	if (data[0] == '\0')
		return;

	String *other_str = copy();
	other_str->data[other_str->len - 1] = '\0';

	other_str->print();
	*this = *other_str;
	DELETE_PTR(other_str);
}

void String::insert(int index, char c)
{
	assert(len >= index);

	String *copied_str = copy();

	DELETE_PTR(data);

	data = new char[len + 2]; // allocate place for new char and \0

	char *first_part_dest_str = data;
	char *first_part_src_str = copied_str->data;

	memcpy(first_part_dest_str, first_part_src_str, sizeof(char) * index);

	data[index] = c;

	char *second_part_dest_str = &data[index + 1];
	char *second_part_src_str = &copied_str->data[index];

	memcpy(second_part_dest_str, second_part_src_str, sizeof(char) * (len - index));

	len += 1;
	data[len] = '\0';

	DELETE_PTR(copied_str);
}

void String::remove(int index)
{
	assert(len > index);

	String *copied_str = copy();

	DELETE_PTR(data);

	data = new char[len]; //decrease len of string on 1

	char *first_part_dest_str = data;
	char *first_part_src_str = copied_str->data;

	memcpy(first_part_dest_str, first_part_src_str, sizeof(char) * index);

	char *second_part_dest_str = &data[index];
	char *second_part_src_str = &copied_str->data[index + 1];

	memcpy(second_part_dest_str, second_part_src_str, sizeof(char) * (len - index - 1));

	len -= 1;
	data[len] = '\0';

	DELETE_PTR(copied_str);
}

void String::replace(char from, char on)
{
	for (int i = 0; i < len; i++) {
		if (data[i] == from) {
			data[i] = on;
		}
	}
}

void String::append(char c)
{
	char *s = new char[2];
	s[0] = c;
	s[1] = '\0';
	append(s);
	DELETE_PTR(s);
}

void String::append(const char *string)
{
	assert(string != NULL);
	assert(string[0] != '\0');

	if (!data) {
		allocate_and_copy_string(string);
		return;
	}

	int new_len = len + strlen(string);
	char *new_string = new char[new_len + 1];

	memset(new_string, 0, sizeof(char) * new_len + 1);
	strcat(new_string, data);
	strcat(new_string, string);

	DELETE_ARRAY(data);

	data = new_string;
	len = new_len;
}

void String::append(const String &string)
{
	append(string.data);
}

void String::allocate_and_copy_string(const char *string)
{
	assert(string);
	assert(data == NULL);

	int l = strlen(string);
	len = l;
	data = new char[l + 1];
	memcpy(data, string, sizeof(char) * (l + 1));
}

int String::find_text(const char *text, int start)
{
	assert(text);

	int result = -1;
	int l = strlen(text);
	int i = start > 0 ? start : 0;

	for (; i < len; i++) {
		if (data[i] == text[0]) {
			result = i;
			for (int j = ++i, k = 1; k < l && j < len; j++, k++) {
				if (data[j] != text[k]) {
					result = -1;
					break;
				}
			}

			if (result >= 0) {
				return result;
			}
		}
	}
	return result;
}

String *String::copy()
{
	String *str = new String(*this);
	return str;
}
