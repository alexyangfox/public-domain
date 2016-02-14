//editing goes TRUE with WM_LBUTTONUP
//editingoldentry goes TRUE with WM_LBUTTONDOWN when pointing at previous entry
//they go FALSE in GotEnter
/*
case WM_COMMAND:
case WM_RBUTTONDOWN:
case WM_MOUSEMOVE:
case WM_RBUTTONUP:
case WM_KEYDOWN:
*/
#include <windows.h>
#include <commdlg.h>//for PrintDlg
#include <stdio.h>//for sprintf
#include <math.h>
#include "resource.h"

#define ONEHUNDRED 100
#define TWOHUNDRED 200
#define BITMAPFILEHEADER_SIZE 14
#define PLUS 1
#define MINUS 2
#define MUL 3
#define DIV 4
#define BLACK 0
#define MAROON 0x80
#define GREEN 0x8000
#define OLIVE 0x8080
#define NAVY 0x800000
#define PURPLE 0x800080
#define TEAL 0x808000
#define GRAY 0x808080
#define SILVER 0xc0c0c0
#define RED 0xff
#define LIME 0xff00
#define YELLOW 0xffff
#define BLUE 0xff0000
#define FUCHSIA 0xff00ff
#define AQUA 0xffff00
#define WHITE 0xffffff

char About[] = "Version 2.96\nMar 16, 2012\nby Doug Cox\njdmcox@jdmcox.com";

char CurrentDirectory[MAX_PATH];
char gswin32c[MAX_PATH] = "C:\\Program Files\\gs\\gs9.05\\bin\\gswin32c -dBATCH -dNOPAUSE -sDEVICE=";
char AnyGS[] = "C:\\Program Files\\gs\\gs*";
char GSwin1[] = "C:\\Program Files\\gs\\gs9.05";
char GSwin[] =  "C:\\Program Files\\gs\\gs9.05\\bin\\gswin32c.exe";
char Mono[] = "bmpmono";
char Gray[] = "bmpgray";
char Color[] = "bmp16m";
char OutputFile[] = " -sOutputFile=\"";
int xPrevious = -1, yPrevious, Left, Top;
int bmpType;
char Resolution[4];
char Ini[50];
char IniFile[] = "PrinterResolution=xxxx";
char FillOutAFormIni[] = "FillOutAForm.ini";

char Help[] = "\
Either select File -Convert .pdf to .bmp\n\
(see Help -GhostScript)\n\
or scan a form and save it as a .bmp file\n\
and then select File -Open .bmp file.\n\
\n\
Press '+' or '-' to zoom in or zoom out.\n\
\n\
Press and hold the Left Mouse button\n\
while moving the mouse to scroll the\n\
form, or use the Arrow keys,\n\
or use the mousewheel to scroll up/down,\n\
or the mousewheel with the Ctrl key down\n\
to scroll left/right.\n\
\n\
To enter text, press and hold the Right\n\
Mouse button, move the Edit Box with the\n\
Mouse to where you want it, and then\n\
release the Mouse button.\n\
\n\
Use the Backspace key to delete while entering.\n\
Press Enter to save text, or Esc to cancel.\n\
\n\
To delete an entry, put the Mouse pointer\n\
over it, and press the Delete key.\n\
\n\
To move already entered text, move the\n\
mouse pointer over the entered text\n\
and then press the Right Mouse button,\n\
and before releasing the Right Mouse button,\n\
move the mouse.\n\
\n\
All your entries are saved in a .dta file\n\
(e.g. 1040.dta for a file named 1040.bmp),\n\
and when you reload the form, all entries\n\
are loaded with it.";

char GhostScript[] = "\
GhostScript will convert a .pdf file to a .bmp file\n\
(or a series of .bmp files) with the same name,\n\
but with a number added at the end for each page.\n\
For example, a one-page file named Document.pdf\n\
will be converted to Document1.bmp.\n\
\n\
Get the free GhostScript at:\n\
www.ghostscript.com/download/\n\
\n\
Install it at:\n\
C:/Program Files/gs/\n\
(FillOutAForm looks for it there).\n\
If you have 64-bit Windows 7, move the\n\
gs folder from Program Files (x86) to\n\
Program Files.";

char Advanced[] = "\
Scan a form at the same dpi (dots per inch) as your printer uses, if possible.\n\
\n\
At 300 dpi, an 8 1/2 by 11 inch piece of paper will be 2,550 dots across. A computer\n\
screen with a 1024 by 768 resolution will be 1,024 pixels (or dots) across. That's a\n\
typical relationship between a printed page and a computer screen.\n\
\n\
If you hold an Arrow key down, the screen scrolls faster the longer you hold it down.\n\
\n\
If you load saved form entries, the default font will be set to the first entry's font.\n\
\n\
You can run this program multiple times and switch between them to show different pages.\n\
\n\
Older Windows operating systems are limited to the size of a .bmp file of about 16\n\
Megabytes. A .bmp file bigger than that is probably color, which normally uses 3\n\
bytes/pixel. You can reduce its size to 1/3 of its current size by opening your color\n\
.bmp file in Paint and then saving it as a 256 color .bmp file, which uses 1 byte/pixel.\n\
\n\
Selected font colors will be saved if the form itself is in color.";

char FormulaHelp[] = "\
If you enter a # followed by a number (e.g. #1),\n\
that will tell FillOutAForm that it's a formula\n\
entry. If you enter something like #1=5,\n\
(don't put any spaces in a formula)\n\
then 5 will be assigned to #1, so that later\n\
you can enter #2=#1, and #2 will equal 5.\n\
You can also enter #3=#2+7/2, and #3 will\n\
equal 6. FillOutAForm adds, subtracts, multiplies,\n\
and divides numbers from left to right.\n\
\n\
Sorry, no decimal points. This is integer math only.\n\
\n\
Results are rounded down to the nearest whole number.\n\
\n\
When you press Enter after entering a formula,\n\
the result of the formula will replace the formula.\n\
\n\
BUT, the formula will still be displayed if it:\n\
  can't find a reference number (e.g.#1),\n\
  contains non-numbers,\n\
  creates a negative number,\n\
  contains a bad operation like divide by zero,\n\
  has no equal sign,\n\
  would cause an infinite loop.\n\
\n\
If you move the Mouse pointer over a number\n\
created by a formula, its formula will be\n\
displayed on the Title bar.";

char EditHelp[] = "\
To edit an input, move the Mouse pointer over it\n\
and press and hold the Right Mouse button to move it\n\
where you want it moved, and release the button.\n\
\n\
Edit it (using the Backspace key to delete)\n\
and then press Enter to save, or Esc to delete.";

struct Data{//7*4 + 100 + 32 = 160 bytes
	int x;
	int y;
	int width;
	int height;
	char Entry[ONEHUNDRED];
	char fontName[LF_FACESIZE];//32
	int fontHeight;
	int fontWeight;
	BYTE fontFamily;
	BYTE fontColor;
	BYTE vertical;
	BYTE rightjustified;
} dta, tempDta, locDta[TWOHUNDRED];
int locDtaSize;
COLORREF Colors[] = {BLACK, MAROON, GREEN, OLIVE, NAVY, PURPLE, TEAL, GRAY, SILVER, RED, LIME, YELLOW, FUCHSIA, AQUA, WHITE};

int v, editX;
unsigned int num;
int FormulaXLoc, FormulaSize, EntryCount, EditBox, EntrySize;
char FormulaResult[50];
int Number = 10, X, Y;
void *Buf;//the system allocates this memory in CreateDIBSection
int cxScreen, cyScreen;
int oldXpos, oldYpos, TextLength, saveWparam;
int saveHeight, saveWeight, saveFamily;
int xLoc, yLoc, xPos, yPos, xPointer, yPointer;
int Height, Width, TotalGoRight, TotalGoDown;
int BitmapInfoSize, totalFiles;
double scannerdpi, printerdpi, WidthInches, HeightInches;
char EditBuf[ONEHUNDRED];
char saveName[50];
char szAppName[] = "FillOutAForm";
char tempFileName[] = "FOAF.tmp";
char FileName[MAX_PATH];
//char FileName2[MAX_PATH];
char ShortName[MAX_PATH] = "FillOutAForm";
char FileNameDta[MAX_PATH];
char FormulaFileName[MAX_PATH];
char Dta[] = "dta";
char Size[110];//for sprintf
char ErrorBuf[10];
BOOL editing = FALSE, editingoldentry = FALSE, gotfile = FALSE, frombuttondown = FALSE, first = TRUE, firstime = TRUE, mousewheel = FALSE, fromesc = FALSE;
BOOL oktodelete = FALSE, LEFTJUSTIFIED = TRUE, leftjustified, text = TRUE, gotformula = FALSE, printoriginalform = FALSE, vertical = FALSE, notfound, zoomedout = TRUE;
DWORD bmpFileSize, fileSize, dwBytesRead, dwBytesWritten, CommDlgError, FormulaRightJustified;
HBITMAP hBitmap = NULL;
HMENU hMenu;
RECT rect;
PAINTSTRUCT ps;
HDC hdc, hdcMem, xdc;
SIZE size, fSize;
BITMAPFILEHEADER bmfh;
BITMAPINFO *pBmi;
BITMAPINFOHEADER bmih;
LOGFONT lf;
CHOOSEFONT cf;
COLORREF FontColor, NewFontColor;
TEXTMETRIC tm;
OPENFILENAME ofn, ofn2;
WIN32_FIND_DATA fd;
HINSTANCE hInst;
HPEN hPen;
HANDLE hFile, hFile2, hTempFile, hDtaFile;
HWND hwnd, hEdit, hPrevious, hwndNext;
PRINTDLG pd;
DOCINFO di;
HGDIOBJ OrigObj, PrevObj;
HCURSOR hCursor, hDrawingCursor;

int CALLBACK PDF2BMPProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
WNDPROC pEditProc;
void ReadBmpFile(void);
void ReadDataFile(void);
void GotEnter(void);
void GotEsc(void);
void ShowEntry(int);
int ReadFormula(struct Data*);
int Operate(int, unsigned int, unsigned int);
void ShowScannerData(void);
BOOL FindGhostScript(void);
//BOOL CopytoGraphicsCard(void);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_ICON1));
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
	wndclass.lpszMenuName  = "FORMFILE";
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass (&wndclass))
		return 0;

	hInst = hInstance;

	hwnd = CreateWindow (szAppName, ShortName,
		WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	hwndNext = GetWindow(hwnd, GW_HWNDNEXT);

	ShowWindow (hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow (hwnd);

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}

int Operate(int OperatorType, unsigned int num, unsigned int prevNum)
{
	_int64 li;
	if (OperatorType == PLUS) {
		li = num;
		li += prevNum;//have to do it this way
		if (li > 0xFFFFFFFF)
			num = 0xFFFFFFFF;
		else
			num = (unsigned int)li;
	}
	else if (OperatorType == MINUS) {
			if (num > prevNum)
				num = 0xFFFFFFFF;
			else
				num = prevNum - num;
	}
	else if (OperatorType == MUL) {
		num = MulDiv(num, prevNum, 1);
		if (num == 0xFFFFFFFF)//overflow
			num = 0xFFFFFFFF;
	}
	else if (OperatorType == DIV) {
		if (num != 0)
			num = prevNum / num;
		else
			num = 0xFFFFFFFF;
	}
	return num;
}

void GotEsc(void)
{
	DestroyCaret();
	SendMessage(hEdit, WM_CLOSE, 0, 0);
	hdc = GetDC(hwnd);
	if (!zoomedout)
		BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, SRCCOPY);
	else
	{
		SetStretchBltMode(hdc, HALFTONE); // this makes better stretched/compressed image
		StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, rect.right*2, rect.bottom*2, SRCCOPY);
	}
	ReleaseDC(hwnd, hdc);
	SetFocus(hwnd);
	editing = FALSE;
	editingoldentry = FALSE;
	EditBuf[0] = 0;
}

//sub-class procedure
LRESULT CALLBACK EditProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN)
	{
		if ((editing) && ((wParam == VK_RETURN) || (wParam == VK_TAB)))
			GotEnter();
		else if ((editing) && (wParam == VK_ESCAPE))
		{
			if (editingoldentry == FALSE)
				GotEsc();
			else
			{
				fromesc = TRUE;
				GotEnter();
				fromesc = FALSE;
			}
		}
	}
	return CallWindowProc(pEditProc, hwnd2, message, wParam, lParam);
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y;

	switch (message)
	{
	case WM_SIZE:
		MoveWindow(hwnd, 0, 0, cxScreen, cyScreen, TRUE);
 		GetClientRect(hwnd, &rect);
		SetWindowText(hwnd, ShortName);
		return 0;

	case WM_CREATE:
		GetCurrentDirectory(MAX_PATH, CurrentDirectory);
		if ((hFile = CreateFile(FillOutAFormIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
		{
			fileSize = GetFileSize(hFile, NULL);
			if (fileSize > 50)
				fileSize = 50;
			ReadFile(hFile, Ini, fileSize, &dwBytesRead, NULL);
			CloseHandle(hFile);
			for (x = 0; (Ini[x] != '=') && (x < 50); x++)
				;
			if (Ini[x] == '=')
			{
				if ((Ini[x+1] >= '0') && (Ini[x+1] <= '9'))
					Resolution[0] = Ini[x+1];
				if ((Ini[x+2] >= '0') && (Ini[x+2] <= '9'))
					Resolution[1] = Ini[x+2];
				if ((Ini[x+3] >= '0') && (Ini[x+3] <= '9'))
					Resolution[2] = Ini[x+3];
				if ((Ini[x+4] >= '0') && (Ini[x+4] <= '9'))
					Resolution[3] = Ini[x+4];
			}
		}

		hCursor = LoadCursor(NULL, IDC_ARROW);
		hDrawingCursor = LoadCursor(NULL, IDC_HAND);

		locDtaSize = sizeof(dta);
		hMenu = GetMenu(hwnd);
		CheckMenuItem(hMenu, ID_EDIT_LEFTJUSTIFIEDTEXT, MF_CHECKED);
		CheckMenuItem(hMenu, ID_EDIT_ENTERHORIZONTALTEXT, MF_CHECKED);

		pd.lStructSize = sizeof(PRINTDLG);
		pd.hwndOwner   = hwnd;
		pd.hDevMode    = NULL;
		pd.hDevNames   = NULL;
		pd.hDC         = NULL;
		pd.Flags       = PD_NOPAGENUMS | PD_NOSELECTION | PD_HIDEPRINTTOFILE | PD_RETURNDC; 
		pd.nCopies     = 1;
		pd.nFromPage   = 0; 
		pd.nToPage     = 0; 
		pd.nMinPage    = 0; 
		pd.nMaxPage    = 0;
		pd.hInstance   = NULL;
		pd.lCustData   = 0;
		pd.nFromPage   = 0;
		pd.nToPage     = 0;
		pd.nMaxPage    = 0;
		pd.nMinPage    = 0;
		pd.lpfnPrintHook       = NULL;
		pd.lpfnSetupHook       = NULL;
		pd.lpPrintTemplateName = NULL;
		pd.lpSetupTemplateName = NULL;
		pd.hPrintTemplate      = NULL;
		pd.hSetupTemplate      = NULL;

		di.cbSize = sizeof(DOCINFO);
		di.fwType = 0;
		di.lpszDatatype = NULL;
		di.lpszDocName = "FillOutAForm";
		di.lpszOutput = NULL;

		ofn.lpstrFile         = FileName;
		ofn.lStructSize       = sizeof (OPENFILENAME);
		ofn.hwndOwner         = hwnd;
		ofn.hInstance         = NULL;
		ofn.lpstrFilter       = " *.bmp\0*.bmp\0\0";
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter    = 0;
		ofn.nFilterIndex      = 0;
		ofn.nMaxFile          = MAX_PATH;
		ofn.lpstrFileTitle    = ShortName;
		ofn.nMaxFileTitle     = MAX_PATH;
		ofn.lpstrInitialDir   = NULL;
		ofn.lpstrTitle        = "Open .bmp file";
		ofn.Flags             = OFN_HIDEREADONLY;
		ofn.nFileOffset       = 0;
		ofn.nFileExtension    = 0;
		ofn.lpstrDefExt       = "bmp";
		ofn.lCustData         = 0L;
		ofn.lpfnHook          = NULL;
		ofn.lpTemplateName    = NULL;
		ofn.lpstrDefExt       =".bmp";

		ofn2.lpstrFile         = FileName;
		ofn2.lStructSize       = sizeof (OPENFILENAME);
		ofn2.hwndOwner         = hwnd;
		ofn2.hInstance         = NULL;
		ofn2.lpstrFilter       = " *.pdf\0*.pdf\0\0";
		ofn2.lpstrCustomFilter = NULL;
		ofn2.nMaxCustFilter    = 0;
		ofn2.nFilterIndex      = 0;
		ofn2.nMaxFile          = MAX_PATH;
		ofn2.lpstrFileTitle    = ShortName;
		ofn2.nMaxFileTitle     = MAX_PATH;
		ofn2.lpstrInitialDir   = NULL;
		ofn2.lpstrTitle        = "Open .pdf file";
		ofn2.Flags             = OFN_HIDEREADONLY | OFN_CREATEPROMPT;
		ofn2.nFileOffset       = 0;
		ofn2.nFileExtension    = 0;
		ofn2.lpstrDefExt       = "pdf";
		ofn2.lCustData         = 0L;
		ofn2.lpfnHook          = NULL;
		ofn2.lpTemplateName    = NULL;
		ofn2.lpstrDefExt       =".pdf";

		cf.lStructSize    = sizeof (CHOOSEFONT) ;
		cf.hwndOwner      = hwnd ;
		cf.hDC            = NULL ;
		cf.lpLogFont      = &lf ;
		cf.iPointSize     = 0 ;
		cf.Flags          = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS ;
		cf.rgbColors      = 0 ;
		cf.lCustData      = 0 ;
		cf.lpfnHook       = NULL ;
		cf.lpTemplateName = NULL ;
		cf.hInstance      = NULL ;
		cf.lpszStyle      = NULL ;
		cf.nFontType      = 0 ;      
		cf.nSizeMin       = 0 ;
		cf.nSizeMax       = 0 ;

		lf.lfWidth = 0;
		lf.lfEscapement = 0;
		lf.lfOrientation = 0;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = ANSI_CHARSET;
		lf.lfOutPrecision = OUT_DEFAULT_PRECIS;
		lf.lfClipPrecision = CLIP_DEFAULT_PRECIS;
		lf.lfQuality = DEFAULT_QUALITY;
		lf.lfHeight = -30;
		lf.lfWeight = 400;
		lf.lfPitchAndFamily = DEFAULT_PITCH | FF_DONTCARE;//34;
		strcpy (lf.lfFaceName, "Arial");

		if (SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0))
		{
			cxScreen = rect.right;
			cyScreen = rect.bottom;
		}
		else
		{
			cxScreen = GetSystemMetrics(SM_CXFULLSCREEN);
			cyScreen = GetSystemMetrics(SM_CYFULLSCREEN);
		}
		pBmi = NULL;
		return 0;

	case WM_COMMAND:
		if (((HWND)lParam == hEdit) &&(HIWORD(wParam) == EN_CHANGE))
		{//***if character typed in or moved***
			HideCaret(hwnd);
			hdc = GetDC(hwnd);
			if (!zoomedout)
				BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, SRCCOPY);
			else
			{
				SetStretchBltMode(hdc, HALFTONE); // this makes better stretched/compressed image
				StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, rect.right*2, rect.bottom*2, SRCCOPY);
			}
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, FontColor);//only works if bmp file is in color
			if (editingoldentry)
				v = tempDta.vertical;
			else//pointing at already entered text
				v = vertical;
			x = lf.lfHeight; // save original
			if (zoomedout)
				lf.lfHeight /= 2;
			if ((leftjustified) && (v == FALSE))
			{
				lf.lfEscapement = lf.lfOrientation = 0;
				OrigObj = SelectObject(hdc, CreateFontIndirect(&lf));
				TextLength = GetWindowText(hEdit, EditBuf, ONEHUNDRED);
				GetTextExtentPoint32(hdc, EditBuf, TextLength, &size);//put entry horiz & vert pixels in size
				SetTextAlign(hdc, TA_LEFT|TA_TOP);
				TextOut(hdc, oldXpos, oldYpos, EditBuf, TextLength);
				SetCaretPos(oldXpos + 1 + size.cx, oldYpos + 2);
				MoveToEx(hdc, oldXpos+tm.tmMaxCharWidth, oldYpos, NULL);
				LineTo(hdc, oldXpos, oldYpos);
				LineTo(hdc, oldXpos, oldYpos+tm.tmHeight);
				LineTo(hdc, oldXpos+tm.tmMaxCharWidth, oldYpos+tm.tmHeight);
			}
			else if ((leftjustified) && (v == TRUE))
			{
				lf.lfEscapement = lf.lfOrientation = 900;
				OrigObj = SelectObject(hdc, CreateFontIndirect(&lf));
				TextLength = GetWindowText(hEdit, EditBuf, ONEHUNDRED);
				GetTextExtentPoint32(hdc, EditBuf, TextLength, &size);//put entry horiz & vert pixels in size
				SetTextAlign(hdc, TA_LEFT|VTA_TOP);
				TextOut(hdc, oldXpos, oldYpos, EditBuf, TextLength);
				SetCaretPos(oldXpos + 2, oldYpos - 1 - size.cx);
				MoveToEx(hdc, xPos, yPos-tm.tmMaxCharWidth, NULL);
				LineTo(hdc, xPos, yPos);
				LineTo(hdc, xPos+tm.tmHeight, yPos);
				LineTo(hdc, xPos+tm.tmHeight, yPos-tm.tmMaxCharWidth);
			}
			else//if (leftjustified == FALSE)
			{
				lf.lfEscapement = lf.lfOrientation = 0;
				OrigObj = SelectObject(hdc, CreateFontIndirect(&lf));
				TextLength = GetWindowText(hEdit, EditBuf, ONEHUNDRED);
				GetTextExtentPoint32(hdc, EditBuf, TextLength, &size);//put entry horiz & vert pixels in size
				SetTextAlign(hdc, TA_LEFT|TA_TOP);
				TextOut(hdc, oldXpos-size.cx, oldYpos, EditBuf, TextLength);
				MoveToEx(hdc, oldXpos-tm.tmMaxCharWidth, oldYpos, NULL);
				LineTo(hdc, oldXpos, oldYpos);
				LineTo(hdc, oldXpos, oldYpos+tm.tmHeight);
				LineTo(hdc, oldXpos-tm.tmMaxCharWidth, oldYpos+tm.tmHeight);
			}
			lf.lfHeight = x; // get original
			SelectObject(hdc, OrigObj);
			ReleaseDC(hwnd, hdc);
			ShowCaret(hwnd);
			break;
		}

		switch (LOWORD(wParam))
		{
		case ID_FILE_CONVERTPDFTOBMP:
			if (INVALID_HANDLE_VALUE == FindFirstFile(AnyGS, &fd) || (0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
			{
				MessageBox(hwnd, "Read Help -GhostScript", "File not found!", MB_OK);
				break;
			}
			notfound = TRUE;
			while (notfound)
			{
				notfound = FindGhostScript();
			}

			if (INVALID_HANDLE_VALUE != FindFirstFile(GSwin, &fd))
			{
				if (GetOpenFileName(&ofn2))
				{
					if (DialogBox(hInst, "PDFTOBMP", NULL, PDF2BMPProc))
					{
						if (bmpType == 1)
							for (x = 0, y = 67; Mono[x] != 0; x++, y++)
								gswin32c[y] = Mono[x];
						else if (bmpType == 2)
							for (x = 0, y = 67; Gray[x] != 0; x++, y++)
								gswin32c[y] = Gray[x];
						else if (bmpType == 3)
							for (x = 0, y = 67; Color[x] != 0; x++, y++)
								gswin32c[y] = Color[x];
						gswin32c[y++] = ' ';
						gswin32c[y++] = '-';
						gswin32c[y++] = 'r';
						gswin32c[y++] = Resolution[0];
						gswin32c[y++] = Resolution[1];
						if (Resolution[2] != 0)
							gswin32c[y++] = Resolution[2];
						for (x = 0; OutputFile[x] != 0; x++, y++)
							gswin32c[y] = OutputFile[x];
						for (x = 0; ShortName[x] != '.'; x++, y++)
							gswin32c[y] = ShortName[x];
						gswin32c[y++] = '"';
						gswin32c[y++] = '%';
						gswin32c[y++] = 'd';
						gswin32c[y++] = '.';
						gswin32c[y++] = 'b';
						gswin32c[y++] = 'm';
						gswin32c[y++] = 'p';
						gswin32c[y++] = ' ';
						gswin32c[y++] = '"';
						for (x = 0; ShortName[x] != 0; x++, y++)
							gswin32c[y] = ShortName[x];
						gswin32c[y++] = '"';
						gswin32c[y++] = 0;
						FileName[0] = 0;
						WinExec(gswin32c, SW_SHOW);
					}
					else break;
				}
				else break;
			}
			else
			{
				MessageBox(hwnd, GSwin, "Couldn't find:", MB_OK);
				break;
			}
//fall thru...
		case ID_FILE_OPENBMPFILE_TOPRINTBMPFILE:
			zoomedout = TRUE;
			printoriginalform = FALSE;
			goto skip;
		case ID_FILE_OPENBMPFILE_TOPRINTORIGINALFORM:
			printoriginalform = TRUE;
skip:		if (editing)
				GotEsc();
			if (GetOpenFileName(&ofn))
			{
				xLoc = yLoc = xPos = yPos = 0;
				FontColor = 0;
				///////////
				ReadBmpFile();
				///////////
			}
			break;

		case IDFILEPRINT:
			if (editing)
				GotEsc();
			////////////
			ReadDataFile();
			////////////
			ShowScannerData();

			if (PrintDlg(&pd))
			{
				printerdpi = GetDeviceCaps(pd.hDC, PHYSICALWIDTH) / 8.8;//another fudge factor
//printerdpi -= 20;//works at 600 dpi...
				sprintf(Size, "Printer resolution is %.0f dpi\n", printerdpi);
				if (IDOK == MessageBox(hwnd, Size, "Info", MB_OKCANCEL))
				{
					if (StartDoc(pd.hDC, &di) > 0)
					{
						if (StartPage(pd.hDC) > 0)
						{
							if (scannerdpi != 0)
								StretchBlt(pd.hDC, 0, 0, (Width*(int)printerdpi)/(int)scannerdpi, (Height*(int)printerdpi)/(int)scannerdpi, hdcMem, 0, 0, Width, Height, SRCCOPY);
							else
							{
								if (!zoomedout)
									BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, SRCCOPY);
								else
								{
									SetStretchBltMode(hdc, HALFTONE); // this makes better stretched/compressed image
									StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, rect.right*2, rect.bottom*2, SRCCOPY);
								}
							}
							if (EndPage(pd.hDC) > 0)
								EndDoc(pd.hDC);//done
						}
						else 
							MessageBox(hwnd, "StartPage is 0!", NULL, MB_OK);
					}
					else 
						MessageBox(hwnd, "StartDoc is 0!", NULL, MB_OK);
					DeleteDC(pd.hDC);
				}
			}
//			else
//			{
//				CommDlgError = CommDlgExtendedError();
//				if (CommDlgError != 0)
//					MessageBox(hwnd, _itoa(CommDlgError, ErrorBuf, 10), "Error number:", MB_OK);
//			}
			break;

		case ID_FILE_EXIT:
			if (editing)
				GotEsc();
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		case ID_PLUS:
			if (zoomedout)
			{
				zoomedout = FALSE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;
		case ID_MINUS:
			if (!zoomedout)
			{
				zoomedout = TRUE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case ID_EDIT_EDITHELP:
			if (editing)
				GotEsc();
			hPrevious = GetFocus();
			MessageBox(hwnd, EditHelp, "Edit Help", MB_OK);
			SetFocus(hPrevious);
			break;

		case ID_EDIT_LEFTJUSTIFIEDTEXT:
			LEFTJUSTIFIED = TRUE;
			CheckMenuItem(hMenu, ID_EDIT_LEFTJUSTIFIEDTEXT, MF_CHECKED);
			CheckMenuItem(hMenu, ID_EDIT_RIGHTJUSTIFIEDTEXT, MF_UNCHECKED);
			break;

		case ID_EDIT_RIGHTJUSTIFIEDTEXT:
			LEFTJUSTIFIED = FALSE;
			CheckMenuItem(hMenu, ID_EDIT_RIGHTJUSTIFIEDTEXT, MF_CHECKED);
			CheckMenuItem(hMenu, ID_EDIT_LEFTJUSTIFIEDTEXT, MF_UNCHECKED);
			break;

		case ID_EDIT_ENTERHORIZONTALTEXT:
			vertical = FALSE;
			CheckMenuItem(hMenu, ID_EDIT_ENTERHORIZONTALTEXT, MF_CHECKED);
			CheckMenuItem(hMenu, ID_EDIT_ENTERVERTICALTEXT, MF_UNCHECKED);
			break;

		case ID_EDIT_ENTERVERTICALTEXT:
			if (LEFTJUSTIFIED)
			{
				vertical = TRUE;
				CheckMenuItem(hMenu, ID_EDIT_ENTERVERTICALTEXT, MF_CHECKED);
				CheckMenuItem(hMenu, ID_EDIT_ENTERHORIZONTALTEXT, MF_UNCHECKED);
			}
			else
				MessageBox(hwnd, "I didn't write any right-justified vertical entry routines.", "Sorry", MB_OK);
			break;

		case ID_FONTS:
			if (editing)
				GotEsc();
			if (ChooseFont(&cf))
			{
				if (bmih.biBitCount > 1)
					FontColor = cf.rgbColors;
				InvalidateRect(hwnd, NULL, TRUE);
			}
			break;

		case ID_HELPUSING:
			if (editing)
				GotEsc();
			hPrevious = GetFocus();
			MessageBox(hwnd, Help, "Help", MB_OK);
			SetFocus(hPrevious);
			break;
		case ID_HELP_GETTINGGHOSTSCRIPT:
			if (editing)
				GotEsc();
			hPrevious = GetFocus();
			MessageBox(hwnd, GhostScript, "GhostScript", MB_OK);
			break;
		case ID_HELPADVANCED:
			if (editing)
				GotEsc();
			hPrevious = GetFocus();
			MessageBox(hwnd, Advanced, "Advanced", MB_OK);
			SetFocus(hPrevious);
			break;
		case ID_HELPFORMULAS:
			if (editing)
				GotEsc();
			hPrevious = GetFocus();
			MessageBox(hwnd, FormulaHelp, "Formula Help", MB_OK);
			SetFocus(hPrevious);
			break;
		case ID_HELPABOUT:
			if (editing)
				GotEsc();
			hPrevious = GetFocus();
			MessageBox(hwnd, About, szAppName, MB_OK);
			SetFocus(hPrevious);
			break;
		}
		frombuttondown = FALSE;
		return 0;

	case WM_RBUTTONDOWN:
		frombuttondown = TRUE;
		EditBox = 0;
		editX = -1;
		if ((!editing) && (gotfile))
		{
			if (LEFTJUSTIFIED)
				leftjustified = TRUE;
			else
				leftjustified = FALSE;
			for (x = 0; locDta[x].x != 0; x++)
			{//see if pointing at already entered text
				X = locDta[x].x;
				Y = locDta[x].y;
				xPointer = xPos + xLoc;
				yPointer = yPos + yLoc;
				if (locDta[x].vertical == FALSE)
				{
					if (locDta[x].rightjustified)
						X -= locDta[x].width;
					if ((xPointer > X) && (xPointer < (X + locDta[x].width))
					 && (yPointer > Y) && (yPointer < (Y + locDta[x].height)))
					{
						EditBox = x;
						for (EntrySize = 0; locDta[EditBox].Entry[EntrySize] != 0; EntrySize++)
							;
						SetWindowText(hwnd, ShortName);
						editingoldentry = TRUE;
						editX = x;
						tempDta = locDta[x];
						xPos = locDta[x].x;
						yPos = locDta[x].y;
						lf.lfHeight = -(locDta[x].fontHeight)*2;
						lf.lfWeight = locDta[x].fontWeight;
						lf.lfPitchAndFamily = locDta[x].fontFamily;
						FontColor = 0;
						for (y = 0; y < 16; y++)
						{
							if (locDta[x].fontColor == y)
							{
								NewFontColor = FontColor = Colors[y];
								break;
							}
						}
						strcpy(lf.lfFaceName, locDta[x].fontName);
						if (locDta[x].rightjustified == TRUE)
							leftjustified = FALSE;
						else
							leftjustified = TRUE;
						break;
					}
				}
				else//if (locDta[x].vertical == TRUE)
				{
					if ((yPointer < Y) && (yPointer > (Y - locDta[x].width))
					 && (xPointer > X) && (xPointer < (X + locDta[x].height)))
					{
						EditBox = x;
						for (EntrySize = 0; locDta[EditBox].Entry[EntrySize] != 0; EntrySize++)
							;
						SetWindowText(hwnd, ShortName);
						editingoldentry = TRUE;
						editX = x;
						tempDta = locDta[x];
						xPos = locDta[x].x;
						yPos = locDta[x].y;
						lf.lfHeight = -(locDta[x].fontHeight*2);
						lf.lfWeight = locDta[x].fontWeight;
						lf.lfPitchAndFamily = locDta[x].fontFamily;
						FontColor = 0;
						for (y = 0; y < 16; y++)
						{
							if (locDta[x].fontColor == y)
							{
								NewFontColor = FontColor = Colors[y];
								break;
							}
						}
						strcpy(lf.lfFaceName, locDta[x].fontName);
						if (locDta[x].rightjustified == TRUE)
							leftjustified = FALSE;
						else
							leftjustified = TRUE;
						break;
					}
				}
			}
			if (editingoldentry == FALSE)
			{
				xPos = LOWORD(lParam);
				yPos = HIWORD(lParam);
			}
			hdc = GetDC(hwnd);
			x = lf.lfHeight; // save original
			if (zoomedout)
				lf.lfHeight /= 2;
			OrigObj = SelectObject(hdc, CreateFontIndirect(&lf));
			GetTextMetrics(hdc, &tm);
			SelectObject(hdc, OrigObj);
			lf.lfHeight = x; // get original
			if (editX == -1)
				v = vertical;
			else//pointing at already entered text
				v = locDta[editX].vertical;
			if ((leftjustified) && (v == FALSE))
			{
				MoveToEx(hdc, xPos+tm.tmMaxCharWidth, yPos, NULL);
				LineTo(hdc, xPos, yPos);
				LineTo(hdc, xPos, yPos+tm.tmHeight);
				LineTo(hdc, xPos+tm.tmMaxCharWidth, yPos+tm.tmHeight);
			}
			else if ((leftjustified) && (v == TRUE))
			{
				MoveToEx(hdc, xPos, yPos-tm.tmMaxCharWidth, NULL);
				LineTo(hdc, xPos, yPos);
				LineTo(hdc, xPos+tm.tmHeight, yPos);
				LineTo(hdc, xPos+tm.tmHeight, yPos-tm.tmMaxCharWidth);
			}
			else//if ((leftjustified == FALSE) && (v == FALSE))
			{
				MoveToEx(hdc, xPos-tm.tmMaxCharWidth, yPos, NULL);
				LineTo(hdc, xPos, yPos);
				LineTo(hdc, xPos, yPos+tm.tmHeight);
				LineTo(hdc, xPos-tm.tmMaxCharWidth, yPos+tm.tmHeight);
			}
			ReleaseDC(hwnd, hdc);
		}
		return 0;

	case 0x020A://WM_MOUSEWHEEL
		if (LOWORD(wParam) == MK_CONTROL)
		{
			mousewheel = TRUE;
			if (wParam & 0x080000000)
				SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
			else
				SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
		}
		else
		{
			if (wParam & 0x080000000)
				SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 0);
			else
				SendMessage(hwnd, WM_KEYDOWN, VK_UP, 0);
		}
		return 0;

	case WM_MOUSEMOVE:
		if ((!editing) && (wParam == MK_LBUTTON))
		{
			SetCursor(hDrawingCursor);//have to do this every time !?!
			if (xPrevious != -1)
			{
				xPrevious = xPos; // from previous WM_MOUSEMOVE
				yPrevious = yPos;
			}
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			if (zoomedout) {
				xPos *= 2;
				yPos *= 2;
			}
			if (xPrevious == -1)
			{//initialize it
				xPrevious = xPos;
				yPrevious = yPos;
			}
			if (Width > cxScreen)
				xLoc += (xPrevious-xPos);
			if (Height > cyScreen)
				yLoc += (yPrevious-yPos);
			if (xLoc < 0) xLoc = 0;
			if (xLoc > Left) xLoc = Left;
			if (yLoc < 0) yLoc = 0;
			if (yLoc > Top) yLoc = Top;
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
		}
		xPos = LOWORD(lParam); // this needs to be here!
		yPos = HIWORD(lParam);
		if (!editing)
		{
			if (wParam != MK_RBUTTON)
			{//check for formula under cursor
				if (zoomedout) {
					xPos *= 2;
					yPos *= 2;
				}
				if (vertical == FALSE)
				{
					for (x = 0; locDta[x].x != 0; x++)
					{
						X = locDta[x].x;
						if (locDta[x].rightjustified)
							X -= locDta[x].width;
						if (((xPos + xLoc) > X) && ((xPos + xLoc) < (X + locDta[x].width))
						 && ((yPos + yLoc) > (locDta[x].y)) && ((yPos + yLoc) < (locDta[x].y + locDta[x].height)))
						{
							if (gotformula == FALSE)
							{//only do it first time over formula
								gotformula = TRUE;
								if (locDta[x].Entry[0] == '#')//indicates formula
									SetWindowText(hwnd, locDta[x].Entry);
							}
							break;
						}				
					}
					if (locDta[x].x == 0)
					{//if no formula
						if (gotformula)
							SetWindowText(hwnd, ShortName);
						gotformula = FALSE;
					}
				}
			}
			else//(wParam == MK_RBUTTON)
			{//***moving the edit box***
				hdc = GetDC(hwnd);
//				if (0 == BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, SRCCOPY))
//				{
//					MessageBox(hwnd, "Can't copy image from memory!", NULL, MB_OK);
//					SendMessage(hwnd, WM_CLOSE, 0, 0);
//					return 0;
//				}
				if (!zoomedout)
					BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, SRCCOPY);
				else
				{
					SetStretchBltMode(hdc, HALFTONE); // this makes better stretched/compressed image
					StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, rect.right*2, rect.bottom*2, SRCCOPY);
				}
				if (leftjustified)
				{
					if (EditBuf[0] == 0)
					{
						if (editingoldentry)
							v = tempDta.vertical;
						else//pointing at already entered text
							v = vertical;
						if (v == FALSE)
						{
							MoveToEx(hdc, xPos+tm.tmMaxCharWidth, yPos, NULL);
							LineTo(hdc, xPos, yPos);
							LineTo(hdc, xPos, yPos+tm.tmHeight);
							LineTo(hdc, xPos+tm.tmMaxCharWidth, yPos+tm.tmHeight);
						}
						else//if (v)
						{
							MoveToEx(hdc, xPos, yPos-tm.tmMaxCharWidth, NULL);
							LineTo(hdc, xPos, yPos);
							LineTo(hdc, xPos+tm.tmHeight, yPos);
							LineTo(hdc, xPos+tm.tmHeight, yPos-tm.tmMaxCharWidth);
						}
					}
					else
					{
						OrigObj = SelectObject(hdc, CreateFontIndirect(&lf));
						SetBkMode(hdc, TRANSPARENT);
						SetTextColor(hdc, Colors[locDta[EditBox].fontColor]);//only works if bmp file is in color
						if (locDta[EditBox].Entry[0] != '#')//normal
							TextOut(hdc, xPos, yPos, locDta[EditBox].Entry, EntrySize);
						else//if formula
						{
							unsigned y;
							EntryCount = 0;
							if (0xFFFFFFFF != (num = ReadFormula(&locDta[EditBox])))
							{
								_itoa(num, FormulaResult, 10);
								if (num != 0)
									for (y = 1, FormulaSize = 0; y <= num; y *= 10)
										FormulaSize++;
								else
									FormulaSize = 1;
								FormulaXLoc = locDta[EditBox].x;
								if (TRUE == locDta[EditBox].rightjustified)
								{
									GetTextExtentPoint32(hdcMem, FormulaResult, FormulaSize, &fSize);
									FormulaXLoc -= fSize.cx;
								}
								TextOut(hdc, xPos, yPos, FormulaResult, FormulaSize);
							}
							else
								TextOut(hdc, xPos, yPos, locDta[EditBox].Entry, EntrySize);
							SelectObject(hdc, OrigObj);
						}
					}
				}
				else
				{
					MoveToEx(hdc, xPos-tm.tmMaxCharWidth, yPos, NULL);
					LineTo(hdc, xPos, yPos);
					LineTo(hdc, xPos, yPos+tm.tmHeight);
					LineTo(hdc, xPos-tm.tmMaxCharWidth, yPos+tm.tmHeight);
				}
				ReleaseDC(hwnd, hdc);
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		SetCursor(hDrawingCursor);//have to do this every time !?!
		break;
		
	case WM_LBUTTONUP:
		xPrevious = -1;
		SetCursor(hCursor);//have to do this every time !?!
		return 0;

	case WM_RBUTTONUP:
		if ((!editing) && (gotfile) && (frombuttondown))
		{
			editing = TRUE;
			oldXpos = LOWORD(lParam);
			oldYpos = HIWORD(lParam);
			hEdit = CreateWindow("EDIT", NULL, WS_CHILD, 0, 100, rect.right, 100, hwnd, NULL, hInst, NULL);
			pEditProc = (WNDPROC)SetWindowLong(hEdit, GWL_WNDPROC, (LONG)EditProc);
			SendMessage(hEdit, EM_LIMITTEXT, ONEHUNDRED, 0);
			if (editingoldentry)
			{
				hTempFile = CreateFile(tempFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				hFile = CreateFile(FileNameDta, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
				fileSize = GetFileSize(hFile, NULL);
				totalFiles = (int)fileSize / locDtaSize;
				for (y = 0; y < totalFiles; y++)
				{
					ReadFile(hFile, &dta, locDtaSize, &dwBytesRead, NULL);
					if (y != editX)//get rid of it
						WriteFile(hTempFile, &dta, locDtaSize, &dwBytesWritten, NULL);
				}
				totalFiles--;
				CloseHandle(hFile);
				CloseHandle(hTempFile);
				DeleteFile(FileNameDta);
				MoveFile(tempFileName, FileNameDta);//rename it
				///////////
				ReadBmpFile();
				///////////
			}
			SetFocus(hEdit);
			if (editingoldentry)
				v = tempDta.vertical;
			else//pointing at already entered text
				v = vertical;
			if ((leftjustified) && (v == FALSE))
			{
				if (!zoomedout)
					CreateCaret(hwnd, NULL, 1, abs(lf.lfHeight));
				else
					CreateCaret(hwnd, NULL, 1, abs(lf.lfHeight/2));
				SetCaretPos(oldXpos + 1, oldYpos + 2);
			}
			else if ((leftjustified) && (v == TRUE))
			{
				if (!zoomedout)
					CreateCaret(hwnd, NULL, abs(lf.lfHeight), 1);
				else
					CreateCaret(hwnd, NULL, abs(lf.lfHeight/2), 1);
				SetCaretPos(oldXpos + 2, oldYpos - 2);
			}
			else//if ((leftjustified == FALSE) && (v == FALSE))
			{
				if (!zoomedout)
					CreateCaret(hwnd, NULL, 1, abs(lf.lfHeight));
				else
					CreateCaret(hwnd, NULL, 1, abs(lf.lfHeight/2));
				SetCaretPos(oldXpos - 2, oldYpos + 2);
			}
			ShowCaret(hwnd);

			if (editingoldentry)
			{
				FontColor = NewFontColor;
				SendMessage(hEdit, WM_SETTEXT, 0, (LPARAM)&tempDta.Entry);//have to use tempDta because editX points to a different entry now
				SendMessage(hEdit, WM_KEYDOWN, (WPARAM)VK_END, 0);
			}
		}
		return 0;

	case WM_KEYUP:
		Number = 10;
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_DELETE:
			if (gotformula)
			{
				gotformula = FALSE;
				SetWindowText(hwnd, ShortName);
			}
			for (x = 0; locDta[x].x != 0; x++)
			{//see if pointing at already entered text
				X = locDta[x].x;
				Y = locDta[x].y;
				xPointer = xPos + xLoc;
				yPointer = yPos + yLoc;
				if (locDta[x].vertical == FALSE)
				{
					if (locDta[x].rightjustified)
						X -= locDta[x].width;
					if ((xPointer > X) && (xPointer < (X + locDta[x].width))
					 && (yPointer > Y) && (yPointer < (Y + locDta[x].height)))
					{
						hTempFile = CreateFile(tempFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
						hFile = CreateFile(FileNameDta, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
						fileSize = GetFileSize(hFile, NULL);
						totalFiles = (int)fileSize / locDtaSize;
						for (y = 0; y < totalFiles; y++)
						{
							ReadFile(hFile, &dta, locDtaSize, &dwBytesRead, NULL);
							if (y != x)//get rid of it
								WriteFile(hTempFile, &dta, locDtaSize, &dwBytesWritten, NULL);
						}
						CloseHandle(hFile);
						CloseHandle(hTempFile);
						DeleteFile(FileNameDta);
						MoveFile(tempFileName, FileNameDta);//rename it
						///////////
						ReadBmpFile();
						///////////
						break;
					}
				}
				else//if (locDta[x].vertical == TRUE)
				{
					if ((yPointer < Y) && (yPointer > (Y - locDta[x].width))
					 && (xPointer > X) && (xPointer < (X + locDta[x].height)))
					{
						hTempFile = CreateFile(tempFileName, GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
						hFile = CreateFile(FileNameDta, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
						fileSize = GetFileSize(hFile, NULL);
						totalFiles = (int)fileSize / locDtaSize;
						for (y = 0; y < totalFiles; y++)
						{
							ReadFile(hFile, &dta, locDtaSize, &dwBytesRead, NULL);
							if (y != x)//get rid of it
								WriteFile(hTempFile, &dta, locDtaSize, &dwBytesWritten, NULL);
						}
						CloseHandle(hFile);
						CloseHandle(hTempFile);
						DeleteFile(FileNameDta);
						MoveFile(tempFileName, FileNameDta);//rename it
						///////////
						ReadBmpFile();
						///////////
						break;
					}
				}
			}
			break;

		case VK_F1:
			SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_HELPUSING, 0);
			break;
		case VK_HOME:
			xLoc = 0;
			yLoc = 0;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case VK_UP:
			if (yLoc >= Number)
			{
				if (zoomedout)
					yLoc -= Number*10;
				else
					yLoc -= Number;
			}
			else if (yLoc < Number)
				yLoc = 0;
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
			Number++;
			break;
		case VK_DOWN:
			if (yLoc < TotalGoDown)
			{
				if ((yLoc + Number) > TotalGoDown)
					yLoc = TotalGoDown;
				else
				{
					if (zoomedout)
						yLoc += Number*10;
					else
						yLoc += Number;
				}
				InvalidateRect(hwnd, &rect, FALSE);
				UpdateWindow(hwnd);
				Number++;
			}
			break;
		case VK_RIGHT:
			if ((xLoc + Number) < TotalGoRight)
			{
				if (zoomedout)
					xLoc += Number*10;
				else
					xLoc += Number;
			}
			else
				xLoc = TotalGoRight;
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
			Number++;
			break;
		case VK_LEFT:
			if (zoomedout)
			{
				if (xLoc > Number*10)
					xLoc -= Number*10;
				else
					xLoc = 0;
			}
			else
			{
				if (xLoc > Number)
					xLoc -= Number;
				else
					xLoc = 0;
			}
			Number++;
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
			break;
		case 187: // '+'
			if (zoomedout)
			{
				zoomedout = FALSE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;
		case 189: // '-'
			if (!zoomedout)
			{
				zoomedout = TRUE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (hBitmap)
		{
			if (!zoomedout)
				BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, SRCCOPY);
			else
			{
				SetStretchBltMode(hdc, HALFTONE); // this makes better stretched/compressed image
				StretchBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, xLoc, yLoc, rect.right*2, rect.bottom*2, SRCCOPY);
			}
		}
		EndPaint(hwnd, &ps);
//		if (first)
//		{//VERY TRICK
//			first = FALSE;
//			hFile2 = FindFirstFile("*.bmp", &fd);
//			strcpy(FileName2, fd.cFileName);
//			if (INVALID_HANDLE_VALUE == hFile2)
//				MessageBox(hwnd, Help, "Help", MB_OK);
//		}
		return 0;

	case WM_CLOSE:
		if (oktodelete)
		{
			SelectObject(hdcMem, PrevObj);
			DeleteDC(hdcMem);
			DeleteObject(hBitmap);
		}
		if (pBmi != NULL)
			free(pBmi);
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		PostQuitMessage(0);
		SetForegroundWindow(hwndNext);
		return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}
//*******************************************************
void ReadBmpFile()
{
	int x, y;

	if ((hFile = CreateFile(FileName, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
	{
		if (bmpFileSize = GetFileSize(hFile, NULL))
		{
			ReadFile(hFile, &bmfh, BITMAPFILEHEADER_SIZE, &dwBytesRead, NULL);
			if (bmfh.bfType != 0x4D42)//"BM"
			{
				CloseHandle(hFile);
				MessageBox(hwnd, "Not a .bmp file", NULL, MB_OK);
				return;
			}
			if (pBmi != NULL)
			{
				free(pBmi);
				pBmi = NULL;
			}
			if (oktodelete == TRUE)
			{
				oktodelete = FALSE;
				SelectObject(hdcMem, PrevObj);
				DeleteDC(hdcMem);
				DeleteObject(hBitmap);
			}
			else
				oktodelete = TRUE;
			BitmapInfoSize = bmfh.bfOffBits - BITMAPFILEHEADER_SIZE;
			pBmi = (BITMAPINFO*) malloc(BitmapInfoSize);
			ReadFile(hFile, pBmi, BitmapInfoSize, &dwBytesRead, NULL);
			bmih = pBmi->bmiHeader;
			bmpFileSize -= bmfh.bfOffBits;
			hBitmap = CreateDIBSection(NULL, pBmi, DIB_RGB_COLORS, &Buf, NULL, 0); // hBitmap is handle for Buf data (with SelectObject(hdcMem, hBitmap), below)
			ReadFile(hFile, Buf, bmpFileSize, &dwBytesRead, NULL);//Buf is allocated by system
			if (printoriginalform)
				FillMemory(Buf, bmpFileSize, 0xFF);
			CloseHandle(hFile);
			SetWindowText(hwnd, ShortName);
			gotfile = TRUE;
		}
		else
		{
			CloseHandle(hFile);
			MessageBox(hwnd, "File size was 0", NULL, MB_OK);
			return;
		}
	}
	else
		return;
	Width = bmih.biWidth;
	Height = abs(bmih.biHeight);
	if (Height > cyScreen)
		Top = Height-cyScreen;
	else
		Top = 0;
	if (Width > cxScreen)
		Left = Width-cxScreen;
	else
		Left = 0;
	if (Width > rect.right)
		TotalGoRight = Width - rect.right;
	else TotalGoRight = 0;
	if (Height > rect.bottom)
		TotalGoDown = Height - rect.bottom;
	else TotalGoDown = 0;

	hdc = GetDC(hwnd);
	hdcMem = CreateCompatibleDC(hdc);
	if (NULL == hdcMem)
	{
		MessageBox(hwnd, "Can't create hdcMem!", NULL, MB_OK);
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		return;
	}
	ReleaseDC(hwnd, hdc);

	if (NULL == hBitmap)
	{//from CreateDIBSection, above
		MessageBox(hwnd, "Can't get hBitmap!", NULL, MB_OK);
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		return;
	}

	PrevObj = SelectObject(hdcMem, hBitmap);
	if (NULL == PrevObj)
	{
		MessageBox(hwnd, "Can't select hBitmap into hdcMem!", NULL, MB_OK);
		SendMessage(hwnd, WM_CLOSE, 0, 0);
		return;
	}

	for (x = 0; ofn.lpstrFile[x] != 0; x++)
		FileNameDta[x] = ofn.lpstrFile[x];
	x -= 3;
	for (y = 0; y < 3; x++, y++)
		FileNameDta[x] = Dta[y];
	FileNameDta[x] = 0;
	////////////
	ReadDataFile();
	////////////
}

void ReadDataFile(void)
{
	int x, y;

	for (x = 0; x < TWOHUNDRED; x++)
	{
		locDta[x].x = 0;
		locDta[x].y = 0;
		locDta[x].width = 0;
		locDta[x].height = 0;
		for (y = 0; y < ONEHUNDRED; y++)
			locDta[x].Entry[y] = 0;
		for (y = 0; y < LF_FACESIZE; y++)
			locDta[x].fontName[y] = 0;
		locDta[x].fontHeight = 0;
		locDta[x].fontWeight = 0;
		locDta[x].fontFamily = 0;
		locDta[x].rightjustified = 0;
		locDta[x].vertical = 0;
	}

	hDtaFile = CreateFile(FileNameDta, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hDtaFile != INVALID_HANDLE_VALUE)
	{
		fileSize = GetFileSize(hDtaFile, NULL);
		totalFiles = (int)fileSize / locDtaSize;
		for (x = 0; x < totalFiles; x++)
			ReadFile(hDtaFile, &locDta[x], locDtaSize, &dwBytesRead, NULL);
		CloseHandle(hDtaFile);

		saveHeight = lf.lfHeight;
		saveWeight = lf.lfWeight;
		saveFamily = lf.lfPitchAndFamily;
		strcpy(saveName, lf.lfFaceName);
		for (x = 0; x < totalFiles; x++)
		{
			lf.lfHeight = -(locDta[x].fontHeight)*2;
			lf.lfWeight = locDta[x].fontWeight;
			lf.lfPitchAndFamily = locDta[x].fontFamily;
			strcpy(lf.lfFaceName, locDta[x].fontName);
			FontColor = 0;
			for (y = 0; y < 16; y++)
			{
				if (locDta[x].fontColor == y)
				{//color position in array holds its COLORREF color
					FontColor = Colors[y];
					break;
				}
			}
			////////////
			ShowEntry(x);
			////////////
//
			if (firstime)
			{//initialize font with first data entry's font
				firstime = FALSE;
				saveHeight = lf.lfHeight;
				saveWeight = lf.lfWeight;
				saveFamily = lf.lfPitchAndFamily;
				strcpy(saveName, lf.lfFaceName);
			}
//
		}
		lf.lfHeight = saveHeight;
		lf.lfWeight = saveWeight;
		lf.lfPitchAndFamily = saveFamily;
		strcpy(lf.lfFaceName, saveName);
	}
	else if (printoriginalform)
		MessageBox(hwnd, "First you have to Open .bmp file\nTo enter or print entries on .bmp file", "Oops", MB_OK);
	InvalidateRect(hwnd, NULL, FALSE);//BitBlt hdcMem to hdc
	UpdateWindow(hwnd);//necessary for editingoldentry
}

void GotEnter(void)
{
	int x, y;

	SendMessage(hEdit, WM_CLOSE, 0, 0);
	if (leftjustified)
		DestroyCaret();
	if (EditBuf[0] != 0)
	{
		for (x = 0; locDta[x].x != 0; x++)
			;//put new data at end
		for (y = 0; y < TWOHUNDRED; y++)
			locDta[x].Entry[y] = 0;//clean it up
		strcpy(locDta[x].Entry, EditBuf);
//		for (z = 0, y = 0; EditBuf[z] != 0; z++)
//			if (EditBuf[z] != ' ') // get rid of spaces
//				locDta[x].Entry[y++] = EditBuf[z];
		locDta[x].fontColor = 0;//in case color not found
		if (editingoldentry == FALSE)
		{//we already have FontColor or NewFontColor from WM_LBUTTONUP
			locDta[x].width = size.cx;
			locDta[x].height = size.cy;
			if (zoomedout) {
				locDta[x].width *= 2;
				locDta[x].height *= 2;
			}
			if (leftjustified == FALSE)
			{
				locDta[x].rightjustified = TRUE;
				locDta[x].x -= locDta[x].width;
			}
			else
				locDta[x].rightjustified = FALSE;
			locDta[x].fontHeight = -(lf.lfHeight)/2;
			locDta[x].fontWeight = lf.lfWeight;
			locDta[x].fontFamily = lf.lfPitchAndFamily;
			strcpy(locDta[x].fontName, lf.lfFaceName);
			if (vertical)
				locDta[x].vertical = TRUE;
			else
				locDta[x].vertical = FALSE;
			for (y = 0; y < 16; y++)
			{
				if (FontColor == Colors[y])
				{
					locDta[x].fontColor = (BYTE)y;
					break;
				}
			}
		}
		else
		{
			editingoldentry = FALSE;
			locDta[x] = tempDta;
		}
		if (fromesc == FALSE)
		{
			if (!zoomedout) {
				locDta[x].x = oldXpos + xLoc; // 663
				locDta[x].y = oldYpos + yLoc; // 456
			}
			else {
				locDta[x].x = (oldXpos*2) + xLoc;
				locDta[x].y = (oldYpos*2) + yLoc;
			}
		}
		hDtaFile = CreateFile(FileNameDta, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hDtaFile != INVALID_HANDLE_VALUE)
			SetFilePointer(hDtaFile, 0, NULL, FILE_END);
		else
			hDtaFile = CreateFile(FileNameDta, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		WriteFile(hDtaFile, &locDta[x], locDtaSize, &dwBytesWritten, NULL);
		CloseHandle(hDtaFile);

		num = 0xFFFFFFFF;
		////////////
		ShowEntry(x);
		///////////
		if (num != 0xFFFFFFFF)//if a formula was found
			///////////
			ReadBmpFile();
			///////////
		SetFocus(hwnd);
	}
	InvalidateRect(hwnd, NULL, FALSE);//BitBlt hdcMem to hdc
	UpdateWindow(hwnd);//necessary for editingoldentry
	EditBuf[0] = 0;
	editing = FALSE;
}

void ShowEntry(int x)
{
	int EntrySize;
	unsigned int y;

	if (locDta[x].vertical == TRUE)
	{
		lf.lfEscapement = lf.lfOrientation = 900;
		SetTextAlign(hdcMem, TA_LEFT|VTA_TOP);
	}
	else
	{
		lf.lfEscapement = lf.lfOrientation = 0;
		SetTextAlign(hdcMem, TA_LEFT|TA_TOP);
	}
	OrigObj = SelectObject(hdcMem, CreateFontIndirect(&lf));
	SetBkMode(hdcMem, TRANSPARENT);
	SetTextColor(hdcMem, FontColor);//only works if bmp file is in color

	for (EntrySize = 0; locDta[x].Entry[EntrySize] != 0; EntrySize++)
		;
	X = locDta[x].x;
	if (locDta[x].rightjustified)
		X -= locDta[x].width;
	if (locDta[x].Entry[0] != '#')//normal
		TextOut(hdcMem, X, locDta[x].y, locDta[x].Entry, EntrySize);
	else//if formula
	{
		EntryCount = 0;
		if (0xFFFFFFFF != (num = ReadFormula(&locDta[x])))
		{
			_itoa(num, FormulaResult, 10);
			if (num != 0)
				for (y = 1, FormulaSize = 0; y <= num; y *= 10)
					FormulaSize++;
			else
				FormulaSize = 1;
			FormulaXLoc = locDta[x].x;
			if (TRUE == locDta[x].rightjustified)
			{
				GetTextExtentPoint32(hdcMem, FormulaResult, FormulaSize, &fSize);
				FormulaXLoc -= fSize.cx;
			}
			TextOut(hdcMem, FormulaXLoc, locDta[x].y, FormulaResult, FormulaSize);
		}
		else
			TextOut(hdcMem, X, locDta[x].y, locDta[x].Entry, EntrySize);
	}
	SelectObject(hdcMem, OrigObj);
}

int ReadFormula(struct Data *Buf)
{//e.g. #3=#2+#1-100
	int w, x, y, z, OperatorType = 0;
	unsigned int num = 0xFFFFFFFF, prevNum;
	BOOL match;

	EntryCount++;
	if (EntryCount > 100)//infinite loop
		return num; 
	for (y = 0; Buf->Entry[y] != 0; y++)
		if (Buf->Entry[y] == '=')//find what Buf->Entry is equal to
			break;
	if (Buf->Entry[y] == 0)
		return num;//shouldn't happen
	for (y++; Buf->Entry[y] != 0; y++) {
		if (Buf->Entry[y] == '#') {//it equals another entry
			for (z = 0; locDta[z].x != 0; z++) {//e.g. compare #1 to #1=123 and replace #1 with 123
//				if (locDta[z].Entry[0] == ' ')
//					continue;
				if (locDta[z].Entry[0] == '#') {
					match = FALSE;
					for (w = y+1, x = 1; (Buf->Entry[w] == locDta[z].Entry[x]); w++, x++)
						;
					if (((Buf->Entry[w] == '+') || (Buf->Entry[w] == '-') || (Buf->Entry[w] == '*') || (Buf->Entry[w] == '/') || (Buf->Entry[w] == 0))
					 && ((locDta[z].Entry[x] == '='))) {
						match = TRUE;
					}
					if (match) {
						num = ReadFormula(&locDta[z]);//recursive
						if (num == 0xFFFFFFFF)
							return num;
						if (OperatorType != 0) {
							num = Operate(OperatorType, num, prevNum);
							if (num == 0xFFFFFFFF)
								return num;
							OperatorType = 0;
						}
						y = w-1;
						break;
					}
				}
			}
			if (match == FALSE) {
				return 0xFFFFFFFF;
			}
		}
		else if ((Buf->Entry[y] >= '0') && (Buf->Entry[y] <= '9')) {//first digit
			num = Buf->Entry[y] - '0';
			for (z = y+1; (Buf->Entry[z] >= '0') && (Buf->Entry[z] <= '9'); z++) {
				if (Buf->Entry[z] != ',') {
					num *= 10;
					num += Buf->Entry[z] - '0';
				}
			}
			if (OperatorType != 0) {
				num = Operate(OperatorType, num, prevNum);
				if (num == 0xFFFFFFFF) {
					return num;
				}
				OperatorType = 0;
			}
			y = z-1;
		}
		else if (Buf->Entry[y] == '+') {
			OperatorType = PLUS;
			prevNum = num;
		}
		else if (Buf->Entry[y] == '-') {
			OperatorType = MINUS;
			prevNum = num;
		}
		else if (Buf->Entry[y] == '*') {
			OperatorType = MUL;
			prevNum = num;
		}
		else if (Buf->Entry[y] == '/') {
			OperatorType = DIV;
			prevNum = num;
		}
	}//end of for (y++
	return num;
}

void ShowScannerData(void)
{
	if (bmih.biXPelsPerMeter != 0)
	{
		scannerdpi = (double)bmih.biXPelsPerMeter / 39.37;
		sprintf(Size, "Scanner resolution was %.0f dpi", scannerdpi);
		MessageBox(hwnd, Size, "Info", MB_OK);
	}
	else
		MessageBox(hwnd, "Scanner resolution is unknown,\nso printer output size may not be correct.", NULL, MB_OK);
}

int CALLBACK PDF2BMPProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y;
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
		if (Resolution)
			SetWindowText(hwndEdit, Resolution);
		bmpType = 2;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RADIO1:
			bmpType = 1;
			break;
		case IDC_RADIO2:
			bmpType = 2;
			break;
		case IDC_RADIO3:
			bmpType = 3;
			break;

		case IDOK:
			if (GetWindowText(hwndEdit, Resolution, 4) == 0)
			{
				SetFocus(hwndEdit);
				break;
			}
		for (x = 0, y = 18; Resolution[x] != 0; x++, y++)
				IniFile[y] = Resolution[x];
			IniFile[y] = 0;
			strcat(CurrentDirectory, "\\");
			strcat(CurrentDirectory, FillOutAFormIni);
			hFile = CreateFile(CurrentDirectory, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			WriteFile(hFile, IniFile, y, &dwBytesWritten, NULL);
			CloseHandle(hFile);
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

BOOL FindGhostScript(void)
{
	if (INVALID_HANDLE_VALUE == FindFirstFile(GSwin1, &fd) || (0 == (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)))
	{
		GSwin1[0x19]++; //0x19 is at the 5 in 9.05
		GSwin[0x19]++; 
		GhostScript[0x19]++;
		if (GSwin[0x19] == ':')
		{
			GSwin1[0x19] = '0';
			GSwin1[0x18]++;
			GSwin[0x19] = '0';
			GSwin[0x18]++;
			GhostScript[0x19] = '0';
			GhostScript[0x18]++;
			if (GSwin[0x18] == ':')
			{
				GSwin1[0x18] = '0';
				GSwin1[0x16]++;
				GSwin[0x18] = '0';
				GSwin[0x16]++;
				GhostScript[0x18] = '0';
				GhostScript[0x16]++;
			}
		}
		return TRUE;
	}
	return FALSE;
}
