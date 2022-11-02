#include <string.h>

#include "win_local.h"
#include "../libs/ds/array.h"
#include <Windows.h>

#define EDIT_ID  100
#define INPUT_ID 101
#define INPUT_LINE_HEIGHT 23


struct Win_Console {
	HWND window;
	HWND text_buffer;
	HWND input_line_buffer;

	COLORREF text_buffer_background_color;
	COLORREF text_buffer_text_color;
	COLORREF input_line_background_color;
	COLORREF input_line_text_color;
	COLORREF text_color;

	WNDPROC input_edit_proc;
};

Win_Console win_console;

HBRUSH hbrBkgnd = NULL;
HBRUSH hbrBkgnd2 = NULL;

static HFONT create_font(int size, const char *font_name)
{
	HDC hDC = GetDC(win_console.window);
	int nHeight = -MulDiv(size, GetDeviceCaps(hDC, LOGPIXELSY), 72);

	HFONT font = CreateFont(nHeight, 0, 0, 0, FW_LIGHT, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN | FIXED_PITCH, font_name);

	ReleaseDC(win_console.window, hDC);
	return font;
}

void get_window_size(HWND window, int *width, int *height)
{
	RECT rect;
	GetClientRect(window, &rect);
	*width = rect.right - rect.left;
	*height = rect.bottom - rect.top;
}

LRESULT CALLBACK console_winproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
		case WM_CTLCOLORSTATIC: {
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, win_console.text_buffer_text_color);
			SetBkColor(hdcStatic, win_console.text_buffer_background_color);

			if (hbrBkgnd == NULL) {
				hbrBkgnd = CreateSolidBrush(win_console.text_buffer_background_color);
			}
			return (INT_PTR)hbrBkgnd;
		}
		case WM_CTLCOLOREDIT: {
			HDC hdcStatic = (HDC)wParam;
			SetTextColor(hdcStatic, win_console.input_line_text_color);
			SetBkColor(hdcStatic, win_console.input_line_background_color);

			if (hbrBkgnd2 == NULL) {
				hbrBkgnd2 = CreateSolidBrush(win_console.input_line_background_color);
			}
			return (INT_PTR)hbrBkgnd2;
		}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

LRESULT CALLBACK console_input_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char temp[300];
	switch (uMsg) {
		//case WM_CHAR: {
		//	switch (wParam) {
		//		case VK_BACK: {
		//			GetWindowText(win_console.input_line_buffer, temp, 300);
		//			char *last_char = &temp[strlen(temp)];
		//			if (*last_char == ':') {
		//				return 1;
		//			} else {
		//				last_char = '\0';
		//				SetWindowText(win_console.input_line_buffer, "");
		//			}
		//		}
		//	}
		//	break;
		//}
	}
	return CallWindowProc(win_console.input_edit_proc, hwnd, uMsg, wParam, lParam);;
}

bool create_console(Win32_Info *win32_state)
{
	win_console.text_buffer_background_color = RGB(30,  30,  30);
	win_console.text_buffer_text_color       = RGB(255, 255, 255);
	win_console.input_line_background_color  = RGB(39,  40,  40);
	win_console.input_line_text_color        = RGB(255, 255, 255);

	char class_name[] = "Win32_Console";
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(WNDCLASS));

	wc.lpfnWndProc = (WNDPROC)console_winproc;
	wc.hInstance = win32_state->hinstance;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = class_name;

	if (!RegisterClass(&wc)) {
		return false;
	}

	win_console.window = CreateWindow(class_name, "Console", WS_OVERLAPPEDWINDOW, 10, 10, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, win32_state->hinstance, NULL);
	if (!win_console.window) {
		return false;
	}

	int text_buffer_width;
	int text_buffer_height;
	get_window_size(win_console.window, &text_buffer_width, &text_buffer_height);
	
	win_console.text_buffer = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL  | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 0, 0, text_buffer_width, text_buffer_height - INPUT_LINE_HEIGHT, win_console.window, (HMENU)EDIT_ID, win32_state->hinstance, NULL);
	if (!win_console.text_buffer) {
		return false;
	}

	int input_line_width;
	int input_line_height;
	get_window_size(win_console.text_buffer, &input_line_width, &input_line_height);

	win_console.input_line_buffer = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_OEMCONVERT, 0, text_buffer_height - INPUT_LINE_HEIGHT, input_line_width, INPUT_LINE_HEIGHT, win_console.window, (HMENU)INPUT_ID, win32_state->hinstance, NULL);
	if (!win_console.input_line_buffer) {
		return false;
	}

	ShowWindow(win_console.window, SW_SHOWDEFAULT);
	UpdateWindow(win_console.window);
	SetFocus(win_console.input_line_buffer);
	
	win_console.input_edit_proc = (WNDPROC)SetWindowLong(win_console.input_line_buffer, GWL_WNDPROC, (long)console_input_proc);

	HFONT text_buffer_font = create_font(11, "Consolas");
	HFONT intput_line_font = create_font(11, "Consolas");
	SendMessage(win_console.text_buffer, WM_SETFONT, (WPARAM)text_buffer_font, 0);
	SendMessage(win_console.input_line_buffer, WM_SETFONT, (WPARAM)intput_line_font, 0);
	return true;
}

void append_text_to_text_buffer(const char *text)
{
	Array<char> buffer;
	const char *t = text;
	while (*t) {
		if (*t == '\n' && *(t - 1) != '\r') {
			buffer.push('\r');
			buffer.push('\n');
		} else {
			buffer.push(*t);
		}
		t++;
	}
	buffer.push('\r');
	buffer.push('\n');
	buffer.push('\0');
	
	SendMessage(win_console.text_buffer, EM_LINESCROLL, 0, 0xffff);
	SendMessage(win_console.text_buffer, EM_SCROLLCARET, 0, 0);
	SendMessage(win_console.text_buffer, EM_REPLACESEL, 0, (LPARAM)(char *)buffer.items);
}
