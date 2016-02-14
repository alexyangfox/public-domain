#include <windows.h>
#include <stdio.h>//for sprintf
#include "ijl.h"//put ijl15.lib in Project/Settings/Link
#include "resource.h"
#define	MAXFILES 2000
#define EXIFSIZE 512-MAX_PATH-5

BOOL fuck = FALSE;

int jpgSize, blackwhite, threshold, UpDown = 0, LeftRight = 0;
int x, y, z, yPos, Width, Height, CmdLineLen = 0, num, CurrentPhoto;
int xLoc = 0, yLoc = 0, xSmall, ySmall, SmallWidth, SmallHeight;
int Name[MAXFILES];//sorted pointers to Files
DWORD dw, fileSize, dwBytesRead, dwBytesWritten, BitmapInfoSize, BitmapSize;
BYTE *Buf, *Pixels, *RotatedPixels;
BYTE ExifDta[13] = "\0\0\0\0\0\0\0\0\0\0\0\0\0";
BOOL first = TRUE, veryfirst = TRUE, itsfucked = FALSE, buttondown = FALSE, showinfo = FALSE, flash, fillscreen = FALSE, rotate = FALSE, bitmap = FALSE;
BOOL nodown = 0, noup = 0, noleft = 0, noright = 0;
//      0       1        2      3     4      5       6           7          8         9            10             11
BOOL camera, datetime, fstop, speed, iso, focalen, flash, exposureprogram, size, meteringmode, exposuremode, whitebalance;

DWORD RegType = REG_SZ, RegValueSize = MAX_PATH;
char SubKey[] = "Software\\ViewPhotos";
char FillScreen[] = "Fill Screen";
char FullPhoto[] = "Full Photo";
HKEY hRegKey;

struct {
	WORD Tag;
	WORD Type;
	DWORD Count;
	DWORD Offset;
} IFD[50];

//char Rows[8], Cols[8];
char Filename[MAX_PATH];// = "C:\\My Documents\\ViewPhotos\\Sarai.jpg";
char ShortFilename[MAX_PATH];
char szAppName[] = "ViewPhotos";
char Ini[] = "ViewPhotos.ini";
char BigIni[MAX_PATH];
char CurrentDir[MAX_PATH];
char ExifData[] = "Exif.dta";
char BigExifData[MAX_PATH];
unsigned char ViewPhotosFolder[MAX_PATH];
char PhotoBuffer[MAX_PATH];
char RedEye[MAX_PATH+7] = "RedEye ";
char jpg[] = "*.jpg";
char bmp[] = "*.bmp";
char *pointer;
char Files[MAXFILES][MAX_PATH];
char TopLine[512];

char ExifInfo[EXIFSIZE];
char Manual[] = "Manual";
char Aperture[] = "Aperture priority";
char Speed[] = "Shutter priority";
char Creative[] = "Creative";
char Action[] = "Action";
char Portrait[] = "Portrait";
char Landscape[] = "Landscape";
char ExposureProg[] = " exposure program";
char AutoExposure[] = "Auto exposure";
char ManualExposure[] = "Manual exposure";
char AutoBracket[] = "Auto bracket";
char AutoWB[] = "Auto whitebalance";
char ManualWB[] = "Manual whitebalance";
char Average[] = "Average";
char CenterWeighted[] = "Center-weighted";
char Spot[] = "Spot";
char MultiSpot[] = "Multi spot";
char Pattern[] = "Pattern";
char Partial[] = "Partial";
char MeteringMode[] = " metering mode";

char Help[] = "\
Move the mouse pointer near the top of the photo to show its name and the\n\
Fill Screen, Options, RedEye, Help, and Exit buttons.\n\
Move the mouse pointer away from the top to stop showing the menu bar.\n\
\n\
Press Arrow keys, PgUp, Pgdn, Home, End,  Enter, or the spacebar to go forward\n\
or backward through the photos in the folder that contained the initial photo selected.\n\
\n\
If you've selected Fill Screen, holding the Ctrl key down while pressing the Arrow keys\n\
will scroll the photo instead of going forward or backward through the photos.\n\
\n\
Hold the left mouse button down to show the photo in its full size.\n\
What's at the mouse pointer will be centered.\n\
\n\
When you press the RedEye button, the RedEye program will open the photo.\n\
\n\
Rotating a photo will only rotate it 180 degrees while viewing it.\n\
\n\
Press the Delete key to delete a photo.\n\
\n\
Press Esc to exit ViewPhotos.";

HWND hwnd, hwndInfo = NULL, hwndExif, hwndRedEye, hwndRotate, hwndHelp, hwndExit, hwndFillScreen;
HINSTANCE hInstance;
HANDLE hFile, hFindFile;
HBRUSH hBrush;
HDC hdc, hdcMem = NULL, hdcCopy = NULL, hdcInfo = NULL;
HBITMAP hBitmap = NULL, hBitmapCopy = NULL, hBitmapInfo = NULL;
PAINTSTRUCT ps;
RECT rect, rectScreen, rectJPG, rectLeft, rectRight, rectTop, rectBottom;
WIN32_FIND_DATA fd;
OPENFILENAME ofn;
BITMAPFILEHEADER bmfh;
BITMAPINFOHEADER bmih, *pbmih = &bmih;
BITMAPINFO bmi, *pbmi = &bmi;
JPEG_CORE_PROPERTIES jcprops;
IJLERR jerr;
SHFILEOPSTRUCT shfo;
WNDPROC pInstrProc;

void GetFile(void);
void Exif(void);
int CALLBACK ExifProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR CmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(hInstance,MAKEINTRESOURCE (IDI_ICON1));//LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = NULL;
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	if (CmdLine[0] != 0)
	{
		for (x = 0, CmdLineLen = 0; CmdLine[x] != 0; x++)
			if (CmdLine[x] != '"')
				Filename[CmdLineLen++] = CmdLine[x];
		Filename[CmdLineLen] = 0;
	}

	hwnd = CreateWindow(szAppName, NULL,
		WS_EX_TOPMOST | WS_POPUP | WS_VISIBLE,//to use FULL screen (not Windowed)
		CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
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
LRESULT CALLBACK InstrProc(HWND hwnd3, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_COMMAND)
	{
		if ((HWND)(LPARAM)lParam == hwndFillScreen)
		{
			if (!fillscreen)
			{
				fillscreen = TRUE;
				SetWindowText(hwndFillScreen, FullPhoto);
			}
			else
			{
				fillscreen = FALSE;
				SetWindowText(hwndFillScreen, FillScreen);
			}
			first = TRUE; // slightly trick
			InvalidateRect(hwnd, &rectScreen, FALSE);
		}
		else if ((HWND)(LPARAM)lParam == hwndRedEye)
		{
			if (Pixels != NULL)
				VirtualFree(Pixels, 0, MEM_RELEASE);
			if (hdcMem)
				DeleteDC(hdcMem);
			if (hBitmap)
				DeleteObject(hBitmap);
			if (hdcInfo)
				DeleteDC(hdcInfo);
			if (hBitmapInfo)
				DeleteObject(hBitmapInfo);
			for (x = 7, y = 0; Files[CurrentPhoto][y] != 0; x++, y++)
				RedEye[x] = Files[CurrentPhoto][y];
			RedEye[x] = 0;
			if (WinExec(RedEye, SW_SHOW) <= 31)
				MessageBox(hwnd, "RedEye.exe isn't in the same folder ViewPhotos.exe is in.", ERROR, MB_OK);
			first = TRUE;
		}
		else if ((HWND)(LPARAM)lParam == hwndExif)
		{
			if (DialogBox(hInstance, "EXIF", NULL, ExifProc))
				first = TRUE;
		}
		else if ((HWND)(LPARAM)lParam == hwndRotate)
		{
			first = TRUE;
			if (!rotate)
				rotate = TRUE;
			else
				rotate = FALSE;
			ReleaseDC(hwnd, hdc);
			showinfo = TRUE;
			InvalidateRect(hwnd, &rectScreen, FALSE);
			SetFocus(hwnd);
			return 0;
		}
		else if ((HWND)(LPARAM)lParam == hwndHelp)
		{
			MessageBox(hwnd, Help, szAppName, MB_OK);
			SetFocus(hwnd);
		}
		else if ((HWND)(LPARAM)lParam == hwndExit)
		{
			DestroyWindow(hwnd);
		}
	}
	return CallWindowProc(pInstrProc, hwnd3, message, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_SIZE:
		if (lParam)
		{
			rectScreen.left = rectScreen.top = 0;
			rectScreen.right = LOWORD(lParam);
			rectScreen.bottom = HIWORD(lParam);
		}
		return 0;

	case WM_CREATE:
		Pixels = Buf = NULL;
		hBrush = CreateSolidBrush(0x404040);//gray
		ViewPhotosFolder[0] = 0;
		BigExifData[0] = 0;
		if (ERROR_SUCCESS == RegOpenKey(HKEY_LOCAL_MACHINE, SubKey, &hRegKey))
		{
			if (ERROR_SUCCESS == RegQueryValueEx(hRegKey, "Install_Dir", NULL, &RegType, ViewPhotosFolder, &RegValueSize))
			{
				for (x = 0, y = 0; ViewPhotosFolder[y] != 0; x++, y++)
					BigExifData[x] = ViewPhotosFolder[y];
				BigExifData[x++] = '\\';
				for (y = 0; ExifData[y] != 0; x++, y++)
					BigExifData[x] = ExifData[y];
				BigExifData[x] = 0;
				hFile = CreateFile(BigExifData, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					if (GetFileSize(hFile, NULL))
					{
						ReadFile(hFile, ExifDta, 13, &dwBytesRead, NULL);
						camera = ExifDta[0];
						datetime = ExifDta[1];
						fstop = ExifDta[2];
						speed = ExifDta[3];
						iso = ExifDta[4];
						focalen = ExifDta[5];
						flash = ExifDta[6];
						exposureprogram = ExifDta[7];
						size = ExifDta[8];
						meteringmode = ExifDta[9];
						exposuremode = ExifDta[10];
						whitebalance = ExifDta[11];
						fillscreen = ExifDta[12]; // not really exif data
						for (x = 0; x < 12; x++)
							if (ExifDta[x] != FALSE)
								veryfirst = FALSE;
					}
					CloseHandle(hFile);
				}
			}
			RegCloseKey(hRegKey);
		}

		if (CmdLineLen)
		{
			for (x = CmdLineLen; (x > 0) && (Filename[x] != '\\'); x--)
				;
			for (x++, y = 0; x < CmdLineLen; x++, y++)
				ShortFilename[y] = Filename[x];
			ShortFilename[y] = 0;
			for ( ; (x > 0) && (Filename[x] != '\\'); x--)
				;
			Filename[x] = 0;

			GetCurrentDirectory(MAX_PATH, CurrentDir);
		}
		else
		{
			ofn.lStructSize       = sizeof(OPENFILENAME);
			ofn.hwndOwner         = hwnd;
			ofn.hInstance         = NULL;
			ofn.lpstrFilter       = "*.jpg;*.bmp\0"" *.jpg;*.bmp\0\0";
			ofn.lpstrFile         = Filename;
			ofn.lpstrFileTitle    = ShortFilename;
			ofn.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
			ofn.lpstrTitle        = NULL;
			ofn.lpstrDefExt       = jpg;
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

			for (x = 0, y = 0; ViewPhotosFolder[y] != 0; x++, y++)
				BigIni[x] = ViewPhotosFolder[y];
			BigIni[x++] = '\\';
			for (y = 0; Ini[y] != 0; x++, y++)
				BigIni[x] = Ini[y];
			BigIni[x] = 0;
			hFile = CreateFile(BigIni, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				fileSize = GetFileSize(hFile, NULL);
				if (fileSize <= MAX_PATH)
				{
					ReadFile(hFile, CurrentDir, fileSize, &dwBytesRead, NULL) ;
					SetCurrentDirectory(CurrentDir);
				}
				CloseHandle(hFile);
			}
			if (GetOpenFileName(&ofn))
			{
				for (x = 0; Filename[x] != 0; x++)
					;
				for ( ; (x != 0) && (Filename[x] != '\\'); x--)
					;
				Filename[x] = 0;
				SetCurrentDirectory(Filename);
				GetCurrentDirectory(MAX_PATH, CurrentDir);
				Filename[x] = '\\';
				for (x++, y = 0; Filename[x] != 0; x++, y++)
					ShortFilename[y] = Filename[x];
				ShortFilename[y] = 0;
			}
			else
			{
				DestroyWindow(hwnd);
				return 0;
			}
		}
		for (y = 0; y < MAXFILES; y++)
		{//initialize
			for (x = 0; Files[y][x] != 0; x++)
				Files[y][x] = 0;
			Name[y] = 0;
		}
		num = 0;
		hFindFile = FindFirstFile(jpg, &fd);
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			GetFile();
			while ((FindNextFile(hFindFile, &fd)) && (num < MAXFILES))
				GetFile();
			FindClose(hFindFile);
			if (num == MAXFILES)
				MessageBox(hwnd, "There are over 2,000 files in this directory!", ERROR, MB_OK);
		}
		hFindFile = FindFirstFile(bmp, &fd);
		if (hFindFile != INVALID_HANDLE_VALUE)
		{
			GetFile();
			while ((FindNextFile(hFindFile, &fd)) && (num < MAXFILES))
				GetFile();
			FindClose(hFindFile);
			if (num == MAXFILES)
				MessageBox(hwnd, "There are over 2,000 files in this directory!", ERROR, MB_OK);
		}
		bmih.biSize = sizeof(BITMAPINFOHEADER);
		bmih.biPlanes = 1;
		bmih.biBitCount = 24;
		bmih.biCompression = BI_RGB;
		bmih.biSizeImage = 0;
		bmih.biXPelsPerMeter = 0;
		bmih.biYPelsPerMeter = 0;
		bmih.biClrUsed = 0;
		bmih.biClrImportant = 0;
		return 0;

	case WM_LBUTTONDOWN:
		if ((Width > rectScreen.right) || (Height > rect.bottom))
		{//center on the mouse pointer
			xSmall = LOWORD(lParam);
			ySmall = HIWORD(lParam);
			SmallWidth = rectScreen.right;
			SmallHeight = rectScreen.bottom;

			x = xSmall*Width/SmallWidth;
			if (x <= (SmallWidth/2))
				xLoc = 0;
			else
			{
				xLoc = x - (SmallWidth/2);
				if (xLoc > (Width - SmallWidth))
					xLoc = Width - SmallWidth;
			}

			y = ySmall*Height/SmallHeight;
			if (y <= (SmallHeight/2))
				yLoc = 0;
			else
			{
				yLoc = y - (SmallHeight/2);
				if (yLoc > (Height - SmallHeight))
					yLoc = Height - SmallHeight;
			}

			buttondown = TRUE;
			InvalidateRect(hwnd, &rectScreen, FALSE);
		}
		return 0;

	case WM_LBUTTONUP:
		buttondown = FALSE;
		InvalidateRect(hwnd, &rectScreen, FALSE);
		return 0;

	case WM_RBUTTONDOWN:
		MessageBox(hwnd, Help, szAppName, MB_OK);
		return 0;

	case WM_MOUSEMOVE:
		yPos = HIWORD(lParam);
		if ((showinfo == FALSE) && (yPos < 25))
		{
			hdc = GetDC(hwnd);
			if (hdcInfo)
				DeleteDC(hdcInfo);
			hdcInfo = CreateCompatibleDC(hdc);
			if (hBitmapInfo)
				DeleteObject(hBitmapInfo);
			hBitmapInfo = CreateCompatibleBitmap(hdc, rectScreen.right, rectScreen.bottom);
			SelectObject(hdcInfo, hBitmapInfo);
			hwndInfo = CreateWindow("STATIC", NULL,
				WS_CHILD | WS_VISIBLE,
				rectScreen.left, 0, rectScreen.right, 25,
				hwnd, (HMENU)90, hInstance, NULL);
			if (fillscreen)
				pointer = FullPhoto;
			else
				pointer = FillScreen;
			hwndFillScreen = CreateWindow("BUTTON", pointer,
				WS_CHILD | WS_VISIBLE,
				rectScreen.right-390, 3, 80, 20,
				hwndInfo, (HMENU)92, hInstance, NULL);
			hwndExif = CreateWindow("BUTTON", "Options",
				WS_CHILD | WS_VISIBLE,
				rectScreen.right-310, 3, 70, 20,
				hwndInfo, (HMENU)92, hInstance, NULL);
			hwndRedEye = CreateWindow("BUTTON", "RedEye",
				WS_CHILD | WS_VISIBLE,
				rectScreen.right-240, 3, 70, 20,
				hwndInfo, (HMENU)92, hInstance, NULL);
			hwndRotate = CreateWindow("BUTTON", "Rotate",
				WS_CHILD | WS_VISIBLE,
				rectScreen.right-170, 3, 70, 20,
				hwndInfo, (HMENU)92, hInstance, NULL);
			hwndHelp = CreateWindow("BUTTON", "Help",
				WS_CHILD | WS_VISIBLE,
				rectScreen.right-100, 3, 50, 20,
				hwndInfo, (HMENU)92, hInstance, NULL);
			hwndExit = CreateWindow("BUTTON", "Exit",
				WS_CHILD | WS_VISIBLE,
				rectScreen.right-50, 3, 50, 20,
				hwndInfo, (HMENU)92, hInstance, NULL);
			pInstrProc = (WNDPROC)SetWindowLong(hwndInfo, GWL_WNDPROC, (LONG)InstrProc);
//			BitBlt(hdcInfo, 0, 0, rectScreen.right, 25, hdc, 0, 0, SRCCOPY);
			ReleaseDC(hwnd, hdc);
			showinfo = TRUE;
			InvalidateRect(hwnd, &rectScreen, FALSE);
			SetFocus(hwnd);
		}
		else if ((showinfo) && (yPos >= 25))
		{
			showinfo = FALSE;
			DestroyWindow(hwndInfo);
			InvalidateRect(hwnd, &rectScreen, FALSE);
		}
		return 0;

	case WM_CHAR:
		if ((wParam == '/') || (wParam == '?'))
			MessageBox(hwnd, Help, szAppName, MB_OK);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_ESCAPE:
			if ((ViewPhotosFolder[0] != 0) && (GetOpenFileName(&ofn)))
			{
				for (x = 0; Filename[x] != 0; x++)
					;
				for ( ; (x != 0) && (Filename[x] != '\\'); x--)
					;
				Filename[x] = 0;
				SetCurrentDirectory(Filename);
				GetCurrentDirectory(MAX_PATH, CurrentDir);
				Filename[x] = '\\';
				for (x++, y = 0; Filename[x] != 0; x++, y++)
					ShortFilename[y] = Filename[x];
				ShortFilename[y] = 0;

				for (y = 0; y < MAXFILES; y++)
				{//initialize
					for (x = 0; Files[y][x] != 0; x++)
						Files[y][x] = 0;
					Name[y] = 0;
				}
				num = 0;
				hFindFile = FindFirstFile(jpg, &fd);
				if (hFindFile != INVALID_HANDLE_VALUE)
				{
					GetFile();
					while ((FindNextFile(hFindFile, &fd)) && (num < MAXFILES))
						GetFile();
					FindClose(hFindFile);
					for (x = 0; x < num; x++)
						if (Files[x][0] == '\27')
							Files[x][0] = '[';
					if (num == MAXFILES)
						MessageBox(hwnd, "There are over 2,000 files in this directory!", ERROR, MB_OK);
				}
				hFindFile = FindFirstFile(bmp, &fd);
				if (hFindFile != INVALID_HANDLE_VALUE)
				{
					GetFile();
					while ((FindNextFile(hFindFile, &fd)) && (num < MAXFILES))
						GetFile();
					FindClose(hFindFile);
					for (x = 0; x < num; x++)
						if (Files[x][0] == '\27')
							Files[x][0] = '[';
					if (num == MAXFILES)
						MessageBox(hwnd, "There are over 2,000 files in this directory!", ERROR, MB_OK);
				}
				first = TRUE;
				rotate = FALSE;
				InvalidateRect(hwnd, &rectScreen, FALSE);
			}
			else
				DestroyWindow(hwnd);
			break;
		case VK_F1:
			MessageBox(hwnd, Help, szAppName, MB_OK);
			break;
		case VK_HOME:
			CurrentPhoto = 0;
			first = TRUE;
			rotate = FALSE;
			InvalidateRect(hwnd, &rectScreen, FALSE);
			break;
		case VK_DOWN:
			if ((fillscreen) && (GetKeyState(VK_CONTROL) & 0x8000))
			{
				if (!nodown) {
					if (UpDown == 0)
						nodown = noup = 0;
					UpDown -= 20;
					first = TRUE;
					rotate = FALSE;
					InvalidateRect(hwnd, &rectScreen, FALSE);
				}
				break;
			}
		case VK_RIGHT:
			if ((fillscreen) && (GetKeyState(VK_CONTROL) & 0x8000))
			{
				if (!noright) {
					if (LeftRight == 0)
						noleft = noright = 0;
					LeftRight -= 20;
					first = TRUE;
					rotate = FALSE;
					InvalidateRect(hwnd, &rectScreen, FALSE);
				}
				break;
			}
		case VK_SPACE:
		case VK_NEXT:
		case VK_RETURN:
//			UpDown = LeftRight = nodown = noup = noleft = noright = 0;
			if (CurrentPhoto < (num-1))
			{
				CurrentPhoto++;
				first = TRUE;
				rotate = FALSE;
				InvalidateRect(hwnd, &rectScreen, FALSE);
			}
			break;
		case VK_UP:
			if ((fillscreen) && (GetKeyState(VK_CONTROL) & 0x8000))
			{
				if (!noup) {
					if (UpDown == 0)
						nodown = noup = 0;
					UpDown += 20;
					first = TRUE;
					rotate = FALSE;
					InvalidateRect(hwnd, &rectScreen, FALSE);
				}
				break;
			}
		case VK_LEFT:
			if ((fillscreen) && (GetKeyState(VK_CONTROL) & 0x8000))
			{
				if (!noleft) {
					if (LeftRight == 0)
						noleft = noright = 0;
					LeftRight += 20;
					first = TRUE;
					rotate = FALSE;
					InvalidateRect(hwnd, &rectScreen, FALSE);
				}
				break;
			}
		case VK_BACK:
		case VK_PRIOR:
//			UpDown = LeftRight = nodown = noup = noleft = noright = 0;
			if (CurrentPhoto)
			{
				CurrentPhoto--;
				first = TRUE;
				rotate = FALSE;
				InvalidateRect(hwnd, &rectScreen, FALSE);
			}
			break;
		case VK_END:
			CurrentPhoto = num-1;
			first = TRUE;
			rotate = FALSE;
			InvalidateRect(hwnd, &rectScreen, FALSE);
			break;
		case VK_DELETE:
			strcpy(PhotoBuffer, CurrentDir);
			strcat(PhotoBuffer, "\\");
			strcat(PhotoBuffer, Files[Name[CurrentPhoto]]);
			shfo.hwnd = hwnd;
			shfo.wFunc = FO_DELETE;
			shfo.pFrom = PhotoBuffer;
			shfo.fFlags = FOF_ALLOWUNDO;
			shfo.fAnyOperationsAborted = FALSE;
			if (0 == SHFileOperation(&shfo))
			{
				if (FALSE == shfo.fAnyOperationsAborted)
				{
					for (x = CurrentPhoto; x < num; x++)
					{
						for (y = 0; Files[x+1][y] != 0; y++)
							Files[x][y] = Files[x+1][y];
						Files[x][y] = 0;
					}
					if (num)
					{
						if  (CurrentPhoto == (num-1))
							CurrentPhoto--;
						num--;
					}
					first = TRUE;
					Sleep(1000);//for the Delete message to disappear
					InvalidateRect(hwnd, &rectScreen, FALSE);
				}
			}
			break;
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (first)
		{
			first = FALSE;
			UpDown = LeftRight = nodown = noup = noleft = noright = 0;
//			if (!UpDown)
//			{
				if (Buf != NULL)
					free(Buf);
				if (Pixels != NULL)
					VirtualFree(Pixels, 0, MEM_RELEASE);
				Pixels = Buf = NULL;
				if (RotatedPixels != NULL)
				{
					VirtualFree(RotatedPixels, 0, MEM_RELEASE);
					RotatedPixels = NULL;
				}
				if (hdcCopy)
					DeleteDC(hdcCopy);
				if (hBitmapCopy)
					DeleteObject(hBitmapCopy);
				if (hdcMem)
					DeleteDC(hdcMem);
				if (hBitmap)
					DeleteObject(hBitmap);

				hFile = CreateFile(Files[CurrentPhoto], GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					fileSize = GetFileSize(hFile, NULL);
					if (fileSize)
					{
						Buf = (BYTE*)malloc(fileSize);
						ReadFile(hFile, Buf, fileSize, &dwBytesRead, NULL);
						if ((*(WORD*)&Buf[0] == 0xD8FF) && (Buf[2] == 0xFF)) // jpeg
						{
							bitmap = FALSE;
							for (x = 0; x < EXIFSIZE; x++)
								ExifInfo[x] = ' ';
							Exif();

							ijlInit(&jcprops);
							jcprops.JPGBytes = Buf;
							jcprops.JPGSizeBytes = fileSize;
							jcprops.JPGFile = NULL;
							jerr = ijlRead(&jcprops, IJL_JBUFF_READPARAMS);
 							Width = jcprops.JPGWidth;
							Height = jcprops.JPGHeight;
							bmi.bmiHeader = *pbmih;
							bmih = *pbmih;
							pbmi->bmiHeader.biWidth = Width;
							pbmi->bmiHeader.biHeight = -(Height);// - for right-side-up picture
							pbmi->bmiHeader.biSizeImage = 0;
							jcprops.jquality = 100;
							jcprops.DIBWidth = Width;
							jcprops.DIBHeight = Height;
							jcprops.DIBPadBytes = IJL_DIB_PAD_BYTES(jcprops.DIBWidth, jcprops.DIBChannels);
							jpgSize = (Width + jcprops.DIBPadBytes) * 3 * Height;
							Pixels = VirtualAlloc(NULL, jpgSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
							if (Pixels)
							{
								jcprops.DIBBytes = Pixels;//destination
								jcprops.DIBChannels = 3;
								jcprops.DIBColor = IJL_BGR;
								jerr = ijlRead(&jcprops, IJL_JBUFF_READWHOLEIMAGE);
							}
							else
								itsfucked = TRUE;
							ijlFree(&jcprops);
							if ((jerr) || (itsfucked))
							{
								MessageBox(hwnd, "was too big\nor it wasn't a JPEG file", Filename, MB_OK);
								DestroyWindow(hwnd);
							}
						}
						else if (*(WORD*)&Buf[0] == 'MB') // 'BM' (bitmap)
						{
							bitmap = TRUE;
							SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
							ReadFile(hFile, &bmfh, sizeof(BITMAPFILEHEADER), &dwBytesRead, NULL);
							BitmapInfoSize = bmfh.bfOffBits - sizeof(BITMAPFILEHEADER);
							ReadFile (hFile, pbmi, BitmapInfoSize, &dwBytesRead, NULL);
							pbmih = &pbmi->bmiHeader;
							bmih = *pbmih;
							BitmapSize = bmfh.bfSize - bmfh.bfOffBits;
							Pixels = VirtualAlloc(NULL, BitmapSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
							ReadFile (hFile, Pixels, BitmapSize, &dwBytesRead, NULL);
							Width = pbmi->bmiHeader.biWidth;
							Height = pbmi->bmiHeader.biHeight;
						}
						else
						{
							MessageBox(hwnd, "That's not a jpeg or bitmap file", ERROR, MB_OK);
							CloseHandle(hFile);
							EndPaint(hwnd, &ps);
							return 0;
						}
					}
					CloseHandle(hFile);
				}

				if (rotate)
				{
					RotatedPixels = (BYTE*)VirtualAlloc(NULL, jpgSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
					RotatedPixels[0] = Pixels[jpgSize-1];
					RotatedPixels[1] = Pixels[jpgSize-2];
					RotatedPixels[2] = Pixels[jpgSize-3];
					z = jpgSize-6;
					for (x = 3, y = z; x < z; x += 3, y -= 3)
						*(DWORD*)&RotatedPixels[x] = *(DWORD*)&Pixels[y];
				}
				hdcCopy = CreateCompatibleDC(hdc);
				hBitmapCopy = CreateCompatibleBitmap(hdc, rectScreen.right, rectScreen.bottom);
				SelectObject(hdcCopy, hBitmapCopy);
				hdcMem = CreateCompatibleDC(hdc);
				hBitmap = CreateCompatibleBitmap(hdc, Width, Height);
				SelectObject(hdcMem, hBitmap);
				if (!rotate)
					SetDIBits(hdcMem, hBitmap, 0, Height, Pixels, pbmi, DIB_RGB_COLORS);
				else
					SetDIBits(hdcMem, hBitmap, 0, Height, RotatedPixels, pbmi, DIB_RGB_COLORS);

				rectJPG.left = rectJPG.top = 0;
				rectJPG.right = Width;
				rectJPG.bottom = Height;
//			} // end of if (!UpDown)

			if ((rectScreen.bottom <= rectJPG.bottom) && (rectScreen.right <= rectJPG.right))
			{//jpeg is bigger than screen
				if (rectScreen.bottom * rectJPG.right <= rectScreen.right * rectJPG.bottom)
				{//a jpeg bigger than the screen that will show space on left & right
					if (!fillscreen) {
						rect.top = rectScreen.top;
						rect.bottom = rectScreen.bottom;
						rect.left = (rectScreen.right - ((rectJPG.right * rectScreen.bottom) / rectJPG.bottom)) / 2;//to center it
						rect.right = (rectJPG.right * rectScreen.bottom) / rectJPG.bottom;//a proportion
						rect.right += rect.left;
					}
					else
					{ // fillscreen
						rect.left = rectScreen.left;
						rect.right = rectScreen.right;
						rect.top = UpDown + (rectScreen.bottom - ((rectJPG.bottom * rectScreen.right) / rectJPG.right)) / 2;//to center it
						rect.bottom = (rectJPG.bottom * rectScreen.right) / rectJPG.right;//a proportion
						rect.bottom += rect.top;
						if (rect.bottom == rectScreen.bottom)
							nodown = TRUE;
						else if (rect.top == rectScreen.top)
							noup = TRUE;
					}
				}
				else
				{//a jpeg bigger than the screen that will show space on top & bottom
					if (!fillscreen) {
						rect.left = rectScreen.left;
						rect.right = rectScreen.right;
						rect.top = (rectScreen.bottom - ((rectJPG.bottom * rectScreen.right) / rectJPG.right)) / 2;//to center it
						rect.bottom = (rectJPG.bottom * rectScreen.right) / rectJPG.right;//a proportion
						rect.bottom += rect.top;
					}
					else
					{ // fillscreen
						rect.top = rectScreen.top;
						rect.bottom = rectScreen.bottom;
						rect.left = LeftRight + (rectScreen.right - ((rectJPG.right * rectScreen.bottom) / rectJPG.bottom)) / 2;//to center it
						rect.right = (rectJPG.right * rectScreen.bottom) / rectJPG.bottom;//a proportion
						rect.right += rect.left;
						if (rect.left > (rectScreen.left-LeftRight))
							noleft = TRUE;
						else if (rect.right < (rectScreen.right-LeftRight))
							noright = TRUE;
					}
				}
			}
			else if ((rectScreen.bottom > rectJPG.bottom) && (rectScreen.right >= rectJPG.right))
			{//jpeg is smaller than screen
				rect.top = (rectScreen.bottom - rectJPG.bottom) / 2;
				rect.bottom = rectScreen.bottom - rect.top;
				rect.left = (rectScreen.right - rectJPG.right) / 2;
				rect.right = rectScreen.right - rect.left;
			}
			else if ((rectScreen.bottom > rectJPG.bottom) && (rectScreen.right < rectJPG.right))
			{//jpeg is wider and shorter
				rect.left = rectScreen.left;
				rect.right = rectScreen.right;
				rect.top = (rectScreen.bottom - ((rectJPG.bottom * rectScreen.right) / rectJPG.right)) / 2;//to center it
				rect.bottom = (rectJPG.bottom * rectScreen.right) / rectJPG.right;//a proportion
				rect.bottom += rect.top;
			}
			else if ((rectScreen.bottom <= rectJPG.bottom) && (rectScreen.right >= rectJPG.right))
			{//jpeg is narrower and taller
				rect.top = rectScreen.top;
				rect.bottom = rectScreen.bottom;
				rect.left = (rectScreen.right - ((rectJPG.right * rectScreen.bottom) / rectJPG.bottom)) / 2;//to center it
				rect.right = (rectJPG.right * rectScreen.bottom) / rectJPG.bottom;//a proportion
				rect.right += rect.left;
			}
			if (rect.left)
			{
				rectLeft.left = rectLeft.top = 0;
				rectLeft.bottom = rectScreen.bottom;
				rectLeft.right = rect.left;
				rectRight.top = rectRight.right = 0;
				rectRight.bottom = rectScreen.bottom;
				rectRight.left = rectScreen.right-rect.left;
				rectRight.right = rectScreen.right;
			}
			if (rect.top)
			{
				rectTop.left = rectTop.top = 0;
				rectTop.bottom = rect.top;
				rectTop.right = rectScreen.right;
				rectBottom.left = 0;
				rectBottom.top = rect.bottom;
				rectBottom.bottom = rectScreen.bottom;
				rectBottom.right = rectScreen.right;
			}
//			SetStretchBltMode(hdc, COLORONCOLOR); // this makes screen-sized photo (NOT GOOD ENOUGH)
			SetStretchBltMode(hdc, HALFTONE); // this makes screen-sized photo
			SetBrushOrgEx(hdc, 0, 0, NULL);
			if (!rotate)
				StretchDIBits(hdc, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 0, 0, Width, Height, Pixels, pbmi, DIB_RGB_COLORS, SRCCOPY);
			else
				StretchDIBits(hdc, rect.left, rect.top, rect.right-rect.left, rect.bottom-rect.top, 0, 0, Width, Height, RotatedPixels, pbmi, DIB_RGB_COLORS, SRCCOPY);
			if (rect.left)
			{
				FillRect(hdc, &rectLeft, hBrush);
				FillRect(hdc, &rectRight, hBrush);
			}
			if (rect.top)
			{
				FillRect(hdc, &rectTop, hBrush);
				FillRect(hdc, &rectBottom, hBrush);
			}
			BitBlt(hdcCopy, 0, 0, rectScreen.right, rectScreen.bottom, hdc, 0, 0, SRCCOPY); // to go back to screen-sized photo
		}//end of if (first)

		if (buttondown)
			BitBlt(hdc, 0, 0, rectScreen.right, rectScreen.bottom, hdcMem, xLoc, yLoc, SRCCOPY);
		else
		{
			BitBlt(hdc, 0, 0, rectScreen.right, rectScreen.bottom, hdcCopy, 0, 0, SRCCOPY); // back to screen-sized photo
			if (showinfo)
			{
				BitBlt(hdc, 0, 0, rectScreen.right, 25, hdcInfo, 0, 0, SRCCOPY);
				sprintf(TopLine, "%s     %s", Files[CurrentPhoto], ExifInfo);
				SetWindowText(hwndInfo, TopLine);
			}
		}
		EndPaint(hwnd, &ps);
if (veryfirst)
{
	veryfirst = FALSE;
	MessageBox(hwnd, "Move the mouse pointer near the top of the photo to show the Menu.", szAppName, MB_OK);
}
		return 0;

	case WM_DESTROY:
		if (Buf != NULL)
			free(Buf);
		if (Pixels != NULL)
			VirtualFree(Pixels, 0, MEM_RELEASE);
		if (RotatedPixels != NULL)
			VirtualFree(RotatedPixels, 0, MEM_RELEASE);
		if (hdcMem)
			DeleteDC(hdcMem);
		if (hBitmap)
			DeleteObject(hBitmap);
		if (hdcCopy)
			DeleteDC(hdcCopy);
		if (hBitmapCopy)
			DeleteObject(hBitmapCopy);
		if (hdcInfo)
			DeleteDC(hdcInfo);
		if (hBitmapInfo)
			DeleteObject(hBitmapInfo);
		if (ViewPhotosFolder[0])
		{
			SetCurrentDirectory(ViewPhotosFolder);
			hFile = CreateFile(Ini, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				WriteFile(hFile, CurrentDir, lstrlen(CurrentDir), &dwBytesWritten, NULL);
				CloseHandle(hFile);
			}

			if (BigExifData[0] == 0)
			{
				for (x = 0; BigExifData[x] != 0; x++)
					;
				BigExifData[x++] = '\\';
				for (y = 0; ExifData[y] != 0; x++, y++)
					BigExifData[x] = ExifData[y];
				BigExifData[x] = 0;
			}
			hFile = CreateFile(BigExifData, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			ExifDta[0] = camera;
			ExifDta[1] = datetime;
			ExifDta[2] = fstop;
			ExifDta[3] = speed;
			ExifDta[4] = iso;
			ExifDta[5] = focalen;
			ExifDta[6] = flash;
			ExifDta[7] = exposureprogram;
			ExifDta[8] = size;
			ExifDta[9] = meteringmode;
			ExifDta[10] = exposuremode;
			ExifDta[11] = whitebalance;
			ExifDta[12] = fillscreen;
			WriteFile(hFile, ExifDta, 13, &dwBytesWritten, NULL);
			CloseHandle(hFile);
		}
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

void GetFile(void)
{
	for (x = 0; fd.cFileName[x] != 0; x++)
		Files[num][x] = fd.cFileName[x];
	Files[num][x] = 0;
	if (0 == stricmp(ShortFilename, Files[num]))
		CurrentPhoto = num;
	for (y = 0; y < num; y++)
	{//insertion sort
		if (stricmp(Files[num], Files[Name[y]]) == -1)
		{// Files[num] < Files[Name[y]]
			for (x = num; x >= y; x--)
				Name[x+1] = Name[x]; 
			Name[y] = num;//insert it before a "bigger" filename
			break;
		}
	}
	Name[y] = num;//or put it at the end
	num++;
}

void Exif(void)
{
	int Offset, Integer;
	WORD NumOfTags, Word;
	DWORD x, y, TiffBegin, TagLoc, NumLoc;
	double Double;

	Offset = 0;
	for (x = 6; (x < 128) && (*(DWORD*)&Buf[x] != 0x66697845); x++)//"Exif"
		;
	if (x != 128)
	{//Exif found
		TiffBegin = x + 6;
		if (*(WORD*)&Buf[TiffBegin] == 0x4949)//"II" (can't do "MM" yet)
		{
			NumLoc = TiffBegin + Buf[TiffBegin+4];
BigLoop:
			TagLoc = NumLoc + 2;
			NumOfTags = *(WORD*)&Buf[NumLoc];
			for (x = 0; x <= NumOfTags; x++, TagLoc += 12)
			{
				IFD[x].Tag = *(WORD*)&Buf[TagLoc];
				IFD[x].Type = *(WORD*)&Buf[TagLoc+2];
				//Type 1 = BYTE
				//Type 2 = ASCII
				//Type 3 = SHORT (WORD)
				//Type 4 = LONG (DWORD)
				//Type 5 = RATIONAL (first DWORD / second DWORD)
				IFD[x].Count = *(int*)&Buf[TagLoc+4];
				IFD[x].Offset = *(int*)&Buf[TagLoc+8];

				switch (IFD[x].Tag)
				{
				case 272:
					if (camera)
					{
						y = TiffBegin + IFD[x].Offset;
						for ( ; Buf[y] != 0; y++)
							ExifInfo[Offset++] = Buf[y];
					}
					break;
				case 306:
					if (datetime)
					{
						y = TiffBegin + IFD[x].Offset;
						Offset += 2;
						ExifInfo[Offset++] = Buf[y+5];//month
						ExifInfo[Offset++] = Buf[y+6];
						ExifInfo[Offset++] = '/';
						ExifInfo[Offset++] = Buf[y+8];//date
						ExifInfo[Offset++] = Buf[y+9];
						ExifInfo[Offset++] = '/';
						ExifInfo[Offset++] = Buf[y+0];//year
						ExifInfo[Offset++] = Buf[y+1];
						ExifInfo[Offset++] = Buf[y+2];
						ExifInfo[Offset++] = Buf[y+3];
						ExifInfo[Offset++] = ' ';
						ExifInfo[Offset++] = Buf[y+11];//hour
						ExifInfo[Offset++] = Buf[y+12];
						ExifInfo[Offset++] = ':';
						ExifInfo[Offset++] = Buf[y+14];//min
						ExifInfo[Offset++] = Buf[y+15];
						ExifInfo[Offset++] = ':';
						ExifInfo[Offset++] = Buf[y+17];//sec
						ExifInfo[Offset++] = Buf[y+18];
					}
					break;
				case 33437:
					if (fstop)
					{
						y = TiffBegin + IFD[x].Offset;
						Double = (double)*(DWORD*)&Buf[y] / (double)*(DWORD*)&Buf[y+4];
						Double *= 10;
						Integer = (int)Double;
						Offset += 2;
						ExifInfo[Offset++] = 'f';
						if (Integer/100)
							ExifInfo[Offset++] = (Integer/100)+'0';
						ExifInfo[Offset++] = ((Integer%100)/10)+'0';
						ExifInfo[Offset++] = '.';
						ExifInfo[Offset++] = (Integer%10)+'0';
					}
					break;
				case 33434:
					if (speed)
					{
						y = TiffBegin + IFD[x].Offset;
						Double = (double)*(DWORD*)&Buf[y] / (double)*(DWORD*)&Buf[y+4];
						Double = 1.0 / Double;
						Integer = (int)Double;
						Offset += 2;
						ExifInfo[Offset++] = '1';
						ExifInfo[Offset++] = '/';
						if (Integer/1000)
							ExifInfo[Offset++] = (Integer/1000)+'0';
						if (Integer/100)
							ExifInfo[Offset++] = ((Integer%1000)/100)+'0';
						ExifInfo[Offset++] = ((Integer%100)/10)+'0';
						ExifInfo[Offset++] = (Integer%10)+'0';
					}
					break;
				case 34855:
					if (iso)
					{
						Word = *(WORD*)&Buf[TagLoc+8];
						Offset += 2;
						ExifInfo[Offset++] = 'I';
						ExifInfo[Offset++] = 'S';
						ExifInfo[Offset++] = 'O';
						if (Word/1000)
							ExifInfo[Offset++] = (Word/1000)+'0';
						ExifInfo[Offset++] = ((Word%1000)/100)+'0';
						ExifInfo[Offset++] = ((Word%100)/10)+'0';
						ExifInfo[Offset++] = (Word%10)+'0';
					}
					break;
				case 37386:
					if (focalen)
					{
						y = TiffBegin + IFD[x].Offset;
						Double = (double)*(DWORD*)&Buf[y] / (double)*(DWORD*)&Buf[y+4];
						Double *= 10;
						Integer = (int)Double;
						Offset += 2;
						ExifInfo[Offset++] = 'F';
						ExifInfo[Offset++] = 'o';
						ExifInfo[Offset++] = 'c';
						ExifInfo[Offset++] = 'a';
						ExifInfo[Offset++] = 'l';
						ExifInfo[Offset++] = ' ';
						ExifInfo[Offset++] = 'L';
						ExifInfo[Offset++] = 'e';
						ExifInfo[Offset++] = 'n';
						ExifInfo[Offset++] = 'g';
						ExifInfo[Offset++] = 't';
						ExifInfo[Offset++] = 'h';
						ExifInfo[Offset++] = ' ';
						if (Integer/1000)
							ExifInfo[Offset++] = (Integer/1000)+'0';
						ExifInfo[Offset++] = ((Integer%1000)/100)+'0';
						ExifInfo[Offset++] = ((Integer%100)/10)+'0';
						if (Integer%10)
						{
							ExifInfo[Offset++] = '.';
							ExifInfo[Offset++] = (Integer%10)+'0';
						}
					}
					break;
				case 37385:
					if (flash)
					{
						Word = *(WORD*)&Buf[TagLoc+8];
						if (Word & 1)
						{
							Offset += 2;
							ExifInfo[Offset++] = 'F';
							ExifInfo[Offset++] = 'l';
							ExifInfo[Offset++] = 'a';
							ExifInfo[Offset++] = 's';
							ExifInfo[Offset++] = 'h';
						}
					}
					break;
				case 34850:
					if (exposureprogram)
					{
						Word = *(WORD*)&Buf[TagLoc+8];
						Offset += 2;
						if (Word == 1)
							for (x = 0; Manual[x] != 0; x++)
								ExifInfo[Offset++] = Manual[x];
						else if (Word == 3)
							for (x = 0; Aperture[x] != 0; x++)
								ExifInfo[Offset++] = Aperture[x];
						else if (Word == 4)
							for (x = 0; Speed[x] != 0; x++)
								ExifInfo[Offset++] = Speed[x];
						else if (Word == 5)
							for (x = 0; Creative[x] != 0; x++)
								ExifInfo[Offset++] = Creative[x];
						else if (Word == 6)
							for (x = 0; Action[x] != 0; x++)
								ExifInfo[Offset++] = Action[x];
						else if (Word == 7)
							for (x = 0; Portrait[x] != 0; x++)
								ExifInfo[Offset++] = Portrait[x];
						else if (Word == 8)
							for (x = 0; Landscape[x] != 0; x++)
								ExifInfo[Offset++] = Landscape[x];
						if ((Word == 1) || (Word == 5) || (Word == 6) || (Word == 7) || (Word == 8))
							for (x = 0; ExposureProg[x] != 0; x++)
								ExifInfo[Offset++] = ExposureProg[x];
					}				
					break;
				case 40962:
					if (size)
					{
						y = *(WORD*)&Buf[TagLoc+8];
						Offset += 2;
						if (y / 1000)
							ExifInfo[Offset++] = (char)(y/1000) + '0';
						ExifInfo[Offset++] = (char)((y%1000)/100) + '0';
						ExifInfo[Offset++] = (char)((y%100)/10) + '0';
						ExifInfo[Offset++] = (char)(y%10) + '0';
						ExifInfo[Offset++] = 'x';
					}
					break;
				case 40963:
					if (size)
					{
						y = *(WORD*)&Buf[TagLoc+8];
						if (y / 1000)
							ExifInfo[Offset++] = (char)(y/1000) + '0';
						ExifInfo[Offset++] = (char)((y%1000)/100) + '0';
						ExifInfo[Offset++] = (char)((y%100)/10) + '0';
						ExifInfo[Offset++] = (char)(y%10) + '0';
					}
					break;
				case 37383:
					if (meteringmode)
					{
						Word = *(WORD*)&Buf[TagLoc+8];
						Offset += 2;
						if (Word == 1)
							for (x = 0; Average[x] != 0; x++)
								ExifInfo[Offset++] = Average[x];
						if (Word == 2)
							for (x = 0; CenterWeighted[x] != 0; x++)
								ExifInfo[Offset++] = CenterWeighted[x];
						if (Word == 3)
							for (x = 0; Spot[x] != 0; x++)
								ExifInfo[Offset++] = Spot[x];
						if (Word == 4)
							for (x = 0; MultiSpot[x] != 0; x++)
								ExifInfo[Offset++] = MultiSpot[x];
						if (Word == 5)
							for (x = 0; Pattern[x] != 0; x++)
								ExifInfo[Offset++] = Pattern[x];
						if (Word == 6)
							for (x = 0; Partial[x] != 0; x++)
								ExifInfo[Offset++] = Partial[x];
						for (x = 0; MeteringMode[x] != 0; x++)
							ExifInfo[Offset++] = MeteringMode[x];
					}
					break;
				case 41986:
					if (exposuremode)
					{
						Word = *(WORD*)&Buf[TagLoc+8];
						Offset += 2;
						if (Word == 0)
							for (x = 0; AutoExposure[x] != 0; x++)
								ExifInfo[Offset++] = AutoExposure[x];
						else if (Word == 1)
							for (x = 0; ManualExposure[x] != 0; x++)
								ExifInfo[Offset++] = ManualExposure[x];
						else if (Word == 2)
							for (x = 0; AutoBracket[x] != 0; x++)
								ExifInfo[Offset++] = AutoBracket[x];
					}
					break;
				case 41987:
					if (whitebalance)
					{
						Word = *(WORD*)&Buf[TagLoc+8];
						Offset += 2;
						if (Word == 0)
							for (x = 0; AutoWB[x] != 0; x++)
								ExifInfo[Offset++] = AutoWB[x];
						else
							for (x = 0; ManualWB[x] != 0; x++)
								ExifInfo[Offset++] = ManualWB[x];
					}
					break;
				case 34665://next IFD offset
					NumLoc = TiffBegin + IFD[x].Offset;
					goto BigLoop;
				}
				if (Offset > 219)//OFFSETSIZE - 30 (30 being supposedly the longest ExifInfo)
					break;
			}//end of for (x = 0; x <= NumOfTags; x++, TagLoc += 12)
		}
	}
	ExifInfo[Offset] = 0;
}

int CALLBACK ExifProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		if (camera)
			CheckDlgButton (hwndDlg, IDC_CHECK1, BST_CHECKED);
		if (datetime)
			CheckDlgButton (hwndDlg, IDC_CHECK2, BST_CHECKED);
		if (fstop)
			CheckDlgButton (hwndDlg, IDC_CHECK3, BST_CHECKED);
		if (speed)
			CheckDlgButton (hwndDlg, IDC_CHECK4, BST_CHECKED);
		if (iso)
			CheckDlgButton (hwndDlg, IDC_CHECK5, BST_CHECKED);
		if (focalen)
			CheckDlgButton (hwndDlg, IDC_CHECK6, BST_CHECKED);
		if (flash)
			CheckDlgButton (hwndDlg, IDC_CHECK7, BST_CHECKED);
		if (exposureprogram)
			CheckDlgButton (hwndDlg, IDC_CHECK8, BST_CHECKED);
		if (size)
			CheckDlgButton (hwndDlg, IDC_CHECK9, BST_CHECKED);
		if (meteringmode)
			CheckDlgButton (hwndDlg, IDC_CHECK10, BST_CHECKED);
		if (exposuremode)
			CheckDlgButton (hwndDlg, IDC_CHECK11, BST_CHECKED);
		if (whitebalance)
			CheckDlgButton (hwndDlg, IDC_CHECK12, BST_CHECKED);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			camera = datetime = fstop = speed = iso = focalen = flash = exposureprogram = size = meteringmode = exposuremode = whitebalance = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK1))
				camera = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK2))
				datetime = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK3))
				fstop = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK4))
				speed = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK5))
				iso = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK6))
				focalen = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK7))
				flash = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK8))
				exposureprogram = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK9))
				size = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK10))
				meteringmode = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK11))
				exposuremode = TRUE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK12))
				whitebalance = TRUE;
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}
//char NewFileName[] = "00-00-0000-00.0.jpg";//month-date-hourminute-second
//		for (ch = '1'; 0 == MoveFile(fd.cFileName, NewFileName); ch++)
//			NewFileName[14] = ch;//for photos taken in the same second
