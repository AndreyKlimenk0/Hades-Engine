#ifndef DICT_H
#define DICT_H

#include "../str.h"
#include "hash_table.h"
#include "../math/vector.h"

struct Args {
	Hash_Table<String, String> storage;

	void set(const char* string, int value);
	void set(const char* string, float value);
	void set(const char* string, Vector3* value);

	void get(const char* string, int* value);
	void get(const char* string, float* value);
	void get(const char* string, Vector3* value);
};

struct Data_Ptr {
	void* ptr = NULL;
	u32 size;  // size in bytes
};

struct Void_Dict {
	Hash_Table<String, Data_Ptr> storage;

	Data_Ptr* get_data_ptr(const char* name);

	void set(const char* string, int value);
	void set(const char* string, float value);
	void set(const char* string, const Vector3& value);
};

#endif