#include <windows.h>
#include <stdio.h>//for sprintf
#include "resource.h"
#define LIGHTGRAY 0xD8E9EC
#define COLS 7
#define ROWS 6
#define FTTICKSPERDAY 60*60*24*(LONGLONG)10000000
#define BOXSIZE 232

char version[] = "Version 3.01  Sep 5, 2007  Doug Cox";
/*
struct SpecialDaysX {
	int specialDates;
	unsigned char birthday[40];
} SDx[17] = {0x00000000, "·Easter·",//MUST STAY AT SD[0]  · is ASCII 249
		  0x03010100, "·M.L.K. Day·",		//third Monday in January
		  0x03010200, "·President's Day·",	//third Monday in February	
		  0x01000400, "·Spring Ahead·",		//first Sunday in April
		  0x02000500, "·Mother's Day·",		//second Sunday in May
		  0x0F010500, "·Memorial Day·",		//last Monday in May
		  0x03000600, "·Father's Day·",		//third Sunday in June
		  0x01010900, "·Labor Day·",		//first Monday in September
		  0x02010A00, "·columbus day·",		//second Monday in October
		  0x0F000A00, "·Fall Back·",		//last Sunday in October
		  0x04040B00, "·Thanksgiving·",		//fourth Thursday in November
		  0x0000020E, "·Valentine's Day·",
		  0x00000311, "·St Patrick's Day·",
		  0x00000704, "·Fourth of July·",
		  0x00000B0B, "·veteran's day·",
		  0x00000A1F, "·Halloween·",
		  0x00000C19, "·Christmas·"};
*/
char Help[] = "\
Esc to exit\n\
Home to go to today\n\
Page-Up or Page-Down for previous or next month\n\
Ctrl-F or Alt-F to find\n\
F3 to find again\n\
\n\
Each date can hold 232 characters.\n\
\n\
Right-click on a box to create/delete a permanent annual event.\n\
There can only be one permanent annual event in a day,\n\
so you might have to combine them.\n\
\n\
Click on the blank button to create a new calendar.";

int xToday = 0xFF, yToday = 0xFF;
int specialDay, week, dayofweek, xBox, yBox;
int col, len, YearLoc, RightSideOfMonth;
int cxBlock, cyBlock;
DWORD SDSize;
DWORD NumOfFileNames;
DWORD PermanentDate, PermanentMonth;
char NewName[MAX_PATH];
char ButtonName[12];
HWND hwndButton[20], hwndHelp, hwndFind;
BOOL fromsyskeydown = FALSE, first = TRUE, fromfind = FALSE;
char Find[101];
char Tabbing[BOXSIZE];//for tabbing
unsigned char fdsa[BOXSIZE];
char *months[13] = {"", "JANUARY", "FEBRUARY", "MARCH", "APRIL", "MAY", "JUNE", "JULY", "AUGUST", "SEPTEMBER", "OCTOBER", "NOVEMBER", "DECEMBER"};
char *months2[13] = {"", "January", "February", "March", "April", "May", "June", "July", "August", "September", "October", "November", "December"};
char *WeekDays[7] = {"Sunday","Monday","Tuesday","Wednesday","Thursday","Friday","Saturday"};
char *Week[5] = {"First", "Second", "Third", "Fourth", "Fifth"};
char Year[] = {"XXXX"};

int v, w, y, z;
BOOL legit = TRUE, gotyear = FALSE;
char FileName[MAX_PATH] = "Calendar3.dta";
char FileNameBak[MAX_PATH] = "Calendar3.bak";
char CapitalFileName[MAX_PATH];
char FileNames[20][MAX_PATH];
char Calendar3Ini[MAX_PATH];// = "Calendar3.ini";
DWORD dwBytesRead, dwBytesWritten, fileSize, numofRecords, x;
HANDLE hFile, hFindFile;
HBRUSH hDialogBrush;

struct SpecialDays {
	int specialDates;
	unsigned char birthday[40];
} SD[365];

struct FileBuffer {
	struct FileBuffer *next;
	long int fulldate;
	char boxes[BOXSIZE];
} *temp, *cur, *prev, *new_node, *findnext, *head = NULL;

struct _SYSTEMTIME st, *lpSystemTime = &st;
struct _FILETIME ft, *lpFileTime= &ft;

RECT rect;
HWND hwnd, hwndChild[COLS][ROWS];
HINSTANCE hInst;
int dates[COLS][ROWS], specialFlag;
int PFM, date, dayofFirst, today, month, year, thismonth, thisyear, currentMonth, currentYear;
LARGE_INTEGER li;
int idFocus = -1;
int firstTime = 1;
BOOL gotini = FALSE, itsaved = TRUE, itstab = FALSE, saveit = FALSE;
HFONT hBigFont, hFont;
WIN32_FIND_DATA fd;
SIZE size;

void ReadIni(void);
void CalendarRead(void);
void CalendarWrite(void);
void GetFindDate(void);
void GetFirstDate(void);
void (*pGetDate)(void);
void PrevMonth(void);
void NextMonth(void);
void NullDate(void);
void SaveToMem(int, int);
void ClearBoxes(void);
void MakeSpecialDays(void);
void ChangeCalendar(DWORD);
void ClearSD(void);
BOOL FindIt(void);
int CALLBACK FindProc(HWND hwndNew, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK NewProc(HWND hwndNew, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK PermanentProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditProc(HWND, UINT, WPARAM, LPARAM);
WNDPROC Editproc[COLS*ROWS];

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR szAppName[] = TEXT ("Calendar3");
	HWND		hwnd;
	MSG			msg;
	WNDCLASS	wndclass;

	wndclass.style			= CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc	= WndProc;
	wndclass.cbClsExtra		= 0;
	wndclass.cbWndExtra		= 0;
	wndclass.hInstance		= hInstance;
	wndclass.hIcon			= LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor		= LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground  = (HBRUSH)GetSysColorBrush(COLOR_MENUBAR); // CreateSolidBrush(LIGHTGRAY);//(HBRUSH) GetStockObject (LTGRAY_BRUSH);
	wndclass.lpszMenuName   = NULL;
	wndclass.lpszClassName  = szAppName;

	hInst = wndclass.hInstance;
	pGetDate = GetFirstDate;

	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT ("error"), szAppName, MB_ICONERROR);
		return 0 ;
	}

	hwnd = CreateWindow (szAppName, szAppName,
		WS_CAPTION | WS_MINIMIZEBOX | WS_MAXIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	iCmdShow = SW_SHOWMAXIMIZED;	//Petzold, p.59
	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	HINSTANCE hInstance;
	HDC hdc;
	PAINTSTRUCT ps;
	int rectIncrement;
	LOGFONT lf;
	HGDIOBJ PrevObject, Pen1, Pen2;
	int xBeg, yBeg, xEnd, yEnd;
	static char SpecialAsciiDate[] = {"XX      T O D A Y"};
	static char AsciiDate[] = {"XX"};
	lf.lfHeight = 28;//30;
	lf.lfWidth = 17;//21;
	lf.lfEscapement = 0;
	lf.lfOrientation = 0;
	lf.lfWeight = 700;
	lf.lfItalic = 0;
	lf.lfUnderline = 0;
	lf.lfStrikeOut = 0;
	lf.lfCharSet = 0;
	lf.lfOutPrecision = 0;
	lf.lfClipPrecision = 0;
	lf.lfQuality = 0;
	lf.lfPitchAndFamily = FF_ROMAN;
	lf.lfFaceName[LF_FACESIZE] = 0;

	switch (message)
	{
	case WM_CREATE:
//		hFile = CreateFile("Calendar3x.ini", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
//		WriteFile(hFile, SDx, 17*sizeof(SDx[0]), &dwBytesWritten, NULL);
//		CloseHandle(hFile);

		hDialogBrush = GetSysColorBrush(COLOR_MENUBAR);
		hBigFont = CreateFontIndirect(&lf);
		hFont = GetStockObject(SYSTEM_FONT);
		for (x = 0; x < 20; x++)
			FileNames[x][0] = 0;
		hFindFile = FindFirstFile("*.*", &fd);
		if (INVALID_HANDLE_VALUE != hFindFile)
		{
			y = 0;
			while (FindNextFile(hFindFile, &fd))
			{
				if ((fd.cFileName[0] != '.') && (fd.cFileName[1] != '.'))
				{//not .. or .
					for (x = 0; fd.cFileName[x] != 0; x++)
					{
						if (fd.cFileName[x] == '.')
						{
							if ((fd.cFileName[x+1] == 'd') && (fd.cFileName[x+2] == 't') && (fd.cFileName[x+3] == 'a'))
							{
								strcpy(FileNames[y], fd.cFileName);
								y++;
							}
							break;//out of for (x = 0;
						}
					}
				}
			}
			FindClose(hFindFile);
		}
		for (x = 0; FileName[x] != '.'; x++)
		{
			if ((FileName[x] >= 'a') && (FileName[x] <= 'z'))
				CapitalFileName[x] = FileName[x] & 0xDF;
			else
				CapitalFileName[x] = FileName[x];
		}
		CapitalFileName[x] = 0;
		SetWindowText(hwnd, CapitalFileName);
		ReadIni();
		CalendarRead();

		hInstance = (HINSTANCE) GetWindowLong (hwnd, GWL_HINSTANCE);
		z = 0;//for child-window identifier
		for (y = 0 ; y < ROWS ; y++)
		{
			for (x = 0 ; x < COLS ; x++)
			{
				hwndChild[x][y] = CreateWindow (TEXT ("edit"), NULL,
					WS_CHILD | WS_VISIBLE | ES_MULTILINE | ES_WANTRETURN | ES_AUTOVSCROLL | ES_CENTER,
					0, 0, 0, 0,
					hwnd, (HMENU)z, hInst, NULL);
				SendMessage(hwndChild[x][y], EM_LIMITTEXT, 232, 0);//max chars
				Editproc[z] = (WNDPROC) SetWindowLong(hwndChild[x][y], GWL_WNDPROC, (LONG) EditProc);
				z++;
			}
		}
		NumOfFileNames = 0;
		for (x = 0, col = 5; (x < 20) && (FileNames[x][0] != 0); x++)
		{
			ButtonName[0] = ' ';
			ButtonName[1] = '&';
			ButtonName[2] = FileNames[x][0];
			ButtonName[3] = FileNames[x][1];
			ButtonName[4] = FileNames[x][2];
			ButtonName[5] = FileNames[x][3];
			ButtonName[6] = FileNames[x][4];
			ButtonName[7] = '.';
			ButtonName[8] = '.';
			ButtonName[9] = '.';
			ButtonName[10] = 0;
			hwndButton[x] = CreateWindow("BUTTON", ButtonName,
				WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON | BS_NOTIFY,
				col, 0, 56, 24,
				hwnd, (HMENU)x, hInst, NULL);
			col += 60;
			NumOfFileNames++;
		}
		if (x < 20)
		{
			hwndButton[x] = CreateWindow("BUTTON", NULL,
				WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				col, 0, 56, 24,
				hwnd, (HMENU)x, hInst, NULL);
			NumOfFileNames++;
		}
		return 0;

	case WM_SIZE:
		cxBlock = LOWORD (lParam) / COLS;
		cyBlock = ((HIWORD (lParam) / ROWS) - 8);
		for (y = 0; y < ROWS; y++)
		{
			for (x = 0; x < COLS; x++)
				MoveWindow (hwndChild[x][y], (x * cxBlock) + 3, (y * cyBlock) + 70, cxBlock - 4, cyBlock - 26, FALSE);
		}
		DestroyWindow(hwndHelp);
		DestroyWindow(hwndFind);
		first = TRUE;
		return 0;

	case WM_SETFOCUS:
		if (idFocus != -1)
		{
			SetFocus(hwndChild[idFocus % 7][idFocus / 7]);
			InvalidateRect(hwnd, &rect, FALSE);
			//UpdateWindow(hwnd);//this ruins timing of everthing
		}
		return 0;

	case WM_COMMAND:
		if (lParam == (long)hwndHelp)
			MessageBox(hwnd, Help, "Help", MB_OK);

		else if (lParam == (long)hwndFind)
			DialogBox(hInst, "FIND", NULL, FindProc);

		else if (HIWORD(wParam) == EN_KILLFOCUS)//from edit control
		{
			idFocus = LOWORD(wParam);//child-window identifier
			x = idFocus % 7;
			y = idFocus / 7;
			SaveToMem(x, y);
		}

		else if (HIWORD(wParam) == EN_CHANGE)
		{
			int id = LOWORD(wParam);
			x = id % 7;
			y = id / 7;
			if (itstab)
			{
				itstab = FALSE;
				SetWindowText(hwndChild[x][y], Tabbing);
				SaveToMem(x, y);
				if (GetKeyState(VK_SHIFT) < 0)
				{
					if (id)
						id--;
				}
				else if (id < 41)
						id++;
				x = id % 7;
				y = id / 7;
				SetFocus(hwndChild[x][y]);
			}
		}
		else
		{
			for (x = 0; x <= NumOfFileNames; x++)
			{
				if (lParam == (long)hwndButton[x])
				{//button pressed
					if (FileNames[x][0])
					{
						legit = TRUE;
						gotini = FALSE;
						for (y = 0; (y < 20) && (FileNames[y][0] != 0); y++)
						{
							if ((hwndButton[y]) && (strcmp(FileNames[y], FileName)))
							{
								ChangeCalendar(x);

								SetWindowText(hwnd, CapitalFileName);
								ReadIni();
								CalendarRead();
								break;
							}
						}
					}
					else//blank button was clicked on
					{
						int xNew = x;

						if (DialogBox(hInst, "NEW", NULL, NewProc))
						{
							ButtonName[0] = ' ';
							ButtonName[1] = NewName[0];
							ButtonName[2] = NewName[1];
							ButtonName[3] = NewName[2];
							ButtonName[4] = NewName[3];
							ButtonName[5] = NewName[4];
							ButtonName[6] = '.';
							ButtonName[7] = '.';
							ButtonName[8] = '.';
							ButtonName[9] = 0;
							SetWindowText(hwndButton[xNew], ButtonName);
							for (x = 0; (NewName[x] != 0) && (NewName[x] != '.'); x++)
								;
							if ((NewName[x] == 0) || (NewName[x+1] != 'd') || (NewName[x+2] != 't') || (NewName[x+3] != 'a'))
							{
								NewName[x++] = '.';
								NewName[x++] = 'd';
								NewName[x++] = 't';
								NewName[x++] = 'a';
								NewName[x] = 0;
							}
							if (xNew < 20)
							{
								col += 60;
								hwndButton[xNew] = CreateWindow("BUTTON", NULL,
									WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
									col, 0, 56, 24,
									hwnd, (HMENU)6, hInst, NULL);
								NumOfFileNames++;
								col += 60;
								strcpy(FileNames[NumOfFileNames], NewName);

								ChangeCalendar(NumOfFileNames);

								for (x = 0; FileName[x] != '.'; x++)
									Calendar3Ini[x] = FileName[x];
								Calendar3Ini[x++] = FileName[x];
								Calendar3Ini[x++] = 'i';
								Calendar3Ini[x++] = 'n';
								Calendar3Ini[x++] = 'i';
								Calendar3Ini[x] = 0;
								SetWindowText(hwnd, CapitalFileName);
								MoveFile(FileName, FileNameBak);//temporarily rename it to .bak
								hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_NEW, 0, NULL);
								CloseHandle (hFile);
							}
						}
					}
					firstTime = 1;
					SetFocus(hwnd);
					break;
				}//end of if (lParam == (long)hwndButton[x])
			}//end of for (x = 0; x <= NumOfFileNames; x++)
		}
		return 0;

	case WM_PAINT:
		pGetDate();

		hdc = BeginPaint (hwnd, &ps);
		GetClientRect(hwnd, &rect);
//		SetBkColor(hdc, LIGHTGRAY);
		SetBkMode(hdc, TRANSPARENT);
		rect.top = 0;
		x = rect.bottom;
		rect.bottom = 25;
//		hDialogBrush = GetSysColorBrush(COLOR_MENUBAR);
		FillRect(hdc, &rect, (HBRUSH)GetSysColorBrush(COLOR_MENUBAR)); // CreateSolidBrush(LIGHTGRAY));//month area
		rect.bottom = x;
		rect.top = 26;
		rectIncrement = rect.right / 7;
		rect.right = rectIncrement;
		SelectObject(hdc, hFont);
		for (x = 0; x < 7; x++)
		{
			DrawText(hdc, WeekDays[x], -1, &rect, DT_CENTER);
			rect.left += rectIncrement;
			rect.right += rectIncrement;
		}
		GetClientRect(hwnd, &rect);
//		SetBkColor(hdc, 0xFFFFFF);
		gotyear = FALSE;
		if (gotini)
			MakeSpecialDays();
		for (y = 0; y < ROWS; y++)
		{
			for (x = 0; x < COLS; x++)
			{
				xBeg = (x * cxBlock) + 2;
				yBeg = (y * cyBlock) + 47;
				xEnd = cxBlock - 2;
				yEnd = cyBlock - 2;
				Pen1 = CreatePen(PS_SOLID, 1, RGB(0x030,0x030,0x030));
				PrevObject = SelectObject(hdc, Pen1);
				MoveToEx(hdc, xBeg, yBeg + yEnd, NULL);
				LineTo(hdc, xBeg, yBeg);
				LineTo(hdc, xBeg + xEnd, yBeg);
				Pen2 = CreatePen(PS_SOLID, 1, RGB(0x0FF,0x0FF,0x0FF));
				SelectObject(hdc, Pen2);
				DeleteObject(Pen1);
				LineTo(hdc, xBeg + xEnd, yBeg + yEnd);
				LineTo(hdc, xBeg, yBeg + yEnd);
				Rectangle(hdc, xBeg + 1, yBeg + 1, xBeg + xEnd - 1, yBeg + 23);	//white background at top of box
				SelectObject(hdc, PrevObject);
				DeleteObject(Pen2);

				date = lpSystemTime->wDay;
				_itoa(date, AsciiDate,10);
				month = lpSystemTime->wMonth;
				year = (lpSystemTime->wYear);
				if ((date == 1) && (gotyear == FALSE))
				{
					_itoa(year, Year, 10);
					gotyear = TRUE;
				}
				dates[x][y] = ((((year-1900) << 8) | month) << 8) | date;//year|month|date for linked-list of FileBuffer->fulldate

				fdsa[0] = 0;//flag
				for (z = 0; z < (int)SDSize; z++)
				{
					if ((SD[z].specialDates & 0xFFFF) == (dates[x][y] & 0xFFFF))
					{
						for (v = 0, w = 0; SD[z].birthday[v] != 0; v++, w++)
							fdsa[w] = SD[z].birthday[v];
						fdsa[w] = 0;
					}
				}

				if ((date == today) && (month == thismonth) && ((year) == thisyear))
				{
					if (firstTime == 1)
					{//to put cursor at TODAY
						firstTime = 0;
						xToday = x;
						yToday = y;
						idFocus = (y * 7) + x; 
					}
					_itoa(date, SpecialAsciiDate,10);
					if (date < 10)
						SpecialAsciiDate[1] = ' ';	// _itoa put 0 there, therefore TODAY wouldn't show
					else
						SpecialAsciiDate[2] = ' ';
					SetTextColor(hdc, RGB(0xff,00,00));	//red
					if (fdsa[0] == 0)
						TextOut(hdc, (x * cxBlock) + 10, (y * cyBlock) + 50, SpecialAsciiDate, lstrlen(SpecialAsciiDate));
					else
					{
						TextOut(hdc, (x * cxBlock) + 10, (y * cyBlock) + 50, AsciiDate, lstrlen(AsciiDate));
						TextOut(hdc, (x * cxBlock) + 36, (y * cyBlock) + 50, fdsa, w);
					}
					SetTextColor(hdc, RGB(0,0,0));
				}
				else
				{
					TextOut(hdc, (x * cxBlock) + 10, (y * cyBlock) + 50, AsciiDate, lstrlen(AsciiDate));
					if (fdsa[0] != 0)
						TextOut(hdc, (x * cxBlock) + 36, (y * cyBlock) + 50, fdsa, w);
				}

				li.QuadPart += FTTICKSPERDAY;	//increment FILETIME by 1 day
				ft = * (FILETIME *) &li;
				FileTimeToSystemTime(lpFileTime, lpSystemTime);	//update date
			}
		}

//put month at top
		SelectObject(hdc, hBigFont);
//		SetBkColor(hdc, LIGHTGRAY);
		SetBkMode(hdc, TRANSPARENT);
		SetTextColor(hdc, RGB(0xff,00,00));//red
		len = lstrlen(months[currentMonth]);
		DrawText(hdc, months[currentMonth], len, &rect, DT_CENTER);
		GetTextExtentPoint(hdc, months[currentMonth], len, &size);
		RightSideOfMonth = (rect.right/2)+(size.cx/2);
		YearLoc = rect.right-150;
		if (first)
		{
			first = FALSE;
			hwndHelp = CreateWindow("BUTTON", "&Help",
				WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				YearLoc-100, 0, 50, 24,
				hwnd, (HMENU)7, hInst, NULL);
			hwndFind = CreateWindow("BUTTON", "&Find",
				WS_CHILD | WS_VISIBLE | BS_PUSHBUTTON,
				YearLoc-155, 0, 50, 24,
				hwnd, (HMENU)8, hInst, NULL);
		}
		TextOut(hdc, YearLoc, 0, Year, lstrlen(Year));
		SetTextColor(hdc, RGB(0,0,0));
		SetBkColor(hdc, 0xFFFFFF);
		SelectObject(hdc, hFont);

		if (legit)
		{
			legit = FALSE;
//put saved data on screen
			if ((head != NULL) && (itsaved))
			{	
				for (cur = head; (cur != NULL) && (cur->fulldate < dates[0][0]); cur = cur->next)
					;
				if (cur != NULL)
				{
					temp = cur;
					for (y = 0; y < ROWS; y++)
						for (x = 0; x < COLS; x++)
						{
							for (cur = temp; (cur != NULL) && (cur->fulldate <= dates[6][5]); cur = cur->next)
							{
								if (cur->fulldate == dates[x][y])
									SetWindowText(hwndChild[x][y], cur->boxes);
							}
						}
				}
				itsaved = FALSE;
			}
/*
//put special days on screen
			if (gotini)
			{
				MakeSpecialDays();
				for (y = 0; y < ROWS; y++)
				{
					for (x = 0; x < COLS; x++)
					{
						for (z = 0; z < (int)SDSize; z++)
						{
							if ((SD[z].specialDates & 0xFFFF) == (dates[x][y] & 0xFFFF))
							{
								for (v = 0, w = 0; SD[z].birthday[v] != 0; v++, w++)
									fdsa[w] = SD[z].birthday[v];
								fdsa[w++] = '\r';
								fdsa[w++] = '\n';
								fdsa[w] = 0;//but don't increment w
								if (GetWindowText(hwndChild[x][y], &fdsa[w], BOXSIZE))
								{
									if ('\xb7' == fdsa[w]);//first marker
									{//remove previously saved special entry
										z = w;
										for (w++; (fdsa[w] != 0) && (fdsa[w] != '\xb7'); w++)
											;//now at the ending marker
looping:								w++;
										if (fdsa[w] == '\r')
											w++;
										if (fdsa[w] == '\n')
											w++;
										if (fdsa[w] == '\xB7')//another first marker
											goto looping;
										fdsa[w] = 0;
										for (z++; fdsa[z] != 0; w++, z++)
											fdsa[z] = fdsa[w];//remove special data
										fdsa[z] = 0;
									}
								}
								SetWindowText(hwndChild[x][y], fdsa);
							}
						}
					}
				}
			}
*/
			if (xToday != 0xFF)
			{
				SetFocus(hwndChild[xToday][yToday]);
				xToday = 0xFF;
			}
			if (fromfind)
			{
				fromfind = FALSE;
				SendMessage(hwndChild[idFocus%7][idFocus/7], EM_SETSEL, 0, -1);
				SetFocus(hwndChild[idFocus%7][idFocus/7]);
			}
			pGetDate = NullDate;
		}
		EndPaint (hwnd, &ps);
		return 0;

	case WM_DESTROY:
		DeleteObject(hBigFont);
		MoveFile(FileNameBak, FileName);//rename FileNameBak to FileName
		if (saveit)
			CalendarWrite();
//		if (head == NULL)//file is empty
//			DeleteFile(FileName);
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}


//sub-class procedure
LRESULT CALLBACK EditProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int daysinmonth[13] = {0, 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
	DWORD i;
	static char asdf[BOXSIZE];
	BOOL specialday;

	int id = GetWindowLong (hwnd, GWL_ID);

	if (message == WM_RBUTTONDOWN)
	{//to create permanent annual event
		xBox = id % 7;
		yBox = id / 7;
		specialday = FALSE;
		week = dayofweek = 0;
		if (0 == (year % 4))//leap year
			daysinmonth[2] = 29;
		if (id < dayofFirst)
		{
			PermanentMonth = currentMonth-1;
			PermanentDate = daysinmonth[currentMonth-1] - (dayofFirst - (id+1));
		}
		else if ((id+1) > (daysinmonth[currentMonth]+dayofFirst))
		{
			PermanentMonth = currentMonth+1;
			PermanentDate = id + 1 - dayofFirst - daysinmonth[currentMonth];
		}
		else
		{
			PermanentMonth = currentMonth;
			PermanentDate = id + 1 - dayofFirst;
			dayofweek = id % 7;
			week = id / 7;
			if (dayofweek < dayofFirst)
				week--;
		}
		specialDay = ((week+1) << 24) | (dayofweek << 16) | (PermanentMonth << 8) | PermanentDate;
		for (i = 1; i < SDSize; i++) // Easter is at SD[0]
		{
			if ((SD[i].specialDates & 0xFFFF) == (specialDay & 0xFFFF))
			{//a special date was found at this date
				specialday = TRUE;
				if (IDYES == MessageBox(hwnd, SD[i].birthday, "  DELETE", MB_YESNO|MB_DEFBUTTON2))
				{
					BOOL got2 = FALSE;

					for ( ; i < SDSize; i++)
						SD[i] = SD[i+1];
					SDSize--;
/*
					GetWindowText(hwndChild[xBox][yBox], asdf, BOXSIZE);
					for (i = 0; asdf[i] != 0; i++)
					{
						if (asdf[i] == '\xB7')
						{
							if (got2)
							{
								i++;
								if (asdf[i] == '\r')
									i++;
								if (asdf[i] == '\n')
									i++;
								break;
							}
							got2 = TRUE;
						}
					}
					SendMessage(hwndChild[xBox][yBox], EM_SETSEL, 0, -1);
					SendMessage(hwndChild[xBox][yBox], WM_CLEAR, 0, 0);
					if (asdf[i] != 0)
						SetWindowText(hwndChild[xBox][yBox], &asdf[i]);
					SaveToMem(xBox, yBox);
*/
					saveit = TRUE;
					break;
				}
			}
		}
		if (specialday == FALSE)
		{//no special entry found at this date
			if (DialogBox(hInst, "PERMANENT", NULL, PermanentProc))
				InvalidateRect(hwnd, &rect, FALSE);
		}
	}

	else if (message == WM_SYSKEYDOWN)
		if (wParam == 'H') {
			MessageBox(hwnd, Help, "Help", MB_OK);
			return 0;
		}
		else if (wParam == 'F')
			DialogBox(hInst, "FIND", NULL, FindProc);
		else
			return 0; // don't let it do anything


	else if (message == WM_KEYDOWN)
	{
		switch (wParam)
		{
		case 'F':
			if (GetKeyState(VK_CONTROL) & 0x8000) // Ctrl is pressed
				DialogBox(hInst, "FIND", NULL, FindProc);
			break;

		case 'H':
			if (GetKeyState(VK_CONTROL) & 0x8000) // Ctrl is pressed
				MessageBox(hwnd, Help, "Help", MB_OK);
			break;

		case VK_F3:
			fromfind = TRUE;//double-use flag
			if (FindIt())
			{
				SetFocus(GetParent(hwnd));
				ClearBoxes();
			}
			break;

		case VK_TAB:
			x = id % 7;
			y = id / 7;
			SendMessage(hwndChild[x][y], EM_SETSEL, -1, -1); // because Windows will add a tab to selected text
			GetWindowText(hwndChild[x][y], Tabbing, BOXSIZE);//save it for SetWindowText
			itstab = TRUE;
			legit = TRUE;
			break;

		case VK_NEXT:
			pGetDate = NextMonth;
			idFocus = id;
			SetFocus(GetParent(hwnd));
			ClearBoxes();
			legit = TRUE;
			break;

		case VK_PRIOR:
			pGetDate = PrevMonth;
			idFocus = id;
			SetFocus(GetParent(hwnd));
			ClearBoxes();
			legit = TRUE;
			break;

		case VK_HOME:
			firstTime = 1;
			pGetDate = GetFirstDate;
			idFocus = id;
			SetFocus(GetParent(hwnd));
			ClearBoxes();
			legit = TRUE;
			break;

		case VK_ESCAPE:
			idFocus = id;
			SetFocus(GetParent(hwnd));
			SendMessage(GetParent(hwnd), WM_DESTROY, wParam, lParam);
			legit = TRUE;
			break;

		case VK_F1:
		case VK_UP:
		case VK_DOWN:
		case VK_LEFT:
		case VK_RIGHT:
		case VK_SHIFT:
		case VK_CONTROL:
			break;
		default:
			saveit = TRUE;
			break;
		}
	}
	return CallWindowProc(Editproc[id], hwnd, message, wParam, lParam);
}

void GetFindDate(void)
{
	idFocus = lpSystemTime->wDay;
	currentMonth = lpSystemTime->wMonth;
	currentYear = lpSystemTime->wYear;
	if (lpSystemTime->wDay != 1)
	{// find day of week 1st is on (subtract date (in FileTime format) from FileTime)
		SystemTimeToFileTime(lpSystemTime, lpFileTime);
		li = *(LARGE_INTEGER *) &ft;
		li.QuadPart -= (lpSystemTime->wDay - 1) * FTTICKSPERDAY;
		ft = *(FILETIME *) &li;
		FileTimeToSystemTime(lpFileTime, lpSystemTime);
	}// now go to 1st date on calendar
	dayofFirst = lpSystemTime->wDayOfWeek;
	idFocus += (dayofFirst-1);
	SystemTimeToFileTime(lpSystemTime, lpFileTime);
	li = *(LARGE_INTEGER *) &ft;
	li.QuadPart -= dayofFirst * FTTICKSPERDAY;
	ft = *(FILETIME *) &li;//0 day of first week
	FileTimeToSystemTime(lpFileTime, lpSystemTime);
	fromfind = TRUE;
}

void GetFirstDate(void)
{
	GetLocalTime(lpSystemTime);
	today = lpSystemTime->wDay;
	thismonth = lpSystemTime->wMonth;
	thisyear = lpSystemTime->wYear;
	if (today != 1)
	{// find day of week 1st is on (subtract date (in FileTime format) from FileTime)
		SystemTimeToFileTime(lpSystemTime, lpFileTime);
		li = *(LARGE_INTEGER *) &ft;
		li.QuadPart -= (today - 1) * FTTICKSPERDAY;
		ft = *(FILETIME *) &li;
		FileTimeToSystemTime(lpFileTime, lpSystemTime);
	}// now go to 1st date on calendar
	dayofFirst = lpSystemTime->wDayOfWeek;
	currentMonth = lpSystemTime->wMonth;
	currentYear = lpSystemTime->wYear;
	SystemTimeToFileTime(lpSystemTime, lpFileTime);
	li = *(LARGE_INTEGER *) &ft;
	li.QuadPart -= dayofFirst * FTTICKSPERDAY;
	ft = *(FILETIME *) &li;//0 day of first week
	FileTimeToSystemTime(lpFileTime, lpSystemTime);
}

void PrevMonth(void)
{
	li = *(LARGE_INTEGER *) &ft;
	li.QuadPart -= 43 * FTTICKSPERDAY;	//go to just before first date on screen
	ft = *(FILETIME *) &li;
	FileTimeToSystemTime(lpFileTime, lpSystemTime);
	li.QuadPart -= ((lpSystemTime->wDay) - 1)* FTTICKSPERDAY;	//go to first of previous month
	ft = *(FILETIME *) &li;
	FileTimeToSystemTime(lpFileTime, lpSystemTime);
	dayofFirst = lpSystemTime->wDayOfWeek;
	currentMonth = lpSystemTime->wMonth;
	currentYear = lpSystemTime->wYear;
	SystemTimeToFileTime(lpSystemTime, lpFileTime);
	li = *(LARGE_INTEGER *) &ft;
	li.QuadPart -= dayofFirst * FTTICKSPERDAY;
	ft = *(FILETIME *) &li;
	FileTimeToSystemTime(lpFileTime, lpSystemTime);
}

void NextMonth(void)
{
	li = *(LARGE_INTEGER *) &ft;
	li.QuadPart -= ((lpSystemTime->wDay) - 1)* FTTICKSPERDAY;
	ft = *(FILETIME *) &li;
	FileTimeToSystemTime(lpFileTime, lpSystemTime);
	dayofFirst = lpSystemTime->wDayOfWeek;
	currentMonth = lpSystemTime->wMonth;
	currentYear = lpSystemTime->wYear;
	SystemTimeToFileTime(lpSystemTime, lpFileTime);
	li.QuadPart -= dayofFirst * FTTICKSPERDAY;
	ft = *(FILETIME *) &li;
	FileTimeToSystemTime(lpFileTime, lpSystemTime);
}

void NullDate(void)
{
	li = *(LARGE_INTEGER *) &ft;
	li.QuadPart -= 42 * FTTICKSPERDAY;
	ft = *(FILETIME *) &li;
	FileTimeToSystemTime(lpFileTime, lpSystemTime);
}

void CalendarRead(void)
{
	DWORD x, dwBytesRead, fileSize; 
	HANDLE hFile;

	numofRecords = 0;
	if (INVALID_HANDLE_VALUE != FindFirstFile(FileNameBak, &fd))
		MoveFile(FileNameBak, FileName);//Calendar3 didn't close properly previously (FileNameBak shouldn't exist)
	if ((hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
	{
		if (fileSize = GetFileSize(hFile, NULL))
		{
			numofRecords = (fileSize / sizeof(struct FileBuffer));//108
			for (x = 0; x < numofRecords; x++)
			{
				cur = (struct FileBuffer*) calloc(1, sizeof(struct FileBuffer));
				ReadFile(hFile, cur, sizeof(struct FileBuffer), &dwBytesRead, NULL);//108
				if (x == 0)
					head = cur;
				else
					temp->next = cur;
				temp = cur;
			}
		}
		CloseHandle (hFile);
		MoveFile(FileName, FileNameBak);//temporarily rename it to .bak
	}
	else
	{
		hFile = CreateFile(FileName, GENERIC_READ, FILE_SHARE_READ, NULL, CREATE_NEW, 0, NULL);
		CloseHandle (hFile);
	}
}

void CalendarWrite(void)
{
	HANDLE hFile;
	int okflg, x, y;

	saveit = FALSE;
	hFile = CreateFile(FileName, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	cur = head;
	while (cur != NULL)
	{
		okflg = FALSE;
		y = 0;
		for (x = 0; x < BOXSIZE; x++)
		{
			if (cur->boxes[x] == '\xB7')//183
			{
				y++;
				if (y == 2)
				{
					x++;
					if (cur->boxes[x] == '\r')
						x++;
					if (cur->boxes[x] == '\n')
						x++;
					for (x++, y = 0; cur->boxes[x] != 0; x++, y++)
						cur->boxes[y] = cur->boxes[x];//remove special data
					cur->boxes[y] = 0;
					y = 0;
				}
			}
			if ((y == 0) && (((cur->boxes[x] >= 'a') && (cur->boxes[x] <= 'z')) ||  ((cur->boxes[x] >= 'A') && (cur->boxes[x] <= 'Z'))))
 			{//if no special day or special day plus other entry
				okflg = TRUE;
				break;
			}
		}
		temp = cur->next;
		cur->next = 0;
		if (okflg)
			WriteFile(hFile, cur, sizeof(*cur), &dwBytesWritten, NULL);
		free(cur);
		cur = temp;
	}
	CloseHandle(hFile);
	if (SD[1].specialDates)
	{
		for (x = 0; x < (int)SDSize; x++)
		{
			if (SD[x].birthday[0] == 0xB7)
			{
				for (y = 0; (y < 40) && (SD[x].birthday[y] != 0); y++)
					SD[x].birthday[y] = SD[x].birthday[y+1];
				SD[x].birthday[y-2] = 0;//0xB7
			}
		}
		hFile = CreateFile(Calendar3Ini, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		WriteFile(hFile, SD, SDSize*sizeof(SD[0]), &dwBytesWritten, NULL);
		CloseHandle(hFile);
	}
}

void SaveToMem(int x, int y)
{
	int i;
	char DateBoxBuffer[BOXSIZE];

	if (GetWindowText(hwndChild[x][y], DateBoxBuffer, BOXSIZE))
	{
		for (cur = head, prev = NULL; (cur != NULL) && (dates[x][y] > cur->fulldate); prev = cur, cur = cur->next)
			;
		if ((cur != NULL) && (dates[x][y] == cur->fulldate))	//if it wasn't empty
		{
			for (i = 0; i < BOXSIZE; i++)
				cur->boxes[i] = 0;
			strcpy(cur->boxes, DateBoxBuffer);	//update it
		}
		else
		{
			new_node = (struct FileBuffer*) calloc(1, sizeof(struct FileBuffer));
			new_node->fulldate = dates[x][y];
			strcpy(new_node->boxes, DateBoxBuffer);
			new_node->next = cur;
			if (prev == NULL)
				head = new_node;
			else 
				prev->next = new_node;
		}
	}
	else//if empty
	{
		for (cur = head, prev = NULL; (cur != NULL) && (cur->fulldate < dates[x][y]); prev = cur, cur = cur->next)
			;
		if ((cur != NULL) && (dates[x][y] == cur->fulldate))
		{
			if (prev != NULL)
				prev->next = cur->next;
			else
				head = cur->next;
			free(cur);
		}
	}
	itsaved = TRUE;
}

void ClearBoxes(void)
{
	int x, y;

	for (y = 0; y < ROWS; y++)
	{
		for (x = 0; x < COLS; x++)
		{
			SendMessage(hwndChild[x][y], EM_SETSEL, 0, -1);
			SendMessage(hwndChild[x][y], WM_CLEAR, 0, 0);
		}
	}
}

void MakeSpecialDays(void)
{//convert 3rd Sunday in May, etc, to specific date in SpecialDays array
	int EasterTable[] = {14,3,23,11,31,18,8,28,16,5,25,13,2,22,10,30,17,7,27};
	int x, specialDayMonth, dayofWeek, whichWeek, dayof31st;
	int PFMday, EasterDate, EasterMonth;
	SD[0].specialDates = 0;	//initialize Easter array
	for (x = 1; (SD[x].specialDates != 0); x++)//don't start with Easter //&& (x < 100)
	{
		if (SD[x].specialDates > 0xFFFF)			//if second Sunday, etc
		{
			SD[x].specialDates &= 0xFFFFFF00;		//clear date byte
			specialDayMonth = (SD[x].specialDates & 0x0000FF00) >> 8;
			if (specialDayMonth == currentMonth)
			{
				whichWeek = (SD[x].specialDates & 0xFF000000) >> 24;
				dayofWeek = (SD[x].specialDates & 0x00FF0000) >> 16;
				if (whichWeek != 5)//was 0x0F (flag)
				{
					if (dayofFirst > dayofWeek)
						dayofWeek += 7;
					SD[x].specialDates |= (dayofWeek - dayofFirst) + 1 + ((whichWeek - 1) * 7);
				}
				else if (currentMonth == 5)//May
				{//if whichWeek == 5, it means the last week of the month
					dayof31st = (dayofFirst + 2) % 7;
					if (dayof31st)
						SD[x].specialDates |= (31 - (dayof31st - dayofWeek));
					else
						SD[x].specialDates |= 25;
				}
				else
				{
					dayof31st = (dayofFirst + 2) % 7;
					SD[x].specialDates |= (31 - (dayof31st - dayofWeek));
				}
			}
			else if ((currentMonth == 6) && (SD[x].specialDates == 0x0F010500))	//to show Memorial Day in June
			{
				if (dayofFirst != 0)
					dayof31st = dayofFirst - 1;
				else dayof31st = 6;
				if (dayof31st)
					SD[x].specialDates |= (31 - (dayof31st - 1));
				else SD[x].specialDates |= 25;
			}
			else if ((currentMonth == 8) && (SD[x].specialDates == 0x01010900))	//to show Labor Day in August
			{
				dayof31st = (dayofFirst + 2) % 7;
				if (dayof31st != 0)
					SD[x].specialDates |= 8 - dayof31st;
				else
					SD[x].specialDates |= 1;
			}
		}
	}
	SDSize = x;
//Easter is first Sunday after Paschal Full Moon
	if ((currentMonth == 3) || (currentMonth == 4))
	{		
/*
		x = currentYear % 19;
		g = x + 1;
		h = currentYear / 100;
		c = (h / 4) + (8 * (h + 11) / 25) - h;
		s = (11 * g + c) % 30;
		PFM = 19 - s;
*/
		PFM = EasterTable[currentYear % 19];//Paschal Full Moon
//		if (PFM == 0) PFM = 31;
		if (PFM > 19) //it's in March
		{
			if (currentMonth == 3)
				PFMday = (dayofFirst - 1 + PFM) % 7;
			else //currentMonth == 4
			{
				PFMday = dayofFirst - ((32 - PFM) % 7);
				if (PFMday < 0)
					PFMday += 7;
			}
			EasterDate = PFM + (7 - PFMday);
			if (EasterDate > 31)
			{
				EasterDate -= 31;
				EasterMonth = 4;
			}
			else 
				EasterMonth = 3;
		}
		else if (PFM > 0)//it's in April
		{
//			if (PFM == 19) PFM = 18;
//			else if ((PFM == 18) && (g >= 12)) PFM = 17;
			if (currentMonth == 4)
				PFMday = (dayofFirst + (PFM - 1)) % 7;
			else //currentMonth == 3
				PFMday = (((dayofFirst + 3) % 7) + (PFM - 1)) %7;
			EasterDate = PFM + (7 - PFMday);
			EasterMonth = 4;
		}
			SD[0].specialDates |= (EasterMonth << 8);
			SD[0].specialDates |= EasterDate;
	}
}

int CALLBACK NewProc(HWND hwndNew, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndNew, IDC_EDIT1);
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (GetWindowText(hwndEdit, NewName, MAX_PATH))
			{
				EndDialog (hwndNew, TRUE);
				return TRUE;
			}
			else
				return FALSE;
		case IDCANCEL:
			EndDialog (hwndNew, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK PermanentProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char Event[BOXSIZE];
	static int eventdate, len;
	static char asdf[36];
	static HWND hwndEdit, hwndRadio1, hwndRadio2, hwndRadio3;
	static HDC hdc;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndRadio1 = GetDlgItem(hwndDlg, IDC_RADIO1);
		hwndRadio2 = GetDlgItem(hwndDlg, IDC_RADIO2);
		hwndRadio3 = GetDlgItem(hwndDlg, IDC_RADIO3);
		sprintf(asdf, "%s %i", months2[PermanentMonth], PermanentDate);
		SetWindowText(hwndRadio1, asdf);
		sprintf(asdf, "%s %s in %s", Week[week], WeekDays[dayofweek], months2[PermanentMonth]);
		SetWindowText(hwndRadio2, asdf);
		sprintf(asdf, "Last %s in %s", WeekDays[dayofweek], months2[PermanentMonth]);
		if (week >= 3)//it's zero-based
			SetWindowText(hwndRadio3, asdf);
		else
			SetWindowText(hwndRadio3, "--------");
		CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO3, IDC_RADIO1);
		eventdate = specialDay & 0xFFFF;
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_RADIO1:
			eventdate = specialDay & 0xFFFF;
			break;
		case IDC_RADIO2:
			eventdate = specialDay;
			break;
		case IDC_RADIO3:
			if (week >= 3)//it's zero-based
			{
				specialDay |= 0x05000000;
				eventdate = specialDay;
			}
			else
				eventdate = 0;
			break;

		case IDOK:
			if (len = GetWindowText(hwndEdit, Event, 40))
			{
				if (eventdate)
				{
					strcpy(SD[SDSize].birthday, Event);
					SD[SDSize].specialDates = eventdate;
					SDSize++;
					saveit = TRUE;
					EndDialog (hwndDlg, TRUE);
					return TRUE;
				}
			}
			break;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

void ReadIni(void)
{
	int x;

	for (x = 0; FileName[x] != '.'; x++)
		Calendar3Ini[x] = FileName[x];
	Calendar3Ini[x++] = FileName[x];
	Calendar3Ini[x++] = 'i';
	Calendar3Ini[x++] = 'n';
	Calendar3Ini[x++] = 'i';
	Calendar3Ini[x] = 0;

	gotini = FALSE;
	if ((hFile = CreateFile(Calendar3Ini, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)) != INVALID_HANDLE_VALUE)
	{
		if (fileSize = GetFileSize(hFile, NULL))
		{
			ReadFile(hFile, SD, fileSize, &dwBytesRead, NULL);
			gotini = TRUE;
		}
		CloseHandle(hFile);
	}
	else
		ClearSD();
}

int CALLBACK FindProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
//		SetWindowText(hwndEdit, Find);
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (GetWindowText(hwndEdit, Find, 100) == 0)
				break;
			findnext = head;
			FindIt();
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			findnext = NULL;
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

BOOL FindIt(void)
{
	int a, b, x, y, C, M, Y, L;
	const int month_table[13] = {0, 0, 3, 3, 6, 1, 4, 6, 2, 5, 0, 3, 5};//first zero is to make January 1 instead of 0
	
	for (cur = findnext; cur != NULL; cur = cur->next)
	{
		for (y = 0; cur->boxes[y] != 0; y++)
		{
			for (x = 0; Find[x] != 0; x++, y++)
			{
				a = Find[x];
				if ((a >= 'a') && (a <= 'z'))
					a &= 0xDF;//make uppercase
				b = cur->boxes[y];
				if ((b >= 'a') && (b <= 'z'))
					b &= 0xDF;
				if (a != b)
					break;
			}
			if (Find[x] == 0)
			{
				findnext = cur->next;
				lpSystemTime->wYear = 1900 + (WORD)(cur->fulldate >> 16);
				lpSystemTime->wMonth = (WORD)((cur->fulldate >> 8) & 0xFF);
				lpSystemTime->wDay = (WORD)(cur->fulldate & 0xFF);
				//the following is from http://www.jimloy.com/math/day-week.htm
				if (lpSystemTime->wYear < 2000)
					C = 0;
				else
					C = 5;
				M = month_table[lpSystemTime->wMonth];
				if (lpSystemTime->wYear < 2000)
					L = lpSystemTime->wYear - 1900;
				else
					L = lpSystemTime->wYear - 2000;
				Y = L;
				L = L / 4;
				if (lpSystemTime->wYear >= 2000)
					L++;
				if (((lpSystemTime->wYear % 4) == 0) && (lpSystemTime->wMonth <= 2))
					L--;//don't count this leap year if in Jan or Feb
				lpSystemTime->wDayOfWeek = (lpSystemTime->wDay + C + M + Y + L) % 7;
				pGetDate = GetFindDate;
				if (!fromfind)//if not from VK_F3
					ClearBoxes();
				legit = TRUE;
				return TRUE;
			}
		}
	}
	return FALSE;
}

void ChangeCalendar(DWORD x)
{
	int y;

	MoveFile(FileNameBak, FileName);//rename FileNameBak to FileName
	if (saveit)
		CalendarWrite();
	else
	{
		temp = head;
		for (y = 0; y < (int)numofRecords; y++)
		{
			cur = temp;
			temp = cur->next;
			free(cur);
		}
	}
	head = NULL;
	strcpy(FileName, FileNames[x]);
	for (x = 0; FileName[x] != '.'; x++)
		FileNameBak[x] = FileName[x];
	FileNameBak[x++] = '.';
	FileNameBak[x++] = 'b';
	FileNameBak[x++] = 'a';
	FileNameBak[x++] = 'k';
	FileNameBak[x] = 0;
	for (x = 0; FileName[x] != '.'; x++)
	{
		if ((FileName[x] >= 'a') && (FileName[x] <= 'z'))
			CapitalFileName[x] = FileName[x] & 0xDF;
		else
			CapitalFileName[x] = FileName[x];
	}
	CapitalFileName[x] = 0;
	ClearBoxes();
	saveit = FALSE;
	ClearSD();
	SDSize = 1;
}

void ClearSD(void)
{
	int x, y;

	for (x = 0 ; x < 100; x++)
	{
		for (y = 0; (y < 40) && (SD[x].birthday[y] != 0); y++)
			SD[x].birthday[y] = 0;
		SD[x].specialDates = 0;
	}
}
