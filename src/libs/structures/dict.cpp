#include <stdlib.h>
#include "dict.h"

void Args::set(const char *string, int value)
{
	storage.set(string, to_string(value));
}

void Args::set(const char *string, float value)
{
	storage.set(string, to_string(value));
}

void Args::set(const char *string, Vector3 *value)
{
	storage.set(string, to_string(value));
}

void Args::get(const char *string, int *value)
{
	String str = storage[string];
	*value = atoi(str);
}

void Args::get(const char *string, float *value)
{
	String str = storage[string];
	*value = (float)atof(str);
}

void Args::get(const char *string, Vector3 *value)
{
	String str = storage[string];

	Array<String> buffer;
	split(&str, " ", &buffer);

	value->x = (float)atof(buffer[0]);
	value->y = (float)atof(buffer[1]);
	value->z = (float)atof(buffer[2]);
}

#define MAKE_DATA_PTR(string, type, value) { \
	Data_Ptr data_ptr; \
	data_ptr.size = sizeof(type); \
	data_ptr.ptr = malloc(data_ptr.size); \
	memcpy(data_ptr.ptr, (void *)&value, data_ptr.size); \
	storage.set(string, data_ptr); } \


Data_Ptr *Void_Dict::get_data_ptr(const char *name)
{
	return nullptr;
}

void Void_Dict::set(const char *string, int value)
{
	MAKE_DATA_PTR(string, int, value);
}

void Void_Dict::set(const char *string, float value)
{
	MAKE_DATA_PTR(string, float, value);
}

void Void_Dict::set(const char *string, const Vector3 &value)
{
	MAKE_DATA_PTR(string, Vector3, value);
}
