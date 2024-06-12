#ifndef STRING_H
#define STRING_H

#include "math/vector.h"
#include "math/matrix.h"
#include "math/structures.h"
#include "structures/array.h"

typedef u32 String_Id;

struct String {
	String() {}
	~String();

	char *data = NULL;
	u32 len = 0;

	explicit String(char _char);
	explicit String(int number);
	explicit String(float number);
	String(const char *string);
	String(const String *other);
	String(const String &other);
	String(const char *string, u32 start, u32 end);
	String(const String &string, u32 start, u32 end);

	operator const char *();
	operator const char *() const;

	String &operator=(const char *string);
	String &operator=(const String &other);

	void free();
	void print();
	void to_lower();
	void pop_char();
	void insert(u32 index, char c);
	void remove(u32 index);
	void removee_all(char c);
	void replace(char from, char on);
	void append(char c);
	void append(const char *string);
	void append(const String &string);
	void append(const String *string);
	void allocate(u32 char_count);
	void allocate_and_copy_string(const char *string);
	void place_end_char();
	void copy(const String &string, u32 start, u32 end);

	bool is_empty();
	s32 find(const char *substring, u32 start_index = 0, bool case_sensitive = true);

	const char *c_str() { return data; }
	String *copy();
};

inline String operator+(const String &first, const String &second);
inline String operator+(const char *first, const String &second);
inline String operator+(const String &first, const char *second);
inline bool operator==(const String &first, const String &second);
inline bool operator==(const char *first, const String &second);
inline bool operator==(const String &first, const char *second);
inline bool operator!=(const String &first, const String &second);
inline bool operator!=(const char *first, const String &second);
inline bool operator!=(const String &first, const char *second);
inline bool operator>(const String &first, const String &second);
inline bool operator<(const String &first, const String &second);
inline bool operator>=(const String &first, const String &second);
inline bool operator<=(const String &first, const String &second);

void free_string(const char *string);
void format_(Array<char *> *array);
void split(const char *string, const char *characters, Array<char *> *array);
void to_upper_first_letter(String *string);

bool is_alphabet(const char *string);
bool split(String *string, const char *characters, Array<String> *array);


char *get_next_line(char **buffer);
char *concatenate_c_str(const char *str1, const char *str2);

char *to_string(int number, int base = 10);
char *to_string(unsigned int number, int base = 10);
char *to_string(long number, int base = 10);
char *to_string(unsigned long number, int base = 10);
char *to_string(s64 number, int base = 10);
char *to_string(u64 number, int base = 10);
char *to_string(float number);
char *to_string(float number, u32 precision);
char *to_string(double num);
char *to_string(bool val);
char *to_string(const char *string);
char *to_string(char c);
char *to_string(String &string);
char *to_string(String *string);
char *to_string(Vector2 *vector);
char *to_string(Vector3 *vector);
char *to_string(Vector4 *vector);
char *to_string(Matrix4 *matrix);
char *to_string(Rect_u32 *rect);
char *to_string(Rect_s32 *rect);
char *to_string(Rect_f32 *rect);
char *to_string(Point_s32 *point);
char *to_string(Matrix4 *matrix);

int is_format_string(const char *string);

template <typename First, typename... Args>
void format_(Array<char *> *array, First first, Args... args)
{
	char *temp = to_string(first);
	array->push(temp);
	format_(array, args...);
}

char *__do_formatting(Array<char *> *strings);

template <typename... Args>
char *format(Args... args)
{
	Array<char *> strings;
	format_(&strings, args...);
	return __do_formatting(&strings);
}

inline String::operator const char *()
{
	return data;
}

inline String::operator const char *() const
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

