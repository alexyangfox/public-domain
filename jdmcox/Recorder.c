#include <windows.h>
#include <mmsystem.h>// add winmm.lib to Project -Settings -Link

//                       R    I    F    F  chunk size            W    A    V    E    f    m    t       subchunk1 size      PMC       channels  sample rate         byte rate           blockalignbits/sample d    a    t    a  subchunk2 size
BYTE WaveHeader[44] = {0x52,0x49,0x46,0x46,0x00,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x10,0x00,0x64,0x61,0x74,0x61,0x00,0x00,0x00,0x00};
//                        0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31   32   33   34   35   36   37   38   39   40   41   42   43
DWORD x, wb, fileSize, dwBytesRead, dwBytesWritten, fileSize, FullSize, BufferSize = 1323000;// 1 minute
DWORD mixerNumber, NumOfMixers, Destination, RecordingDevices = 0;
UINT DeviceNum = 0;
BYTE *WaveBufs[15];
BOOL Used[15], first = TRUE, fromclose = FALSE;
char szAppName[] = " ";
char IniBuf[8];
char Filename[MAX_PATH];
char DeviceName[12][MAXPNAMELEN];
char Number[1];
char Help0[] = "If a mike is connected, and the mike volume is okay, you're recording now. Press Esc to stop.";
char Help1[] = "Records from a microphone at 11.025 kHz, 16-bits, monaural.";
char Help2[] = "Records to 15 buffers that hold 1 minute of sound each (1,323,000 bytes each).";
char Help3[] = "When the last one is full, recording begins at the first buffer again.";
char Help4[] = "When the program is exited, these buffers are saved to a file named [date][time].wav, beginning with the earliest recorded buffer.";
char info[] = "If nothing is heard on playback, change the number in Recorder.ini to the number next to the microphone name, below.";
char Year[4];
char Month[2];
char Day[2];
char Hour[2];
char Minute[2];
char Second[2];

HWND hwnd;
HANDLE hFile;
HDC hdc;
PAINTSTRUCT ps;
SYSTEMTIME st;
HWAVEIN hWaveIn;
WAVEHDR WaveInHdr;
WAVEFORMATEX WaveFormat;
HMIXER hMixer;
MIXERCAPS mixerCaps;
MIXERLINE mixerLine;
MMRESULT MixerError;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
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

void CALLBACK waveInProc(HWAVEIN hWaveIn, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam)
{
	if (message == WIM_DATA)
		PostMessage(hwnd, WM_USER, 0, 0);
}

void FillFilename(void)
{// "2010.5.10 8.20.21.wav"
	int x = 0;

	_itoa(st.wYear, &Filename[x], 10);
	x += 4;
	Filename[x++] = '.';
	_itoa(st.wMonth, &Filename[x], 10);
	if (Filename[x+1] == 0) x++; else x += 2;
	Filename[x++] = '.';
	_itoa(st.wDay, &Filename[x], 10);
	if (Filename[x+1] == 0) x++; else x += 2;
	Filename[x++] = ' ';
	_itoa(st.wHour, &Filename[x], 10);
	if (Filename[x+1] == 0) x++; else x += 2;
	Filename[x++] = '.';
	_itoa(st.wMinute, &Filename[x], 10);
	if (Filename[x+1] == 0) x++; else x += 2;
	Filename[x++] = '.';
	_itoa(st.wSecond, &Filename[x], 10);
	if (Filename[x+1] == 0) x++; else x += 2;
	Filename[x++] = '.';
	Filename[x++] = 'w';
	Filename[x++] = 'a';
	Filename[x++] = 'v';
	Filename[x] = 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		NumOfMixers = mixerGetNumDevs();
		for (mixerNumber = 0; mixerNumber < NumOfMixers; mixerNumber++) {
			mixerOpen(&hMixer, mixerNumber, 0, 0, 0);
			mixerGetDevCaps((UINT)hMixer, &mixerCaps, sizeof(MIXERCAPS));
			for (Destination = 0; Destination < mixerCaps.cDestinations; Destination++) {
				mixerLine.cbStruct = sizeof(MIXERLINE);
				mixerLine.dwDestination = Destination;
				MixerError = mixerGetLineInfo((HMIXEROBJ)hMixer, &mixerLine, MIXER_GETLINEINFOF_DESTINATION);
				if ((!MixerError) && (mixerLine.cControls)) {
					if (mixerLine.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN) {
						strcpy(DeviceName[RecordingDevices++],mixerCaps.szPname);
					}
				}
			}
			mixerClose(hMixer);
		}
		hFile = CreateFile("Recorder.ini", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			if (fileSize = GetFileSize(hFile, NULL)) {
				if (fileSize > 8)
					fileSize = 8;
				ReadFile(hFile, IniBuf, fileSize, &dwBytesRead, NULL);
				DeviceNum = IniBuf[0] - '0';
			}
			CloseHandle(hFile);
		}
		else {
			hFile = CreateFile("Recorder.ini", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			WriteFile(hFile, "0", 1, &dwBytesWritten, NULL);
			CloseHandle(hFile);
		}

		for (x = 0; x < 15; x++)
			Used[x] = FALSE;
		WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
		WaveFormat.nChannels = 1;
		WaveFormat.nBlockAlign = 2;
		WaveFormat.nSamplesPerSec = 11025;
		WaveFormat.nAvgBytesPerSec = (DWORD)22050;
		WaveFormat.wBitsPerSample = 16;
		WaveFormat.cbSize = 0;
		for (x = 0; x < 15; x++)
			WaveBufs[x] = VirtualAlloc(NULL, BufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
		wb = 0;
		WaveInHdr.lpData = (LPSTR)WaveBufs[wb];
		WaveInHdr.dwBufferLength = BufferSize;
		WaveInHdr.dwBytesRecorded = 0;
		WaveInHdr.dwUser = 0;
		WaveInHdr.dwFlags = 0;
		WaveInHdr.dwLoops = 0;
		GetLocalTime(&st);
		FillFilename();
		waveInOpen(&hWaveIn, DeviceNum, &WaveFormat, (DWORD)&waveInProc, 0, CALLBACK_FUNCTION);
		waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
		waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
		waveInStart(hWaveIn);
		return 0;

	case WM_USER:
		if (!fromclose) {
			Used[wb] = TRUE;
			wb++;
			if (wb == 15)
				wb = 0;
			WaveInHdr.lpData = (LPSTR)WaveBufs[wb];
			waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
			waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
			waveInStart(hWaveIn);
		}
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(hwnd);
		break;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		TextOut(hdc, 20, 0, Help0, sizeof(Help0));
		TextOut(hdc, 20, 20, Help1, sizeof(Help1));
		TextOut(hdc, 20, 40, Help2, sizeof(Help2));
		TextOut(hdc, 20, 60, Help3, sizeof(Help3));
		TextOut(hdc, 20, 80, Help4, sizeof(Help4));
		TextOut(hdc, 20, 100, info, sizeof(info));
		Number[0] = '0';
		for (x = 0; x < RecordingDevices; x++) {
			TextOut(hdc, 5, 120 + (x * 20), Number, 1);
			Number[0]++;
			TextOut(hdc, 20, 120 + (x * 20), DeviceName[x], strlen(DeviceName[x]));
		}
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		for (x = 0; x < BufferSize; x += 2) {
			if ((*(WORD*)&WaveBufs[0][x] >= 0x20) && (*(WORD*)&WaveBufs[0][x] <= 0xFFE0))
				break;
		}
		if (x < BufferSize) {
			fromclose = TRUE;
			Used[wb] = TRUE;
			waveInReset(hWaveIn);
			waveInUnprepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
			waveInClose(hWaveIn);
			for (FullSize = 0, x = 0; x < 15; x++)
				if (Used[x])
					FullSize += BufferSize;
			*(DWORD*)&WaveHeader[4] = 36 + FullSize;
			*(WORD*)&WaveHeader[22] = 1;
			*(DWORD*)&WaveHeader[24] = 11025;// SamplesPerSec
			*(DWORD*)&WaveHeader[28] = 22050;// bytes/sec
			*(WORD*)&WaveHeader[32] = 2;// Block Align
			*(WORD*)&WaveHeader[34] = 16;// wBitsPerSample;
			*(DWORD*)&WaveHeader[40] = FullSize;
			hFile = CreateFile(Filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			WriteFile(hFile, WaveHeader, 44, &dwBytesWritten, NULL);
			for (wb++, x = 0; x < 15; x++, wb++) {
				if (wb == 15)
					wb = 0;
				if (Used[wb])
					WriteFile(hFile, WaveBufs[wb], BufferSize, &dwBytesWritten, NULL);
				VirtualFree(WaveBufs[wb], 0, MEM_RELEASE);
			}
			CloseHandle(hFile);
		}
		else
			MessageBox(hwnd, "Either no mike is plugged in,\n\nor the recording is VERY quiet,\n\nor the number in Recorder.ini is wrong.", NULL, MB_OK);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
