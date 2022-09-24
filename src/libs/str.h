#ifndef STRING_H
#define STRING_H

#include "ds/array.h"
#include "../libs/math/common.h"
#include "math/vector.h"
#include "../win32/win_local.h"
#include "../win32/win_types.h"


struct String;

void format_(Array<char *> *array);
void split(const char *string, const char *characters, Array<char *> *array);
bool split(String *string, const char *characters, Array<String> *array);
bool is_alphabet(const char *string);

inline void free_string(const char *string)
{
	delete[] string;
	string = NULL;
}


char *get_next_line(char **buffer);
char *concatenate_c_str(const char *str1, const char *str2);

char *to_string(int number, int base = 10);
char *to_string(unsigned int number, int base = 10);
char *to_string(long number, int base = 10);
char *to_string(unsigned long number, int base = 10);
char *to_string(s64 number, int base = 10);
char *to_string(u64 number, int base = 10);
char *to_string(float number);
char *to_string(double num);
char *to_string(bool val);
char *to_string(const char *string);
char *to_string(char c);
char *to_string(String &string);
char *to_string(String *string);
char *to_string(Vector2 *vector);
char *to_string(Vector3 *vector);
char *to_string(Rect_u32 *rect);
char *to_string(Rect_s32 *rect);

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

struct String {
	String() {}
	~String();

	char *data = NULL;
	int len = 0;

	explicit String(char _char);
	explicit String(int number);
	explicit String(float number);
	String(const char *string);
	String(const String *other);
	String(const String &other);
	String(const char *string, int start, int end);
	String(const String &string, int start, int end);

	operator const char*();
	operator const char*() const;

	String &operator=(const char *string);
	String &operator=(const String &other);

	void free();
	void print();
	void to_lower();
	void pop_char();
	void insert(int index, char c);
	void remove(int index);
	void replace(char from, char on);
	void append(char c);
	void append(const char *string);
	void append(const String &string);
	void append(const String *string);
	void allocate_and_copy_string(const char *string);

	bool is_empty() { return data == NULL && len == 0; }
	
	int find_text(const char *text, int start = 0);

	const char *to_str() { return data; }
	
	String *copy();
};

inline String::operator const char*()
{
	return data;
}

inline String::operator const char*() const
{
	return data;
}

inline String operator+(const String &first, const String &second)
{
	String str = first;
	str.append(second.data);
	return str;
}

inline String operator+(const char *first, const String &second)
{
	String str = first;
	str.append(second.data);
	return str;
}

inline String operator+(const String &first, const char *second)
{
	String str = first;
	str.append(second);
	return str;
}

inline bool operator==(const String &first, const String &second)
{
	return !(strcmp(first.data, second.data));
}

inline bool operator==(const char *first, const String &second)
{
	return !(strcmp(first, second.data));
}

inline bool operator==(const String &first, const char *second)
{
	return !(strcmp(first.data, second));
}

inline bool operator!=(const String &first, const String &second)
{
	return strcmp(first.data, second.data);
}

inline bool operator!=(const char *first, const String &second)
{
	return strcmp(first, second.data);
}

inline bool operator!=(const String &first, const char *second)
{
	return strcmp(first.data, second);
}

inline bool operator>(const String &first, const String &second)
{
	return strlen(first.data) > strlen(second.data);
}

inline bool operator<(const String &first, const String &second)
{
	return strlen(first.data) < strlen(second.data);
}

inline bool operator>=(const String &first, const String &second)
{
	return strlen(first.data) > strlen(second.data);
}

inline bool operator<=(const String &first, const String &second)
{
	return strlen(first.data) > strlen(second.data);
}

inline void String::append(const String *string)
{
	append(string->data);
}
#endif

