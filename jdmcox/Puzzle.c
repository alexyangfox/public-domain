#include <windows.h>
#define Size 60

int xBlank = 0, yBlank = 0;
int x1 = 60, y1 = 0;
int x2 = 120, y2 = 0;
int x3 = 180, y3 = 0;
int x4 = 0, y4 = 60;
int x5 = 60, y5 = 60;
int x6 = 120, y6 = 60;
int x7 = 180, y7 = 60;
int x8 = 0, y8 = 120;
int x9 = 60, y9 = 120;
int xA = 120, yA = 120;
int xB = 180, yB = 120;
int xC = 0, yC = 180;
int xD = 60, yD = 180;
int xE = 120, yE = 180;
int xF = 180, yF = 180;
int x, y, prevy, cxScreen, cyScreen, Frame, Title, Width, Height;
int Scramble[] = {VK_DOWN, VK_UP, VK_RIGHT, VK_LEFT};
char szAppName[] = "Arrow Keys Puzzle";
BOOL first = TRUE;
HWND hwnd, hwndButton1, hwndButton2, hwndButton3, hwndButton4, hwndButton5, hwndButton6, hwndButton7, hwndButton8, hwndButton9, hwndButtonA, hwndButtonB, hwndButtonC, hwndButtonD, hwndButtonE, hwndButtonF;
HINSTANCE hInstance;
RECT rect;
HDC hdcButton;
HFONT hFont;
LOGFONT lf;
char Arial[] = "Arial";
SYSTEMTIME st;
FILETIME ft;
ULARGE_INTEGER ul;

BOOL CheckDown(HWND*, int*, int*);
BOOL CheckUp(HWND *Hwnd, int *xPtr, int *yPtr);
BOOL CheckLeft(HWND *Hwnd, int*, int*);
BOOL CheckRight(HWND *Hwnd, int *xPtr, int *yPtr);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
		0,0,0,0,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_SIZE:
		GetClientRect(hwnd, &rect);
		MoveWindow(hwnd, (cxScreen/2)-(Width/2), (cyScreen/2)-(Height/2), Width, Height, TRUE);
		return 0;

	case WM_CREATE:
		lf.lfHeight = -37;
		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 1;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x31;
		for (x = 0; Arial[x] != 0; x++)
			lf.lfFaceName[x] = Arial[x];
		lf.lfFaceName[x] = 0;

		cxScreen = GetSystemMetrics(SM_CXFULLSCREEN);
		cyScreen = GetSystemMetrics(SM_CYFULLSCREEN);
		Frame = GetSystemMetrics(SM_CXSIZEFRAME);
		Title = GetSystemMetrics(SM_CYCAPTION);
		Width = (Size*4)+Frame;
		Height = (Size*4)+Title+Frame;

		hdcButton = GetDC(hwndButton1);
		hFont = CreateFontIndirect (&lf);
		SelectObject(hdcButton, hFont);
		ReleaseDC(hwndButton1, hdcButton);

		hwndButton1 = CreateWindow("BUTTON", "1",
			WS_CHILD | WS_VISIBLE,
			x1, y1, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton1, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButton2 = CreateWindow("BUTTON", "2",
			WS_CHILD | WS_VISIBLE,
			x2, y2, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton2, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButton3 = CreateWindow("BUTTON", "3",
			WS_CHILD | WS_VISIBLE,
			x3, y3, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton3, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButton4 = CreateWindow("BUTTON", "4",
			WS_CHILD | WS_VISIBLE,
			x4, y4, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton4, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButton5 = CreateWindow("BUTTON", "5",
			WS_CHILD | WS_VISIBLE,
			x5, y5, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton5, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButton6 = CreateWindow("BUTTON", "6",
			WS_CHILD | WS_VISIBLE,
			x6, y6, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton6, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButton7 = CreateWindow("BUTTON", "7",
			WS_CHILD | WS_VISIBLE,
			x7, y7, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton7, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButton8 = CreateWindow("BUTTON", "8",
			WS_CHILD | WS_VISIBLE,
			x8, y8, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton8, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButton9 = CreateWindow("BUTTON", "9",
			WS_CHILD | WS_VISIBLE,
			x9, y9, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButton9, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButtonA = CreateWindow("BUTTON", "A",
			WS_CHILD | WS_VISIBLE,
			xA, yA, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButtonA, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButtonB = CreateWindow("BUTTON", "B",
			WS_CHILD | WS_VISIBLE,
			xB, yB, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButtonB, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButtonC = CreateWindow("BUTTON", "C",
			WS_CHILD | WS_VISIBLE,
			xC, yC, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButtonC, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButtonD = CreateWindow("BUTTON", "D",
			WS_CHILD | WS_VISIBLE,
			xD, yD, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButtonD, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButtonE = CreateWindow("BUTTON", "E",
			WS_CHILD | WS_VISIBLE,
			xE, yE, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButtonE, WM_SETFONT, (UINT)hFont, TRUE);

		hwndButtonF = CreateWindow("BUTTON", "F",
			WS_CHILD | WS_VISIBLE,
			xF, yF, Size, Size,
			hwnd, (HMENU)1, hInstance, NULL);
		SendMessage(hwndButtonF, WM_SETFONT, (UINT)hFont, TRUE);
		return 0;

	case WM_KEYDOWN:
		if (first)
		{
			first = FALSE;
			GetSystemTime(&st);
			SystemTimeToFileTime(&st, &ft);
			ul.LowPart = ft.dwLowDateTime;
			ul.HighPart = ft.dwHighDateTime;
			ul.QuadPart /= 10000;//because low 4 digits are 0's
			srand(ul.LowPart);
			srand((UINT)rand());
			for (x = 0; x < 256; )
			{
				y = rand()%4;
				if (((y == 0) && (prevy != 1)) || ((y == 2) && (prevy != 3)) || ((y == 1) && (prevy != 0)) || ((y == 3) && (prevy != 2)))
				{//don't just move it back
					SendMessage(hwnd, WM_KEYDOWN, Scramble[y], 0);
					x++;
				}
				prevy = y;
			}
			break;
		}
		switch (wParam)
		{
		case VK_ESCAPE:
			DestroyWindow(hwnd);
			break;
		case VK_DOWN:
			if (CheckDown(&hwndButton1, &x1, &y1))
				break;
			if (CheckDown(&hwndButton2, &x2, &y2))
				break;
			if (CheckDown(&hwndButton3, &x3, &y3))
				break;
			if (CheckDown(&hwndButton4, &x4, &y4))
				break;
			if (CheckDown(&hwndButton5, &x5, &y5))
				break;
			if (CheckDown(&hwndButton6, &x6, &y6))
				break;
			if (CheckDown(&hwndButton7, &x7, &y7))
				break;
			if (CheckDown(&hwndButton8, &x8, &y8))
				break;
			if (CheckDown(&hwndButton9, &x9, &y9))
				break;
			if (CheckDown(&hwndButtonA, &xA, &yA))
				break;
			if (CheckDown(&hwndButtonB, &xB, &yB))
				break;
			if (CheckDown(&hwndButtonC, &xC, &yC))
				break;
			if (CheckDown(&hwndButtonD, &xD, &yD))
				break;
			if (CheckDown(&hwndButtonE, &xE, &yE))
				break;
			if (CheckDown(&hwndButtonF, &xF, &yF))
				break;
			break;
		case VK_UP:
			if (CheckUp(&hwndButton1, &x1, &y1))
				break;
			if (CheckUp(&hwndButton2, &x2, &y2))
				break;
			if (CheckUp(&hwndButton3, &x3, &y3))
				break;
			if (CheckUp(&hwndButton4, &x4, &y4))
				break;
			if (CheckUp(&hwndButton5, &x5, &y5))
				break;
			if (CheckUp(&hwndButton6, &x6, &y6))
				break;
			if (CheckUp(&hwndButton7, &x7, &y7))
				break;
			if (CheckUp(&hwndButton8, &x8, &y8))
				break;
			if (CheckUp(&hwndButton9, &x9, &y9))
				break;
			if (CheckUp(&hwndButtonA, &xA, &yA))
				break;
			if (CheckUp(&hwndButtonB, &xB, &yB))
				break;
			if (CheckUp(&hwndButtonC, &xC, &yC))
				break;
			if (CheckUp(&hwndButtonD, &xD, &yD))
				break;
			if (CheckUp(&hwndButtonE, &xE, &yE))
				break;
			if (CheckUp(&hwndButtonF, &xF, &yF))
				break;
			break;
		case VK_LEFT:
			if (CheckLeft(&hwndButton1, &x1, &y1))
				break;
			if (CheckLeft(&hwndButton2, &x2, &y2))
				break;
			if (CheckLeft(&hwndButton3, &x3, &y3))
				break;
			if (CheckLeft(&hwndButton4, &x4, &y4))
				break;
			if (CheckLeft(&hwndButton5, &x5, &y5))
				break;
			if (CheckLeft(&hwndButton6, &x6, &y6))
				break;
			if (CheckLeft(&hwndButton7, &x7, &y7))
				break;
			if (CheckLeft(&hwndButton8, &x8, &y8))
				break;
			if (CheckLeft(&hwndButton9, &x9, &y9))
				break;
			if (CheckLeft(&hwndButtonA, &xA, &yA))
				break;
			if (CheckLeft(&hwndButtonB, &xB, &yB))
				break;
			if (CheckLeft(&hwndButtonC, &xC, &yC))
				break;
			if (CheckLeft(&hwndButtonD, &xD, &yD))
				break;
			if (CheckLeft(&hwndButtonE, &xE, &yE))
				break;
			if (CheckLeft(&hwndButtonF, &xF, &yF))
				break;
			break;
		case VK_RIGHT:
			if (CheckRight(&hwndButton1, &x1, &y1))
				break;
			if (CheckRight(&hwndButton2, &x2, &y2))
				break;
			if (CheckRight(&hwndButton3, &x3, &y3))
				break;
			if (CheckRight(&hwndButton4, &x4, &y4))
				break;
			if (CheckRight(&hwndButton5, &x5, &y5))
				break;
			if (CheckRight(&hwndButton6, &x6, &y6))
				break;
			if (CheckRight(&hwndButton7, &x7, &y7))
				break;
			if (CheckRight(&hwndButton8, &x8, &y8))
				break;
			if (CheckRight(&hwndButton9, &x9, &y9))
				break;
			if (CheckRight(&hwndButtonA, &xA, &yA))
				break;
			if (CheckRight(&hwndButtonB, &xB, &yB))
				break;
			if (CheckRight(&hwndButtonC, &xC, &yC))
				break;
			if (CheckRight(&hwndButtonD, &xD, &yD))
				break;
			if (CheckRight(&hwndButtonE, &xE, &yE))
				break;
			if (CheckRight(&hwndButtonF, &xF, &yF))
				break;
			break;
		}
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

BOOL CheckDown(HWND *Hwnd, int *xPtr, int *yPtr)
{
	if ((*yPtr == yBlank - Size) && (*xPtr == xBlank))
	{
		*yPtr += Size;
		yBlank -= Size;
		MoveWindow(*Hwnd, *xPtr, *yPtr, Size, Size, TRUE);
		InvalidateRect(hwnd, &rect, FALSE);
		return TRUE;
	}
	return FALSE;
}

BOOL CheckUp(HWND *Hwnd, int *xPtr, int *yPtr)
{
	if ((*yPtr == yBlank + Size) && (*xPtr == xBlank))
	{
		*yPtr -= Size;
		yBlank += Size;
		MoveWindow(*Hwnd, *xPtr, *yPtr, Size, Size, TRUE);
		InvalidateRect(hwnd, &rect, FALSE);
		return TRUE;
	}
	return FALSE;
}

BOOL CheckLeft(HWND *Hwnd, int *xPtr, int *yPtr)
{
	if ((*xPtr == xBlank + Size) && (*yPtr == yBlank))
	{ 
		*xPtr -= Size;
		xBlank += Size;
		MoveWindow(*Hwnd, *xPtr, *yPtr, Size, Size, TRUE);
		InvalidateRect(hwnd, &rect, FALSE);
		return TRUE;
	}
	return FALSE;
}
BOOL CheckRight(HWND *Hwnd, int *xPtr, int *yPtr)
{
	if ((*xPtr == xBlank - Size) && (*yPtr == yBlank))
	{
		*xPtr += Size;
		xBlank -= Size;
		MoveWindow(*Hwnd, *xPtr, *yPtr, Size, Size, TRUE);
		InvalidateRect(hwnd, &rect, FALSE);
		return TRUE;
	}
	return FALSE;
}
