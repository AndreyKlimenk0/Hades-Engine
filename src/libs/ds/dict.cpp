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
	*value = atof(str);
}

void Args::get(const char *string, Vector3 *value)
{
	String str = storage[string];
	
	Array<String> buffer;
	split(&str, " ", &buffer);
	
	value->x = atof(buffer[0]);
	value->y = atof(buffer[1]);
	value->z = atof(buffer[2]);
}

