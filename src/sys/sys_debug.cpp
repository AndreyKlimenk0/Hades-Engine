#include <stdio.h>
#include <windows.h>

#include "sys_local.h"


char *get_str_error_message_from_hresult_description(HRESULT hr)
{
	char error_message[512];
	if (FACILITY_WINDOWS == HRESULT_FACILITY(hr)) {
		hr = HRESULT_CODE(hr);
	}

	DWORD result = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), error_message, 512, NULL);
	if (!result) {
		//printf("[Could not find a description for error # %#x.]\n", hr);
		return NULL;
	}
	return _strdup(error_message);
}

void report_hresult_error(const char *file, u32 line, HRESULT hr, const char *expr)
{
	char buffer[1024];
	char *error_message = get_str_error_message_from_hresult_description(hr);
	sprintf(buffer, "File: %s\nLine: %d\nFunction: %s\nMessage: %s\n", file, line, expr, error_message);
	int button = MessageBox(NULL, buffer, "Error", MB_OKCANCEL | MB_ICONERROR | MB_APPLMODAL);
	
	DELETE_PTR(error_message);
	
	if (IDOK) {
		ExitProcess(1);
	}
}

void report_error(const char *error_message)
{
	int button = MessageBox(NULL, error_message, "Error", MB_OK | MB_ICONERROR | MB_APPLMODAL);
	DELETE_PTR(error_message);

	if (IDOK) {
		ExitProcess(1);
	}
}