#include <winsock2.h>//put ws2_32.lib in Project/Settings/Link
#include <windows.h>
#include <stdio.h>//for sprintf
#include "resource.h"
#define FTP_PORT 21
#define BUFSIZE 0xFFFF
#define ONEMEG 1024*1024
#define TAB 9
#define WM_USER2 WM_USER+1
#define WM_USER3 WM_USER2+1

BOOL debug = TRUE;

char About[] = "\
by Doug Cox\nJune 8, 2007\njdmcox@jdmcox.com\n\
\n\
You can press:\n\
S  for the Server List\n\
Enter  to go to a folder\n\
or to download a file\n\
D  to download\n\
U  to upload\n\
V  to view a file\n\
Del  to delete\n\
H  for help\n\
Esc  to exit\n\
\n\
You can right-click on a file in Windows Explorer\n\
and select MySimpleFTP to send that file to the\n\
initial directory of your FTP server/website\n\
(if the currently selected FTP server is associated\n\
with your website, and you've entered the\n\
initial directory that you may upload files to).\n\
\n\
This version doesn't support UNIX server shortcuts to\n\
files in other folders. You'll have to go there manually.";

char ServerHelp[] = "\
by Doug Cox\nJune 8, 2007\njdmcox@jdmcox.com\n\
\n\
The Address is usually something like ftp.intel.com\n\
If you don't have a user ID and a password,\n\
enter anonymous as the User,\n\
and your email address as the Password.\n\
The Initial Dir could be something like /pub\n\
or it can be blank.\n\
\n\
FTP port 21 is used, and the command\n\
PASV (passive) is sent to have the FTP server\n\
select a data port for data transfer.\n\
\n\
The server list is saved in MySimpleFTP.ini.";

int x, y, z, i, iLast, IniBufEnd, LineBeg, NextLine, PacketSize, PacketPtr, DataPacketSize, data, num, filesize;
int NameOffset, DateOffset, SizeOffset, DateLoc, commas, aftertab, index, indexServer, Zero, filenameOffset;
int cxScreen, cyScreen, TitleAndMenu, Frame, ItemHeight, TopIndex;
int Tabs[] = {120,0};
DWORD fileSize, IniFileSize, dwBytesRead, dwBytesWritten;
double percentage, p;
char szAppName[] = "MySimpleFTP";
char MySimpleFTPIni[] = "MySimpleFTP.ini";
char IniBuf[BUFSIZE];
char Buf[BUFSIZE];
char *DataBuf;
char DirBuf[1000][256];
char CurrentDir[MAX_PATH];
char Filename[MAX_PATH] = {0};
char ShortFilename[MAX_PATH];
char SERVER[] = "SERVER ";
char Host[256];
char User[256] = "USER ";
char Pass[256] = "PASS ";
char Init[] = "INITIAL DIR ";
char SYST[] = "SYST\r\n";//215 Windows_NT or 215 UNIX
char DIR[] = "DIR ";
char TYPE[] = "TYPE A\r\n";
char PASV[] = "PASV\r\n";
char PWD[] = "PWD\r\n";
char LIST[] = "LIST\r\n";
char QUIT[] = "QUIT\r\n";
char CWD[300] = "CWD ";
char RETR[300] = "RETR ";
char STOR[300] = "STOR ";
char DELE[300] = "DELE ";
char ParentDir[] = "[..]";
char DirFilename[MAX_PATH] = " [..]";
char PrevDirFilename[MAX_PATH];
char TitleBar[1024];
char CourierNew[] = "Courier New";
char Counter[48];
char Months[] = "JanFebMarAprMayJunJulAugSepOctNovDec";

struct {
	char Addr[32];
	char User[32];
	char Pass[32];
	char Init[64];
} Server[32];

char tempAddr[32];
char tempUser[32];
char tempPass[32];
char tempInit[64];

char Template[] = "01\r\n\
SERVER ftp.intel.com\r\n\
USER anonymous\r\n\
PASS jdmcox@jdmcox.com\r\n\
INITIAL DIR /pub\r\n\
DIR C:\\\r\n";

BOOL first = TRUE, veryfirst = TRUE, download, upload, sendfile, itsspace, fromupload, empty = FALSE;
BOOL numbervalid = TRUE, read = FALSE, itsunix, finished = TRUE, cmdline = FALSE, gotlist, gotserverlist = FALSE;
struct sockaddr_in dest, dataDest;
struct hostent *hp;
HANDLE hFile = NULL, hDownloadedFile, hIniFile, hUploadFile;
HANDLE hDataFile = NULL;
HWND hwnd, hwndList, hwndEdit, hwndCounter, hwndServerDlg, hwndServerDlg2;
HINSTANCE hInst;
MSG	msg;
HMENU hMenu;
HFONT hFont;
HCURSOR hCursor, hWaitingCursor;
//HCURSOR hCrossCursor, hUpArrowCursor;
RECT rect, rectList;
HDC hdc, hdcList;
PAINTSTRUCT ps;
HBITMAP hBitmap;
SOCKET s, sData;
WSADATA wsaData;
LOGFONT lf;
WNDPROC pListProc;
OPENFILENAME ofn;
WIN32_FIND_DATA fd;

void SaveDir(void);
BOOL Receive(char *ErrorPtr, char Response, char *ErrorPtr2);
BOOL GetData(void);
BOOL SendData(void);
void FillHUP(void);
BOOL PrepUpload(void);
int CALLBACK ServersProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK NewProc(HWND hwndNewDlg, UINT message, WPARAM wParam, LPARAM lParam);
int CALLBACK ReadProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	WNDCLASS wndclass;
	hInst = hInstance;

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = NULL;//see hCursor, below
	wndclass.hbrBackground = (HBRUSH)(COLOR_MENU+1);
	wndclass.lpszMenuName  = "MENU";
	wndclass.lpszClassName = szAppName;
	RegisterClass (&wndclass);

	if (szCmdLine[0] != 0)
	{
		for (x = 0, y = 0; szCmdLine[x] != 0; x++)
			if (szCmdLine[x] != '"')
				Filename[y++] = szCmdLine[x];
		Filename[y] = 0;
	}

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
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

//sub-class procedure for hwndList
LRESULT CALLBACK ListProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y;

	if (message == WM_KEYDOWN)
	{
		switch (wParam)
		{
		case 'S':
			SendMessage(hwnd, WM_COMMAND, ID_SERVERLIST, 0);
			break;
		case 'D':
			SendMessage(hwnd, WM_COMMAND, ID_DOWNLOADFROMSERVER, 0);
			break;
		case 'U':
			SendMessage(hwnd, WM_COMMAND, ID_UPLOADTOSERVER, 0);
			break;
		case 'V':
			SendMessage(hwnd, WM_COMMAND, ID_READSELECTEDFILE, 0);
			break;
		case 'H':
			SendMessage(hwnd, WM_COMMAND, HELP, 0);
			break;
		case VK_DELETE:
			SendMessage(hwnd, WM_COMMAND, ID_DELETESELECTEDFILE, 0);
			break;
		case VK_ESCAPE:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;
		case VK_LEFT:
		case VK_RIGHT:
			return 0;//don't let the list use these keys
		case VK_BACK:
			SendMessage(hwndList, LB_SETCURSEL, 0, 0);//move to top of list
		case VK_RETURN:
			if (empty == FALSE)
			{
				index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
				strcpy(PrevDirFilename, DirFilename);
				SendMessage(hwndList, LB_GETTEXT, index, (LPARAM)DirFilename);
			}
			else
				empty = FALSE;
			if (DirFilename[1] == '[')
			{
				if ((finished) && (gotlist))
				{
					for (x = 4, y = 2; (y < MAX_PATH) && (DirFilename[y] != ']'); x++, y++)
						CWD[x] = DirFilename[y];
					CWD[x++] = '\r';
					CWD[x++] = '\n';
					CWD[x++] = 0;

					if (SOCKET_ERROR == send(s, CWD, strlen(CWD), 0))
					{
						MessageBox(hwnd, "bad send CWD", ERROR, MB_OK);
						DestroyWindow(hwnd);
					}
					if (FALSE == Receive("bad receive CWD", '2', "Couldn't change directories"))
					{
						strcpy(DirFilename, PrevDirFilename);
						break;
					}
					SendMessage(hwnd, WM_USER2, 0, 0);
				}
			}
			else
				SendMessage(hwnd, WM_COMMAND, ID_DOWNLOADFROMSERVER, 0);
			break;
		}
	}
	else if (message == WM_LBUTTONDBLCLK)
		SendMessage(hwndList, WM_KEYDOWN, VK_RETURN, 0);

	return CallWindowProc(pListProc, hwnd2, message, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		download = upload = fromupload = sendfile = FALSE;
		DataBuf = NULL;
		if ((Filename[0] != 0) && (INVALID_HANDLE_VALUE != FindFirstFile(Filename, &fd)))
		{
			cmdline = TRUE;
			if (FALSE == PrepUpload())
			{
				DestroyWindow(hwnd);
				return 0;
			}
		}
		hCursor = LoadCursor (NULL, IDC_ARROW);
		hWaitingCursor = LoadCursor(NULL, IDC_WAIT);
//		hCrossCursor = LoadCursor(NULL, IDC_CROSS);
//		hUpArrowCursor = LoadCursor(NULL, IDC_UPARROW);
		SetCursor(hCursor);
		if (INVALID_HANDLE_VALUE != (hIniFile = CreateFile(MySimpleFTPIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)))
		{
			IniFileSize = GetFileSize(hIniFile, NULL);
			if (IniFileSize < BUFSIZE)
				ReadFile(hIniFile, IniBuf, IniFileSize, &dwBytesRead, NULL);
			else
			{
				MessageBox(hwnd, "MySimpleFTP.ini is bigger than 65,535 bytes!", ERROR, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}
		}
		else
		{
			for (x = 0; Template[x] != 0; x++)
				IniBuf[x] = Template[x];
			IniBuf[x] = 0;
			IniFileSize = x;
			hIniFile = CreateFile(MySimpleFTPIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			WriteFile(hIniFile, Template, IniFileSize, &dwBytesRead, NULL);
		}
		CloseHandle(hIniFile);
		indexServer = ((IniBuf[0] -'0') * 10) + (IniBuf[1] -'0') - 1;
		if (indexServer > 99)
		{
			MessageBox(hwnd, "MySimpleFTP.ini doesn't start\nwith a number less than 100.", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		if ((IniBuf[4] != 'S') || (IniBuf[5] != 'E') || (IniBuf[6] != 'R') || (IniBuf[7] != 'V'))
		{
			MessageBox(hwnd, "MySimpleFTP.ini doesn't have\n'SERVER ' on the second line.", ERROR, MB_OK);
			DestroyWindow(hwnd);
			break;
		}
		for (IniBufEnd = (int)IniFileSize-3; (IniBufEnd > 0) && (IniBuf[IniBufEnd] != '\n'); IniBufEnd--)
			;
		IniBufEnd++;
		y = IniBufEnd;
		if ((IniBuf[y] == 'D') && (IniBuf[y+1] == 'I') && (IniBuf[y+2] == 'R') && (IniBuf[y+3] == ' '))
		{
			for (x = 0, y += 4; (y < (int)IniFileSize) && (IniBuf[y] != '\r'); x++, y++)
				CurrentDir[x] = IniBuf[y];
			CurrentDir[x] = 0;
		}
		else
			MessageBox(hwnd, "isn't at the beginning of the\nlast line of MySimpleFTP.ini!\nFix it in Notepad.", "DIR ", MB_OK);
		for (i = 0, x = 11; (i < 32) && (x < IniBufEnd); x++, i++)
		{
			for (y = 0; (y < 32) && (IniBuf[x] != '\r') && (x < (int)IniFileSize); x++, y++)
				Server[i].Addr[y] = IniBuf[x];
			Server[i].Addr[y] = 0;
			x += 7;//to next line & past USER
			for (y = 0; (y < 32) && (IniBuf[x] != '\r') && (x < (int)IniFileSize); x++, y++)
				Server[i].User[y] = IniBuf[x];
			Server[i].User[y] = 0;
			x += 7;//to next line & past PASS
			for (y = 0; (y < 32) && (IniBuf[x] != '\r') && (x < (int)IniFileSize); x++, y++)
				Server[i].Pass[y] = IniBuf[x];
			Server[i].Pass[y] = 0;
			x += 14;//to next line & past INITIAL DIR
			for (y = 0; (y < 32) && (IniBuf[x] != '\r') && (x < (int)IniFileSize); x++, y++)
				Server[i].Init[y] = IniBuf[x];
			Server[i].Init[y] = 0;
			x += 8;//to next server
		}
		iLast = i;
		FillHUP();

		hMenu = LoadMenu(hInst, "MENU");
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
		Frame = GetSystemMetrics(SM_CXSIZEFRAME);
		TitleAndMenu = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU);

		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.hwndOwner         = hwnd;
		ofn.hInstance         = NULL;
		ofn.lpstrFilter       = NULL;
		ofn.lpstrFile         = Filename;
		ofn.lpstrFileTitle    = ShortFilename;
		ofn.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
		ofn.lpstrTitle        = "Download To";
		ofn.lpstrDefExt       = NULL;
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

		lf.lfHeight = -16;
		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 1;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x31;
		for (x = 0; CourierNew[x] != 0; x++)
			lf.lfFaceName[x] = CourierNew[x];
		lf.lfFaceName[x] = 0;

		s = INVALID_SOCKET;
		WSAStartup(0x0202, &wsaData);
		return 0;

	case WM_SIZE:
		MoveWindow(hwnd, (cxScreen/2)-400, 0, 800, cyScreen, TRUE);
		return 0;

	case WM_SETFOCUS:
		if ((gotserverlist == TRUE) && (INVALID_SOCKET == s))
			SetFocus(hwndServerDlg2);
		else
			SetFocus(hwndList);
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_EXIT:
			DestroyWindow(hwnd);
			break;

		case ID_SERVERLIST:
			if (gotserverlist == FALSE)
			{
				gotserverlist = TRUE;
				if (DialogBox(hInst, "SERVERS", NULL, ServersProc))
				{
					if (finished)
					{
						gotserverlist = FALSE;
						gotlist = FALSE;
						SendMessage(hwnd, WM_USER, 0, 0);
					}
				}
				else if (INVALID_SOCKET == s)
					DestroyWindow(hwnd);//a socket was never opened
			}
			break;

		case ID_DOWNLOADFROMSERVER:
			if ((finished) && (gotlist))
			{
				download = FALSE;
				index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
				SendMessage(hwndList, LB_GETTEXT, index, (LPARAM)DirFilename);
				if (DirFilename[1] != '[')
				{
					for (x = 5, y = 1, z = 0; (DirFilename[y] != 0) && (DirFilename[y] != TAB); x++, y++, z++)
					{
						RETR[x] = DirFilename[y];
						Filename[z] = DirFilename[y];
					}
					Filename[z] = 0;
					RETR[x++] = '\r';
					RETR[x++] = '\n';
					RETR[x] = 0;
					for (y++; DirFilename[y] == ' '; y++);
						;
					for (filesize = 0; DirFilename[y] != ' '; y++)
					{
						if (DirFilename[y] != ',')
						{
							filesize *= 10;
							filesize += DirFilename[y] - '0';
						}
					}
					if (read)
					{
						download = TRUE;
						SendMessage(hwnd, WM_USER3, 0, 0);
					}
					else if (FALSE != GetSaveFileName(&ofn))
					{
						SaveDir();
						InvalidateRect(hwndList, &rectList, FALSE);
						UpdateWindow(hwndList);
						download = TRUE;
						SendMessage(hwnd, WM_USER3, 0, 0);
					}
				}
			}
			break;

		case ID_UPLOADTOSERVER:
			if ((finished) && (gotlist))
			{
				ofn.lpstrTitle = "Upload";
				if (FALSE != GetOpenFileName(&ofn))
				{
					if (PrepUpload())
						SendMessage(hwnd, WM_USER3, 0, 0);
				}
				ofn.lpstrTitle = "Download To";
			}
			break;

		case ID_READSELECTEDFILE:
			if ((finished) && (gotlist))
			{
				read = TRUE;
				SendMessage(hwnd, WM_COMMAND, ID_DOWNLOADFROMSERVER, 0);
			}
			break;

		case ID_DELETESELECTEDFILE:
			if ((finished) && (gotlist))
			{
				index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
				SendMessage(hwndList, LB_GETTEXT, index, (LPARAM)DirFilename);
				if (DirFilename[1] != '[')
				{//don't try to delete a directory
					for (x = 5, y = 1, z = 0; DirFilename[y] != TAB; x++, y++, z++)
					{
						DELE[x] = DirFilename[y];
						Filename[z] = DirFilename[y];
					}
					Filename[z] = 0;
					DELE[x++] = '\r';
					DELE[x++] = '\n';
					DELE[x] = 0;
					if (IDYES == MessageBox(hwnd, Filename, "Delete this file?", MB_YESNO|MB_DEFBUTTON2))
					{
						if (SOCKET_ERROR == send(s, DELE, strlen(DELE), 0))
						{
							MessageBox(hwnd, "bad send DELE", ERROR, MB_OK);
							DestroyWindow(hwnd);
							return 0;
						}
						if (FALSE == Receive("bad receive DELE", '2', "bad DELE command"))
							return 0;
						SendMessage(hwnd, WM_USER2, 0, 0);
					}
				}
				else
					MessageBox(hwnd, "You can't delete a directory", ERROR, MB_OK);
			}
			break;

		case HELP:
			MessageBox(hwnd, About, szAppName, MB_OK);
			break;
		}
		return 0;


	case WM_USER:
		finished = FALSE;
		SetCursor(hWaitingCursor);
		s = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (INVALID_SOCKET == s)
		{
			MessageBox(hwnd, "bad socket", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		hp = gethostbyname(Host);
		if (NULL == hp)
		{
			MessageBox(hwnd, "bad host", ERROR, MB_OK);
			finished = TRUE;
			return 0;
		}
		memcpy(&dest.sin_addr, hp->h_addr_list[0], hp->h_length);
		dest.sin_family = AF_INET;
		dest.sin_port = htons(FTP_PORT);

		if (SOCKET_ERROR == connect(s, (struct sockaddr*)&dest, sizeof(SOCKADDR)))
		{
			MessageBox(hwnd, "bad connect", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}

		hFile = CreateFile("FTPServerLog.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (debug)
			hDataFile = CreateFile("DataBuf.txt", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);

		PacketPtr = 0;
		if (FALSE == Receive("bad initial receive", '2', "server not ready"))
		{
			finished = TRUE;
			return 0;
		}

		if (SOCKET_ERROR == send(s, User, strlen(User), 0))
		{
			MessageBox(hwnd, "bad send User", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		numbervalid = FALSE;
		if (FALSE == Receive("bad receive User", 'x', " "))
		{
			finished = TRUE;
			return 0;
		}
		numbervalid = TRUE;

		if (SOCKET_ERROR == send(s, Pass, strlen(Pass), 0))
		{
			MessageBox(hwnd, "bad send Pass", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		if (FALSE == Receive("bad receive Pass", '2', "bad Pass command"))
		{
			finished = TRUE;
			return 0;
		}

		if (SOCKET_ERROR == send(s, SYST, strlen(SYST), 0))
		{
			MessageBox(hwnd, "bad send SYST", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		if (FALSE == Receive("bad receive SYST", '2', "bad SYST command"))
		{
			finished = TRUE;
			return 0;
		}

		if (Buf[PacketPtr-PacketSize+4] == 'U')//UNIX
			itsunix = TRUE;
		else if (Buf[PacketPtr-PacketSize+4] == 'W')//Windows_NT
			itsunix = FALSE;

		if ((Server[indexServer].Init[0]) && (Server[indexServer].Init[1]))
		{
			for (x = 4, y = 0; Server[indexServer].Init[y] != 0; x++, y++)
				CWD[x] = Server[indexServer].Init[y];
			CWD[x++] = '\r';
			CWD[x++] = '\n';
			CWD[x] = 0;
			if (SOCKET_ERROR == send(s, CWD, strlen(CWD), 0))
			{
				MessageBox(hwnd, "bad send CWD", ERROR, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}
			if (FALSE == Receive("bad receive CWD", '2', "Couldn't change directories"))
			{
				finished = TRUE;
				return 0;
			}
		}
		if (cmdline)
			goto wmuser3;

	case WM_USER2:
		finished = FALSE;
		SetCursor(hWaitingCursor);
		if (SOCKET_ERROR == send(s, TYPE, strlen(TYPE), 0))
		{
			MessageBox(hwnd, "bad send TYPE", ERROR, MB_OK);
			finished = TRUE;
			return 0;
		}
		if (FALSE == Receive("bad receive TYPE", '2', "bad TYPE command"))
		{
			finished = TRUE;
			return 0;
		}

		if (SOCKET_ERROR == send(s, PWD, strlen(PWD), 0))
		{
			MessageBox(hwnd, "bad send PWD", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		if (FALSE == Receive("bad send PWD", '2', "PWD command not successful"))
		{
			finished = TRUE;
			return 0;
		}

		for (x = 0; Host[x] != 0; x++)
			TitleBar[x] = Host[x];
		TitleBar[x++] = ' ';
		TitleBar[x++] = ' ';
		TitleBar[x++] = ' ';
		for (y = PacketPtr-PacketSize; Buf[y] != '"'; y++)
			;
		y++;
		for (; (y < (PacketPtr)) && (Buf[y] != '"'); x++, y++)
			TitleBar[x] = Buf[y];
		TitleBar[x] = 0;
		SetWindowText(hwnd, TitleBar);//put directory name on Title bar

	case WM_USER3:
wmuser3:cmdline = FALSE;
		finished = FALSE;

		if (SOCKET_ERROR == send(s, PASV, strlen(PASV), 0))
		{
			MessageBox(hwnd, "bad send PASV", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		if (FALSE == Receive("bad receive PASV", '2', "bad PASV command"))
		{
			finished = TRUE;
			return 0;
		}
		x = PacketPtr-PacketSize;
		if ((Buf[x] == '2') && (Buf[x+1] == '2') && (Buf[x+2] == '7'))
		{//227 Entering Passive Mode (216,239,138,164,198,171)
			char ServerIP[16];
			unsigned short DataPort, a, b;

			for (y = x; (y < PacketPtr) && (Buf[y] != '(') && (Buf[y] != '\n'); y++)
				;
			if (Buf[y] == '\n') 
			{
				MessageBox(hwnd, "227 Entering Passive Mode (xxx.xxx.xxx.xxx.xxx.xxx) not received", ERROR, MB_OK);
				finished = TRUE;
				return 0;
			}
			commas = 0;
			for (y++, z = 0; (y < PacketPtr); y++, z++)
			{
				if (Buf[y] == ',')
				{
					if (commas == 3)
					{
						ServerIP[z] = 0;
						break;
					}
					ServerIP[z] = '.';
					commas++;
				}
				else
					ServerIP[z] = Buf[y];
			}
			if (y >= PacketPtr)
			{
				MessageBox(hwnd, "y >= PacketPtr", ERROR, MB_OK);
				finished = TRUE;
				return 0;
			}
			dataDest.sin_family = AF_INET;
			dataDest.sin_addr.s_addr = inet_addr(ServerIP);
			y++;
			a = atoi(&Buf[y]);
			for (y++; (y < PacketPtr) && (Buf[y] != ','); y++)
				;
			if (y >= PacketPtr)
			{
				MessageBox(hwnd, "y >= PacketPtr", ERROR, MB_OK);
				finished = TRUE;
				return 0;
			}
			y++;
			b = atoi(&Buf[y]);
			DataPort = (a*256)+b;
			dataDest.sin_port = htons(DataPort);
		}
		else
		{
			MessageBox(hwnd, "227 Entering Passive Mode not received", ERROR, MB_OK);
			finished = TRUE;
			return 0;
		}

		sData = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
		if (INVALID_SOCKET == sData)
		{
			MessageBox(hwnd, "bad socket", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		if (SOCKET_ERROR == connect(sData, (struct sockaddr*)&dataDest, sizeof(SOCKADDR)))
		{
			MessageBox(hwnd, "bad connect", ERROR, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}

		if (download)
		{
			TYPE[5] = 'I';//for binary transfer
			if (SOCKET_ERROR == send(s, TYPE, strlen(TYPE), 0))
			{
				MessageBox(hwnd, "bad send TYPE", ERROR, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}
			TYPE[5] = 'A';//default ASCII
			if (FALSE == Receive("bad receive TYPE", '2', "bad PWD command"))
			{
				download = FALSE;
				finished = TRUE;
				return 0;
			}
			if (SOCKET_ERROR == send(s, RETR, strlen(RETR), 0))
			{
				MessageBox(hwnd, "bad send RETR", ERROR, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}
			if (FALSE == Receive("bad receive RETR", '1', "download command not successful"))
			{
				download = FALSE;
				finished = TRUE;
				return 0;
			}

			TopIndex = SendMessage(hwndList, LB_GETTOPINDEX, 0, 0);
			hwndCounter = CreateWindow("STATIC", NULL,
				WS_POPUP | WS_VISIBLE,
				(cxScreen/2)-400+Frame, TitleAndMenu+Frame+(ItemHeight*(index-TopIndex)), 300, ItemHeight,
				hwndList, NULL, hInst, NULL);

			DataBuf = (char*)VirtualAlloc(NULL, filesize+BUFSIZE, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

			if (FALSE == GetData())
			{
				download = FALSE;
				finished = TRUE;
				VirtualFree(DataBuf, 0, MEM_RELEASE);
				DataBuf = NULL;
				return 0;
			}
			download = FALSE;

			if (FALSE == Receive("bad receive data results", '2', "bad transfer of data"))
			{
				finished = TRUE;
				VirtualFree(DataBuf, 0, MEM_RELEASE);
				DataBuf = NULL;
				return 0;
			}

			if (read)
			{
				read = FALSE;
				DialogBox(hInst, "READ", NULL, ReadProc);
			}
			else
			{
				hDownloadedFile = CreateFile(ofn.lpstrFile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				WriteFile(hDownloadedFile, DataBuf, data, &dwBytesWritten, NULL);
				CloseHandle(hDownloadedFile);
			}
			VirtualFree(DataBuf, 0, MEM_RELEASE);
			DataBuf = NULL;
		}

		else if (upload)
		{
			upload = FALSE;
			TYPE[5] = 'I';//for binary transfer
			if (SOCKET_ERROR == send(s, TYPE, strlen(TYPE), 0))
			{
				MessageBox(hwnd, "bad send TYPE", ERROR, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}
			TYPE[5] = 'A';//default ASCII
			if (FALSE == Receive("bad receive TYPE", '2', "bad PWD command"))
			{
				finished = TRUE;
				VirtualFree(DataBuf, 0, MEM_RELEASE);
				DataBuf = NULL;
				return 0;
			}
			if (SOCKET_ERROR == send(s, STOR, strlen(STOR), 0))
			{
				MessageBox(hwnd, "STOR not sent.", ERROR, MB_OK);
				DestroyWindow(hwnd);
			}
			if (FALSE == Receive("bad receive STOR", '1', "Server doesn't allow uploads to this folder."))
			{
				finished = TRUE;
				VirtualFree(DataBuf, 0, MEM_RELEASE);
				DataBuf = NULL;
				return 0;
			}

			if (FALSE == SendData())
			{
				finished = TRUE;
				VirtualFree(DataBuf, 0, MEM_RELEASE);
				DataBuf = NULL;
				return 0;
			}

			if (FALSE == Receive("bad receive data results", '2', "bad transfer of data"))
			{
				finished = TRUE;
				return 0;
			}
			SendMessage(hwnd, WM_USER2, 0, 0);//to show list again
		}

		else//list
		{
			if (SOCKET_ERROR == send(s, LIST, strlen(LIST), 0))
			{
				MessageBox(hwnd, "bad send LIST", ERROR, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}

			if (FALSE == Receive("bad receive list", '1', "bad transfer of data"))
			{
				finished = TRUE;
				return 0;
			}

			DataBuf = (char*)VirtualAlloc(NULL, ONEMEG, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			if (FALSE == GetData())
			{
				finished = TRUE;
				VirtualFree(DataBuf, 0, MEM_RELEASE);
				DataBuf = NULL;
				return 0;
			}

			if (FALSE == Receive("bad receive data results", '2', "bad transfer of data"))
			{
				finished = TRUE;
				VirtualFree(DataBuf, 0, MEM_RELEASE);
				DataBuf = NULL;
				return 0;
			}
			if (itsunix)
			{
				x = 0;
				if ((DataBuf[x] != 'd') && (DataBuf[x] != '-') || (DataBuf[x] != 'l'))
				{//go to first listing
					for ( ; x < data; x++)
						if (((x == 0) || (DataBuf[x-1] == '\n')) && ((DataBuf[x] == 'd') || (DataBuf[x] == '-') || (DataBuf[x] == 'l')))
							break;
					if (x == data)
					{
						MessageBox(hwnd, "directory is empty", ERROR, MB_OK);
						strcpy(DirFilename, PrevDirFilename);
						empty = TRUE;
						finished = TRUE;
						SendMessage(hwndList, WM_KEYDOWN, VK_RETURN, 0);
						return 0;
					}
				}
				Zero = x;
/*
				itsspace = TRUE;
				for (x += 10, num = 0; num < 4; x++)
				{
					if ((DataBuf[x] != ' ') && (itsspace == TRUE))
					{
						itsspace = FALSE;
						num++;
					}
					else if (DataBuf[x] == ' ')
						itsspace = TRUE;
				}
				for ( ;DataBuf[x] != ' '; x++)
					;
				x--;//to last digit in size
				SizeOffset = x - Zero;
				DateOffset = SizeOffset + 2;
				NameOffset = DateOffset + 13;
*/
				for (x += 10, y = 0; DataBuf[x] != '\n'; x++)
				{
					for (y = 0; y < (3*12); y += 3)
						if ((DataBuf[x] == Months[y]) && (DataBuf[x+1] == Months[y+1]) && (DataBuf[x+2] == Months[y+2]))
								goto breakout;
				}
breakout:		if (DataBuf[x] != '\n')
				{
					DateOffset = x - Zero;
					SizeOffset = x - Zero - 2;
					NameOffset = x - Zero + 13;
				}
				else
					MessageBox(hwnd, "Couldn't find month!", ERROR, MB_OK);
				SendMessage (hwndList, LB_RESETCONTENT, 0, 0);
				for (num = 0, LineBeg = Zero; (num < 1000) && (LineBeg < data); num++)
				{
					y = 0;
					DirBuf[num][y++] = ' '; 
					if (DataBuf[LineBeg] == 'l')//symbolic link
					{
						z = LineBeg + NameOffset;
						for ( ; DataBuf[z] != '\n'; z++)
							;
						z++;
						LineBeg = z;
						num--;//so it doesn't increment in loop
						continue;//don't show
					}
					else if (DataBuf[LineBeg] == 'd')
					{
						z = LineBeg + NameOffset;
						if ((DataBuf[z] == '.') && ((DataBuf[z+1] == '.') || (DataBuf[z+1] == '\r') || (DataBuf[z+1] == '\n')))
						{
							for ( ; DataBuf[z] != '\n'; z++)
								;
							z++;
							LineBeg = z;
							num--;//so it doesn't increment in loop
							continue;//don't show
						}
						DirBuf[num][y++] = '[';
						for ( ; (z < data) && (DataBuf[z] != '\r') && (DataBuf[z] != '\n'); z++, y++)
							DirBuf[num][y] = DataBuf[z];
						DirBuf[num][y++] = ']';
						DirBuf[num][y] = 0;
						if (DataBuf[z] == '\r')
							z++;
						NextLine = z+1;//after \n
					}
					else
					{
						for (z = LineBeg + NameOffset; (z < data) && (DataBuf[z] != '\r') && (DataBuf[z] != '\n'); z++, y++)
							DirBuf[num][y] = DataBuf[z];
						if (DataBuf[z] == '\r')
							z++;
						NextLine = z+1;//after \n
						aftertab = y;
						DirBuf[num][y++] = TAB;
						y += 16;
						DateLoc = y+1;
						DirBuf[num][DateLoc++] = ' ';
						DirBuf[num][DateLoc++] = ' ';
						DirBuf[num][DateLoc++] = ' ';
						DirBuf[num][y--] = 's';
						DirBuf[num][y--] = 'e';
						DirBuf[num][y--] = 't';
						DirBuf[num][y--] = 'y';
						DirBuf[num][y--] = 'b';
						DirBuf[num][y--] = ' ';

						z = LineBeg + SizeOffset;
						for (commas = 0; (z > LineBeg) && (y > aftertab); y--)
						{
							if ((commas == 3) && (DataBuf[z] >= '0') && (DataBuf[z] <= '9'))
							{
								DirBuf[num][y] = ',';
								commas = 0;
							}
							else
							{
								if (DataBuf[z] != ' ')
									commas++;
								else
								{
									for ( ; y > aftertab; y--)
										DirBuf[num][y] = ' ';
									break;
								}
								DirBuf[num][y] = DataBuf[z--];
							}
						}
						for (z = LineBeg + DateOffset, y = DateLoc; (z < data) && (z < (LineBeg + DateOffset + 12)) && (DataBuf[z] != '\n'); z++, y++)
							DirBuf[num][y] = DataBuf[z];
						DirBuf[num][y] = 0;
					}
					LineBeg = NextLine;
				}//end of for (num = 0;
			}//end of if (itsunix)
			else
			{
				SizeOffset = 37;
				NameOffset = 39;
				SendMessage (hwndList, LB_RESETCONTENT, 0, 0);
				for (num = 0, LineBeg = 0; (num < 1000) && (LineBeg < data); num++)
				{
					y = 0;
					DirBuf[num][y++] = ' ';
					if ((DataBuf[LineBeg+24] == '<') && (DataBuf[LineBeg+28] == '>'))//DIR
					{
						DirBuf[num][y++] = '[';
						for (z = LineBeg+NameOffset; DataBuf[z] != '\r'; y++, z++)
							DirBuf[num][y] = DataBuf[z];
						NextLine = z+2;
						DirBuf[num][y++] = ']';
						DirBuf[num][y] = 0;
					}
					else
					{
						for (z = LineBeg+NameOffset; DataBuf[z] != '\r'; y++, z++)
							DirBuf[num][y] = DataBuf[z];
						NextLine = z+2;

						aftertab = y;
						DirBuf[num][y++] = TAB;
						y += 16;
						DateLoc = y+1;
						DirBuf[num][DateLoc++] = ' ';
						DirBuf[num][DateLoc++] = ' ';
						DirBuf[num][DateLoc++] = ' ';
						DirBuf[num][y--] = 's';
						DirBuf[num][y--] = 'e';
						DirBuf[num][y--] = 't';
						DirBuf[num][y--] = 'y';
						DirBuf[num][y--] = 'b';
						DirBuf[num][y--] = ' ';

						z = LineBeg + SizeOffset;
						for (commas = 0; (z > LineBeg) && (y > aftertab); y--)
						{
							if ((commas == 3) && (DataBuf[z] >= '0') && (DataBuf[z] <= '9'))
							{
								DirBuf[num][y] = ',';
								commas = 0;
							}
							else
							{
								if (DataBuf[z] != ' ')
									commas++;
								else
								{
									for ( ; y > aftertab; y--)
										DirBuf[num][y] = ' ';
									break;
								}
								DirBuf[num][y] = DataBuf[z--];
							}
						}
						for (z = LineBeg, y = DateLoc; (z < data) && (z < (LineBeg + 17)) && (DataBuf[z] != '\r'); z++, y++)
							DirBuf[num][y] = DataBuf[z];//date
						DirBuf[num][y] = 0;
					}
					LineBeg = NextLine;
				}
			}
			y = 0;
			DirBuf[num][y++] = ' ';
			for (x = 0; ParentDir[x] != 0; x++, y++)
				DirBuf[num][y] = ParentDir[x];
			DirBuf[num][y] = 0;
			SendMessage (hwndList, LB_ADDSTRING, 0, (LPARAM)&DirBuf[num][0]);
			for (x = 0; x < num; x++)
				SendMessage (hwndList, LB_ADDSTRING, 0, (LPARAM)&DirBuf[x][0]);
			if (fromupload)
			{
				char NameinDir[MAX_PATH];

				fromupload = FALSE;
				NameinDir[0] = ' ';
				for (x = 0, y = 1; ShortFilename[x] != 0; x++, y++)
					NameinDir[y] = ShortFilename[x];
				NameinDir[x] = 0;
				index = SendMessage(hwndList, LB_SELECTSTRING, 0, (LPARAM)NameinDir);
			}
			else
				index = 0;
			SendMessage(hwndList, LB_SETCURSEL, (WPARAM)index, 0);
			SetFocus(hwndList);
			if (debug == FALSE)
			{
				VirtualFree(DataBuf, 0, MEM_RELEASE);
				DataBuf = NULL;
			}
			gotlist = TRUE;
		}//end of else//list
		SetCursor(hCursor);
		finished = TRUE;
		return 0;


	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (first)
		{
			first = FALSE;

			rectList.left = 0;
			rectList.top = 0;
			rectList.right = 800-(Frame*2);
			rectList.bottom = cyScreen-TitleAndMenu;
			hwndList = CreateWindow("LISTBOX", NULL,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | LBS_HASSTRINGS | LBS_SORT | LBS_USETABSTOPS,
				0, 0, rectList.right, rectList.bottom,
				hwnd, NULL, hInst, NULL);
			hFont = CreateFontIndirect (&lf);
			SendMessage(hwndList, WM_SETFONT, (WPARAM)(HFONT)hFont, FALSE);
			ItemHeight = SendMessage(hwndList, LB_GETITEMHEIGHT, 0, 0);
			SendMessage(hwndList, LB_SETTABSTOPS, (WPARAM)2, (LPARAM)&Tabs);
			pListProc = (WNDPROC)SetWindowLong(hwndList, GWL_WNDPROC, (LONG)ListProc);

			if (cmdline)
				SendMessage(hwnd, WM_USER, 0, 0);
			else
				SendMessage(hwnd, WM_COMMAND, ID_SERVERLIST, 0);
		}
		EndPaint(hwnd, &ps);
		return 0;

	case WM_CLOSE:
	case WM_DESTROY:
		send(s, QUIT, strlen(QUIT), 0);
		if (hFile)
		{
			WriteFile(hFile, Buf, PacketPtr, &dwBytesWritten, NULL);
			if (debug)
			{
				WriteFile(hDataFile, DataBuf, data, &dwBytesWritten, NULL);
				CloseHandle(hDataFile);
			}
		}
		if (hFile)
			CloseHandle(hFile);
		if (DataBuf)
			VirtualFree(DataBuf, 0, MEM_RELEASE);
		if (INVALID_SOCKET != s)
			closesocket(s);
		WSACleanup();
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}

/////////////////////////////////SUBROUTINES///////////////////////////////////////

void SaveDir(void)
{
	int y;

	for (x = 0; Filename[x] != 0; x++)
		;
	for ( ; (x > 0) && (Filename[x] != '\\'); x--)
		;
	filenameOffset = x+1;//for PrepUpload
	for (y = 0; (y < MAX_PATH) && (y <= x); y++)
		CurrentDir[y] = Filename[y];
	CurrentDir[y] = 0;

	for (y = (int)IniFileSize-3; (y != 0) && (IniBuf[y] != '\n'); y--)
		;
	if ((IniBuf[y+1] != 'D') || (IniBuf[y+2] != 'I') || (IniBuf[y+3] != 'R') || (IniBuf[y+4] != ' '))
	{
		return;
	}
	y += 5;//to directory
	for (x = 0; CurrentDir[x] != 0; x++, y++)
		IniBuf[y] = CurrentDir[x];
	IniBuf[y++] = '\r';
	IniBuf[y++] = '\n';
	IniBuf[y] = 0;//for debugging only
	hIniFile = CreateFile(MySimpleFTPIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	WriteFile(hIniFile, IniBuf, y, &dwBytesRead, NULL);
	FlushFileBuffers(hIniFile);
	CloseHandle(hIniFile);
}

BOOL Receive(char *ErrorPtr, char Response, char *ErrorPtr2)
{
	do
	{
		if (PacketPtr > BUFSIZE)
		{
			MessageBox(hwnd, "PacketPtr > BUFSIZE", ERROR, MB_OK);
			finished = TRUE;
			return 0;
		}
//SetCursor(hUpArrowCursor);
		PacketSize = recv(s, &Buf[PacketPtr], BUFSIZE, 0);
//SetCursor(hWaitingCursor);
		if (SOCKET_ERROR == PacketSize)
		{
			MessageBox(hwnd, ErrorPtr, ERROR, MB_OK);
			finished = TRUE;
			return FALSE;
		}
		if (numbervalid)
		{
			numbervalid = FALSE;
			if (Buf[PacketPtr] != Response)// || (Buf[PacketPtr+1] != Response[1]) || (Buf[PacketPtr+2] != Response[2]))
			{
				PacketPtr += PacketSize;
				MessageBox(hwnd, ErrorPtr2, ERROR, MB_OK);
				return FALSE;
			}
		}
		if ((Buf[PacketPtr] >= '0') && (Buf[PacketPtr] <= '9') && (Buf[PacketPtr+1] >= '0') && (Buf[PacketPtr+1] <= '9') && (Buf[PacketPtr+2] >= '0') && (Buf[PacketPtr+2] <= '9') && (Buf[PacketPtr+3] == ' '))
		{
			PacketPtr += PacketSize;
			break;//e.g. "220 "
		}
		PacketPtr += PacketSize;
		if (Buf[PacketPtr-1] == '\n')
		{
			for (z = PacketPtr-2; (z != 0) && (Buf[z-1] != '\n'); z--)
				;
			if ((Buf[z] >= '0') && (Buf[z] <= '9') && (Buf[z+1] >= '0') && (Buf[z+1] <= '9') && (Buf[z+2] >= '0') && (Buf[z+2] <= '9') && (Buf[z+3] == ' '))
				break;//e.g. "220 "
		}
	}
	while (PacketSize);
	numbervalid = TRUE;//normal
	return TRUE;
}

BOOL SendData(void)
{
	p = 100.0 / (double)fileSize;
	data = 0;
	do
	{
		if (fileSize >= 1024)
		{
			filesize = fileSize - 1024;
			DataPacketSize = send(sData, &DataBuf[data], 1024, 0);
		}
		else
		{
			filesize = fileSize;
			DataPacketSize = send(sData, &DataBuf[data], fileSize, 0);
		}
		if (SOCKET_ERROR == DataPacketSize)
		{
			PacketPtr += PacketSize;
			MessageBox(hwnd, "bad upload", ERROR, MB_OK);
			finished = TRUE;
			VirtualFree(DataBuf, 0, MEM_RELEASE);
			DataBuf = NULL;
			closesocket(sData);
			return FALSE;
		}
		data += DataPacketSize;
		if (fileSize > BUFSIZE)
		{
			percentage = (double)data * p;;
			sprintf(Counter, "   Progress: %.0f%%  (press any key to abort)", percentage);
			SetWindowText(hwnd, Counter);
		}
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
		{//the server will write the file to their disk as it's being uploaded so an abort will leave a partial file
			if (msg.message == WM_KEYDOWN)
			{
				MessageBox(hwnd, "bad upload\n\nThe file on the server\nis only a partial file!", ERROR, MB_OK);
				data = filesize;
			}
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	} while (data < filesize);
	DataPacketSize = send(sData, &DataBuf[data], fileSize-data, 0);
	if (SOCKET_ERROR == DataPacketSize)
	{
		PacketPtr += PacketSize;
		MessageBox(hwnd, "bad upload", ERROR, MB_OK);
		finished = TRUE;
		VirtualFree(DataBuf, 0, MEM_RELEASE);
		DataBuf = NULL;
		closesocket(sData);
		return FALSE;
	}
	VirtualFree(DataBuf, 0, MEM_RELEASE);
	DataBuf = NULL;
	closesocket(sData);
	return TRUE;
}

BOOL GetData(void)
{
	p = 100.0 / (double)filesize;
	data = 0;
	do
	{
		DataPacketSize = recv(sData, &DataBuf[data], BUFSIZE, 0);
		if (SOCKET_ERROR == DataPacketSize)
		{
			MessageBox(hwnd, "bad receive data", ERROR, MB_OK);
			closesocket(sData);
			DestroyWindow(hwndCounter);
			return FALSE;
		}
		data += DataPacketSize;
		if (filesize > BUFSIZE)
		{
			percentage = (double)data * p;
			sprintf(Counter, "   Progress: %.0f%%  (press any key to abort)", percentage);
			SetWindowText(hwndCounter, Counter);
		}
		if (download)
		{
			while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE))
			{
				if (msg.message == WM_KEYDOWN)
					DataPacketSize = 0;
				TranslateMessage(&msg);
				DispatchMessage(&msg);
			}
		}
	} while (DataPacketSize);

	closesocket(sData);
	DestroyWindow(hwndCounter);
	return TRUE;
}

BOOL PrepUpload(void)
{
	download = FALSE;
	SaveDir();
	if (cmdline)
	{
		for (x = filenameOffset, y = 0; Filename[x] != 0; x++, y++)
			ShortFilename[y] = Filename[x];
		ShortFilename[y] = 0;
	}
	InvalidateRect(hwndList, &rectList, FALSE);
	UpdateWindow(hwndList);
	if (INVALID_HANDLE_VALUE != (hUploadFile = CreateFile(Filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)))
	{
		fileSize = GetFileSize(hUploadFile, NULL);
		DataBuf = (char*)VirtualAlloc(NULL, fileSize+BUFSIZE, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		ReadFile(hUploadFile, DataBuf, fileSize, &dwBytesRead, NULL);
		CloseHandle(hUploadFile);
		for (x = 0, y = 5; ShortFilename[x] != 0; x++, y++)
			STOR[y] = ShortFilename[x];
		STOR[y++] = '\r';
		STOR[y++] = '\n';
		STOR[y] = 0;
		upload = TRUE;
		fromupload = TRUE;
		return TRUE;
	}
	else
	{
		MessageBox(hwnd, "couldn't be opened.", Filename, MB_OK);
		return FALSE;
	}
}

void FillHUP(void)
{
	int h, u, p;

	for (x = 0, h = 0; (h < 32) && (Server[indexServer].Addr[h] != 0); h++, x++)
		Host[x] = Server[indexServer].Addr[h];
	Host[x] = 0;
	for (x = 5, u = 0; (u < 32) && (Server[indexServer].User[u] != 0); u++, x++)
		User[x] = Server[indexServer].User[u];
	User[x++] = '\r';
	User[x++] = '\n';
	User[x] = 0;
	for (x = 5, p = 0; (p < 32) && (Server[indexServer].Pass[p] != 0); p++, x++)
		Pass[x] = Server[indexServer].Pass[p];
	Pass[x++] = '\r';
	Pass[x++] = '\n';
	Pass[x] = 0;
}

void RewriteIni(void)
{
	int i, x = 0, y;

	IniBuf[x++] = ((indexServer+1) / 10) + '0';
	IniBuf[x++] = ((indexServer+1) % 10) + '0';
	IniBuf[x++] = '\r';
	IniBuf[x++] = '\n';
	for (i = 0; i < iLast; i++)
	{
		for (y = 0; SERVER[y] != 0; x++, y++)
			IniBuf[x] = SERVER[y];
		for (y = 0; Server[i].Addr[y] != 0; x++, y++)
			IniBuf[x] = Server[i].Addr[y];
		IniBuf[x++] = '\r';
		IniBuf[x++] = '\n';
		for (y = 0; y < 5; x++, y++)
			IniBuf[x] = User[y];
		for (y = 0; Server[i].User[y] != 0;x++, y++)
			IniBuf[x] = Server[i].User[y];
		IniBuf[x++] = '\r';
		IniBuf[x++] = '\n';
		for (y = 0; y < 5; x++, y++)
			IniBuf[x] = Pass[y];
		for (y = 0; Server[i].Pass[y] != 0; x++, y++)
			IniBuf[x] = Server[i].Pass[y];
		IniBuf[x++] = '\r';
		IniBuf[x++] = '\n';
		for (y = 0; y < 12; x++, y++)
			IniBuf[x] = Init[y];
		for (y = 0; Server[i].Init[y] != 0; x++, y++)
			IniBuf[x] = Server[i].Init[y];
		IniBuf[x++] = '\r';
		IniBuf[x++] = '\n';
	}
	for (y = 0; DIR[y] != 0; x++, y++)
		IniBuf[x] = DIR[y];
	for (y = 0; CurrentDir[y] != 0; x++, y++)
		IniBuf[x] = CurrentDir[y];
	IniBuf[x++] = '\r';
	IniBuf[x++] = '\n';
	IniBuf[x] = 0;//for debugging only
	IniFileSize = x;
	hIniFile = CreateFile(MySimpleFTPIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	WriteFile(hIniFile, IniBuf, IniFileSize, &dwBytesRead, NULL);
	FlushFileBuffers(hIniFile);
	CloseHandle(hIniFile);
}

int CALLBACK NewProc(HWND hwndNewDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndAddr, hwndUser, hwndPass;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndAddr = GetDlgItem(hwndNewDlg, IDC_EDIT1);
		hwndUser = GetDlgItem(hwndNewDlg, IDC_EDIT2);
		hwndPass = GetDlgItem(hwndNewDlg, IDC_EDIT3);
		SetFocus(hwndAddr);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (0 == GetWindowText(hwndAddr, tempAddr, 32))
				break;
			if (0 == GetWindowText(hwndUser, tempUser, 32))
				break;
			if (0 == GetWindowText(hwndPass, tempPass, 32))
				break;
			strcpy(Server[iLast].Addr, tempAddr);
			strcpy(Server[iLast].User, tempUser);
			strcpy(Server[iLast].Pass, tempPass);
			iLast++; 
			RewriteIni();
			FillHUP();
			EndDialog (hwndNewDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndNewDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK ReadProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;
	RECT rect;

	switch (message)
	{
	case WM_INITDIALOG:
		MoveWindow(hwndDlg, (cxScreen/2)-400, 0, 800, cyScreen, TRUE);
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		GetClientRect(hwndDlg, &rect);
		MoveWindow(hwndEdit, 0, 0, rect.right, rect.bottom, TRUE);
		hFont = CreateFontIndirect (&lf);
		SendMessage(hwndEdit, WM_SETFONT, (WPARAM)(HFONT)hFont, FALSE);
		SetWindowText(hwndDlg, Filename);
		DataBuf[data] = 0;
		SetWindowText(hwndEdit, DataBuf);
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK ServersProc(HWND hwndServerDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x;
	static HWND hwndList, hwndAddr, hwndUser, hwndPass, hwndInit;
	static BOOL change, addrchange = FALSE;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndServerDlg2 = hwndServerDlg;
		hwndList = GetDlgItem(hwndServerDlg, IDC_LIST1);
		hwndUser = GetDlgItem(hwndServerDlg, IDC_EDIT1);
		hwndPass = GetDlgItem(hwndServerDlg, IDC_EDIT2);
		hwndAddr = GetDlgItem(hwndServerDlg, IDC_EDIT3);
		hwndInit = GetDlgItem(hwndServerDlg, IDC_EDIT4);
		for (x = 0; x < iLast; x++)
			SendMessage (hwndList, LB_INSERTSTRING, x, (LPARAM)Server[x].Addr);
		SendMessage (hwndList, LB_SETCURSEL, indexServer, 0);
		SetWindowText(hwndAddr, Server[indexServer].Addr);
		strcpy(tempAddr, Server[indexServer].Addr);
		SetWindowText(hwndUser, Server[indexServer].User);
		strcpy(tempUser, Server[indexServer].User);
		SetWindowText(hwndPass, Server[indexServer].Pass);
		strcpy(tempPass, Server[indexServer].Pass);
		SetWindowText(hwndInit, Server[indexServer].Init);
		strcpy(tempInit, Server[indexServer].Init);
		SetFocus(hwndList);
		change = FALSE;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LIST1:
			if (HIWORD (wParam) == LBN_SELCHANGE)
			{
				indexServer = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
				SetWindowText(hwndAddr, Server[indexServer].Addr);
				strcpy(tempAddr, Server[indexServer].Addr);
				SetWindowText(hwndUser, Server[indexServer].User);
				strcpy(tempUser, Server[indexServer].User);
				SetWindowText(hwndPass, Server[indexServer].Pass);
				strcpy(tempPass, Server[indexServer].Pass);
				SetWindowText(hwndInit, Server[indexServer].Init);
				strcpy(tempInit, Server[indexServer].Init);
				break;
			}
			else if (HIWORD (wParam) == LBN_DBLCLK)
			{
				EndDialog (hwndServerDlg, TRUE);
				return TRUE;
			}
			break;

		case IDC_EDIT1:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				GetWindowText(hwndUser, tempUser, 32);
				if (strcmp(Server[indexServer].User, tempUser))
				{
					strcpy(Server[indexServer].User, tempUser);
					change = TRUE;
				}
			}
			break;

		case IDC_EDIT2:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				GetWindowText(hwndPass, tempPass, 32);
				if (strcmp(Server[indexServer].Pass, tempPass))
				{
					strcpy(Server[indexServer].Pass, tempPass);
					change = TRUE;
				}
			}
			break;

		case IDC_EDIT3:
			if (HIWORD(wParam) == EN_CHANGE)
			{
				GetWindowText(hwndAddr, tempAddr, 32);
				if (strcmp(Server[indexServer].Addr, tempAddr))
				{
					strcpy(Server[indexServer].Addr, tempAddr);
					change = TRUE;
					addrchange = TRUE;
				}
			}
			else if (HIWORD(wParam) == EN_KILLFOCUS)
			{
				if (addrchange)
				{
					addrchange = FALSE;
					SendMessage(hwndList, LB_DELETESTRING, indexServer, 0);
					SendMessage(hwndList, LB_INSERTSTRING, indexServer, (LPARAM)tempAddr);
				}
			}
			break;						

		case IDC_EDIT4://Initial Dir
			if (HIWORD(wParam) == EN_CHANGE)
			{
				GetWindowText(hwndInit, tempInit, 64);
				if (strcmp(Server[indexServer].Init, tempInit))
				{
					strcpy(Server[indexServer].Init, tempInit);
					change = TRUE;
				}
			}
			break;

		case IDC_BUTTON1://New
			if (DialogBox(hInst, "NEW", NULL, NewProc))
				SendMessage (hwndList, LB_INSERTSTRING, iLast-1, (LPARAM)Server[iLast-1].Addr);
			SetFocus(hwndList);
			break;

		case IDC_BUTTON2://Remove
			if (IDYES == MessageBox(hwndList, Server[indexServer].Addr, "Remove this server?", MB_YESNO|MB_DEFBUTTON2))
			{
				for (x = indexServer; x < iLast; x++)
					Server[x] = Server[x+1];
				if (indexServer == iLast-1)
					indexServer--;
				iLast--; 
				RewriteIni();
				FillHUP();
				SendMessage (hwndList, LB_RESETCONTENT, 0, 0);
				for (x = 0; x < iLast; x++)
					SendMessage (hwndList, LB_INSERTSTRING, x, (LPARAM)Server[x].Addr);
				SendMessage (hwndList, LB_SETCURSEL, indexServer, 0);
				SetWindowText(hwndAddr, Server[indexServer].Addr);
				strcpy(tempAddr, Server[indexServer].Addr);
				SetWindowText(hwndUser, Server[indexServer].User);
				strcpy(tempUser, Server[indexServer].User);
				SetWindowText(hwndPass, Server[indexServer].Pass);
				strcpy(tempPass, Server[indexServer].Pass);
				SetWindowText(hwndInit, Server[indexServer].Init);
				strcpy(tempInit, Server[indexServer].Init);
				SetFocus(hwndList);
			}
			break;

		case IDC_BUTTON3:
			MessageBox(hwndList, ServerHelp, szAppName, MB_OK);
			break;

		case IDOK://Connect
			closesocket(s);
			FillHUP();
			if (change)
				RewriteIni();
			else
			{
				IniBuf[0] = ((indexServer+1) / 10) + '0';
				IniBuf[1] = ((indexServer+1) % 10) + '0';
				hIniFile = CreateFile(MySimpleFTPIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				WriteFile(hIniFile, IniBuf, IniFileSize, &dwBytesRead, NULL);
				FlushFileBuffers(hIniFile);
				CloseHandle(hIniFile);
			}
//			if (Server[indexServer].Init)
//				strcpy(DirFilename, Server[indexServer].Init);
//			else
//				DirFilename[0] = '/';
			EndDialog (hwndServerDlg, TRUE);
			return TRUE;

		case IDCANCEL://Exit
			EndDialog (hwndServerDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}
