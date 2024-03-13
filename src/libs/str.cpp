#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>

#include "str.h"
#include "math/vector.h"
#include "math/matrix.h"
#include "../sys/sys_local.h"


#define MAX_DIGITS_IN_INT 12
#define MAX_DIGITS_IN_LONG_LONG 21

#define bytes_of(type, count) (sizeof(type) * count)

#define COPY_STRING_TO_CHAR_ARRAY(string, char_array) {\
	u32 s_len = (u32)strlen(string); \
	while ((char_array->size - char_array->count) < s_len) { \
		char_array->resize(char_array->size * 2);\
	} \
	char *symbol = &char_array->get(char_array->count); \
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
	u32 curr_pos = 0;
	u32 prev_pos = 0;
	u32	len = (u32)strlen(symbols);

	while ((curr_pos = string->find_text(symbols, curr_pos)) != -1) {
		if (prev_pos != curr_pos) {
			array->push(String(string->data, prev_pos, curr_pos));
		}
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

void to_upper_first_letter(String *string)
{
	for (u32 i = 0; i < string->len; i++) {
		char c = string->data[i];
		if (isalpha((int)c)) {
			if (i > 0) {
				char prev = string->data[i - 1];
				if (prev == ' ') {
					string->data[i] = toupper((int)c);
				}
			} else {
				string->data[i] = toupper((int)c);
			}
		}
	}
}

bool is_alphabet(const char *string)
{
	u32 len = (u32)strlen(string);

	for (u32 i = 0; i < len; i++) {
		if (!isalpha(string[i])) {
			return false;
		}
	}
	return true;
}

static void format_string(const char *format_string, Array<char> *formatting_string, Array<char *> *vars)
{
	assert(format_string);
	assert(formatting_string);
	assert(vars);

	u32 var_index = 0;
	const char *f_string = format_string;

	while (*f_string) {
		if (*f_string == '}') {
			f_string++;
			continue;
		}
		if (*f_string == '{') {
			if (!strcmp(vars->items[var_index], "")) {
				var_index++;
				f_string++;
				continue;
			}
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

	for (u32 i = 0; i < strings->count; i++) {
		char *string = strings->get(i);
		int result = is_format_string(string);
		if (result) {
			assert(strings->count >= (u32)result);
			int var_index = i + 1;
			for (int j = 0; j < result; j++, i++) {
				vars_buffer.push(strings->get(var_index++));
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
	u32 str1_len = (s32)strlen(str1);
	u32 str2_len = (s32)strlen(str2);
	u32 len = str1_len + str2_len;
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

#define FLOAT_PRECISION(n) #n

char *to_string(float number, u32 precision)
{
	char *str = new char[64];
	memset(str, 0, 64);
	char *format = ::format("%.{}f", precision);
	_sprintf_p(str, 64, format, number);
	free_string(format);
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

char *to_string(Vector2 *vector)
{
	return format("vec2({}, {})", vector->x, vector->y);
}

char *to_string(Vector3 *vector)
{
	return format("vec3({}, {}, {})", vector->x, vector->y, vector->z);
}

char *to_string(Vector4 *vector)
{
	return format("vec4({}, {}, {}, {})", vector->x, vector->y, vector->z, vector->w);
}

char *to_string(Matrix4 *matrix)
{
	return format("Matrix4(\n\t{}, {}, {}, {},\n\t{}, {}, {}, {},\n\t{}, {}, {}, {}\n\t{}, {}, {}, {})",
		matrix->_11, matrix->_12, matrix->_13, matrix->_14,
		matrix->_21, matrix->_22, matrix->_23, matrix->_24,
		matrix->_31, matrix->_32, matrix->_33, matrix->_34,
		matrix->_41, matrix->_42, matrix->_43, matrix->_44);
}

char *to_string(Rect_u32 *rect)
{
	return format("Rect_u32({}, {}, {}, {})", rect->x, rect->y, rect->width, rect->height);
}

char *to_string(Rect_s32 *rect)
{
	return format("Rect_s32({}, {}, {}, {})", rect->x, rect->y, rect->width, rect->height);
}

char *to_string(Rect_f32 * rect)
{
	return format("Rect_f32({}, {}, {}, {})", rect->x, rect->y, rect->width, rect->height);
}

char *to_string(Point_s32 *point)
{
	return format("Point_s32({}, {})", point->x, point->y);
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

String::String(char _char)
{
	len = 1;
	data = new char[2];
	data[0] = _char;
	data[1] = '\0';
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

	if (string == data) {
		return;
	}

	allocate_and_copy_string(string);
}

String::String(const char *string, u32 start, u32 end)
{
	copy(string, (u32)start, (u32)end);
}

String::String(const String &string, u32 start, u32 end)
{
	copy(string, (u32)start, (u32)end);
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

	if (string == data) {
		return *this;
	}

	DELETE_ARRAY(data);
	allocate_and_copy_string(string);
	return *this;
}

String &String::operator=(const String &other)
{
	if ((other.data == NULL) && (other.len == 0)) {
		free();
		return *this;
	}

	if (this == &other) {
		return *this;
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

	for (u32 i = 0; i < len; i++) {
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

	*this = *other_str;
	DELETE_PTR(other_str);
}

void String::insert(u32 index, char c)
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

void String::remove(u32 index)
{
	assert(len > index);

	if (len == 1) {
		len = 0;
		data = NULL;
		return;
	}

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

void String::removee_all(char c)
{
	for (u32 i = 0; i < len; i++) {
		char character = data[i];
		if (character == c) {
			remove(i);
		}
	}
}

void String::replace(char from, char on)
{
	for (u32 i = 0; i < len; i++) {
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

	u32 new_len = len + (u32)strlen(string);
	char *new_string = new char[new_len + 1];

	memset(new_string, 0, sizeof(char) * new_len + 1);
	strcat_s(new_string, new_len + 1, data);
	strcat_s(new_string, new_len + 1, string);

	DELETE_ARRAY(data);

	data = new_string;
	len = new_len;
}

void String::append(const String &string)
{
	append(string.data);
}

void String::allocate(u32 char_count)
{
	DELETE_ARRAY(data);
	data = new char[char_count];
	len = char_count;
}

void String::allocate_and_copy_string(const char *string)
{
	assert(string);
	assert(data == NULL);

	int string_len = (u32)strlen(string);
	len = string_len;
	data = new char[string_len + 1];
	memcpy(data, string, sizeof(char) * (string_len + 1));
}

void String::place_end_char()
{
	if ((len > 0) && (data != NULL)) {
		data[len - 1] = '\0';
	}
}

u32 String::find_text(const char *text, u32 start)
{
	assert(text);

	u32 result = -1;
	u32 l = (u32)strlen(text);
	u32 i = start > 0 ? start : 0;

	for (; i < len; i++) {
		if (data[i] == text[0]) {
			result = i;
			for (u32 j = ++i, k = 1; k < l && j < len; j++, k++) {
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

void String::copy(const String & string, u32 start, u32 end)
{
	u32 string_len = end - start;
	if (string_len > 0) {
		DELETE_ARRAY(data);
		len = string_len;
		const char *ptr = string.data;
		ptr += start;

		data = new char[string_len + 1];
		memcpy(data, ptr, sizeof(char) * string_len);
		data[string_len] = '\0';
	}
}

bool String::is_empty() 
{
	if ((data == NULL) && (len == 0)) {
		return true;
	} else if ((data && (data[0] == '\0')) && (len == 0)) {
		return true;
	}
	return false;
}

String *String::copy()
{
	String *str = new String(*this);
	return str;
}
