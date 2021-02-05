#include "win_local.h"

Win_Console win_console;

#define EDIT_ID 100

//void 
//void print();

LRESULT CALLBACK console_winproc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	HBRUSH hbrBkgnd = NULL;
	switch (uMsg) {
	case WM_CTLCOLORSTATIC:
	{
		HDC hdcStatic = (HDC)wParam;
		SetTextColor(hdcStatic, RGB(255, 255, 255));
		SetBkColor(hdcStatic, RGB(0, 0, 0));

		if (hbrBkgnd == NULL) {
			hbrBkgnd = CreateSolidBrush(RGB(0, 0, 0));
		}
		return (INT_PTR)hbrBkgnd;
	}
	//case WM_CTLCOLORSTATIC: {
	//	SetTextColor((HDC)wParam, RGB(0xa, 0xff, 0xff));
	//	SetBkColor((HDC)wParam, RGB(0x00, 0x00, 0x00));
	//	return (long)wconsole.background_color;
	//}
	case WM_CREATE:
	{
		win_console.background_color = CreateSolidBrush(RGB(0x00, 0x00, 0x00));
		break;
	}
	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

void create_console()
{
	char class_name[] = "Win32_Console";
	WNDCLASS wc;
	ZeroMemory(&wc, sizeof(WNDCLASS));

	wc.lpfnWndProc = (WNDPROC)console_winproc;
	wc.hInstance = win32.hinstance;
	wc.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	wc.lpszClassName = class_name;

	if (!RegisterClass(&wc))
		return;

	win_console.window = CreateWindow(class_name, "Console", WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, NULL, NULL, win32.hinstance, NULL);
	if (!win_console.window)
		return;

	RECT rect;
	GetClientRect(win_console.window, &rect);
	int width = rect.right - rect.left;
	int height = rect.bottom - rect.top;
	HWND b = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
		ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY, CW_USEDEFAULT, CW_USEDEFAULT, width, height, win_console.window, (HMENU)100, win32.hinstance, NULL);


	ShowWindow(win_console.window, SW_SHOWDEFAULT);
	UpdateWindow(win_console.window);



	const char *buffer = "append this!\r\n asldfjlasdfjl;";
	HWND hEdit = GetDlgItem(win_console.window, EDIT_ID);
	int index = GetWindowTextLength(hEdit);
	SetFocus(hEdit); // set focus
	//SendMessageA(hEdit, EM_SETSEL, (WPARAM)index, (LPARAM)index); // set selection - end of text
	//SendMessageA(hEdit, EM_REPLACESEL, 0, (LPARAM)buffer);
	SendMessageA(hEdit, EM_SCROLLCARET, 0, 0);
}

void append_text_to_edit_buffer(const char *text)
{

}