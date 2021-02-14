#ifndef STRING_H
#define STRING_H

#include "array.h"

void split(char *string, const char *characters, Array<char *> *array);
void make_format_string(const char *string, Array<char> *buffer, Array<char *> *format_elements);

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

//#define FOR(data_struct, item_buffer) for (int _i; _i < data_struct.items ? item_buffer = data_struct[_i]:; _i++)

template <typename... Args>
char *format(Args... args)
{
	Array<char>   buffer;
	Array<char *> args_buffer;
	Array<char *> format_elements;

	//format_(&args_buffer, args...);
	//for (int i = 0; i < args_buffer.items; i++) {
	//	char *string = args_buffer[i];
	//	int result = is_format_string();
	//}
	//char *string = NULL;
	//FOR(args_buffer, string) {

	//}

	//while (!args_buffer.is_empty()) {
	//	char *string = args_buffer.pop();
	//	int result = verify_is_format_string(string);
	//	if (result) {
	//		assert(args_buffer.items >= result);
	//		for (int i = 0; i < result; i++) {
	//			format_elements.push(args_buffer.pop());
	//		}
	//		make_format_string(string, &buffer, &format_elements);
	//	} else {
	//		copy_common_string(string, &buffer);
	//	}
	//}
	//int len = buffer.items - 2;
	//char *new_string = new char[len];
	//memcpy(new_string, buffer.data, len);
	//new_string[len] = '\0';
	//return new_string;
}

#endif