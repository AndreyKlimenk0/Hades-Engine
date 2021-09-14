#ifndef DICT_H
#define DICT_H

#include "../str.h"
#include "../math/vector.h"
#include "hash_table.h"

struct Args {
	Hash_Table<String, String> storage;

	void set(const char *string, int value);
	void set(const char *string, float value);
	void set(const char *string, Vector3 *value);

	void get(const char *string, int *value);
	void get(const char *string, float *value);
	void get(const char *string, Vector3 *value);
};

#endif