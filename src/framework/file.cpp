#include <stdio.h>
#include <windows.h>

#include "file.h"
#include "../elib/ds/string.h"


char *read_entire_file(const char *name, const char *mode)
{
	FILE *f;
	if (fopen_s(&f, name, mode)) {
		//@Note: may be here need to be warring message
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long len = ftell(f);
	rewind(f);

	char *buffer = new char[len + 1];
	fread(buffer, 1, len, f);
	buffer[len] = '\0';
	fclose(f);
	return buffer;
}

Array<char *> *get_file_names_from_dir(const char *full_path)
{
	full_path = concatenate_c_str(full_path, "*");
	Array<char *> *file_names = NULL;

	WIN32_FIND_DATA data;
	HANDLE handle = FindFirstFile(full_path, &data);

	if (handle == INVALID_HANDLE_VALUE) {
		//@Note: may be here need to be warring message
		return NULL;
	}

	do {
		if (data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
			continue;
		}
		file_names->push(data.cFileName);
	} while (FindNextFile(handle, &data));
	FindClose(handle);

	return file_names;
}