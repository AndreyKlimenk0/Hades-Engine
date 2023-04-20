#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "file.h"
#include "../../sys/sys_local.h"


bool get_file_names_from_dir(const char *full_path, Array<String> *file_names)
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
		file_names->push(String(data.cFileName));
	} while (FindNextFile(handle, &data));
	FindClose(handle);
	return true;
}

bool file_exists(const char *full_path)
{
  DWORD dwAttrib = GetFileAttributes(full_path);

  return (dwAttrib != INVALID_FILE_ATTRIBUTES && !(dwAttrib & FILE_ATTRIBUTE_DIRECTORY));
}

u8 read_u8(FILE *file)
{
	u8 byte;
	fread(&byte, sizeof(u8), 1, file);
	return byte;
}

u16 read_u16(FILE *file)
{
	u16 short_word;
	fread(&short_word, sizeof(u16), 1, file);
	return short_word;
}

u32 read_u32(FILE *file)
{
	u32 word;
	fread(&word, sizeof(u32), 1, file);
	return word;
}

u64 read_u64(FILE *file)
{
	u64 long_word;
	fread(&long_word, sizeof(u64), 1, file);
	return long_word;
}

s8 read_s8(FILE *file)
{
	s8 byte;
	fread(&byte, sizeof(s8), 1, file);
	return byte;
}

s16 read_s16(FILE *file)
{
	s16 short_word;
	fread(&short_word, sizeof(s16), 1, file);
	return short_word;
}

s32 read_s32(FILE *file)
{
	s32 word;
	fread(&word, sizeof(s32), 1, file);
	return word;
}

s64 read_s64(FILE *file)
{
	s64 long_word;
	fread(&long_word, sizeof(s64), 1, file);
	return long_word;
}

float read_real32(FILE *file)
{
	float real32;
	fread(&real32, sizeof(float), 1, file);
	return real32;
}

double read_real64(FILE *file)
{
	double real64;
	fread(&real64, sizeof(double), 1, file);
	return real64;
}

char *read_string(FILE *file, int len)
{
	char *buffer = new char[len + 1];
	fread(buffer, sizeof(u8), len, file);
	buffer[len] = '\0';
	return buffer;
}

char *read_entire_file(const char *name, const char *mode, int *file_size)
{
	FILE *f;
	if (fopen_s(&f, name, mode)) {
		print("Falied of reading file {}", name);
		return NULL;
	}

	fseek(f, 0, SEEK_END);
	long size = ftell(f);
	if (file_size) *file_size = size;
	rewind(f);

	char *buffer = new char[size + 1];
	fread(buffer, sizeof(char), (size_t)size, f);
	buffer[size] = '\0';
	fclose(f);
	return buffer;
}

void extract_file_extension(const char *file_name, String &result)
{
	Array<String> buffer;
	String str = file_name;
	split(&str, ".", &buffer);
	result = buffer.last_item();
}

void extract_file_name(const char *file_name, String &result)
{
	Array<String> buffer;
	String str = file_name;
	split(&str, ".", &buffer);
	result = buffer.items[0];
}

void extract_file_from_path(const char *path, String &result)
{
	Array<String> buffer;
	String str = path;
	split(&str, "\\", &buffer);
	result = buffer.last_item();
}

static DWORD file_mode_to_win32(File_Mode file_mode)
{
	switch (file_mode)
	{
		case FILE_MODE_READ:
			return GENERIC_READ;
		case FILE_MODE_WRITE:
			return GENERIC_WRITE;
	}
	assert(false);
	return 0;
}

static DWORD file_creation_to_win32(File_Creation file_creation)
{
	switch (file_creation)
	{
		case FILE_CREATE_ALWAYS:
			return CREATE_ALWAYS;
		case FILE_CREATE_NEW:
			return CREATE_NEW;
		case FILE_OPEN_ALWAYS:
			return OPEN_ALWAYS;
		case FILE_OPEN_EXISTING:
			return OPEN_EXISTING;
	}
	assert(false);
	return 0;
}

File::~File()
{
	if (is_file_open) {
		CloseHandle(file_handle);
	}
}

bool File::open(const char *path_to_file, File_Mode mode, File_Creation file_creation)
{
	extract_file_from_path(path_to_file, file_name);

	file_handle = CreateFile(path_to_file, file_mode_to_win32(mode), 0, NULL, file_creation_to_win32(file_creation), FILE_ATTRIBUTE_NORMAL, NULL);
	
	if (file_handle == INVALID_HANDLE_VALUE) {
		DWORD error_id = GetLastError();
		char *error_message = get_str_error_message_from_hresult_description(error_id);
		u32 len = (u32)strlen(error_message);
		if (len > 0) {
			error_message[len - 1] = '\0';
			print("[Error] File::open: ", error_message);
			is_file_open = false;
			free_string(error_message);
		}
		return false;
	}
	LARGE_INTEGER size;
	if (!GetFileSizeEx(file_handle, &size)) {
		print("[Error] File::open: Failed to get file size from {}.", file_name);
	}
	file_size = (u32)size.QuadPart;
	is_file_open = true;
	return true;
}

void File::read(void *data, u32 data_size)
{
	DWORD bytes_read = 0;
	DWORD bytes_to_read = data_size;
	
	BOOL result = ReadFile(file_handle, data, bytes_to_read, (LPDWORD)&bytes_read, NULL);

	if (result == FALSE) {
		DWORD error_id = GetLastError();
		char *error_message = get_str_error_message_from_hresult_description(error_id);
		print("File::read failed. Error message: ", error_message);
		free_string(error_message);
		return;
	}

	if (bytes_to_read != bytes_read) {
		print("[Warning] File::read: wrote data in file {} less than must be", file_name);
		return;
	}
}

void File::write(void *data, u32 data_size)
{
	DWORD bytes_written = 0;
	DWORD bytes_to_write = data_size;

	BOOL result = WriteFile(file_handle, data, bytes_to_write, (LPDWORD)&bytes_written, NULL);

	if (result == FALSE) {
		DWORD error_id = GetLastError();
		char *error_message = get_str_error_message_from_hresult_description(error_id);
		print("File::write failed. Error message: ", error_message);
		free_string(error_message);
		return;
	}

	if (bytes_to_write != bytes_written) {
		print("[Warning] File::write: wrote data in file {} less than must be", file_name);
		return;
	}
	return;
}

void File::write(const char *string, bool new_line)
{
	DWORD bytes_written = 0;
	DWORD bytes_to_write = (DWORD)strlen(string);
	
	BOOL result = WriteFile(file_handle, string, bytes_to_write, (LPDWORD)&bytes_written, NULL);

	if (result == FALSE) {
		DWORD error_id = GetLastError();
		char *error_message = get_str_error_message_from_hresult_description(error_id);
		print("File::write failed. Error message: ", error_message);
		free_string(error_message);
		return;
	}

	if (bytes_to_write != bytes_written) {
		print("[Warning] File::write: wrote data in file {} less than must be", file_name);
		return;
	}
}
