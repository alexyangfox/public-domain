#include <windows.h>
#include <wintab.h>//(for a Tablet) put wintab32.lib in Project -Settings -Link
#include "MSGPACK.H"//for Wintab
#define PACKETDATA	(PK_X|PK_Y|PK_BUTTONS|PK_NORMAL_PRESSURE|PK_ORIENTATION)//for Wintab
#define PACKETMODE	PK_BUTTONS//for Wintab
#include <pktdef.h>//for Wintab (must be below PACKETDATA and PACKETMODE defines)
#define BLACK 0
#define WHITE 0xE8E8E8
#define RED 0xFF
#define MAXPOINTS 100000

int x, ptr = 0, deline;
DWORD fileSize, dwBytesRead, dwBytesWritten;
BOOL nofile = TRUE, first = TRUE, wintab, pendown = FALSE, linebegin, deletit = FALSE, fromreturn = FALSE;
char Blackboard[] = "Blackboard";
char Filename[] = "Blackboard.dta";
char Help[] = "Press a function key or the right mouse button for this Help.\n\nEsc to exit Blackboard.\n\nTo delete:\n  Move the Wacom Pen away from the Wacom Tablet.\n  Press Delete to make the last line red.\n  Press Left and Right Arrow keys for a different red line.\n  Press Delete to delete the red line.\n\nTo end deleting:\n  Move the Wacom Pen near the Wacom Tablet\n  or press Esc.\n\nHold the Ctrl key down while pressing the Del key to delete everything.";

struct {
	WORD x;
	WORD y;
} points[MAXPOINTS];

HWND hwnd;
HANDLE hFile;
HDC hdc, hdcMem;
HBITMAP hBitmap;
PAINTSTRUCT ps;
RECT rect;
HBRUSH hBlackBrush, hOldBrush;
HPEN hPen, hOldPen, hDelPen;
LOGCONTEXT lc;
PACKET pkt;
HCTX hTab = NULL;


LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	hBlackBrush = CreateSolidBrush(BLACK);

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = hBlackBrush;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = Blackboard;

	if (!RegisterClass(&wndclass))
		return 0;

	hwnd = CreateWindow(Blackboard, Blackboard,
		WS_EX_TOPMOST | WS_POPUP | WS_VISIBLE,//to use FULL screen (not Windowed)
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	return msg.wParam;
}

void Delete(void)
{ // draw everything and highlight deletable line in red
	if (fromreturn) {
		fromreturn = FALSE;
		hOldBrush = SelectObject(hdcMem, hBlackBrush);
		FillRect(hdcMem, &rect, FALSE);
		SelectObject(hdcMem, hOldBrush);
	}
	hPen = CreatePen(PS_SOLID, 3, WHITE);
	hDelPen = CreatePen(PS_SOLID, 3, RED);
	hOldPen = SelectObject(hdcMem, hPen);
	for (x = 0; x < ptr; ) { // show lines
		if (x == deline)
			SelectObject(hdcMem, hDelPen);
		else
			SelectObject(hdcMem, hPen);
		MoveToEx(hdcMem, points[x].x, points[x].y & 0x7FFF, NULL);
		for (x++; (x < ptr) && (0 == (points[x].y & 0x8000)); x++)
			LineTo(hdcMem, points[x].x, points[x].y);
	}
	SelectObject(hdcMem, hOldPen);
	DeleteObject(hPen);
	DeleteObject(hDelPen);
	hdc = GetDC(hwnd);
	BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
	ReleaseDC(hwnd, hdc);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		wintab = WTInfo(0, 0, NULL);
		if (wintab) {
			WTInfo(WTI_DEFSYSCTX, 0, &lc);
			lc.lcOptions |= CXO_MESSAGES;
			lc.lcPktData = PACKETDATA;
			lc.lcPktMode = PACKETMODE;
			lc.lcMoveMask = PACKETDATA;
			lc.lcBtnUpMask = lc.lcBtnDnMask;
			lc.lcOutOrgX = 0;
			lc.lcOutExtX = GetSystemMetrics(SM_CXSCREEN);
			lc.lcOutOrgY = 0;
			lc.lcOutExtY = -GetSystemMetrics(SM_CYSCREEN);
			hTab = WTOpen(hwnd, &lc, TRUE);
		}
		pkt.pkOrientation.orAltitude = 0;
		pkt.pkX = pkt.pkY = -1;
		hFile = CreateFile(Filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			if (fileSize = GetFileSize(hFile, NULL))
			{
				ReadFile(hFile, points, fileSize, &dwBytesRead, NULL);
				ptr = fileSize / 4;
				nofile = FALSE;
			}
			CloseHandle(hFile);
		}
		return 0;

	case WM_SIZE: // no re-sizing
		rect.left = rect.top = 0;
		rect.right = LOWORD(lParam);
		rect.bottom = HIWORD(lParam);
		if (wintab == 0)
			lc.lcOutExtY = -rect.bottom;
		break;

	case WM_RBUTTONDOWN:
		MessageBox(hwnd, Help, Blackboard, MB_OK);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_F1:
		case VK_F2:
		case VK_F3:
		case VK_F4:
		case VK_F5:
		case VK_F6:
		case VK_F7:
		case VK_F8:
		case VK_F9:
		case VK_F11:
		case VK_F12:
			MessageBox(hwnd, Help, Blackboard, MB_OK);
			break;
		case VK_ESCAPE:
			if (deletit) {
				fromreturn = TRUE; // slightly trick
				deline = ptr; // slightly trick
				Delete();
				deletit = FALSE;
			}
			else
				DestroyWindow(hwnd);
			break;
		case VK_DELETE:
			if (GetKeyState(VK_CONTROL) & 0x80000000) { // Ctrl down
				for (x = 0; x < ptr; x++) {
					points[x].x = 0;
					points[x].y = 0;
				}
				ptr = 0;
				hOldBrush = SelectObject(hdcMem, hBlackBrush);
				FillRect(hdcMem, &rect, FALSE);
				SelectObject(hdcMem, hOldBrush);
				hdc = GetDC(hwnd);
				BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
				ReleaseDC(hwnd, hdc);
			}
			else if (deletit == FALSE) {
				deletit = TRUE;
				for (deline = ptr; (deline >= 0) && (0 == (points[deline].y & 0x8000)); deline--) // decrement deline to previous linebegin flag
					;
				Delete(); // make last (deline) line red
			}
			else { // remove red line
				for (x = deline + 1; (x < ptr) && (0 == (points[x].y & 0x8000)); x++) // increment deline to next linebegin flag
					;
				for ( ; x < ptr; deline++, x++)
					points[deline] = points[x];
				for (x = deline ; x < ptr; x++) {
					points[x].x = 0;
					points[x].y = 0;
				}
				ptr = deline;
				for (deline-- ; (deline >= 0) && (0 == (points[deline].y & 0x8000)); deline--) // decrement deline to previous linebegin flag
					;
				fromreturn = TRUE;
				Delete();
			}
			break;
		case VK_LEFT:
			if (deletit) {
				if (deline == 0)
					return 0;
				for (deline--; (deline >= 0) && (0 == (points[deline].y & 0x8000)); deline--) // decrement deline to previous linebegin flag
					;
				Delete();
			}
			break;
		case VK_RIGHT:
			if (deletit) {
				if (deline == ptr)
					return 0;
				for (x = deline + 1; (x < ptr) && (0 == (points[x].y & 0x8000)); x++) // increment deline to next linebegin flag
					;
				if (x != ptr) {
					deline = x;
					Delete();
				}
			}
			break;
		}
		return 0;

	case WT_PACKET:
		if (WTPacket((HCTX)lParam, wParam, &pkt)) {
			if (deletit) {
				fromreturn = TRUE; // slightly trick
				deline = ptr; // slightly trick
				Delete();
				deletit = FALSE;
			}
			if ((HIWORD(pkt.pkButtons) == TBN_DOWN) && (LOWORD(pkt.pkButtons) == 0)) {
				if (pendown == FALSE)
					linebegin = TRUE;
				pendown = TRUE;
			}
			else if ((HIWORD(pkt.pkButtons) == TBN_UP) && (LOWORD(pkt.pkButtons) == 0)) // raised pen off tablet
				pendown = FALSE;
			InvalidateRect(hwnd, &rect, FALSE);
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (first) {
			first = FALSE;
			hdcMem = CreateCompatibleDC(hdc);
			hBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			SelectObject(hdcMem, hBitmap);
			hPen = CreatePen(PS_SOLID, 2, WHITE);
			hOldPen = SelectObject(hdcMem, hPen);
			for (x = 0; x < ptr; ) { // show lines
				MoveToEx(hdcMem, points[x].x, points[x].y & 0x7FFF, NULL);
				for (x++; (x < ptr) && (0 == (points[x].y & 0x8000)); x++)
					LineTo(hdcMem, points[x].x, points[x].y);
			}
			SelectObject(hdcMem, hOldPen);
			DeleteObject(hPen);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
			if (nofile) {
				nofile = FALSE;
				MessageBox(hwnd, Help, Blackboard, MB_OK);
			}
		}
		else if ((pendown) && (ptr < MAXPOINTS)) {
			points[ptr].x = (WORD)pkt.pkX;
			points[ptr].y = (WORD)pkt.pkY;
			if (points[ptr].y == 0)
				points[ptr].y = 1;
			if (points[ptr].x == 0)
				points[ptr].x = 1;
			if (linebegin) {
				linebegin = FALSE;
				points[ptr].y |= 0x8000; // flag for delete
			}
			else if (ptr) {
				hPen = CreatePen(PS_SOLID, 2,	WHITE);
				hOldPen = SelectObject(hdcMem, hPen);
				MoveToEx(hdcMem, points[ptr-1].x, (points[ptr-1].y & 0x7FFF), NULL);
				LineTo(hdcMem, points[ptr].x, (points[ptr].y & 0x7FFF));
				SelectObject(hdcMem, hOldPen);
				DeleteObject(hPen);
			}
			ptr++;
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
		}
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		DeleteDC(hdcMem);
		DeleteObject(hBitmap);
		if (pkt.pkX != -1) { // if any Wacom Tablet input
			hFile = CreateFile(Filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
			WriteFile(hFile, points, ptr*4, &dwBytesWritten, NULL);
			CloseHandle(hFile);
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
