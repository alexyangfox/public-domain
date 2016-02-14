#include <windows.h>
#include "resource.h"
#define FTTICKSPERHOUR (60 * 60 * (LONGLONG) 10000000)

int AlarmTime, TimeNow;
char CurrentTime[10];
char Time[10];
char szAppName[] = "Alarm Clock2";

HINSTANCE hInst;
SYSTEMTIME st;
LARGE_INTEGER li;
FILETIME ft;

LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);
int CALLBACK AlarmProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);

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
		WS_POPUP | WS_CAPTION | WS_SYSMENU,
		0, 0, 0, 0,
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
		GetLocalTime(&st);
		SystemTimeToFileTime(&st, &ft);
		li = * (LARGE_INTEGER *) &ft;
		ft = * (FILETIME *) &li;
		FileTimeToSystemTime(&ft, &st);
		st.wSecond = 0;
		st.wMilliseconds = 0;
		TimeNow = (st.wHour * 100) + st.wMinute;
		CurrentTime[0] = (TimeNow / 1000) + '0';
		CurrentTime[1] = ((TimeNow % 1000) / 100) + '0';
		CurrentTime[2] = ((TimeNow % 100) / 10) + '0';
		CurrentTime[3] = (TimeNow % 10) + '0';

		if (!DialogBox(hInst, "ALARMCLOCK", NULL, AlarmProc))
			DestroyWindow(hwnd);
		else
		{
			SetWindowText(hwnd, Time);
			st.wHour = AlarmTime / 100;
			st.wMinute = AlarmTime % 100;
			SystemTimeToFileTime (&st, &ft);
			li = *(LARGE_INTEGER*)&ft;
			GetLocalTime(&st);
			SystemTimeToFileTime(&st, &ft);
			li.QuadPart -= ((LARGE_INTEGER *) &ft)->QuadPart;
			while (li.QuadPart < 0)
				 li.QuadPart += 24 * FTTICKSPERHOUR;
			li.QuadPart %= 24 * FTTICKSPERHOUR;
			SetTimer(hwnd, 0, (int)(li.QuadPart / 10000), 0);
		}
		return 0;

	case WM_TIMER:
		KillTimer(hwnd, 0);
		PlaySound("The Microsoft Sound.wav", NULL, SND_FILENAME);
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

int CALLBACK AlarmProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetWindowText(hwndEdit, CurrentTime);
		SendMessage(hwndEdit, EM_SETSEL, 0, -1);
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (GetWindowText(hwndEdit, Time, 9) == 0)
			{
				SetFocus(hwndEdit);
				break;
			}
			AlarmTime = atoi(Time);
			if (AlarmTime > 2359)
			{
				SetFocus(hwndEdit);
				break;
			}
			PlaySound("The Microsoft Sound.wav", NULL, SND_FILENAME);
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}
