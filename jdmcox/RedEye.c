#include <windows.h>
#include "ijl.h"//add ijl15.lib to Project -Settings -Link
#include "resource.h"
#define ERROR_MSG_SIZE 200
#define READ_FILE WM_USER
#define OK 1

char About[] = "Jan 19, 2006\nDoug Cox\nhttp:\\\\jdmcox.com\\\njdmcox@jdmcox.com";
char szAppName[] = "RedEye 1.8";
char RedEyeIni[] = "RedEye.ini";
char JFile[MAX_PATH];// = "10-12-1112-20 F2.7 060 50 100.jpg";
char Help[] = "\
To scroll the photo:\n\
Hold the Left Button down\n\
while moving the Mouse,\n\
or use the arrow keys,\n\
or use the mousewheel\n\
to scroll up/down,\n\
or use the mousewheel\n\
with the Ctrl key down\n\
to scroll left/right.\n\
\n\
To reduce the red:\n\
Move the Mouse pointer\n\
to a corner of a red eye,\n\
and press and hold the\n\
Right Mouse Button and\n\
move the Mouse pointer\n\
to the diagonal corner\n\
of the red eye and\n\
release the Button.\n\
\n\
_____________________\n\
\n\
If you right-click on a photo\n\
file in Windows Explorer\n\
and select Send To, RedEye\n\
should be an option to use\n\
to open that photo.";

char Advanced[] = "Color is made up of red, green, and blue values.\n\
In a photo, each of these 3 components can have a value between 0 and 255.\n\n\
RedEye looks at the colors in the selected rectangle, and:\n\
if (red > (green + 48)) and (red > (blue + 48))\n\
     then if (green >= 128) and (blue >= 128))\n\
          then green = red and blue = red (convert a bright reddish color to white)\n\
     else if ( green < 128) or (blue < 128) (reduce the red component)\n\
          if (green < blue) then red = green + 30\n\
          else red = blue + 30\n\n\
In the second case, a dialog box will pop up, allowing you to change\n\
that value of 30 to something between 0 and 50";

int x, y, cxScreen, cyScreen, Response, xTopLeft, yTopLeft, xBottomRight, yBottomRight, xLoc, yLoc, xPos, yPos, Quality = 90, PrevQuality;
char CmdLine;
char Directory[MAX_PATH];
DWORD fileSize, OutFileSize;
int ExifSize;
BYTE *Exif;
BOOL sendtoflg = TRUE, secondtimethru, writefile, fromclose = FALSE;

struct NewColors{
	int x;
	int y;
	COLORREF color;
} newColors, *pNewColors, *pNC;

HWND hwnd;
HDC hdc, hdcMem, hdcUndo;
HINSTANCE hInst;
HMENU hMenu;
OPENFILENAME ofn;
BITMAPINFO bmi, *pbmi = &bmi;
BITMAPINFOHEADER bmih, *pbmih = &bmih;
JPEG_CORE_PROPERTIES jcprops;
IJLERR jerr;
FILETIME ft;
HCURSOR hCursor, hDrawingCursor, hWaitingCursor;

int CALLBACK QualityProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
int CALLBACK ChangeRedProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam);
void RedrawEye(void);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	hInst = hInstance;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_ICON1));
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
	wndclass.lpszMenuName  = "REDEYEMENU";
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
			JFile[x] = szCmdLine[y];
		JFile[x] = 0;
	}

	hwnd = CreateWindow (szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
//		WS_OVERLAPPED|WS_CAPTION|WS_MINIMIZEBOX|WS_SYSMENU,
		CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	iCmdShow = SW_SHOWMAXIMIZED;//Petzold, p.59
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
	HPEN hPen;
	HBRUSH hBrush;
	LOGBRUSH lb;
	HGDIOBJ hObject;
	PAINTSTRUCT ps;
	HANDLE hFile;
	DWORD dwBytesRead, dwBytesWritten;
	static char ErrorMsg[ERROR_MSG_SIZE];
	static RECT rect, rect2;
	static DWORD Attribs;
	static HBITMAP hBitmap, hBitmap2;
	static unsigned char *Buf, *pixel_buf, *pWidth, *pHeight;
	static int Width, Height, padBytes, x1, y1, x2, y2, changes = 0;
	static int xPrevious, yPrevious, Top, Left;
//	static char Red[2], Green[2], Blue[2];	
	static BOOL first = TRUE, buttondown = FALSE, gothdc = FALSE;

	switch (message)
	{
	case WM_SIZE:
		rect2.right = LOWORD(lParam);
		rect2.bottom = HIWORD(lParam);
		if (Height > rect2.bottom)
			Top = Height-rect2.bottom;
		else
			Top = 0;
		if (Width > rect2.right)
 			Left = Width-rect2.right;
		else
			Left = 0;
		break;

	case WM_CREATE:
		Width = 0;
		hCursor = LoadCursor(NULL, IDC_ARROW);
		hDrawingCursor = LoadCursor(NULL, IDC_HAND);
		hWaitingCursor = LoadCursor(NULL, IDC_WAIT);

		Exif = NULL;
		hdcMem = NULL;
		pNC = NULL;
		hMenu = GetMenu(hwnd);
		hFile = CreateFile(RedEyeIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			if (fileSize = GetFileSize(hFile, NULL))
				ReadFile(hFile, Directory, fileSize, &dwBytesRead, NULL);
			else
				GetCurrentDirectory(MAX_PATH, Directory);
			CloseHandle(hFile);
		}
		else
			GetCurrentDirectory(MAX_PATH, Directory);
		ofn.lpstrFile         = JFile;
		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.hwndOwner         = hwnd ;
		ofn.hInstance         = NULL ;
		ofn.lpstrFilter       = "*.jpg\x0*.jpg\x0\x0";
		ofn.lpstrCustomFilter = NULL ;
		ofn.nMaxCustFilter    = 0 ;
		ofn.nFilterIndex      = 0 ;
		ofn.nMaxFile          = MAX_PATH ;
		ofn.lpstrFileTitle    = NULL ;//Set in Open and Close functions
		ofn.nMaxFileTitle     = MAX_PATH ;
		ofn.lpstrInitialDir   = Directory;
		ofn.lpstrTitle        = NULL ;
		ofn.Flags             = OFN_HIDEREADONLY|OFN_CREATEPROMPT|OFN_NOCHANGEDIR;
		ofn.nFileOffset       = 0 ;
		ofn.nFileExtension    = 0 ;
		ofn.lpstrDefExt       = "jpg" ;
		ofn.lCustData         = 0L ;
		ofn.lpfnHook          = NULL ;
		ofn.lpTemplateName    = NULL ;
		ofn.lpstrDefExt       =".jpg";

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
		return 0;

	case WM_COMMAND:
		switch (LOWORD (wParam))
		{
		case ID_FILES_OPEN:
			if (changes != 0)
			{
				if ((MessageBox(hwnd, "Save changes?", "", MB_YESNO)) == IDYES)
					SendMessage(hwnd, WM_COMMAND, ID_SAVE, 0);
			}
			if (Exif != NULL)
			{
				free(Exif);
				Exif = NULL;
			}
			if (sendtoflg == FALSE)
			{
				if (GetOpenFileName(&ofn))
				{
					for (x = ofn.nFileExtension; ofn.lpstrFile[x] != '\\'; x--)
						;
					for (y = 0; y < x; y++)
						Directory[y] = ofn.lpstrFile[y];
					Directory[y++] = '\\';
					Directory[y] = 0;
					hFile = CreateFile(RedEyeIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
					WriteFile(hFile, Directory, strlen(Directory), &dwBytesWritten, NULL);
					CloseHandle(hFile);
					SendMessage(hwnd, READ_FILE, 0, 0);
				}
			}
			else
			{
				sendtoflg = FALSE;
				SendMessage(hwnd, READ_FILE, 0, 0);
			}
			break;

		case ID_SAVE:
			if (Width == 0)
				return 0;
			Buf = NULL;
			pixel_buf = NULL;
			if (GetSaveFileName(&ofn))
			{
				secondtimethru = FALSE;
				writefile = FALSE;
				SetCursor(hWaitingCursor);
				pixel_buf = (char*)VirtualAlloc(NULL, (Width + padBytes) * Height * 3, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
				Buf = (char*)VirtualAlloc(NULL, (Width + padBytes) * Height * 3, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
				GetDIBits(hdcMem, hBitmap, 0, Height, pixel_buf, pbmi, 0);
				Quality = 98;
retry:
				jerr = ijlInit (&jcprops);//use Intel's ijl15.dll to convert JPEG files
				jcprops.DIBWidth = Width;
				jcprops.DIBHeight = Height;
				jcprops.DIBPadBytes = IJL_DIB_PAD_BYTES(Width, 3);
				jcprops.DIBChannels = 3;
				jcprops.DIBColor = IJL_BGR;
				jcprops.DIBBytes = pixel_buf;//input buffer
				jcprops.JPGColor = IJL_YCBCR;
				jcprops.JPGWidth = Width;
				jcprops.JPGHeight = Height;
quality:
				jcprops.jquality = Quality;
				jcprops.JPGSubsampling = IJL_422; // 4:2:2 subsampling.
				jcprops.JPGBytes = Buf;//output buffer
				jcprops.JPGSizeBytes = (Width + padBytes) * Height * 3;//estimate
				jcprops.JPGFile = NULL;
				jerr = ijlWrite(&jcprops,IJL_JBUFF_WRITEWHOLEIMAGE);
				OutFileSize = jcprops.JPGSizeBytes;//re-computed by ijl15.dll
				if ((secondtimethru == FALSE) && (OutFileSize > fileSize))
				{
					Quality--;
					goto quality;
				}
				ijlFree(&jcprops);
				SetCursor(hCursor);

				PrevQuality = Quality;
				if (DialogBox(hInst, "QUALITY", hwnd, QualityProc))
				{
					if (PrevQuality == Quality)
					{
						writefile = TRUE;
					}
					else
					{
						secondtimethru = TRUE;
						goto retry;
					}
				}
			}
			if (writefile)
			{
				hFile = CreateFile(ofn.lpstrFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE)
				{
					if (ExifSize)
					{
						WriteFile(hFile, Exif, ExifSize, &dwBytesWritten, NULL);
						WriteFile(hFile, &Buf[0x44], OutFileSize-0x44, &dwBytesWritten, NULL);
//						free(Exif);
					}
					else
						WriteFile(hFile, Buf, OutFileSize, &dwBytesWritten, NULL);
					SetFileTime(hFile, NULL, NULL, &ft);
				}
				CloseHandle(hFile);
				
				changes = 0;
//				if (fromclose == FALSE)
//					SendMessage(hwnd, WM_CLOSE, 0, 0);
			}
			if (Buf != NULL)
				VirtualFree(Buf, 0, MEM_RELEASE);
			if (pixel_buf != NULL)
				VirtualFree(pixel_buf, 0, MEM_RELEASE);
 			break;

		case ID_FILE_EXITNOSAVE:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		case ID_UNDO:
			if (changes != 0)
			{
				changes--;
				BitBlt (hdcMem, 0, 0, Width, Height, hdcUndo, 0, 0, SRCCOPY);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case IDHELP:
			MessageBox(hwnd, Help, "Using", MB_OK);
			break;

		case ID_ADVANCED:
			MessageBox(hwnd, Advanced, "The Algorithm", MB_OK);
			break;

		case ID_HELPABOUT:
			MessageBox(hwnd, About, szAppName, MB_OK);
			break;
		}
		return 0;

	case READ_FILE:
		SetCursor(hWaitingCursor);
		changes = 0;
		buttondown = FALSE;
		fileSize = 0;
		Attribs = GetFileAttributes(JFile);
		if (Attribs & FILE_ATTRIBUTE_READONLY)//look only at readonly attributes
			SetFileAttributes(JFile, Attribs & (FILE_ATTRIBUTE_READONLY ^ -1));//get rid of readonly
		hFile = CreateFile(JFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
			fileSize = GetFileSize(hFile, NULL);
		else
		{
			FormatMessage(FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), ErrorMsg, ERROR_MSG_SIZE, NULL);
			MessageBox(hwnd, ErrorMsg, NULL, MB_OK);
			return 0;
		}
		if (fileSize == 0)
			MessageBox(hwnd, "File is empty", NULL, MB_OK);
		else
		{
			Buf = (char*)VirtualAlloc(NULL, fileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			ReadFile(hFile, Buf, fileSize, &dwBytesRead, NULL) ;
			GetFileTime(hFile, NULL, NULL, &ft);

			if (*(int*)&Buf[0] == 0xE1FFD8FF)//Exif data exists
			{
				ExifSize = ((Buf[4] & 0xFF) << 8) | Buf[5];
				ExifSize += 4;
				Exif = (BYTE*)malloc(ExifSize);
				for (x = 0; x < ExifSize; x++)
					Exif[x] = Buf[x];
			}
			else
				ExifSize = 0;

			jerr = ijlInit (&jcprops);//use Intel's ijl15.dll to convert JPEG files
			jcprops.JPGBytes = Buf;//source
			jcprops.JPGSizeBytes = fileSize;
			jcprops.JPGFile = NULL;
			jerr = ijlRead(&jcprops, IJL_JBUFF_READPARAMS);
			Width = jcprops.JPGWidth;
			Height = jcprops.JPGHeight;
			jcprops.DIBWidth = Width;
			jcprops.DIBHeight = Height;
			padBytes = IJL_DIB_PAD_BYTES(Width, 3);
			jcprops.DIBPadBytes = padBytes;
			pixel_buf = (char*)VirtualAlloc(NULL, (Width + padBytes) * Height * 3, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			jcprops.DIBBytes = pixel_buf;//destination
			jcprops.DIBChannels = 3;
			jcprops.DIBColor = IJL_BGR;
			jerr = ijlRead (&jcprops, IJL_JBUFF_READWHOLEIMAGE);
			ijlFree(&jcprops);
			VirtualFree(Buf, 0, MEM_RELEASE);

			bmih.biSize = sizeof(BITMAPINFOHEADER);
			bmih.biWidth = Width;
			bmih.biHeight = -(Height);// - for right-side-up picture
			bmih.biPlanes = 1;
			bmih.biBitCount = 24;
			bmih.biCompression = BI_RGB;
			bmih.biSizeImage = 0;
			bmih.biXPelsPerMeter = 0;
			bmih.biYPelsPerMeter = 0;
			bmih.biClrUsed = 0;
			bmih.biClrImportant = 0;
			bmi.bmiHeader = *pbmih;

			if (gothdc == TRUE)
			{
				DeleteDC (hdcMem);
				DeleteObject (hBitmap);
				DeleteDC (hdcUndo);
				DeleteObject (hBitmap2);
			}
			gothdc = TRUE;
			hdc = GetDC(hwnd);
			hdcMem = CreateCompatibleDC (hdc);
			hBitmap = CreateCompatibleBitmap (hdc, Width, Height);
			hdcUndo = CreateCompatibleDC (hdc);
			hBitmap2 = CreateCompatibleBitmap (hdc, Width, Height);
			ReleaseDC(hwnd, hdc);
			SelectObject (hdcMem, hBitmap);
			SelectObject (hdcUndo, hBitmap2);
			SetDIBits(hdcMem, hBitmap, 0, Height, pixel_buf, pbmi, 0);//final destination
			SetDIBits(hdcUndo, hBitmap, 0, Height, pixel_buf, pbmi, 0);//final destination
			VirtualFree(pixel_buf, 0, MEM_RELEASE);
			InvalidateRect(hwnd, &rect, FALSE);

			if (Height > rect2.bottom)
				Top = Height-rect2.bottom;
			else
				Top = 0;
			if (Width > rect2.right)
 				Left = Width-rect2.right;
			else
				Left = 0;
			xLoc = yLoc = 0;
			xPrevious = -1;
		}
		CloseHandle(hFile);
		SetCursor(hCursor);
		return 0;

	case 0x020A://WM_MOUSEWHEEL
		if (LOWORD(wParam) == MK_CONTROL)
		{
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

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_F1:
			SendMessage(hwnd, WM_COMMAND, (WPARAM)IDHELP, 0);
			break;
		case VK_UP:
			if (yLoc >= 50)
				yLoc -= 50;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case VK_DOWN:
			if (yLoc < (Height - rect2.bottom - 50))
				yLoc += 50;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case VK_RIGHT:
			if (xLoc < (Width - rect2.right - 50))
				xLoc += 50;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case VK_LEFT:
			if (xLoc >= 50)
				xLoc -= 50;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'Z':
		case 'z':
			if (GetKeyState(VK_CONTROL) < 0)//if hi bit is set
				SendMessage(hwnd, WM_COMMAND, (WPARAM)ID_UNDO, 0);
			break;
		}
		return 0;

	case WM_NCMOUSEMOVE:
			if (buttondown)//if the mouse goes out of bounds
				SendMessage(hwnd, WM_LBUTTONUP, 0, 0);
			break;

	case WM_MOUSEMOVE:
		if (wParam == MK_LBUTTON)
		{
			SetCursor(hDrawingCursor);
			if (xPrevious != -1)
			{//normal
				xPrevious = xPos;
				yPrevious = yPos;
			}
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			if (xPrevious == -1)
			{//initialize it
				xPrevious = xPos;
				yPrevious = yPos;
			}

			if (Width > rect2.right)
				xLoc += (xPrevious-xPos);
			if (Height > rect2.bottom)
				yLoc += (yPrevious-yPos);
			if (xLoc < 0)
				xLoc = 0;
			if (xLoc > Left)
				xLoc = Left;
			if (yLoc < 0)
				yLoc = 0;
			if (yLoc > Top)
				yLoc = Top;

			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
		}
		else
		{
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
		}

		if (buttondown)
		{
			x = LOWORD(lParam);
			y = HIWORD(lParam);
			hdc = GetDC(hwnd);
			BitBlt (hdc, 0, 0, cxScreen, cyScreen, hdcMem, xLoc, yLoc, SRCCOPY);//clear the previous ellipse outline
			hPen = CreatePen(PS_SOLID, 1, 0xFFFFFF);
			SelectObject(hdc, hPen);
			lb.lbStyle = BS_HOLLOW;
			hBrush = CreateBrushIndirect(&lb);
			hObject = SelectObject(hdc, hBrush);
			Ellipse(hdc, x1, y1, x, y);
			SelectObject(hdc, hObject);
			DeleteObject(hBrush);
			DeleteObject(hPen);
			ReleaseDC(hwnd, hdc);
		}
/*
		else//show Red, Green, & Blue values
		{
			x = LOWORD(lParam);
			y = HIWORD(lParam);
			hdc = GetDC(hwnd);
			color = GetPixel(hdc, x, y);
			ReleaseDC(hwnd, hdc);
			x = ((color >> 20) & 0xF) + '0';
			if (x > '9') x += 7;//for hexadecimal letter
			Blue[0] = x;
			x = ((color >> 16) & 0xF) + '0';
			if (x > '9') x += 7;
			Blue[1] = x;
			x = ((color >> 12) & 0xF) + '0';
			if (x > '9') x += 7;
			Green[0] = x;
			x = ((color >> 8) & 0xF) + '0';
			if (x > '9') x += 7;
			Green[1] = x;
			x = ((color >> 4) & 0xF) + '0';
			if (x > '9') x += 7;
			Red[0] = x;
			x = (color & 0xF) + '0';
			if (x > '9') x += 7;
			Red[1] = x;
			InvalidateRect(hwnd, &rect, FALSE);
		}
*/
		return 0;

	case WM_LBUTTONDOWN:
		SetCursor(hDrawingCursor);
		return 0;

	case WM_LBUTTONUP:
		SetCursor(hCursor);
		return 0;

	case WM_RBUTTONDOWN:
		buttondown = TRUE;
		x1 = LOWORD(lParam);
		y1 = HIWORD(lParam);
		return 0;

	case WM_RBUTTONUP:
		if (buttondown)
		{
			x2 = LOWORD(lParam);
			y2 = HIWORD(lParam);

			if ((abs(x2 - x1) > 3) && abs(y2 - y1) > 3)
			{
				if (y1 < y2)
				{
					yTopLeft = y1;
					yBottomRight = y2;
				}
				else
				{
					yTopLeft = y2;
					yBottomRight = y1;
				}
				if (x1 < x2)
				{
					xTopLeft = x1;
					xBottomRight = x2;
				}
				else
				{
					xTopLeft = x2;
					xBottomRight = x1;
				}
				hdc = GetDC(hwnd);
				BitBlt (hdc, 0, 0, cxScreen, cyScreen, hdcMem, xLoc, yLoc, SRCCOPY);
				BitBlt (hdcUndo, 0, 0, Width, Height, hdcMem, 0, 0, SRCCOPY);//"before"

				RedrawEye();

				if (pNC != NULL)//if red component reduced
				{
					Response = DialogBox(hInst, "CHANGERED", hwnd, ChangeRedProc);
					free(pNC);
					pNC = NULL;
				}
				else
					Response = OK;
				if (Response == OK)
				{
//					GetClientRect(hwnd, &rect2);
					BitBlt (hdcMem, xLoc, yLoc, rect2.right, rect2.bottom, hdc, 0, 0, SRCCOPY);
					changes++;
				}
				else
					BitBlt (hdc, 0, 0, cxScreen, cyScreen, hdcMem, xLoc, yLoc, SRCCOPY);
				ReleaseDC(hwnd, hdc);
			}
			else
			{//if circle is VERY small
				hdc = GetDC(hwnd);
				BitBlt (hdc, 0, 0, cxScreen, cyScreen, hdcMem, xLoc, yLoc, SRCCOPY);
				BitBlt (hdcUndo, 0, 0, Width, Height, hdcMem, 0, 0, SRCCOPY);//"before"
				ReleaseDC(hwnd, hdc);
			}
			buttondown = FALSE;
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint (hwnd, &ps);
		if (hdcMem)
			BitBlt (hdc, 0, 0, cxScreen, cyScreen, hdcMem, xLoc, yLoc, SRCCOPY);
//		TextOut(hdc, 0, 0, Red, 2);
//		TextOut(hdc, 0, 20, Green, 2);
//		TextOut(hdc, 0, 40, Blue, 2);
		if (first)
		{
			first = FALSE;
			FillRect(hdc, &rect, (HBRUSH) (COLOR_WINDOW+1));
			SendMessage(hwnd, WM_COMMAND, ID_FILES_OPEN, 0);
		}
		EndPaint (hwnd, &ps);
		return 0;

	case WM_CLOSE:
		if (changes != 0)
		{
			if ((MessageBox(hwnd, "Save changes?", "", MB_YESNO)) == IDYES)
			{
				fromclose = TRUE;
				SendMessage(hwnd, WM_COMMAND, ID_SAVE, 0);
			}
		}
		if (gothdc == TRUE)
		{
			DeleteDC (hdcMem);
			DeleteObject (hBitmap);
			DeleteDC (hdcUndo);
			DeleteObject (hBitmap2);
		}
		break;//NOT RETURN 0!

	case WM_DESTROY:
		if (Exif != NULL)
			free(Exif);
		PostQuitMessage (0);
		return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

int CALLBACK ChangeRedProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndStatic, hwndScroll;
	RECT dlgRect;
	int dlgWidth, dlgHeight;
	COLORREF color;
	char number[10];
	static int ScrollPosition;
	static short adjustment;

	switch (message)
	{
	case WM_INITDIALOG:
		GetWindowRect(hwndDlg, &dlgRect);
		dlgWidth = dlgRect.right - dlgRect.left;
		dlgHeight = dlgRect.bottom - dlgRect.top;
		MoveWindow(hwndDlg, (cxScreen/2) - (dlgWidth/2), 50, dlgWidth, dlgHeight, TRUE);
		hwndStatic = GetDlgItem(hwndDlg, IDC_XTATIC);
		hwndScroll = GetDlgItem(hwndDlg, IDC_SCROLLBAR1);
		SendMessage(hwndScroll, SBM_SETRANGE, (WPARAM) 0, (LPARAM) 50);
		ScrollPosition = 30;
		SendMessage(hwndScroll, SBM_SETPOS, (WPARAM) ScrollPosition, (LPARAM) TRUE);
		adjustment = 0;
		SetFocus(hwndDlg);
		break;

	case WM_HSCROLL:
		if ((HWND)lParam == hwndScroll)
		{
			switch (LOWORD(wParam))
			{
				case SB_LINELEFT:
					if (ScrollPosition > 0)
						ScrollPosition--;
					break;
				case SB_LINERIGHT: 
					if (ScrollPosition < 50)
						ScrollPosition++;
					break;
				case SB_PAGELEFT:
					if (ScrollPosition >= 10)
						ScrollPosition -= 10;
					break;
				case SB_PAGERIGHT:
					if (ScrollPosition <= 40)
						ScrollPosition += 10;
					break;
				case SB_THUMBPOSITION:
					ScrollPosition = HIWORD(wParam);
					break;
			}
			adjustment = ScrollPosition - 30;
			SendMessage(hwndScroll, SBM_SETPOS, ScrollPosition, TRUE);
			_itoa(ScrollPosition, number, 10);
			SendMessage(hwndStatic, WM_SETTEXT, 0, (LPARAM)number);
			for (pNewColors = pNC; pNewColors->x != 0; pNewColors++)
			{
				color = pNewColors->color;
				color += adjustment;//color + adjustment won't rollover into green color
				SetPixel(hdc, pNewColors->x, pNewColors->y, color);
			}
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog (hwndDlg, OK);
			return OK;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

void RedrawEye(void)
{
  	int w, x, y, z, red, green, blue;
	DWORD v, RegionSize, NumofRects;
	COLORREF color;
	HRGN hRegion;
	RGNDATA *rgnData;

	hRegion = CreateEllipticRgn(xTopLeft, yTopLeft, xBottomRight, yBottomRight);
	RegionSize = GetRegionData(hRegion, 0, NULL);
	rgnData = HeapAlloc(GetProcessHeap(), 0, RegionSize);
	GetRegionData(hRegion, RegionSize, rgnData);
	NumofRects = rgnData->rdh.nCount;
	z = 0;
	w = 0;
	for (v = 0; v < NumofRects; v++)
	{
		xTopLeft = (rgnData->Buffer[w] & 0xFF) | ((rgnData->Buffer[w+1] & 0xFF) << 8) | ((rgnData->Buffer[w+2] & 0xFF) << 16) | ((rgnData->Buffer[w+3] & 0xFF) << 24);
		w += 4;
		yTopLeft = (rgnData->Buffer[w] & 0xFF) | ((rgnData->Buffer[w+1] & 0xFF) << 8) | ((rgnData->Buffer[w+2] & 0xFF) << 16) | ((rgnData->Buffer[w+3] & 0xFF) << 24);
		w += 4;
		xBottomRight = (rgnData->Buffer[w] & 0xFF) | ((rgnData->Buffer[w+1] & 0xFF) << 8) | ((rgnData->Buffer[w+2] & 0xFF) << 16) | ((rgnData->Buffer[w+3] & 0xFF) << 24);
		w += 4;
		yBottomRight = (rgnData->Buffer[w] & 0xFF) | ((rgnData->Buffer[w+1] & 0xFF) << 8) | ((rgnData->Buffer[w+2] & 0xFF) << 16) | ((rgnData->Buffer[w+3] & 0xFF) << 24);
		w += 4;

		for (y = yTopLeft; y < yBottomRight; y++)
		{
			for (x = xTopLeft; x < xBottomRight; x++)
			{
				color = GetPixel(hdc, x, y);
				red = color & 0xFF;
				green = (color >> 8) & 0xFF;
				blue = (color >> 16) & 0xFF;
				if ((red > (green + 0x30)) && (red > (blue + 0x30)))
					if ((green < 0x80) || (blue < 0x80))
						z++;//get size of pNewColors
			}
		}
	}
	if (z)
	{
		pNewColors = (struct NewColors*) malloc((z+1) * sizeof(struct NewColors));//+1 for 0 at end
		pNC = pNewColors;//for free(pNC), etc

		w = 0;
		for (v = 0; v < NumofRects; v++)
		{
			xTopLeft = (rgnData->Buffer[w] & 0xFF) | ((rgnData->Buffer[w+1] & 0xFF) << 8) | ((rgnData->Buffer[w+2] & 0xFF) << 16) | ((rgnData->Buffer[w+3] & 0xFF) << 24);
			w += 4;
			yTopLeft = (rgnData->Buffer[w] & 0xFF) | ((rgnData->Buffer[w+1] & 0xFF) << 8) | ((rgnData->Buffer[w+2] & 0xFF) << 16) | ((rgnData->Buffer[w+3] & 0xFF) << 24);
			w += 4;
			xBottomRight = (rgnData->Buffer[w] & 0xFF) | ((rgnData->Buffer[w+1] & 0xFF) << 8) | ((rgnData->Buffer[w+2] & 0xFF) << 16) | ((rgnData->Buffer[w+3] & 0xFF) << 24);
			w += 4;
			yBottomRight = (rgnData->Buffer[w] & 0xFF) | ((rgnData->Buffer[w+1] & 0xFF) << 8) | ((rgnData->Buffer[w+2] & 0xFF) << 16) | ((rgnData->Buffer[w+3] & 0xFF) << 24);
			w += 4;

			for (y = yTopLeft; y < yBottomRight; y++)
			{
				for (x = xTopLeft; x < xBottomRight; x++)
				{
					color = GetPixel(hdc, x, y);
					red = color & 0xFF;
					green = (color >> 8) & 0xFF;
					blue = (color >> 16) & 0xFF;
					if ((red > (green + 0x30)) && (red > (blue + 0x30)))
					{
						if ((green >= 0x80) && (blue >= 0x80))
						{
							green = red;
							blue  = red;
							color = red + (green << 8) + (blue << 16);
							SetPixel(hdc, x, y, color);
						}
						else //if ((green < 0x80) || (blue < 0x80))
						{
							if (green < blue)
								red = green + 30;
							else
								red = blue + 30;
							color = red + (green << 8) + (blue << 16);
							SetPixel(hdc, x, y, color);
							pNewColors->x = x;
							pNewColors->y = y;

							pNewColors->color = color;
							pNewColors++;
						}
					}
				}
			}
		}
		pNewColors->x = 0;
	}
	HeapFree(GetProcessHeap(), 0, rgnData);
}

/*
	int ExifTagsOffset, IFDOffset, NumOfDirectories, NumOfExifDirectories, IFDValueOffset[20], ExifValueOffset[50];
	WORD IFDTag[20], ExifTag[50];

	if ((Buf[6] == 'E') && (Buf[7] == 'x'))//Exif format JPEG
	{
		IFDOffset = (*(int*)&Buf[0x10] + 0x0C);
		NumOfDirectories = *(WORD*)&Buf[IFDOffset];
		for (x = 0; x < NumOfDirectories; x++)
		{
			IFDTag[x] = *(WORD*)&Buf[IFDOffset + 2 + (x * 12)];
			if (IFDTag[x] == 0x8769)
			{
				IFDValueOffset[x] = *(int*)&Buf[IFDOffset + 10 + (x * 12)];
				ExifTagsOffset = IFDValueOffset[x] + 0x0C;
				NumOfExifDirectories = *(WORD*)&Buf[ExifTagsOffset];
				for (x = 0; x < NumOfExifDirectories; x++)
				{
					ExifTag[x] = *(WORD*)&Buf[ExifTagsOffset + 2 + (x * 12)];
					ExifValueOffset[x] = *(int*)&Buf[ExifTagsOffset + 10 + (x * 12)];
					if (ExifTag[x] == 0xA002)
						Width = ExifValueOffset[x];
					if (ExifTag[x] == 0xA003)
						Height = ExifValueOffset[x];
				}
				break;
			}
		}
	}
*/

int CALLBACK QualityProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char asdf[10];
	static HWND hwndQuality, hwndInputSize, hwndOutputSize, hwndNote;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndQuality = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndInputSize = GetDlgItem(hwndDlg, IDC_EDIT2);
		hwndOutputSize = GetDlgItem(hwndDlg, IDC_EDIT3);
		hwndNote = GetDlgItem(hwndDlg, IDC_EDIT4);
		_itoa(Quality, asdf, 10);
		SetWindowText(hwndQuality, asdf);
		_itoa(fileSize, asdf, 10);
		SetWindowText(hwndInputSize, asdf);
		_itoa(OutFileSize, asdf, 10);
		SetWindowText(hwndOutputSize, asdf);
		if (secondtimethru)
			SetWindowText(hwndNote, "(new Output.jpg filesize)");
		SetFocus(hwndQuality);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK://Continue
			GetWindowText(hwndQuality, asdf, 10);
			Quality = atoi(asdf);
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL://Retry with new JPEG quality
			GetWindowText(hwndQuality, asdf, 10);
			Quality = atoi(asdf);
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}
