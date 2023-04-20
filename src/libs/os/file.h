#ifndef FILE_H
#define FILE_H

#include <stdio.h>
#include <windows.h>

#include "../../win32/win_types.h"
#include "../str.h"
#include "../ds/array.h"


void extract_file_name(const char *file_name, String &result);
void extract_file_extension(const char *file_name, String &result);
void extract_file_from_path(const char *path, String &result);

bool get_file_names_from_dir(const char *full_path, Array<String> *file_names);
bool file_exists(const char *full_path);

u8  read_u8(FILE *file);
u16 read_u16(FILE *file);
u32 read_u32(FILE *file);
u64 read_u64(FILE *file);

s8  read_s8(FILE *file);
s16 read_s16(FILE *file);
s32 read_s32(FILE *file);
s64 read_s64(FILE *file);

float read_real32(FILE *file);
double read_real64(FILE *file);

char *read_string(FILE *file, int len);
char *read_entire_file(const char *name, const char *mode = "r", int *file_size = NULL);


enum File_Mode {
	FILE_MODE_READ,
	FILE_MODE_WRITE,
};

enum File_Creation {
	FILE_CREATE_ALWAYS,
	FILE_CREATE_NEW,
	FILE_OPEN_ALWAYS,
	FILE_OPEN_EXISTING
};

struct File {
	File() {}
	~File();

	bool is_file_open = false;
	u32 file_size = 0;
	HANDLE file_handle = NULL;
	String file_name;

	bool open(const char *path_to_file, File_Mode mode, File_Creation file_creation);
	void read(void *data, u32 data_size);
	void write(void *data, u32 data_size);
	//@Node: Get rid of the method ?
	void write(const char *string, bool new_line = true);

	template< typename T>
	void read(T *data);
	template< typename T>
	void read(Array<T> *array);
	
	template< typename T>
	void write(T *data);
	template< typename T>
	void write(Array<T> *array);
};

template<typename T>
inline void File::read(T * data)
{
	return read((void *)data, sizeof(T));
}

template<typename T>
inline void File::write(T *data)
{
	return write((void *)data, sizeof(T));
}

template<typename T>
inline void File::read(Array<T> *array)
{
	u32 count = 0;
	read(&count);
	if (count > 0) {
		array->reserve(count);
		read((void *)array->items, array->get_size());
	}
}

template<typename T>
inline void File::write(Array<T> *array)
{
	write(&array->count);
	write((void *)array->items, array->get_size());
}

#endif
