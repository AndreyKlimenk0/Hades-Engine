#ifndef WIN_LOCAL_H
#define WIN_LOCAL_H

#include <windows.h>

struct Win32_State {
	HWND window;
	int window_width;
	int window_height;
	HINSTANCE hinstance;
};

typedef struct {
	HWND		hWnd;
	HWND		hwndBuffer;

	HWND		hwndButtonClear;
	HWND		hwndButtonCopy;
	HWND		hwndButtonQuit;

	HWND		hwndErrorBox;
	HWND		hwndErrorText;

	HBITMAP		hbmLogo;
	HBITMAP		hbmClearBitmap;

	HBRUSH		hbrEditBackground;
	HBRUSH		hbrErrorBackground;

	HFONT		hfBufferFont;
	HFONT		hfButtonFont;

	HWND		hwndInputLine;

	char		errorString[80];

	char		consoleText[512], returnedText[512];
	bool		quitOnClose;
	int			windowWidth, windowHeight;

	WNDPROC		SysInputLineWndProc;
	int			nextHistoryLine;// the last line in the history buffer, not masked
	int			historyLine;	// the line being displayed from history buffer
								// will be <= nextHistoryLine
} WinConData;

static WinConData s_wcd;
void create_console(const Win32_State *win32);
#endif