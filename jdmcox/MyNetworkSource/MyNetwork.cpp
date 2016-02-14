#include <winsock2.h>//put ws2_32.lib in Project -Settings -Link
#include <windows.h>
#include <commctrl.h>//for INITCOMMONCONTROLSEX Initccsex;
#include <psapi.h>//put psapi.lib in Project -Settings -Link (for EnumProcesses, etc)
#include "resource.h"

#define BUFSIZE 30000000
#define FILES 1000000
#define MAXFILES 100000
#define MENUCOLOR 0xD8E9EC
#define TAB 9

char Version[] = "Version 2.1 July 18, 2007 Doug Cox jdmcox@jdmcox.com";

int Tabs[] = {112, 148};
int Tabs2[] = {112, 148};
int Index[MAXFILES], CurIndex, CurIndex2, ItemsOnScreen;
int Items[1000];
int Items2[1000];
int longest, hour, num, PacketSize, NumSelected, Time;
ULONG Ptr[MAXFILES];
DWORD w, x, y, z, i, ffn, fileSize, TotalPacketSize, dwBytesRead, dwBytesWritten, SelCount, Sel, Pointers, Pointers2;
DWORD iFiles, ifile, TimeDate, len, TopIndex, ListHeight, cyPointer, pass, myNum, myPtr;
DWORD MyName[1000];
DWORD dwCrc32, ReceivedCRC, Processes[1024], NumOfProcesses;
DWORD RegValueSize = MAX_PATH, RegType = REG_SZ;
BYTE Password[8];
double d;
char ButtonSelect[] = "Select";
char ButtonRight[] ="---->";
char ButtonLeft[] ="<----";
char OtherComputer[] = "OTHER COMPUTER";
char ThisComputer[] = "THIS COMPUTER ";
char TempFile[] = "temporary.uqz";
char OtherURL[17];
char Port[6];
BYTE *Buf;
BYTE tempBuf[0xFFFF];
char Files[FILES];
char MyFiles[1000][MAX_PATH];
char MyNetworkIni[] = "MyNetworkConfiguration.txt";
char Folder[MAX_PATH];
char VirtualFolder[MAX_PATH];
char VirtualFolder2[MAX_PATH];
char Selected[MAX_PATH];
char Name[256];
char IniBuf[1000];
char File[MAX_PATH];
char CurrentDir[MAX_PATH];
char FullFilename[MAX_PATH];
char *SortedBuf, *ChangeBuf;
char *ProcessNames;
char Sanserif[] = "Microsoft Sans Serif";
struct hostent *hp;
struct sockaddr_in sock;
char RegValue[MAX_PATH];
BOOL firstime, first = TRUE, listisfocus = TRUE, fromlbuttondown = FALSE, fromtab = FALSE, bigfile = FALSE;
HKEY hRegKey;
HANDLE hFile, hFindFile;
HWND hwnd, hwndButton, hwndList, hwndList2, hwndHeader, hwndHeader2;
HINSTANCE hInst;
RECT rect, rect2, copyRect;
HDC hdc, hdcList;
PAINTSTRUCT ps;
HBRUSH hBrush;
HFONT hFont;
HICON hIcon;
LOGFONT lf;
TEXTMETRIC tm;
SYSTEMTIME st;
FILETIME ft, ft2;
WIN32_FIND_DATA fd;
WSADATA wsaData;
SOCKET s;
INITCOMMONCONTROLSEX Initccsex;
HDLAYOUT layout, layout2;
WINDOWPOS winpos, winpos2;
HDITEM hditem, hditem2;
LPNMHEADER phdn;
WNDPROC pEditProc, pEditProc2;
SIZE size;
LARGE_INTEGER li, li2;


LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;
	char szAppName[] = "MyNetwork";

	wndclass.style         = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_ICON1));
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)CreateSolidBrush(MENUCOLOR);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass (&wndclass))
		return 0;

	hInst = hInstance;

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPED|WS_CAPTION|WS_SYSMENU|WS_MINIMIZEBOX|WS_BORDER,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	ShowWindow (hwnd, iCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}


int compare(const void *arg1, const void *arg2)
{
   return _stricmp(*(char**)arg1, *(char**) arg2);
}

void ChangeButton(void)
{
	if (listisfocus)
	{
		SendMessage(hwndList, LB_GETTEXT, CurIndex, (LPARAM)Selected);
		if (Selected[0] == '[')
			SetWindowText(hwndButton, ButtonSelect);
		else
			SetWindowText(hwndButton, ButtonRight);
	}
	else
	{
		SendMessage(hwndList2, LB_GETTEXT, CurIndex2, (LPARAM)Selected);
		if (Selected[1] == '[')
			SetWindowText(hwndButton, ButtonSelect);
		else
			SetWindowText(hwndButton, ButtonLeft);
	}
}

//sub-class procedure
LRESULT CALLBACK EditProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONDOWN)
	{
		cyPointer = HIWORD(lParam);
		fromlbuttondown = TRUE;
		listisfocus = TRUE;
		SetFocus(hwnd);
		return 0;
	}
	else if (message == WM_SETFOCUS)
	{
		int x;

		NumSelected = SendMessage(hwndList, LB_GETSELITEMS, 1000, (LPARAM)Items);
		if (NumSelected >= 1000)
			MessageBox(hwnd, "Over a thousand items were selected!", ERROR, MB_OK);
		for (x = 0; x < NumSelected; x++)
			SendMessage(hwndList, LB_SETSEL, FALSE, Items[x]);
		NumSelected = SendMessage(hwndList2, LB_GETSELITEMS, 1000, (LPARAM)Items2);
		if (NumSelected >= 1000)
			MessageBox(hwnd, "Over a thousand items were selected!", ERROR, MB_OK);
		for (x = 0; x < NumSelected; x++)
			SendMessage(hwndList2, LB_SETSEL, FALSE, Items2[x]);

		if (fromlbuttondown == FALSE)
		{
			if (fromtab)
			{
				fromtab = FALSE;
				CurIndex = Items[0];
			}
			else
				CurIndex = 0;
		}
		else
		{
			fromlbuttondown = FALSE;
			TopIndex = SendMessage(hwndList, LB_GETTOPINDEX, 0, 0);
			CurIndex = TopIndex + (cyPointer/ListHeight);
		}
		SendMessage(hwndList, LB_SETSEL, TRUE, CurIndex);
		listisfocus = TRUE;
		ChangeButton();
		return 0;
	}
	else if (message == WM_KEYDOWN)
	{
		x = VK_PRIOR;
		if (wParam == VK_ESCAPE)
		{
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		else if (wParam == VK_RETURN)
		{
			SendMessage(hwnd, WM_USER, 0, 0);
			return 0;
		}
		else if (wParam == VK_BACK)
		{
			CurIndex = 0;
			SendMessage(hwndList, LB_SETSEL, (WPARAM)FALSE, (LPARAM)CurIndex);
			SendMessage(hwndList, LB_SETSEL, TRUE, 0);
			SendMessage(hwnd, WM_USER, 0, 0);
			return 0;
		}
		else if ((wParam == TAB) || (wParam == VK_RIGHT))
		{
			if (!VirtualFolder[0])
			{
				fromtab = TRUE;
				SetFocus(hwndList2);
			}
			return 0;
		}
		else if (wParam == VK_LEFT)
			return 0;
		else if (wParam == VK_DOWN)
		{
			if (CurIndex < (int)(Pointers-1))
			{
				CurIndex++;
				ChangeButton();
			}
		}
		else if (wParam == VK_UP)
		{
			if (CurIndex)
			{
				CurIndex--;
				ChangeButton();
			}
		}
		else if (wParam == VK_HOME)
		{
			CurIndex = 0;
			ChangeButton();
		}
		else if (wParam == VK_END)
		{
			CurIndex = Pointers-1;
			ChangeButton();
		}
		else if (wParam == VK_NEXT)
		{
			if ((CurIndex+(ItemsOnScreen-1)) > ((int)Pointers-1))
				CurIndex = Pointers-1;
			else
				CurIndex += (ItemsOnScreen-1);
			ChangeButton();
		}
		else if (wParam == VK_PRIOR)
		{
			if ((ItemsOnScreen-1) < CurIndex)
				CurIndex -= (ItemsOnScreen-1);
			else
				CurIndex = 0;
			ChangeButton();
		}
	}
	return CallWindowProc(pEditProc, hwnd2, message, wParam, lParam);
}

//sub-class procedure
LRESULT CALLBACK EditProc2(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONDOWN)
	{
		if (!VirtualFolder[0])
		{
			cyPointer = HIWORD(lParam);
			fromlbuttondown = TRUE;
			listisfocus = FALSE;
			SetFocus(hwnd);
		}
		return 0;
	}
	if (message == WM_SETFOCUS)
	{
		int x;

		NumSelected = SendMessage(hwndList, LB_GETSELITEMS, 1000, (LPARAM)Items);
		if (NumSelected >= 1000)
			MessageBox(hwnd, "Over a thousand items were selected!", ERROR, MB_OK);
		for (x = 0; x < NumSelected; x++)
			SendMessage(hwndList, LB_SETSEL, FALSE, Items[x]);
		NumSelected = SendMessage(hwndList2, LB_GETSELITEMS, 1000, (LPARAM)Items2);
		if (NumSelected >= 1000)
			MessageBox(hwnd, "Over a thousand items were selected!", ERROR, MB_OK);
		for (x = 0; x < NumSelected; x++)
			SendMessage(hwndList2, LB_SETSEL, FALSE, Items2[x]);
		if (fromlbuttondown == FALSE)
		{
			if (fromtab)
			{
				fromtab = FALSE;
				CurIndex2 = Items2[0];
			}
			else
				CurIndex2 = 0;
		}
		else
		{
			fromlbuttondown = FALSE;
			TopIndex = SendMessage(hwndList2, LB_GETTOPINDEX, 0, 0);
			CurIndex2 = TopIndex + (cyPointer/ListHeight);
		}
		SendMessage(hwndList2, LB_SETSEL, TRUE, CurIndex2);
		listisfocus = FALSE;
		ChangeButton();
		return 0;
	}
	if (message == WM_KEYDOWN)
	{
		if (wParam == VK_ESCAPE)
		{
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			return 0;
		}
		else if (wParam == VK_RETURN)
		{
			SendMessage(hwnd, WM_USER, 0, 0);
			return 0;
		}
		else if (wParam == VK_BACK)
		{
			CurIndex2 = 0;//SendMessage(hwndList2, LB_GETCURSEL, 0, 0);
			SendMessage(hwndList2, LB_SETSEL, (WPARAM)FALSE, (LPARAM)CurIndex2);
			SendMessage(hwndList2, LB_SETSEL, (WPARAM)TRUE, (LPARAM)0);
			SendMessage(hwnd, WM_USER, 0, 0);
			return 0;
		}
		else if ((wParam == TAB) || (wParam == VK_LEFT))
		{
			fromtab = TRUE;
			SetFocus(hwndList);
			return 0;
		}
		else if (wParam == VK_RIGHT)
			return 0;
		else if (wParam == VK_DOWN)
		{
			if (CurIndex2 < (int)myNum-1)
			{
				CurIndex2++;
				ChangeButton();
			}
		}
		else if (wParam == VK_UP)
		{
			if (CurIndex2)
			{
				CurIndex2--;
				ChangeButton();
			}
		}
		else if (wParam == VK_HOME)
		{
			CurIndex2 = 0;
			ChangeButton();
		}
		else if (wParam == VK_END)
		{
			CurIndex2 = myNum-1;
			ChangeButton();
		}
		else if (wParam == VK_NEXT)
		{
			if ((CurIndex2+(ItemsOnScreen-1)) > ((int)myNum-1))
				CurIndex2 = myNum-1;
			else
				CurIndex2 += (ItemsOnScreen-1);
			ChangeButton();
		}
		else if (wParam == VK_PRIOR)
		{
			if ((ItemsOnScreen-1) < CurIndex2)
				CurIndex2 -= (ItemsOnScreen-1);
			else
				CurIndex2 = 0;
			ChangeButton();
		}
	}
	return CallWindowProc(pEditProc2, hwnd2, message, wParam, lParam);
}

void GetProcessName(DWORD processID)
{//get all running process names in ProcessNames
	HANDLE hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, processID);
	if (NULL != hProcess)
	{
		HMODULE hMod;
		DWORD cbNeeded;

		if (EnumProcessModules( hProcess, &hMod, sizeof(hMod), &cbNeeded))
		{
			GetModuleBaseName(hProcess, hMod, &ProcessNames[num], 256);
			num += 256;
		}
	}
	CloseHandle(hProcess);
}

void SetHeader(char* Name, HWND hwndH)
{
	hditem.mask = HDI_FORMAT | HDI_WIDTH | HDI_TEXT;
	hditem.pszText = Name;
	hditem.cchTextMax = 14;
	hditem.cxy = (Tabs[0] << 1) + 30;//250
	hditem.fmt = HDF_STRING | HDF_CENTER;
	Header_InsertItem(hwndH, 0, &hditem);

	hditem.mask = HDI_FORMAT | HDI_WIDTH | HDI_TEXT;
	hditem.pszText = "Size";
	hditem.cchTextMax = 4;
	hditem.cxy = 90;
	hditem.fmt = HDF_STRING | HDF_CENTER;
	Header_InsertItem(hwndH, 1, &hditem);

	hditem.mask = HDI_FORMAT | HDI_WIDTH | HDI_TEXT;
	hditem.pszText = "Date Modified";
	hditem.cchTextMax = 13;
	hditem.cxy = 168;
	hditem.fmt = HDF_STRING | HDF_CENTER;
	Header_InsertItem(hwndH, 2, &hditem);
}

void GetFile(void)
{
	int x, y;
	char ThisFileSize[10];

	if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
		return;
	if ((fd.cFileName[0] == '.') && (fd.cFileName[1] == 0))
		return;
	x = 0;
	if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
	{
		MyFiles[myNum][x++] = ' ';
		MyFiles[myNum][x++] = '[';
		for (y = 0; fd.cFileName[y] != 0; x++, y++)
			MyFiles[myNum][x] = fd.cFileName[y];
		MyFiles[myNum][x++] = ']';
	}
	else
	{
		for ( ; fd.cFileName[x] != 0; x++)
			MyFiles[myNum][x] = fd.cFileName[x];
		MyFiles[myNum][x++] = TAB;
		_itoa(fd.nFileSizeLow, ThisFileSize, 10);
		for (y = 0; ThisFileSize[y] != 0; x++, y++)
			MyFiles[myNum][x] = ThisFileSize[y];
		MyFiles[myNum][x++] = TAB;
		FileTimeToLocalFileTime(&fd.ftLastWriteTime, &ft);
		FileTimeToSystemTime(&ft, &st);
		if (st.wMonth > 9)
			MyFiles[myNum][x++] = (st.wMonth / 10) + '0';
		MyFiles[myNum][x++] = (st.wMonth % 10) + '0';
		MyFiles[myNum][x++] = '/';
		if (st.wDay > 9)
			MyFiles[myNum][x++] = (st.wDay / 10) + '0';
		MyFiles[myNum][x++] = (st.wDay % 10) + '0';
		MyFiles[myNum][x++] = '/';
		MyFiles[myNum][x++] = (st.wYear / 1000) + '0';//1234
		MyFiles[myNum][x++] = ((st.wYear % 1000) / 100) + '0';
		MyFiles[myNum][x++] = ((st.wYear % 100) / 10) + '0';
		MyFiles[myNum][x++] = (st.wYear % 10) + '0';
		MyFiles[myNum][x++] = ' ';
		hour = st.wHour;
		if (hour > 12)
			hour -= 12;
		if (hour > 9)
			MyFiles[myNum][x++] = (hour / 10) + '0';
		MyFiles[myNum][x++] = (hour % 10) + '0';
		MyFiles[myNum][x++] = ':';
		MyFiles[myNum][x++] = (st.wMinute / 10) + '0';
		MyFiles[myNum][x++] = (st.wMinute % 10) + '0';
		MyFiles[myNum][x++] = ' ';
		if (st.wHour < 12)
			MyFiles[myNum][x++] = 'A';
		else
			MyFiles[myNum][x++] = 'P';
		MyFiles[myNum][x++] = 'M';
	}
	MyFiles[myNum][x] = 0;
	for (y = 0; y < (int)myNum; y++)
	{//insertion sort
		if (stricmp(MyFiles[myNum], MyFiles[MyName[y]]) == -1)
		{// MyFiles[myNum] < MyFiles[MyName[y]]
			for (x = myNum; x >= y; x--)
				MyName[x+1] = MyName[x]; 
			MyName[y] = myPtr;//insert it before a "bigger" filename
			break;
		}
	}
	MyName[y] = myNum;//or put it at the end
	myPtr++;
	myNum++;
}

void GetFileDate(void)
{
	st.wMonth = atoi(&Files[TimeDate]);
	for (x = 0; Files[TimeDate + x] != '/'; x++)
		;
	x++;
	st.wDay = atoi(&Files[TimeDate+x]);
	for ( ; Files[TimeDate + x] != '/'; x++)
		;
	x++;
	st.wYear = atoi(&Files[TimeDate+x]);
	for ( ; Files[TimeDate + x] != ' '; x++)
		;
	x++;
	hour = atoi(&Files[TimeDate+x]);
	for ( ; Files[TimeDate + x] != ':'; x++)
		;
	x++;
	st.wMinute = atoi(&Files[TimeDate+x]);
	for ( ; Files[TimeDate + x] != ' '; x++)
		;
	x++;
	if ((Files[TimeDate + x] == 'P') && (hour != 12))
		hour += 12;
	else if ((Files[TimeDate + x] == 'A') && (hour == 12))
		hour = 0;
	st.wHour = hour;
	SystemTimeToFileTime(&st, &ft);
	FileTimeToSystemTime(&ft, &st);
	LocalFileTimeToFileTime(&ft, &ft2);
	li = *(LARGE_INTEGER*)&ft2;
}

void ModifyChangeBuf(DWORD w)
{
	for (x = 0; (SortedBuf[w+x] != TAB) && (SortedBuf[w+x] != 0) && (x < 100); x++)
		Name[x] = SortedBuf[w+x];
	Name[x] = 0;
	hdcList = GetDC(hwnd);
	GetTextExtentPoint32(hdcList, Name, x, &size);
	if (size.cx > longest)
		longest = size.cx;
	if (size.cx > (Tabs[0] << 1))//the * 2 is for this font (Microsoft must round the 7 pixels of the average character up to 8 to get 8 / 4)
	{
		for (z = x-1; z > 0; z--)
		{
			Name[z-1] = '.';
			Name[z-2] = '.';
			Name[z-3] = '.';
			Name[z] = 0;
			GetTextExtentPoint32(hdcList, Name, z, &size);
			if (size.cx <= (Tabs[0] << 1))
			{
				ChangeBuf[w+z-1] = '.';
				ChangeBuf[w+z-2] = '.';
				ChangeBuf[w+z-3] = '.';
				for (x = w+z; ChangeBuf[x] != TAB; x++)
					;//now x is at tab
				for ( ; ChangeBuf[w+z] != 0; z++, x++)
					ChangeBuf[w+z] = ChangeBuf[x];
				ChangeBuf[w+z] = 0;
				break;
			}
		}
	}
	ReleaseDC(hwnd, hdcList);
}

void NotifyStuff(void)
{
	DWORD x;

	for (x = 0; x < fileSize; x++)
		ChangeBuf[x] = SortedBuf[x];
	Tabs[1] = Tabs[0] + 40;
	for (w = 0; w < Pointers; w++)
		if (SortedBuf[Ptr[w]] != '[')
			ModifyChangeBuf(Ptr[w]);
	SendMessage(hwndList, LB_SETTABSTOPS, (WPARAM)2, (LPARAM)&Tabs);
	SendMessage(hwndList, LB_RESETCONTENT, 0, 0);
	for (i = 0; i < Pointers; i++)
		SendMessage (hwndList, LB_ADDSTRING, 0, (LPARAM)&ChangeBuf[Ptr[i]]);
	SendMessage(hwndList, LB_SETSEL, TRUE, CurIndex);
}

void GetDirectoryFileNames(void)
{
	x = 0;
	do
	{//get directory file names in Buf & show them in list
		//***************************************
		PacketSize = recv(s, (char*)&Buf[x], 0xFFFF, 0);
		//***************************************

		if ((SOCKET_ERROR != PacketSize) && (0 != PacketSize))
		{
			if (firstime)
			{
				firstime = FALSE;
				fileSize = *(DWORD*)&Buf[0];
				if ((fileSize > FILES) || (fileSize < 5))
				{
					if ((fileSize == 0) && (Buf[4] == 'B') && (Buf[5] == 'a') && (Buf[6] == 'd'))
						MessageBox(NULL, "Bad Password", ERROR, MB_OK);
					else
						MessageBox(NULL, "Something went wrong!", ERROR, MB_OK);
					DestroyWindow(hwnd);
					return;
				}
			}
			x += PacketSize;
		}
		else
			break;
	} while (x < fileSize);

	if (SOCKET_ERROR == PacketSize)
	{
		x = WSAEFAULT;
		x = WSAGetLastError();
		MessageBox(hwnd, "Socket Error", "", MB_OK);
		DestroyWindow(hwnd);
		return;
	}

	for (x = 4, y = 0; (Buf[x] != '\r') && (x < MAX_PATH); x++, y++)
		Folder[y] = Buf[x];
	Folder[y] = 0;
	if (x == MAX_PATH)
	{
		MessageBox(hwnd, "x == 256", ERROR, MB_OK);
		DestroyWindow(hwnd);
		return;
	}
	Buf[x++] = 0;//first entry is directory name
	x++;//past \n

	SetWindowText(hwnd, Folder);//put computer name and directory name on Title bar
	if (0 == SetCurrentDirectory(Folder))
		strcpy(VirtualFolder, Folder);
	else
		VirtualFolder[0] = 0;//flag
	for (i = 4, ffn = 0; Buf[i] != 0; i++, ffn++)
		FullFilename[ffn] = Buf[i];//the first entry is the full filename
	FullFilename[ffn++] = '\\';
	for (i = 0 ; (x < fileSize) && (i < MAXFILES); x++)//x begins at the first filename
	{//put pointer to each directory entry in Ptr[]
		y = (ULONG)&Buf[x];
		for ( ; Buf[x] != '\r'; x++)
			;
		Buf[x++] = 0;
		Ptr[i++] = y;
	}
	Pointers = i;

	qsort((void*)Ptr, Pointers, sizeof(char*), compare);//requires pointers to absolute addresses

	for (i = 0; i < Pointers; i++)
		Ptr[i] -= (ULONG)&Buf[0];//get relative addresses

	longest = 0;
	for (i = 0, y = 0; i < Pointers; i++)
	{
		w = y;
		for (x = Ptr[i]; Buf[x] != 0; x++, y++)
		{
			SortedBuf[y] = Buf[x];
			ChangeBuf[y] = Buf[x];
		}
		SortedBuf[y] = 0;
		ChangeBuf[y++] = 0;
		if (SortedBuf[w] == '/')
		{
			SortedBuf[w] = '[';
			SortedBuf[y-2] = ']';
			ChangeBuf[w] = '[';
			ChangeBuf[y-2] = ']';
		}
		Ptr[i] = w;//now it will hold SortedBuf offsets

		if (SortedBuf[w] != '[')
			ModifyChangeBuf(w);
	}
	SetCurrentDirectory((char*)&Buf[4]);
	GetCurrentDirectory(MAX_PATH, CurrentDir);
	for (y = 0; y < 1000; y++)
	{//initialize
		for (x = 0; MyFiles[y][x] != 0; x++)
			MyFiles[y][x] = 0;
		MyName[y] = 0;
	}
	hwndList = CreateWindow("LISTBOX", NULL,//WC_LISTVIEW
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | LBS_MULTIPLESEL | LBS_EXTENDEDSEL | WS_VSCROLL | LBS_NOTIFY | LBS_USETABSTOPS | CS_DBLCLKS,
		0, rect.top, 508, (rect.bottom-rect.top-60),
		hwnd, (HMENU)400, hInst, NULL);
	SendMessage(hwndList, WM_SETFONT, (WPARAM)(HFONT)hFont, FALSE);
	pEditProc = (WNDPROC)SetWindowLong(hwndList, GWL_WNDPROC, (LONG)EditProc);
	ListHeight = SendMessage(hwndList, LB_GETITEMHEIGHT, 0, 0);
	ItemsOnScreen = ((rect.bottom-rect.top-60) / ListHeight) - 1;//who knows why the - 1 is necessary...
	SendMessage(hwndList, LB_SETTABSTOPS, (WPARAM)2, (LPARAM)&Tabs);
	for (i = 0; i < Pointers; i++)
		SendMessage (hwndList, LB_ADDSTRING, 0, (LPARAM)&ChangeBuf[Ptr[i]]);

	hwndList2 = CreateWindow("LISTBOX", NULL,
		WS_CHILD | WS_VISIBLE | WS_DLGFRAME | LBS_MULTIPLESEL | LBS_EXTENDEDSEL | WS_VSCROLL | LBS_NOTIFY | LBS_USETABSTOPS,
		508, rect2.top, rect2.right, rect2.bottom-rect2.top-60,
		hwnd, (HMENU)400, hInst, NULL);
	SendMessage(hwndList2, WM_SETFONT, (WPARAM)(HFONT)hFont, FALSE);
	pEditProc2 = (WNDPROC)SetWindowLong(hwndList2, GWL_WNDPROC, (LONG)EditProc2);
	if (!VirtualFolder[0])
	{
		myPtr = myNum = 0;
		hFindFile = FindFirstFile("*.*", &fd);
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			GetFile();
			while ((FindNextFile(hFindFile, &fd)) && (myNum < MAXFILES))
				GetFile();
			FindClose(hFindFile);
			if (myNum == MAXFILES)
				MessageBox(hwnd, "There are over 100,000 files in this directory!", ERROR, MB_OK);
		}
		SendMessage(hwndList2, LB_SETTABSTOPS, (WPARAM)2, (LPARAM)&Tabs2);
		for (i = 0; i < myNum; i++)//filename TAB filesize TAB date&time
			SendMessage (hwndList2, LB_ADDSTRING, 0, (LPARAM)&MyFiles[MyName[i]]);
	}
	CurIndex = CurIndex2 = 0;
	SetFocus(hwnd);
}

DWORD Crc32Table[256] =
{
	0x00000000, 0x77073096, 0xEE0E612C, 0x990951BA,
	0x076DC419, 0x706AF48F, 0xE963A535, 0x9E6495A3,
	0x0EDB8832, 0x79DCB8A4, 0xE0D5E91E, 0x97D2D988,
	0x09B64C2B, 0x7EB17CBD, 0xE7B82D07, 0x90BF1D91,
	0x1DB71064, 0x6AB020F2, 0xF3B97148, 0x84BE41DE,
	0x1ADAD47D, 0x6DDDE4EB, 0xF4D4B551, 0x83D385C7,
	0x136C9856, 0x646BA8C0, 0xFD62F97A, 0x8A65C9EC,
	0x14015C4F, 0x63066CD9, 0xFA0F3D63, 0x8D080DF5,
	0x3B6E20C8, 0x4C69105E, 0xD56041E4, 0xA2677172,
	0x3C03E4D1, 0x4B04D447, 0xD20D85FD, 0xA50AB56B,
	0x35B5A8FA, 0x42B2986C, 0xDBBBC9D6, 0xACBCF940,
	0x32D86CE3, 0x45DF5C75, 0xDCD60DCF, 0xABD13D59,
	0x26D930AC, 0x51DE003A, 0xC8D75180, 0xBFD06116,
	0x21B4F4B5, 0x56B3C423, 0xCFBA9599, 0xB8BDA50F,
	0x2802B89E, 0x5F058808, 0xC60CD9B2, 0xB10BE924,
	0x2F6F7C87, 0x58684C11, 0xC1611DAB, 0xB6662D3D,

	0x76DC4190, 0x01DB7106, 0x98D220BC, 0xEFD5102A,
	0x71B18589, 0x06B6B51F, 0x9FBFE4A5, 0xE8B8D433,
	0x7807C9A2, 0x0F00F934, 0x9609A88E, 0xE10E9818,
	0x7F6A0DBB, 0x086D3D2D, 0x91646C97, 0xE6635C01,
	0x6B6B51F4, 0x1C6C6162, 0x856530D8, 0xF262004E,
	0x6C0695ED, 0x1B01A57B, 0x8208F4C1, 0xF50FC457,
	0x65B0D9C6, 0x12B7E950, 0x8BBEB8EA, 0xFCB9887C,
	0x62DD1DDF, 0x15DA2D49, 0x8CD37CF3, 0xFBD44C65,
	0x4DB26158, 0x3AB551CE, 0xA3BC0074, 0xD4BB30E2,
	0x4ADFA541, 0x3DD895D7, 0xA4D1C46D, 0xD3D6F4FB,
	0x4369E96A, 0x346ED9FC, 0xAD678846, 0xDA60B8D0,
	0x44042D73, 0x33031DE5, 0xAA0A4C5F, 0xDD0D7CC9,
	0x5005713C, 0x270241AA, 0xBE0B1010, 0xC90C2086,
	0x5768B525, 0x206F85B3, 0xB966D409, 0xCE61E49F,
	0x5EDEF90E, 0x29D9C998, 0xB0D09822, 0xC7D7A8B4,
	0x59B33D17, 0x2EB40D81, 0xB7BD5C3B, 0xC0BA6CAD,

	0xEDB88320, 0x9ABFB3B6, 0x03B6E20C, 0x74B1D29A,
	0xEAD54739, 0x9DD277AF, 0x04DB2615, 0x73DC1683,
	0xE3630B12, 0x94643B84, 0x0D6D6A3E, 0x7A6A5AA8,
	0xE40ECF0B, 0x9309FF9D, 0x0A00AE27, 0x7D079EB1,
	0xF00F9344, 0x8708A3D2, 0x1E01F268, 0x6906C2FE,
	0xF762575D, 0x806567CB, 0x196C3671, 0x6E6B06E7,
	0xFED41B76, 0x89D32BE0, 0x10DA7A5A, 0x67DD4ACC,
	0xF9B9DF6F, 0x8EBEEFF9, 0x17B7BE43, 0x60B08ED5,
	0xD6D6A3E8, 0xA1D1937E, 0x38D8C2C4, 0x4FDFF252,
	0xD1BB67F1, 0xA6BC5767, 0x3FB506DD, 0x48B2364B,
	0xD80D2BDA, 0xAF0A1B4C, 0x36034AF6, 0x41047A60,
	0xDF60EFC3, 0xA867DF55, 0x316E8EEF, 0x4669BE79,
	0xCB61B38C, 0xBC66831A, 0x256FD2A0, 0x5268E236,
	0xCC0C7795, 0xBB0B4703, 0x220216B9, 0x5505262F,
	0xC5BA3BBE, 0xB2BD0B28, 0x2BB45A92, 0x5CB36A04,
	0xC2D7FFA7, 0xB5D0CF31, 0x2CD99E8B, 0x5BDEAE1D,

	0x9B64C2B0, 0xEC63F226, 0x756AA39C, 0x026D930A,
	0x9C0906A9, 0xEB0E363F, 0x72076785, 0x05005713,
	0x95BF4A82, 0xE2B87A14, 0x7BB12BAE, 0x0CB61B38,
	0x92D28E9B, 0xE5D5BE0D, 0x7CDCEFB7, 0x0BDBDF21,
	0x86D3D2D4, 0xF1D4E242, 0x68DDB3F8, 0x1FDA836E,
	0x81BE16CD, 0xF6B9265B, 0x6FB077E1, 0x18B74777,
	0x88085AE6, 0xFF0F6A70, 0x66063BCA, 0x11010B5C,
	0x8F659EFF, 0xF862AE69, 0x616BFFD3, 0x166CCF45,
	0xA00AE278, 0xD70DD2EE, 0x4E048354, 0x3903B3C2,
	0xA7672661, 0xD06016F7, 0x4969474D, 0x3E6E77DB,
	0xAED16A4A, 0xD9D65ADC, 0x40DF0B66, 0x37D83BF0,
	0xA9BCAE53, 0xDEBB9EC5, 0x47B2CF7F, 0x30B5FFE9,
	0xBDBDF21C, 0xCABAC28A, 0x53B39330, 0x24B4A3A6,
	0xBAD03605, 0xCDD70693, 0x54DE5729, 0x23D967BF,
	0xB3667A2E, 0xC4614AB8, 0x5D681B02, 0x2A6F2B94,
	0xB40BBE37, 0xC30C8EA1, 0x5A05DF1B, 0x2D02EF8D,
};

void Crc32Assembly(BYTE* buffer,DWORD fSize)
{
	__asm
	{
		push esi
		push edi
		mov ecx, 0xFFFFFFFF
		mov edi, offset Crc32Table
		mov esi, dword ptr buffer
		mov ebx, fSize
		lea edx, dword ptr [esi+ebx]
	crc32loop:
		xor eax, eax
		mov bl, byte ptr [esi]
		mov al, cl
		inc esi
		xor al, bl
		shr ecx, 8
		mov ebx, dword ptr [edi+eax*4]
		xor ecx, ebx
		cmp edx, esi
		jne crc32loop
		pop edi
		pop esi
		not ecx
		mov dword ptr dwCrc32, ecx
	}
}


//*****************************************************************************

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_CREATE:
		GetCurrentDirectory(MAX_PATH, CurrentDir);
		if (INVALID_HANDLE_VALUE == FindFirstFile(MyNetworkIni, &fd))
		{
			if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, "SOFTWARE\\MyNetwork", &hRegKey))
			{
				if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, "Install_Dir", NULL, &RegType, (LPBYTE)RegValue, &RegValueSize))
				{
					for (x = 0; RegValue[x] != 0; x++)
						;
					RegValue[x++] = '\\';
					for (y = 0; MyNetworkIni[y] != 0; x++, y++)
						RegValue[x] = MyNetworkIni[y];
					RegValue[x] = 0;
					for (y = 0; y < x; y++)
						MyNetworkIni[y] = RegValue[y];
					MyNetworkIni[y] = 0;
				}
				else
				{
					RegCloseKey(hRegKey);
					MessageBox(hwnd, "Couldn't find 'MyNetworkConfiguration.txt'", CurrentDir, MB_OK);
					DestroyWindow(hwnd);
					return 0;
				}
				RegCloseKey(hRegKey);
			}
			else
			{
				MessageBox(hwnd, "Couldn't find 'MyNetworkConfiguration.txt'", CurrentDir, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}
		}
		Buf = 0;
		SortedBuf = ChangeBuf = 0;
		VirtualFolder[0] = 0;
		s = INVALID_SOCKET;
		EnumProcesses(Processes, 1024, &NumOfProcesses);
		NumOfProcesses /= 4;//change number of bytes to number of DWORDs
		ProcessNames = (char*)malloc(NumOfProcesses*256);
		num = 0;
		for ( i = 0; i < NumOfProcesses; i++ )
		{
			if (Processes[i] != 0)
				GetProcessName(Processes[i]);
		}
		NumOfProcesses = num/256;//when OpenProcess doesn't return hProcess, num isn't incremented

		Initccsex.dwSize = sizeof(INITCOMMONCONTROLSEX);
		Initccsex.dwICC  = ICC_LISTVIEW_CLASSES;
		InitCommonControlsEx(&Initccsex);//for header

		if (INVALID_HANDLE_VALUE != (hFile = CreateFile(MyNetworkIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)))
		{
			hBrush = CreateSolidBrush(MENUCOLOR);
			Buf = (BYTE*)VirtualAlloc(NULL, BUFSIZE, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			SortedBuf = (char*)VirtualAlloc(NULL, FILES, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			ChangeBuf = (char*)VirtualAlloc(NULL, FILES, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			fileSize = GetFileSize(hFile, NULL);
			ReadFile(hFile, IniBuf, fileSize, &dwBytesRead, NULL);
			CloseHandle(hFile);
			hFile = NULL;

			x = 0;
			while (TRUE)
			{
				for ( ; (x < fileSize) && (IniBuf[x] != '='); x++)
					;
				if (x >= fileSize)
					break;
				else if (IniBuf[x-1] == 'S')
				{//IP ADDRESS=
					x++;
					for (y = 0; (IniBuf[x] != '\r') && (x < fileSize); x++, y++)
						OtherURL[y] = IniBuf[x];
					OtherURL[y] = 0;
					if (x >= fileSize)
						break;
				}
				else if ((IniBuf[x-1] == 'T') && (IniBuf[x-4] == 'P'))
				{//PORT=
					x++;
					for (y = 0; (IniBuf[x] != '\r') && (y < 6); x++, y++) 
						Port[y] = IniBuf[x];
				}
				else if ((IniBuf[x-1] == 'D') && (IniBuf[x-8] == 'P'))
				{//PASSWORD=
					x++;
					for (y = 0; (IniBuf[x] != '\r') && (y < 8); x++, y++) 
						Password[y] = IniBuf[x];
				}
				else
					x++;
			}
		}
		else
		{
			MessageBox(hwnd, "Couldn't find 'MyNetworkConfiguration.txt'", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}

		if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0))
		{
			rect.left = rect.top = 0;
			rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
			rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN)+GetSystemMetrics(SM_CYCAPTION)+(2*GetSystemMetrics(SM_CYBORDER));
		}
		//*********
		MoveWindow(hwnd, (rect.right/2)-508, 0, 1016, rect.bottom, TRUE);
		//*********
		lf.lfHeight = -13;
		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 1;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x22;
		for (x = 0; Sanserif[x] != 0; x++)
			lf.lfFaceName[x] = Sanserif[x];
		lf.lfFaceName[x] = 0;
		hFont = CreateFontIndirect (&lf);

		hwndButton = CreateWindow("BUTTON", "---->",
			WS_CHILD | WS_VISIBLE | BS_DEFPUSHBUTTON,
			508-36, rect.bottom-64, 72, 24,
			hwnd, NULL, hInst, NULL);
		SendMessage(hwndButton, WM_SETFONT, (WPARAM)(HFONT)hFont, FALSE);

		hwndHeader = CreateWindow(WC_HEADER, NULL,
			WS_CHILD | WS_BORDER | HDS_BUTTONS,
			0, 0, 0, 0,
			hwnd, (HMENU)1234, hInst, NULL);
		SendMessage(hwndHeader, WM_SETFONT, (WPARAM)(HFONT)hFont, FALSE);
		SetHeader(OtherComputer, hwndHeader);

		hwndHeader2 = CreateWindow(WC_HEADER, NULL,
			WS_CHILD | WS_BORDER | HDS_BUTTONS,
			0, 0, 0, 0,
			hwnd, (HMENU)1235, hInst, NULL);
		SendMessage(hwndHeader2, WM_SETFONT, (WPARAM)(HFONT)hFont, FALSE);
		SetHeader(ThisComputer, hwndHeader2);

		rect.left = rect.top = 0;
		rect.right = 508;
		layout.prc = &rect;
		layout.pwpos = &winpos;
		Header_Layout(hwndHeader, &layout);
		rect.top = winpos.cy;
		MoveWindow(hwndHeader, winpos.x, winpos.y, winpos.cx, winpos.cy, TRUE);
		ShowWindow(hwndHeader, SW_SHOW);

		rect2.top = 0;//necessary
		rect2.bottom = rect.bottom;
		rect2.left = 508;
		rect2.right = 1016;
		layout2.prc = &rect2;
		layout2.pwpos = &winpos2;
		Header_Layout(hwndHeader2, &layout2);
		rect2.top = winpos2.cy;
		MoveWindow(hwndHeader2, winpos2.x, winpos2.y, winpos2.cx, winpos2.cy, TRUE);
		ShowWindow(hwndHeader2, SW_SHOW);
		return 0;

	case WM_SETFOCUS:
		if (listisfocus)
			SetFocus(hwndList);
		else
			SetFocus(hwndList2);
		return 0;

	case WM_COMMAND:
		if (lParam == (long)hwndButton)
		{
			if (listisfocus)
				SendMessage(hwnd, WM_USER, 0, 0);
			else
				SetFocus(hwnd);
		}
		return 0;

	case WM_NOTIFY://for moving header boundaries
		phdn = (LPNMHEADER)lParam;
		if (phdn->iItem == 0)
		{
			if (phdn->hdr.code == HDN_TRACK)
			{
				Tabs[0] = (phdn->pitem->cxy - 30) >> 1;
				NotifyStuff();
			}
			else if (phdn->hdr.code == HDN_DIVIDERDBLCLICK)
			{
				Tabs[0] = longest >> 1;
				NotifyStuff();

				DestroyWindow(hwndHeader);
				hwndHeader = CreateWindow(WC_HEADER, NULL,
					WS_CHILD | WS_BORDER | HDS_BUTTONS,
					winpos.x, winpos.y, winpos.cx, winpos.cy,
					hwnd, (HMENU)1234, hInst, NULL);
				SendMessage(hwndHeader, WM_SETFONT, (WPARAM)(HFONT)hFont, FALSE);
				SetHeader(OtherComputer, hwndHeader);
				ShowWindow(hwndHeader, SW_SHOW);
			}
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (first)
		{
			first = FALSE;
			if (0 == WSAStartup(0x0202, &wsaData))
			{
				s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
				if (s != INVALID_SOCKET)
				{
					sock.sin_family = AF_INET;
					sock.sin_port = htons(atoi(Port));
					sock.sin_addr.s_addr = inet_addr((char*)OtherURL);
					if (0 == connect(s, (LPSOCKADDR)&sock, sizeof(SOCKADDR)))
					{
						//**********************
						send(s, (char*)Password, 8, 0);
						//**********************
						firstime = TRUE;
						GetDirectoryFileNames();//in WM_PAINT
					}
					else
					{
						x = WSAGetLastError();
						MessageBox(NULL, "You forgot to run MyNetworkServer on the other computer,\nor MyNetworkConfiguration.txt doesn't have the other computer's correct IP address,\nor MyNetworkConfiguration.txt doesn't have the other computer's correct port,\nor there isn't a connection between the two computers,\nor a firewall is blocking the connection,\nor something else is wrong...", ERROR, MB_OK);
						SendMessage(hwnd, WM_CLOSE, 0, 0);
					}
				}
				else
				{
					MessageBox(NULL, "huh!", ERROR, MB_OK);
					SendMessage(hwnd, WM_CLOSE, 0, 0);
				}
			}
			else
			{
				MessageBox(NULL, "wha!", ERROR, MB_OK);
				SendMessage(hwnd, WM_CLOSE, 0, 0);
			}
		}
		EndPaint(hwnd, &ps);
		return 0;

	case WM_USER:
		ZeroMemory(Files, FILES);
		SelCount = SendMessage(hwndList, LB_GETSELCOUNT, 0, 0);
		if (SelCount == 0)
			return 0;
		SendMessage(hwndList, LB_GETSELITEMS, (WPARAM)SelCount, (LPARAM)Index);

		for (x = y = z = 0, i = Index[z]; z < SelCount; )
		{
			if ((SortedBuf[Ptr[i]] == '[') || (SortedBuf[Ptr[i]] == ' '))
			{
				strcpy(Files, &SortedBuf[Ptr[i]]);
				SelCount = 1;
				break;//with directory name in Files
			}
			Files[y++] = SortedBuf[Ptr[i]+x];
			if (SortedBuf[Ptr[i]+x] != 0)
				x++;
			else
			{
				if (y > (FILES-100))
					break;
				z++;
				i = Index[z];
				x = 0;
			}
		}
		//***********
		DestroyWindow(hwndList);
		hwndList = NULL;
		DestroyWindow(hwndList2);
		hwndList2 = NULL;
		//***********

		copyRect.left = 0;
		copyRect.right = rect.right;
		copyRect.top = rect.bottom/2;

		for (Sel = 0, iFiles = 0; Sel < SelCount; Sel++)
		{//filename TAB filesize TAB date&time 0
			*(DWORD*)&File[0] = *(DWORD*)&Password[0];
			*(DWORD*)&File[4] = *(DWORD*)&Password[4];
			for (ifile = 8; (Files[iFiles] != TAB) && (Files[iFiles] != 0); ifile++, iFiles++)
				File[ifile] = Files[iFiles];
			File[ifile++] = 0;//the ++ is to send the ending 0
			if (Files[iFiles] != 0)//not a directory (a TAB)
			{
				x = atoi(&Files[iFiles+1]);
				for ( ; Files[iFiles] != 0; iFiles++)
				{
					if (Files[iFiles+1] == TAB)
					{
						TimeDate = iFiles + 2;
						break;
					}
				}
				for ( ; Files[iFiles] != 0; iFiles++)
					;
				iFiles++;//to next selection
				if (x == 0)//fileSize
					continue;
				for (x = 0; (int)x < num; x += 256)
				{
					if (0 == strcmp(&File[8], &ProcessNames[x]))
						break;//file is running on this computer
				}
				if ((int)x < num)
				{
					MessageBox(hwnd, "can't be copied while it's running on this computer.",&File[8], MB_OK);
					continue;//if file is running on this computer
				}
			}

 			hdc = GetDC(hwnd);
			len = lstrlen(&File[8]);
			GetTextExtentPoint32(hdc, &File[8], len, &size);//get size.cx
			copyRect.bottom = copyRect.top + size.cy;
			FillRect(hdc, &copyRect, hBrush);
			TextOut(hdc, (300)-(size.cx/2), rect.bottom/2, &File[8], len);
			ReleaseDC(hwnd, hdc);

			//******************
renew:		PacketSize = send(s, File, ifile, 0);
			if ((SOCKET_ERROR == PacketSize) || (0 == PacketSize))
			{
				MessageBox(hwnd, "Error", ERROR, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}
			//******************

			firstime = TRUE;
			TotalPacketSize = fileSize = 0;
			if ((File[8] == '[') || (File[8] == ' '))
			{
				//***********
				GetDirectoryFileNames();//in WM_USER
				return 0;
				//***********
			}
			if (hFile)
			{
				CloseHandle(hFile);
				hFile = NULL;
			}

			if (VirtualFolder[0])
			{//create real folder(s)
				BOOL notdone = TRUE;
				x = 0;
				strcpy(VirtualFolder2, VirtualFolder);
				do
				{
					strcpy(VirtualFolder, VirtualFolder2);
					for (x++; (VirtualFolder[x] != 0) && (VirtualFolder[x] != '\\'); x++)
						;
					if (VirtualFolder[x] == 0)
					{
						VirtualFolder[x] = '\\';
						notdone = FALSE;
					}
					VirtualFolder[++x] = 0;
					CreateDirectory(VirtualFolder, NULL);
				} while (notdone);
				VirtualFolder[0] = 0;//flag
			}
			DeleteFile(TempFile);//it shouldn't exist
			y = 0;
			do
			{//get file
				//************************************
				PacketSize = recv(s, (char*)&Buf[y], 0xFFFF, 0);
				//************************************

				if ((SOCKET_ERROR != PacketSize) && (0 != PacketSize))
				{
					if (firstime)
					{
						firstime = FALSE;
						fileSize = *(DWORD*)&Buf[0];
						ReceivedCRC= *(DWORD*)&Buf[4];
						if (fileSize > BUFSIZE)
						{
							bigfile = TRUE;
							for (x = 0; (int)x < PacketSize; x++)
								tempBuf[x] = Buf[x];
							VirtualFree(Buf, 0, MEM_RELEASE);
							Buf = (BYTE*)VirtualAlloc(NULL, fileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
							for (x = 0; (int)x < PacketSize; x++)
								Buf[x] = tempBuf[x];
						}
						if (fileSize != 0)
						{
							for (z = 8, x = ffn; File[z] != 0; z++, x++)
								FullFilename[x] = File[z];
							FullFilename[x] = 0;
							GetFileDate();
							hFindFile = FindFirstFile(FullFilename, &fd);
							if (INVALID_HANDLE_VALUE != hFindFile)
							{
								FindClose(hFindFile);
								li2 = *(LARGE_INTEGER*)&fd.ftLastWriteTime;
								hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
								if (li.QuadPart > li2.QuadPart)//if current file is older than downloaded file
								{
									CloseHandle(hFile);
									hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
								}
								else
								{
									CloseHandle(hFile);
									hFile = CreateFile(TempFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
								}
							}
							else//if file doesn't exist on this computer
								hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
							if (WriteFile(hFile, &Buf[8], (DWORD)PacketSize-8, &dwBytesWritten, NULL))
								SetFileTime(hFile, NULL, NULL, &ft2);
						}
						else//if (fileSize == 0) should not happen
						{
							if (hFile)
							{
								CloseHandle(hFile);
								hFile = NULL;
							}
							MessageBox(hwnd, "fileSize == 0", ERROR, MB_OK);
							DestroyWindow(hwnd);
							return 0;
						}
					}//end of if (firstime)
					else
						WriteFile(hFile, &Buf[y], (DWORD)PacketSize, &dwBytesWritten, NULL);
					TotalPacketSize += PacketSize;
				}
				else if (SOCKET_ERROR == PacketSize)
				{
					x = WSAEFAULT;
					x = WSAGetLastError();
					MessageBox(hwnd, "Socket Error", "", MB_OK);
					DestroyWindow(hwnd);
					return 0;
				}
				else//0 == PacketSize
					break;
				y += PacketSize;
			} while (TotalPacketSize < fileSize);

			if (hFile != NULL)
			{
				FlushFileBuffers(hFile);
				CloseHandle(hFile);
				hFile = NULL;
			}
			Crc32Assembly(&Buf[8], fileSize);//TotalPacketSize
			if (dwCrc32 != ReceivedCRC)
			{
				MessageBox(hwnd, "The CRC32 numbers didn't match!\nYou'd better try again!", ERROR, MB_OK);
				break;
			}

			hFindFile = FindFirstFile(TempFile, &fd);
			if (INVALID_HANDLE_VALUE != hFindFile)
			{//if TempFile was written
				FindClose(hFindFile);
				if (IDYES != MessageBox(hwnd, "has an older or same modification date !\nOverwrite the existing file with it ?!?", FullFilename, MB_YESNO|MB_DEFBUTTON2))
					DeleteFile(TempFile);
				else
				{
					DeleteFile(FullFilename);
					MoveFile(TempFile, FullFilename);
				}
			}
		}//end of for (Sel = 0
		if (bigfile)
		{
			bigfile = FALSE;
			VirtualFree(Buf, 0, MEM_RELEASE);
			Buf = (BYTE*)VirtualAlloc(NULL, BUFSIZE, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		}
		File[8] = '[';
		for (x = 0, ifile = 9; Folder[x] != 0; x++, ifile++)
			File[ifile] = Folder[x];
		File[ifile++] = ']';
		File[ifile++] = 0;
		goto renew;//to re-get folder contents after a file transfer
		return 0;


	case WM_CLOSE:
		DestroyWindow(hwnd);
		return 0;

	case WM_DESTROY:
		free(ProcessNames);
		if (Buf)
		{
			VirtualFree(Buf, 0, MEM_RELEASE);
			VirtualFree(SortedBuf, 0, MEM_RELEASE);
			VirtualFree(ChangeBuf, 0, MEM_RELEASE);
		}
		if (hFile)
			CloseHandle(hFile);
		if (s != INVALID_SOCKET)
		{
			shutdown(s, SD_BOTH);
			closesocket(s);
		}
		WSACleanup();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

/*
//sending and receiving an unconnected broadcast:
	if (0 == WSAStartup(0x0202, &wsaData))
	{
		int Broadcast = 1;
		int BufLen = 12;
		BYTE RecvAddr[BIG_ENOUGH];
		SOCKET RecvSocket, BroadcastSocket;
		struct sockaddr_in RecvSocket, sock;

		RecvSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		WSAAsyncSelect(RecvSocket, hwnd, WM_USER, FD_READ);
		RecvAddr.sin_family = AF_INET;
		RecvAddr.sin_port = htons(atoi(Port));
		RecvAddr.sin_addr.s_addr = htonl(INADDR_ANY);
		bind(RecvSocket, (SOCKADDR*)&RecvAddr, sizeof(RecvAddr));

		BroadcastSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		setsockopt(BroadcastSocket, SOL_SOCKET, SO_BROADCAST, (char*)&Broadcast, sizeof(Broadcast));
		sock.sin_family = AF_INET;
		sock.sin_port = htons(atoi(Port));
		sock.sin_addr.s_addr = htonl(INADDR_BROADCAST);
		x = sendto(BroadcastSocket, Whatever, lstrlen(Whatever), 0, (LPSOCKADDR)&sock, sizeof(SOCKADDR));
		closesocket(BroadcastSocket);
	}

	case WM_USER:
		if (LOWORD(lParam) == FD_READ)
		{
			PacketSize = recv(RecvSocket, Buffer, 0xFFFF, 0);
			closesocket(RecvSocket);
			if (0 == strcmp(Buffer, MyBuffer))
				return 0;//because it receives its own broadcast
			...
		}
*/
