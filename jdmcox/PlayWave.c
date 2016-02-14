#include <windows.h>
#include <mmsystem.h>// add winmm.lib to Project -Settings -Link
#include "resource.h"
#define WHITE 0xFCFCFC
#define TIMER_RESOLUTION 1
#define WM_USER2 WM_USER+1
#define STOPPED 0
#define PLAYING 1
#define PAUSED 2

int x, y, z, PTR, BufSize, divisor, zoom, middle, topmiddle, bottommiddle, PointsPerScreen, MillisecondsPerScreen;
int playing, lastPTR, xLoc, yLoc, linesperscreen = 1, rectrightx2, ratio = 16;
unsigned int uTimerID = 0;
DWORD fileSize, fileSize2, dwBytesRead, dwBytesWritten, subchunksize, nextchunk, nSamplesPerSec, Milliseconds, Seconds, Minutes;
DWORD X;
WORD nChannels, wBitsPerSample;
SHORT Short;
double pointsPerScreen, pointsPerMillisecond, millisecondsPerScreen, totalMilliseconds, d;
char Filename[MAX_PATH];
char FullFilename[MAX_PATH];
char **FilePart;
char CmdLine;
char szAppName[] = "PlayWave";
BYTE *WaveBuf, *WaveBuf2;
BYTE *Buf;
BOOL first = TRUE, nosendtoflg = FALSE, oddeven = 1;
char WaveHeight[] = "Vertical Scale = 1:1";
char Left[] = "1";
char Right[] = "1";
char time[] = "00:00:000";
char temp[10];
char Help[] = "\
The middle line shows what's being heard.\n\
The top line is the preceding waveform.\n\
The bottom line is the next waveform.\n\
\n\
SpaceBar to Play or Pause\n\
PgDown for the next screen's waveform\n\
PgUp for the previous screen's waveform\n\
Home or End\n\
Esc to Stop Playing or to Exit if not Playing\n\
\n\
If the WAVE file is stereo, instead of lines,\n\
red and green dots will show for the two channels.\n\
\n\
Doug Cox\n\
1 Nov 2010\n\
http://jdmcox.com\n\
jdmcox@jdmcox.com";

HWND hwnd;
HANDLE hFile;
HINSTANCE hInst;
HMENU hMenu;
RECT rect;
HBRUSH hBrush;
HPEN hPen, hRedPen, hGreenPen;
HDC hdc;
PAINTSTRUCT ps;
OPENFILENAME ofn;
HWAVEOUT hWaveOut;
WAVEHDR WaveOutHdr;
WAVEFORMATEX WaveFormat;
WIN32_FIND_DATA fd;

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	hInst = hInstance;
	hBrush = CreateSolidBrush(WHITE);
	hPen = CreatePen(PS_SOLID, 0, 0);
	hRedPen = CreatePen(PS_SOLID, 0, 0x0000F0);
	hGreenPen = CreatePen(PS_SOLID, 0, 0x00F000);

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = hBrush; // (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName  = "MENU";
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	if (szCmdLine[0] == 0)
		nosendtoflg = TRUE;
	else {
		y = 0;
		CmdLine = '\x0';
		if (szCmdLine[0] == '"') {
			CmdLine = '"';
			y = 1;
		}
		for (x = 0; szCmdLine[y] != CmdLine ; x++, y++)
			Filename[x] = szCmdLine[y];
		Filename[x] = 0;
		GetFullPathName(Filename, MAX_PATH, FullFilename, FilePart);
	}

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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

void GetTime(void)
{
	_asm fld totalMilliseconds
	_asm fistp Milliseconds
	Minutes = Milliseconds / 60000;
	Seconds = (Milliseconds % 60000) / 1000;
	time[0] = (char)(Minutes / 10) + '0';
	time[1] = (char)(Minutes % 10) + '0';
	time[3] = (char)(Seconds / 10) + '0';
	time[4] = (char)(Seconds % 10) + '0';
	time[6] = (char)((Milliseconds % 1000) / 100) + '0';
	time[7] = (char)((Milliseconds % 100) / 10) + '0';
	time[8] = (char)(Milliseconds % 10) + '0';
}

void StopPlaying(void)
{
	waveOutUnprepareHeader(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
	waveOutReset(hWaveOut);
	waveOutClose(hWaveOut);
	if (uTimerID) {
		timeKillEvent(uTimerID);
		uTimerID = 0;
		timeEndPeriod(TIMER_RESOLUTION);
	}
	playing = STOPPED;
}

void ShowRatios(void)
{
	switch (ratio)
	{
	case 2:
		Left[0] = '1';
		Right[0] = '8';
		break;
	case 4:
		Left[0] = '1';
		Right[0] = '4';
		break;
	case 8:
		Left[0] = '1';
		Right[0] = '2';
		break;
	case 16:
		Left[0] = '1';
		Right[0] = '1';
		break;
	case 32:
		Left[0] = '2';
		Right[0] = '1';
		break;
	case 64:
		Left[0] = '4';
		Right[0] = '1';
		break;
	case 128:
		Left[0] = '8';
		Right[0] = '1';
		break;
	}
}

LRESULT CALLBACK GotoProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char temp[3];
	static HWND hwndEdit1, hwndEdit2, hwndEdit3;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit1 = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndEdit2 = GetDlgItem(hwndDlg, IDC_EDIT2);
		hwndEdit3 = GetDlgItem(hwndDlg, IDC_EDIT3);
		SetFocus(hwndEdit1);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (GetWindowText(hwndEdit1, temp, 3)) {
				if (temp[1])
					Minutes = ((temp[0] - '0') * 10) + (temp[1] - '0');
				else
					Minutes = temp[0] - '0';
			}
			else
				Minutes = 0;
			if (GetWindowText(hwndEdit2, temp, 3)) {
				if (temp[1])
					Seconds = ((temp[0] - '0') * 10) + (temp[1] - '0');
				else
					Seconds = temp[0] - '0';
			}
			else
				Seconds = 0;
			temp[0] = '0'; temp[1] = '0'; temp[2] = '0';
			if (GetWindowText(hwndEdit3, temp, 4)) {
				Milliseconds = ((temp[0] - '0') * 100) + ((temp[1] - '0') * 10) + (temp[2] - '0');
			}
			else
				Milliseconds = 0;
			EndDialog (hwndDlg, TRUE);
			return TRUE;
		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

void CALLBACK TimerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	PostMessage(hwnd, WM_USER2, 0, 0);
}

void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam)
{
	if (message == WOM_DONE)
		PostMessage(hwnd, WM_USER, 0, 0);
}

void OpenWaveFile(void)
{
	hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		if (fileSize = GetFileSize(hFile, NULL)) {
			PTR = 0;
			divisor = 0x10000;
			ratio = 16;
			totalMilliseconds = 0.0;
			WaveHeight[17] = Left[0];
			WaveHeight[19] = Right[0];
			time[0] = '0';
			time[1] = '0';
			time[3] = '0';
			time[4] = '0';
			time[6] = '0';
			time[7] = '0';
			time[8] = '0';
			if (WaveBuf)
				VirtualFree(WaveBuf, 0, MEM_RELEASE);
			WaveBuf = VirtualAlloc(NULL, fileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			ReadFile(hFile, WaveBuf, fileSize, &dwBytesRead, NULL);
			CloseHandle(hFile);

			if (*(DWORD*)&WaveBuf[8] == 0x45564157) {// "WAVE"
				if (*(WORD*)&WaveBuf[20] == 1) {// PCM type
					subchunksize = *(DWORD*)&WaveBuf[16];
					nextchunk = subchunksize + 20;
					if (*(DWORD*)&WaveBuf[nextchunk] == 0x74636166)// "fact"
						nextchunk += 12;
					if (*(DWORD*)&WaveBuf[nextchunk] != 0x61746164) // "data"
						nextchunk += (*(DWORD*)&WaveBuf[nextchunk+4]) + 8;
					if (*(DWORD*)&WaveBuf[nextchunk] == 0x61746164) {// "data"
						Buf = &WaveBuf[nextchunk+8]; // beginning of data
						BufSize = *(int*)&WaveBuf[nextchunk+4];
						WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
						WaveFormat.nChannels = *(WORD*)&WaveBuf[22];
//asdf
//*(DWORD*)&WaveBuf[24] *= 2;
						WaveFormat.nSamplesPerSec = *(DWORD*)&WaveBuf[24];
						WaveFormat.nBlockAlign = *(WORD*)&WaveBuf[32];
//asdf
//*(DWORD*)&WaveBuf[28] *= 2;
						WaveFormat.nAvgBytesPerSec = *(DWORD*)&WaveBuf[28];
						WaveFormat.wBitsPerSample = *(WORD*)&WaveBuf[34];
						WaveFormat.cbSize = 0;
						nChannels = WaveFormat.nChannels;
						nSamplesPerSec = WaveFormat.nSamplesPerSec;
						pointsPerMillisecond = (double)nSamplesPerSec / 1000.0;
						millisecondsPerScreen = pointsPerScreen / pointsPerMillisecond;
						wBitsPerSample = WaveFormat.wBitsPerSample;
// see how 2 wave forms combine...
for (x = 0; FullFilename[x] != '.'; x++)
	;
FullFilename[x++] = 'z';
FullFilename[x++] = '.';
FullFilename[x++] = 'w';
FullFilename[x++] = 'a';
FullFilename[x++] = 'v';
hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
if (hFile != INVALID_HANDLE_VALUE) {
	if (fileSize2 = GetFileSize(hFile, NULL)) {
		WaveBuf2 = VirtualAlloc(NULL, fileSize2, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		ReadFile(hFile, WaveBuf2, fileSize2, &dwBytesRead, NULL);
		CloseHandle(hFile);
		for (x = nextchunk+8; (x < (int)fileSize) && (x < (int)fileSize2); x += 2) {
			z = *(SHORT*)&WaveBuf[x] + *(SHORT*)&WaveBuf2[x];
			z >>= 1;
			*(WORD*)&WaveBuf[x] = (WORD)z;
		}
	}
}
if (fileSize2 < fileSize)
	fileSize = fileSize2;

/*
//asdf
for (x = 2, dTemp = 0.0; x < (int)fileSize; x += 2) {
	shortX = *(SHORT*)&Buf[x];
	_asm fild shortX
	_asm fstp doubleX
	dTemp += weight * (doubleX - dTemp);
	_asm fld dTemp
	_asm fistp shortX
	*(SHORT*)&Buf[x] = shortX;
}
hFile = CreateFile("Talking2.wav", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
if (hFile != INVALID_HANDLE_VALUE) {
	WriteFile(hFile, WaveBuf, fileSize, &dwBytesWritten, NULL);
	CloseHandle(hFile);
}
*/

						waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)&waveOutProc, 0, CALLBACK_FUNCTION);
						WaveOutHdr.lpData = (LPSTR)&WaveBuf[nextchunk+8];
						WaveOutHdr.dwBufferLength = *(DWORD*)&WaveBuf[nextchunk+4];
						lastPTR = WaveOutHdr.dwBufferLength - rect.right;
						WaveOutHdr.dwBytesRecorded = 0;
						WaveOutHdr.dwUser = 0;
						WaveOutHdr.dwFlags = 0;
						WaveOutHdr.dwLoops = 0;
						waveOutPrepareHeader(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
						x = waveOutWrite(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));

						timeBeginPeriod(TIMER_RESOLUTION);
						uTimerID = timeSetEvent(100, TIMER_RESOLUTION, TimerFunc, 0, TIME_PERIODIC);// every 100 milliseconds
						playing = PLAYING;
						InvalidateRect(hwnd, &rect, FALSE);
						for (x = 0; Filename[x] != 0; x++)
							;
						Filename[x++] = ' ';
						_itoa(WaveFormat.nSamplesPerSec, temp, 10);
						for (y = 0; temp[y] != 0; x++, y++)
							Filename[x] = temp[y];
						Filename[x++] = 'H';
						Filename[x++] = 'z';
						Filename[x++] = ' ';
						_itoa(WaveFormat.wBitsPerSample, temp, 10);
						for (y = 0; temp[y] != 0; x++, y++)
							Filename[x] = temp[y];
						Filename[x++] = 'b';
						Filename[x++] = 'i';
						Filename[x++] = 't';
						Filename[x] = 0;
						SetWindowText(hwnd, Filename); // 11025Hz 16bit
					}
					else {
					VirtualFree(WaveBuf, 0, MEM_RELEASE);
					WaveBuf = NULL;
					MessageBox(hwnd, "", ERROR, MB_OK);
					}
				}
				else {
					VirtualFree(WaveBuf, 0, MEM_RELEASE);
					WaveBuf = NULL;
					MessageBox(hwnd, "Can't play that kind of WAVE file.", ERROR, MB_OK);
				}
			}
			else {
				VirtualFree(WaveBuf, 0, MEM_RELEASE);
				WaveBuf = NULL;
				MessageBox(hwnd, "That's not a WAVE file", ERROR, MB_OK);
			}
		}
	}
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
//asdf
//dHz = 1200.0;
//weight = (dHz - 20.0) / 20000.0;

		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.hwndOwner         = hwnd;
		ofn.hInstance         = hInst;
		ofn.lpstrFilter       = " WAVE files\0""*.wav;*.wave\0\0";
		ofn.lpstrFile         = FullFilename;
		ofn.lpstrFileTitle    = Filename;
		ofn.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
		ofn.lpstrTitle        = NULL;
		ofn.lpstrDefExt       = "wav";
		ofn.nMaxFile          = MAX_PATH;
		ofn.lpstrCustomFilter = 0;
		ofn.nMaxCustFilter    = 0;
		ofn.nFilterIndex      = 0;
		ofn.nMaxFileTitle     = MAX_PATH;
		ofn.lpstrInitialDir   = NULL;
		ofn.nFileOffset       = 0;
		ofn.nFileExtension    = 0;
		ofn.lCustData         = 0;
		ofn.lpfnHook          = NULL;
		ofn.lpTemplateName    = NULL;
		hMenu = GetMenu(hwnd);
		WaveBuf = NULL;
		playing = STOPPED;
		return 0;

	case WM_SIZE:
		rect.left = rect.top = 0;
		rect.right = LOWORD(lParam);
		rectrightx2 = rect.right * 2;
		rect.bottom = HIWORD(lParam);
		middle = rect.bottom / 2;
		topmiddle = rect.bottom / 4;
		bottommiddle = rect.bottom * 3 / 4;
		PointsPerScreen = rect.right;
		_asm fild PointsPerScreen
		_asm fstp pointsPerScreen
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILES_OPEN:
			if (GetOpenFileName(&ofn))
				OpenWaveFile();
			break;
		case ID_FILES_EXIT:
			DestroyWindow(hwnd);
			break;

		case ID_PLAY:
			if (playing != PLAYING) {
				if (PTR < (BufSize - rectrightx2)) {
					if (playing == STOPPED) {
						waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)&waveOutProc, 0, CALLBACK_FUNCTION);
						WaveOutHdr.lpData = (LPSTR)&Buf[PTR];
						WaveOutHdr.dwBufferLength = *(DWORD*)&WaveBuf[nextchunk+4];
						WaveOutHdr.dwBufferLength -= PTR;
						WaveOutHdr.dwBytesRecorded = 0;
						WaveOutHdr.dwUser = 0;
						WaveOutHdr.dwFlags = 0;
						WaveOutHdr.dwLoops = 0;
						waveOutPrepareHeader(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
						waveOutWrite(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
					}
					else if (playing == PAUSED) {
						waveOutRestart(hWaveOut);
					}
					timeBeginPeriod(TIMER_RESOLUTION);
					uTimerID = timeSetEvent(100, TIMER_RESOLUTION, TimerFunc, 0, TIME_PERIODIC);
					playing = PLAYING;
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			break;

		case ID_PAUSE:
			if (playing == PLAYING) {
				waveOutPause(hWaveOut);
				if (uTimerID) {
					timeKillEvent(uTimerID);
					uTimerID = 0;
					timeEndPeriod(TIMER_RESOLUTION);
				}
				playing = PAUSED;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case ID_STOP:
			if (playing != STOPPED) {
				StopPlaying();
				SendMessage(hwnd, WM_KEYDOWN, VK_HOME, 0);
			}
			break;

		case ID_GOTO:
			if (Filename[0]) {
				if (DialogBox(hInst, "GOTO", hwnd, GotoProc)) {
					x = nSamplesPerSec * 2 * ((Minutes * 60) + Seconds);
					d = pointsPerMillisecond * 2.0 * (double)(Milliseconds + (((Minutes * 60) + Seconds) * 1000));
					x = (int)d & 0xFFFFFFFE; // to make it an even number (data is in WORDS)
					x &= 0xFFFFFFFE;
					if (x < (BufSize - rectrightx2)) {
						PTR = x;
						StopPlaying();
						time[0] = (char)(Minutes / 10) + '0';
						time[1] = (char)(Minutes % 10) + '0';
						time[3] = (char)(Seconds / 10) + '0';
						time[4] = (char)(Seconds % 10) + '0';
						time[6] = (char)((Milliseconds % 1000) / 100) + '0';
						time[7] = (char)((Milliseconds % 100) / 10) + '0';
						time[8] = (char)(Milliseconds % 10) + '0';
						InvalidateRect(hwnd, &rect, FALSE);
					}
				}
			}
			break;

		case ID_LINESSCREEN_1:
			linesperscreen = 1;
			CheckMenuItem(hMenu, ID_LINESSCREEN_1, MF_BYCOMMAND|MF_CHECKED);
			CheckMenuItem(hMenu, ID_LINESSCREEN_3, MF_BYCOMMAND|MF_UNCHECKED);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case ID_LINESSCREEN_3:
			linesperscreen = 3;
			CheckMenuItem(hMenu, ID_LINESSCREEN_3, MF_BYCOMMAND|MF_CHECKED);
			CheckMenuItem(hMenu, ID_LINESSCREEN_1, MF_BYCOMMAND|MF_UNCHECKED);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case MY_HELP:
			MessageBox(hwnd, Help, szAppName, MB_OK);
			break;

		case PLUS:
			if (ratio < 128) {
				divisor >>= 1;
				ratio <<= 1;
				ShowRatios();
				WaveHeight[17] = Left[0];
				WaveHeight[19] = Right[0];
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case MINUS:
			if (ratio > 2) {
				divisor <<= 1;
				ratio >>= 1;
				ShowRatios();
				WaveHeight[17] = Left[0];
				WaveHeight[19] = Right[0];
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;
		}
		return 0;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_SPACE:
			if (playing == PLAYING)
				SendMessage(hwnd, WM_COMMAND, ID_PAUSE, 0);
			else if (Filename[0])
				SendMessage(hwnd, WM_COMMAND, ID_PLAY, 0);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case VK_NEXT:
			if (playing != PLAYING) {
				if (PTR < (BufSize - rectrightx2)) {
					PTR += rectrightx2;
					totalMilliseconds += millisecondsPerScreen;
					GetTime();
					if (playing == PAUSED)
						StopPlaying();
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			break;
		case VK_PRIOR:
			if (playing != PLAYING) {
//				if (PTR < (int)(nextchunk+8+rect.right)) {
//					PTR = rect.right;
//					totalMilliseconds = millisecondsPerScreen;
//					InvalidateRect(hwnd, &rect, FALSE);
//				}
				if (PTR >= rectrightx2) {
					PTR -= rectrightx2;
					totalMilliseconds -= millisecondsPerScreen;
					GetTime();
					if (playing == PAUSED)
						StopPlaying();
					InvalidateRect(hwnd, &rect, FALSE);
					UpdateWindow(hwnd);
				}
			}
			break;
		case VK_HOME:
			if (playing != PLAYING) {
				PTR = 0;
				totalMilliseconds = 0.0;
				time[0] = '0';
				time[1] = '0';
				time[3] = '0';
				time[4] = '0';
				time[6] = '0';
				time[7] = '0';
				time[8] = '0';
				playing = STOPPED;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;
		case VK_END:
			if (playing != PLAYING) {
				PTR = BufSize - rectrightx2;
				totalMilliseconds = PTR / (pointsPerMillisecond * 2);
				GetTime();
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;
		case VK_ESCAPE:
			if (playing == PLAYING)
				SendMessage(hwnd, WM_COMMAND, ID_STOP, 0);
			else
				DestroyWindow(hwnd);
			break;
		case 187:// '+'
			SendMessage(hwnd, WM_COMMAND, PLUS, 0);
			break;
		case 189:// '-'
			SendMessage(hwnd, WM_COMMAND, MINUS, 0);
			break;
		}
		return 0;

	case WM_USER2:// "00:00:000"
		time[6]++;
		if (time[6] == ':') {
			time[6] = '0';
			time[4]++;
			if (time[4] == ':') {
				time[4] = '0';
				time[3]++;
				if (time[3] == '6') {
					time[3] = '0';
					time[1]++;
					if (time[1] == ':') {
						time[1] = '0';
						time[0]++;
					}
				}
			}
		}
		PTR += (nSamplesPerSec / 5) & 0xFFFFFFFE; // to make it even
		if ((PTR < lastPTR) && (PTR < (BufSize - rectrightx2))) {
			totalMilliseconds += 100;
			InvalidateRect(hwnd, &rect, FALSE);
		}
		else
			PTR -= (nSamplesPerSec / 5) & 0xFFFFFFFE; // to make it even
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		FillRect(hdc, &rect, hBrush);
		if (linesperscreen == 3) {
			MoveToEx(hdc, 0, topmiddle, NULL);
			LineTo(hdc, rect.right, topmiddle);
		}
		MoveToEx(hdc, 0, middle, NULL);
		LineTo(hdc, rect.right, middle);
		if (linesperscreen == 3) {
			MoveToEx(hdc, 0, bottommiddle, NULL);
			LineTo(hdc, rect.right, bottommiddle);
		}
		if (WaveBuf) {
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, 0, middle - 20, WaveHeight, 20);
			TextOut(hdc, 0, middle + 10, time, 9);
			SetBkMode(hdc,	OPAQUE);
			if (WaveFormat.nChannels == 1) {
				if ((linesperscreen == 3) && (PTR >= rectrightx2) && (PTR < (BufSize - rectrightx2))) {
					MoveToEx(hdc, 0, topmiddle, NULL);
					for (x = 0, y = 0; x < rect.right; x++, y += 2) {
						LineTo(hdc, x, topmiddle - (*(SHORT*)&Buf[y+PTR-rectrightx2]*rect.bottom/divisor));
					}
				}
				if (PTR < (BufSize - rectrightx2)) {
					MoveToEx(hdc, 0, middle, NULL);
					for (x = 0, y = 0; x < rect.right; x++, y += 2) {
						LineTo(hdc, x, middle - (*(SHORT*)&Buf[y+PTR]*rect.bottom/divisor));
					}
				}
				if ((linesperscreen == 3) && (PTR < (BufSize - (rect.right*4)))) {
					MoveToEx(hdc, 0, bottommiddle, NULL);
					for (x = 0, y = 0; x < rect.right; x++, y += 2) {
						LineTo(hdc, x, bottommiddle - (*(SHORT*)&Buf[y+PTR+rectrightx2]*rect.bottom/divisor));
					}
				}
			}
			else { // stereo
				for (x = 0, z = 0; x < rect.right; x++, z += 2) {
					if (oddeven)
						SelectObject(hdc, hRedPen);
					else
						SelectObject(hdc, hGreenPen);
					oddeven ^= 1;
					y = middle - *(SHORT*)&Buf[z+PTR]*rect.bottom/divisor;
					Ellipse(hdc, x-2, y-2, x+2, y+2);
				}
			}
		}
		EndPaint(hwnd, &ps);

		if (first) {
			first = FALSE;
			if (nosendtoflg)
				GetOpenFileName (&ofn);
			OpenWaveFile();
		}
		return 0;

	case WM_USER:
		StopPlaying();
		return 0;

	case WM_DESTROY:
		if (WaveBuf != NULL)
			VirtualFree(WaveBuf, 0, MEM_RELEASE);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
