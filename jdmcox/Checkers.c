#include <windows.h>
#include "resource.h"
#define DIALOGCOLOR 0xD8E9EC
#define BUFF 0xC0F0F8
#define GREEN 0x60B888
#define RED 0x3030F0
#define WHITE 0XF0F8F8

int x, y, col, row, jumpCol, jumpRow, Space, ScreenWidth, ScreenHeight;
int BoardWidth, SquareWidth, SquareWidth2, PieceRadius, LeftSide, xPos, yPos, MovedRow, MovedCol, JumpingRow = 0, JumpingCol = 0;
char szAppName[] = " RED";
char Oops[] = "Oops";
char Georgia[] = "Georgia";
BOOL first = TRUE, redsturn = TRUE, jumped, jumpavailable, played, showhand = FALSE;
struct {
	int x;
	int y;
	BOOL moving;
	BOOL king;
	COLORREF color;
} Piece[12][12];
HBRUSH hDialogBrush, hBuffBrush, hGreenishBrush, hRedBrush, hWhiteBrush, hOldBrush;
HPEN hRedPen, hWhitePen, hOldPen;
HFONT hFont, hOldFont;// -13 -16 -19 -21 -24 -27 -29 -32 -35 -37 -64
HCURSOR hCursor, hHandCursor;
HWND hwnd;
RECT rect, rectMem;
HBITMAP hBitmap;
HDC hdc, hdcMem;
PAINTSTRUCT ps;
COLORREF Color = 0, Red, White, OtherColor;
SYSTEMTIME st;
LOGFONT lf;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	Red = RED;
	White = WHITE;
	hDialogBrush = CreateSolidBrush(DIALOGCOLOR);
	hBuffBrush = CreateSolidBrush(BUFF);
	hGreenishBrush = CreateSolidBrush(GREEN);
	hRedBrush = CreateSolidBrush(Red);
	hWhiteBrush = CreateSolidBrush(White);
	hRedPen = CreatePen(PS_SOLID, 2, Red);
	hWhitePen = CreatePen(PS_SOLID, 2, White);
	hCursor = LoadCursor(NULL, IDC_ARROW);
	hHandCursor = LoadCursor(NULL, IDC_HAND);

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = NULL;// LoadIcon(hInstance, MAKEINTRESOURCE("Checkers.ico"));
	wndclass.hCursor       = hCursor;
	wndclass.hbrBackground = hDialogBrush;
	wndclass.lpszMenuName  = "MENU";
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0))
	{
			x = rect.right;
			y = rect.bottom;
	}
	else
	{
			x = GetSystemMetrics(SM_CXFULLSCREEN);
			y = GetSystemMetrics(SM_CYFULLSCREEN);
	}

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
		(x/2)-248,(y/2)-248,496,496,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, SW_SHOW);
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

	case WM_CREATE:
		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 3;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x12;
		for (x = 0; Georgia[x] != 0; x++)
			lf.lfFaceName[x] = Georgia[x];
		lf.lfFaceName[x] = 0;
		return 0;

	case WM_SIZE:
		ScreenWidth = LOWORD(lParam);
		ScreenHeight = HIWORD(lParam);
		rectMem.left = rectMem.top = 0;
		rectMem.right = ScreenWidth;
		rectMem.bottom = ScreenHeight;
		SquareWidth = ScreenHeight / 8;
		SquareWidth2 = SquareWidth / 2;
		PieceRadius = (SquareWidth2) - (SquareWidth / 10);
		Space = (ScreenHeight % 8) / 2;
		BoardWidth = ScreenHeight - (Space*2);
		LeftSide = (ScreenWidth/2) - (BoardWidth/2);
		if (first) {
			first = FALSE;
			for (x = 0; x < 12; x++)
				for (y = 0; y < 12; y++)
					Piece[y][x].color = 1; // border color
			for (x = 2; x < 10; x++) {
				for (y = 2; y < 10; y++) {
					Piece[y][x].color = 0;
					Piece[y][x].moving = FALSE;
					Piece[y][x].king = FALSE;
				}
			}
			for (x = 3; x < 10; x += 2) {
				Piece[2][x].color = Red;
				Piece[2][x].x = LeftSide+(SquareWidth*(x-2));
				Piece[2][x].y = Space;
			}
			for (x = 2; x < 10; x += 2) {
				Piece[3][x].color = Red;
				Piece[3][x].x = LeftSide+(SquareWidth*(x-2));
				Piece[3][x].y = Space+SquareWidth;
			}
			for (x = 3; x < 10; x += 2) {
				Piece[4][x].color = Red;
				Piece[4][x].x = LeftSide+(SquareWidth*(x-2));
				Piece[4][x].y = Space+(SquareWidth*2);
			}
			for (x = 2; x < 10; x += 2) {
				Piece[7][x].color = White;
				Piece[7][x].x = LeftSide+(SquareWidth*(x-2));
				Piece[7][x].y = Space+(SquareWidth*5);
			}
			for (x = 3; x < 10; x += 2) {
				Piece[8][x].color = White;
				Piece[8][x].x = LeftSide+(SquareWidth*(x-2));
				Piece[8][x].y = Space+(SquareWidth*6);
			}
			for (x = 2; x < 10; x += 2) {
				Piece[9][x].color = White;
				Piece[9][x].x = LeftSide+(SquareWidth*(x-2));
				Piece[9][x].y = Space+(SquareWidth*7);
			}
		}
		else { // if not first (keep pieces in place)
			for (y = 2; y < 10; y++) {
				for (x = 2; x < 10; x++) {
					Piece[y][x].x = LeftSide+(SquareWidth*(x-2));
					Piece[y][x].y = Space+(SquareWidth*(y-2));
				}
			}
		}
		DeleteDC(hdcMem);
		DeleteObject(hBitmap);
		hdc = GetDC(hwnd);
		hdcMem = CreateCompatibleDC(hdc);
		hBitmap = CreateCompatibleBitmap(hdc, ScreenWidth, ScreenHeight);
		ReleaseDC(hwnd, hdc);
		SelectObject(hdcMem, hBitmap);
		if (SquareWidth < 24)
			lf.lfHeight = -13;
		else if (SquareWidth < 28)
			lf.lfHeight = -16;
		else if (SquareWidth < 32)
			lf.lfHeight = -19;
		else if (SquareWidth < 36)
			lf.lfHeight = -21;
		else if (SquareWidth < 38)
			lf.lfHeight = -24;
		else if (SquareWidth < 42)
			lf.lfHeight = -27;
		else if (SquareWidth < 46)
			lf.lfHeight = -29;
		else if (SquareWidth < 48)
			lf.lfHeight = -32;
		else if (SquareWidth < 55)
			lf.lfHeight = -35;
		else if (SquareWidth < 84)
			lf.lfHeight = -37;
		else
			lf.lfHeight = -64;
		hFont = CreateFontIndirect(&lf);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_EXIT:
			DestroyWindow(hwnd);
			break;
		case ID_PLAYAGAIN:
			if (Red == RED) {
				Red = WHITE;
				White = RED;
				redsturn = FALSE;
			}
			else {
				Red = RED;
				White = WHITE;
				redsturn = TRUE;
			}
			SetWindowText(hwnd, " RED");
			hRedBrush = CreateSolidBrush(Red);
			hWhiteBrush = CreateSolidBrush(White);
			first = TRUE;
			SendMessage(hwnd, WM_SIZE, (WPARAM)SIZE_MAXIMIZED, (LPARAM)(ScreenWidth | (ScreenHeight << 0x10)));
			InvalidateRect(hwnd, &rectMem, FALSE);
			break;
		case ID_HELP_RULES:
			MessageBox(hwnd, "Red moves first.\n\nPieces move 1 square diagonally.\n\nYou have to move and you have to jump.\n\nYou have to take multiple jumps if available.\n\nYour piece can only go forward unless it's a King.\n\nA piece becomes a King if it reaches the other side.\n\nYou lose if you don't have any more pieces\nor you can't move any pieces.", "Rules", MB_OK);
			break;
		case ID_HELP_ABOUT:
			MessageBox(hwnd, "Doug Cox\nAug 8, 2010\nhttp://jdmcox.com", "About", MB_OK);
			break;
		}
		return 0;

	case WM_LBUTTONDOWN:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		showhand = FALSE;
			jumpavailable = FALSE;
			played = FALSE;
		for (col = 2, x = LeftSide; col < 10; x += SquareWidth, col++) {
			for (row = 2, y = Space; row < 10; y += SquareWidth, row++) {
				if ((xPos >= x) && (xPos <= (x+SquareWidth)) && (yPos >= y) && (yPos <= (y+SquareWidth))) {
					Color = Piece[row][col].color;
					if ((Color == Red) && (redsturn == FALSE)) {
						if (Red == RED)
							PlaySound("White.wav", NULL, SND_FILENAME);
						else
							PlaySound("Red.wav", NULL, SND_FILENAME);
						return 0;
					}
					else if ((Color == White) && (redsturn == TRUE)) {
						if (Red == RED)
							PlaySound("Red.wav", NULL, SND_FILENAME);
						else
							PlaySound("White.wav", NULL, SND_FILENAME);
						return 0;
					}
					for (jumpCol = 2, x = LeftSide; jumpCol < 10; x += SquareWidth, jumpCol++) {
						for (jumpRow = 2, y = Space; jumpRow < 10; y += SquareWidth, jumpRow++) {
							if (Piece[jumpRow][jumpCol].king) {
								if ((redsturn) && (Piece[jumpRow][jumpCol].color == Red)) {
									if (((Piece[jumpRow+1][jumpCol+1].color == White) && (Piece[jumpRow+2][jumpCol+2].color == 0)) // down right
									 || ((Piece[jumpRow+1][jumpCol-1].color == White) && (Piece[jumpRow+2][jumpCol-2].color == 0)) // down left
									 || ((Piece[jumpRow-1][jumpCol+1].color == White) && (Piece[jumpRow-2][jumpCol+2].color == 0)) // up right
									 || ((Piece[jumpRow-1][jumpCol-1].color == White) && (Piece[jumpRow-2][jumpCol-2].color == 0))) // up 
									{
										jumpavailable = TRUE;
										if ((jumpRow == row) && (jumpCol == col))
											goto jump;
									}
								}
								else if ((!redsturn) && (Piece[jumpRow][jumpCol].color == White)) {
									if (((Piece[jumpRow+1][jumpCol+1].color == Red) && (Piece[jumpRow+2][jumpCol+2].color == 0)) // down right
									 || ((Piece[jumpRow+1][jumpCol-1].color == Red) && (Piece[jumpRow+2][jumpCol-2].color == 0)) // down left
									 || ((Piece[jumpRow-1][jumpCol+1].color == Red) && (Piece[jumpRow-2][jumpCol+2].color == 0)) // up right
									 || ((Piece[jumpRow-1][jumpCol-1].color == Red) && (Piece[jumpRow-2][jumpCol-2].color == 0))) // up 
									{
										jumpavailable = TRUE;
										if ((jumpRow == row) && (jumpCol == col))
											goto jump;
									}
								}
							}
							else if ((redsturn) && (Piece[jumpRow][jumpCol].color == Red)) {
								if (((Piece[jumpRow+1][jumpCol+1].color == White) && ((Piece[jumpRow+2][jumpCol+2].color == 0)))
								 || ((Piece[jumpRow+1][jumpCol-1].color == White) && ((Piece[jumpRow+2][jumpCol-2].color == 0))))
								{
									jumpavailable = TRUE;
									if ((jumpRow == row) && (jumpCol == col))
										goto jump;
								}
							}
							else if ((!redsturn) && (Piece[jumpRow][jumpCol].color == White)) {
								if (((Piece[jumpRow-1][jumpCol+1].color == Red) && ((Piece[jumpRow-2][jumpCol+2].color == 0)))
								 || ((Piece[jumpRow-1][jumpCol-1].color == Red) && ((Piece[jumpRow-2][jumpCol-2].color == 0))))
								{
									jumpavailable = TRUE;
									if ((jumpRow == row) && (jumpCol == col))
										goto jump;
								}
							}
						}
					}
					if (jumpavailable) {
						played = TRUE;
						PlaySound("Jump.wav", NULL, SND_FILENAME);
						return 0;
					}
jump:				Piece[row][col].moving = TRUE;
					MovedRow = row;
					MovedCol = col;
					showhand = TRUE;
					SetCursor(hHandCursor);
				}
			}
		}
		return 0;

	case WM_MOUSEMOVE:
		if (wParam == MK_LBUTTON) {
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			if ((showhand) && ((Color == Red) || (Color == White)))
				SetCursor(hHandCursor);
			InvalidateRect(hwnd, &rectMem, FALSE);
		}
		return 0;

	case WM_LBUTTONUP:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		if ((Color == Red) || (Color == White)) {
			if (((Color == Red) && (redsturn == TRUE)) || ((Color == White) && (redsturn == FALSE))) {
				SetCursor(hCursor);
			for (col = 2, x = LeftSide; col < 10; x += SquareWidth, col++) {
					for (row = 2, y = Space; row < 10; y += SquareWidth, row++) {
						if ((xPos >= x) && (xPos <= (x+SquareWidth)) && (yPos >= y) && (yPos <= (y+SquareWidth))) {
							jumped = FALSE;
							if (Piece[MovedRow][MovedCol].king) {
								if (Piece[row][col].color == 0) {
									if (row == (MovedRow+2)) {
										if (col == MovedCol+2) {
											if ((Piece[MovedRow+1][MovedCol+1].color) && (Piece[MovedRow+1][MovedCol+1].color) != (Piece[MovedRow][MovedCol].color)) {
												Piece[MovedRow+1][MovedCol+1].color = 0;
												jumped = TRUE;
											}
											else {
												Piece[MovedRow][MovedCol].moving = FALSE;
												goto endo;
											}
										}
										else if (col == (MovedCol-2)) {
											if ((Piece[MovedRow+1][MovedCol-1].color) && (Piece[MovedRow+1][MovedCol-1].color) != (Piece[MovedRow][MovedCol].color)) {
												Piece[MovedRow+1][MovedCol-1].color = 0;
												jumped = TRUE;
											}
											else {
												Piece[MovedRow][MovedCol].moving = FALSE;
												goto endo;
											}
										}
										else {
											Piece[MovedRow][MovedCol].moving = FALSE;
											goto endo;
										}
									}
									else if (row == (MovedRow-2)) {
										if (col == MovedCol+2) {
											if ((Piece[MovedRow-1][MovedCol+1].color) && (Piece[MovedRow-1][MovedCol+1].color) != (Piece[MovedRow][MovedCol].color)) {
												Piece[MovedRow-1][MovedCol+1].color = 0;
												jumped = TRUE;
											}
											else {
												Piece[MovedRow][MovedCol].moving = FALSE;
												goto endo;
											}
										}
										else if (col == (MovedCol-2)) {
											if ((Piece[MovedRow-1][MovedCol-1].color) && (Piece[MovedRow-1][MovedCol-1].color) != (Piece[MovedRow][MovedCol].color)) {
												Piece[MovedRow-1][MovedCol-1].color = 0;
												jumped = TRUE;
											}
											else {
												Piece[MovedRow][MovedCol].moving = FALSE;
												goto endo;
											}
										}
										else {
											Piece[MovedRow][MovedCol].moving = FALSE;
											goto endo;
										}
									}
									else if (((row != (MovedRow+1)) && (row != (MovedRow-1))) || ((col != (MovedCol+1))) && (col != (MovedCol-1))) {
										Piece[MovedRow][MovedCol].moving = FALSE;
										goto endo;
									}
								}
								else {
									Piece[MovedRow][MovedCol].moving = FALSE;
									goto endo;
								}
							}
							else { // not a king
								if (Piece[MovedRow][MovedCol].color == Red) {
									if (Piece[row][col].color == 0) {
										if (row == (MovedRow+2)) {
											if (col == MovedCol+2) {
												if ((Piece[MovedRow+1][MovedCol+1].color) && (Piece[MovedRow+1][MovedCol+1].color) != (Piece[MovedRow][MovedCol].color)) {
													Piece[MovedRow+1][MovedCol+1].color = 0;
													jumped = TRUE;
												}
												else {
													Piece[MovedRow][MovedCol].moving = FALSE;
													goto endo;
												}
											}
											else if (col == (MovedCol-2)) {
												if ((Piece[MovedRow+1][MovedCol-1].color) && (Piece[MovedRow+1][MovedCol-1].color) != (Piece[MovedRow][MovedCol].color)) {
													Piece[MovedRow+1][MovedCol-1].color = 0;
													jumped = TRUE;
												}
												else {
													Piece[MovedRow][MovedCol].moving = FALSE;
													goto endo;
												}
											}
											else {
												Piece[MovedRow][MovedCol].moving = FALSE;
												goto endo;
											}
										}
										else if ((row != (MovedRow+1)) || ((col != MovedCol-1) && (col != MovedCol+1))) {
											Piece[MovedRow][MovedCol].moving = FALSE;
											goto endo;
										}
									}
									else {
										Piece[MovedRow][MovedCol].moving = FALSE;
										goto endo;
									}
								}
								else if (Piece[MovedRow][MovedCol].color == White) {
									if (Piece[row][col].color == 0) {
										if (row == (MovedRow-2)) {
											if (col == MovedCol+2) {
												if ((Piece[MovedRow-1][MovedCol+1].color) && (Piece[MovedRow-1][MovedCol+1].color) != (Piece[MovedRow][MovedCol].color)) {
													Piece[MovedRow-1][MovedCol+1].color = 0;
													jumped = TRUE;
												}
												else {
													Piece[MovedRow][MovedCol].moving = FALSE;
													goto endo;
												}
											}
											else if (col == (MovedCol-2)) {
												if ((Piece[MovedRow-1][MovedCol-1].color) && (Piece[MovedRow-1][MovedCol-1].color) != (Piece[MovedRow][MovedCol].color)) {
													Piece[MovedRow-1][MovedCol-1].color = 0;
													jumped = TRUE;
												}
												else {
													Piece[MovedRow][MovedCol].moving = FALSE;
													goto endo;
												}
											}
											else {
												Piece[MovedRow][MovedCol].moving = FALSE;
												goto endo;
											}
										}
										else if ((row != (MovedRow-1)) || ((col != MovedCol-1) && (col != MovedCol+1))) {
											Piece[MovedRow][MovedCol].moving = FALSE;
											goto endo;
										}
									}
									else {
										Piece[MovedRow][MovedCol].moving = FALSE;
										goto endo;
									}
								}
							} // end of else not a king
							if ((jumpavailable) && (jumped == FALSE)) {
								Piece[MovedRow][MovedCol].moving = FALSE;
								if (!played) {
									PlaySound("Jump.wav", NULL, SND_FILENAME);
								}
								goto endo;
							}
							Piece[row][col].x = x;
							Piece[row][col].y = y;
							Piece[row][col].color = Color;
							Piece[row][col].moving = FALSE;
							if (Piece[MovedRow][MovedCol].king)
								Piece[row][col].king = TRUE;
							else
								Piece[row][col].king = FALSE;
							Piece[MovedRow][MovedCol].color = 0;
							Piece[MovedRow][MovedCol].moving = FALSE;
							Piece[MovedRow][MovedCol].king = FALSE;
							if ((row == 2) || (row == 9)) {
								Piece[row][col].king = TRUE;
								jumped = FALSE; // new king can't keep jumping
							}
							if (jumped) {
								if (Piece[row][col].king == TRUE) {
									if (Color == Red)
										OtherColor = White;
									else
										OtherColor = Red;
									if (((Piece[row+1][col+1].color != OtherColor) || (Piece[row+2][col+2].color != 0)) // down right
									 && ((Piece[row+1][col-1].color != OtherColor) || (Piece[row+2][col-2].color != 0)) // down left
									 && ((Piece[row-1][col+1].color != OtherColor) || (Piece[row-2][col+2].color != 0)) // up right
									 && ((Piece[row-1][col-1].color != OtherColor) || (Piece[row-2][col-2].color != 0))) // up keft
									{
										redsturn ^= 1;
										JumpingRow = JumpingCol = 0;
									}
									else {
										JumpingRow = row;
										JumpingCol = col;
									}
								}
								else { // not king
									if (Color == Red) {
										if (((Piece[row+1][col+1].color != White) || (Piece[row+2][col+2].color != 0))
										 && ((Piece[row+1][col-1].color != White) || (Piece[row+2][col-2].color != 0)))
										{
											redsturn = FALSE;
											JumpingRow = JumpingCol = 0;
										}
										else {
											JumpingRow = row;
											JumpingCol = col;
										}
									}
									else { // White
										if (((Piece[row-1][col+1].color != Red) || (Piece[row-2][col+2].color != 0))
										 && ((Piece[row-1][col-1].color != Red) || (Piece[row-2][col-2].color != 0)))
										{
											redsturn = TRUE;
											JumpingRow = JumpingCol = 0;
										}
										else {
											JumpingRow = row;
											JumpingCol = col;
										}
									}
								}
							}
							else { // not jumped
								if (Color == Red) redsturn = FALSE;
								else redsturn = TRUE;
							}
							goto endo;
						} // end of if ((xPos >= x) && (xPos <= (x+SquareWidth)) && (yPos >= y) && (yPos <= (y+SquareWidth)))
					}
				}
				if ((col == 10) && (row == 10))
					Piece[MovedRow][MovedCol].moving = FALSE; // don't show move
			} // end of if (((Color == Red) && (redsturn == TRUE)) || ((Color == White) && (redsturn == FALSE)))
endo:		Color = 0;
			if (((redsturn) && (Red == RED)) || ((!redsturn) && (Red == WHITE)))
				SetWindowText(hwnd, " RED");
			else
				SetWindowText(hwnd, " WHITE");
			InvalidateRect(hwnd, &rectMem, FALSE);
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		FillRect(hdcMem, &rectMem, hDialogBrush);
		hOldBrush = SelectObject(hdcMem, hBuffBrush);
		hOldFont = SelectObject(hdcMem, hFont);
		SetBkMode(hdcMem, TRANSPARENT);
		for (col = 0, x = LeftSide; col < 4; x += (SquareWidth*2), col++) {
			for (row = 0, y = Space; row < 4; y+= (SquareWidth*2), row++) {
				Rectangle(hdcMem, x, y, x + SquareWidth+1, y + SquareWidth+1);
			}
		}
		for (col = 0, x = LeftSide+SquareWidth; col < 4; x += (SquareWidth*2), col++) {
			for (row = 0, y = Space+SquareWidth; row < 4; y+= (SquareWidth*2), row++) {
				Rectangle(hdcMem, x, y, x + SquareWidth+1, y + SquareWidth+1);
			}
		}
		SelectObject(hdcMem, hGreenishBrush);
		for (col = 0, x = LeftSide+SquareWidth; col < 4; x += (SquareWidth*2), col++) {
			for (row = 0, y = Space; row < 4; y+= (SquareWidth*2), row++) {
				Rectangle(hdcMem, x, y, x + SquareWidth+1, y + SquareWidth+1);
			}
		}
		for (col = 0, x = LeftSide; col < 4; x += (SquareWidth*2), col++) {
			for (row = 0, y = Space+SquareWidth; row < 4; y+= (SquareWidth*2), row++) {
				Rectangle(hdcMem, x, y, x + SquareWidth+1, y + SquareWidth+1);
			}
		}
		for (x = 2; x < 10; x++) {
			for (y = 2; y < 10; y++) {
				if (Piece[y][x].color == Red) {
					SelectObject(hdcMem, hRedBrush);
				}
				else if (Piece[y][x].color == White) {
					SelectObject(hdcMem, hWhiteBrush);
				}
				else
					continue;
				if (Piece[y][x].moving == FALSE) {
					Ellipse(hdcMem, Piece[y][x].x+(SquareWidth/2)-PieceRadius, Piece[y][x].y+(SquareWidth/2)-PieceRadius, Piece[y][x].x+(SquareWidth/2)+PieceRadius, Piece[y][x].y+(SquareWidth/2)+PieceRadius);
					if (Piece[y][x].king)
						TextOut(hdcMem, Piece[y][x].x+(SquareWidth/2)-PieceRadius+5, Piece[y][x].y+(SquareWidth/2)-PieceRadius, "K", 1);
				}
				else {
					Ellipse(hdcMem, xPos-PieceRadius, yPos-PieceRadius, xPos+PieceRadius, yPos+PieceRadius);
					if (Piece[y][x].king)
						TextOut(hdcMem, xPos-PieceRadius+5, yPos-PieceRadius, "K", 1);
				}
			}
		}
		SelectObject(hdcMem, hOldFont);
		SelectObject(hdcMem, hOldBrush);
		BitBlt(hdc, 0, 0, ScreenWidth, ScreenHeight, hdcMem, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
