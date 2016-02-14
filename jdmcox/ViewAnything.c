#include <windows.h>
#include "resource.h"

char Help[] = "Version 1.8\n\
Doug Cox\n\
jdmcox@jdmcox.com\n\
http://jdmcox.com\n\
\n\
Use the Arrow Keys,\n\
Page Up, Page Down,\n\
Home, End,\n\
Ctrl-Home, Ctrl-End,\n\
and Esc\n\
\n\
F - find\n\
F3 - find again from cursor\n\
C - clear find highlights\n\
Decimal numbers must be preceeded by a \\\n\
A decimal number can only be a byte (0-255)\n\
A hexadecimal number can be a byte, word, or integer\n\
Hexadecimal numbers must have this format: \\xA2\n\
\n\
J - jump to a hexadecimal address\n\
\n\
E - enter text or numbers\n\
Ctrl-S - save changed file\n\
\n\
D - display location and contents in unsigned decimal\n\
S - display contents in Signed Decimal\n\
U - display contents in Unsigned Decimal\n\
X - display location and contents in hexadecimal\n\
\n\
B - display contents as a byte\n\
W - display contents as a word (2 bytes)\n\
I - display contents as an integer (4 bytes)\n\
\n\
Put a Shortcut to ViewAnything in\n\
C:\\Documents and Settings\\Administrator\\SendTo\n\
so you can right-click on a file and see it in ViewAnything.\n\
";

char fileName[MAX_PATH];
char DirBuf[MAX_PATH];
char TopLine[] = " Press F1 for Help        Cursor Location: 00000     Contents: 00               ";
                //012345678901234567890123456789012345678901234567890123456789012345678901234567890
char Zero[] = "0       ";
int curRow = 0, vScrollPos = 0, bytes = 1;
int cxChar, cyChar, cxScreen, cyScreen, charsPerLine, linesPerScreen, totalLines;
DWORD w, x, y, z, row, col, X, Y, FindBufSize = 0, TempBufSize;
DWORD fileSize, Width, Height, curCol = 0;
unsigned char FindBuf[50];
unsigned char TempBuf[50];
char titleName[MAX_PATH];
char CmdLine;
unsigned char *fileBuf, *fileBuf2, *fb;
DWORD FirstLetterLoc;
DWORD dwBytesRead, dwBytesWritten;
BOOL sendtoflg = TRUE, highlighted, itshex = TRUE, find = FALSE, changed = FALSE, itsigned = FALSE;
BOOL altdown = FALSE;
RECT rect ;
OPENFILENAME ofn ;
LOGFONT lf ;
HINSTANCE hInst ;
HANDLE hFile ;
HWND hwndButton, hwndFind ;
HWND hwnd ;
HDC hdc ;
PAINTSTRUCT ps ;
RECT rect2, rect3 ;
COLORREF Color;

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
int CALLBACK HilightProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK JumpProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK FindProc(HWND, UINT, WPARAM, LPARAM);
void ShowLocContents(void);
int GetNumbers(void);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = TEXT ("ViewAnything");
	MSG          msg ;
	WNDCLASS     wndclass ;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject (LTGRAY_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass (&wndclass))
		return 0;

	if (szCmdLine[0] == 0)
		sendtoflg = FALSE;
	else
	{
		y = 0;
		CmdLine = '\x0';
		if (szCmdLine[0] == '"')
		{
			CmdLine = '"';
			y = 1;
		}
		for (x = 0; szCmdLine[y] != CmdLine ; x++, y++)
			fileName[x] = szCmdLine[y];
		fileName[x] = 0;
	}

	hwnd = CreateWindow (szAppName, szAppName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX | WS_VSCROLL,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	hInst = hInstance;

	ShowWindow (hwnd, iCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
switch (message)
	{
	case WM_CREATE:
		ofn.lStructSize       = sizeof (OPENFILENAME) ;
		ofn.hwndOwner         = hwnd ;
		ofn.hInstance         = NULL ;
		ofn.lpstrFilter       = NULL ;
		ofn.lpstrCustomFilter = NULL ;
		ofn.nMaxCustFilter    = 0 ;
		ofn.nFilterIndex      = 0 ;
		ofn.lpstrFile         = fileName;// Set in Open and Close functions
		ofn.nMaxFile          = MAX_PATH ;
		ofn.lpstrFileTitle    = NULL ;// Set in Open and Close functions
		ofn.nMaxFileTitle     = MAX_PATH ;
		ofn.lpstrInitialDir   = NULL ;
		ofn.lpstrTitle        = NULL ;
		ofn.Flags             = 0 ;// Set in Open and Close functions
		ofn.nFileOffset       = 0 ;
		ofn.nFileExtension    = 0 ;
		ofn.lpstrDefExt       = NULL ;
		ofn.lCustData         = 0L ;
		ofn.lpfnHook          = NULL ;
		ofn.lpTemplateName    = NULL ;
		ofn.hwndOwner         = hwnd ;
		ofn.lpstrFileTitle    = titleName ;
		ofn.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT ;

		if (sendtoflg == FALSE)
		{
			if (0 == GetOpenFileName (&ofn))
			{
				SendMessage(hwnd, WM_CLOSE, 0, 0) ;
				return 0;
			}
		}
		cxChar = 10;
		cyChar = 20;
		cxScreen = GetSystemMetrics(SM_CXSCREEN);
		cyScreen = GetSystemMetrics(SM_CYSCREEN);
		X = GetSystemMetrics(SM_CXVSCROLL) + 2*(GetSystemMetrics(SM_CXFIXEDFRAME));
		Y = GetSystemMetrics(SM_CYMENU) +  cyChar + 2*(GetSystemMetrics(SM_CYFIXEDFRAME));
		y = GetSystemMetrics(SM_CYMENU);
		y = GetSystemMetrics(SM_CYFIXEDFRAME);
		Width = (cxScreen - X) / cxChar;
		Height = (cyScreen - Y) / cyChar;
		if (Width > 80)
			Width = 80;
		if (Height > 25)
			Height = 25;

		lf.lfCharSet = 255 ;
		lf.lfClipPrecision = 2 ;
		lf.lfEscapement = 0 ;
		lf.lfHeight = -(cyChar) ;
		lf.lfItalic = 0 ;
		lf.lfOrientation = 0 ;
		lf.lfOutPrecision = 1 ;
		lf.lfPitchAndFamily = '1' ;
		lf.lfQuality = 1 ;
		lf.lfStrikeOut = 0 ;
		lf.lfUnderline = 0 ;
		lf.lfWeight = 400 ;
		lf.lfWidth = 0 ;
		strcpy (lf.lfFaceName, "Terminal") ;
		hdc = GetDC(hwnd);
		SelectObject(hdc, CreateFontIndirect(&lf)) ;
		ReleaseDC(hwnd, hdc) ;

		if ((hFile = CreateFile(fileName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
		{
			if (fileSize = GetFileSize(hFile, NULL))
			{
//fileSize = 500000;
				if (fileSize < (Width * Height))
					x = Width * Height;
				else
					x = fileSize + Width;
				fileBuf = (unsigned char*)VirtualAlloc(NULL, x, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
				if (fileBuf == NULL)
				{
					MessageBox(hwnd, "Couldn't reserve enough memory!", NULL, MB_OK);
					SendMessage(hwnd, WM_CLOSE, 0, 0);
					return 0;
				}
				fb = fileBuf;
//SetFilePointer(hFile, 0x1F4EEEDB, NULL, FILE_BEGIN);
				ReadFile(hFile, fileBuf, fileSize, &dwBytesRead, NULL);
				SetWindowText(hwnd, fileName);
				ShowLocContents();
			}
			else
			{
				MessageBox(hwnd, "File is empty", "Huh!", MB_OK);
				SendMessage(hwnd, WM_CLOSE, 0, 0);
			}
			CloseHandle(hFile);
		}
		else
		{
			MessageBox(hwnd, "File not found", "Oops", MB_OK) ;
			SendMessage(hwnd, WM_CLOSE, 0, 0) ;
		}
		return 0;

	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			MoveWindow(hwnd, (cxScreen/2)-((cxChar*Width)+X)/2 , (cyScreen/2)-((cyChar*Height)+Y)/2, (cxChar*Width)+X, (cyChar*Height)+Y+8, TRUE);
			GetClientRect(hwnd, &rect);
			rect2.top = rect.top ;
			rect2.left = rect.left ;
			rect2.right = rect.right ;
			rect2.bottom = cyChar ;
			rect3.top = rect.top ;
			rect3.left = 10*cyChar ;
			rect3.right = rect.right ;
			rect3.bottom = cyChar ;
			hdc = GetDC(hwnd);
			FillRect(hdc, &rect2, (HBRUSH)(COLOR_MENU+1)) ;
			ReleaseDC(hwnd, hdc);
			charsPerLine = Width;
			linesPerScreen = Height;
			totalLines = fileSize / charsPerLine ;
			if (fileSize % charsPerLine)
				totalLines++;
			SetScrollRange(hwnd, SB_VERT, 0, totalLines - (linesPerScreen-1), FALSE);
			SetScrollPos(hwnd, SB_VERT, vScrollPos, TRUE);
		}
		return 0 ;

	case WM_VSCROLL:
		switch (LOWORD (wParam))
		{
		case SB_TOP:
			vScrollPos = 0;
			break;
		case SB_BOTTOM:
			vScrollPos = totalLines - linesPerScreen;
			break;
		case SB_LINEUP:
			vScrollPos -= 1;
			break;
		case SB_LINEDOWN:
			vScrollPos += 1;
			break;
		case SB_PAGEUP:
			vScrollPos -= linesPerScreen;
			break;
		case SB_PAGEDOWN:
			vScrollPos += linesPerScreen;
			break;
		case SB_THUMBPOSITION:
			vScrollPos = HIWORD (wParam);
			break;
		case 0xFFFF: // from 'J'
			vScrollPos = *(DWORD*)FindBuf / charsPerLine;
			curRow = 0;
			curCol = *(DWORD*)FindBuf % charsPerLine;
			break;
		}
		vScrollPos = max (0, min (vScrollPos, totalLines - (linesPerScreen)));
		if (vScrollPos != GetScrollPos(hwnd, SB_VERT))
			SetScrollPos(hwnd, SB_VERT, vScrollPos, TRUE);
		ShowLocContents();
		return 0;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case 'J':
			DialogBox(hInst, "HILIGHT", hwnd, JumpProc);
			SendMessage(hwnd, WM_VSCROLL, -1, 0);
			break;
		case 'F':
			DialogBox(hInst, "FIND", NULL, FindProc);
			break;
		case VK_F3:
			FirstLetterLoc = curCol + (curRow+vScrollPos)*charsPerLine + 1;
			for (x = FirstLetterLoc; x < fileSize; x++)
			{
//				ch = fb[x];
//				if ((ch >= 'a') && (ch <= 'z'))
//					ch &= 0xDF;
				if (FindBuf[0] == fb[x])
				{
					for (y = 1, z = x+1; y < FindBufSize; z++, y++)
					{
//						ch = fb[z];
//						if ((ch >= 'a') && (ch <= 'z'))
//							ch &= 0xDF;
						if (FindBuf[y] != fb[z])
							break;
					}
					if (y == FindBufSize)
					{
						FirstLetterLoc = z - y;
						curCol = FirstLetterLoc % charsPerLine;
						if (curCol == -1)
							curCol = charsPerLine - 1;
						if (FirstLetterLoc > (DWORD)(charsPerLine*linesPerScreen))
						{
							curRow = (linesPerScreen/2);
							vScrollPos = (FirstLetterLoc/charsPerLine) - ((linesPerScreen/2) - 1);
						}
						else
						{
							curRow = FirstLetterLoc / charsPerLine;
							vScrollPos = 0;
						}
						SendMessage(hwnd, WM_VSCROLL, 0, 0);
						break;//found it
					}
				}
			}
			break;
		case 'E':
//			if (GetKeyState(VK_CONTROL) < 0)//Ctrl pressed
				DialogBox(hInst, "HILIGHT", hwnd, HilightProc);
			break;
		case 'S':
			if (GetKeyState(VK_CONTROL) < 0)//Ctrl pressed
			{
				if (changed)
				{
					changed = FALSE;
					if (IDOK == MessageBox(hwnd, "Save changes?", "", MB_OKCANCEL))
					{
						changed = FALSE;
						hFile = CreateFile(fileName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
						WriteFile(hFile, fb, fileSize, &dwBytesWritten, NULL);
						CloseHandle(hFile);
					}
				}
			}
			else { // signed
				itsigned = TRUE;
				ShowLocContents();
			}
			break;
		case 'U': // unsigned
			itsigned = FALSE;
			ShowLocContents();
			break;
		case 'C':
			find = FALSE;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'X':
			itshex = TRUE;
			ShowLocContents();
			break;
		case 'D':
			itshex = FALSE;
			ShowLocContents();
			break;
		case 'B'://byte
			bytes = 1;
			ShowLocContents();
			break;
		case 'W'://word
			bytes = 2;
			ShowLocContents();
			break;
		case 'I'://integer
			bytes = 4;
			ShowLocContents();
			break;
		case VK_F1:
			MessageBox(hwnd, Help, "ViewAnything", MB_OK);
			break;
		case VK_HOME:
			if (GetKeyState(VK_CONTROL) < 0)//CTRL-PAGE UP
			{
				curCol = 0;
				curRow = 0;
				SendMessage(hwnd, WM_VSCROLL, SB_TOP, 0);
			}
			else
			{
				curCol = 0;
				ShowLocContents();
			}
			break;
		case VK_END:
			if (GetKeyState(VK_CONTROL) < 0)
			{
				curCol = (fileSize % charsPerLine) - 1;
				if (curCol == -1)
					curCol = charsPerLine - 1;
				if (fileSize > (Width * Height))
					curRow = Height-1;
				else
					curRow = fileSize / Width;
				SendMessage(hwnd, WM_VSCROLL, SB_BOTTOM, 0);
			}
			else
			{
				curCol = Width-1;
				ShowLocContents();
			}
			break;
		case VK_DOWN:
			if ((curRow < linesPerScreen-1) && (curRow < totalLines-1))
			{
				curRow++;
				ShowLocContents();
			}
			else if (curRow+vScrollPos < totalLines-1)
				SendMessage(hwnd, WM_VSCROLL, SB_LINEDOWN, 0);
			break;
		case VK_UP:
			if (curRow > 0)
			{
				curRow--;
				ShowLocContents();
			}
			else
				SendMessage(hwnd, WM_VSCROLL, SB_LINEUP, 0);
			break;
		case VK_RIGHT:
			if (GetKeyState(VK_CONTROL) < 0)
			{
				changed = TRUE;
				fb[((curRow+vScrollPos) * charsPerLine) + curCol] = 0;
			}
			if (curCol < Width-1)
			{
				curCol++;
				ShowLocContents();
			}
			else//at end of line
			{
				curCol = 0;
				curRow++;
				ShowLocContents();
			}
			break;
		case VK_LEFT:
			if (curCol > 0)
			{
				curCol--;
				ShowLocContents();
			}
			else if (curRow > 0)//at end of line
			{
				curCol = Width - 1;
				curRow--;
				ShowLocContents();
			}
			break;
		case VK_PRIOR:
			SendMessage(hwnd, WM_VSCROLL, SB_PAGEUP, 0);
			break;
		case VK_NEXT:
			SendMessage(hwnd, WM_VSCROLL, SB_PAGEDOWN, 0);
			break;

		case VK_ESCAPE:
			if (changed)
			{
				if (IDOK == MessageBox(hwnd, "Save changes?", "", MB_OKCANCEL))
				{
					changed = FALSE;
					hFile = CreateFile(fileName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
					WriteFile(hFile, fb, fileSize, &dwBytesWritten, NULL);
					CloseHandle(hFile);
				}
			}
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		}
		return 0;

	case WM_LBUTTONUP:
		y = HIWORD(lParam);
		if ((int)y > cyChar)
		{
			x = LOWORD(lParam) / cxChar;
			curCol = Width - 1;
			if (curCol > x)
				curCol = x;
			curRow = (y / cyChar) - 1;
			ShowLocContents();
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint (hwnd, &ps) ;
		fileBuf = fb + (vScrollPos * charsPerLine) ;
		fileBuf2 = fileBuf;
		Color = 0xD0D0D0;//GetSysColor(COLOR_MENU);
		SetBkColor(hdc, Color) ;
		TextOut(hdc, 0, 0, TopLine, 80);
		Color -= 0x101010;//1 level darker
		SetBkColor(hdc, Color);
		for (row = 1; row < (DWORD)(linesPerScreen+1); row++)
		{
			for (x = 0; x < 80; x++)//probably necessary because of a page-change in virtual memory
				TextOut(hdc, x*cxChar, row*cyChar, &fileBuf[x], 1);
			fileBuf += charsPerLine ;
		}
		if ((fileSize+(Width-(fileSize%Width))) > (curCol+(curRow*Width)))
		{
			SetBkColor(hdc, 0xFFFFFF) ;
			TextOut(hdc, cxChar*curCol, cyChar*(curRow+1), &fileBuf2[curCol+(curRow*Width)], bytes);
		}
		if (find)
		{
			SetBkColor(hdc, 0x00FFFF);//yellow
			w = charsPerLine * linesPerScreen;
			for (x = 0; x < w; x++)
			{
//				ch = fileBuf2[x];
//				if ((ch >= 'a') && (ch <= 'z'))
//					ch &= 0xDF;
				if (FindBuf[0] == fileBuf2[x])
				{
					for (y = 1, z = x+1; y < FindBufSize; y++, z++)
					{
//						ch = fileBuf2[z];
//						if ((ch >= 'a') && (ch <= 'z'))
//							ch &= 0xDF;
						if (FindBuf[y] != fileBuf2[z])
							 break;
					}
					if (y == FindBufSize)
					{
						row = ((&(fileBuf2[x]) - &(fileBuf2[0])) / charsPerLine) + 1 ;
						col = (&(fileBuf2[x]) - &(fileBuf2[0])) % charsPerLine ;
						TextOut(hdc, col*cxChar, row*cyChar, FindBuf, y) ;
					}
				}
			}
		}
		EndPaint (hwnd, &ps) ;
		return 0;

	case WM_CLOSE:
		if (changed)
		{
			if (IDCANCEL == MessageBox(hwnd, "Exit without saving changes?", "EXIT", MB_OKCANCEL))
				return 0;
		}
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		if (fileSize)
			VirtualFree(fileBuf, 0, MEM_RELEASE);
		PostQuitMessage (0);
		return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

int CALLBACK FindProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndFind;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndFind = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetFocus(hwndFind);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			for (x = 0; x < 50; x++)
			{
				TempBuf[x] = 0;
				FindBuf[x] = 0;
			}
			TempBufSize = GetWindowText(hwndFind, TempBuf, 50);
			if (0 == TempBufSize)
				break;
			FindBufSize = GetNumbers();
			if (0 == FindBufSize)
				break;
//			for (x = 0; x < Length; x++)
//			{
//				FindBuf[x] = TempBuf[x];
//				if ((FindBuf[x] >= 'a') && (FindBuf[x] <= 'z'))
//					FindBuf[x] &= 0xDF;//make uppercase
//			}
//			FindBuf[x] = 0;
//			ch = FindBuf[0];
			for (x = 0; x < fileSize; x++)
			{
//				ch = fb[x];
//				if ((ch >= 'a') && (ch <= 'z'))
//					ch &= 0xDF;
				if (FindBuf[0] == fb[x])
				{
					for (y = 1, z = x+1; y < FindBufSize; z++, y++)
					{
//						ch = fb[z];
//						if ((ch >= 'a') && (ch <= 'z'))
//							ch &= 0xDF;
						if (FindBuf[y] != fb[z])
							break;
					}
					if (y == FindBufSize)
					{
						FirstLetterLoc = z - y;
						curCol = FirstLetterLoc % charsPerLine;
						if (curCol == -1)
							curCol = charsPerLine - 1;
						if (FirstLetterLoc > (DWORD)(charsPerLine*linesPerScreen))
						{
							curRow = (linesPerScreen/2);
							vScrollPos = (FirstLetterLoc/charsPerLine) - ((linesPerScreen/2) - 1);
						}
						else
						{
							curRow = FirstLetterLoc / charsPerLine;
							vScrollPos = 0;
						}
						find = TRUE;
						SendMessage(hwnd, WM_VSCROLL, 0, 0);
						break;//found it
					}
				}
			}
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK HilightProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndFind;
	
	switch (message)
	{
	case WM_INITDIALOG:
		find = FALSE;
		hwndFind = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetFocus(hwndFind);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			for (x = 0; x < 50; x++)
			{
				TempBuf[x] = 0;
				FindBuf[x] = 0;
			}
			TempBufSize = GetWindowText(hwndFind, TempBuf, 50);
			if (TempBufSize == 0)
				break ;
			FindBufSize = GetNumbers();
			if (0 == FindBufSize)
				break;
			x = ((curRow+vScrollPos)*charsPerLine) + curCol;
			for (y = 0; y < FindBufSize; x++, y++)
				fb[x] = FindBuf[y];
			ShowLocContents();
			changed = TRUE;
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK JumpProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int tempBytes;

	static HWND hwndJump;
	
	switch (message)
	{
	case WM_INITDIALOG:
		find = FALSE;
		hwndJump = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetFocus(hwndJump);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			for (x = 0; x < 50; x++)
			{
				TempBuf[x] = 0;
				FindBuf[x] = 0;
			}
			TempBufSize = GetWindowText(hwndJump, TempBuf, 50);
			if (TempBufSize == 0)
				break ;
			tempBytes = bytes;
			bytes = 4;
			GetNumbers();
			bytes = tempBytes;
			if (*(DWORD*)FindBuf > fileSize)
				break;
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

void ShowLocContents(void)
{
	int x, y, z, loc, row, col;
	unsigned char iContents, ch;
	UINT w, uContents;

	row = curRow+vScrollPos;
	col = curCol;
	TopLine[63] = ' ';
	TopLine[64] = ' ';
	TopLine[65] = ' ';
	loc = (row * charsPerLine) + col ;
	fileBuf = fb;
	y = loc;
	if (itshex)
	{
		for (x = 0, z = 63; x < 11; x++)
			TopLine[x+z] = ' ';
		for (z = 0; z < 7; z++)
		{
			x = y & 0xF ;
			if (x <= 9) x += 0x30 ;
			else x += 0x37 ;
			TopLine[49-z] = (char) x ;
			y >>= 4 ;
		}
		if (bytes == 1)
		{
			iContents = fileBuf[loc] ;
			ch = (iContents & 0xF) ;
			if (ch <= 9) ch += 0x30 ;
			else ch += 0x37 ;
			TopLine[64] = ch ;
			ch = ((iContents >> 4) & 0xF) ;
			if (ch <= 9) ch += 0x30 ;
			else ch += 0x37 ;
			TopLine[63] = ch ;
		}
		else if (bytes == 2)
		{
			for (y = 63, z = loc+1; z >= loc; y +=2, z--)
			{
				iContents = fileBuf[z] ;
				ch = (iContents & 0xF) ;
				if (ch <= 9) ch += 0x30 ;
				else ch += 0x37 ;
				TopLine[y+1] = ch ;
				ch = ((iContents >> 4) & 0xF) ;
				if (ch <= 9) ch += 0x30 ;
				else ch += 0x37 ;
				TopLine[y] = ch ;
			}
		}
		else//if (bytes == 4)
		{
			for (y = 63, z = loc+3; z >= loc; y +=2, z--)
			{
				iContents = fileBuf[z] ;
				ch = (iContents & 0xF) ;
				if (ch <= 9) ch += 0x30 ;
				else ch += 0x37 ;
				TopLine[y+1] = ch ;
				ch = ((iContents >> 4) & 0xF) ;
				if (ch <= 9) ch += 0x30 ;
				else ch += 0x37 ;
				TopLine[y] = ch ;
			}
		}
	}
	else//decimal
	{
		z = 43;
		for (x = 0; x < 8; x++)
			TopLine[x+z] = Zero[x];
		if (loc != 0)
		{
			for (y = 100000000; y > (loc*10); y /= 10)//hundred million
				;
			while (y > 1)
			{
				x = loc % y;
				y /= 10;
				TopLine[z++] = (x / y) + '0';//location
			}
		}
		z = 63;
		for (x = 0; x < 11; x++)
			TopLine[x+z] = ' ';
		if (bytes == 1) {
			uContents = *(BYTE*)&fileBuf[loc];
			if ((itsigned) && (uContents >= 0x80)) {
				uContents = 0xFF - uContents + 1;
				TopLine[z++] = '-';
			}
		}
		else if (bytes == 2) {
			uContents = *(WORD*)&fileBuf[loc];
			if ((itsigned) && (uContents >= 0x8000)) {
				uContents = 0xFFFF - uContents + 1;
				TopLine[z++] = '-';
			}
		}
		else { // if (bytes == 4)
			uContents = *(UINT*)&fileBuf[loc];
			if ((itsigned) && (uContents >= 0x80000000)) {
				uContents = 0xFFFFFFFF - uContents + 1;
				TopLine[z++] = '-';
			}
		}
		if (uContents >= 1000000000)//1 billion
			TopLine[z++] = (uContents / 1000000000) + '0';
		for (w = 1000000000; w > (uContents*10); w /= 10)//one billion
			;
		while (w > 1)
		{
			x = uContents % w;
			w /= 10;
			TopLine[z++] = (x / w) + '0';//location
		}
//			if (uContents < 10)
//				TopLine[z++] = uContents + '0';
//			else if (uContents < 100)
//			{
//				TopLine[z++] = (uContents / 10) + '0';
//				TopLine[z] = (uContents % 10) + '0';
//			}
//			else
//			{
//				TopLine[z++] = (uContents / 100) + '0';
//				TopLine[z++] = ((uContents % 100) / 10) + '0';
//				TopLine[z] = (uContents % 10) + '0';
//			}
	}
	hdc = GetDC(hwnd) ;
	SetBkColor(hdc, 0xC8D0D4) ;
	TextOut(hdc, 0, 0, TopLine, 80);
	ReleaseDC(hwnd, hdc) ;
	InvalidateRect(hwnd, &rect, FALSE);		
}

int ConvertByte(int w)
{
	if ((w >= 'a') && (w <= 'z'))
		w -= 0x20;//make it uppercase
	if ((w >= 'A') && (w <= 'F'))
		z = 0x37;
	else if ((w >= '0') && (w <= '9'))
		z = 0x30;
	else
		return -1;
	return w-z;
}

int GetNumbers(void)
{
	for (x = 0, y = 0; TempBuf[x] != 0; x++, y++)
	{
		if (TempBuf[x] == '\\')
		{
			if ((TempBuf[x+1] == 'x') || (TempBuf[x+1] == 'X'))
			{
				x += 2;//past \x
				if (bytes == 1)
				{
					if (TempBufSize == 3)
					{
						TempBuf[3] = TempBuf[2];
						TempBuf[2] = '0';
					}
					FindBuf[y] = ConvertByte(TempBuf[x]);
					if (FindBuf[y] == -1)
						return FALSE;
					FindBuf[y] <<= 4;
					FindBuf[y] |= ConvertByte(TempBuf[x+1]);
					if (FindBuf[y] == -1)
						return FALSE;
					x += 3 ;//past \x08
				}
				else if (bytes == 2)
				{
					if (TempBufSize < 6)
					{
						for (w = 6; TempBuf[w] == 0; w--)
							;
						for (z = 5; w >= 2; w--, z--)
							TempBuf[z] = TempBuf[w];
						for (w = x, z = TempBufSize; z < 6; w++, z++)
							TempBuf[w] = '0';
					}
					FindBuf[y] = ConvertByte(TempBuf[x+2]);
					if (FindBuf[y] == -1)
						return FALSE;
					FindBuf[y] <<= 4;
					FindBuf[y] |= ConvertByte(TempBuf[x+3]);
					if (FindBuf[y] == -1)
						return FALSE;
					y++;
					FindBuf[y] = ConvertByte(TempBuf[x]);
					if (FindBuf[y] == -1)
						return FALSE;
					FindBuf[y] <<= 4;
					FindBuf[y] |= ConvertByte(TempBuf[x+1]);
					if (FindBuf[y] == -1)
						return FALSE;
					x += 3;
				}
				else // if (bytes == 4)
				{
					if (TempBufSize < 10)
					{
						for (w = 10; TempBuf[w] == 0; w--)
							;
						for (z = 9; w >= 2; w--, z--)
							TempBuf[z] = TempBuf[w];
						for (w = x, z = TempBufSize; z < 10; w++, z++)
							TempBuf[w] = '0';
					}
					FindBuf[y] = ConvertByte(TempBuf[x+6]);
					if (FindBuf[y] == -1)
						return FALSE;
					FindBuf[y] <<= 4;
					FindBuf[y] |= ConvertByte(TempBuf[x+7]);
					if (FindBuf[y] == -1)
						return FALSE;
					y++;
					FindBuf[y] = ConvertByte(TempBuf[x+4]);
					if (FindBuf[y] == -1)
						return FALSE;
					FindBuf[y] <<= 4;
					FindBuf[y] |= ConvertByte(TempBuf[x+5]);
					if (FindBuf[y] == -1)
						return FALSE;
					y++;
					FindBuf[y] = ConvertByte(TempBuf[x+2]);
					if (FindBuf[y] == -1)
						return FALSE;
					FindBuf[y] <<= 4;
					FindBuf[y] |= ConvertByte(TempBuf[x+3]);
					if (FindBuf[y] == -1)
						return FALSE;
					y++;
					FindBuf[y] = ConvertByte(TempBuf[x]);
					if (FindBuf[y] == -1)
						return FALSE;
					FindBuf[y] <<= 4;
					FindBuf[y] |= ConvertByte(TempBuf[x+1]);
					if (FindBuf[y] == -1)
						return FALSE;
					x += 7;
				}
			}
			else if (TempBuf[x+1] == '\\')
			{
				x++;
				FindBuf[y] = TempBuf[x];
			}
			else
			{// TempBuf[] = "\123"
				x++;
				FindBuf[y] = 0;
				for (z = x + 3; (x < z) && ((TempBuf[x] >= '0') && (TempBuf[x] <= '9')); x++)
				{
					if (FindBuf[y] < 26)
					{
						FindBuf[y] *= 10;
						if ((FindBuf[y] == 250) && (TempBuf[x] >= '6'))
							break;
						FindBuf[y] += TempBuf[x] - '0';
					}
					else
						break;
				}
				x--;
			}
		}
		else
			FindBuf[y] = TempBuf[x];
	}
	FindBuf[y] = 0;
	return y;
}
