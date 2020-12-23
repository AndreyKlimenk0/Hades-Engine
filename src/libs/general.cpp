#include <windows.h>
#include "general.h"

Vector4 White = { 1.0f, 1.0f, 1.0f, 1.0f };
Vector4 Black = { 0.0f, 0.0f, 0.0f, 1.0f };
Vector4 Red = { 1.0f, 0.0f, 0.0f, 1.0f };
Vector4 Green = { 0.0f, 1.0f, 0.0f, 1.0f };
Vector4 Blue = { 0.0f, 0.0f, 1.0f, 1.0f };
Vector4 Yellow = { 1.0f, 1.0f, 0.0f, 1.0f };
Vector4 Cyan = { 0.0f, 1.0f, 1.0f, 1.0f };
Vector4 Magenta = { 1.0f, 0.0f, 1.0f, 1.0f };
Vector4 Silver = { 0.75f, 0.75f, 0.75f, 1.0f };
Vector4 LightSteelBlue = { 0.69f, 0.77f, 0.87f, 1.0f };


char *get_str_error_message_from_hresult_description(HRESULT hr)
{
	char error_message[512];
	if (FACILITY_WINDOWS == HRESULT_FACILITY(hr)) {
		hr = HRESULT_CODE(hr);
	}

	DWORD result = FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM, NULL, hr, MAKELANGID(LANG_ENGLISH, SUBLANG_ENGLISH_US), error_message, 512, NULL);
	if (!result) {
		printf("[Could not find a description for error # %#x.]\n", hr);
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
	if (IDOK) {
		ExitProcess(1);
	}
	DELETE_PTR(error_message);
}