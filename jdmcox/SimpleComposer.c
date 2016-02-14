#include <windows.h>
#include <mmsystem.h>//add winmm.lib to Project -Settings -Link
#include "resource.h"
#define WM_USER2 WM_USER+1
#define WM_USER3 WM_USER2+1
#define IDM_DEVICE 0x100
#define TOTAL_BLACK_KEYS 25
#define NOTELOCS 0x7FFF
#define EVENTSIZE 0x7FFF
#define TIMER_RESOLUTION 2

char szAppName[] = "SimpleComposer 1.12";
char About[] = "by Doug Cox\nusing an M-Audio KeyRig 49\n\nhttp://jdmcox.com/";

unsigned int uTimerID, time;
WORD MidiFormat, MidiTracks, tracks, TicksPerBeat, ticksinnote, SpaceAtEndOfNote = 10, extraticks;
BYTE MetaEventType, Channel, MidiNote, MidiVelocity;
BYTE Controller, ControllerValue, MidiInstrument, Aftertouch, PitchBlend1, PitchBlend2, ChannelEvent;
BYTE TimeNumerator, TimeDenominator, MetronomeTicksPerBeat;
BYTE Powers[] = {1, 2, 4, 8, 16, 32};
BYTE Midi[102400];
BYTE notetype, insertype;
BYTE ch;
double dMilliSecondsPerTick;
char restType, KeySignature = 0, MajorMinor = 0;
char NoteType[4], tempNoteType[4];
char Maestro[] = "Maestro";
char Staff[256];
int staffWidth[1];
int StaffWidth, NumOfStaffs, NotesOnStaff;
int BlackKeyNotes[TOTAL_BLACK_KEYS] = {37,39,42,44,46,49,51,54,56,58,61,63,66,68,70,73,75,78,80,82,85,87,90,92,94};
//                C       D       E   F       G       A       B   C       D       E   F       G       A       B   C       D       E   F       G       A       B   C       D       E   F       G       A       B   C       D       E   F       G       A       B   C
//               36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96 97,98,99,100,101
int NoteLoc[] = {98, 98, 94, 94, 90, 86, 86, 82, 82, 78, 78, 74, 70, 70, 66, 66, 62, 58, 58, 54, 54, 50, 50, 46, 42, 42, 38, 38, 34, 30, 30, 26, 26, 22, 22, 18, 14, 14, 10, 10,  6,  2,  2, -2, -2, -6, -6,-10,-14,-14,-18,-18,-22,-26,-26,-30,-30,-34,-34,-38,-42,42,42,42};

struct {
	int note;
	int velocity;
	int xLoc;
	int yLoc;
	int y;//note position on staff+yLoc (see NoteLoc)
	DWORD ticksinnote;
	BYTE notetype[4];//[0]accidental, [1]type of note, [2]dot, [3]fakenote if on stem
} NoteLocs[NOTELOCS], tempNoteLocs;
int n = 0, CurrentX, NoteLocsY;

struct EVENT {
	DWORD tickptr;
	DWORD ticksinnote;
	DWORD message;
	DWORD xLoc;
	DWORD yLoc;
	double dMilliSecondsPerTick;
	BYTE note;
	BYTE velocity;
} Event[EVENTSIZE], EventTemp;
WORD e, timePtr, ptr;

struct {
	DWORD tickptr;
	WORD xOn;
	BYTE note;
} OnOff[64];
int on;

BYTE Note = 0, Velocity;
int v, w, x, y, z, Instrument = 0, Volume = 127, insertNote, xLoc, yLoc = 0, LastNote, extraX;
int MidiDeviceOut = MIDIMAPPER;//-1 (pointer to driver for default Windows MIDI sounds)
int mSecondsPerBeat, BeatsPerMinute = 120, insertedN, PageBegin = 0, lastRowLoc, Rows, iNumDevs, middle;
int NotesOnStaffAccidentals[NOTELOCS], HorizontalHilight = 0, saveX, highest, lowest, h, l, fakenote;
DWORD i, fileSize, dwBytesRead, dwBytesWritten;
DWORD Time[2] = {0,0};
DWORD TicksPerSecond, TrackLen, TrackBegin, DeltaTicks, MetaEventLen, TotalTicks, LastTickptr, LastTicksinnote = 0;
DWORD X, Y, Y2;
char Play[] = "&PLAY";
char Stop[] = "STO&P";
char ActiveKeyboard[] = "Un-link &Keyboard";
char InactiveKeyboard[] = "Link &Keyboard";
char UsingSharps[] = "Make Flats &Default";
char UsingFlats[] = "Make Sharps &Default";
char Filename[MAX_PATH];
char FullFilename[MAX_PATH];
char Editing[] = "\
A MIDI keyboard is needed to enter notes.\n\
\n\
When you play a tune, only quarter notes\n\
will initially be shown on the staffs.\n\
\n\
To edit, use the Arrow keys to move the highlight.\n\
(as well as Home, PageDown, PageUp, and End).\n\
\n\
Change the type of note by pressing\n\
   T for a Thirtysecond note\n\
   S for a Sixteenth note\n\
   E for an Eighth note\n\
   Q for a Quarter note\n\
   H for a Half note\n\
   W for a Whole note\n\
   . for a dotted note.\n\
   Press T, S, E, Q, H, or W again to remove a dot.\n\
\n\
   Press Insert to insert a quarter rest.\n\
   Press T, S, E, Q, H, or W to change the type of rest.\n\
   Playing a note/notes at a rest will replace that rest.\n\
   Press Delete to delete a note or rest.\n\
\n\
After you've written a tune, select PLAY (or press\n\
the Space bar) to either play or stop playing it.\n\
\n\
You can only edit music written with this program,\n\
but you can open and play any MIDI music file with it,\n\
and you can play MIDI music created with this program\n\
in any other MIDI music player.\
";

BOOL first = TRUE, midi_in = FALSE, keyboardactive = TRUE, usingsharp = TRUE;
BOOL playing = FALSE, fromuser2 = FALSE, firstime;
HWND hwnd;
HMENU hMenu, hMenuPopup;
HANDLE hFile;
RECT rect;
HDC hdc, hdcMem;
HBITMAP hBitmap;
PAINTSTRUCT ps;
HBRUSH hBrush;
HFONT hMaestroFont, hOldFont;
HPEN hPen, hHorizontalPen, hOldPen;
HCURSOR hCursor, hWaitingCursor;
LOGFONT lf;
OPENFILENAME ofn;
HMIDIOUT hMidiOut;
HMIDIIN hMidiIn;
MIDIOUTCAPS moc;

void ReadMidi(void);
BOOL WriteMidi(void);
void ShowNotes(void);
void ShowSignature(void);
void DrawSharpLines(void);
void DrawFlatLines(void);
void DrawLines(void);
void ShowNaturalFlatSharp(int);
void Uncheck(UINT);
void Uncheck2(UINT);
void ChangeNote(BYTE);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	hBrush = GetStockObject(WHITE_BRUSH);

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = NULL;//LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = hBrush;
	wndclass.lpszMenuName  = "MENU";
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

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

void CALLBACK MidiInProc(HMIDIIN hMidiIn, WORD wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{//from midiInOpen (if a MIDI keyboard is attached)
	if (wMsg == MIM_DATA)
		PostMessage(hwnd, WM_USER, (WPARAM)dwParam1 & 0xFF, (LPARAM)(dwParam1 >> 8) & 0xFFFF);//dwParam1 contains velocity, note, and status bytes
}

int compare(const void *x, const void *y)
{//for qsort
	if (((struct EVENT*)x)->tickptr >= ((struct EVENT*)y)->tickptr)
		return 1;
	else if (((struct EVENT*)y)->tickptr >= ((struct EVENT*)x)->tickptr)
		return -1;
	else
		return 0;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		hCursor = LoadCursor(NULL, IDC_ARROW);
		SetCursor(hCursor);
		hWaitingCursor = LoadCursor(NULL, IDC_WAIT);
		hMenu = GetMenu(hwnd);
		Midi[0] = 0;//flag to only do certain tasks with music created in this program
		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.hwndOwner         = hwnd;
		ofn.hInstance         = NULL;
		ofn.lpstrFilter       = " *.mid\0*.mid\0\0";
		ofn.lpstrFile         = FullFilename;
		ofn.lpstrFileTitle    = Filename;
		ofn.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR;
		ofn.lpstrTitle        = NULL;
		ofn.lpstrDefExt       = "mid";
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

		lf.lfHeight = -32;
		lf.lfWeight = 400;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 2;
		lf.lfOutPrecision = 3;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x02;
		for (x = 0; Maestro[x] != 0; x++)
			lf.lfFaceName[x] = Maestro[x];
		lf.lfFaceName[x] = 0;
		hMaestroFont = CreateFontIndirect(&lf);
		if (hMaestroFont == NULL) {
			MessageBox(hwnd, "Maestro font not found!", "This won't work!", MB_OK);
			DestroyWindow(hwnd);
			return 0;
		}
		for (x = 0; x < NOTELOCS; x++) {
			NoteLocs[x].note = 0;
			NoteLocs[x].velocity = 0;
			NoteLocs[x].xLoc = 0;//flag
			NoteLocs[x].yLoc = 0;
			NoteLocs[x].y = 0;
			NoteLocs[x].notetype[0] = ' ';
			NoteLocs[x].notetype[1] = 0;
			NoteLocs[x].notetype[2] = ' ';
			NoteLocs[x].notetype[3] = 0;
		}

		if (MMSYSERR_NOERROR == midiOutOpen(&hMidiOut, MidiDeviceOut, 0, 0, 0)) {
			midiOutShortMsg(hMidiOut, 0x0C0 | (Instrument << 8));//channel 0 and piano (instrument 0)
		}
		if (MMSYSERR_NOERROR == midiInOpen((LPHMIDIIN)&hMidiIn, 0, (DWORD)MidiInProc, 0, CALLBACK_FUNCTION)) {
			midi_in = TRUE;
//			midiConnect((HMIDI)hMidiIn, (HMIDIOUT)hMidiOut, NULL);//THRU MIDI
			midiInStart(hMidiIn);
		}
		return 0;

	case WM_SIZE:
		if (first == FALSE) {
			rect.left = rect.top = 0;
			rect.right = LOWORD(lParam);
			rect.bottom = HIWORD(lParam);
			NumOfStaffs = (rect.right-5) / (StaffWidth);
			for (x = 0; x < NumOfStaffs; x++)
				Staff[x] = '=';
			Staff[x] = 0;
		}
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILES_OPEN:
			if (GetOpenFileName(&ofn)) {
				hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					if (fileSize = GetFileSize(hFile, NULL)) {
						SetWindowText(hwnd, Filename);
						ReadFile(hFile, Midi, fileSize, &dwBytesRead, NULL);
						CloseHandle(hFile);
						playing = TRUE;
						ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Stop);
						DrawMenuBar(hwnd);
						ReadMidi();
					}
					else
						CloseHandle(hFile);
				}
			}
			break;

		case ID_FILES_SAVE:
			if ((Midi[0] == 0) && (NoteLocs[0].xLoc) && (GetSaveFileName(&ofn))) {
				if (WriteMidi()) {
					hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
					WriteFile(hFile, Midi, i, &dwBytesWritten, NULL);
					CloseHandle(hFile);
				}
			}
			break;

		case ID_FILES_EXIT:
			DestroyWindow(hwnd);
			break;

		case ID_EDIT:
			MessageBox(hwnd, Editing, szAppName, MB_OK);
			break;

		case FILES_NEW:
		case ID_NEW:
			if (!playing) {
				SetWindowText(hwnd, szAppName);
				for (x = 0; x < n; x++) {
					NoteLocs[x].xLoc = 0;
					NoteLocs[x].y = 0;
					NoteLocs[x].yLoc = 0;
					NoteLocs[x].notetype[0] = ' ';
					NoteLocs[x].notetype[1] = 0;
					NoteLocs[x].notetype[2] = ' ';
					NoteLocs[x].notetype[3] = 0;
				}
				n = 0;
				Note = 0;
				xLoc = 96 - StaffWidth;
				yLoc = 0;
				HorizontalHilight = 0;
				Time[1] = 0;
				Midi[0] = 0;
				PageBegin = 0;
				InvalidateRect(hwnd, &rect, FALSE);
				UpdateWindow(hwnd);
			}
			break;

		case PLAY:
			if (playing) {
				PostMessage(hwnd, WM_USER3, 0, 0);
			}
			else if (n) {
				playing = TRUE;
				ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Stop);
				DrawMenuBar(hwnd);
				if (WriteMidi())
					ReadMidi();
			}
			break;

		case ID_VOLUME_128:
			Volume = 127;
			Uncheck(ID_VOLUME_128);
			CheckMenuItem(hMenu, ID_VOLUME_128, MF_CHECKED);
			break;
		case ID_VOLUME_112:
			Volume = 111;
			Uncheck(ID_VOLUME_112);
			CheckMenuItem(hMenu, ID_VOLUME_112, MF_CHECKED);
			break;
		case ID_VOLUME_96:
			Volume = 95;
			Uncheck(ID_VOLUME_96);
			CheckMenuItem(hMenu, ID_VOLUME_96, MF_CHECKED);
			break;
		case ID_VOLUME_80:
			Volume = 79;
			Uncheck(ID_VOLUME_80);
			CheckMenuItem(hMenu, ID_VOLUME_80, MF_CHECKED);
			break;
		case ID_VOLUME_64:
			Volume = 63;
			Uncheck(ID_VOLUME_64);
			CheckMenuItem(hMenu, ID_VOLUME_64, MF_CHECKED);
			break;
		case ID_VOLUME_48:
			Volume = 47;
			Uncheck(ID_VOLUME_48);
			CheckMenuItem(hMenu, ID_VOLUME_48, MF_CHECKED);
			break;
		case ID_VOLUME_32:
			Volume = 31;
			Uncheck(ID_VOLUME_32);
			CheckMenuItem(hMenu, ID_VOLUME_32, MF_CHECKED);
			break;
		case ID_VOLUME_16:
			Volume = 15;
			Uncheck(ID_VOLUME_16);
			CheckMenuItem(hMenu, ID_VOLUME_16, MF_CHECKED);
			break;

		case ID_TEMPO_220:
			BeatsPerMinute = 220;
			Uncheck2(ID_TEMPO_220);
			CheckMenuItem(hMenu, ID_TEMPO_220, MF_CHECKED);
			break;
		case ID_TEMPO_200:
			BeatsPerMinute = 200;
			Uncheck2(ID_TEMPO_200);
			CheckMenuItem(hMenu, ID_TEMPO_200, MF_CHECKED);
			break;
		case ID_TEMPO_180:
			BeatsPerMinute = 180;
			Uncheck2(ID_TEMPO_180);
			CheckMenuItem(hMenu, ID_TEMPO_180, MF_CHECKED);
			break;
		case ID_TEMPO_160:
			BeatsPerMinute = 160;
			Uncheck2(ID_TEMPO_160);
			CheckMenuItem(hMenu, ID_TEMPO_160, MF_CHECKED);
			break;
		case ID_TEMPO_140:
			BeatsPerMinute = 140;
			Uncheck2(ID_TEMPO_140);
			CheckMenuItem(hMenu, ID_TEMPO_140, MF_CHECKED);
			break;
		case ID_TEMPO_120:
			BeatsPerMinute = 120;
			Uncheck2(ID_TEMPO_120);
			CheckMenuItem(hMenu, ID_TEMPO_120, MF_CHECKED);
			break;
		case ID_TEMPO_100:
			BeatsPerMinute = 100;
			Uncheck2(ID_TEMPO_100);
			CheckMenuItem(hMenu, ID_TEMPO_100, MF_CHECKED);
			break;
		case ID_TEMPO_80:
			BeatsPerMinute = 80;
			Uncheck2(ID_TEMPO_80);
			CheckMenuItem(hMenu, ID_TEMPO_80, MF_CHECKED);
			break;

		case KEYBOARD:
			if (!playing) {
				if (keyboardactive) {
					keyboardactive = FALSE;
					ModifyMenu(hMenu, KEYBOARD, MF_BYCOMMAND|MF_STRING, KEYBOARD, InactiveKeyboard);
				}
				else {
					keyboardactive = TRUE;
					ModifyMenu(hMenu, KEYBOARD, MF_BYCOMMAND|MF_STRING, KEYBOARD, ActiveKeyboard);
				}
				DrawMenuBar(hwnd);
			}
			break;

		case ACCIDENTALS:
			if (usingsharp) {
				usingsharp = FALSE;
				ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingFlats);
			}
			else {
				usingsharp = TRUE;
				ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingSharps);
			}
			DrawMenuBar(hwnd);
			break;

		case ID_KEYSIGNATURE_NOSHARPSORFLATS:
			KeySignature = 0;
			Note = 0;
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_1FLAT:
			KeySignature = -1;
			Note = 0;
			usingsharp = FALSE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingFlats);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_2FLATS:
			KeySignature = -2;
			Note = 0;
			usingsharp = FALSE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingFlats);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_3FLATS:
			KeySignature = -3;
			Note = 0;
			usingsharp = FALSE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingFlats);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_4FLATS:
			KeySignature = -4;
			Note = 0;
			usingsharp = FALSE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingFlats);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_5FLATS:
			KeySignature = -5;
			Note = 0;
			usingsharp = FALSE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingFlats);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_6FLATS:
			KeySignature = -6;
			Note = 0;
			usingsharp = FALSE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingFlats);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_1SHARP:
			KeySignature = 1;
			Note = 0;
			usingsharp = TRUE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingSharps);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_2SHARPS:
			KeySignature = 2;
			Note = 0;
			usingsharp = TRUE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingSharps);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_3SHARPS:
			KeySignature = 3;
			Note = 0;
			usingsharp = TRUE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingSharps);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_4SHARPS:
			KeySignature = 4;
			Note = 0;
			usingsharp = TRUE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingSharps);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_5SHARPS:
			KeySignature = 5;
			Note = 0;
			usingsharp = TRUE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingSharps);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case ID_KEYSIGNATURE_6SHARPS:
			KeySignature = 6;
			Note = 0;
			usingsharp = TRUE;
			ModifyMenu(hMenu, ACCIDENTALS, MF_BYCOMMAND|MF_STRING, ACCIDENTALS, UsingSharps);
			DrawMenuBar(hwnd);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case ID_TRANSPOSEUP:
			for (x = 0; x < n; x++) {//check for upper limit
				if ((NoteLocs[x].y - NoteLocs[x].yLoc) <= -42)
					break;
			}
			if (x == n) {
				for (x = 0; x < n; x++) {
					NoteLocs[x].note++;
					if (usingsharp) {
						if (NoteLocs[x].notetype[0] == ' ') {
							for (y = 0; y < 96; y++) {
								if (NoteLocs[x].y-NoteLocs[x].yLoc == NoteLoc[y]) {//found it in NoteLoc
									if (NoteLoc[y] != NoteLoc[y+1])
										NoteLocs[x].y -= 4;//single interval
									else
										NoteLocs[x].notetype[0] = '#';//double interval
									break;
								}
							}
						}
						else {//if (NoteLocs[x].notetype[0] == '#')
							NoteLocs[x].y -= 4;
							NoteLocs[x].notetype[0] = ' ';
						}
					}
					else {//if (usingsharp == FALSE)
						if (NoteLocs[x].notetype[0] == ' ') {
							for (y = 0; y < 96; y++) {
								if (NoteLocs[x].y-NoteLocs[x].yLoc == NoteLoc[y]) {//found it in NoteLoc
									if (NoteLoc[y] == NoteLoc[y+1])
										NoteLocs[x].notetype[0] = 'b';//double interval
									else
										NoteLocs[x].y -= 4;//single interval
									break;
								}
							}
						}
						else {//if (NoteLocs[x].notetype[0] == 'b')
							NoteLocs[x].y -= 4;
							NoteLocs[x].notetype[0] = ' ';
						}
					}
				}
				Note = 0;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else
				MessageBox(hwnd, "A note is at the upper limit.", "", MB_OK);
			break;

		case ID_TRANSPOSEDOWN:
			for (x = 0; x < n; x++) {//check for lower limit
				if (((NoteLocs[x].y - NoteLocs[x].yLoc) >= 98) && (NoteLocs[x].notetype[0] == ' '))
					break;
			}
			if (x == n) {
				for (x = 0; x < n; x++) {
					NoteLocs[x].note--;
					if (usingsharp) {
						if (NoteLocs[x].notetype[0] == ' ') {
							for (y = 0; y < 96; y++) {
								if (NoteLocs[x].y-NoteLocs[x].yLoc == NoteLoc[y]) {//found it in NoteLoc
									if (NoteLoc[y-1] == NoteLoc[y-2])//double interval
										NoteLocs[x].notetype[0] = '#';
									NoteLocs[x].y += 4;//single interval
									break;
								}
							}
						}
						else//if (NoteLocs[x].notetype[0] == '#')
							NoteLocs[x].notetype[0] = ' ';
					}
					else {//if (usingsharp == FALSE)
						if (NoteLocs[x].notetype[0] == ' ') {
							for (y = 0; y < 96; y++) {
								if (NoteLocs[x].y-NoteLocs[x].yLoc == NoteLoc[y]) {//found it in NoteLoc
									if (NoteLoc[y-1] == NoteLoc[y-2])
										NoteLocs[x].notetype[0] = 'b';//double interval
									NoteLocs[x].y += 4;//single interval
									break;
								}
							}
						}
						else//if (NoteLocs[x].notetype[0] == 'b')
							NoteLocs[x].notetype[0] = ' ';
					}
				}
				Note = 0;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else
				MessageBox(hwnd, "A note is at the lower limit.", "", MB_OK);
			break;

		case HELP_USING:
			MessageBox(hwnd, Editing, szAppName, MB_OK);
			break;
		case ID_HELP_ABOUT:
			MessageBox(hwnd, About, szAppName, MB_OK);
			break;
		}
		return 0;

	case WM_CHAR:
		if ((wParam == '.') && (Midi[0] == 0)) {
			ch = NoteLocs[x].notetype[1];
			if ((ch != 0xCE) && (ch != 0xB7) && (ch != 0xEE) && (ch != 0xE4) && (ch != 0xC5) && (ch != 0xA8)) {
				if (HorizontalHilight) {
					for (x = 0; x < n; x++) {
						if (((xLoc+StaffWidth) == NoteLocs[CurrentX].xLoc) && (yLoc == NoteLocs[CurrentX].yLoc)) {
							insertedN = CurrentX;
							break;
						}
					}
				}
				else for (x = 0; x < n; x++) {
					if (((xLoc+StaffWidth) == NoteLocs[x].xLoc) && (yLoc == NoteLocs[x].yLoc)) {
						insertedN = x;
						break;
					}
				}
				ChangeNote('k');
			}
		}
		return 0;

	case WM_KEYDOWN:
		insertype = 0;
		if ((wParam == VK_ESCAPE)  && (uTimerID)) {
			PostMessage(hwnd, WM_USER3, 0, 0);//stop playing
		}

		else if (wParam == VK_SPACE) {
			SendMessage(hwnd, WM_COMMAND, PLAY, 0);
		}

		else if ((wParam == VK_DELETE) && (Midi[0] == 0)) {
			for (x = 0; x < n; x++) {
				if ((NoteLocs[x].xLoc == (xLoc + StaffWidth)) && (NoteLocs[x].yLoc == yLoc)) {//get NoteLoc with current xLoc & yLoc
					saveX = x;
					if (HorizontalHilight) {
						if (NoteLocs[CurrentX].notetype[3] == NoteLocs[CurrentX].notetype[1]) {
							if (NoteLocs[CurrentX].notetype[1] & 0x20)//if it's a lower case letter
								NoteLocs[CurrentX+1].notetype[3] = NoteLocs[CurrentX].notetype[1];
							else
								NoteLocs[CurrentX-1].notetype[3] = NoteLocs[CurrentX].notetype[1];
						}
						for (x = CurrentX, y = CurrentX+1; x < n; x++, y++)
							NoteLocs[x] = NoteLocs[y];
					}
					else {
						for (y = x+1; x < n; x++, y++) {
							if (NoteLocs[y].xLoc != 96)
								NoteLocs[y].xLoc -= StaffWidth;
							else {
								NoteLocs[y].xLoc = LastNote;
								NoteLocs[y].yLoc -= 160;
								NoteLocs[y].y -= 160;
							}
							NoteLocs[x] = NoteLocs[y];
						}
					}
					n--;
					break;
				}
			}
			HorizontalHilight = 0;//a single note
			for (x = saveX; x < n; x++) {
				if ((NoteLocs[x].xLoc == xLoc+StaffWidth) && (NoteLocs[x].yLoc == yLoc)) {
					if (NoteLocs[x].xLoc == NoteLocs[x+1].xLoc) {
						HorizontalHilight = NoteLocs[x].y - PageBegin;
						if (NoteLocs[x].notetype[0] == 'b')
							HorizontalHilight -= 4;
						CurrentX = x;
						for (y = x+1; y < n; y++) {
							if (HorizontalHilight > (NoteLocs[y].y - PageBegin)) {
								HorizontalHilight = NoteLocs[y].y - PageBegin;
								if (NoteLocs[y].notetype[0] == 'b')
									HorizontalHilight -= 4;
								CurrentX = y;
							}
							if (NoteLocs[y].xLoc != NoteLocs[y+1].xLoc)
								break;
						}
						x = y;
					}
				}
			}
			Note = 0;
			InvalidateRect(hwnd, &rect, FALSE);
		}

		else if ((wParam == VK_INSERT) && (Midi[0] == 0)) {
			for (x = 0; x < n; x++) {
				if ((NoteLocs[x].xLoc == (xLoc + StaffWidth)) && (NoteLocs[x].yLoc == yLoc)) {
					if ((NoteLocs[x].notetype[1] != 0xCE) && (NoteLocs[x-1].notetype[1] != 0xCE)) {//quarter rest
						for (w = n-1, v = n; w >= x; w--, v--) {// and make a space just before it
							if (NoteLocs[w].xLoc < LastNote) {
								NoteLocs[v].xLoc = NoteLocs[w].xLoc + StaffWidth;
								NoteLocs[v].yLoc = NoteLocs[w].yLoc;
								NoteLocs[v].y = NoteLocs[w].y;
							}
							else {

								NoteLocs[v].xLoc = 96;
								NoteLocs[v].yLoc = NoteLocs[w].yLoc + 160;
								NoteLocs[v].y = NoteLocs[w].y + 160;
							}
							NoteLocs[v].note = NoteLocs[w].note;
							NoteLocs[v].velocity = NoteLocs[w].velocity;
							NoteLocs[v].notetype[0] = NoteLocs[w].notetype[0];
							NoteLocs[v].notetype[1] = NoteLocs[w].notetype[1];
							NoteLocs[v].notetype[2] = NoteLocs[w].notetype[2];
							NoteLocs[v].notetype[3] = NoteLocs[w].notetype[3];
						}
						NoteLocs[x].note = 0;
						NoteLocs[x].velocity = 0;
						NoteLocs[x].notetype[0] = ' ';
						NoteLocs[x].notetype[1] = 0xCE;//quarter rest
						NoteLocs[x].notetype[3] = 0;
						NoteLocs[x].y = yLoc + 37;
						n++;
						HorizontalHilight = 0;//a single note
						InvalidateRect(hwnd, &rect, FALSE);
					}
					break;
				}
			}
		}

		else if ((wParam == VK_PRIOR) && (PageBegin) && (!playing)) {
			xLoc = 96 - StaffWidth;
			yLoc -= PageBegin;
			PageBegin -= lastRowLoc;
			Note = 0;
			InvalidateRect(hwnd, &rect, FALSE);
		}
		else if ((wParam == VK_NEXT) && (!playing)) {
			if (NoteLocs[n-1].yLoc >= PageBegin + lastRowLoc) {
				PageBegin += lastRowLoc;
				xLoc = 96 - StaffWidth;
				yLoc = PageBegin;
				Note = 0;
				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		else if ((wParam == VK_HOME) && (!playing)) {
			xLoc = 96 - StaffWidth;
			yLoc = 0;
			if (NoteLocs[1].xLoc == NoteLocs[0].xLoc) {//more than one note on a stem
				HorizontalHilight = NoteLocs[0].y;
				CurrentX = 0;
			}
			PageBegin = 0;
			Note = 0;
			InvalidateRect(hwnd, &rect, FALSE);
		}

		else if ((wParam == VK_END) && (!playing)) {
			xLoc = NoteLocs[n-1].xLoc;
			yLoc = NoteLocs[n-1].yLoc;
			if (xLoc == (NotesOnStaff+1) * StaffWidth) {
				xLoc = 96 - StaffWidth;
				yLoc += 160;
			}
			PageBegin = yLoc - (yLoc % lastRowLoc);
			HorizontalHilight = 0;
			Note = 0;
			InvalidateRect(hwnd, &rect, FALSE);
		}

		else if ((wParam == VK_LEFT) && (!playing) && (Midi[0] == 0)) {
			if (xLoc > (96 - StaffWidth)) {
				HorizontalHilight = 0;//a single note
				for (x = n-1; x >= 0; x--) {
					if ((NoteLocs[x].xLoc == xLoc) && (NoteLocs[x].yLoc == yLoc)) {
						if (NoteLocs[x].xLoc == NoteLocs[x-1].xLoc) {
							HorizontalHilight = NoteLocs[x].y - PageBegin;
							if (NoteLocs[x].notetype[0] == 'b')
								HorizontalHilight -= 4;
							CurrentX = x;
							for (y = x-1; y >= 0; y--) {
								if (HorizontalHilight > NoteLocs[y].y - PageBegin) {
									HorizontalHilight = NoteLocs[y].y - PageBegin;
									if (NoteLocs[y].notetype[0] == 'b')
										HorizontalHilight -= 4;
									CurrentX = y;
								}
								if (NoteLocs[y].xLoc != NoteLocs[y-1].xLoc)
									break;
							}
							x = y;
						}
					}
				}
			}
			if ((xLoc == (96 - StaffWidth)) && (yLoc)) {
				xLoc = LastNote - StaffWidth;
				if (yLoc % lastRowLoc == 0)
					PageBegin -= lastRowLoc;
				yLoc -= 160;
				HorizontalHilight = 0;
			}
			else if (xLoc > (96 - StaffWidth))
				xLoc -= StaffWidth;
			Note = 0;
			InvalidateRect(hwnd, &rect, FALSE);
		}
		else if ((wParam == VK_RIGHT) && (!playing) && (Midi[0] == 0)) {
			if ((yLoc < NoteLocs[n-1].yLoc) || ((xLoc < NoteLocs[n-1].xLoc) && (yLoc == NoteLocs[n-1].yLoc))) {//can't go beyond space beyond last note
				if (xLoc == (NotesOnStaff+1) * StaffWidth) {
					xLoc = 96 - StaffWidth;
					yLoc += 160;
					if (yLoc % lastRowLoc == 0)
						PageBegin += lastRowLoc;
				}
				else if (xLoc < LastNote)
					xLoc += StaffWidth;
				HorizontalHilight = 0;//a single note
				for (x = 0; x < n; x++) {
					if ((NoteLocs[x].xLoc == xLoc+StaffWidth) && (NoteLocs[x].yLoc == yLoc)) {
						if (NoteLocs[x].xLoc == NoteLocs[x+1].xLoc) {
							HorizontalHilight = NoteLocs[x].y - PageBegin;
							if (NoteLocs[x].notetype[0] == 'b')
								HorizontalHilight -= 4;
							CurrentX = x;
							for (y = x+1; y < n; y++) {
								if (HorizontalHilight > (NoteLocs[y].y - PageBegin)) {
									HorizontalHilight = NoteLocs[y].y - PageBegin;
									if (NoteLocs[y].notetype[0] == 'b')
										HorizontalHilight -= 4;
									CurrentX = y;
								}
								if (NoteLocs[y].xLoc != NoteLocs[y+1].xLoc)
									break;
							}
							x = y;
						}
					}
				}
				Note = 0;
				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		else if ((wParam == VK_DOWN) && (!playing) && (Midi[0] == 0)) {
			if ((HorizontalHilight) && (NoteLocs[CurrentX].xLoc == NoteLocs[CurrentX+1].xLoc)) {
				CurrentX++;
				HorizontalHilight = NoteLocs[CurrentX].y-PageBegin;
				if (NoteLocs[CurrentX].notetype[0] == 'b')
					HorizontalHilight -= 4;
				Note = 0;
				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		else if ((wParam == VK_UP) && (!playing) && (Midi[0] == 0)) {
			if ((HorizontalHilight) && (NoteLocs[CurrentX].xLoc == NoteLocs[CurrentX-1].xLoc)) {
				CurrentX--;
				HorizontalHilight = NoteLocs[CurrentX].y-PageBegin;
				if (NoteLocs[CurrentX].notetype[0] == 'b')
					HorizontalHilight -= 4;
				Note = 0;
				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		else if ((wParam == 'E') && (Midi[0] == 0)) {
			ChangeNote('E');
		}
		else if ((wParam == 'H') && (Midi[0] == 0)) {
			ChangeNote('H');
		}
		else if ((wParam == 'S') && (Midi[0] == 0)) {
			ChangeNote('X');
		}
		else if ((wParam == 'T') && (Midi[0] == 0)) {//thirtysecond note
			ChangeNote('J');
		}
		else if ((wParam == 'W') && (Midi[0] == 0)) {
			ChangeNote('w');
		}
		else if ((wParam == 'Q') && (Midi[0] == 0)) {
			ChangeNote('Q');
		}
		return 0;

	case WM_USER://from MIDI piano keyboard
		if ((wParam == 0x90) && (!playing) && ((Midi[0] == 0) || (keyboardactive == FALSE))) {//Note On or Off
			Note = lParam & 0xFF;
			if ((Note < 36) || (Note > 96))
				return 0;
			Velocity = (lParam >> 8) & 0xFF;
			if (Velocity == 0)//0 velocity means end note
				midiOutShortMsg(hMidiOut, 0x090 | (Note << 8));//0 Velocity
			else {//Note On
				if (Volume != 127)
					Velocity = Volume;
				else {
					Velocity += 30;//because the M-Audio MIDI keyboard doesn't easily play at full volume
					if (Velocity > 127)
						Velocity = 127;
				}
				midiOutShortMsg(hMidiOut, 0x090 | (Velocity << 16) | (Note << 8));
				if (keyboardactive) {
					Time[0] = Time[1];
					Time[1] = timeGetTime();
					if (((Time[1] - Time[0]) > 36)) {//not simutaneous notes
						xLoc += StaffWidth;//doing this before InvalidateRect causes complications, but it seems necessary
						if (xLoc > LastNote) {
							xLoc = 96;
							yLoc += 160;
							if (yLoc % lastRowLoc == 0) {
								PageBegin += lastRowLoc;
							}
						}
					}
					insertype = 0;
					insertedN = n;
					for (x = 0; x < n; x++) {
						y = x;//for NoteType[1] = NoteLocs[y].notetype[1]; (below)
						if ((xLoc == NoteLocs[x].xLoc) && (yLoc == NoteLocs[x].yLoc)) {//a note was added to same xLoc as another note (insert it into NoteLocs)
							NoteLocsY = yLoc+NoteLoc[Note-36];
							insertedN = x;

							if (NoteLocs[x].notetype[1] == 0xCE) {//quarter rest
								NoteLocs[x].note = Note;
								NoteLocs[x].y = NoteLocsY;
								if ((Note >= 70) || ((Note < 60) && (Note > 49)))
									NoteLocs[x].notetype[1] = 'Q';//replace rest with note
								else
									NoteLocs[x].notetype[1] = 'q';
							}
							else if (NoteLocs[x].notetype[1] == 0xEE) {//whole rest
								NoteLocs[x].note = Note;
								NoteLocs[x].y = NoteLocsY;
								NoteLocs[x].notetype[1] = 'w';//replace rest with note
							}
							else if (NoteLocs[x].notetype[1] == 0xB7) {//half rest
								NoteLocs[x].note = Note;
								NoteLocs[x].y = NoteLocsY;
								if ((Note >= 70) || ((Note < 60) && (Note > 49)))
									NoteLocs[x].notetype[1] = 'H';//replace rest with note
								else
									NoteLocs[x].notetype[1] = 'h';
							}
							else if (NoteLocs[x].notetype[1] == 0xE4) {//eighth rest
								NoteLocs[x].note = Note;
								NoteLocs[x].y = NoteLocsY;
								if ((Note >= 70) || ((Note < 60) && (Note > 49)))
									NoteLocs[x].notetype[1] = 'E';//replace rest with note
								else
									NoteLocs[x].notetype[1] = 'e';
							}
							else if (NoteLocs[x].notetype[1] == 0xC5) {//sixteenth rest
								NoteLocs[x].note = Note;
								NoteLocs[x].y = NoteLocsY;
								if ((Note >= 70) || ((Note < 60) && (Note > 49)))
									NoteLocs[x].notetype[1] = 'X';//replace rest with note
								else
									NoteLocs[x].notetype[1] = 'x';
							}
							else if (NoteLocs[x].notetype[1] == 0xA8) {//thirtysecond rest
								NoteLocs[x].note = Note;
								NoteLocs[x].y = NoteLocsY;
								if ((Note >= 70) || ((Note < 60) && (Note > 49)))
									NoteLocs[x].notetype[1] = 'J';//replace rest with note
								else
									NoteLocs[x].notetype[1] = 'j';
							}

							else {//insert a note at xLoc, highest note first
								for (y = x; (xLoc == NoteLocs[y].xLoc) && (NoteLocsY > NoteLocs[y].y); y++)
									;//put y just before lower note
								for (w = n, v = n+1; w >= y; w--, v--)
									NoteLocs[v] = NoteLocs[w];//make a space for new NoteLocs
								NoteLocs[y] = NoteLocs[x];
								NoteLocs[y].y = NoteLocsY;// and insert note (xLoc, yLoc, and notetype will be the same as what it's inserted before
								NoteLocs[y].note = Note;
								insertedN = y;
					//insert fakenote info
								if (NoteLocs[y].notetype[1] == 'w')
									fakenote = 0;
								else if ((NoteLocs[y].notetype[1] == 'H') || (NoteLocs[y].notetype[1] == 'h'))
									fakenote = 'H';
								else
									fakenote = 'Q';
								for ( ; NoteLocs[x].xLoc == xLoc; x++) {
									for (y = x+1; (NoteLocs[y].xLoc == xLoc) && ((NoteLocs[y].y - NoteLocs[y-1].y) < 32); y++)
										;
									y--;
									if ((NoteLocs[y].y - yLoc) <= 42)//if bottom note is equalto/higher than middle C
										middle = 18;
									else if ((NoteLocs[x].y - yLoc) > 42)//if top note => middle C
										middle = 66;
									else
										middle = 38;//notes encompass middle C
									if ((middle - (NoteLocs[x].y - yLoc)) > ((NoteLocs[y].y - yLoc) - middle)) {//stem will go down if lowest note is closer to middle B
										for ( ; x < y; x++)
											NoteLocs[x].notetype[3] = fakenote;
										if (fakenote)
											NoteLocs[x].notetype[3] = (NoteLocs[x].notetype[1] & 0xDF);//make it uppercase
									}
									else {//stem will go up if highest note is closer to middle B
										if (fakenote)
											fakenote |= 0x20;//make it lowercase
										NoteLocs[x].notetype[3] = (NoteLocs[x].notetype[1] | 0x20);
										for (x++; x <= y; x++) {
											NoteLocs[x].notetype[3] = fakenote;
										}
									}
									x = y+1;
								}
								n++;
							}//end of else
							NoteType[1] = NoteLocs[insertedN].notetype[1];
							NoteType[3] = NoteLocs[insertedN].notetype[3];
							goto endo;
						}
					}//end of for (x = 0; x < n; x++)
					if (insertedN == n) {//not at same xLoc as another note
						if ((Note >= 70) || ((Note < 60) && (Note > 49)))
							NoteType[1] = 'Q';//equal to or above Bb above middle C, or below middle C and equal to or above D below middle C
						else
							NoteType[1] = 'q';
						NoteType[3] = 0;
					}
endo:				NoteType[2] = ' ';
					NoteLocs[insertedN].xLoc = xLoc;
					NoteLocs[insertedN].y = yLoc+NoteLoc[Note-36];
					NoteLocs[insertedN].yLoc = yLoc;
					for (x = 0; x < TOTAL_BLACK_KEYS; x++) {
						if (Note == BlackKeyNotes[x]) {
							if (usingsharp)
								NoteType[0] = '#';
							else
								NoteType[0] = 'b';
							break;								
						}
					}
					if (x == TOTAL_BLACK_KEYS)
						NoteType[0] = ' ';
					NoteLocs[insertedN].notetype[0] = NoteType[0];
					NoteLocs[insertedN].notetype[1] = NoteType[1];
					NoteLocs[insertedN].notetype[2] = NoteType[2];
					NoteLocs[insertedN].note = Note;
					NoteLocs[insertedN].velocity = Velocity;

					HorizontalHilight = 0;
					InvalidateRect(hwnd, &rect, FALSE);
					UpdateWindow(hwnd);
				}//end of if (keyboardactive)
			}
		}
		else if ((wParam == 0x80) && (!playing) && ((Midi[0] == 0) || (keyboardactive == FALSE)))//Note Off
			midiOutShortMsg(hMidiOut, 0x080 | ((lParam & 0xFF) << 8));
		return 0;

	case WM_USER2://from TimerFunc
		if (firstime) {
			firstime = FALSE;
			SetCursor(hCursor);
		}
		X = Event[wParam].xLoc;
		Y2 = Event[wParam].yLoc;
		if ((Y2) && ((Y2 % lastRowLoc) == 0) && (X == 96) && (Y != Y2)) {//Y and Y2 are only different at the beginning of a page
			PageBegin += lastRowLoc;
		}
		Y = Y2;
		fromuser2 = TRUE;
		InvalidateRect(hwnd, &rect, FALSE);
		return 0;

	case WM_USER3://to stop playing
		if (firstime) {
			firstime = FALSE;
			SetCursor(hCursor);
		}
		timeKillEvent(uTimerID);
		timeEndPeriod(TIMER_RESOLUTION);
		uTimerID = 0;
		timePtr = 0;
		midiOutReset(hMidiOut);
		playing = FALSE;
		if (keyboardactive) {
			keyboardactive = FALSE;
			SendMessage(hwnd, WM_COMMAND, KEYBOARD, 0);
		}
		ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Play);
		DrawMenuBar(hwnd);
		Note = 0;
		yLoc = 0;
		xLoc = 96 - StaffWidth;
		insertype = 0;
		PageBegin = 0;

		HorizontalHilight = 0;//a single note
		for (x = 0; x < n; x++) {
			if ((NoteLocs[x].xLoc == xLoc+StaffWidth) && (NoteLocs[x].yLoc == yLoc)) {
				if (NoteLocs[x].xLoc == NoteLocs[x+1].xLoc) {
					HorizontalHilight = NoteLocs[x].y - PageBegin;
					if (NoteLocs[x].notetype[0] == 'b')
						HorizontalHilight -= 4;
					CurrentX = x;
					for (y = x+1; y < n; y++) {
						if (HorizontalHilight > NoteLocs[y].y - PageBegin) {
							HorizontalHilight = NoteLocs[y].y - PageBegin;
							if (NoteLocs[y].notetype[0] == 'b')
								HorizontalHilight -= 4;
							CurrentX = y;
						}
						if (NoteLocs[y].xLoc != NoteLocs[y+1].xLoc)
							break;
					}
					x = y;
				}
			}
		}
		InvalidateRect(hwnd, &rect, FALSE);
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (first) {
			first = FALSE;
			rect.left = ps.rcPaint.left;
			rect.top = ps.rcPaint.top;
			rect.right = ps.rcPaint.right;
			rect.bottom = ps.rcPaint.bottom;
			hdcMem = CreateCompatibleDC(hdc);
			hBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			SelectObject(hdcMem, hBitmap);
			GetCharWidth32(hdcMem, 61, 61, staffWidth);//61 is '='
			StaffWidth = staffWidth[0]*4;
			xLoc = 96 - StaffWidth;
			NumOfStaffs = (rect.right-5) / (StaffWidth);
			NotesOnStaff = NumOfStaffs - (96 / StaffWidth) - (96 % StaffWidth);
			LastNote = (NumOfStaffs * StaffWidth) - (StaffWidth);
			for (x = 0; x < NumOfStaffs; x++)
				Staff[x] = '=';
			Staff[x] = 0;
			hPen = CreatePen(PS_SOLID, StaffWidth, 0xE0E0E0);//grey
			hHorizontalPen = CreatePen(PS_SOLID, 8, 0xB8B8B8);//grey
		}//end of if (first)
		FillRect(hdcMem, &rect, hBrush);
		if ((fromuser2)) {
			fromuser2 = FALSE;
			MoveToEx(hdcMem, X + 7, Y - PageBegin + 16, NULL);
			LineTo(hdcMem, X + 7, Y - PageBegin + 140);//vertical line
		}
		if ((Midi[0] == 0) && (!playing)) {
			hOldPen = SelectObject(hdcMem, hPen);
			MoveToEx(hdcMem, xLoc+StaffWidth+14, yLoc - PageBegin + 16, NULL);
			LineTo(hdcMem, xLoc+StaffWidth+14, yLoc - PageBegin + 140);
			if (HorizontalHilight) {
				SelectObject(hdcMem, hHorizontalPen);
				MoveToEx(hdcMem, xLoc+StaffWidth, HorizontalHilight+47, NULL);
				LineTo(hdcMem, xLoc+StaffWidth+26, HorizontalHilight+47);
			}
		}
		SelectObject(hdcMem, hOldPen);
		hOldFont = SelectObject(hdcMem, hMaestroFont);
		SetBkMode(hdcMem, TRANSPARENT);
		for (y = 0; y < (rect.bottom-160); y += 160) {
			TextOut(hdcMem, 5, y+34, Staff, NumOfStaffs);
			TextOut(hdcMem, 5, y+82, Staff, NumOfStaffs);
			TextOut(hdcMem, 6, y+25, "&", 1);
			TextOut(hdcMem, 6, y+57, "?", 1);
			TextOut(hdcMem, 5, y+34, "\\", 1);
			TextOut(hdcMem, 5, y+82, "\\", 1);
			TextOut(hdcMem, 5, y+58, "\\", 1);
			TextOut(hdcMem, 5 + NumOfStaffs*StaffWidth, y+34, "\\", 1);
			TextOut(hdcMem, 5 + NumOfStaffs*StaffWidth, y+82, "\\", 1);
			TextOut(hdcMem, 5 + NumOfStaffs*StaffWidth, y+58, "\\", 1);
		}
		lastRowLoc = y;
		Rows = y / 160;
		ShowSignature();
		////////////
		ShowNotes();
		////////////
		SetBkMode(hdcMem, OPAQUE);
		SelectObject(hdcMem, hOldFont);
		BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
//{ char asdf[10]; _itoa(Note, asdf, 10); TextOut(hdc, 0, 0, asdf, lstrlen(asdf)); }
		EndPaint(hwnd, &ps);
		return 0;


	case WM_DESTROY:
		if (midi_in) {
			midiInStop(hMidiIn);
			midiInReset(hMidiIn);
			midiInClose(hMidiIn);
		}
		midiOutMessage(hMidiOut, 0xB0, 123, 0);//set controller for channel 0 to all voices off
		midiOutReset(hMidiOut);
		midiOutClose(hMidiOut);
		if (timePtr)
			timeEndPeriod(TIMER_RESOLUTION);
		DeleteObject(hMaestroFont);
		DeleteObject(hOldFont);
		DeleteObject(hPen);
		DeleteObject(hOldPen);
		DeleteObject(hBrush);
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

//SUBROUTINES//////////////////////////////////////////////////////////////

void ShowNotes(void)
{
	for (x = 0; x < n; x++) {//re-draw all previous notes
		if ((NoteLocs[x].y-PageBegin+42) < lastRowLoc) {
			tempNoteType[0] = NoteLocs[x].notetype[0];
			tempNoteType[2] = NoteLocs[x].notetype[2];
			tempNoteType[3] = 0;//thirtysecond note flag

			if (NoteLocs[x].notetype[1] == 'j') {
				tempNoteType[3] = 1;//flag
				if ((NoteLocs[x].notetype[3] == 'Q') || (NoteLocs[x].notetype[3] == 'q'))
					tempNoteType[1] = NoteLocs[x].notetype[3];
				else if (NoteLocs[x].notetype[3] == 'j')
					tempNoteType[1] = 'x';//trick
				else if (NoteLocs[x].notetype[3] == 'J')
					tempNoteType[1] = 'X';//trick
				else
					tempNoteType[1] = 'x';
			}
			else if (NoteLocs[x].notetype[1] == 'J') {
				tempNoteType[3] = 1;//flag
				if ((NoteLocs[x].notetype[3] == 'Q') || (NoteLocs[x].notetype[3] == 'q'))
					tempNoteType[1] = NoteLocs[x].notetype[3];
				else if (NoteLocs[x].notetype[3] == 'j')
					tempNoteType[1] = 'x';//trick
				else if (NoteLocs[x].notetype[3] == 'J')
					tempNoteType[1] = 'X';//trick
				else
					tempNoteType[1] = 'X';
			}

			else if (NoteLocs[x].notetype[3])
				tempNoteType[1] = NoteLocs[x].notetype[3];//bug (it's a space)
			else
				tempNoteType[1] = NoteLocs[x].notetype[1];

			if (NoteLocs[x].notetype[0] == '#') {
				if (NoteLocs[x].y == 98+NoteLocs[x].yLoc) {//low C#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin-8, "__", 2);
				}
				else if (NoteLocs[x].y == 94+NoteLocs[x].yLoc)//low D#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin-4, "__", 2);
				else if (NoteLocs[x].y == 42+NoteLocs[x].yLoc)//middle C#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
				else if (NoteLocs[x].y == -6+NoteLocs[x].yLoc)//high A#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
				else if (NoteLocs[x].y == -14+NoteLocs[x].yLoc) {//high C#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
				}
				else if (NoteLocs[x].y == -18+NoteLocs[x].yLoc) {//high D#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
				}
				else if (NoteLocs[x].y == -26+NoteLocs[x].yLoc) {//high F#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+20, "__", 2);
				}
				else if (NoteLocs[x].y == -30+NoteLocs[x].yLoc) {//high G#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+16, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+24, "__", 2);
				}
				else if (NoteLocs[x].y == -34+NoteLocs[x].yLoc) {//high A#
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+20, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+28, "__", 2);
				}
				extraX = 2;
				if (KeySignature) {
					ShowNaturalFlatSharp(NoteLocs[x].note);
					if (tempNoteType[0] == ' ')
						extraX = 7;
				}
				if (tempNoteType[3] == 0) {
					if (NoteLocs[x].notetype[1] != 'w')
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin + 4, tempNoteType, 3);
					else
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin, tempNoteType, 3);
				}
				else {
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin + 4, tempNoteType, 3);
					if (tempNoteType[1] == 'X') {//thirtysecond note
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 9, NoteLocs[x].y - PageBegin + 24, "\x93", 1);
					}
					else if (tempNoteType[1] == 'x') {
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 17, NoteLocs[x].y - PageBegin - 23, "\x91", 1);
					}
				}
			}

			else if (NoteLocs[x].notetype[0] == 'b') {
				if (NoteLocs[x].y == 98+NoteLocs[x].yLoc)//low Db
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin-8, "__", 2);
				else if (NoteLocs[x].y == 94+NoteLocs[x].yLoc)//low Eb
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin-4, "__", 2);
				else if (NoteLocs[x].y == -2+NoteLocs[x].yLoc)//high Ab
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin-4, "__", 2);
				else if (NoteLocs[x].y == -6+NoteLocs[x].yLoc)//high Bb
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
				else if (NoteLocs[x].y == -18+NoteLocs[x].yLoc) {//high Db
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
				}
				else if (NoteLocs[x].y == -22+NoteLocs[x].yLoc) {//high Eb
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+16, "__", 2);
				}
				else if (NoteLocs[x].y == -30+NoteLocs[x].yLoc) {//high Gb
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+16, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+24, "__", 2);
				}
				else if (NoteLocs[x].y == -34+NoteLocs[x].yLoc) {//high Ab
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+20, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+28, "__", 2);
				}
				else if (NoteLocs[x].y == -38+NoteLocs[x].yLoc) {//high Bb
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+16, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+24, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+32, "__", 2);
				}
				extraX = 3;
				if (KeySignature) {
					ShowNaturalFlatSharp(NoteLocs[x].note);
					if (tempNoteType[0] == ' ')
						extraX = 7;
				}
				if (tempNoteType[3] == 0) {
					if (NoteLocs[x].notetype[1] != 'w')
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin, tempNoteType, 3);
					else
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin - 4, tempNoteType, 3);
				}
				else {
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin + 4, tempNoteType, 3);
					if (tempNoteType[1] == 'X') {//thirtysecond note
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 8, NoteLocs[x].y - PageBegin + 24, "\x93", 1);
					}
					else if (tempNoteType[1] == 'x') {
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 16, NoteLocs[x].y - PageBegin - 23, "\x91", 1);
					}
				}
			}

			else {//not sharp or flat
				if (NoteLocs[x].y == 98+NoteLocs[x].yLoc) {//low C
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin-8, "__", 2);
				}
				else if (NoteLocs[x].y == 94+NoteLocs[x].yLoc)//low D
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin-4, "__", 2);
				else if (NoteLocs[x].y == 90+NoteLocs[x].yLoc)//low E
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
				else if ((NoteLocs[x].y == 42+NoteLocs[x].yLoc) && (NoteLocs[x].notetype[0] != 0xCE))//middle C
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
				else if (NoteLocs[x].y == -6+NoteLocs[x].yLoc)//high A
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
				else if (NoteLocs[x].y == -10+NoteLocs[x].yLoc)//high B
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
				else if (NoteLocs[x].y == -14+NoteLocs[x].yLoc) {//high C
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
				}
				else if (NoteLocs[x].y == -18+NoteLocs[x].yLoc) {//high D
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
				}
				else if (NoteLocs[x].y == -22+NoteLocs[x].yLoc) {//high E
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+16, "__", 2);
				}
				else if (NoteLocs[x].y == -26+NoteLocs[x].yLoc) {//high F
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+20, "__", 2);
				}
				else if (NoteLocs[x].y == -30+NoteLocs[x].yLoc) {//high G
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+16, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+24, "__", 2);
				}
				else if (NoteLocs[x].y == -34+NoteLocs[x].yLoc) {//high A
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+20, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+28, "__", 2);
				}
				else if (NoteLocs[x].y == -38+NoteLocs[x].yLoc) {//high B
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+8, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+16, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+24, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+32, "__", 2);
				}
				else if (NoteLocs[x].y == -42+NoteLocs[x].yLoc) {//high C
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+4, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+12, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+20, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+28, "__", 2);
					TextOut(hdcMem, NoteLocs[x].xLoc, NoteLocs[x].y-PageBegin+36, "__", 2);
				}
				extraX = 7;
				if (KeySignature) {
					ShowNaturalFlatSharp(NoteLocs[x].note);
					if (tempNoteType[0] == 'n')
						extraX = 3;
				}
				if (tempNoteType[3] == 0) {
					if (NoteLocs[x].notetype[1] != 'w')//not a whole note
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin + 4, tempNoteType, 3);
					else
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin, tempNoteType, 3);
				}
				else {
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin + 4, tempNoteType, 3);
					if (tempNoteType[1] == 'X') {//trick for thirtysecond note
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 4, NoteLocs[x].y - PageBegin + 24, "\x93", 1);
					}
					else if (tempNoteType[1] == 'x') {
						TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 12, NoteLocs[x].y - PageBegin - 23, "\x91", 1);
					}
				}
			}
		}
	}//end of for (x = 0; x < n; x++)
//currently played Note///////////////////////////////////////////////////////////////////////////////////
	if ((Note >= 36) && (Note <= 96) && (NoteLocs[n].xLoc)) {//NoteLocs[n].xLoc because otherwise WM_PAINT will draw a note at 0 xLoc and yLoc
		if (NoteType[1] == 'j') {
			NoteType[3] = 1;//flag
			if ((NoteType[3] == 'Q') || (NoteType[3] == 'q'))
				NoteType[1] = NoteType[3];
			else if (NoteType[3] == 'j')
				NoteType[1] = 'x';//trick
			else if (NoteType[3] == 'J')
				NoteType[1] = 'X';//trick
			else
				NoteType[1] = 'x';
		}
		else if (NoteType[1] == 'J') {
			NoteType[3] = 1;//flag
			if ((NoteType[3] == 'Q') || (NoteType[3] == 'q'))
				NoteType[1] = NoteType[3];
			else if (NoteType[3] == 'j')
				NoteType[1] = 'x';//trick
			else if (NoteType[3] == 'J')
				NoteType[1] = 'X';//trick
			else
				NoteType[1] = 'X';
		}

		else if (NoteType[3])
			NoteType[1] = NoteType[3];
		else
			NoteType[1] = NoteType[1];

		NoteType[2] = NoteType[2];

		if (NoteType[0] == '#') {
			DrawSharpLines();
			extraX = 2;
			if (KeySignature) {
				ShowNaturalFlatSharp(Note);
				if (NoteType[0] == ' ')
					extraX = 7;
			}
			if (NoteType[1] != 'w')//not a whole note
				TextOut(hdcMem, xLoc + extraX, yLoc - PageBegin + NoteLoc[Note-36] + 4, NoteType, 3);
			else
				TextOut(hdcMem, xLoc + extraX, yLoc - PageBegin + NoteLoc[Note-36], NoteType, 3);
		}
		else if (NoteType[0] == 'b') {
			DrawFlatLines();
			extraX = 3;
			if (KeySignature) {
				ShowNaturalFlatSharp(Note);
				if (NoteType[0] == ' ')
					extraX = 7;
			}
			if (tempNoteType[3] == 0) {
				if (NoteLocs[x].notetype[1] != 'w')
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin, tempNoteType, 3);
				else
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin - 4, tempNoteType, 3);
			}
			else {
				TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin + 4, tempNoteType, 3);
				if (tempNoteType[1] == 'X') {//thirtysecond note
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 8, NoteLocs[x].y - PageBegin + 24, "\x93", 1);
				}
				else if (tempNoteType[1] == 'x') {
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 16, NoteLocs[x].y - PageBegin - 23, "\x91", 1);
				}
			}
		}
		else {
			DrawLines();
			extraX = 7;
			if (KeySignature) {
				ShowNaturalFlatSharp(Note);
				if (NoteType[0] == 'n')
					extraX = 3;
			}
			if (NoteType[3] == 0) {
				if (NoteLocs[x].notetype[1] != 'w')//not a whole note
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin + 4, NoteType, 3);
				else
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin, NoteType, 3);
			}
			else {
				TextOut(hdcMem, NoteLocs[x].xLoc+extraX, NoteLocs[x].y - PageBegin + 4, NoteType, 3);
				if (NoteType[1] == 'X') {//trick for thirtysecond note
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 4, NoteLocs[x].y - PageBegin + 24, "\x93", 1);
				}
				else if (NoteType[1] == 'x') {
					TextOut(hdcMem, NoteLocs[x].xLoc+extraX + 12, NoteLocs[x].y - PageBegin - 23, "\x91", 1);
				}
			}
		}
		if (insertedN == n)
			n++;
	}
}

void ShowSignature(void)
{
	for (y = 0; y < lastRowLoc; y  += 160) {
		switch (KeySignature)
		{
		case -6:
			TextOut(hdcMem, 32+50, y+19-4, "b", 1);
			TextOut(hdcMem, 32+50, y+19-4+56, "b", 1);
		case -5:
			TextOut(hdcMem, 32+40, y+19+8, "b", 1);
			TextOut(hdcMem, 32+40, y+19+8+56, "b", 1);
		case -4:
			TextOut(hdcMem, 32+30, y+19-8, "b", 1);
			TextOut(hdcMem, 32+30, y+19-8+56, "b", 1);
		case -3:
			TextOut(hdcMem, 32+20, y+19+4, "b", 1);
			TextOut(hdcMem, 32+20, y+19+4+56, "b", 1);
		case -2:
			TextOut(hdcMem, 32+10, y+19-12, "b", 1);
			TextOut(hdcMem, 32+10, y+19-12+56, "b", 1);
		case -1:
			TextOut(hdcMem, 32, y+19, "b", 1);
			TextOut(hdcMem, 32, y+19-4+60, "b", 1);
			break;
		case 6:
			TextOut(hdcMem, 32+50, y+1+4, "#", 1);
			TextOut(hdcMem, 32+50, y+2+4+56, "#", 1);
		case 5:
			TextOut(hdcMem, 32+40, y+1+20, "#", 1);
			TextOut(hdcMem, 32+40, y+2+20+56, "#", 1);
		case 4:
			TextOut(hdcMem, 32+30, y+1+8, "#", 1);
			TextOut(hdcMem, 32+30, y+2+8+56, "#", 1);
		case 3:
			TextOut(hdcMem, 32+20, y+1-4, "#", 1);
			TextOut(hdcMem, 32+20, y+1-4+56, "#", 1);
		case 2:
			TextOut(hdcMem, 32+10, y+1+12, "#", 1);
			TextOut(hdcMem, 32+10, y+1+12+56, "#", 1);
		case 1:
			TextOut(hdcMem, 32, y+1, "#", 1);
			TextOut(hdcMem, 32, y+1+56, "#", 1);
			break;
		}
	}
}

void CALLBACK TimerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	do {
		if (Event[timePtr].xLoc) {
			PostMessage(hwnd, WM_USER2, (WPARAM)timePtr, 0);//draw vertical line and scroll screen
		}
		if (Event[timePtr].message) {//send any ChannelEvent message, but not just a flag
			midiOutShortMsg(hMidiOut, Event[timePtr].message);
		}
		else if (Event[timePtr].dMilliSecondsPerTick)
			dMilliSecondsPerTick = Event[timePtr].dMilliSecondsPerTick;
		timePtr++;
	} while ((Event[timePtr].tickptr - Event[timePtr-1].tickptr) < 6);// 6 works...

	if (timePtr < e) {
		time = (int)(dMilliSecondsPerTick * (double)(Event[timePtr].tickptr - Event[timePtr-1].tickptr));
		uTimerID = timeSetEvent(time, TIMER_RESOLUTION, TimerFunc, 0, TIME_ONESHOT);
	}

	else {//done
		PostMessage(hwnd, WM_USER3, 0, 0);
	}
}

void AddRest(unsigned char restType, int y)
{
	xLoc += StaffWidth;//xLoc and yLoc are used temporarily here and then reset in WM_USER3
	if (xLoc > LastNote) {
		xLoc = 96;
		yLoc += 160;
		Event[ptr].xLoc = xLoc;
		Event[ptr].yLoc = yLoc;
		ptr++;
	}
	NoteLocs[n].xLoc = xLoc;
	NoteLocs[n].yLoc = yLoc;
	NoteLocs[n].y = y + yLoc;
	NoteLocs[n].notetype[1] = restType;
	n++;
}

int ReadVLV(void)
{//get integer from Variable Length Value
	DWORD output = 0;

	while (TRUE) {
		output <<= 7;
		output |= (Midi[i] & 0x7F);
		if ((Midi[i] & 0x80))
			i++;
		else
			break;
	}
	i++;
	return output;
}

void ReadMidi(void)
{//1 beat per quarter note
	BOOL firstnote = TRUE;

	for (e = 0; e < EVENTSIZE; e++) {
		Event[e].tickptr = 0;
		Event[e].ticksinnote = 0;
		Event[e].message = 0;//this is only changed with note on or off
		Event[e].xLoc = 0;
		Event[e].yLoc = 0;
		Event[e].dMilliSecondsPerTick = 0;
		Event[e].note = 0;
		Event[e].velocity = 0;
	}
	e = 0;
	for (n = 0; n < NOTELOCS; n++) {
		NoteLocs[n].note = 0;
		NoteLocs[n].xLoc = 0;
		NoteLocs[n].yLoc = 0;
		NoteLocs[n].y = 0;
		NoteLocs[n].notetype[0] = ' ';
		NoteLocs[n].notetype[1] = 0;
		NoteLocs[n].notetype[2] = ' ';
		NoteLocs[n].notetype[3] = 0;
	}
	n = 0;
	Note = 0;
	xLoc = 96;
	yLoc = 0;
	HorizontalHilight = 0;
	Time[1] = 0;
	ChannelEvent = 0;
	TicksPerSecond = 0;
	uTimerID = 0;
	LastTickptr = 0;
	LastTicksinnote = 0;

	if (*(DWORD*)&Midi[0] != 0x6468544D) {//"MThd" - MIDI files are in Big Endian format (not Intel format)
		MessageBox(hwnd, "That's not a MIDI file", ERROR, MB_OK);
		return;
	}
	MidiFormat = (Midi[8] << 8) | Midi[9];
	MidiTracks = (Midi[10] << 8) | Midi[11];
	TicksPerBeat = (Midi[12] << 8) | Midi[13];//not computer ticks! doesn't change!
//	BeatsPerMinute = 120;//default - changed in Meta Event 0x51
//	Event[e].dMilliSecondsPerTick = 60000.0 / (double)(TicksPerBeat * BeatsPerMinute);//default - changed in Meta Event 0x51
//don't do this because it will be sorted along with other tickptr = 0 Events and used out of order
	if (TicksPerBeat & 0x8000)//if hi-bit is set
		TicksPerSecond = (*(BYTE*)&Midi[12] & 0x7FFF) * (*(BYTE*)&Midi[13]);//alternate format
	i = 14;
	for (tracks = 0; tracks < MidiTracks; tracks++) {
		if (*(DWORD*)&Midi[i] != 0x6B72544D) {//"MTrk"
			MessageBox(hwnd, "'MTrk' wasn't where it was supposed to be", ERROR, MB_OK);
			return;
		}
		TotalTicks = 0;
		i += 4;
		TrackLen = ((Midi[i] << 24) | (Midi[i+1] << 16) | (Midi[i+2] << 8) | Midi[i+3]);
		i += 4;
		TrackBegin = i;
		while (i < (TrackBegin + TrackLen)) {
			DeltaTicks = ReadVLV();
			TotalTicks += DeltaTicks;
			Event[e].tickptr = TotalTicks;
			switch (Midi[i] & 0xF0)//status byte without channel info
			{///////////////////////////////0x80 thru 0xE0 are Channel Events
			case 0x80://note off
				Channel = Midi[i++] & 0x0F;
				MidiNote = Midi[i++];
				MidiVelocity = Midi[i++];
				ChannelEvent = 0x80;
				Event[e].note = MidiNote;
				Event[e].velocity = 0;
				Event[e].message = (0x80|Channel) | (MidiVelocity << 16) | (MidiNote << 8);
				break;
			case 0x90://note on (or note off with 0 velocity)
				Channel = Midi[i++] & 0x0F;
				MidiNote = Midi[i++];
				MidiVelocity = Midi[i++];
				ChannelEvent = 0x90;
				if ((MidiVelocity) && (Volume != 127))
					MidiVelocity = Volume;
				Event[e].note = MidiNote;
				Event[e].velocity = MidiVelocity;
				Event[e].message = (0x90|Channel) | (MidiVelocity << 16) | (MidiNote << 8);
				break;
			case 0xA0:
				Channel = Midi[i++] & 0x0F;
				MidiNote = Midi[i++];
				Aftertouch = Midi[i++];
				ChannelEvent = 0xA0;
				Event[e].message = (0xA0|Channel) | (Aftertouch << 16) | (MidiNote << 8);
				break;
			case 0xB0:
				Channel = Midi[i++] & 0x0F;
				Controller = Midi[i++];
				ControllerValue = Midi[i++];
				ChannelEvent = 0xB0;
				Event[e].message = (0xB0|Channel) | (ControllerValue << 16) | (Controller << 8);
				break;
			case 0xC0:
				Channel = Midi[i++] & 0x0F;
				MidiInstrument = Midi[i++];
				ChannelEvent = 0xC0;
				Event[e].message = (0xC0|Channel) | (MidiInstrument << 8);
				break;
			case 0xD0:
				Channel = Midi[i++] & 0x0F;
				Aftertouch = Midi[i++];
				ChannelEvent = 0xD0;
				Event[e].message = (0xD0|Channel) | (Aftertouch << 8);
				break;
			case 0xE0:
				Channel = Midi[i++] & 0x0F;
				PitchBlend1 = Midi[i++];
				PitchBlend2 = Midi[i++];
				ChannelEvent = 0xE0;
				Event[e].message = (0xE0|Channel) | (PitchBlend1 << 16) | (PitchBlend2 << 8);
				break;

			case 0xF0:
				ChannelEvent = 0;//not a ChannelEvent
				if (Midi[i] == 0xFF) {//Meta Events
					i++;//to MetaEventType
					MetaEventType = Midi[i];
					i++;//to MetaEventLen
					switch (MetaEventType)
					{
					case 0:
						i += 3;
						break;
					case 1://Text Event
						MetaEventLen = ReadVLV();
						if (*(DWORD*)&Midi[i] == 0x434D444A)//"JDMC"
							Midi[0] = 0;//flag
						i += MetaEventLen;
						break;
					case 2:
					case 3:
					case 4:
					case 5:
					case 6:
					case 7:
					case 8:
					case 9:
						MetaEventLen = ReadVLV();
						i += MetaEventLen;
						break;
					case 0x20:
					case 0x21:
						i += 2;
						break;
					case 0x2F:
						//not at length, but at 0 (this is track end id)
						i++;
						break;
					case 0x51://tempo
						i++;
						mSecondsPerBeat = (Midi[i] << 16) | (Midi[i+1] << 8) | Midi[i+2];//microseconds/beat
						Event[e].dMilliSecondsPerTick = (((double)mSecondsPerBeat / (double)TicksPerBeat)) / 1000.0;
//						BeatsPerMinute = 60000000 / mSecondsPerBeat;
						i += 3;
						break;
					case 0x54://SMPTE offset
						i += 6;
						break;
					case 0x58://time signature
						i++;
						TimeNumerator = Midi[i++];
						if (Midi[i] > 5)
							MessageBox(hwnd, "?","?", MB_OK);
						TimeDenominator = Powers[Midi[i++]];
						MetronomeTicksPerBeat = 24/Midi[i++];//24 clocks/beat
						i++;//ignore the 4th byte
						break;
					case 0x59://key signature
						i++;//past the number 2
						if (Midi[0] == 0) {//created here
							KeySignature = Midi[i++];
							MajorMinor = Midi[i++];
						}
						else {//don't show key signatures in music not created here (they'll change keys). This is SIMPLEComposer
							KeySignature = 0;
							MajorMinor = 0;
							i += 2;
						}
						break;
					case 0x7F:
						MetaEventLen = ReadVLV();
						i += MetaEventLen;
						break;
					default:
						MessageBox(hwnd, "Unknown Meta Event", "", MB_OK);
					}//end of switch (MetaEventType)
					break;//to get out of switch that this switch is in
				}//end of if (Midi[i] == 0xFF)
				else {//SysEx Events - either 0xF0 or 0xF7
					i += (Midi[i+1] + 2);//skip it
					break;
				}
			default://it must be a shortened Channel Event
				switch (ChannelEvent)
				{
				case 0x80:
					MidiNote = Midi[i++];
					MidiVelocity = Midi[i++];
					Event[e].note = MidiNote;
					Event[e].velocity = 0;
					Event[e].message = (0x80|Channel) | (MidiVelocity << 16) | (MidiNote << 8);
					break;
				case 0x90:
					MidiNote = Midi[i++];
					MidiVelocity = Midi[i++];
					if ((MidiVelocity) && (Volume != 127))
						MidiVelocity = Volume;
					Event[e].note = MidiNote;
					Event[e].velocity = MidiVelocity;
					Event[e].message = (0x90|Channel) | (MidiVelocity << 16) | (MidiNote << 8);
					break;
				case 0xA0:
					MidiNote = Midi[i++];
					Aftertouch = Midi[i++];
					Event[e].message = (0xA0|Channel) | (Aftertouch << 16) | (MidiNote << 8);
					break;
				case 0xB0:
					Controller = Midi[i++];
					ControllerValue = Midi[i++];
					Event[e].message = (0xB0|Channel) | (ControllerValue << 16) | (Controller << 8);
					break;
				case 0xC0:
					MidiInstrument = Midi[i++];
					Event[e].message = (0xC0|Channel) | (MidiInstrument << 8);
					break;
				case 0xD0:
					Aftertouch = Midi[i++];
					Event[e].message = (0xD0|Channel) | (Aftertouch << 8);
					break;
				case 0xE0:
					PitchBlend1 = Midi[i++];
					PitchBlend2 = Midi[i++];
					Event[e].message = (0xE0|Channel) | (PitchBlend1 << 16) | (PitchBlend2 << 8);
					break;
				default:
					MessageBox(hwnd, "Unknown MIDI data", "", MB_OK);
					return;
				}
			}//end of switch (Midi[i] & 0xF0)
			if ((Event[e].message) || (Event[e].dMilliSecondsPerTick))
				e++;
			if (e == EVENTSIZE) {
				MessageBox(hwnd, "MIDI Event array wasn't big enough!", ERROR, MB_OK);
				return;
			}
		}//end of while
	}//end of for (tracks = 0

	qsort(Event, e, sizeof(Event[0]), compare);

	for (on = 0; on < 64; on++) {
		OnOff[on].tickptr = 0;
		OnOff[on].xOn = 0;
		OnOff[on].note = 0;
	}
	on = 0;

	for (x = 0; x < e; x++) {
		if (Event[x].note) {
			if (Event[x].velocity) {
				OnOff[on].note = Event[x].note;
				OnOff[on].tickptr = Event[x].tickptr;
				OnOff[on].xOn = x;
				on++;
			}
			else {//if (Event[x].velocity == 0)
				for (y = 0; y < on; y++) {
					if (OnOff[y].note == Event[x].note) {//first occurrence of this note turned on and not turned off yet
						/////////////////////////////
						Event[OnOff[y].xOn].ticksinnote = Event[x].tickptr - OnOff[y].tickptr;
						/////////////////////////////
						for (z = y+1; z < on; y++, z++) {//clean up OnOff array
							OnOff[y] = OnOff[z];
						}
						on--;
						OnOff[on].note = 0;
						OnOff[on].xOn = 0;
						OnOff[on].tickptr = 0;
						break;
					}
				}
			}
		}
	}

//**fill NoteLocs**********************
	ptr = 0;
	PageBegin = 0;
	for ( ; ptr < e; ptr++) {
		ticksinnote = (WORD)Event[ptr].ticksinnote;
		if (ticksinnote) {
			Note = Event[ptr].note;//(Event[ptr].message >> 8) & 0xFF;
			MidiVelocity = Event[ptr].velocity;//((Event[ptr].message >> 16) & 0xFF);
			NoteType[2] = ' ';//might become a dot

			if (ticksinnote <= (TicksPerBeat >> 3)) {
				notetype = 'J';//flag for 32nd note
			}
			else if (ticksinnote <= (TicksPerBeat >> 2)) {//sixteenth note
				notetype = 'X';//tail down
			}
			else if (ticksinnote <= ((TicksPerBeat >> 2) + (TicksPerBeat >> 3))) {//dotted sixteenth note
				notetype = 'X';//tail down
				NoteType[2] = 'k';//a dot
			}
			else if (ticksinnote <= (TicksPerBeat >> 1)) {//eighth note
				notetype = 'E';//tail down
			}
			else if (ticksinnote <= ((TicksPerBeat >> 1) + (TicksPerBeat >> 2))) {//dotted eighth note
				notetype = 'E';//tail down
				NoteType[2] = 'k';
			}
			else if (ticksinnote <= TicksPerBeat) {//quarter note
				notetype = 'Q';//tail down
			}
			else if (ticksinnote <= (TicksPerBeat + (TicksPerBeat >> 1))) {//dotted quarter note
				notetype = 'Q';//tail down
				NoteType[2] = 'k';
			}
			else if (ticksinnote <= (TicksPerBeat << 1)) {//half note
				notetype = 'H';//tail down
			}
			else if (ticksinnote <= ((TicksPerBeat << 1) + TicksPerBeat)) {//dotted half note
				notetype = 'H';//tail down
				NoteType[2] = 'k';
			}
			else if (ticksinnote <= (TicksPerBeat << 2)) {//whole note
				notetype = 'w';
			}
			else if (ticksinnote <= ((TicksPerBeat << 2) + (TicksPerBeat << 1))) {//dotted whole note
				notetype = 'w';
				NoteType[2] = 'k';
			}
			else
				notetype = '\x44';//Maestro 'unknown'

			if ((Note) && (MidiVelocity))
			{
				if (firstnote) {
					firstnote = FALSE;
					LastTicksinnote = Event[ptr].ticksinnote;
					LastTickptr = Event[ptr].tickptr;
				}
				if ((Event[ptr].tickptr - LastTickptr) >= (DWORD)(TicksPerBeat >> 3)) {//thirtysecond note (if it's not on the same stem)
					if (Event[ptr].tickptr > (LastTickptr + LastTicksinnote)) {
						extraticks = (WORD)(Event[ptr].tickptr - (LastTicksinnote + LastTickptr));
						if (extraticks >= (TicksPerBeat << 2))//whole rest
							AddRest('\xEE', 34);
						else if (extraticks >= (TicksPerBeat << 1))//half rest
							AddRest('\xB7', 49);
						else if (extraticks >= TicksPerBeat)//quarter rest
							AddRest(0xCE, 37);
						else if (extraticks >= (TicksPerBeat >> 1))//eighth rest
							AddRest('\xE4', 37);
						else if (extraticks >= (TicksPerBeat >> 2))//sixteenth rest
							AddRest('\xC5', 37);
						else if (extraticks >= (TicksPerBeat >> 3))//thirtysecond rest
							AddRest('\xA8', 37);
					}
					LastTicksinnote = Event[ptr].ticksinnote;
					LastTickptr = Event[ptr].tickptr;
					xLoc += StaffWidth;//xLoc and yLoc are used temporarily here and then reset in WM_USER3
					if (xLoc > LastNote) {
						xLoc = 96;
						yLoc += 160;
					}
				}
				Event[ptr].xLoc = xLoc;
				Event[ptr].yLoc = yLoc;
			}
			NoteLocs[n].xLoc = xLoc;
			NoteLocs[n].y = yLoc+NoteLoc[Note-36];
			NoteLocs[n].yLoc = yLoc;
			NoteLocs[n].notetype[1] = notetype;
			NoteLocs[n].notetype[2] = NoteType[2];
			for (y = 0; y < TOTAL_BLACK_KEYS; y++) {
				if (BlackKeyNotes[y] == Note) {
					if (usingsharp)
						NoteLocs[n].notetype[0] = '#';
					else//using flats
						NoteLocs[n].notetype[0] = 'b';
					break;
				}
			}
			if (y == TOTAL_BLACK_KEYS) {
				NoteLocs[n].notetype[0] = ' ';
			}
			NoteLocs[n].note = Note;
			NoteLocs[n].velocity = MidiVelocity;
			NoteLocs[n].ticksinnote = ticksinnote;
			n++;
		}//end of if (ticksinnote)
	}//end of for (ptr = 0; ptr < e; ptr++)

	//put info in NoteLocs to show correct stems
	for (x = 0; x < n; x++) {
		if (NoteLocs[x].xLoc != NoteLocs[x+1].xLoc) {
			if ((NoteLocs[x].note >= 70) || ((NoteLocs[x].note < 60) && (NoteLocs[x].note > 49))) {
				if (NoteLocs[x].notetype[1] != 'w')
					NoteLocs[x].notetype[1] &= 0xDF;//make it uppercase
			}
			else if ((NoteLocs[x].note <= 96) && (NoteLocs[x].note >= 36))
				NoteLocs[x].notetype[1] |= 0x20;//make it lowercase
		}

		else {//notes on stem
			for (y = x; (NoteLocs[y].xLoc == NoteLocs[x].xLoc) && (NoteLocs[y].ticksinnote == NoteLocs[x].ticksinnote); y++) {//put y just past end of notes at xLoc
				;
			}
			y--;
			//sort: start with first number and compare it with last number, then next to last, etc, and move numbers up and then put it after the first bigger number found
			saveX = x;
			for (v = x; v <= y; v++) {//sort them, biggest note first
				tempNoteLocs = NoteLocs[x];
				for (z = y; tempNoteLocs.note > NoteLocs[z].note; z--)
					;
				if (z != x) {//found bigger number
					for (w = x; w <= z; w++)
						NoteLocs[w] = NoteLocs[w+1];
					NoteLocs[z] = tempNoteLocs;
				}
				else
					x++;
			}
			//put notes that are within 8 intervals on same stem and point the stem in the right direction
			x = saveX;
			if ((NoteLocs[x].notetype[1] == 'H') || (NoteLocs[x].notetype[1] == 'h'))
				fakenote = 'H';
			else
				fakenote = 'Q';
			for (z = x+1; (z <= y) && ((NoteLocs[z].y - NoteLocs[z-1].y) < 32); z++)
				;//put z just past at bottom note on stem
			z--;
			if (NoteLocs[z].notetype[1] != 'w') {
				if ((NoteLocs[z].y - yLoc) <= 42)//if bottom note => middle C
					middle = 18;
				else if ((NoteLocs[x].y - yLoc) > 42)//if top note => middle C
					middle = 66;
				else//notes encompass middle C
					middle = 38;
				if ((middle - (NoteLocs[x].y - yLoc)) > ((NoteLocs[z].y - yLoc) - middle)) {//stem will go down if lowest note is closer to middle B
					for ( ; x < z; x++)
						NoteLocs[x].notetype[3] = fakenote;
					NoteLocs[x].notetype[3] = (NoteLocs[x].notetype[1] & 0xDF);
				}
				else {//stem will go up if highest note is closer to middle B
					fakenote |= 0x20;//make it lower case
					NoteLocs[x].notetype[3] = (NoteLocs[x].notetype[1] | 0x20);
					for (x++; x <= z; x++)
							NoteLocs[x].notetype[3] = fakenote;
				}
			}
			x = z;
		}//end of else//notes on stem
	}//end of for (x = 0; x < n; x++)
//*************************************
	Y = 0;//for WM_USER2
	Note = 0;//so there's no currently played note to show
	SetCursor(hWaitingCursor);
	firstime = TRUE;
	timePtr = 0;
	timeBeginPeriod(TIMER_RESOLUTION); 
	uTimerID = timeSetEvent(1, TIMER_RESOLUTION, TimerFunc, 0, TIME_ONESHOT);
}

void WriteVLV(void)
{
	DWORD x = DeltaTicks & 0x7F;

	while (DeltaTicks >>= 7)
	{
		x <<= 8;
		x |= ((DeltaTicks & 0x7F) | 0x80);
	}
	while (TRUE)
	{
		Midi[i++] = (BYTE)x;
		if (x & 0x80)
			x >>= 8;
		else
			break;
	}
}

BOOL WriteMidi(void)
{//use data in NoteLocs to write .mid file
	DWORD shortestnote, shortestX;

	if (Midi[0] == 0) {//only write my stuff
		*(DWORD*)&Midi[0] = 0x6468544D;//"MThd"
		*(DWORD*)&Midi[4] = 0x06000000;//bytes go in in reverse order because of Big Endian
		*(WORD*)&Midi[8] = 0x0100;//type 1 track data
		*(WORD*)&Midi[0xA] = 0x0100;//1 track
		TicksPerBeat = 480;
		*(WORD*)&Midi[0xC] = 0xE001;//TicksPerBeat
		*(DWORD*)&Midi[0xE] = 0x6B72544D;//"MTrk"
		//*(DWORD*)&Midi[18] will hold TrackLen
		i = 22;//beginning of track data
		Midi[i++] = 0;//delta tick
		Midi[i++] = 0xFF;
		Midi[i++] = 0x01;//Text Event
		Midi[i++] = 4;//variable length number
		Midi[i++] = 'J';
		Midi[i++] = 'D';
		Midi[i++] = 'M';
		Midi[i++] = 'C';
		Midi[i++] = 0;//delta tick
		Midi[i++] = 0xC0;//instrument type
		Midi[i++] = 0;//piano
		Midi[i++] = 0;//delta tick
		Midi[i++] = 0xFF;
		Midi[i++] = 0x51;//set tempo
		Midi[i++] = 3;//3 bytes follow
		mSecondsPerBeat = 60000000 / BeatsPerMinute;
		Midi[i++] = (mSecondsPerBeat >> 16) & 0xFF;
		Midi[i++] = (mSecondsPerBeat >> 8) & 0xFF;
		Midi[i++] = mSecondsPerBeat & 0xFF;
		Midi[i++] = 0;//delta tick
		Midi[i++] = 0xFF;
		Midi[i++] = 0x59;
		Midi[i++] = 2;
		Midi[i++] = KeySignature;
		Midi[i++] = MajorMinor;

		//fill Event
		for (x = 0; x < n; x++) {
			ch = NoteLocs[x].notetype[1];
			if ((ch != 0xCE) && (ch != 0xB7) && (ch != 0xEE) && (ch != 0xE4) && (ch != 0xC5) && (ch != 0xA8)) {
				Event[x].note = NoteLocs[x].note;
				Event[x].velocity = NoteLocs[x].velocity;
			}
			else {
				Event[x].note = 0;//flag
				Event[x].velocity = 0;//flag
			}
			notetype = NoteLocs[x].notetype[1];
			switch (notetype)
			{
			case 0x4A://thirtysecond note flag
			case 0x6A:
				Event[x].ticksinnote = (TicksPerBeat >> 3) - SpaceAtEndOfNote;
				break;
			case 0xA8://rest
				Event[x].ticksinnote = TicksPerBeat >> 3;
				break;
			case 0x58://sixteenth note
			case 0x78:
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = (TicksPerBeat >> 2) - SpaceAtEndOfNote;
				else
					Event[x].ticksinnote = (TicksPerBeat >> 2) + (TicksPerBeat >> 3) - SpaceAtEndOfNote;
				break;
			case 0xC5://rest
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = (TicksPerBeat >> 2);
				else
					Event[x].ticksinnote = (TicksPerBeat >> 2) + (TicksPerBeat >> 3);
				break;
			case 0x45://eighth note
			case 0x65:
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = (TicksPerBeat >> 1) - SpaceAtEndOfNote;
				else
					Event[x].ticksinnote = (TicksPerBeat >> 1) + (TicksPerBeat >> 2) - SpaceAtEndOfNote;
				break;
			case 0xE4://rest
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = (TicksPerBeat >> 1);
				else
					Event[x].ticksinnote = (TicksPerBeat >> 1) + (TicksPerBeat >> 2);
				break;
			case 0x51://quarter note
			case 0x71:
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = TicksPerBeat - SpaceAtEndOfNote;
				else
					Event[x].ticksinnote = TicksPerBeat + (TicksPerBeat >> 1) - SpaceAtEndOfNote;
				break;
			case 0xCE://rest
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = TicksPerBeat;
				else
					Event[x].ticksinnote = TicksPerBeat + (TicksPerBeat >> 1);
				break;
			case 0x48://half note
			case 0x68:
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = (TicksPerBeat << 1) - SpaceAtEndOfNote;
				else
					Event[x].ticksinnote = (TicksPerBeat << 1) + TicksPerBeat - SpaceAtEndOfNote;
				break;
			case 0xB7://rest
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = (TicksPerBeat << 1);
				else
					Event[x].ticksinnote = (TicksPerBeat << 1) + TicksPerBeat;
				break;
			case 0x77://whole note
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = (TicksPerBeat << 2) - SpaceAtEndOfNote;
				else
					Event[x].ticksinnote = (TicksPerBeat << 2) + (TicksPerBeat << 1) - SpaceAtEndOfNote;
				break;
			case 0xEE://rest
				if (NoteLocs[x].notetype[2] != 'k')//a dot
					Event[x].ticksinnote = (TicksPerBeat << 2);
				else
					Event[x].ticksinnote = (TicksPerBeat << 2) + (TicksPerBeat << 1);
				break;
			default:
				MessageBox(hwnd, "Huh?", "", MB_OK);
				return FALSE;
			}
		}//end of for (x = 0; x < n; x++)

		Event[0].tickptr = 0;
		for (x = 0; x < n; x++) {
			shortestnote = Event[x].ticksinnote;
			shortestX = x;
			if (NoteLocs[x+1].xLoc == NoteLocs[x].xLoc) {
				for (y = x+1; (y < n) && (NoteLocs[y].xLoc == NoteLocs[x].xLoc); y++) {
					Event[y].tickptr = Event[x].tickptr;
					if (Event[y].ticksinnote < shortestnote) {
						shortestnote = Event[y].ticksinnote;
						shortestX = y;
					}
				}
				x = y-1;
			}
			Event[x+1].tickptr = Event[shortestX].tickptr + shortestnote + SpaceAtEndOfNote;
		}
		//insert note-off entry into Event
		EventTemp.velocity = 0;
		EventTemp.ticksinnote = SpaceAtEndOfNote;
		for (y = 0; y < x; y++) {
			if ((Event[y].velocity) && (Event[y].note)) {
				EventTemp.tickptr = Event[y].tickptr + Event[y].ticksinnote;
				EventTemp.note = Event[y].note;
				for (z = y; (z < x) && (Event[z].tickptr < EventTemp.tickptr); z++)
					;// find first Event entry with higher/equal tickptr compared to this tickptr+ticksinnote
				for (w = x, v = x+1; w >= z; w--, v--)// and make a space just before it
					Event[v] = Event[w];
				Event[z] = EventTemp;// and insert note off
				x++;
			}
		}

		for (y = 0; y < x; y++) {
			if (Event[y].note != 0) {//ignore rests
				if (y)
					DeltaTicks = Event[y].tickptr - Event[y-1].tickptr;
				else
					DeltaTicks = 0;
				WriteVLV();
				Midi[i++] = 0x90;
				Midi[i++] = Event[y].note;
				Midi[i++] = Event[y].velocity;
			}
		}
		Midi[i++] = 0;//delta tick
		Midi[i++] = 0xFF;
		Midi[i++] = 0x2F;//end of track
		Midi[i++] = 0;
		TrackLen = i - 22;
		_asm mov eax, TrackLen
		_asm bswap eax
		_asm mov TrackLen, eax
		*(DWORD*)&Midi[0x12] = TrackLen;
	}
	return TRUE;
}

void DrawSharpLines(void)
{
	if (Note == 37) {//low C#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-4-36], "__", 2);
	}
	else if (Note == 39)//low D#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note+1-36], "__", 2);
	else if (Note == 61)//middle C#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
	else if (Note == 82)//high A#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
	else if (Note == 85) {//high C#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-4-36], "__", 2);
	}
	else if (Note == 87) {//high D#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-6-36], "__", 2);
	}
	else if (Note == 90) {//high F#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-6-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-9-36], "__", 2);
	}
	else if (Note == 92) {//high G#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-4-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-8-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-11-36], "__", 2);
	}
	else if (Note == 94) {//high A#
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-6-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-9-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-13-36], "__", 2);
	}
}

void DrawFlatLines(void)
{
	if (Note == 37)//low Db
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note+3-36], "__", 2);
	else if (Note == 39)//low Eb
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note+1-36], "__", 2);
	else if (Note == 80)//high Ab
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note+1-36], "__", 2);
	else if (Note == 82)//high Bb
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-1-36], "__", 2);
	else if (Note == 85) {//high Db
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-1-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-4-36], "__", 2);
	}
	else if (Note == 87) {//high Eb
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note+1-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-6-36], "__", 2);
	}
	else if (Note == 90) {//high Gb
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note+1-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-6-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-8-36], "__", 2);
	}
	else if (Note == 92) {//high Ab
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-1-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-4-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-8-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-10-36], "__", 2);
	}
	else if (Note == 94) {//high Bb
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note+1-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-6-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-10-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-12-36], "__", 2);
	}
}

void DrawLines(void)
{
	if (Note == 36) {//low C
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-3-36], "__", 2);
	}
	else if (Note == 38)//low D
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note+2-36], "__", 2);
	else if (Note == 40)//low E
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
	else if (Note == 60)//middle C
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
	else if (Note == 81)//high A
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
	else if (Note == 83)//high B
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
	else if (Note == 84) {//high C
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
	}
	else if (Note == 86) {//D
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-2-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-4-36], "__", 2);
	}
	else if (Note == 88) {//E
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-4-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-6-36], "__", 2);
	}
	else if (Note == 89) {//F
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-1-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-5-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-7-36], "__", 2);
	}
	else if (Note == 91) {//G
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-3-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-6-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-10-36], "__", 2);
	}
	else if (Note == 93) {//A
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-1-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-5-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-8-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-12-36], "__", 2);
	}
	else if (Note == 95) {//B
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-4-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-7-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-10-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-14-36], "__", 2);
	}
	else if (Note == 96) {//C
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-1-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-5-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-8-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-11-36], "__", 2);
		TextOut(hdcMem, xLoc, yLoc - PageBegin + NoteLoc[Note-15-36], "__", 2);
	}
}

void ShowNaturalFlatSharp(int Note)
{
	if (0 == Note % 12) {//C
		if ((KeySignature == -6) || (KeySignature == 2) || (KeySignature == 3) || (KeySignature == 4) || (KeySignature == 5) || (KeySignature == 6)) {
			if (tempNoteType[0] == ' ')
				tempNoteType[0] = 0x6E;//Natural
		}
	}
	else if (0 == (Note - 1) % 12) {//C# or Db
		if ((KeySignature == -5) || (KeySignature == -6) || (KeySignature == 2) || (KeySignature == 3) || (KeySignature == 4) || (KeySignature == 5) || (KeySignature == 6)) {
			if ((tempNoteType[0] == '#') || (tempNoteType[0] == 'b'))
				tempNoteType[0] = ' ';
		}
	}
	else if (0 == (Note - 2) % 12) {//D
		if ((KeySignature == -4) || (KeySignature == -5) || (KeySignature == -6) || (KeySignature == 4) || (KeySignature == 5) || (KeySignature == 6)) {
			if (tempNoteType[0] == ' ')
				tempNoteType[0] = 0x6E;//Natural
		}
	}
	else if (0 == (Note - 3) % 12) {//D# or Eb
		if ((KeySignature == -2) || (KeySignature == -3) || (KeySignature == -4) || (KeySignature == -5) || (KeySignature == -6) || (KeySignature == 4) || (KeySignature == 5) || (KeySignature == 6)) {
			if ((tempNoteType[0] == '#') || (tempNoteType[0] == 'b'))
				tempNoteType[0] = ' ';
		}
	}
	else if (0 == (Note - 4) % 12) {//E
		if ((KeySignature == -2) || (KeySignature == -3) || (KeySignature == -4) || (KeySignature == -5) || (KeySignature == -6) || (KeySignature == 6)) {
			if (tempNoteType[0] == ' ')
				tempNoteType[0] = 0x6E;//Natural
		}
	}
	else if (0 == (Note - 5) % 12) {//F
		if ((KeySignature == 1) || (KeySignature == 2) || (KeySignature == 3) || (KeySignature == 4) || (KeySignature == 5) || (KeySignature == 6)) {
			if (tempNoteType[0] == ' ')
				tempNoteType[0] = 0x6E;//Natural
		}
	}
	else if (0 == (Note - 6) % 12) {//F# or Gb
		if ((KeySignature == 1) || (KeySignature == 2) || (KeySignature == 3) || (KeySignature == 4) || (KeySignature == 5) || (KeySignature == 6) || (KeySignature == -5) || (KeySignature == -6)) {
			if ((tempNoteType[0] == '#') || (tempNoteType[0] == 'b'))
				tempNoteType[0] = ' ';
		}
	}
	else if (0 == (Note - 7) % 12) {//G
		if ((KeySignature == -5) ||(KeySignature == -6) || (KeySignature == 3) || (KeySignature == 4) || (KeySignature == 5) || (KeySignature == 6)) {
			if (tempNoteType[0] == ' ')
				tempNoteType[0] = 0x6E;//Natural
		}
	}
	else if (0 == (Note - 8) % 12) {//G# or Ab
		if ((KeySignature == -3) || (KeySignature == -4) || (KeySignature == -5) || (KeySignature == -6) || (KeySignature == 3) || (KeySignature == 4) || (KeySignature == 5) || (KeySignature == 6)) {
			if ((tempNoteType[0] == '#') || (tempNoteType[0] == 'b'))
				tempNoteType[0] = ' ';
		}
	}
	else if (0 == (Note - 9) % 12) {//A
		if ((KeySignature == -3) || (KeySignature == -4) || (KeySignature == -5) || (KeySignature == -6) || (KeySignature == 5) || (KeySignature == 6)) {
			if (tempNoteType[0] == ' ')
				tempNoteType[0] = 0x6E;//Natural
		}
	}
	else if (0 == (Note - 10) % 12) {//A# or Bb
		if ((KeySignature == -1) || (KeySignature == -2) || (KeySignature == -3) || (KeySignature == -4) || (KeySignature == -5) || (KeySignature == -6) || (KeySignature == 5) || (KeySignature == 6)) {
			if ((tempNoteType[0] == '#') || (tempNoteType[0] == 'b'))
				tempNoteType[0] = ' ';
		}
	}
	else if (0 == (Note - 11) % 12) {//B
		if ((KeySignature == -1) || (KeySignature == -2) || (KeySignature == -3) || (KeySignature == -4) || (KeySignature == -5) || (KeySignature == -6)) {
			if (tempNoteType[0] == ' ')
				tempNoteType[0] = 0x6E;//Natural
		}
	}
}

void Uncheck(UINT id)
{
	if (id != ID_VOLUME_128)
		CheckMenuItem(hMenu, ID_VOLUME_128, MF_UNCHECKED);
	if (id != ID_VOLUME_112)
		CheckMenuItem(hMenu, ID_VOLUME_112, MF_UNCHECKED);
	if (id != ID_VOLUME_96)
		CheckMenuItem(hMenu, ID_VOLUME_96, MF_UNCHECKED);
	if (id != ID_VOLUME_80)
		CheckMenuItem(hMenu, ID_VOLUME_80, MF_UNCHECKED);
	if (id != ID_VOLUME_64)
		CheckMenuItem(hMenu, ID_VOLUME_64, MF_UNCHECKED);
	if (id != ID_VOLUME_48)
		CheckMenuItem(hMenu, ID_VOLUME_48, MF_UNCHECKED);
	if (id != ID_VOLUME_32)
		CheckMenuItem(hMenu, ID_VOLUME_32, MF_UNCHECKED);
	if (id != ID_VOLUME_16)
		CheckMenuItem(hMenu, ID_VOLUME_16, MF_UNCHECKED);
}

void Uncheck2(UINT id)
{
	if (id != ID_TEMPO_220)
		CheckMenuItem(hMenu, ID_TEMPO_220, MF_UNCHECKED);
	if (id != ID_TEMPO_200)
		CheckMenuItem(hMenu, ID_TEMPO_200, MF_UNCHECKED);
	if (id != ID_TEMPO_180)
		CheckMenuItem(hMenu, ID_TEMPO_180, MF_UNCHECKED);
	if (id != ID_TEMPO_160)
		CheckMenuItem(hMenu, ID_TEMPO_160, MF_UNCHECKED);
	if (id != ID_TEMPO_140)
		CheckMenuItem(hMenu, ID_TEMPO_140, MF_UNCHECKED);
	if (id != ID_TEMPO_120)
		CheckMenuItem(hMenu, ID_TEMPO_120, MF_UNCHECKED);
	if (id != ID_TEMPO_100)
		CheckMenuItem(hMenu, ID_TEMPO_100, MF_UNCHECKED);
	if (id != ID_TEMPO_80)
		CheckMenuItem(hMenu, ID_TEMPO_80, MF_UNCHECKED);
}

void ChangeNote(BYTE insertype)
{
	if (HorizontalHilight) {//else
		NoteLocs[CurrentX].notetype[2] = ' ';
		if (insertype != 'k') {
			if (insertype == 'H')
				fakenote = 'H';
			else
				fakenote = 'Q';
			//notes on stem have to be within 8 intervals of previous one
			for (x = CurrentX; (NoteLocs[x].xLoc == (xLoc+StaffWidth)) && ((NoteLocs[x].y - NoteLocs[x-1].y) < 32); x--)
				;//put x just before top note on stem
			if (NoteLocs[x].xLoc != xLoc+StaffWidth)
				x++;
			if (((NoteLocs[x+1].y - NoteLocs[x].y) >= 32)) {//not on a stem
				NoteLocs[x].notetype[3] = 0;
				if ((NoteLocs[x].notetype[1] | 0xDF) == 0xFF)//small letter
					NoteLocs[x].notetype[1] = insertype | 0x20;//make it a small letter
				else if (insertype != 'w')
					NoteLocs[x].notetype[1] = insertype & 0xDF;//make it a capital letter
				else
					NoteLocs[x].notetype[1] = insertype;
			}
			else {
				for (y = x+1; (NoteLocs[y].xLoc == (xLoc+StaffWidth)) && ((NoteLocs[y].y - NoteLocs[y-1].y) < 32); y++)
					;//put y just after bottom note on stem at xLoc
				y--;
				if (insertype != 'w') {
					if ((NoteLocs[y].y - yLoc) <= 42)//if bottom note => middle C
						middle = 18;
					else if ((NoteLocs[x].y - yLoc) > 42)//if top note => middle C
						middle = 66;
					else//notes encompass middle C
						middle = 38;
					if ((middle - (NoteLocs[x].y - yLoc)) > ((NoteLocs[y].y - yLoc) - middle)) {//stem will go down if lowest note is closer to middle
						for ( ; x < y; x++) {
							NoteLocs[x].notetype[1] = insertype & 0xDF;
							NoteLocs[x].notetype[3] = fakenote;
						}
						NoteLocs[x].notetype[1] = insertype & 0xDF;
						NoteLocs[x].notetype[3] = insertype & 0xDF;
					}
					else {//stem will go up if highest note is closer to middle
						fakenote |= 0x20;//make it lower case
						NoteLocs[x].notetype[1] = insertype | 0x20;//make it a small letter;
						NoteLocs[x].notetype[3] = insertype | 0x20;
						for (x++; x <= y; x++) {
							NoteLocs[x].notetype[1] = insertype | 0x20;
							NoteLocs[x].notetype[3] = fakenote;
						}
					}
				}
				else {//if (insertype == 'w')
					for ( ; x <= y; x++) {
						NoteLocs[x].notetype[1] = 'w';
						NoteLocs[x].notetype[3] = 0;
					}
				}
			}
		}
		else
			NoteLocs[CurrentX].notetype[2] = 'k';
	}//end of if (HorizontalHilight)

	else for (x = 0; x < n; x++) {
		if (((xLoc+StaffWidth) == NoteLocs[x].xLoc) && (yLoc == NoteLocs[x].yLoc)) {
			ch = NoteLocs[x].notetype[1];
			NoteLocs[x].notetype[2] = ' ';
			if ((ch == 0xCE) || (ch == 0xE4) || (ch == 0xC5) || (ch == 0xB7) || (ch == 0xEE)) {//it's a rest
				if (insertype == 'Q') {
					NoteLocs[x].notetype[1] = '\xCE';//change the rest type to a quarter rest
					NoteLocs[x].y = yLoc + 37;
				}
				else if (insertype == 'E') {
					NoteLocs[x].notetype[1] = 0xE4;//eighth rest
					NoteLocs[x].y = yLoc + 37;
				}
				else if (insertype == 'H') {
					NoteLocs[x].notetype[1] = 0xB7;//half rest
					NoteLocs[x].y = yLoc + 49;
				}
				else if (insertype == 'X') {
					NoteLocs[x].notetype[1] = 0xC5;//sixteenth rest
					NoteLocs[x].y = yLoc + 37;
				}
				else if (insertype == 'J') {
					NoteLocs[x].notetype[1] = 0xA8;//thirtysecond rest
					NoteLocs[x].y = yLoc + 37;
				}
				else if (insertype == 'w') {
					NoteLocs[x].notetype[1] = 0xEE;//whole rest
					NoteLocs[x].y = yLoc + 34;
				}
			}
			else {//it's a note
				if (insertype == 'k')
					NoteLocs[x].notetype[2] = 'k';
				else if ((NoteLocs[x].notetype[1] | 0xDF) == 0xFF)//small letter
					NoteLocs[x].notetype[1] = insertype | 0x20;//make it a small letter
				else if (insertype != 'w')
					NoteLocs[x].notetype[1] = insertype & 0xDF;//make it a capital letter
				else
					NoteLocs[x].notetype[1] = insertype;
			}
			break;
		}
	}
	Note = 0;
	InvalidateRect(hwnd, &rect, FALSE);
}
