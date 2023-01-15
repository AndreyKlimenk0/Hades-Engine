#ifndef ENUM_HELPER_H
#define ENUM_HELPER_H

#include <ctype.h>

#include "str.h"
#include "ds/hash_table.h"

template <typename Enum_Type>
struct Enum_Helper {
	Hash_Table<u32, String> index_str_table;
	Hash_Table<String, u32> str_index_table;

	void get_string_enums(Array<String> *array);
	String to_string(Enum_Type index);
	Enum_Type from_string(String &enum_str);
};

template<typename Enum_Type>
inline void Enum_Helper<Enum_Type>::get_string_enums(Array<String> *array)
{
	for (u32 i = 0; i < index_str_table.count; i++) {
		array->push(index_str_table.get_value(i));
	}
}

template<typename Enum_Type>
String Enum_Helper<Enum_Type>::to_string(Enum_Type index)
{
	return index_str_table[(u32)index];
}

template<typename Enum_Type>
Enum_Type Enum_Helper<Enum_Type>::from_string(String &enum_str)
{
	return (Enum_Type)str_index_table[enum_str];
}

enum Enum_Format_Type {
	SIMPLE_ENUM_FORMATTING,
	NORMAL_ENUM_FORMATTING,
};

static Enum_Format_Type enum_formatting_type = NORMAL_ENUM_FORMATTING;

inline void set_simple_enum_formatting()
{
	enum_formatting_type = SIMPLE_ENUM_FORMATTING;
}

inline void set_normal_enum_formatting()
{
	enum_formatting_type = NORMAL_ENUM_FORMATTING;
}

template <typename Enum_Type>
Enum_Helper<Enum_Type> *make_enum_helper(const char *string)
{
	String enum_string = string;
	enum_string.removee_all(' ');

	Array<String> enums;
	split(&enum_string, ",", &enums);

	Enum_Helper<Enum_Type> *helper = new Enum_Helper<Enum_Type>();
	if (enum_formatting_type == SIMPLE_ENUM_FORMATTING) {
		for (u32 index = 0; index < enums.count; index++) {
			helper->index_str_table.set(index, enums[index]);
			helper->str_index_table.set(enums[index], index);
		}
	} else if (enum_formatting_type == NORMAL_ENUM_FORMATTING) {
		for (u32 index = 0; index < enums.count; index++) {
			enums[index].replace('_', ' ');
			enums[index].to_lower();
			to_upper_first_letter(&enums[index]);
			
			helper->index_str_table.set(index, enums[index]);
			helper->str_index_table.set(enums[index], index);
		}
	}
	return helper;
}


#define MAKE_ENUM_HELPER(enum_type, ...) (make_enum_helper<enum_type>(#__VA_ARGS__))

#endif