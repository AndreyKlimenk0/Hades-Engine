#ifndef FILE_H
#define FILE_H

#include <windows.h>
#include "../elib/ds/array.h"


Array<char *> *get_files_name_from_dir(const char *full_path)
{
	WIN32_FIND_DATA data;
	Array<char *> *file_names = NULL;
	HANDLE handle = FindFirstFile(full_path, &data);

	if (handle == INVALID_HANDLE_VALUE) {
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
#endif