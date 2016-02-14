//add photo drawing capability
//if a photo jpeg is opened, and the same-named drawing is in the photo's folder, open them both
//if there's no same-named drawing, create it after opening photo jpeg
//alpha-blending blends photo and its drawing on-screen if they exist, but they remain separate data
//photo and drawing will show full-size, with button to make it fit screen
//mouse movement will scroll full-size photo and drawing if menus are showing

//compiled using Microsoft's Visual Studio 6.0
#include <windows.h>
#include <math.h>//for sqrt
#include <stdio.h>//for stream
#include <wintab.h>//(for a Tablet) put wintab32.lib in Project -Settings -Link
#include "MSGPACK.H"//for Wintab
#include "zlib.h"//put zdll.lib in Project -Settings -Link
#include "zconf.h"//and make zlib1.dll accessible
#define PACKETDATA	(PK_X|PK_Y|PK_BUTTONS|PK_NORMAL_PRESSURE|PK_ORIENTATION)//for Wintab
#define PACKETMODE	PK_BUTTONS//for Wintab
#include <pktdef.h>//for Wintab
#include "resource.h"
#define MAXPOINTS 100000
#define ALPHABET FALSE

int index = 0, Files, changecolor = 0;
int c = 0, maxc, Added[10000];
int x, y, X, Y, ptr = 0, lbMax, Radius = 8, OldRadius = 0, radiusTemp = 8, radius, ButtonDownX, ButtonDownY;
int Blend = 255, oldBlend = 255, opacity, response, CmdLineLen = 0, Color, rightXbottom;
int xRedPos = 15, yRedPos, xGreenPos = 50, yGreenPos, xBluePos = 85, yBluePos, Red = 0xB0, Green = 0x60, Blue = 0x60;
int CenterX, CenterY, LeftOfColors, origRed, origGreen, origBlue, endRed, endGreen, endBlue;
int BoxL, BoxT, BoxR, BoxB, boxl, boxt, oldRed = 0xFF, oldGreen = 0xFF, oldBlue = 0xFF;
unsigned long screenLen, SavedScreenLen;
DWORD fileSize, dwBytesRead, dwBytesWritten, rightXbottomX4;
DWORD *MemBof, *Mem2Bof, *Mem3Bof;
BYTE *MemBuf, *Mem2Buf, *Mem3Buf, *SaveBuf;
BYTE red, green, blue, srcRed, srcGreen, srcBlue, destRed, destGreen, destBlue;
BYTE *screen;
BYTE *Screen[10000];
BOOL wintab, usingmouse = FALSE, first = TRUE, pendown = FALSE, gettingsomething = FALSE, magnifying = FALSE, fromsave = FALSE, keyup = TRUE;
BOOL inred = FALSE, ingreen = FALSE, inblue = FALSE, inopaque = FALSE, incircle = FALSE, newdot, checkcolor = FALSE, inmenu;
BOOL addRed, addGreen, addBlue, showalphabet = FALSE, openingfile = FALSE, help = FALSE, changed, straightline = FALSE, endstraightline = FALSE;
double blend = 255.0, a, b;
double SrcRed, SrcGreen, SrcBlue, DestRed, DestGreen, DestBlue;
double dRed, dGreen, dBlue;
char *Help;
char szAppName[] = "SimplePaint";
char Filename[MAX_PATH] = "\0";
char FullFilename[MAX_PATH];
char CurrentDir[MAX_PATH];
char ArialRounded[] = "Arial Rounded MT Bold";
char ABCs[] = "ABCs ";
char Alphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";
//COLORREF MagnifiedBox[36][36];
HWND hwnd, hwndColor, hwndMenu, hwndMagnify, hwndHelp, hwndList;
HWND hwndButton1, hwndButton3,  hwndButton4, hwndButton5, hwndButton6, hwndButton7, hwndButton8, hwndButton9;
HWND hwndMessage, hwndMessage2, hwndYes, hwndNo;
HINSTANCE hInstance;
HANDLE hFile, hGZFile, hFind;
HDC hdc, hdcMem, hdcMem2, hdcMem3;
HBITMAP hBitmap, hMemBitmap, hMem2Bitmap, hMem3Bitmap;
PAINTSTRUCT ps;
RECT rect, smallrect;
SIZE size;
HPEN hPen, hMagPen;
HBRUSH hWhiteBrush, hBlackBrush, hMagBrush, hFillBrush = NULL;
HGDIOBJ hOldPen, hOldBrush, OldhMemBitmap, OldhMem2Bitmap, OldhMem3Bitmap, hOldFont;
HCURSOR hCursor, hWaitingCursor;
HFONT hFont;
HICON hIcon;
LOGCONTEXT lcMine, lc;
AXIS np;
PACKET pkt, pktOld;
HCTX hTab = NULL;
BITMAPFILEHEADER bmfh;
BITMAPINFO bmi;
BITMAPINFOHEADER bmih;
COLORREF DestColor, SrcColor, MagColor, tempColor, AlphabetColor, ChangeColor, NewColor;
OPENFILENAME ofn;
SYSTEMTIME st;
LOGFONT lf;
WIN32_FIND_DATA fd;
WNDPROC pColorsProc, pMenuProc, pMagnifyProc, pHelpProc, pMessageProc, pListProc;

struct {
	WORD x;
	WORD y;
	WORD radius;
} points[MAXPOINTS];

void ShowHelp(void);
void Magnify(void);
void PenUp(void);
void PreGetColor(void);
void SaveScreen(void);
void OpenSavedFile(void);
void Menu(void);
void DeleteOrUndelete(void);
void GetDateTime(void);
void QuitSaveCancel(void);
void ShowAlphabet(void);
void EndStraightLine(void);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR CmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	hWhiteBrush = CreateSolidBrush(0xFFFFFF);
	hBlackBrush = CreateSolidBrush(0x000000);
	hCursor = LoadCursor(NULL, IDC_ARROW);
	hWaitingCursor = LoadCursor(NULL, IDC_WAIT);

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = hCursor;
	wndclass.hbrBackground = hWhiteBrush;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	if (CmdLine[0] != 0)
	{
		for (x = 0, CmdLineLen = 0; x < MAX_PATH && CmdLine[x] != 0; x++)
			if (CmdLine[x] != '"')
				Filename[CmdLineLen++] = CmdLine[x];
		Filename[CmdLineLen] = 0;
	}

	hwnd = CreateWindow(szAppName, szAppName,
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


//sub-class procedure
LRESULT CALLBACK ColorsProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y;
	char RedNum[] = "    ", GreenNum[] = "    ", BlueNum[] = "    ", AlphaNum[] = "    ";
	HBRUSH hColorBrush;
	HDC hdcColors;
	PAINTSTRUCT psColors;

	if (message == WM_PAINT) {
///
		if (pkt.pkOrientation.orAltitude & 0x80000000) {//using eraser
			return 0;
		}
///
		hdcColors = BeginPaint(hwnd2, &psColors);
		hColorBrush = CreateSolidBrush((Blue << 16) | (Green << 8) | Red);//the opposite of the order colors are stored in in screen memory
		SelectObject(hdcColors, hColorBrush);
		if (inred)
			SetBkColor(hdcColors, 0xFF);
		else
			SetBkColor(hdcColors, 0xFFFFFF);
		TextOut(hdcColors, 0, 30, "   R  ", 6);
		if (ingreen)
			SetBkColor(hdcColors, 0xFF00);
		else
			SetBkColor(hdcColors, 0xFFFFFF);
		TextOut(hdcColors, 35, 30, "   G  ", 6);
		if (inblue)
			SetBkColor(hdcColors, 0xFF0000);
		else
			SetBkColor(hdcColors, 0xFFFFFF);
		TextOut(hdcColors, 70, 30, "   B  ", 6);
		if (inopaque) {
			SetBkColor(hdcColors, 0);
			SetTextColor(hdcColors, 0xFFFFFF);
		}
		else
			SetBkColor(hdcColors, 0xFFFFFF);
		TextOut(hdcColors, 102, 30, "Blend", 5);
//		TextOut(hdcColors, 102, 30, "Alpha", 5);
		SetBkColor(hdcColors, 0xFFFFFF);
		SetTextColor(hdcColors, 0);
		RedNum[1] = (Red / 0x10) + '0';
		if (RedNum[1] > '9') RedNum[1] += 7;
		RedNum[2] = (Red % 0x10) + '0';
		if (RedNum[2] > '9') RedNum[2] += 7;
		TextOut(hdcColors, 0, 570, RedNum, 4);
		GreenNum[1] = (Green / 0x10) + '0';
		if (GreenNum[1] > '9') GreenNum[1] += 7;
		GreenNum[2] = (Green % 0x10) + '0';
		if (GreenNum[2] > '9') GreenNum[2] += 7;
		TextOut(hdcColors, 35, 570, GreenNum, 4);
		BlueNum[1] = (Blue / 0x10) + '0';
		if (BlueNum[1] > '9') BlueNum[1] += 7;
		BlueNum[2] = (Blue % 0x10) + '0';
		if (BlueNum[2] > '9') BlueNum[2] += 7;
		TextOut(hdcColors, 70, 570, BlueNum, 4);
		AlphaNum[1] = (Blend / 0x10) + '0';
		if (AlphaNum[1] > '9') AlphaNum[1] += 7;
		AlphaNum[2] = (Blend % 0x10) + '0';
		if (AlphaNum[2] > '9') AlphaNum[2] += 7;
		TextOut(hdcColors, 107, 570, AlphaNum, 4);
		y = 50;
		x = 0;//red loc
		Rectangle(hdcColors, x, y, x+30, y+516);
		x = 35;//green loc
		Rectangle(hdcColors, x, y, x+30, y+516);
		x = 70;//blue loc
		Rectangle(hdcColors, x, y, x+30, y+516);
		SelectObject(hdcColors, hBlackBrush);
		Rectangle(hdcColors, xRedPos-3, yRedPos-3, xRedPos+3, yRedPos+3);
		Rectangle(hdcColors, xGreenPos-3, yGreenPos-3, xGreenPos+3, yGreenPos+3);
		Rectangle(hdcColors, xBluePos-3, yBluePos-3, xBluePos+3, yBluePos+3);
		SelectObject(hdcColors, hColorBrush);
		if (incircle) {
			SelectObject(hdcColors, hWhiteBrush);
			Ellipse(hdcColors, 0, rect.bottom-140, 140, rect.bottom);//clear any previous
			SelectObject(hdcColors, hColorBrush);
		}
		Ellipse(hdcColors, 70-Radius, rect.bottom-70-Radius, 70+Radius, rect.bottom-70+Radius);
		for (y = 52, x = 256; x >= 0; y += 2, x--) {//Alpha blending
			a = x/255.0;
			b = 1.0 - a;
			dRed = (((double)Red * a) + (255.0 * b));
			dGreen = ((double)Green * a) + (255.0 * b);
			dBlue = ((double)Blue * a) + (255.0 * b);
			hPen = CreatePen(PS_SOLID, 2, ((BYTE)dBlue << 16) | ((BYTE)dGreen << 8) | ((BYTE)dRed));//reversed color order
			hOldPen = SelectObject(hdcColors, hPen);
			MoveToEx(hdcColors, 105, y, NULL);//show opacity
			LineTo(hdcColors, 134, y);
			SelectObject(hdcColors, hOldPen);
			MoveToEx(hdcColors, 105, 50, NULL);
			LineTo(hdcColors, 135, 50);
			LineTo(hdcColors, 135, 565);
			LineTo(hdcColors, 105, 565);
			LineTo(hdcColors, 105, 50);
			DeleteObject(hPen);
		}
		opacity = 563 - (Blend*2);
		x = 105;
		Rectangle(hdcColors, x+15-3, opacity-3, x+15+3, opacity+3);
		DeleteObject(hColorBrush);
		EndPaint(hwnd2, &psColors);
	}
	else if (message == WM_KEYDOWN) {
		if ((wParam == VK_CONTROL) || (wParam == VK_SHIFT) || (wParam == VK_ESCAPE) || (wParam == VK_DELETE) || (wParam == VK_BACK)) {
			inmenu = FALSE;
			if (checkcolor) {
				checkcolor = FALSE;
				Blend = 0xFF;
				NewColor = (Red << 16) | (Green << 8) | Blue;
				for (x = 0; x < rightXbottom; x++)
					if (Mem2Bof[x] == ChangeColor)
						Mem2Bof[x] = NewColor;
				SelectObject(hdcMem2, OldhMem2Bitmap);
				SetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, Mem2Buf, &bmi, DIB_RGB_COLORS);
				SelectObject(hdcMem2, hMem2Bitmap);
				hdc = GetDC(hwnd);
				BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
				if (showalphabet)
					ShowAlphabet();
				ReleaseDC(hwnd, hdc);
				////////////
				SaveScreen();
				////////////
			}
			else {
				Blend = (563 - opacity)/2;
				blend = (double)Blend;
				a = blend/255.0;//for Microsoft's AlphaBlend
				b = 1.0 - a;
			}
			DestroyWindow(hwndColor);
			DestroyWindow(hwndMenu);
			SetFocus(hwnd);
			gettingsomething = FALSE;
			InvalidateRect(hwnd, &rect, FALSE);
		}
		else if ((wParam == 'R') && (help == FALSE))
			ShowHelp();
		else if (wParam == 'E')
			SendMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
		else if (wParam == 'O')
			SendMessage (hwndMenu, WM_COMMAND, 0, (LPARAM)(HWND)hwndButton4);
		else if (wParam == 'S')
			SendMessage (hwndMenu, WM_COMMAND, 0, (LPARAM)(HWND)hwndButton5);
		else if (wParam == 'C')
			SendMessage (hwndMenu, WM_COMMAND, 0, (LPARAM)(HWND)hwndButton6);
		else if (wParam == 'F')
			SendMessage (hwndMenu, WM_COMMAND, 0, (LPARAM)(HWND)hwndButton7);
	}
	else if (message == WM_SYSKEYDOWN) {
		if (wParam == 'R')
			ShowHelp();
		else if (wParam == 'E')
			SendMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
		else if (wParam == 'O')
			SendMessage (hwndMenu, WM_COMMAND, 0, (LPARAM)(HWND)hwndButton4);
		else if (wParam == 'S')
			SendMessage (hwndMenu, WM_COMMAND, 0, (LPARAM)(HWND)hwndButton5);
		else if (wParam == 'C')
			SendMessage (hwndMenu, WM_COMMAND, 0, (LPARAM)(HWND)hwndButton6);
		else if (wParam == 'F')
			SendMessage (hwndMenu, WM_COMMAND, 0, (LPARAM)(HWND)hwndButton7);
		return 0;// not break;
	}
	return CallWindowProc(pColorsProc, hwnd2, message, wParam, lParam);
}

void GetColor(void)
{
	incircle = TRUE;
	if ((oldRed != 0xFF) && (oldGreen!= 0xFF) && (oldBlue != 0xFF)) {
		Red = oldRed;
		Green = oldGreen;
		Blue = oldBlue;
		oldRed = 0xFF;
		oldGreen = 0xFF;
		oldBlue = 0xFF;
	}
	yRedPos = 563 - (Red*2);//because Red = (563-yRedPos)/2;
	yGreenPos = 563 - (Green*2);
	yBluePos = 563 - (Blue*2);
	gettingsomething = TRUE;
	hwndColor = CreateWindow("STATIC", NULL,
		WS_CHILD | WS_VISIBLE,
		LeftOfColors, 0, 140, -lcMine.lcOutExtY,//if wintab == 0 it equals rect.bottom
		hwnd, (HMENU)188, hInstance, NULL);
	SetFocus(hwndColor);
	pColorsProc = (WNDPROC)SetWindowLong(hwndColor, GWL_WNDPROC, (LONG)ColorsProc);
}

/*
			else if ((HIWORD (wParam) == LBN_SELCHANGE) && (fromnewlink == FALSE))
			{
				if (index == -1)
				{
					IndivNum = 0;//flag
					if (hwndIndiv)
					{
						if (DestroyWindow(hwndIndiv))
							hwndIndiv = NULL;
					}
					hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);
					indivbox = TRUE;
					SetFocus(hwndList);
				}
				index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
*/


//sub-class procedure
LRESULT ListProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONUP)
	{
		index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
		if (index < Files) {
			SendMessage(hwndList, LB_GETTEXT, index, (LPARAM) (LPCTSTR)Filename);
			DestroyWindow(hwndList);
			OpenSavedFile();
		}
	}
	else if (message == WM_KEYUP)
	{
		if ((wParam == VK_DOWN) && (index < (Files-1)))
			index++;
		else if ((wParam == VK_UP) && (index != 0))
			index--;

		else if (wParam == VK_ESCAPE) {
			DestroyWindow(hwndList);
		}
		else if (wParam == VK_RETURN) {
			index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			if (index < Files) {
				SendMessage(hwndList, LB_GETTEXT, index, (LPARAM) (LPCTSTR)Filename);
				DestroyWindow(hwndList);
				OpenSavedFile();
			}
		}
	}
	else if (message == WM_DESTROY) {
		openingfile = FALSE;
	}
	return CallWindowProc(pListProc, hwnd2, message, wParam, lParam);
}

//sub-class procedure
LRESULT HelpProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN) {
		DestroyWindow(hwndHelp);
	}
	else if (message == WM_DESTROY) {
		help = FALSE;
		SetFocus(hwndColor);
	}
	return CallWindowProc(pHelpProc, hwnd2, message, wParam, lParam);
}

LRESULT MenuProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
///
		if (pkt.pkOrientation.orAltitude & 0x80000000) {//using eraser
			SetFocus(hwndColor);
			return CallWindowProc(pMenuProc, hwnd2, message, wParam, lParam);
		}
///
	if (message == WM_COMMAND) {
		if (((HWND)(LPARAM)lParam == hwndButton1) && (help == FALSE)) {//HELP
			ShowHelp();
		}
		else if (((HWND)(LPARAM)lParam == hwndButton3)) {//EXIT
			SendMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
		}

		else if ((((HWND)(LPARAM)lParam == hwndButton4)) && (openingfile == FALSE)) {//OPEN
			if (help) {
				help = FALSE;
				SetFocus(hwndColor);
				DestroyWindow(hwndHelp);
			}
			hFind = FindFirstFile("*.bmp.gz", &fd);
			if (INVALID_HANDLE_VALUE != hFind) {
				hwndList = CreateWindow("LISTBOX", "Saved Files",
					WS_CHILD | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_VSCROLL | LBS_SORT,// | LBS_NOTIFY
					(rect.right/2)-120, (rect.bottom/2)-200, 240, 400,
					hwnd, (HMENU)400, hInstance, NULL);
				pListProc = (WNDPROC)SetWindowLong(hwndList, GWL_WNDPROC, (LONG)ListProc);
				SetFocus(hwndList);
				Files = 0;

				SendMessage (hwndList, LB_ADDSTRING, 0, (LPARAM)&fd.cFileName);
				Files++;
				while (FindNextFile(hFind, &fd)) {
					SendMessage (hwndList, LB_ADDSTRING, 0, (LPARAM)&fd.cFileName);
					Files++;
				}
				SendMessage(hwndList, LB_SETCURSEL, index, 0);
				FindClose(hFind);
				openingfile = TRUE;
			}
			else {
				hdc = GetDC(hwnd);
				TextOut(hdc, rect.right-140-220-200, 30, "No Saved Files To Open", 22);
				ReleaseDC(hwnd, hdc);
				SetFocus(hwndColor);
			}
		}

		if ((fromsave == FALSE) && (message == WM_COMMAND) && ((HWND)(LPARAM)lParam == hwndButton5)) {//SAVE
			for (x = 0; x < rightXbottom; x++) {
				if (Mem2Bof[x] == 0x00FFFFFE)
					Mem2Bof[x] = 0x00FFFFFF;
				if (Mem2Bof[x] != 0x00FFFFFF)
					break;
			}
			if (x != rightXbottom) {
				fromsave = TRUE;
				QuitSaveCancel();
			}
			else
				SetFocus(hwndColor);
		}
		else  if (((HWND)(LPARAM)lParam == hwndButton6)) {//CLEAR
			if (c) {
				c--;
				if (Mem3Buf) {
					VirtualFree(Mem3Buf, 0, MEM_RELEASE);
					Mem3Buf = NULL;
					DeleteObject(hMem3Bitmap);
				}
				if (hFillBrush) {
					DeleteObject(hFillBrush);
					hFillBrush = NULL;
				}
				FillRect(hdcMem2, &rect, hWhiteBrush);
				for (x = 0; x < rightXbottom; x++)
					Mem2Bof[x] = 0x00FFFFFF;
				hdc = GetDC(hwnd);
				BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
				if (showalphabet)
					ShowAlphabet();
				ReleaseDC(hwnd, hdc);
				SendMessage(hwndColor, WM_KEYDOWN, VK_ESCAPE, 0);
				GetColor();
				changecolor = 0;
				Menu();
			}
			else
				SetFocus(hwndColor);
		}
		else  if (((HWND)(LPARAM)lParam == hwndButton7)) {//FILL
			if (Mem3Buf) {
				VirtualFree(Mem3Buf, 0, MEM_RELEASE);
				Mem3Buf = NULL;
				DeleteObject(hMem3Bitmap);
			}
			if (hFillBrush)
				DeleteObject(hFillBrush);
			hFillBrush = CreateSolidBrush((Blue << 16) | (Green << 8) | Red);
			FillRect(hdcMem2, &rect, hFillBrush);
			for (x = 0; x < rightXbottom; x++)
				Mem2Bof[x] = (Red << 16) | (Green << 8) | Blue;
			SelectObject(hdcMem2, OldhMem2Bitmap);
			SetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, Mem2Buf, &bmi, DIB_RGB_COLORS);
			SelectObject(hdcMem2, hMem2Bitmap);
			hdc = GetDC(hwnd);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
			if (showalphabet)
				ShowAlphabet();
			ReleaseDC(hwnd, hdc);
			////////////
			SaveScreen();
			////////////
			SendMessage(hwndColor, WM_KEYDOWN, VK_ESCAPE, 0);
			GetColor();
			changecolor = 0;
			Menu();
		}
		else  if (((HWND)(LPARAM)lParam == hwndButton8)) {//ABCs
			if (showalphabet) {
				showalphabet = FALSE;
				ABCs[0] = 'A';
				ABCs[1] = 'B';
				ABCs[2] = 'C';
				ABCs[3] = 's';

			}
			else {
				showalphabet = TRUE;
				AlphabetColor = (Blue << 16) | (Green << 8) | Red;
				ABCs[0] = ' ';
				ABCs[1] = ' ';
				ABCs[2] = ' ';
				ABCs[3] = ' ';
			}
			SendMessage(hwndColor, WM_KEYDOWN, VK_ESCAPE, 0);
			GetColor();
			changecolor = 0;
			Menu();
		}
		else if (((HWND)(LPARAM)lParam == hwndButton9)) {//Change This Color
			SendMessage(hwndColor, WM_KEYDOWN, VK_ESCAPE, 0);
			GetColor();
			changecolor = 1;
			checkcolor = TRUE;
			Menu();
		}
	}
	return CallWindowProc(pMenuProc, hwnd2, message, wParam, lParam);
}

void Menu(void)
{
	inmenu = TRUE;
	hwndMenu = CreateWindow("STATIC", NULL,
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME,
		0, 0, rect.right-140, 30,
		hwnd, (HMENU)189, hInstance, NULL);
	pMenuProc = (WNDPROC)SetWindowLong(hwndMenu, GWL_WNDPROC, (LONG)MenuProc);
	SetWindowText(hwndMenu, "SimplePaint");
	hwndButton1 = CreateWindow("BUTTON", "&READ!",
		WS_CHILD | WS_VISIBLE,
		rect.right-140-50-10, 2, 50, 20,
		hwndMenu, NULL, hInstance, NULL);
	hwndButton3 = CreateWindow("BUTTON", "&Exit",
		WS_CHILD | WS_VISIBLE,
		rect.right-140-420, 2, 50, 20,
		hwndMenu, NULL, hInstance, NULL);
	hwndButton4 = CreateWindow("BUTTON", "&Open",
		WS_CHILD | WS_VISIBLE,
		rect.right-140-360, 2, 50, 20,
		hwndMenu, NULL, hInstance, NULL);
	hwndButton5 = CreateWindow("BUTTON", "&Save",
		WS_CHILD | WS_VISIBLE,
		rect.right-140-300, 2, 50, 20,
		hwndMenu, NULL, hInstance, NULL);
	hwndButton6 = CreateWindow("BUTTON", "&Clear",
		WS_CHILD | WS_VISIBLE,
		rect.right-140-240, 2, 50, 20,
		hwndMenu, NULL, hInstance, NULL);
	hwndButton7 = CreateWindow("BUTTON", "&Fill",
		WS_CHILD | WS_VISIBLE,
		rect.right-140-180, 2, 50, 20,
		hwndMenu, NULL, hInstance, NULL);
	if (ALPHABET) {
		hwndButton8 = CreateWindow("BUTTON", ABCs,
			WS_CHILD | WS_VISIBLE,
			rect.right-140-120, 2, 50, 20,
			hwndMenu, NULL, hInstance, NULL);
	}
	if (changecolor == 2) {
		hwndButton9 = CreateWindow("BUTTON", "Change This Color",
			WS_CHILD | WS_VISIBLE,
			rect.right-140-600, 2, 140, 20,
			hwndMenu, NULL, hInstance, NULL);
	}
	else if (changecolor == 1) {
		hwndButton9 = CreateWindow("BUTTON", "COLOR WILL CHANGE",
			WS_CHILD | WS_VISIBLE,
			rect.right-140-600, 2, 160, 20,
			hwndMenu, NULL, hInstance, NULL);
	}
}

//*****************************************************************************
LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_SIZE:
		rect.left = rect.top = 0;
		rect.right = LOWORD(lParam);
		rect.bottom = HIWORD(lParam);
		rightXbottom = rect.right*rect.bottom;
		rightXbottomX4 = rightXbottom*4;
		screenLen = compressBound(rightXbottomX4);
		SavedScreenLen = screenLen;
		screen = (BYTE*)VirtualAlloc(NULL, screenLen, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		LeftOfColors = rect.right-140;
		CenterX = LeftOfColors+70;
		CenterY = rect.bottom-70;
		if (wintab == 0)
			lcMine.lcOutExtY = -rect.bottom;
		break;

	case WM_CREATE:
		wintab = WTInfo(0, 0, NULL);
///
		pkt.pkOrientation.orAltitude = 0;
///
		GetCurrentDirectory(MAX_PATH, CurrentDir);
		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.hwndOwner         = hwnd;
		ofn.hInstance         = NULL;
		ofn.lpstrFilter       = " *.bmp.gz\0*.bmp.gz\0\0";
		ofn.lpstrFile         = FullFilename;
		ofn.lpstrFileTitle    = Filename;
		ofn.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
		ofn.lpstrTitle        = NULL;
		ofn.lpstrDefExt       = "bmp.gz";
		ofn.nMaxFile          = MAX_PATH;
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter    = 0;
		ofn.nFilterIndex      = 0;
		ofn.nMaxFileTitle     = MAX_PATH;
		ofn.lpstrInitialDir   = CurrentDir;
		ofn.nFileOffset       = 0;
		ofn.nFileExtension    = 0;
		ofn.lCustData         = 0;
		ofn.lpfnHook          = NULL;
		ofn.lpTemplateName    = NULL;

		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 3;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x22;
		for (x = 0; ArialRounded[x] != 0; x++)
			lf.lfFaceName[x] = ArialRounded[x];
		lf.lfFaceName[x] = 0;

		MemBuf = NULL;
		Mem3Buf = NULL;
		if (wintab) {
			WTInfo(WTI_DEFSYSCTX, 0, &lcMine);
			lcMine.lcOptions |= CXO_MESSAGES;
			lcMine.lcPktData = PACKETDATA;
			lcMine.lcPktMode = PACKETMODE;
			lcMine.lcMoveMask = PACKETDATA;
			lcMine.lcBtnUpMask = lcMine.lcBtnDnMask;
			lcMine.lcOutOrgX = 0;
			lcMine.lcOutExtX = GetSystemMetrics(SM_CXSCREEN);
			lcMine.lcOutOrgY = 0;
			lcMine.lcOutExtY = -GetSystemMetrics(SM_CYSCREEN);
			hTab = WTOpen(hwnd, &lcMine, TRUE);
			WTGet(hTab, &lc);
			WTInfo(WTI_DEVICES + lc.lcDevice, DVC_NPRESSURE, &np);//for pressure
		}
		a = blend/255.0;//for Microsoft's AlphaBlend
		b = 1.0 - a;
		pkt.pkX = pkt.pkY = -1;
		return 0;

	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE) {
			if (gettingsomething)
				SendMessage(hwndColor, WM_KEYDOWN, VK_ESCAPE, 0);
			if (magnifying)
				SendMessage(hwnd, WM_SYSKEYDOWN, 0, 0);
		}
		if (hTab && GET_WM_ACTIVATE_STATE(wParam, lParam))
			WTOverlap(hTab, TRUE);
		break;

	case WM_KEYUP:
		if (wParam == 0x41) { // 'A' key
			keyup = TRUE;
			if (straightline) {
				EndStraightLine();
				PenUp();
			}
		}
		return 0;

	case WM_KEYDOWN:
		if ((wParam == 0x41) && (keyup)) { // 'A' key
			if ((straightline == FALSE) && (pkt.pkX != -1)) {
				straightline = TRUE;
				keyup = FALSE;
				ButtonDownX = -1;
			}
		}

		else if (pendown == FALSE) {
			if ((wParam == VK_DELETE) && (!magnifying)) {
				if (c) {
					c--;
					DeleteOrUndelete();
				}
			}
			else if ((wParam == VK_BACK) && (!magnifying)) {
				if (c < maxc) {
					c++;
					DeleteOrUndelete();
				}
			}
			else if (wParam == VK_CONTROL) {//EpressKey designated as Modifier
				magnifying = FALSE;
				InvalidateRect(hwnd, &rect, FALSE);
				GetColor();
				changecolor = 0;
				Menu();
			}
			else if ((wParam == VK_SHIFT)  && (!gettingsomething) && (pkt.pkX != -1)) {
				hdc = GetDC(hwnd);
				Color = GetPixel(hdc, pkt.pkX, pkt.pkY);
				ReleaseDC(hwnd, hdc);
				if (Color != 0xFFFFFF) {
					Red = Color & 0xFF;
					Green = (Color & 0xFF00) >> 8;
					Blue = (Color & 0xFF0000) >> 16;
					GetColor();
					ChangeColor = (Red << 16) | (Green << 8) | Blue;
					changecolor = 2;
					Menu();
				}
			}
			else if (wParam == VK_INSERT) {
				if (pkt.pkX != -1) {
					hdc = GetDC(hwnd);
					BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
					if (showalphabet)
						ShowAlphabet();

					if (magnifying == FALSE) {
						magnifying = TRUE;
						changed = FALSE;
						hMagPen = CreatePen(PS_SOLID, 0, 0xE0E0E0);
						SelectObject(hdc, hMagPen);
						BoxL = pkt.pkX-300;
						BoxT = pkt.pkY-300;
						BoxR = pkt.pkX+300;
						BoxB = pkt.pkY+300;
						boxl = pkt.pkX-30;
						boxt = pkt.pkY-30;
						for (y = BoxT, Y = boxt; y < BoxB; y += 10, Y++) {
							for (x = BoxL, X = boxl; x < BoxR; x += 10, X++) {
								MagColor = GetPixel(hdcMem2, X, Y);
								hMagBrush = CreateSolidBrush(MagColor);
								hOldBrush = SelectObject(hdc, hMagBrush);
								Rectangle(hdc, x, y, x+10, y+10);
								SelectObject(hdc, hOldBrush);
								DeleteObject(hMagBrush);
							}
						}
						DeleteObject(hMagPen);
					}
					else {//if (magnifying == TRUE)
						magnifying = FALSE;
						if (changed) {
							hdc = GetDC(hwnd);
							if (Mem3Buf == NULL) {
								Mem3Buf = (BYTE*)VirtualAlloc(NULL, bmih.biSizeImage, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
								Mem3Bof = (DWORD*)Mem3Buf;
								hdcMem3 = CreateCompatibleDC(hdc);
								hMem3Bitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
								OldhMem3Bitmap = SelectObject(hdcMem3, hMem3Bitmap);
								SelectObject(hdcMem2, OldhMem2Bitmap);
							}
							BitBlt(hdcMem3, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
							SelectObject(hdcMem2, OldhMem2Bitmap);//deselect hMemBitmap
							GetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, Mem2Buf, &bmi, DIB_RGB_COLORS);
							SelectObject(hdcMem2, hMem2Bitmap);
							////////////
							SaveScreen();
							////////////
							ReleaseDC(hwnd, hdc);
						}
					}
				}
			}
			else if (wParam == VK_ESCAPE) {
				if (!magnifying)
					QuitSaveCancel();
				else
					SendMessage(hwnd, WM_KEYDOWN, VK_INSERT, 0);
			}
			else if ((wParam == VK_RETURN) || (wParam == VK_F1)) {
				ShowHelp();
			}
		}
		return 0;
/*
	case WM_RBUTTONDOWN://erase to white
		pkt.pkOrientation.orAltitude = 0x80000000;
		oldRed = Red;
		oldGreen = Green;
		oldBlue = Blue;
		oldBlend = Blend;
		Red = 0xFF;
		Green = 0xFF;
		Blue = 0xFE;
		Blend = 0xFF;
		a = 1.0;
		b = 0.0;
	case WM_LBUTTONDOWN:
		pkt.pkX = LOWORD(lParam);
		pkt.pkY = HIWORD(lParam);
		pendown = TRUE;
		ptr = 0;
		usingmouse = TRUE;
		if (magnifying)
			Magnify();
		if (!gettingsomething) 
			FillRect(hdcMem, &rect, hWhiteBrush);//so only next brush stroke will be there
		else//if (gettingsomething)
			PreGetColor();
		break;

	case WM_RBUTTONUP:
		pkt.pkOrientation.orAltitude = 0;
		Red = oldRed;
		Green = oldGreen;
		Blue = oldBlue;
		Blend = oldBlend;
		blend = (double)Blend;
		a = blend/255.0;//for Microsoft's AlphaBlend
		b = 1.0 - a;
	case WM_LBUTTONUP:
		ptr = 0;
		usingmouse = FALSE;
		PenUp();
		break;

	case WM_MOUSEMOVE:
		pkt.pkX = LOWORD(lParam);
		pkt.pkY = HIWORD(lParam);
		if ((wParam == MK_LBUTTON) || (wParam == MK_RBUTTON)) {
			if (magnifying)
				Magnify();
			if (!gettingsomething)
				InvalidateRect(hwnd, &rect, FALSE);
			else//if (gettingsomething)
				PreGetColor();
		}
		break;
*/
	case WT_PACKET:
		pktOld = pkt;
		if (WTPacket((HCTX)lParam, wParam, &pkt)) {

			if ((!openingfile) && (!help)) {
				if ((pkt.pkX != pktOld.pkX) || (pkt.pkY != pktOld.pkY))
					newdot = TRUE;
				else
					newdot = FALSE;

				if ((HIWORD(pkt.pkButtons) == TBN_DOWN) && (LOWORD(pkt.pkButtons) == 0)) {
					pendown = TRUE;
					ptr = 0;
					FillRect(hdcMem, &rect, hWhiteBrush);//so only next brush stroke will be there
///
					if (pkt.pkOrientation.orAltitude & 0x80000000) {//using eraser
						if ((Red != 0xFF) && (Green != 0xFF) && (Blue != 0xFE) && (Blue != 0xFF)) {
							oldRed = Red;
							oldGreen = Green;
							oldBlue = Blue;
							oldBlend = Blend;
							Red = 0xFF;
							Green = 0xFF;
							Blue = 0xFE;
							Blend = 0xFF;
							a = 1.0;
							b = 0.0;
						}
					}
///
					else if ((oldRed != 0xFF) && (oldGreen!= 0xFF) && (oldBlue != 0xFF)) { //else
						Red = oldRed;
						Green = oldGreen;
						Blue = oldBlue;
						Blend = oldBlend;
						blend = (double)Blend;
						a = blend/255.0;//for Microsoft's AlphaBlend
						b = 1.0 - a;
						oldRed = 0xFF;
						oldGreen = 0xFF;
						oldBlue = 0xFF;
					}
				}

				else if ((HIWORD(pkt.pkButtons) == TBN_UP) && (LOWORD(pkt.pkButtons) == 0)) {// raised pen off tablet
					pendown = FALSE;
					if (straightline)
						EndStraightLine();
					PenUp();
				}

				else if ((newdot) && (pendown)) {
					if (magnifying) {
						Magnify();
					}
					else if (!gettingsomething) {
						InvalidateRect(hwnd, &rect, FALSE);//draw brush-stroke
						UpdateWindow(hwnd);
					}
					else if ((gettingsomething) && ((pkt.pkOrientation.orAltitude & 0x80000000) == 0) && (pkt.pkX >= LeftOfColors)) {
///				else if ((gettingsomething) && (pkt.pkX >= LeftOfColors)) {
						PreGetColor();
					}//end of if (gettingsomething)
				}
/*
				else if ((inmenu == FALSE) && (!pendown)) {// doesn't work!
					hdc = GetDC(hwnd);
					BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
					hOldPen = SelectObject(hdc, hPen);
					MoveToEx(hdc, pkt.pkX, pkt.pkY, NULL);
					LineTo(hdc, pkt.pkX+10, pkt.pkY+10);
					SelectObject(hdc, hOldPen);
					ReleaseDC(hwnd, hdc);
				}
*/
			}
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (first) {
			BYTE *Buf;

			first = FALSE;
			bmih.biSize = sizeof(BITMAPINFOHEADER);
			bmih.biWidth = rect.right;
			bmih.biHeight = rect.bottom;
			bmih.biPlanes = 1;
			bmih.biBitCount = 32;//4 bytes/pixel
			bmih.biCompression = BI_RGB;
			bmih.biSizeImage = 4 * rect.right * rect.bottom;
			bmi.bmiHeader = bmih;
			Buf = (BYTE*)VirtualAlloc(NULL, bmih.biSizeImage, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			hBitmap = CreateDIBitmap(hdc, &bmih, CBM_INIT, Buf, &bmi, DIB_RGB_COLORS);
			SelectObject(hdc, hBitmap);
			VirtualFree(Buf, 0, MEM_RELEASE);
			MemBuf = (BYTE*)VirtualAlloc(NULL, bmih.biSizeImage, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			MemBof = (DWORD*)MemBuf;
			Mem2Buf = (BYTE*)VirtualAlloc(NULL, bmih.biSizeImage, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			Mem2Bof = (DWORD*)Mem2Buf;
			hdcMem = CreateCompatibleDC(hdc);
			hMemBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			OldhMemBitmap = SelectObject(hdcMem, hMemBitmap);
			hdcMem2 = CreateCompatibleDC(hdc);
			hMem2Bitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			OldhMem2Bitmap = SelectObject(hdcMem2, hMem2Bitmap);

			if (Filename[0]) {//from command line ("Open With")
				hFile = CreateFile(Filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					fileSize = GetFileSize(hFile, NULL);
					if (fileSize) {
						BYTE Header[54];

						ReadFile(hFile, Header, 54, &dwBytesRead, NULL);
						if ((*(WORD*)&Header[0] == 0x4D42) || (*(WORD*)&Header[0] == 0x8B1F)) {//bitmap or zlib file
							if (Mem3Buf == NULL) {
								Mem3Buf = (BYTE*)VirtualAlloc(NULL, bmih.biSizeImage, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
								Mem3Bof = (DWORD*)Mem3Buf;
								hdcMem3 = CreateCompatibleDC(hdc);
								hMem3Bitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
								OldhMem3Bitmap = SelectObject(hdcMem3, hMem3Bitmap);
							}
							if (*(WORD*)&Header[0] == 0x4D42)//"BM"
								ReadFile(hFile, Mem3Buf, bmih.biSizeImage, &dwBytesRead, NULL);
							else if (*(WORD*)&Header[0] == 0x8B1F) {//zlib file
								CloseHandle(hFile);//necessary
								hFile = NULL;
								hGZFile = gzopen(Filename, "rb");
								gzseek(hGZFile, 54, SEEK_SET);
								gzread(hGZFile, Mem3Buf, rightXbottomX4);
								gzclose(hGZFile);
							}
							SelectObject(hdcMem3, OldhMem3Bitmap);
							SetDIBits(hdcMem3, hMem3Bitmap, 0, rect.bottom, Mem3Buf, &bmi, DIB_RGB_COLORS);
							SelectObject(hdcMem3, hMem3Bitmap);
							BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem3, 0, 0, SRCCOPY);
							if (showalphabet)
								ShowAlphabet();
							BitBlt(hdcMem2, 0, 0, rect.right, rect.bottom, hdcMem3, 0, 0, SRCCOPY);
							SelectObject(hdcMem2, OldhMem2Bitmap);//deselect hMemBitmap
							GetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, Mem2Buf, &bmi, DIB_RGB_COLORS);
							SelectObject(hdcMem2, hMem2Bitmap);
							////////////
							SaveScreen();
							////////////
						}
					}
					if (hFile)
						CloseHandle(hFile);
				}
			}
			else {
				FillRect(hdcMem2, &rect, hWhiteBrush);
				for (x = 0; x < rightXbottom; x++)
					Mem2Bof[x] = 0x00FFFFFF;//necessary
			}
			GetColor();
			changecolor = 0;
			Menu();
		}//end of if (first)

		else if (pendown) {
			if (ptr < MAXPOINTS) {
				if ((Radius > 2) && (!straightline) && (!endstraightline))//(!usingmouse))
					radius = MulDiv(pkt.pkNormalPressure, Radius, np.axMax);
				else
					radius = Radius;
				points[ptr].radius = radius;
				points[ptr].x = (WORD)pkt.pkX;
				points[ptr].y = (WORD)pkt.pkY;
				hPen = CreatePen(PS_SOLID, radius*2, (Blue << 16) | (Green << 8) | (Red));//reversed color order
				hOldPen = SelectObject(hdc, hPen);
				hOldPen = SelectObject(hdcMem, hPen);
				if (straightline) {
					if (ButtonDownX == -1) {
						ButtonDownX = points[ptr].x;
						ButtonDownY = points[ptr].y;
					}
					BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
					MoveToEx(hdc, ButtonDownX, ButtonDownY, NULL);//to show instantly
					LineTo(hdc, points[ptr].x, points[ptr].y);
				}
				else if (endstraightline) {
					endstraightline = FALSE;
					straightline = FALSE;
					MoveToEx(hdcMem, ButtonDownX, ButtonDownY, NULL);
					LineTo(hdcMem, pkt.pkX, pkt.pkY);
					MoveToEx(hdc, ButtonDownX, ButtonDownY, NULL);//to show instantly
					LineTo(hdc, pkt.pkX, pkt.pkY);
				}
				else if (ptr) {
					MoveToEx(hdc, points[ptr-1].x, points[ptr-1].y, NULL);//to show instantly
					LineTo(hdc, points[ptr].x, points[ptr].y);
					MoveToEx(hdcMem, points[ptr-1].x, points[ptr-1].y, NULL);//to show after brushstroke ends (with alpha-blending)
					LineTo(hdcMem, points[ptr].x, points[ptr].y);
				}
				ptr++;
				SelectObject(hdc, hOldPen);
				SelectObject(hdcMem, hOldPen);
				DeleteObject(hPen);
			}
		}
		else {// if (!pendown)
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
		}
		if (showalphabet)
			ShowAlphabet();
		if (magnifying) {
			hMagPen = CreatePen(PS_SOLID, 0, 0xE0E0E0);
			SelectObject(hdc, hMagPen);
			for (y = BoxT, Y = boxt; y < BoxB; y += 10, Y++) {
				for (x = BoxL, X = boxl; x < BoxR; x += 10, X++) {
					MagColor = GetPixel(hdcMem2, X, Y);
					hMagBrush = CreateSolidBrush(MagColor);
					hOldBrush = SelectObject(hdc, hMagBrush);
					Rectangle(hdc, x, y, x+10, y+10);
					SelectObject(hdc, hOldBrush);
					DeleteObject(hMagBrush);
				}
			}
			DeleteObject(hMagPen);
		}
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		DeleteObject(hWhiteBrush);
		DeleteObject(hBlackBrush);
		if (MemBuf) {
			VirtualFree(MemBuf, 0, MEM_RELEASE);
			VirtualFree(Mem2Buf, 0, MEM_RELEASE);
			DeleteObject(hBitmap);
			DeleteObject(hMemBitmap);
			DeleteObject(OldhMemBitmap);
			DeleteObject(hMem2Bitmap);
			DeleteObject(OldhMem2Bitmap);
		}
		if (Mem3Buf) {
			VirtualFree(Mem3Buf, 0, MEM_RELEASE);
			DeleteObject(hMem3Bitmap);
		}
		for (x = 0; x < maxc; x++)
			VirtualFree(Screen[x], 0, MEM_RELEASE);
		VirtualFree(screen, 0, MEM_RELEASE);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
//*****************************************************************************

void DeleteOrUndelete(void)
{
	if (c >= 1) {
		//////////
		uncompress(Mem2Buf, &rightXbottomX4, Screen[c-1], Added[c-1]);
		//////////
		SelectObject(hdcMem2, OldhMem2Bitmap);
		SetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, Mem2Buf, &bmi, DIB_RGB_COLORS);
		SelectObject(hdcMem2, hMem2Bitmap);
	}
	else {
		if (Mem3Buf) {
			VirtualFree(Mem3Buf, 0, MEM_RELEASE);
			Mem3Buf = NULL;
			DeleteObject(hMem3Bitmap);
		}
		if (hFillBrush) {
			DeleteObject(hFillBrush);
			hFillBrush = NULL;
		}
		FillRect(hdcMem2, &rect, hWhiteBrush);
		for (x = 0; x < rightXbottom; x++)
			Mem2Bof[x] = 0x00FFFFFF;
	}		
	hdc = GetDC(hwnd);
	BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
	if (showalphabet)
		ShowAlphabet();
	ReleaseDC(hwnd, hdc);
}

void GetDateTime(void)
{
	GetLocalTime(&st);
	FullFilename[0] = (st.wYear / 1000) + '0';
	FullFilename[1] = ((st.wYear % 1000) / 100) + '0';
	FullFilename[2] = ((st.wYear %100) / 10) + '0';
	FullFilename[3] = (st.wYear % 10) + '0';
	FullFilename[4] = '-';
	FullFilename[5] = (st.wMonth / 10) + '0';
	FullFilename[6] = (st.wMonth % 10) + '0';
	FullFilename[7] = '-';
	FullFilename[8] = (st.wDay / 10) + '0';
	FullFilename[9] = (st.wDay % 10) + '0';
	FullFilename[10] = ' ';
	FullFilename[11] = ' ';
	FullFilename[12] = (st.wHour / 10) + '0';
	FullFilename[13] = (st.wHour % 10) + '0';
	FullFilename[14] = '-';
	FullFilename[15] = (st.wMinute / 10) + '0';
	FullFilename[16] = (st.wMinute % 10) + '0';
	FullFilename[17] = '-';
	FullFilename[18] = (st.wSecond / 10) + '0';
	FullFilename[19] = (st.wSecond % 10) + '0';
	FullFilename[20] = '.';
	FullFilename[21] = 'b';
	FullFilename[22] = 'm';
	FullFilename[23] = 'p';
	FullFilename[24] = '.';
	FullFilename[25] = 'g';
	FullFilename[26] = 'z';
	FullFilename[27] = 0;
}

LRESULT MessageProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_COMMAND) {
		if ((HWND)(LPARAM)lParam == hwndNo)
			if (fromsave == FALSE)
				DestroyWindow(hwnd);
			else
				DestroyWindow(hwndMessage);
		else if ((HWND)(LPARAM)lParam == hwndYes) {
			hGZFile = gzopen(FullFilename, "wb");
			gzseek(hGZFile, 0, SEEK_SET);
			gzwrite(hGZFile, SaveBuf, bmfh.bfSize);
			gzclose(hGZFile);
			VirtualFree(SaveBuf, 0, MEM_RELEASE);
			if (fromsave == FALSE)
				DestroyWindow(hwnd);
			else
				DestroyWindow(hwndMessage);
		}
	}
	else if (message == WM_KEYDOWN) {
		if ((wParam == VK_ESCAPE) || (wParam == 'N'))
			if (fromsave == FALSE)
				DestroyWindow(hwnd);
			else
				DestroyWindow(hwndMessage);
		else if ((wParam == VK_RETURN)  || (wParam == 'Y')) {
			hGZFile = gzopen(FullFilename, "wb");
			gzseek(hGZFile, 0, SEEK_SET);
			gzwrite(hGZFile, Mem2Buf, rightXbottomX4);
			gzclose(hGZFile);
			VirtualFree(SaveBuf, 0, MEM_RELEASE);
			if (fromsave == FALSE)
				DestroyWindow(hwnd);
			else
				DestroyWindow(hwndMessage);
		}
	}
	else if (message == WM_PAINT) {
		HDC hdc;
		PAINTSTRUCT ps;
		RECT rect;
		HBRUSH hGrayBrush;

		hdc = BeginPaint(hwnd2, &ps);
		rect.left = rect.top = 0;
		rect.right = ps.rcPaint.right;
		rect.bottom = ps.rcPaint.bottom;
		hGrayBrush = CreateSolidBrush(0xD8E9EC);//MessageBox color
		FillRect(hdc, &rect, hGrayBrush);
		DeleteObject(hGrayBrush);
		SetBkMode(hdc, TRANSPARENT);
		TextOut(hdc, 20, 10, FullFilename, 27);
		EndPaint(hwnd2, &ps);
	}
	else if (message == WM_DESTROY) {
		fromsave = FALSE;
		SetFocus(hwndColor);
	}
	return CallWindowProc(pMessageProc, hwnd2, message, wParam, lParam);
}

void QuitSaveCancel(void)
{
	if (fromsave == FALSE) {
		for (x = 0; x < rightXbottom; x++) {
			if (Mem2Bof[x] == 0x00FFFFFE)
				Mem2Bof[x] = 0x00FFFFFF;
			if (Mem2Bof[x] != 0x00FFFFFF)
				break;
		}
	}
	if (x != rightXbottom) {//if the screen wasn't entirely erased
		bmih.biSize = sizeof(BITMAPINFOHEADER);
		bmih.biWidth = rect.right;
		bmih.biHeight = rect.bottom;
		bmih.biPlanes = 1;
		bmih.biBitCount = 32;//4 bytes/pixel
		bmih.biCompression = BI_RGB;
		bmih.biSizeImage = rightXbottomX4;
		bmi.bmiHeader = bmih;

		bmfh.bfType = 0x4D42;//"BM"
		bmfh.bfOffBits = 54;
		bmfh.bfSize = bmfh.bfOffBits + bmih.biSizeImage;

		SaveBuf = (BYTE*)VirtualAlloc(NULL, bmfh.bfSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		*(WORD*)&SaveBuf[0] = bmfh.bfType;
		*(DWORD*)&SaveBuf[2] = bmfh.bfSize;
		*(WORD*)&SaveBuf[6] = 0;
		*(WORD*)&SaveBuf[8] = 0;
		*(DWORD*)&SaveBuf[10] = bmfh.bfOffBits;
		*(DWORD*)&SaveBuf[14] = bmih.biSize;
		*(LONG*)&SaveBuf[18] = bmih.biWidth;
		*(LONG*)&SaveBuf[22] = bmih.biHeight;
		*(WORD*)&SaveBuf[26] = bmih.biPlanes;
		*(WORD*)&SaveBuf[28] = bmih.biBitCount;
		*(DWORD*)&SaveBuf[30] = bmih.biCompression;
		*(DWORD*)&SaveBuf[34] = bmih.biSizeImage;
		*(LONG*)&SaveBuf[38] = 0;//bmih.biXPelsPerMeter
		*(LONG*)&SaveBuf[42] = 0;//bmih.biYPelsPerMeter
		*(DWORD*)&SaveBuf[46] = 0;//bmih.biClrUsed
		*(DWORD*)&SaveBuf[50] = 0;//bmih.biClrImportant
		SelectObject(hdcMem2, OldhMem2Bitmap);
		GetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, &SaveBuf[54], &bmi, DIB_RGB_COLORS);
		SelectObject(hdcMem2, hMem2Bitmap);

//		if (Filename[0] == 0)
//			hFile = CreateFile("SimplePaint.bmp", GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
//		else
//			hFile = CreateFile(Filename, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, 0, NULL);
//		WriteFile(hFile, SaveBuf, bmfh.bfSize, &dwBytesWritten, NULL);
//		CloseHandle(hFile);
		GetDateTime();
		hwndMessage = CreateWindow("EDIT", "  SAVE as Current Date/Time:",
			WS_CHILD | WS_VISIBLE | WS_CAPTION | WS_SYSMENU,
			rect.right-140-220, 30, 220, 100,
			hwnd, (HMENU)190, hInstance, NULL);
		pMessageProc = (WNDPROC)SetWindowLong(hwndMessage, GWL_WNDPROC, (LONG)MessageProc);
		SetFocus(hwndMessage);
		hwndYes = CreateWindow("BUTTON", "&YES (Enter)",
			WS_CHILD | WS_VISIBLE,
			10, 40, 90, 20,
			hwndMessage, NULL, hInstance, NULL);
		hwndNo = CreateWindow("BUTTON", "&NO (Esc)",
			WS_CHILD | WS_VISIBLE,
			120, 40, 90, 20,
			hwndMessage, NULL, hInstance, NULL);
	}
	else if (fromsave == FALSE)
		DestroyWindow(hwnd);
	else
		DestroyWindow(hwndMessage);
}

void ShowAlphabet(void)
{
	lf.lfHeight = -72;
	hFont = CreateFontIndirect(&lf);
	hOldFont = SelectObject (hdc, hFont);
	GetTextExtentPoint32(hdc, Alphabet, 26, &size);//put entry horiz & vert pixels in size
	if (size.cx > rect.right) {
		lf.lfHeight = -48;
		SelectObject(hdc, hOldFont);
		hFont = CreateFontIndirect(&lf);
		hOldFont = SelectObject (hdc, hFont);
	}
	SetTextColor(hdc, AlphabetColor);//(Blue << 16) | (Green << 8) | Red
	SetBkMode(hdc, TRANSPARENT);
	BeginPath(hdc);
	TextOut(hdc, 0, 0, Alphabet, 26);
	EndPath(hdc);
	StrokePath(hdc);
	SetTextColor(hdc, 0);
	SetBkMode(hdc, OPAQUE);
	SelectObject(hdc, hOldFont);
}

void OpenSavedFile(void)
{
	hdc = GetDC(hwnd);
	if (Mem3Buf == NULL) {
		Mem3Buf = (BYTE*)VirtualAlloc(NULL, rightXbottomX4, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		Mem3Bof = (DWORD*)Mem3Buf;
		hdcMem3 = CreateCompatibleDC(hdc);
		hMem3Bitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
		OldhMem3Bitmap = SelectObject(hdcMem3, hMem3Bitmap);
	}
	hGZFile = gzopen(Filename, "rb");
	gzseek(hGZFile, 54, SEEK_SET);
	gzread(hGZFile, Mem3Buf, rightXbottomX4);
	gzclose(hGZFile);
	Filename[0] = 0;//for QuitSaveCancel
	SelectObject(hdcMem3, OldhMem3Bitmap);
	SetDIBits(hdcMem3, hMem3Bitmap, 0, rect.bottom, Mem3Buf, &bmi, DIB_RGB_COLORS);
	SelectObject(hdcMem3, hMem3Bitmap);
	BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem3, 0, 0, SRCCOPY);
	if (showalphabet)
		ShowAlphabet();
	BitBlt(hdcMem2, 0, 0, rect.right, rect.bottom, hdcMem3, 0, 0, SRCCOPY);
	SelectObject(hdcMem2, OldhMem2Bitmap);
	GetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, Mem2Buf, &bmi, DIB_RGB_COLORS);
	SelectObject(hdcMem2, hMem2Bitmap);
	ReleaseDC(hwnd, hdc);
	////////////
	SaveScreen();
	////////////
	SendMessage(hwndColor, WM_KEYDOWN, VK_ESCAPE, 0);
	GetColor();
	changecolor = 0;
	Menu();
}

void SaveScreen(void)
{
	screenLen = SavedScreenLen;
	compress(screen, &screenLen, Mem2Buf, rightXbottomX4);
	Screen[c] = (BYTE*)VirtualAlloc(NULL, screenLen, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	Added[c] = screenLen;
	memcpy(Screen[c], screen, screenLen);
	c++;
	maxc = c;
}

void PreGetColor(void)
{
	if ((pkt.pkY > 52) && (pkt.pkY <= 563)) {
		if ((inred == FALSE) && (ingreen ==	FALSE) && (inblue == FALSE) && (inopaque == FALSE)) {
			if ((pkt.pkX >= LeftOfColors+3) && (pkt.pkX < LeftOfColors+27))
				inred = TRUE;
			else if ((pkt.pkX >= LeftOfColors+38) && (pkt.pkX < LeftOfColors+62))
				ingreen = TRUE;
			else if ((pkt.pkX >= LeftOfColors+73) && (pkt.pkX < LeftOfColors+97))
				inblue = TRUE;
			else if ((pkt.pkX >= LeftOfColors+105) && (pkt.pkX < LeftOfColors+135))
				inopaque = TRUE;
		}
		if ((inred)) {
			yRedPos = pkt.pkY;
			xRedPos = 15;
			Red = (563-yRedPos)/2;
		}
		else if ((ingreen)) {
			yGreenPos = pkt.pkY;
			xGreenPos = 50;
			Green = (563-yGreenPos)/2;
		}
		else if ((inblue)) {
			yBluePos = pkt.pkY;
			xBluePos = 85;
			Blue = (563-yBluePos)/2;
		}
		else if ((inopaque)) {
			Blend = (563-pkt.pkY)/2;
		}
		else
			return;
		InvalidateRect(hwndColor, &rect, FALSE);
	}
	else if ((pkt.pkY > (rect.bottom-140))) {
		incircle = TRUE;
		OldRadius = Radius;
		Radius = (int)sqrt((abs((CenterX-pkt.pkX)*(CenterX-pkt.pkX))) + (abs((CenterY-pkt.pkY)*(CenterY-pkt.pkY))));
		if (Radius > 70)
			Radius = 70;
		InvalidateRect(hwndColor, &rect, FALSE);
	}
}

void PenUp(void)
{
	inred = ingreen = inblue = inopaque = incircle = FALSE;
	if (gettingsomething)
		InvalidateRect(hwndColor, &rect, FALSE);
	else if ((!gettingsomething) && (!magnifying)){
		SelectObject(hdcMem, OldhMemBitmap);
		SelectObject(hdcMem2, OldhMem2Bitmap);
		GetDIBits(hdcMem, hMemBitmap, 0, rect.bottom, MemBuf, &bmi, DIB_RGB_COLORS);
		GetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, Mem2Buf, &bmi, DIB_RGB_COLORS);

		for (x = 0; x < rightXbottom; x++) {
			SrcColor = MemBof[x];
			if (SrcColor != 0xFFFFFF) {
				if (SrcColor == 0xFFFFFE)//Blue down one
					SrcColor = 0xFFFFFF;//tricky, but the only way to erase to 0xFFFFFF
				DestColor = Mem2Bof[x];
				if  ((DestColor != SrcColor) && (DestColor != 0xFFFFFF) && (blend != 255)) {
					SrcRed = (double)((SrcColor >> 16) & 0xFF);
					SrcGreen = (double)((SrcColor >> 8) & 0xFF);
					SrcBlue = (double)(SrcColor & 0xFF);
					DestRed = (double)((DestColor >> 16) & 0xFF);
					DestGreen = (double)((DestColor >> 8) & 0xFF);
					DestBlue = (double)(DestColor & 0xFF);
					red = (BYTE)((SrcRed * a) + (DestRed * b));
					green = (BYTE)((SrcGreen * a) + (DestGreen * b));
					blue = (BYTE)((SrcBlue * a) + (DestBlue * b));
					DestColor = (red << 16) | (green << 8) | blue;
					Mem2Bof[x] = DestColor;
				}
				else
					Mem2Bof[x] = SrcColor;
			}
		}
		SetDIBits(hdcMem2, hMem2Bitmap, 0, rect.bottom, Mem2Buf, &bmi, DIB_RGB_COLORS);
		SelectObject(hdcMem, hMemBitmap);
		SelectObject(hdcMem2, hMem2Bitmap);
		hdc = GetDC(hwnd);
		BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
		if (showalphabet)
			ShowAlphabet();
		ReleaseDC(hwnd, hdc);
		////////////
		SaveScreen();
		////////////
	}
}

void Magnify(void)
{
	if ((pkt.pkX > BoxL) && (pkt.pkX < BoxR) && (pkt.pkY > BoxT) && (pkt.pkY < BoxB)) {
		x = ((pkt.pkX - BoxL) / 10) + boxl;
		y = ((pkt.pkY - BoxT) / 10) + boxt;
		if ((Red == 0xFF) && (Green == 0xFF) && (Blue == 0xFE))
			Blue = 0xFF;//for Insert key erasing
		SetPixel(hdcMem2, x, y, (Blue << 16) | (Green << 8) | Red);
//		MagnifiedBox[y-boxt][x-boxl] = (Blue << 16) | (Green << 8) | Red;
		X = pkt.pkX - ((pkt.pkX - BoxL) % 10);
		Y = pkt.pkY - ((pkt.pkY - BoxT) % 10);
		hMagBrush = CreateSolidBrush((Blue << 16) | (Green << 8) | Red);
		hMagPen = CreatePen(PS_SOLID, 0, 0xE0E0E0);
		hdc = GetDC(hwnd);
		SelectObject(hdc, hMagPen);
		SelectObject(hdc, hMagBrush);
		Rectangle(hdc, X, Y, X+10, Y+10);
		ReleaseDC(hwnd, hdc);
		DeleteObject(hMagBrush);
		DeleteObject(hMagPen);
		changed = TRUE;
	}
}

void ShowHelp(void)
{
	if (openingfile) {
		DestroyWindow(hwndList);
	}
	hwndHelp = CreateWindow("EDIT", szAppName,
		WS_CHILD | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | ES_MULTILINE,//WS_DLGFRAME,
		(rect.right/2)-260, (rect.bottom/2)-(400/2), 520, 420,
		hwnd, (HMENU)191, hInstance, NULL);
	pHelpProc = (WNDPROC)SetWindowLong(hwndHelp, GWL_WNDPROC, (LONG)HelpProc);
	SendMessage(hwndHelp,  EM_SETREADONLY, 1, 0);
	help = TRUE;
	SetFocus(hwndHelp);
	hFile = CreateFile("SimplePaint.txt", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		fileSize = GetFileSize(hFile, NULL);
		if (fileSize) {
			Help = (char*)malloc(fileSize);
			ReadFile(hFile, Help, fileSize, &dwBytesRead, NULL);
			Help[fileSize-2] = 0;//weird but necessary
			SetWindowText(hwndHelp, Help);
			free(Help);
		}
		CloseHandle(hFile);
	}
}

void EndStraightLine(void)
{
	straightline = FALSE;
	endstraightline = FALSE;
	hdc = GetDC(hwnd);
	hPen = CreatePen(PS_SOLID, Radius*2, (Blue << 16) | (Green << 8) | (Red));//reversed color order
	hOldPen = SelectObject(hdc, hPen);
	hOldPen = SelectObject(hdcMem, hPen);
	MoveToEx(hdcMem, ButtonDownX, ButtonDownY, NULL);
	LineTo(hdcMem, pkt.pkX, pkt.pkY);
	MoveToEx(hdc, ButtonDownX, ButtonDownY, NULL);//to show instantly
	LineTo(hdc, pkt.pkX, pkt.pkY);
	SelectObject(hdc, hOldPen);
	SelectObject(hdcMem, hOldPen);
	DeleteObject(hPen);
	ReleaseDC(hwnd, hdc);
}
