#include <winsock2.h>
#include <windows.h>
#include <tlhelp32.h>//for PROCESSENTRY32 pe
#include <psapi.h>//put psapi.lib in Project -Settings -Link
#include "resource.h"
#define FROMTRAY WM_USER+1
#define BUFSIZE 1000000
#define TAB 9
#define KEYLENGTH 1024

char Version[] = "Version 2.1 July 17, 2007 Doug Cox jdmcox@jdmcox.com";

DWORD dwCrc32, i, Processes[1024], NumOfProcesses;
char MyNetworkServer[] = "MyNetworkServer";
int FileSize, FilePtr;
BYTE Password[8];
char Port[6];
char RegValue[MAX_PATH];
DWORD RegValueSize = MAX_PATH, RegType = REG_SZ;
char MyNetworkIni[MAX_PATH] = "MyNetworkConfiguration.txt";
char IniBuf[1000];
char FileName[50];
int keyTemplateSize;
DWORD num, Length;
//char Drives[100];
int x, y, z, BytesSent, PacketSize, count;
char Tip[64] = "MyNetworkServer";
char RequestBuf[BUFSIZE];//BYTE
char Dir[MAX_PATH];
char CurrentDir[MAX_PATH];
char ProcessName[100];
BYTE *FileBuf;
BYTE *DirBuf;
char *ProcessNames;
HKEY hRegKey;
DWORD fileSize, dwBytesRead, dwBytesWritten;
BOOL iRet, status, gotparent, gettingfile, gotit = FALSE;
HANDLE hFile, hFile2, hFile3, hFind, hIcon;
HINSTANCE hInst;
WIN32_FIND_DATA fd;
FILETIME ft;
SYSTEMTIME st;
WSADATA wsaData;
SOCKADDR_IN saServer;		
SOCKET sListen, sAccept = INVALID_SOCKET;
PROCESSENTRY32 pe;
NOTIFYICONDATA nid;
POINT pt;
HMENU hmenu, hpopup;

LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	HWND hwnd;
	MSG	msg;
	WNDCLASS wndclass;
	hInst = hInstance;

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = NULL;
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)(COLOR_MENU+1);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = MyNetworkServer;

	RegisterClass (&wndclass);

	hwnd = CreateWindow(MyNetworkServer, MyNetworkServer, 
		WS_POPUP | WS_CAPTION | WS_SYSMENU,
		0, 0, 0, 0,
		NULL, NULL, hInstance, NULL);

	iCmdShow = SW_HIDE;
	ShowWindow (hwnd, iCmdShow);
	UpdateWindow (hwnd);

	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}

void GetDirBuf(void)
{
	int hour;
	char ThisFileSize[10];

	if ((fd.dwFileAttributes & FILE_ATTRIBUTE_HIDDEN) || (fd.dwFileAttributes & FILE_ATTRIBUTE_SYSTEM))
		return;
	if ((fd.cFileName[0] != '.') || (fd.cFileName[1] == '.'))
	{
		if ((fd.cFileName[0] == '.') && (fd.cFileName[1] == '.'))
			gotparent = TRUE;
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			DirBuf[x++] = '/';
		for (y = 0; fd.cFileName[y] != 0; x++, y++)
			DirBuf[x] = fd.cFileName[y];
		if (fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
			DirBuf[x++] = '/';
		else
		{
			DirBuf[x++] = TAB;
			_itoa(fd.nFileSizeLow, ThisFileSize, 10);
			for (y = 0; ThisFileSize[y] != 0; x++, y++)
				DirBuf[x] = ThisFileSize[y];
			DirBuf[x++] = TAB;
			FileTimeToLocalFileTime(&fd.ftLastWriteTime, &ft);
			FileTimeToSystemTime(&ft, &st);
			if (st.wMonth > 9)
				DirBuf[x++] = (st.wMonth / 10) + '0';
			DirBuf[x++] = (st.wMonth % 10) + '0';
			DirBuf[x++] = '/';
			if (st.wDay > 9)
				DirBuf[x++] = (st.wDay / 10) + '0';
			DirBuf[x++] = (st.wDay % 10) + '0';
			DirBuf[x++] = '/';
			DirBuf[x++] = (st.wYear / 1000) + '0';//1234
			DirBuf[x++] = ((st.wYear % 1000) / 100) + '0';
			DirBuf[x++] = ((st.wYear % 100) / 10) + '0';
			DirBuf[x++] = (st.wYear % 10) + '0';
			DirBuf[x++] = ' ';
			hour = st.wHour;
			if (hour > 12)
				hour -= 12;
			if (hour > 9)
				DirBuf[x++] = (hour / 10) + '0';
			DirBuf[x++] = (hour % 10) + '0';
			DirBuf[x++] = ':';
			DirBuf[x++] = (st.wMinute / 10) + '0';
			DirBuf[x++] = (st.wMinute % 10) + '0';
			DirBuf[x++] = ' ';
			if (st.wHour < 12)
				DirBuf[x++] = 'A';
			else
				DirBuf[x++] = 'P';
			DirBuf[x++] = 'M';
		}
		DirBuf[x++] = '\r';
		DirBuf[x++] = '\n';
	}
}

void SendDirectory(void)
{
	if (DirBuf == NULL)
		DirBuf = (BYTE*)VirtualAlloc(NULL, 1000000, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	GetCurrentDirectory(MAX_PATH, Dir);
	for (x = 4, y = 0; Dir[y] != 0; x++, y++)//save room for filesize
		DirBuf[x] = Dir[y];
	DirBuf[x++] = '\r';
	DirBuf[x++] = '\n';

	gotparent = FALSE;
	hFind = FindFirstFile("*.*", &fd);
	GetDirBuf();//uses & increments x
	while (FindNextFile(hFind, &fd))
		GetDirBuf();//uses & increments x
	FindClose(hFind);
/*
	if (gotparent == FALSE)
	{
		Length = GetLogicalDriveStrings(100, Drives);
		for (y = 0; (y < 100) && (Drives[y] != 0); )
		{
			if (Drives[y] != Dir[0])
			{
				DirBuf[x++] = ' ';
				for ( ; Drives[y] != 0; x++, y++)
					DirBuf[x] = Drives[y];
				DirBuf[x++] = '\r';
				DirBuf[x++] = '\n';
			}
			else
				for ( ; (y < 100) && (Drives[y] != 0); y++)
					;
			y++;
		}
	}
*/
	fileSize = x;
	DirBuf[0] = (BYTE)fileSize & 0xFF;
	DirBuf[1] = (BYTE)(fileSize >> 8) & 0xFF;
	DirBuf[2] = (BYTE)(fileSize >> 16) & 0xFF;
	DirBuf[3] = (BYTE)(fileSize >> 24) & 0xFF;
	y = 0;

	do
	{
		x = (int)fileSize - y;
		//**********************************************
		BytesSent = send(sAccept, (char*)&DirBuf[y], x, 0);
		//**********************************************
		if ((BytesSent != 0) && (BytesSent != -1))
			y += BytesSent;
		else
			break;
	} while (y < (int)fileSize);
	VirtualFree(DirBuf, 0, MEM_RELEASE);
	DirBuf = NULL;
}

void GetProcessName(DWORD processID)
{
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

DWORD Crc32Table[256] =
{// Static CRC table
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
		mov esi, DWORD ptr buffer
		mov ebx, fSize
		lea edx, DWORD ptr [esi+ebx]
	crc32loop:
		xor eax, eax
		mov bl, byte ptr [esi]
		mov al, cl
		inc esi
		xor al, bl
		shr ecx, 8
		mov ebx, DWORD ptr [edi+eax*4]
		xor ecx, ebx
		cmp edx, esi
		jne crc32loop
		pop edi
		pop esi
		not ecx
		mov DWORD ptr dwCrc32, ecx
	}
}

LRESULT CALLBACK WndProc (HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
	switch (msg)
	{
	case WM_CREATE:
		hFile = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			pe.dwSize = sizeof(pe);
			status = Process32First(hFile, &pe);
			count = 0;
			while(status == TRUE)
			{//retain only the file name portion of pe.szExeFile
				memset(ProcessName, 0, sizeof(ProcessName));
				for(x = strlen(pe.szExeFile); x != 0; x--)
				{
					if(pe.szExeFile[x] == '\\')
					{
						x++;
						break;
					}
				}
				strcpy(ProcessName, pe.szExeFile + x);
				if(_stricmp("MyNetworkServer.exe", ProcessName) == 0)
					count++;
				status = Process32Next(hFile, &pe);
			}
			CloseHandle(hFile);
			if(count > 1)
			{
				MessageBox(hwnd, "It's already running.", ERROR, MB_OK);
				DestroyWindow(hwnd);
				return 0;
			}
		}
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
		hIcon = LoadImage(hInst, "MYNETWORKSERVER", IMAGE_ICON, 0, 0, LR_DEFAULTSIZE);//the following deserves thanks to tray42
		nid.cbSize = sizeof(NOTIFYICONDATA);
		nid.hWnd = hwnd;
		nid.hIcon = (struct HICON__*)hIcon;
		nid.uFlags = NIF_ICON|NIF_TIP|NIF_MESSAGE;
		nid.uCallbackMessage = FROMTRAY;
		nid.uID = 499;
		for (x = 0; x < 64; x++)
			nid.szTip[x] = Tip[x];
		if (Shell_NotifyIcon(NIM_ADD, &nid))
			gotit = TRUE;

		if (INVALID_HANDLE_VALUE != (hFile = CreateFile(MyNetworkIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL)))
		{
			fileSize = GetFileSize(hFile, NULL);
			ReadFile(hFile, IniBuf, fileSize, &dwBytesRead, NULL);
			CloseHandle(hFile);

			x = 0;
			while (TRUE)
			{
				for ( ; (x < (int)fileSize) && (IniBuf[x] != '='); x++)
					;
				if (x >= (int)fileSize)
					break;
				else if ((IniBuf[x-1] == 'T') && (IniBuf[x-4] == 'P'))
				{//"PORT="
					x++;
					for (y = 0; (IniBuf[x] != '\r') && (y < 6); x++, y++) 
						Port[y] = IniBuf[x];
				}
				else if ((IniBuf[x-1] == 'D') && (IniBuf[x-8] == 'P'))
				{//"PASSWORD="
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
			MessageBox(hwnd, "Couldn't find 'MyNetworkConfiguration.txt'", CurrentDir, MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		FileBuf = NULL;
		DirBuf = NULL;
		iRet = WSAStartup(0x0202, &wsaData);
		if (iRet == 0) {
			sListen = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
			if (iRet == 0) {
				iRet = WSAAsyncSelect(sListen, hwnd, WM_USER, FD_ACCEPT|FD_READ);
				if (iRet == 0) {
					saServer.sin_family = AF_INET;
					saServer.sin_addr.s_addr = INADDR_ANY;
					saServer.sin_port = htons(atoi(Port));
					iRet = bind(sListen, (LPSOCKADDR)&saServer, sizeof(struct sockaddr));
					if (iRet == 0) {
						iRet = listen(sListen, SOMAXCONN);
						if (iRet != 0) {
							MessageBox(NULL, "Error", ERROR, MB_OK);
						}
					}
					else MessageBox(NULL, "Error", ERROR, MB_OK);
				}
				else MessageBox(NULL, "Error", ERROR, MB_OK);
			}
			else MessageBox(NULL, "Error", ERROR, MB_OK);
		}
		else MessageBox(NULL, "Error", ERROR, MB_OK);
		return 0;

	case FROMTRAY:
		switch (lParam)
		{
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
			GetCursorPos(&pt);
			hmenu = LoadMenu(hInst, MAKEINTRESOURCE(IDR_MENU1));
			hpopup = GetSubMenu(hmenu, 0);
			SetForegroundWindow(hwnd);
			if (ID_EXIT == TrackPopupMenu(hpopup, TPM_RETURNCMD|TPM_RIGHTBUTTON, pt.x, pt.y, 0, hwnd, NULL))
				DestroyWindow(hwnd);
			break;
		}
		return 0;

	case WM_USER:
		switch (LOWORD(lParam))
		{
		case FD_ACCEPT:
			sAccept = accept(sListen, NULL, NULL);
			if (INVALID_SOCKET == sAccept)
			{
				MessageBox(NULL, "Error", ERROR, MB_OK);
				return 0;
			}
			break;

//receive either the Password, a request for a directory, or a request for a file
		case FD_READ:
			//*************************************************
			PacketSize = recv(sAccept, (char*)RequestBuf, 0xFFFF, 0);
			//*************************************************
			if ((PacketSize == SOCKET_ERROR) || (PacketSize == 0))
			{
				DestroyWindow(hwnd);
				break;
			}
			if ((*(DWORD*)&RequestBuf[0] != *(DWORD*)&Password[0]) || (*(DWORD*)&RequestBuf[4] != *(DWORD*)&Password[4]))
			{
				send(sAccept, "\0\0\0\0Bad password", 16, 0);
				return 0;
			}
			if (PacketSize == 8)
			{
				{//send initial directory
					gettingfile = FALSE;
					//*************
					SendDirectory();
					//*************
				}
			}

			else
			{
				if ((RequestBuf[8] == '[') || (RequestBuf[8] == ' '))
				{//request for another directory or drive
					for (x = 0, y = 9; (RequestBuf[y] != ']') && (RequestBuf[y] != 0); x++, y++)
						Dir[x] = RequestBuf[y];
					Dir[x] = 0;
					SetCurrentDirectory(Dir);
					//*************
					SendDirectory();
					//*************
				}
				else
				{//request for a file (named in RequestBuf)					
					EnumProcesses(Processes, 1024, &NumOfProcesses);
					NumOfProcesses /= 4;//change number of bytes to number of DWORDs
					ProcessNames = (char*)malloc(NumOfProcesses*256);
					num = 0;
					for ( i = 0; i < NumOfProcesses; i++ )
					{
						if (Processes[i] != 0)
							GetProcessName(Processes[i]);
					}
					for (i = 0; i < num; i += 256)
					{
						if (0 == strcmp(&RequestBuf[8], &ProcessNames[i]))
							break;//file is running on this computer
					}

					if (i == num)
					{//file isn't running on this computer
						hFile2 = CreateFile((char*)&RequestBuf[8], GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
						if (hFile2 != INVALID_HANDLE_VALUE)
						{
							if (fileSize = GetFileSize(hFile2, NULL))
							{
								if (FileBuf != NULL)
									VirtualFree(FileBuf, 0, MEM_RELEASE);
								FileBuf = (BYTE*)VirtualAlloc(NULL, fileSize+8, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
								ReadFile(hFile2, &FileBuf[8], fileSize, &dwBytesWritten, NULL);
								CloseHandle(hFile2);
								*(DWORD*)&FileBuf[0] = fileSize;
							}
							else
							{
								CloseHandle(hFile2);
								return 0;
							}
						}
						else
							return 0;
					}
					else
					{//"Can't copy this file while it's running on this computer."
						fileSize = 0;
						FileBuf = (BYTE*)VirtualAlloc(NULL, fileSize+8, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
						FileBuf[0] = (unsigned char)0xFF;
						FileBuf[1] = (unsigned char)0xFF;
						FileBuf[2] = (unsigned char)0xFF;
						FileBuf[3] = (unsigned char)0xFF;
					}

					Crc32Assembly(&FileBuf[8], fileSize);
					*(DWORD*)&FileBuf[4] = dwCrc32;
					fileSize += 8;
					y = 0;
					do
					{
						x = (int)fileSize - y;
						//***************************************
						BytesSent = send(sAccept, (char*)&FileBuf[y], x, 0);
						//***************************************
						if ((BytesSent != 0) && (BytesSent != -1))
							y += BytesSent;
						else
							break;
					} while (y < (int)fileSize);

					if (FileBuf != NULL)
					{
						VirtualFree(FileBuf, 0, MEM_RELEASE);
						FileBuf = NULL;
					}
				}
			}//end of else

/*
			else if ((RequestBuf[0] == 0xFF)
				 && (RequestBuf[1] == 0xFF)
				 && (RequestBuf[2] == 0xFF)
				 && (RequestBuf[3] == 0xFF))
			{//receiving file
				int x, y;
				char Something[] = "C:\\A Network Folder\\";

				gettingfile = TRUE;
				for (x = 0; Something[x] != 0; x++)
					FileName[x] = Something[x];
				for (y = 4; RequestBuf[y] != 0xFF; x++, y++)
					FileName[x] = RequestBuf[y];
				FileName[x] = 0;
				y++;
				FileSize = *(DWORD*)&RequestBuf[y];
				y += 4;
				hFile3 = CreateFile(FileName, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				WriteFile(hFile3, &RequestBuf[y], PacketSize - y, &dwBytesWritten, NULL);
				if (PacketSize == (FileSize + y))
				{
					CloseHandle(hFile3);
					gettingfile = FALSE;
				}
				FilePtr = PacketSize - y;
			}
			else if (gettingfile)
			{
				WriteFile(hFile3, RequestBuf, PacketSize, &dwBytesWritten, NULL);
				FilePtr += PacketSize;
				if (FilePtr == FileSize)
				{
					CloseHandle(hFile3);
					gettingfile = FALSE;
				}
			}
*/
		}
		return 0;

	case WM_DESTROY:
		if (gotit)
			Shell_NotifyIcon(NIM_DELETE, &nid);
		shutdown(sAccept, SD_BOTH);
		closesocket(sListen);
		if (sAccept != INVALID_SOCKET)
			closesocket(sAccept);
		WSACleanup();

		if (FileBuf != NULL)
			VirtualFree(FileBuf, 0, MEM_RELEASE);
		if (DirBuf != NULL)
			VirtualFree(DirBuf, 0, MEM_RELEASE);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, msg, wParam, lParam);
}
