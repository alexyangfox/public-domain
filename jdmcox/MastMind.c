#include <windows.h>
#define LIGHTGRAY 0xE0E0E0
#define ROWS 8
#define COLS 4
#define WIDTH 275
#define HEIGHT 308

int x, y, cxScreen, cyScreen, CurrentX, CurrentY;
DWORD TickCount;
BYTE Six = 6;
char ch;
char Answer[5];
char tempAnswer[5];
char Line[5];
char tempLine[5];
char MastMind[] = " MastMind";
char ABCDEF[] = "A  B  C  D  E  F";
char Locations[] = "0 Locations";
char Letters[] = "0 Letters";
char Blank[] = " ";
char Letter[1];
char Help[] = "\
Enter any of the 6 letters in the boxes.\n\
Get the correct letters in the correct order.";

BOOL first = TRUE;
HWND hwnd, hwndChild[ROWS][COLS], hwndExit, hwndHelp, hwndRetry, hwndNext;
HINSTANCE hInst;
HDC hdc, hdcMem;
HBITMAP hBitmap;
PAINTSTRUCT ps;
RECT rect;
SIZE Size, Size2, Size3, Size4;

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	hInst = hInstance;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)CreateSolidBrush(LIGHTGRAY);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = MastMind;

	if (!RegisterClass (&wndclass))
		return 0;

	hwnd = CreateWindow(MastMind, MastMind,
		WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX,
		0,0,0,0,
		NULL, NULL, hInstance, NULL);

	hwndNext = GetWindow(hwnd, GW_HWNDNEXT);

	ShowWindow (hwnd, iCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}


void GetRandomLetters(void)
{
	TickCount = GetTickCount();
	__asm
	{
  PUSHA
  MOV CL,6
  MOV ESI,OFFSET Answer
  MOV EDX,TickCount
  SUB EAX,EAX
  SUB EBX,EBX
  MOV BX,4	;for four loop
FOUR:
  MOV CH,8	;for random loop
RANDOM: 	;from Marco Ruiz's routine
  MOV AL,DH
  RCR AL,1
  XOR AL,DH
  RCR AL,1
  RCR AL,1
  XOR AL,DH
  RCR AL,1
  XOR AL,DL
  RCR AL,1
  RCR AL,1
  RCR AL,1
  AND AL,1
  ADD DX,DX
  ADD AL,DL
  MOV DL,AL
  DEC CH
  JNZ RANDOM

  AND EAX,0xF
  DIV CL
  ADD AH,0x41

  MOV Answer[EBX-1],AH
  INC SI
  DEC BX
  JNZ FOUR
  POPA
	}
}

void ShowLetter(void)
{
	SetWindowText(hwndChild[CurrentY][CurrentX], Letter);
	BitBlt(hdcMem, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY);
	if (CurrentY == 0)
		return;
	Line[CurrentX] = Letter[0];
	CurrentX++;
	if (CurrentX == 4)
	{
		if (0 == strcmp(Line, Answer))
		{//got it!
			for (x = 0; x < 4; x++)
			{
				hdc = GetDC(hwndChild[0][x]);
				SetBkColor(hdc, LIGHTGRAY);
				SetTextColor(hdc, 0xFF);
				TextOut(hdc, 0, 0, &Answer[x], 1);
				ReleaseDC(hwndChild[0][x],hdc);
			}
			SetFocus(hwnd);
		}
		else
		{
			Locations[0] = '0';
			Letters[0] = '0';
			for (x = 0; x < 4; x++)
				tempAnswer[x] = Answer[x];
			for (x = 0; x < 4; x++)
				tempLine[x] = Line[x];
			for (x = 0; x < 4; x++)
			{
				if (Line[x] == Answer[x])
					Locations[0]++;
				for (y = 0; y < 4; y++)
				{
					if ((tempAnswer[y]) && (tempLine[x]) && (tempLine[x] == tempAnswer[y]))
					{
						Letters[0]++;
						tempAnswer[y] = 0;
						tempLine[x] = 0;
					}
				}
			}
			if (Letters[0] == '1')
				Letters[8] = ' ';
			else
				Letters[8] = 's';
			if (Locations[0] == '1')
				Locations[10] = ' ';
			else
				Locations[10] = 's';
			SetBkColor(hdcMem, LIGHTGRAY);
			TextOut(hdcMem, 20, 24*(CurrentY+1), Letters, 9);
			TextOut(hdcMem, 60+(24*5), 24*(CurrentY+1), Locations, 11);
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);

			CurrentX = 0;
			Line[0] = Line[1] = Line[2] = Line[3] = 0;
			CurrentY--;
			if (CurrentY == 0)
			{
				for (x = 0; x < 4; x++)
				{
					hdc = GetDC(hwndChild[0][x]);
					SetBkColor(hdc, LIGHTGRAY);
					SetTextColor(hdc, 0xFF);
					TextOut(hdc, 0, 0, &Answer[x], 1);
					ReleaseDC(hwndChild[0][x],hdc);
				}
			}
		}
	}
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		MoveWindow(hwnd, (cxScreen/2)-(WIDTH/2), (cyScreen/2)-(HEIGHT/2), WIDTH, HEIGHT, TRUE);
		GetClientRect(hwnd, &rect);
		return 0;

	case WM_CREATE:
		cxScreen = GetSystemMetrics(SM_CXFULLSCREEN);
		cyScreen = GetSystemMetrics(SM_CYFULLSCREEN);
		GetRandomLetters();
		return 0;

	case WM_COMMAND:
		if ((HWND)lParam == hwndRetry)
		{
			for (y = CurrentY; y < 8; y++)
			{
				if (CurrentX == 0)
				{
					CurrentX = 4;//to not skip line again
					continue;
				}
				for (x = 0; x < 4; x++)
					SetWindowText(hwndChild[y][x], " ");
			}
			GetRandomLetters();
			for (x = 0; x < 4; x++)
				SetWindowText(hwndChild[0][x], "?");
			CurrentX = 0;
			CurrentY = 7;
			SetBkColor(hdcMem, LIGHTGRAY);
			for (y = 2; y < 9; y++)
			{
				for (x = 0 ;Size2.cx > x; x += Size4.cx)
					TextOut(hdcMem, 20+x, 24*(y), Blank, 1);
				for (x = 0 ;Size.cx > x; x += Size4.cx)
					TextOut(hdcMem, 60+(24*5)+x, 24*(y), Blank, 1);
			}
			InvalidateRect(hwnd, &rect, FALSE);
			SetFocus(hwnd);
		}
		else if ((HWND)lParam == hwndHelp)
			MessageBox(hwnd, Help, "", MB_OK);
		else if ((HWND)lParam == hwndExit)
			DestroyWindow(hwnd);
		break;

	case WM_SYSKEYDOWN:
		if (wParam == 'E')
			DestroyWindow(hwnd);
		else if (wParam == 'H')
			MessageBox(hwnd, Help, "", MB_OK);
		else if (wParam == 'A')
			SendMessage(hwnd, WM_COMMAND, 0, (LPARAM)hwndRetry);
		return 0;

	case WM_KEYDOWN:
		if (CurrentY == 8)
			x=x;
		switch(wParam)
		{
		case 'A':
			Letter[0] = 'A';
			ShowLetter();
			break;
		case 'B':
			Letter[0] = 'B';
			ShowLetter();
			break;
		case 'C':
			Letter[0] = 'C';
			ShowLetter();
			break;
		case 'D':
			Letter[0] = 'D';
			ShowLetter();
			break;
		case 'E':
			Letter[0] = 'E';
			ShowLetter();
			break;
		case 'F':
			Letter[0] = 'F';
			ShowLetter();
			break;
		case VK_ESCAPE:
			DestroyWindow(hwnd);
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint (hwnd, &ps);
		if (first)
		{
			first = FALSE;
			hdcMem = CreateCompatibleDC(hdc);
			hBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			SelectObject(hdcMem, hBitmap);
			FillRect(hdcMem, &rect, (HBRUSH)CreateSolidBrush(LIGHTGRAY));

			GetTextExtentPoint32(hdc, Locations, 11, &Size);//get Size.cx & Size.cy
			GetTextExtentPoint32(hdc, Letters, 9, &Size2);//get Size2.cx
			GetTextExtentPoint32(hdc, ABCDEF, 16, &Size3);//get Size3.cx
			GetTextExtentPoint32(hdc, Blank, 1, &Size4);
			SetBkColor(hdc, LIGHTGRAY);
			TextOut(hdc, (WIDTH/2)-(Size3.cx/2), (ROWS+1)*24, ABCDEF, 16);

			for (y = 0; y < ROWS; y++)
			{
				for (x = 0; x < COLS; x++)
				{
					hwndChild[y][x] = CreateWindow("STATIC", "",
						WS_CHILD | WS_VISIBLE | WS_DLGFRAME,
						((x+1)*24)+60, (y+1)*24, Size.cy+4, Size.cy+4,
						hwnd, (HMENU)1, hInst, NULL);
				}
			}
			for (x = 0; x < 4; x++)
				SetWindowText(hwndChild[0][x], "?");
			CurrentX = 0;
			CurrentY = 7;

			hwndExit = CreateWindow("BUTTON", "",
				WS_CHILD | WS_VISIBLE | WS_BORDER,
				10,(Size.cy+4)*12, 60, 24,
				hwnd, (HMENU)1, hInst, NULL);
			SetWindowText(hwndExit, "&Exit");
			hwndHelp = CreateWindow("BUTTON", "",
				WS_CHILD | WS_VISIBLE | WS_BORDER,
				WIDTH-175,(Size.cy+4)*12, 60, 24,
				hwnd, (HMENU)1, hInst, NULL);
			SetWindowText(hwndHelp, "&Help");
			hwndRetry = CreateWindow("BUTTON", "",
				WS_CHILD | WS_VISIBLE | WS_BORDER,
				WIDTH-80,(Size.cy+4)*12, 60, 24,
				hwnd, (HMENU)1, hInst, NULL);
			SetWindowText(hwndRetry, "&Again");

			BitBlt(hdcMem, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY);
		}
		else
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		PostQuitMessage (0);
		SetForegroundWindow(hwndNext);
		return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}
