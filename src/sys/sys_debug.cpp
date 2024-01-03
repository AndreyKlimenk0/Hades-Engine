#include <stdio.h>
#include <string.h>
#include <windows.h>

#include "sys_local.h"
#include "../libs/ds/hash_table.h"


char *get_error_message_from_error_code(DWORD error_code)
{
	char error_message[512];
	DWORD result = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error_code, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), error_message, 512, NULL);
	if (!result) {
		//printf("[Could not find a description for error # %#x.]\n", hr);
		return NULL;
	}
	size_t len = strlen(error_message);
	if ((len > 1) && (error_message[len - 1] == '\n')) {
		error_message[len - 1] = '\0';
	}
	if ((len > 2) && (error_message[len - 2] == '\r')) {
		error_message[len - 2] = '\0';
	}
	return _strdup(error_message);
}

bool is_string_unique(const char *string)
{
	//@Note: Use set data struct when it will be implemented.
	static Hash_Table<int, u8> unique_string;

	int key = fast_hash(string);
	if (!unique_string.key_in_table(key)) {
		unique_string.set(fast_hash(string), 0);
		return true;
	}
	return false;
}

void report_hresult_error(const char *file, u32 line, HRESULT hr, const char *expr)
{
	char buffer[1024];
	char *error_message = get_error_message_from_error_code(hr);
	sprintf_s(buffer, "File: %s\nLine: %d\nFunction: %s\nMessage: %s", file, line, expr, error_message);
	int button = MessageBox(NULL, buffer, "Error", MB_OKCANCEL | MB_ICONERROR | MB_APPLMODAL);
	
	DELETE_PTR(error_message);
	
	if (IDOK) {
		ExitProcess(1);
	}
}

void report_info(const char *info_message)
{
	int button = MessageBox(NULL, info_message, "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
}

void report_error(const char *error_message)
{
	int button = MessageBox(NULL, error_message, "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
	if (IDOK) {
		ExitProcess(1);
	}
}