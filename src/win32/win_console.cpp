#include "win_local.h"


#define COPY_ID			1
#define QUIT_ID			2
#define CLEAR_ID		3

#define ERRORBOX_ID		10
#define ERRORTEXT_ID	11

#define EDIT_ID			100
#define INPUT_ID		101

#define	COMMAND_HISTORY	64

static LONG WINAPI ConWndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	char *cmdString;
	static bool s_timePolarity;

	switch (uMsg) {
		case WM_ACTIVATE:
			if (LOWORD(wParam) != WA_INACTIVE) {
				SetFocus(s_wcd.hwndInputLine);
			}
			break;
		case WM_CLOSE:
			if (s_wcd.quitOnClose) {
				PostQuitMessage(0);
			} else {
//				Sys_ShowConsole(0, false);
				//win32.win_viewlog.SetBool(false);
			}
			return 0;
		case WM_CTLCOLORSTATIC:
			if ((HWND)lParam == s_wcd.hwndBuffer) {
				SetBkColor((HDC)wParam, RGB(0x00, 0x00, 0x80));
				SetTextColor((HDC)wParam, RGB(0xff, 0xff, 0x00));
				return (long)s_wcd.hbrEditBackground;
			} else if ((HWND)lParam == s_wcd.hwndErrorBox) {
				if (s_timePolarity & 1) {
					SetBkColor((HDC)wParam, RGB(0x80, 0x80, 0x80));
					SetTextColor((HDC)wParam, RGB(0xff, 0x0, 0x00));
				} else {
					SetBkColor((HDC)wParam, RGB(0x80, 0x80, 0x80));
					SetTextColor((HDC)wParam, RGB(0x00, 0x0, 0x00));
				}
				return (long)s_wcd.hbrErrorBackground;
			}
			break;
		case WM_SYSCOMMAND:
			if (wParam == SC_CLOSE) {
				PostQuitMessage(0);
			}
			break;
		case WM_COMMAND:
			if (wParam == COPY_ID) {
				SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
				SendMessage(s_wcd.hwndBuffer, WM_COPY, 0, 0);
			} else if (wParam == QUIT_ID) {
				if (s_wcd.quitOnClose) {
					PostQuitMessage(0);
				} else {
//					cmdString = Mem_CopyString("quit");
					//Sys_QueEvent(SE_CONSOLE, 0, 0, strlen(cmdString) + 1, cmdString, 0);
				}
			} else if (wParam == CLEAR_ID) {
				SendMessage(s_wcd.hwndBuffer, EM_SETSEL, 0, -1);
				SendMessage(s_wcd.hwndBuffer, EM_REPLACESEL, FALSE, (LPARAM) "");
				UpdateWindow(s_wcd.hwndBuffer);
			}
			break;
		case WM_CREATE:
			s_wcd.hbrEditBackground = CreateSolidBrush(RGB(0x00, 0x00, 0x80));
			s_wcd.hbrErrorBackground = CreateSolidBrush(RGB(0x80, 0x80, 0x80));
			SetTimer(hWnd, 1, 1000, NULL);
			break;
			/*
					case WM_ERASEBKGND:
						HGDIOBJ oldObject;
						HDC hdcScaled;
						hdcScaled = CreateCompatibleDC( ( HDC ) wParam );
						assert( hdcScaled != 0 );
						if ( hdcScaled ) {
							oldObject = SelectObject( ( HDC ) hdcScaled, s_wcd.hbmLogo );
							assert( oldObject != 0 );
							if ( oldObject )
							{
								StretchBlt( ( HDC ) wParam, 0, 0, s_wcd.windowWidth, s_wcd.windowHeight,
									hdcScaled, 0, 0, 512, 384,
									SRCCOPY );
							}
							DeleteDC( hdcScaled );
							hdcScaled = 0;
						}
						return 1;
			*/
		case WM_TIMER:
			if (wParam == 1) {
				s_timePolarity = (bool)!s_timePolarity;
				if (s_wcd.hwndErrorBox) {
					InvalidateRect(s_wcd.hwndErrorBox, NULL, FALSE);
				}
			}
			break;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}


void create_console(const Win32_State *win32)
{
	HDC hDC;
	WNDCLASS wc = { 0 };
	RECT rect;
	const char *DEDCLASS = "CONCOLE";
	int nHeight;
	int swidth, sheight;
	int DEDSTYLE = WS_POPUPWINDOW | WS_CAPTION | WS_MINIMIZEBOX;
	int i;

	memset(&wc, 0, sizeof(wc));

	wc.style = 0;
	wc.lpfnWndProc = (WNDPROC)ConWndProc;
	wc.cbClsExtra = 0;
	wc.cbWndExtra = 0;
	wc.hInstance = win32->hinstance;
	//wc.hIcon = LoadIcon(win32->hinstance, MAKEINTRESOURCE(IDI_ICON1));
	//wc.hCursor = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground = (struct HBRUSH__ *)COLOR_WINDOW;
	wc.lpszMenuName = 0;
	wc.lpszClassName = DEDCLASS;

	if (!RegisterClass(&wc)) {
		return;
	}

	rect.left = 0;
	rect.right = 540;
	rect.top = 0;
	rect.bottom = 450;
	AdjustWindowRect(&rect, DEDSTYLE, FALSE);

	hDC = GetDC(GetDesktopWindow());
	swidth = GetDeviceCaps(hDC, HORZRES);
	sheight = GetDeviceCaps(hDC, VERTRES);
	ReleaseDC(GetDesktopWindow(), hDC);

	s_wcd.windowWidth = rect.right - rect.left + 1;
	s_wcd.windowHeight = rect.bottom - rect.top + 1;

	//s_wcd.hbmLogo = LoadBitmap( win32.hInstance, MAKEINTRESOURCE( IDB_BITMAP_LOGO) );

	s_wcd.hWnd = CreateWindowEx(0,
		DEDCLASS,
		"test",
		DEDSTYLE,
		(swidth - 600) / 2, (sheight - 450) / 2, rect.right - rect.left + 1, rect.bottom - rect.top + 1,
		NULL,
		NULL,
		win32->hinstance,
		NULL);

	if (s_wcd.hWnd == NULL) {
		return;
	}

	//
	// create fonts
	//
	hDC = GetDC(s_wcd.hWnd);
	nHeight = -MulDiv(8, GetDeviceCaps(hDC, LOGPIXELSY), 72);

	s_wcd.hfBufferFont = CreateFont(nHeight, 0, 0, 0, FW_LIGHT, 0, 0, 0, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, DEFAULT_QUALITY, FF_MODERN | FIXED_PITCH, "Courier New");

	ReleaseDC(s_wcd.hWnd, hDC);

	//
	// create the input line
	//
	s_wcd.hwndInputLine = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_BORDER |
		ES_LEFT | ES_AUTOHSCROLL,
		6, 400, 528, 20,
		s_wcd.hWnd,
		(HMENU)INPUT_ID,	// child window ID
		win32->hinstance, NULL);

	//
	// create the buttons
	//
	s_wcd.hwndButtonCopy = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		5, 425, 72, 24,
		s_wcd.hWnd,
		(HMENU)COPY_ID,	// child window ID
		win32->hinstance, NULL);
	SendMessage(s_wcd.hwndButtonCopy, WM_SETTEXT, 0, (LPARAM) "copy");

	s_wcd.hwndButtonClear = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		82, 425, 72, 24,
		s_wcd.hWnd,
		(HMENU)CLEAR_ID,	// child window ID
		win32->hinstance, NULL);
	SendMessage(s_wcd.hwndButtonClear, WM_SETTEXT, 0, (LPARAM) "clear");

	s_wcd.hwndButtonQuit = CreateWindow("button", NULL, BS_PUSHBUTTON | WS_VISIBLE | WS_CHILD | BS_DEFPUSHBUTTON,
		462, 425, 72, 24,
		s_wcd.hWnd,
		(HMENU)QUIT_ID,	// child window ID
		win32->hinstance, NULL);
	SendMessage(s_wcd.hwndButtonQuit, WM_SETTEXT, 0, (LPARAM) "quit");


	//
	// create the scrollbuffer
	//
	s_wcd.hwndBuffer = CreateWindow("edit", NULL, WS_CHILD | WS_VISIBLE | WS_VSCROLL | WS_BORDER |
		ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY,
		6, 40, 526, 354,
		s_wcd.hWnd,
		(HMENU)EDIT_ID,	// child window ID
		win32->hinstance, NULL);
	SendMessage(s_wcd.hwndBuffer, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);

	//s_wcd.SysInputLineWndProc = (WNDPROC)SetWindowLong(s_wcd.hwndInputLine, GWL_WNDPROC, (long)InputLineWndProc);
//	SendMessage(s_wcd.hwndInputLine, WM_SETFONT, (WPARAM)s_wcd.hfBufferFont, 0);

	// don't show it now that we have a splash screen up
//	if (win32.win_viewlog.GetBool()) {
		ShowWindow(s_wcd.hWnd, SW_SHOWDEFAULT);
		UpdateWindow(s_wcd.hWnd);
		SetForegroundWindow(s_wcd.hWnd);
		SetFocus(s_wcd.hwndInputLine);
//	}
	;
	
}