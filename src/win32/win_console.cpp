#include <string.h>
#include <windows.h>

#include "win_helpers.h"
#include "../libs/structures/array.h"

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

static LRESULT CALLBACK console_winproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
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

static LRESULT CALLBACK console_input_proc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CallWindowProc(win_console.input_edit_proc, hwnd, uMsg, wParam, lParam);;
}

bool create_console(HINSTANCE hinstance)
{
	win_console.text_buffer_background_color = RGB(30, 30, 30);
	win_console.text_buffer_text_color = RGB(255, 255, 255);
	win_console.input_line_background_color = RGB(39, 40, 40);
	win_console.input_line_text_color = RGB(255, 255, 255);

	char class_name[] = "Win32_Console";
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(WNDCLASS));

	wc.lpfnWndProc = (WNDPROC)console_winproc;
	wc.hInstance = hinstance;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = class_name;

	if (!RegisterClass(&wc)) {
		return false;
	}

	win_console.window = CreateWindow(class_name, "Console", WS_OVERLAPPEDWINDOW, 10, 10, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, hinstance, NULL);
	if (!win_console.window) {
		return false;
	}

	int text_buffer_width;
	int text_buffer_height;
	get_window_size(win_console.window, &text_buffer_width, &text_buffer_height);

	win_console.text_buffer = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, 0, 0, text_buffer_width, text_buffer_height - INPUT_LINE_HEIGHT, win_console.window, (HMENU)EDIT_ID, hinstance, NULL);
	if (!win_console.text_buffer) {
		return false;
	}

	int input_line_width;
	int input_line_height;
	get_window_size(win_console.text_buffer, &input_line_width, &input_line_height);

	win_console.input_line_buffer = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | ES_LEFT | ES_AUTOHSCROLL | ES_OEMCONVERT, 0, text_buffer_height - INPUT_LINE_HEIGHT, input_line_width, INPUT_LINE_HEIGHT, win_console.window, (HMENU)INPUT_ID, hinstance, NULL);
	if (!win_console.input_line_buffer) {
		return false;
	}

	ShowWindow(win_console.window, SW_SHOWDEFAULT);
	UpdateWindow(win_console.window);
	SetFocus(win_console.input_line_buffer);

	win_console.input_edit_proc = (WNDPROC)SetWindowLongPtr(win_console.input_line_buffer, GWLP_WNDPROC, (LONG_PTR)console_input_proc);

	HFONT text_buffer_font = create_font(11, "Consolas");
	HFONT intput_line_font = create_font(11, "Consolas");
	SendMessage(win_console.text_buffer, WM_SETFONT, (WPARAM)text_buffer_font, 0);
	SendMessage(win_console.text_buffer, EM_SETLIMITTEXT, 20000000, 0);
	SendMessage(win_console.input_line_buffer, WM_SETFONT, (WPARAM)intput_line_font, 0);
	return true;
}

void append_text_to_console_buffer(const char *text, bool move_to_next_line)
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
	if (move_to_next_line) {
		buffer.push('\r');
		buffer.push('\n');
	}
	buffer.push('\0');

	SendMessage(win_console.text_buffer, EM_LINESCROLL, 0, 0xffff);
	SendMessage(win_console.text_buffer, EM_SCROLLCARET, 0, 0);
	SendMessage(win_console.text_buffer, EM_REPLACESEL, 0, (LPARAM)(char *)buffer.items);
}
