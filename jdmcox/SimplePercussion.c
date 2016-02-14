//Dan's wish list:
//disable/enable instruments
//remove instruments
//focus play window
#include <windows.h>
#include <mmsystem.h>//add winmm.lib to Project -Settings -Link
#define IDM_FILES 0x100
#define IDM_DEVICE 0x200
#define IDM_PLAY 0300
#define IDM_VOL 0x400
#define IDM_TIME 0x500
#define IDM_BEATS 0x600
#define IDM_VERTLINE 0x700
#define IDM_ABOUT 0x800
#define TIMER_RESOLUTION 5

char About[] = "Use the Mouse Buttons to select/deselect,\nand the SPACE BAR to play/stop playing.\n\nVersion 1.065\nFeb 10, 2009\nhttp://jdmcox.com\n\nThanks to Charles Petzold";

int i, w, x, y, z, Line = 0, xPos, yPos, Volume = 127, Time = 10, Beats = -1, maxBeats = 0, BeatEntries, VertLine = 4;
int iNumDevs, iOutDevice = MIDIMAPPER, CurrentBeat = -1, PreviousBeat = 0, CmdLineLen, number;
int	copyiOutDevice = -1, copyVolume = -1, copyTime= -1, copyBeats= -1, copyVertLine = -1;
DWORD fileSize, dwBytesRead, dwBytesWritten;
unsigned int uTimerID;
char Filename[MAX_PATH] = "\x0";
char FullFilename[MAX_PATH] = "\x0";
char szAppName[] = "SimplePercussion";
char Play[] = "&PLAY";
char Stop[] = "&STOP";
char VolumeChoice[8][4] = {"128","112"," 96"," 80"," 64"," 48"," 32"," 16"};
char MenuBeat[3];
char MenuBeats[4] = "\0\0\0";
char Number[16];
char DoubleLine[11][9] = {"Every  2","Every  3","Every  4","Every  5","Every  6","Every  7","Every  8","Every  9","Every 10","Every 11","Every 12"};
BYTE BeatsInLoop[100];
BYTE Percussion[47][120];
BYTE PercussionCopy[47][120];
BOOL fromleftbutton, play = FALSE, showingtime = FALSE, showingbeats = FALSE, clearscreen = FALSE, first = TRUE, fromuser = FALSE;
HWND hwnd, hwndTime, hwndBeats;
HINSTANCE hInst;
HANDLE hFile = NULL, hFindFile;
HDC hdc;
PAINTSTRUCT ps;
RECT rect;
HBRUSH hGrayBrush, hWhiteBrush, hBlackBrush, hOldBrush;
HPEN hInvisiblePen, hDoublePen, hBlackPen, hOldPen;
HMENU hMenu, hMenuPopup, hMenuBeatsInLoopPopup;
HMIDIOUT hMidiOut = NULL;
MIDIOUTCAPS moc;
OPENFILENAME ofn;
WNDPROC pTimeProc, pBeatsProc;

char *Files[4] = {"&Open","&New","&Save","E&xit"};
char *Choices[47] =
{
"Acoustic Bass Drum",
"Bass Drum 1",
"Side Stick",
"Acoustic Snare",
"Hand Clap",
"Electric Snare",
"Low Floor Tom",
"Closed Hi-Hat",
"High Floor Tom",
"Pedal Hi-Hat",
"Low Tom",
"Open Hi-Hat",
"Low-Mid Tom",
"Hi-Mid Tom",
"Crash Cymbal 1",
"High Tom",
"Ride Cymbal 1",
"Chinese Cymbal",
"Ride Bell",
"Tambourine",
"Splash Cymbal",
"Cowbell",
"Crash Cymbal 2",
"Vibraslap",
"Ride Cymbal 2",
"Hi Bongo",
"Low Bongo",
"Mute Hi Conga",
"Open Hi Conga",
"Low Conga",
"High Timbale",
"Low Timbale",
"High Agogo",
"Low Agogo",
"Cabasa",
"Maracas",
"Short Whistle",
"Long Whistle",
"Short Guiro",
"Long Guiro",
"Claves",
"Hi Wood Block",
"Low Wood Block",
"Mute Cuica",
"Open Cuica",
"Mute Triangle",
"Open Triangle"
};

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;
	hInst = hInstance;

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

	if (szCmdLine[0] != 0)
	{
		for (x = 0, CmdLineLen = 0; szCmdLine[x] != 0; x++)
			if (szCmdLine[x] != '"')
				FullFilename[CmdLineLen++] = szCmdLine[x];
		FullFilename[CmdLineLen] = 0;
	}

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPED|WS_SYSMENU|WS_MINIMIZEBOX,
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

/////////////////////////////////////subroutines
void EraseHighlight(void)
{
	hdc = GetDC(hwnd);
	hOldBrush = SelectObject(hdc, hWhiteBrush);
	hOldPen = SelectObject(hdc, hInvisiblePen);
	Rectangle(hdc, 1, (Line*w)+1, z+1, ((Line+1)*w)+1);
	SelectObject(hdc, hBlackBrush);
	for (x = 0; x < Beats; x++)//, y = (z-150)/w
		if (Percussion[Line][x] == 1)
			Rectangle(hdc, (x*w)+150+1, (Line*w)+1, (x*w)+150+w+1, (Line*w)+w+1);
	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldBrush);
	ReleaseDC(hwnd, hdc);
}

void DrawRectangle(void)
{
	if ((xPos > 150) && (xPos < z)) {
		x = (xPos - ((xPos-150) % w)) + 1;
		y = (yPos - (yPos % w)) + 1;
		hdc = GetDC(hwnd);
		if (fromleftbutton) {
			Percussion[Line][(x-150)/w] = 1;
			hOldBrush = SelectObject(hdc, hBlackBrush);
		}
		else {
			Percussion[Line][(x-150)/w] = 0;
			hOldBrush = SelectObject(hdc, hGrayBrush);
		}
		Rectangle(hdc, x, y, x+w, y+w);
		SelectObject(hdc, hOldBrush);
		ReleaseDC(hwnd, hdc);
	}
}

void CALLBACK DrumTimerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	int i;

	if (CurrentBeat != -1)
	{
		for (i = 0; i < 47; i++)
			if (Percussion[i][PreviousBeat])
				midiOutShortMsg(hMidiOut, 0x89 | ((i+35) << 8));//first drum midi number is 35
		PreviousBeat = CurrentBeat;
	}
	CurrentBeat = (CurrentBeat+1) % Beats;// % Beats to loop
	for (i = 0 ; i < 47 ; i++)
	{
		if (Percussion[i][CurrentBeat])
			midiOutShortMsg(hMidiOut, 0x99 | (Volume << 16) | ((i+35) << 8));
	}
	PostMessage(hwnd, WM_USER, 0, 0);
//	uTimerID = timeSetEvent(1000/Time, TIMER_RESOLUTION, DrumTimerFunc, 0, TIME_ONESHOT);
}

void DrumBeginSequence(void)
{
	if (MMSYSERR_NOERROR == midiOutOpen(&hMidiOut, iOutDevice, 0, 0, 0)) {
		midiOutShortMsg(hMidiOut, 0xC9);
		timeBeginPeriod(TIMER_RESOLUTION); 
//		uTimerID = timeSetEvent(1000/Time, TIMER_RESOLUTION, DrumTimerFunc, 0, TIME_ONESHOT);//changed to get a more consistent beat
		uTimerID = timeSetEvent(1000/Time, TIMER_RESOLUTION, DrumTimerFunc, 0, TIME_PERIODIC);
		if (uTimerID == 0) {
			timeEndPeriod(TIMER_RESOLUTION);
			midiOutClose(hMidiOut);
			hMidiOut = NULL;
		}
		CurrentBeat = -1;
	}
}

void DrumEndSequence()
{
	play = FALSE;
	ModifyMenu(hMenu, IDM_PLAY, MF_BYCOMMAND|MF_STRING, IDM_PLAY, Play);
	DrawMenuBar(hwnd);
	if (uTimerID)
		timeKillEvent(uTimerID);
	timeEndPeriod(TIMER_RESOLUTION);
	midiOutShortMsg(hMidiOut, 0xB9 | (123 << 8));
	midiOutClose(hMidiOut);
	hMidiOut = NULL;
	CurrentBeat = -1;
	PreviousBeat = 0;
	clearscreen = TRUE;
	InvalidateRect(hwnd, &rect, FALSE);
}

void Open(void)
{
	hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		if (fileSize = GetFileSize(hFile, NULL)) {
			ReadFile(hFile, &iOutDevice, 4, &dwBytesRead, NULL);
			ReadFile(hFile, &Volume, 4, &dwBytesRead, NULL);
			ReadFile(hFile, &Time, 4, &dwBytesRead, NULL);
			ReadFile(hFile, &Beats, 4, &dwBytesRead, NULL);
			ReadFile(hFile, &VertLine, 4, &dwBytesRead, NULL);
			ReadFile(hFile, Percussion, fileSize-20, &dwBytesRead, NULL);
			CopyMemory(PercussionCopy, Percussion, 47*120);//to check for changes when closing
			copyiOutDevice = iOutDevice;
			copyVolume = Volume;
			copyTime = Time;
			copyBeats = Beats;
			copyVertLine = VertLine;
		}
		CloseHandle(hFile);
	}
}

void Save(void)
{
	for (y = 0; y < 47; y++) {
		for (x = 0; x < 120; x++) {
			if (Percussion[y][x] != 0) {
				hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				WriteFile(hFile, &iOutDevice, 4, &dwBytesWritten, NULL);
				WriteFile(hFile, &Volume, 4, &dwBytesWritten, NULL);
				WriteFile(hFile, &Time, 4, &dwBytesWritten, NULL);
				WriteFile(hFile, &Beats, 4, &dwBytesWritten, NULL);
				WriteFile(hFile, &VertLine, 4, &dwBytesWritten, NULL);
				WriteFile(hFile, &Percussion, 47*120, &dwBytesWritten, NULL);
				CloseHandle(hFile);
				CopyMemory(PercussionCopy, Percussion, 47*120);//to check for changes when closing
				return;
			}
		}
	}
}

/////////////////////////////////////end subroutines

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.hwndOwner         = hwnd;
		ofn.hInstance         = NULL;
		ofn.lpstrFilter       = " *.sp\0*.sp\0\0";
		ofn.lpstrFile         = FullFilename;
		ofn.lpstrFileTitle    = Filename;
		ofn.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
		ofn.lpstrTitle        = NULL;
		ofn.lpstrDefExt       = "sp";
		ofn.nMaxFile          = MAX_PATH;
		ofn.lpstrCustomFilter = NULL;
		ofn.nMaxCustFilter    = 0;
		ofn.nFilterIndex      = 0;
		ofn.nMaxFileTitle     = MAX_PATH;
		ofn.lpstrInitialDir   = NULL;
		ofn.nFileOffset       = 0;
		ofn.nFileExtension    = 0;
		ofn.lCustData         = 0;
		ofn.lpfnHook          = NULL;
		ofn.lpTemplateName    = NULL;

		if (FullFilename[0] != 0) {//file opened from command line or by "Open With"
			for (x = CmdLineLen; (x > 0) && (FullFilename[x] != '\\'); x--)
				;
			for (x++, y = 0; x < CmdLineLen; x++, y++)
				Filename[y] = FullFilename[x];//for Title bar
			Filename[y] = 0;
			Open();
		}
		else {
			for (y = 0; y < 47; y++)
				for (x = 0; x < 120; x++)
					Percussion[y][x] = 0;
		}
		hMenu = CreateMenu();
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "&Files");
		for (x = 0; x < 4; x++)
			AppendMenu(hMenuPopup, MF_STRING, IDM_FILES + x, Files[x]);
		iNumDevs = midiOutGetNumDevs();
		if (!midiOutGetDevCaps(MIDIMAPPER, &moc, sizeof(moc))) {
			hMenuPopup = CreateMenu();
			AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "&Output Device");
			AppendMenu(hMenuPopup, MF_STRING, IDM_DEVICE + (int)MIDIMAPPER, moc.szPname);
			for (x = 0; x < iNumDevs; x++) {
				midiOutGetDevCaps(x, &moc, sizeof(moc));
				AppendMenu(hMenuPopup, MF_STRING, IDM_DEVICE + x, moc.szPname);
			}
			CheckMenuItem(hMenuPopup, IDM_DEVICE + iOutDevice, MF_CHECKED);
		}
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING, IDM_PLAY, Play);
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "Percussion &Volume");
		for (x = 0; x < 8; x++)
			AppendMenu(hMenuPopup, MF_STRING, IDM_VOL + x, VolumeChoice[x]);
		CheckMenuItem(hMenuPopup, 8 - ((Volume+1) / 16) + IDM_VOL, MF_CHECKED);
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "&Beats/Second");
		MenuBeat[0] = '0';
		MenuBeat[1] = '1';
		for (x = 0; x < 20; x++) {
			AppendMenu(hMenuPopup, MF_STRING, IDM_TIME + x, MenuBeat);
			MenuBeat[1]++;
			if (MenuBeat[1] > '9') {
				if (MenuBeat[0] == ' ')
					MenuBeat[0] = '0';
				MenuBeat[1] = '0';
				MenuBeat[0]++;
			}	
		}
		CheckMenuItem(hMenu, IDM_TIME + Time - 1, MF_CHECKED);

		hMenuBeatsInLoopPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuBeatsInLoopPopup, "&Beats/Loop");

		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "&Heavy Vertical Lines");
		for (x = 0; x < 11; x++)
			AppendMenu(hMenuPopup, MF_STRING, IDM_VERTLINE + x, DoubleLine[x]);
		CheckMenuItem(hMenuPopup, IDM_VERTLINE + 2, MF_CHECKED);
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING, IDM_ABOUT, "&About");
		SetMenu(hwnd, hMenu);

		hGrayBrush = CreateSolidBrush(0xE0E0E0);//light gray
		hWhiteBrush = CreateSolidBrush(0xFFFFFF);
		hBlackBrush = CreateSolidBrush(0);
		hInvisiblePen = CreatePen(PS_NULL, 0, 0);
		hBlackPen = CreatePen(PS_SOLID, 1, 0);
		hDoublePen = CreatePen(PS_SOLID, 2, 0);
		if ((hFile) && (hFile != INVALID_HANDLE_VALUE))
			SetWindowText(hwnd, Filename);
		else
			SetWindowText(hwnd, szAppName);
		return 0;

	case WM_SIZE:
		rect.left = rect.top = 0;
		rect.right = LOWORD(lParam);
		rect.bottom = HIWORD(lParam);
		if ((rect.right >= 150) && (rect.bottom >= 47)) {
			w = rect.bottom / 47;
			if ((Beats == -1) || (FullFilename[0] != 0)) {//first time
				z = rect.right - ((rect.right - 150) % w);
				maxBeats = (z-150)/w;
				if (FullFilename[0] == 0)//not from running by opening a .sp file
					Beats = ((z-150) / w) - (((z-150) / w) % VertLine);
			}
			z = 150+(Beats*w);
			for (x = Beats, y = 0; x > 0; x -= VertLine, y++) {
				_itoa(x, MenuBeats, 10);
				AppendMenu(hMenuBeatsInLoopPopup, MF_STRING, IDM_BEATS + y, MenuBeats);
			}
			BeatEntries = y;
			CheckMenuItem(hMenuBeatsInLoopPopup, IDM_BEATS + BeatEntries - (Beats/VertLine), MF_CHECKED);
			DrawMenuBar(hwnd);
		}
		return 0;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDM_FILES) {
			if (GetOpenFileName(&ofn)) {
				if (play)
					DrumEndSequence();
				SetWindowText(hwnd, Filename);
				CheckMenuItem(hMenu, IDM_DEVICE + iOutDevice, MF_UNCHECKED);
				CheckMenuItem(hMenu, 7 - (Volume / 16) + IDM_VOL, MF_UNCHECKED);
				CheckMenuItem(hMenu, IDM_TIME + Time - 1, MF_UNCHECKED);
				CheckMenuItem(hMenuBeatsInLoopPopup, IDM_BEATS + BeatEntries - (Beats/VertLine), MF_UNCHECKED);
				CheckMenuItem(hMenu, IDM_VERTLINE + VertLine-2, MF_UNCHECKED);
				Open();
				CheckMenuItem(hMenu, IDM_DEVICE + iOutDevice, MF_CHECKED);
				CheckMenuItem(hMenu, IDM_VOL + 7 - (Volume / 16), MF_CHECKED);
				CheckMenuItem(hMenu, IDM_TIME + Time - 1, MF_CHECKED);
				for (x = maxBeats, y = 0; x > 0; x -= VertLine, y++)
					RemoveMenu(hMenu, IDM_BEATS + y, MF_BYCOMMAND);
				for (x = Beats, y = 0; x > 0; x -= VertLine, y++) {
					_itoa(x, MenuBeats, 10);
					AppendMenu(hMenuBeatsInLoopPopup, MF_STRING, IDM_BEATS + y, MenuBeats);
				}
				BeatEntries = y;
				CheckMenuItem(hMenuBeatsInLoopPopup, IDM_BEATS + BeatEntries - (Beats/VertLine), MF_CHECKED);
				CheckMenuItem(hMenu, IDM_VERTLINE + VertLine - 2, MF_CHECKED);
				z = 150+(Beats*w);
				clearscreen = TRUE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		else if (LOWORD(wParam) == IDM_FILES+1) {
			if (IDYES == MessageBox(hwnd, "Clear Screen without Saving it?", "", MB_YESNO|MB_DEFBUTTON2)) {
				SetWindowText(hwnd, szAppName);
				CheckMenuItem(hMenu, IDM_VOL + 7 - (Volume / 16), MF_UNCHECKED);
				Volume = 127;
				CheckMenuItem(hMenu, IDM_VOL + 8 - ((Volume+1) / 16), MF_CHECKED);
				CheckMenuItem(hMenu, IDM_TIME + Time - 1, MF_UNCHECKED);
				Time = 10;
				CheckMenuItem(hMenu, IDM_TIME + Time - 1, MF_CHECKED);

				for (x = maxBeats, y = 0; x > 0; x -= VertLine, y++)
					RemoveMenu(hMenu, IDM_BEATS + y, MF_BYCOMMAND);
				CheckMenuItem(hMenu, IDM_VERTLINE + VertLine-2, MF_UNCHECKED);
				VertLine = 4;
				CheckMenuItem(hMenu, IDM_VERTLINE + VertLine-2, MF_CHECKED);
				z = 150+(maxBeats*w);
				Beats = ((z-150) / w) - (((z-150) / w) % VertLine);
				z = 150+(Beats*w);
				for (x = Beats, y = 0; x > 0; x -= VertLine, y++) {
					_itoa(x, MenuBeats, 10);
					AppendMenu(hMenuBeatsInLoopPopup, MF_STRING, IDM_BEATS + y, MenuBeats);
				}
				BeatEntries = y;
				CheckMenuItem(hMenuBeatsInLoopPopup, IDM_BEATS + BeatEntries - (Beats/VertLine), MF_CHECKED);

				copyiOutDevice = iOutDevice;
				copyVolume = Volume;
				copyTime = Time;
				copyBeats = Beats;
				copyVertLine = VertLine;
				CurrentBeat = -1;
				PreviousBeat = 0;
				Filename[0] = '\x0';
				FullFilename[0] = '\x0';
				for (y = 0; y < 47; y++) {
					for (x = 0; x < 120; x++)
						Percussion[y][x] = 0;
				}
				if (play)
					DrumEndSequence();
				DrawMenuBar(hwnd);
				clearscreen = TRUE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		else if (LOWORD(wParam) == IDM_FILES+2) {
			if (GetSaveFileName(&ofn))
				Save();
		}
		else if (LOWORD(wParam) == IDM_FILES+3) {
			DestroyWindow(hwnd);
		}

		else if ((LOWORD(wParam) >= (IDM_DEVICE-1)) && (LOWORD(wParam) < (IDM_DEVICE+0100))) {
			CheckMenuItem(hMenu, IDM_DEVICE + iOutDevice, MF_UNCHECKED);
			iOutDevice = LOWORD(wParam) - IDM_DEVICE;
			iOutDevice;
			CheckMenuItem(hMenu, IDM_DEVICE + iOutDevice, MF_CHECKED);
			if (play)
				DrumEndSequence();
		}

		else if (LOWORD(wParam) == IDM_PLAY) {
			if (play == FALSE) {
				play = TRUE;
				ModifyMenu(hMenu, IDM_PLAY, MF_BYCOMMAND|MF_STRING, IDM_PLAY, Stop);
				DrawMenuBar(hwnd);
				DrumBeginSequence();
			}
			else {
				DrumEndSequence();
			}
		}

		else if ((LOWORD(wParam) >= (IDM_VOL-1)) && (LOWORD(wParam) < (IDM_TIME-1))) {
			CheckMenuItem(hMenu, IDM_VOL + 7 - (Volume / 16), MF_UNCHECKED);
			Volume = ((8 - (LOWORD(wParam)-IDM_VOL)) * 16) - 1;
			CheckMenuItem(hMenu, LOWORD(wParam), MF_CHECKED);
		}

		else if ((LOWORD(wParam) >= (IDM_TIME-1)) && (LOWORD(wParam) < (IDM_BEATS-1))) {
			CheckMenuItem(hMenu, IDM_TIME + Time - 1, MF_UNCHECKED);
			Time = LOWORD(wParam) - IDM_TIME + 1;
			CheckMenuItem(hMenu, LOWORD(wParam), MF_CHECKED);
			if (play)
				DrumEndSequence();
		}

		else if ((LOWORD(wParam) >= (IDM_BEATS-1)) && (LOWORD(wParam) < (IDM_VERTLINE-1))) {
			CheckMenuItem(hMenu, IDM_BEATS + BeatEntries - (Beats/VertLine), MF_UNCHECKED);
			Beats = (BeatEntries - (LOWORD(wParam) - IDM_BEATS)) * VertLine;
			CheckMenuItem(hMenu, LOWORD(wParam), MF_CHECKED);
			z = 150+(Beats*w);
			clearscreen = TRUE;
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
		}

		else if ((LOWORD(wParam) >= (IDM_VERTLINE-1)) && (LOWORD(wParam) < (IDM_ABOUT-1))) {
			for (x = maxBeats, y = 0; x > 0; x -= VertLine, y++)
				RemoveMenu(hMenu, IDM_BEATS + y, MF_BYCOMMAND);
//				DeleteMenu(hMenu, IDM_BEATS + y, MF_BYCOMMAND);//this works, too
			CheckMenuItem(hMenu, IDM_VERTLINE + VertLine-2, MF_UNCHECKED);
			VertLine = LOWORD(wParam) - IDM_VERTLINE + 2;
			CheckMenuItem(hMenu, IDM_VERTLINE + VertLine-2, MF_CHECKED);
			z = 150+(maxBeats*w);
			Beats = ((z-150) / w) - (((z-150) / w) % VertLine);
			z = 150+(Beats*w);
			for (x = Beats, y = 0; x > 0; x -= VertLine, y++) {
				_itoa(x, MenuBeats, 10);
				AppendMenu(hMenuBeatsInLoopPopup, MF_STRING, IDM_BEATS + y, MenuBeats);
			}
			BeatEntries = y;
			CheckMenuItem(hMenuBeatsInLoopPopup, IDM_BEATS + BeatEntries - (Beats/VertLine), MF_CHECKED);
			clearscreen = TRUE;
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
		}

		else if (LOWORD(wParam) == IDM_ABOUT) {
			MessageBox(hwnd, About, szAppName, MB_OK);
		}
		return 0;

	case WM_LBUTTONDOWN:
		fromleftbutton = TRUE;
		DrawRectangle();
		return 0;

	case WM_RBUTTONDOWN:
 		fromleftbutton = FALSE;
		DrawRectangle();
		return 0;

	case WM_MOUSEMOVE:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		if ((yPos < (47*w)) && (xPos < z)){
			EraseHighlight();
			Line = yPos / w;
			InvalidateRect(hwnd, &rect, FALSE);
			if (wParam == MK_LBUTTON) {
 				fromleftbutton = TRUE;
				DrawRectangle();
			}
			else if (wParam == MK_RBUTTON) {
 				fromleftbutton = FALSE;
				DrawRectangle();
			}
		}
		return 0;

	case WM_NCLBUTTONDBLCLK:
		return 0;//because the WM_LBUTTONUP message is also sent with the wrong lParam valuse

	case WM_KEYDOWN:
		if (wParam == VK_SPACE)
			SendMessage(hwnd, WM_COMMAND, IDM_PLAY, 0);
		return 0;

	case WM_USER:
		fromuser = TRUE;
		InvalidateRect(hwnd, &rect, FALSE);
		return 0;

	case WM_PAINT:
		if ((w) && (z >= 150)) {
			hdc = BeginPaint(hwnd, &ps);
			if (clearscreen) {
				clearscreen = FALSE;
				FillRect(hdc, &rect, hWhiteBrush);
			}
			hOldPen = SelectObject(hdc, hInvisiblePen);
			if (fromuser) {
				fromuser = FALSE;
				hOldBrush = SelectObject(hdc, hWhiteBrush);
				Rectangle(hdc, 150+(PreviousBeat*w)+1, 1, 150+(PreviousBeat*w)+w+1, rect.bottom);
				SelectObject(hdc, hOldBrush);
			}
			hOldBrush = SelectObject(hdc, hGrayBrush);
			Rectangle(hdc, 1, (Line*w)+1, z+1, ((Line+1)*w)+1);
			if (CurrentBeat != -1)
				Rectangle(hdc, 150+(CurrentBeat*w)+1, 1, 150+(CurrentBeat*w)+w+1, rect.bottom);
			SelectObject(hdc, hOldBrush);
			SelectObject(hdc, hOldPen);
			SetBkMode(hdc, TRANSPARENT);
			MoveToEx(hdc, 150, 0, NULL);
			LineTo(hdc, z, 0);
			for (x = 0, y = 0; x < 47; x++, y += w) {
				TextOut(hdc, 10, y, Choices[x], lstrlen(Choices[x]));
				MoveToEx(hdc, 150, y+w, NULL);
				LineTo(hdc, z, y+w);
			}
			for (x = 150, y = 0; x <= z; x += w, y++) {
				if (y % (VertLine))
					hOldPen = SelectObject(hdc, hBlackPen);
				else
					hOldPen = SelectObject(hdc, hDoublePen);
				MoveToEx(hdc, x, 0, NULL);
				LineTo(hdc, x, 47*w);
				SelectObject(hdc, hOldPen);
			}
			SetBkMode(hdc, OPAQUE);
			hOldBrush = SelectObject(hdc, hBlackBrush);
			for (i = 0; i < 47; i++) {
				for (x = 0; x < Beats; x++) {
					if (Percussion[i][x] == 1)
						Rectangle(hdc, (x*w)+150+1, (i*w)+1, (x*w)+150+w+1, (i*w)+w+1);
				}
			}
			SelectObject(hdc, hOldBrush);
			EndPaint(hwnd, &ps);
		}
		return 0;

	case WM_DESTROY:
		if (hMidiOut)
			midiOutClose(hMidiOut);
		for (y = 0; y < 47; y++) {
			for (x = 0; x < 120; x++) {
				if (Percussion[y][x] != 0) {
					if ((copyiOutDevice != iOutDevice) || (copyVolume != Volume) || (copyTime != Time) || (copyBeats != Beats) || (copyVertLine != VertLine)) {
						if (IDYES == MessageBox(hwnd, Filename, "Save?", MB_YESNO))
							Save();
						goto endo;
					}
					if (FullFilename[0]) {
						for (y = 0; y < 47; y++) {
							for (x = 0; x < 120; x++) {
								if (Percussion[y][x] != PercussionCopy[y][x]) {
									if (IDYES == MessageBox(hwnd, Filename, "Save", MB_YESNO))
										Save();
									goto endo;
								}
							}
						}
					}
					else if (GetSaveFileName(&ofn))
						Save();
					goto endo;
				}
			}
		}
endo:	PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}
