#include <stdio.h>
#include <windows.h>

#include "file.h"
#include "../libs/ds/string.h"

char *read_entire_file(const char *name, const char *mode, int *file_size)
{
	FILE *f;
	if (fopen_s(&f, name, mode)) {
		//@Note: may be here need to be warring message
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	if (file_size) *file_size = size;
	rewind(f);

	char *buffer = new char[size + 1];
	int result = fread(buffer, sizeof(char), size, f);
	buffer[size] = '\0';
	fclose(f);
	return buffer;
}

bool get_file_names_from_dir(const char *full_path, Array<char *> *file_names)
{
	full_path = concatenate_c_str(full_path, "*");

	WIN32_FIND_DATA data;
	HANDLE handle = FindFirstFile(full_path, &data);

	if (handle == INVALID_HANDLE_VALUE) {
		//@Note: may be here need to be warring message
		return false;
	}

	do {
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}
		file_names->push(_strdup(data.cFileName));
	} while (FindNextFile(handle, &data));
	FindClose(handle);
	return true;
}