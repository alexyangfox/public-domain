#include <windows.h>
#include <mmsystem.h> // add winmm.lib to Project -Settings -Link
#include "resource.h"

int x, y, cxScreen, cyScreen;
int AlarmTime, CountDown;
char Time[2];
char MinutesSeconds[] = " 0:00 ";
char szAppName[] = "Timer";
char ComicSansMS[] = "Comic Sans MS";
HINSTANCE hInst;
HDC hdc;
PAINTSTRUCT ps;
HFONT hFont, hOldFont;
LOGFONT lf;
SIZE Size;

LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int CALLBACK TimerProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	HWND hwnd;
	MSG	msg;
	WNDCLASS wndclass;
	hInst = hInstance;

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_MENU+1);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	RegisterClass (&wndclass);

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

 	ShowWindow (hwnd, SW_SHOW);
	UpdateWindow (hwnd);

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		lf.lfHeight = -64;
		lf.lfWeight = 400;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 3;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x42;
		for (x = 0; ComicSansMS[x] != 0; x++)
			lf.lfFaceName[x] = ComicSansMS[x];
		lf.lfFaceName[x] = 0;
		hFont = CreateFontIndirect(&lf);
		hdc = GetDC(hwnd);
		GetTextExtentPoint32(hdc, MinutesSeconds, 6, &Size);
		ReleaseDC(hwnd, hdc);
		cxScreen = GetSystemMetrics(SM_CXFULLSCREEN);
		cyScreen = GetSystemMetrics(SM_CYFULLSCREEN);
		x = (cxScreen / 2) - (Size.cx / 2);
		y = (cyScreen / 2) - (Size.cy / 2);
		MoveWindow(hwnd, x, y, 188, 120, TRUE);

		if (DialogBox(hInst, "TIMER", NULL, TimerProc)) {
			if (AlarmTime/10)
				MinutesSeconds[0] = (AlarmTime/10) + '0';
			MinutesSeconds[1] = (AlarmTime%10) + '0';
			CountDown = AlarmTime*60000;
			SetTimer(hwnd, 0, 1000, 0);
		}
		else
			DestroyWindow(hwnd);
		return 0;

	case WM_TIMER:
		CountDown -= 1000;
		if (CountDown) {
			MinutesSeconds[4]--;
			if (MinutesSeconds[4] == '/') { // 1 less than '0';
				MinutesSeconds[4] = '9';
				MinutesSeconds[3]--;
				if (MinutesSeconds[3] == '/') {
					MinutesSeconds[3] = '5';
					MinutesSeconds[1]--;
					if (MinutesSeconds[1] == '/') { // 1 less than '0';
						MinutesSeconds[1] = '9';
						if (MinutesSeconds[0] != ' ')
							MinutesSeconds[0]--;
						if (MinutesSeconds[0] == '0')
							MinutesSeconds[0] = ' ';
					}
				}
			}
			SetWindowText(hwnd, MinutesSeconds);
			hdc = GetDC(hwnd);
			hOldFont = SelectObject(hdc, hFont);
			TextOut(hdc, 0, 0, MinutesSeconds, 6);
			SelectObject(hdc, hOldFont);
			ReleaseDC(hwnd, hdc);
		}
		else {
			KillTimer(hwnd, 0);
			PlaySound("timer.wav", NULL, SND_FILENAME);
			DestroyWindow(hwnd);
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		hOldFont = SelectObject(hdc, hFont);
		TextOut(hdc, 50, 0, Time, strlen(Time));
		SelectObject(hdc, hOldFont);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int CALLBACK TimerProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x;
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			x = GetWindowText(hwndEdit, Time, 4);
			if ((x < 1) || (x > 2))
			{
				SetFocus(hwndEdit);
				break;
			}
			AlarmTime = atoi(Time);
			if (AlarmTime == 0)
				break;
			PlaySound("timer.wav", NULL, SND_FILENAME);
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}
