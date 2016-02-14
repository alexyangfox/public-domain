//compiled with Microsoft's Visual C/C++ 6.0 (with the Feb 2003 MSDN library of API's)
//MidiPlyr.c in the MSDN Samples got me going,
//Petzold's Programming Windows showed how simple it could be,
//and MIDImon.c in the MSDN Samples showed how to read MIDI keyboard data.
#include <windows.h>
#include <mmsystem.h>//add winmm.lib to Project -Settings -Link
#include <math.h>//for sin and cos
//#include <commctrl.h>//add comctl32.lib
#include "resource.h"
#define PI 3.141592653589793
#define IDM_EXIT 0x100
#define IDM_INSTRUMENT 0x200
#define IDM_INPUT 0X300
#define IDM_DEVICE 0x400
#define IDM_VELOCITY 0x500
#define IDM_KEYS 0x600
#define IDM_CHORDS 0x700
#define IDM_ACCIDENTAL 0x800
#define IDM_TEST 0x900
#define IDM_UNSTICK 0xA00
#define IDM_ABOUT 0xB00
#define NEITHER 0
#define LEFT 1
#define RIGHT 2
#define BOTH 3
#define ERROR_MSG_SIZE 1024

char About[] = "Version 1.17\nMar 18, 2012\nDoug Cox\nhttp://jdmcox.com\njdmcox@jdmcox.com";

int BlackKeyNotes[25] = {37,39,42,44,46,49,51,54,56,58,61,63,66,68,70,73,75,78,80,82,85,87,90,92,94};
int saveAccidental;

//keys                       2   3       5   6   7   8   9   0       =           Q   W   E   R   T   Y   U   I   O   P   [   ]           A   S   D       G   H       K   L  ;                    Z   X   C   V   B   N   M   <   >   /
//scan codes     0,  1,  2,  3,  4, -1,  6,  7,  8, -1, 10, 11, -1, 13, -1, -1, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, -1, -1, 30, 31, 32, -1, 34, 35, -1, 37, 38, 39, -1, -1, -1, -1, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53  -1  55  56  57  58  -1  60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80
//notes                     C#  D#      F#  G#  A#      C#  D#      F#           C   D   E   F   G   A   B   C   D   E   F   G          F#  G#  A#      C#  D#      F#  G#  A#                   G   A   B   C   D   E   F   G   A   B       C   D   E   F       A   B   C   D   E   F   G   A   B  C#  D#      G#  A#  C#  D#  F#  G#  A#  B#   C
int Notes[] = { -1, -1, -1, 61, 63, -1, 66, 68, 70, -1, 73, 75, -1, 78, -1, -1, 60, 62, 64, 65, 67, 69, 71, 72, 74, 76, 77, 79, -1, -1, 42, 44, 46, -1, 49, 51, -1, 54, 56, 58, -1, -1, -1, -1, 43, 45, 47, 48, 50, 52, 53, 55, 57, 59, -1, 36, 38, 40, 41, -1, 81, 83, 84, 86, 88, 89, 91, 93, 95, 37, 39, -1, 80, 82, 85, 87, 90, 92, 94, 96, 97};
int xKey[] =  { -1, -1, -1, 15, 16, -1, 18, 19, 20, -1, 22, 23, -1, 25, -1, -1, 14, 15, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, -1, -1,  4,  5,  6, -1,  8,  9, -1, 11, 12, 13, -1, -1, -1, -1,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13, -1,  0,  1,  2,  3, -1, 26, 27, 28, 29, 30, 31, 32, 33, 34,  1,  2, -1, 26, 27, 29, 30, 32, 33, 34};
int yKey[] =  { -1, -1, -1,  0,  0, -1,  0,  0,  0, -1,  0,  0, -1,  0, -1, -1,  2,  3,  1,  2,  3,  3,  1,  2,  3,  1,  2,  3, -1, -1,  0,  0,  0, -1,  0,  0, -1,  0,  0,  0, -1, -1, -1, -1,  3,  3,  1,  2,  3,  1,  2,  3,  3,  1, -1,  2,  3,  1,  2, -1,  3,  1,  2,  3,  1,  2,  3,  3,  1,  0,  0, -1,  0,  0,  0,  0,  0,  0,  0,  0,  0};
//t yStaff[] ={ -1, -1, -1,382,368, -1,356,350,344,338,332,326, -1,314, -1, -1,382,368,362,356,350,344,338,332,326,320,314,308, -1, -1,458,452,446, -1,434,428, -1,416,410,404, -1, -1, -1, -1,452,446,440,434,428,422,416,410,404,398, -1,476,470,464,458, -1,302,296,290,284,278,272,266,260,254,476,470, -1,308,302,290,284,272,266,260,254,248};
int yStaff[61];//36 is the lowest note

//               C   D   E   F   G   A   B   C   D   E   F   G   A   B   C   D   E   F   G   A   B   C   D   E   F   G   A   B   C   D   E   F   G   A   B
int LKeys[] = { 55, 69, 70, 58, 30, 31, 32, 47, 34, 35, 50, 37, 38, 39, 16,  3,  4, 19,  6,  7,  8, 23, 10, 11, 26, 13, 72, 73, 62, 74, 75, 65, 76, 77, 78};
int RKeys[] = { 69, 70, 57, 30, 31, 32, 46, 34, 35, 49, 37, 38, 39, 53,  3,  4, 18,  6,  7,  8, 22, 10, 11, 25, 13, 72, 73, 61, 74, 75, 64, 76, 77, 78, 68};
int MouseKeys[] = { -1, -1, -1, -1, 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 55, 56, 57, 58, 60, 61, 62, 63, 64, 65, 66, 67, 68};

//              36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96
int Codes[] = { 55, 69, 56, 70, 57, 58, 30, 44, 31, 45, 32, 46, 47, 34, 48, 35, 49, 50, 37, 51, 38, 52, 39, 53, 16,  3, 17,  4, 18, 19,  6, 20,  7, 21,  8, 22, 23, 10, 24, 11, 25, 26, 13, 27, 72, 60, 73, 61, 62, 74, 63, 75, 64, 65, 76, 66, 77, 67, 78, 68, 80};

char CDEFGAB[] = "CDEFGAB";
char ComputerKeys1[] = "ZXCVBNM,./QWERTYUIOP[]";
char ComputerKeys2[] = "ASDFGHJKL;'234567890-=";
char Staff[128];
int StaffWidth[1];
int NoteWidth[1];
int GClefWidth[1];
int FClefWidth[1];
int NumOfStaffs;
int Widths[7];
int Widths1[22];
int Widths2[22];
int ScanCodes[40];

int x, y, z, left, saveleft, top, top260, top260min30, top260min30div2, xPos, yPos, accidental = 1, index, TitleAndMenu;
int iNumDevs, iInDevice = 0, iOutDevice = MIDIMAPPER;//-1 (pointer to driver for default Windows MIDI sounds)
int WhiteKeyWidth, BlackKeyWidth, ExtraSpace, noteloc, RandomNoteLoc, ChordType = 0, KeyName = 0, Inversion = 0;
int ScanCode, Note, PreviousNote = -1, RandomNote, PreviousRandomNote = 36, SavedNote, BracketingKeys, Velocity, DefaultVelocity = 127;
DWORD dwInstance, dwParam1, dwParam2;
DWORD fileSize, dwBytesRead, dwBytesWritten;
double d1, d2, d3, d4;
char szAppName[] = "SimplePiano";
char SimplePianoIni[] = "SimplePiano.ini";
char Ini[512];
char InDev[] = "Input Device=";
char OutDev[] = "Output Device=";
char Vel[] = "Default Velocity=";
char ShowChords[] = "Play a &Chord";
char HideChords[] = "Hide &Chords";
char ShowKeys[] = "Show &Keys";
char HideKeys[] = "Hide &Keys";
char temp[16];
char VelocityChoice[8][4] = {"128","112"," 96"," 80"," 64"," 48"," 32"," 16"};
//char Keys[12][3] = {"C ","F ","Bb","Eb","Ab","Db","Gb","B ","E ","A ","D ","G "};
  char Keys[12][3] = {"C ","G ","D ","A ","E ","B ","Gb","Db","Ab","Eb","Bb","F "};
//int Scale[12] = {60,65,70,63,68,61,66,71,64,69,62,67};
  int Scale[12] = {60,67,62,69,64,71,66,61,68,63,70,65};
int Interval[] = {0,2,4,5,7,9,11,12};
int xKeyLoc[12];
int yKeyLoc[12];
char Chords[10][16] = {"No Chord       ","Major Triad    ","Minor Triad    ","Dimished Triad ","Augmented Triad","Dominant 7th   ","Major 7th      ","Minor 7th      ","Diminished 7th ","All Scale Notes"};
char Inversions[4][14] = {"No Inversion ","1st Inversion","2nd Inversion","3rd Inversion"};
char PlayNote[] =  "&Note Reading Test";
char StopNotes[] = "&Stop Note Reading";
char Arial[] = "Arial";
char Maestro[] = "Maestro";
char ArialRounded[] = "Arial Rounded MT Bold";
char ErrorMsg[1024];
BOOL first = TRUE, fromkeydown, midi_in = FALSE, showchords = FALSE, showkeys = FALSE, showtest = FALSE, showscale = FALSE, showinginstruments = FALSE;
HWND hwnd, hwndChords[9], hwndInversions[4], hwndKeys[12], hwndShowScale, hwndList;
HINSTANCE hInst;
HANDLE hFile;
HMENU hMenu, hMenuPopup;
HFONT hFont, hSmallFont, hMaestroFont;
HPEN hPen;
HBRUSH hWhiteBrush, hBlackBrush, hBlueBrush;
HGDIOBJ hOldFont, hOldPen, hOldBrush;
RECT rect, testRect;
HDC hdc, hdcMem;
HBITMAP hMemBitmap;
PAINTSTRUCT ps;
LOGFONT lf, lf1, lf2;
HMIDIOUT hMidiOut;
MIDIOUTCAPS moc;
HMIDIIN hMidiIn;
MIDIINCAPS mic;
SYSTEMTIME st;
FILETIME ft;
ULARGE_INTEGER ul;
//TRACKMOUSEEVENT tme;

char Piano[] = "000 Acoustic Grand Piano";
char Piano2[] = "001 Bright Acoustic Piano";
char Piano3[] = "002 Electric Grand Piano";
char Piano4[] = "003 Honky Tonk Piano";
char Piano5[] = "004 Electric Piano 1";
char Piano6[] = "005 Electric Piano 2";
char Piano7[] = "006 Harpsichord";
char Piano8[] = "007 Clavinet";
char ChromaticPercussion[] = "008 Celesta";
char ChromaticPercussion2[] = "009 Glockenspiel";
char ChromaticPercussion3[] = "010 Music Box";
char ChromaticPercussion4[] = "011 Vibraphone";
char ChromaticPercussion5[] = "012 Marimba";
char ChromaticPercussion6[] = "013 Xylophone";
char ChromaticPercussion7[] = "014 Tubular Bells";
char ChromaticPercussion8[] = "015 Dulcimer";
char Organ[] = "016 Drawbar Organ";
char Organ2[] = "017 Percussive Organ";
char Organ3[] = "018 Rock Organ";
char Organ4[] = "019 Church Organ";
char Organ5[] = "020 Reed Organ";
char Organ6[] = "021 Accoridan";
char Organ7[] = "022 Harmonica";
char Organ8[] = "023 Tango Accordian";
char Guitar[] = "024 Nylon Acoustic Guitar";
char Guitar2[] = "025 Steel Acoustic Guitar";
char Guitar3[] = "026 Jazz Electric Guitar";
char Guitar4[] = "027 Clean Electric Guitar";
char Guitar5[] = "028 Muted Electric Guitar";
char Guitar6[] = "029 Overdrive Guitar";
char Guitar7[] = "030 Distorted Guitar";
char Guitar8[] = "031 Harmonica Guitar";
char Bass[] = "032 Acoustic Bass";
char Bass2[] = "033 Electric Fingered Bass";
char Bass3[] = "034 Electric Picked Bass";
char Bass4[] = "035 Fretless Bass";
char Bass5[] = "036 Slap Bass 1";
char Bass6[] = "037 Slap Bass 2";
char Bass7[] = "038 Syn Bass 1";
char Bass8[] = "039 Syn Bass 2";
char Strings[] = "040 Violin";
char Strings2[] = "041 Viola";
char Strings3[] = "042 Cello";
char Strings4[] = "043 Contrabass";
char Strings5[] = "044 Tremolo Strings";
char Strings6[] = "045 Pizzicato Strings";
char Strings7[] = "046 Orchestral Harp";
char Strings8[] = "047 Timpani";
char Ensemble[] = "048 String Ensemble 1";
char Ensemble2[] = "049 String Ensemble 2 (Slow)";
char Ensemble3[] = "050 Syn Strings 1";
char Ensemble4[] = "051 Syn Strings 2";
char Ensemble5[] = "052 Choir Aahs";
char Ensemble6[] = "053 Voice Olhs";
char Ensemble7[] = "054 Syn Choir";
char Ensemble8[] = "055 Orchestral Hit";
char Brass[] = "056 Trumpet";
char Brass2[] = "057 Trombone";
char Brass3[] = "058 Tuba";
char Brass4[] = "059 Muted Trumpet";
char Brass5[] = "060 French Horn";
char Brass6[] = "061 Brass Section";
char Brass7[] = "062 Syn Brass 1";
char Brass8[] = "063 Syn Brass 2";
char Reed[] = "064 Soprano Sax";
char Reed2[] = "065 Alto Sax";
char Reed3[] = "066 Tenor Sax";
char Reed4[] = "067 Baritone Sax";
char Reed5[] = "068 Oboe";
char Reed6[] = "069 English Horn";
char Reed7[] = "070 Bassoon";
char Reed8[] = "071 Clarinet";
char Pipe[] = "072 Piccolo";
char Pipe2[] = "073 Flute";
char Pipe3[] = "074 Recorder";
char Pipe4[] = "075 Pan Flute";
char Pipe5[] = "076 Bottle Blow";
char Pipe6[] = "077 Shakuhachi";
char Pipe7[] = "078 Whistle";
char Pipe8[] = "079 Ocarina";
char SynthLead[] = "080 Syn Square Wave";
char SynthLead2[] = "081 Syn Sawtooth Wave";
char SynthLead3[] = "082 Syn Calliope";
char SynthLead4[] = "083 Syn Chiff";
char SynthLead5[] = "084 Syn Chrang";
char SynthLead6[] = "085 Syn Voice";
char SynthLead7[] = "086 Syn Fifths Sawtooth Wave";
char SynthLead8[] = "087 Syn Brass & Lead";
char SynthPad[] = "088 New Age Syn Pad";
char SynthPad2[] = "089 Warm Syn Pad";
char SynthPad3[] = "090 Polysynth Syn Pad";
char SynthPad4[] = "091 Choir Syn Pad";
char SynthPad5[] = "092 Bowed Syn Pad";
char SynthPad6[] = "093 Metal Syn Pad";
char SynthPad7[] = "094 Halo Syn Pad";
char SynthPad8[] = "095 Sweep Syn Pad";
char SynthEffects[] = "096 SFX Rain";
char SynthEffects2[] = "097 SFX Soundtrack";
char SynthEffects3[] = "098 SFX Crystal";
char SynthEffects4[] = "099 SFX Atmosphere";
char SynthEffects5[] = "100 SFX Brightness";
char SynthEffects6[] = "101 SFX Goblins";
char SynthEffects7[] = "102 SFX Echoes";
char SynthEffects8[] = "103 SFX Sci-Fi";
char Ethnic[] = "104 Sitar";
char Ethnic2[] = "105 Banjo";
char Ethnic3[] = "106 Shamisen";
char Ethnic4[] = "107 Koto";
char Ethnic5[] = "108 Kalimba";
char Ethnic6[] = "109 Bag Pipe";
char Ethnic7[] = "110 Fiddle";
char Ethnic8[] = "111 Shanai";
char Percussive[] = "112 Tinkle Bell";
char Percussive2[] = "113 Agogo";
char Percussive3[] = "114 Steel Drum";
char Percussive4[] = "115 Woodblock";
char Percussive5[] = "116 Taiko Drum";
char Percussive6[] = "117 Melodic Tom";
char Percussive7[] = "118 Syn Drum";
char Percussive8[] = "119 Reverse Cymbal";
char SoundEffects[] = "120 Guitar Fret Noise";
char SoundEffects2[] = "121 Breath Noise";
char SoundEffects3[] = "122 Seashore";
char SoundEffects4[] = "123 Bird Tweet";
char SoundEffects5[] = "124 Telephone Ring";
char SoundEffects6[] = "125 Helicopter";
char SoundEffects7[] = "126 Applause";
char SoundEffects8[] = "127 Gun Shot";

DWORD *Instruments[128] = {\
(DWORD*)Piano,
(DWORD*)Piano2,
(DWORD*)Piano3,
(DWORD*)Piano4,
(DWORD*)Piano5,
(DWORD*)Piano6,
(DWORD*)Piano7,
(DWORD*)Piano8,
(DWORD*)ChromaticPercussion,
(DWORD*)ChromaticPercussion2,
(DWORD*)ChromaticPercussion3,
(DWORD*)ChromaticPercussion4,
(DWORD*)ChromaticPercussion5,
(DWORD*)ChromaticPercussion6,
(DWORD*)ChromaticPercussion7,
(DWORD*)ChromaticPercussion8,
(DWORD*)Organ,
(DWORD*)Organ2,
(DWORD*)Organ3,
(DWORD*)Organ4,
(DWORD*)Organ5,
(DWORD*)Organ6,
(DWORD*)Organ7,
(DWORD*)Organ8,
(DWORD*)Guitar,
(DWORD*)Guitar2,
(DWORD*)Guitar3,
(DWORD*)Guitar4,
(DWORD*)Guitar5,
(DWORD*)Guitar6,
(DWORD*)Guitar7,
(DWORD*)Guitar8,
(DWORD*)Bass,
(DWORD*)Bass2,
(DWORD*)Bass3,
(DWORD*)Bass4,
(DWORD*)Bass5,
(DWORD*)Bass6,
(DWORD*)Bass7,
(DWORD*)Bass8,
(DWORD*)Strings,
(DWORD*)Strings2,
(DWORD*)Strings3,
(DWORD*)Strings4,
(DWORD*)Strings5,
(DWORD*)Strings6,
(DWORD*)Strings7,
(DWORD*)Strings8,
(DWORD*)Ensemble,
(DWORD*)Ensemble2,
(DWORD*)Ensemble3,
(DWORD*)Ensemble4,
(DWORD*)Ensemble5,
(DWORD*)Ensemble6,
(DWORD*)Ensemble7,
(DWORD*)Ensemble8,
(DWORD*)Brass,
(DWORD*)Brass2,
(DWORD*)Brass3,
(DWORD*)Brass4,
(DWORD*)Brass5,
(DWORD*)Brass6,
(DWORD*)Brass7,
(DWORD*)Brass8,
(DWORD*)Reed,
(DWORD*)Reed2,
(DWORD*)Reed3,
(DWORD*)Reed4,
(DWORD*)Reed5,
(DWORD*)Reed6,
(DWORD*)Reed7,
(DWORD*)Reed8,
(DWORD*)Pipe,
(DWORD*)Pipe2,
(DWORD*)Pipe3,
(DWORD*)Pipe4,
(DWORD*)Pipe5,
(DWORD*)Pipe6,
(DWORD*)Pipe7,
(DWORD*)Pipe8,
(DWORD*)SynthLead,
(DWORD*)SynthLead2,
(DWORD*)SynthLead3,
(DWORD*)SynthLead4,
(DWORD*)SynthLead5,
(DWORD*)SynthLead6,
(DWORD*)SynthLead7,
(DWORD*)SynthLead8,
(DWORD*)SynthPad,
(DWORD*)SynthPad2,
(DWORD*)SynthPad3,
(DWORD*)SynthPad4,
(DWORD*)SynthPad5,
(DWORD*)SynthPad6,
(DWORD*)SynthPad7,
(DWORD*)SynthPad8,
(DWORD*)SynthEffects,
(DWORD*)SynthEffects2,
(DWORD*)SynthEffects3,
(DWORD*)SynthEffects4,
(DWORD*)SynthEffects5,
(DWORD*)SynthEffects6,
(DWORD*)SynthEffects7,
(DWORD*)SynthEffects8,
(DWORD*)Ethnic,
(DWORD*)Ethnic2,
(DWORD*)Ethnic3,
(DWORD*)Ethnic4,
(DWORD*)Ethnic5,
(DWORD*)Ethnic6,
(DWORD*)Ethnic7,
(DWORD*)Ethnic8,
(DWORD*)Percussive,
(DWORD*)Percussive2,
(DWORD*)Percussive3,
(DWORD*)Percussive4,
(DWORD*)Percussive5,
(DWORD*)Percussive6,
(DWORD*)Percussive7,
(DWORD*)Percussive8,
(DWORD*)SoundEffects,
(DWORD*)SoundEffects2,
(DWORD*)SoundEffects3,
(DWORD*)SoundEffects4,
(DWORD*)SoundEffects5,
(DWORD*)SoundEffects6,
(DWORD*)SoundEffects7,
(DWORD*)SoundEffects8};

int Instrument = 0;

void DrawKey(void);
void StartNote(void);
void EndNote(void);
void CALLBACK MidiInProc(HMIDIIN hMidiIn, WORD wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2);
int Atoi(char*);
void WriteIni(void);
void NoteTest(void);
WNDPROC pListProc;
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
	wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
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
LRESULT ListProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_LBUTTONUP)
	{
		index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
		Instrument = index;
		midiOutShortMsg(hMidiOut, 0x0C0 | (Instrument << 8));
		DestroyWindow(hwndList);
	}
	else if (message == WM_KEYUP)
	{
		if ((wParam == VK_DOWN) && (index < 127))
			index++;
		else if ((wParam == VK_UP) && (index != 0))
			index--;

		else if (wParam == VK_ESCAPE) {
			DestroyWindow(hwndList);
		}
		else if (wParam == VK_RETURN) {
			index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			if (index < 128) {
				DestroyWindow(hwndList);
				Instrument = index;
				midiOutShortMsg(hMidiOut, 0x0C0 | (Instrument << 8));
			}
		}
	}
	else if (message == WM_DESTROY)
		showinginstruments = FALSE;

	return CallWindowProc(pListProc, hwnd2, message, wParam, lParam);
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		if (0 == AddFontResource("MAESTRO_.TTF"))
			MessageBox(hwnd, "Couldn't load the MAESTRO_.TTF font!\nMake sure it's in the Simple Piano folder,\nor the folder that Windows7 moves data files to.", ERROR, MB_OK);
 		GetLocalTime(&st);
		SystemTimeToFileTime(&st, &ft);
		ul.LowPart = ft.dwLowDateTime;
		ul.HighPart = ft.dwHighDateTime;
		ul.QuadPart /= 10000;//because low 4 digits are 0's
		srand(ul.LowPart);

		hwndKeys[0] = hwndChords[0] = NULL;
		hFile = CreateFile(SimplePianoIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			fileSize = GetFileSize(hFile, NULL);
			if ((fileSize > 0) && (fileSize < 512))
			{
				ReadFile(hFile, Ini, fileSize, &dwBytesRead, NULL);
				for (x = 0; (x < (int)fileSize) && (Ini[x] != '='); x++)
					;
				if (Ini[x] == '=')
				{
					if ((Ini[x-12] == 'I') && (Ini[x-11] == 'n'))
					{
						z = Ini[x+1];
						if (z == '-')
							iInDevice = -1;
						else
							iInDevice = z - '0';
						for (x++; (x < (int)fileSize) && (Ini[x] != '='); x++)
							;
					}
					z = Ini[x+1];
					if (z == '-')
						iOutDevice = -1;
					else
						iOutDevice = z - '0';
					for (x++; (x < (int)fileSize) && (Ini[x] != '='); x++)
						;
					if (Ini[x] == '=')
						DefaultVelocity = Atoi(&Ini[x+1]);
				}
			}
			CloseHandle(hFile);
		}
		hMenu = CreateMenu();
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING, IDM_EXIT, "&Exit");
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING, IDM_INSTRUMENT, "&Instrument");
		iNumDevs = midiOutGetNumDevs();
		z = midiInGetNumDevs();
		if (z) {
			AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "Input &Device");
			for (x = 0; x < z; x++) {
				if (MMSYSERR_NOERROR == midiInGetDevCaps(x, &mic, sizeof(mic)))
					AppendMenu(hMenuPopup, MF_STRING, IDM_INPUT + x, mic.szPname);
			}
			CheckMenuItem(hMenuPopup, IDM_INPUT + iInDevice, MF_CHECKED);
		}
		hMenuPopup = CreateMenu();
		if (MMSYSERR_NOERROR == midiOutGetDevCaps(MIDIMAPPER, &moc, sizeof(moc)))//Microsoft MIDI Mapper
		{
			AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "&Output Device");
			AppendMenu(hMenuPopup, MF_STRING, IDM_DEVICE + (int)MIDIMAPPER, moc.szPname);
			for (x = 0; x < iNumDevs; x++)
			{
				midiOutGetDevCaps(x, &moc, sizeof(moc));
				AppendMenu(hMenuPopup, MF_STRING, IDM_DEVICE + x, moc.szPname);
			}
			CheckMenuItem(hMenuPopup, IDM_DEVICE + iOutDevice, MF_CHECKED);
		}
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "Default Key &Volume");
		for (x = 0; x < 8; x++)
		{
			AppendMenu(hMenuPopup, MF_STRING, IDM_VELOCITY + x, VelocityChoice[x]);
		}
		CheckMenuItem(hMenuPopup, 8 - ((DefaultVelocity+1) / 16) + IDM_VELOCITY, MF_CHECKED);
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING|MF_POPUP, (UINT)hMenuPopup, "Default &Accidental");
		AppendMenu(hMenuPopup, MF_STRING, IDM_ACCIDENTAL + 1, "#");
		AppendMenu(hMenuPopup, MF_STRING, IDM_ACCIDENTAL + 2, "b");
		CheckMenuItem(hMenuPopup, IDM_ACCIDENTAL+1, MF_CHECKED);
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING, IDM_KEYS, ShowKeys);
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING, IDM_CHORDS, ShowChords);
		hMenuPopup = CreateMenu();
		AppendMenu(hMenu, MF_STRING, IDM_TEST, PlayNote);
		hMenuPopup = CreateMenu();
//		AppendMenu(hMenu, MF_STRING, IDM_UNSTICK, "&Unstick Notes");
//		SetMenu(hwnd, hMenu);
		AppendMenu(hMenu, MF_STRING, IDM_ABOUT, "&About");
		SetMenu(hwnd, hMenu);

		if (MMSYSERR_NOERROR == midiOutOpen(&hMidiOut, iOutDevice, 0, 0, 0)) {
			midiOutShortMsg(hMidiOut, 0x0C0 | (Instrument << 8));//channel 0 and grand piano (instrument 0)
			midiConnect((HMIDI)hMidiIn, (HMIDIOUT)hMidiOut, NULL);//THRU MIDI
		}

		if (MMSYSERR_NOERROR == midiInOpen((LPHMIDIIN)&hMidiIn, iInDevice, (DWORD)MidiInProc, 0, CALLBACK_FUNCTION))
		{//if a MIDI keyboard is attached
			midi_in = TRUE;
			midiInStart(hMidiIn);
		}

		lf.lfHeight = -29;
		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 3;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x22;
		for (x = 0; Arial[x] != 0; x++)
			lf.lfFaceName[x] = Arial[x];
		lf.lfFaceName[x] = 0;
		hFont = CreateFontIndirect(&lf);

		lf1.lfHeight = -13;
		lf1.lfWeight = 400;
		lf1.lfItalic = 0;
		lf1.lfUnderline = 0;
		lf1.lfStrikeOut = 0;
		lf1.lfCharSet = 0;
		lf1.lfOutPrecision = 3;
		lf1.lfClipPrecision = 2;
		lf1.lfQuality = 1;
		lf1.lfPitchAndFamily = 0x22;
		for (x = 0; Arial[x] != 0; x++)
			lf1.lfFaceName[x] = Arial[x];
		lf1.lfFaceName[x] = 0;
		hSmallFont = CreateFontIndirect(&lf1);

		lf2.lfHeight = -48;
		lf2.lfWeight = 400;
		lf2.lfItalic = 0;
		lf2.lfUnderline = 0;
		lf2.lfStrikeOut = 0;
		lf2.lfCharSet = 2;
		lf2.lfOutPrecision = 3;
		lf2.lfClipPrecision = 2;
		lf2.lfQuality = 1;
		lf2.lfPitchAndFamily = 0x02;
		for (x = 0; Maestro[x] != 0; x++)
			lf2.lfFaceName[x] = Maestro[x];
		lf2.lfFaceName[x] = 0;
		hMaestroFont = CreateFontIndirect(&lf2);

		hPen = CreatePen(PS_SOLID, 2, 0x808080);
		hWhiteBrush = CreateSolidBrush(0xFFFFFF);
		hBlackBrush = CreateSolidBrush(0);
		hBlueBrush = CreateSolidBrush(0xFF0000);
		for (x = 0; x < 40; x++)
			ScanCodes[x] = 0;
		TitleAndMenu = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU);
		return 0;


	case WM_SIZE:
		rect.left = rect.top = 0;
		rect.right = LOWORD(lParam);
		rect.bottom = HIWORD(lParam);
		top = rect.bottom - 180;
		top260 = top - 260;
		top260min30 = top260 - 30;
		if (top260min30 > 252)//top of screen in 1024x768
			top260min30 = 252;
		top260min30div2 = top260min30/2;
		noteloc = top-98;//low C, note 36
		for (x = 0; x < 60; x += 12)
		{
			for (z = 0; z < 12; )
			{
				if ((x+z) == (60-36))//middle C
					noteloc -= 10;
				yStaff[x+(z++)] = noteloc;//36 C
				yStaff[x+z++] = noteloc;//37 C#
				noteloc -= 6;
				if ((x+z) == (62-36))//D above middle C
					noteloc -= 8;
				yStaff[x+z++] = noteloc;//38 D
				yStaff[x+z++] = noteloc;//39 D#
				noteloc -= 6;
				yStaff[x+z++] = noteloc;//40 E
				noteloc -= 6;
				yStaff[x+z++] = noteloc;//41 F
				yStaff[x+z++] = noteloc;//42 F#
				noteloc -= 6;
				yStaff[x+z++] = noteloc;//43 G
				yStaff[x+z++] = noteloc;//44 G#
				noteloc -= 6;
				yStaff[x+z++] = noteloc;//45 A
				yStaff[x+z++] = noteloc;//46 A#
				noteloc -= 6;
				yStaff[x+z++] = noteloc;//47 B
				noteloc -= 6;
			}
		}
//                      0    1    2    3    4    5    6    7    8    9   10   11
//char Keys[12][3] = {"C ","F ","Bb","Eb","Ab","Db","Gb","B ","E ","A ","D ","G "};
//char Keys[12][3] = {"C ","G ","D ","A ","E ","B ","Gb","Db","Ab","Eb","Bb","F "};
		d1 = (double)top260min30div2 * cos(60.0 * PI / 180.0);//for circle of fifths
		d2 = (double)top260min30div2 * sin(60.0 * PI / 180.0);
		d3 = (double)top260min30div2 * cos(30.0 * PI / 180.0);
		d4 = (double)top260min30div2 * sin(30.0 * PI / 180.0);
		xKeyLoc[0] = top260min30div2;
		yKeyLoc[0] = 0;
		xKeyLoc[1] = (int)d1 + top260min30div2;
		yKeyLoc[1] = top260min30div2 - (int)d2;
		xKeyLoc[2] = (int)d3 + top260min30div2;
		yKeyLoc[2] = top260min30div2 - (int)d4;
		xKeyLoc[3] = top260min30;
		yKeyLoc[3] = top260min30div2;
		xKeyLoc[4] = xKeyLoc[2];
		yKeyLoc[4] = top260min30div2 + (int)d4;
		xKeyLoc[5] = xKeyLoc[1];
		yKeyLoc[5] = top260min30div2 + (int)d2;
		xKeyLoc[6] = top260min30div2;
		yKeyLoc[6] = top260min30;
		xKeyLoc[7] = top260min30div2 - (int)d1;
		yKeyLoc[7] = top260min30 - top260min30div2 + (int)d2;
		xKeyLoc[8] = top260min30div2 - (int)d3;
		yKeyLoc[8] = top260min30 - top260min30div2 + (int)d4;
		xKeyLoc[9] = 0;
		yKeyLoc[9] = top260min30div2;
		xKeyLoc[10] = top260min30div2 - (int)d3;
		yKeyLoc[10] = top260min30 - top260min30div2 - (int)d4;
		xKeyLoc[11] = top260min30div2 - (int)d1;
		yKeyLoc[11] = top260min30 - top260min30div2 - (int)d2;
		if (hwndKeys[0]!= NULL)
		{
			for (x = 0; x < 12; x++)
			{
				DestroyWindow(hwndKeys[x]);
				hwndKeys[x] = NULL;
			}
			for (x = 0, z = top260-270; x < 12; x++)
			{
				hwndKeys[x] = CreateWindow("BUTTON", Keys[x],
					WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
					xKeyLoc[x], yKeyLoc[x], 45, 30,
					hwnd, NULL, hInst, NULL);
			}
			DestroyWindow(hwndShowScale);
			hwndShowScale = CreateWindow("BUTTON", "Show Scale",
				WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
				top260min30-(top260min30/2)-35, top260min30-(top260min30/2), 110, 30,
				hwnd, NULL, hInst, NULL);
		}
		if (hwndChords[0] != NULL)
		{
			for (x = 0; x < 9; x++)
			{
				DestroyWindow(hwndChords[x]);
				hwndChords[x] = NULL;
			}
			for (x = 0, z = 0; x < 9; x++, z += 30)
			{
				hwndChords[x] = CreateWindow("BUTTON", Chords[x],
					WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
					top260min30 + 50, z, 135, 30,
					hwnd, NULL, hInst, NULL);
			}
			SendMessage(hwndChords[ChordType], BM_SETCHECK, BST_CHECKED, 0);
			for (x = 0; x < 4; x++)
				DestroyWindow(hwndInversions[x]);
			for (x = 0, z = 30; x < 3; x++, z += 30)
			{
				hwndInversions[x] = CreateWindow("BUTTON", Inversions[x],
					WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
					top260min30 + 50 + 135, z, 120, 30,
					hwnd, NULL, hInst, NULL);
			}
			hwndInversions[3] = CreateWindow("BUTTON", Inversions[3],
				WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
				top260min30 + 50 + 135, 150, 120, 30,
				hwnd, NULL, hInst, NULL);
			SendMessage(hwndInversions[Inversion], BM_SETCHECK, BST_CHECKED, 0);
		}

		if (showtest)
		{
			SendMessage(hwnd, WM_COMMAND, IDM_ACCIDENTAL+saveAccidental, 0);
			showtest = FALSE;
			ModifyMenu(hMenu, IDM_TEST, MF_BYCOMMAND|MF_STRING, IDM_TEST, PlayNote);
			DrawMenuBar(hwnd);
		}
		WhiteKeyWidth = rect.right / 35;//5 octaves = 60 keys = 35 white keys
		BlackKeyWidth = WhiteKeyWidth*2/3;
		ExtraSpace = (rect.right % 35) / 2;
		return 0;


	case WM_ACTIVATE:
		if (LOWORD(wParam) == WA_INACTIVE)
		{
			if (showtest)
			{
				showtest = FALSE;
				SendMessage(hwnd, WM_COMMAND, IDM_ACCIDENTAL+saveAccidental, 0);
				ModifyMenu(hMenu, IDM_TEST, MF_BYCOMMAND|MF_STRING, IDM_TEST, PlayNote);
				DrawMenuBar(hwnd);
			}
			for (x = 0; x < 40; x++)
				ScanCodes[x] = 0;
		}
		break;

	case WM_CAPTURECHANGED:
		for (x = 0; x < 40; x++)
			ScanCodes[x] = 0;
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == IDM_EXIT)
			DestroyWindow(hwnd);

		else if (LOWORD(wParam) == IDM_INSTRUMENT)
		{
			if (showinginstruments == FALSE)
			{
				showinginstruments = TRUE;
				InvalidateRect(hwnd, &rect, FALSE);
				hwndList = CreateWindow("LISTBOX", "Instruments",
					WS_POPUP | WS_VISIBLE | WS_CAPTION | WS_SYSMENU | WS_VSCROLL,// | LBS_NOTIFY,
					0, TitleAndMenu, 240, rect.bottom,
					hwnd, NULL, hInst, NULL);
				pListProc = (WNDPROC)SetWindowLong(hwndList, GWL_WNDPROC, (LONG)ListProc);
				for (x = 0; x < 128; x++)
					SendMessage(hwndList, LB_ADDSTRING, 0, *(DWORD*)&Instruments[x]);
				index = Instrument;
				SendMessage(hwndList, LB_SETCURSEL, index, 0);
				SetFocus(hwndList);
			}
		}

		else if ((LOWORD(wParam) >= IDM_INPUT-1) && (LOWORD(wParam) < (IDM_DEVICE-1)))
		{
			CheckMenuItem(hMenu, IDM_INPUT + iInDevice, MF_UNCHECKED);
			iInDevice = LOWORD(wParam) - IDM_INPUT;
			if (iInDevice)
				WriteIni();
			CheckMenuItem(hMenu, IDM_INPUT + iInDevice, MF_CHECKED);

			midiInClose(hMidiIn);
			if (MMSYSERR_NOERROR == midiInOpen((LPHMIDIIN)&hMidiIn, iInDevice, (DWORD)MidiInProc, 0, CALLBACK_FUNCTION))
			{//if a MIDI keyboard is attached
				midi_in = TRUE;
				midiInStart(hMidiIn);
			}
		}

		else if ((LOWORD(wParam) >= IDM_DEVICE-1) && (LOWORD(wParam) < (IDM_VELOCITY-1)))
		{
			CheckMenuItem(hMenu, IDM_DEVICE + iOutDevice, MF_UNCHECKED);
			iOutDevice = LOWORD(wParam) - IDM_DEVICE;
			WriteIni();
			CheckMenuItem(hMenu, IDM_DEVICE + iOutDevice, MF_CHECKED);

			midiOutClose(hMidiOut);
			if (MMSYSERR_NOERROR == midiOutOpen(&hMidiOut, iOutDevice, 0, 0, 0))
				midiOutShortMsg(hMidiOut, 0x0C0 | (Instrument << 8));
		}

		else if ((LOWORD(wParam) >= (IDM_VELOCITY-1)) && (LOWORD(wParam) < (IDM_KEYS-1)))
		{
			CheckMenuItem(hMenu, 7 - (DefaultVelocity / 16) + IDM_VELOCITY, MF_UNCHECKED);
			DefaultVelocity = ((8 - (LOWORD(wParam)-IDM_VELOCITY)) * 16) - 1;
			WriteIni();
			CheckMenuItem(hMenu, LOWORD(wParam), MF_CHECKED);
		}

		else if ((LOWORD(wParam) >= (IDM_ACCIDENTAL-1)) && (LOWORD(wParam) < IDM_TEST))
		{
			CheckMenuItem(hMenu, IDM_ACCIDENTAL + accidental, MF_UNCHECKED);
			accidental = LOWORD(wParam) - IDM_ACCIDENTAL;
			CheckMenuItem(hMenu, IDM_ACCIDENTAL + accidental, MF_CHECKED);
			if (showkeys)
			{
				if (accidental == 1)
					*(WORD*)&Keys[6][0] = '#F';
				else//if (accidental == 2)
					*(WORD*)&Keys[6][0] = 'bG';
				DestroyWindow(hwndKeys[6]);
				hwndKeys[6] = CreateWindow("BUTTON", Keys[6],
					WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_DLGFRAME,
					xKeyLoc[6], yKeyLoc[6], 45, 30,
					hwnd, NULL, hInst, NULL);
			}
		}

		else if (LOWORD(wParam) == IDM_KEYS)
		{
			if (showkeys == FALSE)
			{
				showkeys = TRUE;
				ModifyMenu(hMenu, IDM_KEYS, MF_BYCOMMAND|MF_STRING, IDM_KEYS, HideKeys);
				DrawMenuBar(hwnd);
				if (accidental == 1)
					*(WORD*)&Keys[6][0] = '#F';
				else//if (accidental == 2)
					*(WORD*)&Keys[6][0] = 'bG';
				for (x = 0, z = top260-270; x < 12; x++)
				{
					hwndKeys[x] = CreateWindow("BUTTON", Keys[x],
						WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
						xKeyLoc[x], yKeyLoc[x], 45, 30,
						hwnd, NULL, hInst, NULL);
				}
				SendMessage(hwndKeys[KeyName], BM_SETCHECK, BST_CHECKED, 0);
				hwndShowScale = CreateWindow("BUTTON", "Show Scale",
					WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
					top260min30-(top260min30/2)-35, top260min30-(top260min30/2), 110, 30,
					hwnd, NULL, hInst, NULL);
				SendMessage(hwndShowScale, BM_SETCHECK, BST_UNCHECKED, 0);
			}
			else
			{
				showkeys = FALSE;
				showscale = FALSE;
				ChordType = 0;
				ModifyMenu(hMenu, IDM_KEYS, MF_BYCOMMAND|MF_STRING, IDM_KEYS, ShowKeys);
				DrawMenuBar(hwnd);
				for (x = 0; x < 12; x++)
				{
					DestroyWindow(hwndKeys[x]);
					hwndKeys[x] = NULL;
				}
				DestroyWindow(hwndShowScale);
				hwndKeys[0] = NULL;//flag
			}
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
		}

		else if (LOWORD(wParam) == IDM_CHORDS)
		{
			if (showchords == FALSE)
			{
				showchords = TRUE;
				ModifyMenu(hMenu, IDM_CHORDS, MF_BYCOMMAND|MF_STRING, IDM_CHORDS, HideChords);
				DrawMenuBar(hwnd);
				for (x = 0, z = 0; x < 9; x++, z += 30)
				{
					hwndChords[x] = CreateWindow("BUTTON", Chords[x],
						WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
						top260min30 + 50, z, 135, 30,
						hwnd, NULL, hInst, NULL);
				}
				SendMessage(hwndChords[ChordType], BM_SETCHECK, BST_CHECKED, 0);
				for (x = 0, z = 30; x < 3; x++, z += 30)
				{
					hwndInversions[x] = CreateWindow("BUTTON", Inversions[x],
						WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
						top260min30 + 50 + 135, z, 120, 30,
						hwnd, NULL, hInst, NULL);
				}
				hwndInversions[3] = CreateWindow("BUTTON", Inversions[3],
					WS_CHILD | WS_VISIBLE | BS_RADIOBUTTON | WS_DLGFRAME,
					top260min30 + 50 + 135, 150, 120, 30,
					hwnd, NULL, hInst, NULL);
				SendMessage(hwndInversions[Inversion], BM_SETCHECK, BST_CHECKED, 0);
			}
			else
			{
				showchords = FALSE;
				ModifyMenu(hMenu, IDM_CHORDS, MF_BYCOMMAND|MF_STRING, IDM_CHORDS, ShowChords);
				DrawMenuBar(hwnd);
				for (x = 0; x < 9; x++)
					DestroyWindow(hwndChords[x]);
				hwndChords[0] = NULL;//flag
				for (x = 0; x < 4; x++)
					DestroyWindow(hwndInversions[x]);
			}
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
		}

		else if (LOWORD(wParam) == IDM_TEST)
		{
			if (showtest == FALSE)
			{
				showtest = TRUE;
				ModifyMenu(hMenu, IDM_TEST, MF_BYCOMMAND|MF_STRING, IDM_TEST, StopNotes);
				DrawMenuBar(hwnd);
				testRect.left = top260min30 + 325;
				testRect.right = testRect.left + (2* StaffWidth[0]);
				testRect.top = top-462-59;
				testRect.bottom = top260;
				hdc = GetDC(hwnd);
				SelectObject(hdc, hMaestroFont);
				NoteTest();
				ReleaseDC(hwnd, hdc);
			}
			else//if (showtest)
			{
				SendMessage(hwnd, WM_COMMAND, IDM_ACCIDENTAL+saveAccidental, 0);
				showtest = FALSE;
				ModifyMenu(hMenu, IDM_TEST, MF_BYCOMMAND|MF_STRING, IDM_TEST, PlayNote);
				DrawMenuBar(hwnd);
				hdc = GetDC(hwnd);
				FillRect(hdc, &rect, hWhiteBrush);
				ReleaseDC(hwnd, hdc);
				InvalidateRect(hwnd, &rect, FALSE);
				UpdateWindow(hwnd);
			}
		}

//		else if (LOWORD(wParam) == IDM_UNSTICK)
//			InvalidateRect(hwnd, &rect, FALSE);

		else if (LOWORD(wParam) == IDM_ABOUT)
			MessageBox(hwnd, About, szAppName, MB_OK);

		else if (HIWORD(wParam) == BN_CLICKED)
		{
			if (showkeys)
			{
				for (x = 0; x < 12; x++)
				{
					if (lParam == (LONG)hwndShowScale)
					{
						if (showscale)
						{
							SendMessage(hwndShowScale, BM_SETCHECK, BST_UNCHECKED, 0);
							SavedNote = Scale[KeyName];
							ScanCode = Codes[SavedNote-36];//36 is the lowest MIDI note number in this program
							EndNote();
							showscale = FALSE;
						}
						else
						{
							showscale = TRUE;
							SendMessage(hwndShowScale, BM_SETCHECK, BST_CHECKED, 0);
							SavedNote = Scale[KeyName];
							ScanCode = Codes[SavedNote-36];//36 is the lowest MIDI note number in this program
							StartNote();
						}
						break;
					}
					else if (lParam == (LONG)hwndKeys[x])
					{
						SendMessage(hwndKeys[KeyName], BM_SETCHECK, BST_UNCHECKED, 0);
						KeyName = x;
						SendMessage(hwndKeys[KeyName], BM_SETCHECK, BST_CHECKED, 0);
//						if ((KeyName >= 1) && (KeyName <= 5))
						if ((KeyName >= 7) && (KeyName <= 11))
						{
							CheckMenuItem(hMenu, IDM_ACCIDENTAL + accidental, MF_UNCHECKED);
							accidental = 2;
							CheckMenuItem(hMenu, IDM_ACCIDENTAL + accidental, MF_CHECKED);
							*(WORD*)&Keys[6][0] = 'bG';
							DestroyWindow(hwndKeys[6]);
							hwndKeys[6] = CreateWindow("BUTTON", Keys[6],
								WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_DLGFRAME,
								xKeyLoc[6], yKeyLoc[6], 45, 30,
								hwnd, NULL, hInst, NULL);
						}
//						else if ((KeyName >= 7) && (KeyName <= 11))
						else if ((KeyName >= 1) && (KeyName <= 5))
						{
							CheckMenuItem(hMenu, IDM_ACCIDENTAL + accidental, MF_UNCHECKED);
							accidental = 1;
							CheckMenuItem(hMenu, IDM_ACCIDENTAL + accidental, MF_CHECKED);
							*(WORD*)&Keys[6][0] = '#F';
							DestroyWindow(hwndKeys[6]);
							hwndKeys[6] = CreateWindow("BUTTON", Keys[6],
								WS_CHILD | WS_VISIBLE | BS_AUTORADIOBUTTON | WS_DLGFRAME,
								xKeyLoc[6], yKeyLoc[6], 45, 30,
								hwnd, NULL, hInst, NULL);
						}
						InvalidateRect(hwnd, &rect, FALSE);
						UpdateWindow(hwnd);
						if (showscale)
						{
							SendMessage(hwndShowScale, BM_SETCHECK, BST_UNCHECKED, 0);
							SavedNote = Scale[KeyName];
							ScanCode = Codes[SavedNote-36];//36 is the lowest MIDI note number in this program
							EndNote();
							SendMessage(hwndShowScale, BM_SETCHECK, BST_CHECKED, 0);
							SavedNote = Scale[KeyName];
							ScanCode = Codes[SavedNote-36];//36 is the lowest MIDI note number in this program
							StartNote();
						}
						break;
					}
				}
			}
			if (showchords)
			{
				for (x = 0; x < 9; x++)
				{
					if (lParam == (LONG)hwndChords[x])
					{
						SendMessage(hwndChords[ChordType], BM_SETCHECK, BST_UNCHECKED, 0);
						ChordType = x;
						SendMessage(hwndChords[ChordType], BM_SETCHECK, BST_CHECKED, 0);
						break;
					}
				}
				for (x = 0; x < 4; x++)
				{
					if (lParam == (LONG)hwndInversions[x])
					{
						SendMessage(hwndInversions[Inversion], BM_SETCHECK, BST_UNCHECKED, 0);
						Inversion = x;
						SendMessage(hwndInversions[Inversion], BM_SETCHECK, BST_CHECKED, 0);
						break;
					}
				}
			}
			SetFocus(hwnd);
			for (x = 0; x < 40; x++)
				ScanCodes[x] = 0;//just in case
		}
		return 0;

	case WM_KEYUP:
		ScanCode = HIWORD(lParam) & 0x0FF;
		if (ScanCode <= 53)
		{
			SavedNote = Notes[ScanCode];
			if (SavedNote != -1)
				EndNote();
		}
		return 0;

	case WM_KEYDOWN:
		if (0x40000000 & lParam)//bit 30
			return 0;//ignore typematics
		ScanCode = HIWORD(lParam) & 0x0FF;
		if (ScanCode <= 53)
		{
			SavedNote = Notes[ScanCode];
			if (SavedNote != -1)
			{
				Velocity = DefaultVelocity;
				StartNote();
			}
		}
		return 0;

	case WM_NCLBUTTONDBLCLK:
		return 0;//because the WM_LBUTTONUP message is also sent with the wrong lParam valuse

	case WM_LBUTTONUP:
		x = (xPos-ExtraSpace) / WhiteKeyWidth;
		if ((x >= 4) && (x <= 25))
			ScanCode = MouseKeys[x];
		else if (x < 4)
			ScanCode = MouseKeys[x+26];
		else if (x > 25)
			ScanCode = MouseKeys[x+4];
		if (yPos > (top+100))
		{
			SavedNote = Notes[ScanCode];
			if (SavedNote != -1)
				EndNote();
		}
		else//if black key, get it from table
		{
			if (((xPos-ExtraSpace)%WhiteKeyWidth) > (WhiteKeyWidth*2/3))
				ScanCode = RKeys[x];
			else if (((xPos-ExtraSpace)%WhiteKeyWidth) < (WhiteKeyWidth/3))
				ScanCode = LKeys[x];
			if (ScanCode != -1)
			{
				SavedNote = Notes[ScanCode];
				if (SavedNote != -1)
					EndNote();
			}
		}
		return 0;

	case WM_LBUTTONDOWN:
		if ((yPos > (top)) && (yPos < (top+150)) && (xPos > ExtraSpace) && (xPos < (WhiteKeyWidth*35)))
		{
			x = (xPos-ExtraSpace) / WhiteKeyWidth;
			if ((x >= 4) && (x <= 25))
				ScanCode = MouseKeys[x];
			else if (x < 4)
				ScanCode = MouseKeys[x+26];
			else if (x > 25)
				ScanCode = MouseKeys[x+4];

			if (yPos > (top+100))
			{
				SavedNote = Notes[ScanCode];
				if (SavedNote != -1)
				{
					Velocity = DefaultVelocity;
					StartNote();
				}
			}
			else//if black key, get it from table
			{
				if (((xPos-ExtraSpace)%WhiteKeyWidth) > (WhiteKeyWidth*2/3))
					ScanCode = RKeys[x];
				else if (((xPos-ExtraSpace)%WhiteKeyWidth) < (WhiteKeyWidth/3))
					ScanCode = LKeys[x];
				if (ScanCode != -1)
				{
					SavedNote = Notes[ScanCode];
					if (SavedNote != -1)
					{
						Velocity = DefaultVelocity;
						StartNote();
					}
				}
			}
		}
		return 0;

	case WM_MOUSEMOVE:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		if (wParam == MK_LBUTTON)
		{
			if ((saveleft != -1) && (fromkeydown) && (ChordType == 0))
			{
				if ((yPos > top) && (yPos < (top+150)) && (xPos > ExtraSpace) && (xPos < (WhiteKeyWidth*35)))
				{
					if (yKey[ScanCode] == 0)//on a black key
					{
						if ((xPos < (saveleft-WhiteKeyWidth/3)) || (xPos > (saveleft+(WhiteKeyWidth/3))))
						{
							EndNote();
							SendMessage(hwnd, WM_LBUTTONDOWN, 0, lParam);
						}
					}
					else if (yKey[ScanCode] == 3)//on a white key with black keys on the left and right
					{
						if (yPos > (top+100))
						{
							if ((xPos < saveleft) || (xPos > (saveleft+WhiteKeyWidth)))
							{
								EndNote();
								SendMessage(hwnd, WM_LBUTTONDOWN, 0, lParam);
							}
						}
						else if ((xPos < (saveleft+WhiteKeyWidth/3)) || (xPos > (saveleft + (WhiteKeyWidth*2/3))))
						{
							EndNote();
							SendMessage(hwnd, WM_LBUTTONDOWN, 0, lParam);
						}
					}
					else if (yKey[ScanCode] == 1)//on a white key with a black key on the left
					{
						if (yPos > (top+100))
						{
							if ((xPos < saveleft) || (xPos > (saveleft+WhiteKeyWidth)))
							{
								EndNote();
								SendMessage(hwnd, WM_LBUTTONDOWN, 0, lParam);
							}
						}
						else if ((xPos < (saveleft+WhiteKeyWidth/3)) || (xPos > (saveleft + WhiteKeyWidth)))
						{
							EndNote();
							SendMessage(hwnd, WM_LBUTTONDOWN, 0, lParam);
						}
					}
					else if (yKey[ScanCode] == 2)//on a white key with a black key on the right
					{
						if (yPos > (top+100))
						{
							if ((xPos < saveleft) || (xPos > (saveleft+WhiteKeyWidth)))
							{
								EndNote();
								SendMessage(hwnd, WM_LBUTTONDOWN, 0, lParam);
							}
						}
						else if ((xPos < saveleft) || (xPos > (saveleft + (WhiteKeyWidth*2/3))))
						{
							EndNote();
							SendMessage(hwnd, WM_LBUTTONDOWN, 0, lParam);
						}
					}
				}
				else
					EndNote();
			}
		}
		return 0;

	case WM_USER:
		if (wParam == 0x80)//Note Off
		{
			wParam = 0x90;//trick
			lParam &= 0xFF;//trick
		}
		if (wParam == 0x90)//Note On or Off
		{
			Velocity = (lParam >> 8) & 0xFF;
			SavedNote = lParam & 0xFF;
			ScanCode = Codes[SavedNote-36];//36 is the lowest MIDI note number in this program
			if (Velocity)
			{
				Velocity += 25;
				if (Velocity > 127)
					Velocity = 127;
				StartNote();
			}
			else//0 velocity means end note
			{
				EndNote();
			}
		}
		return 0;

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (first)
		{
			first = FALSE;
			hOldFont = SelectObject(hdc, hMaestroFont);
			GetCharWidth32(hdc, 61, 61, StaffWidth);//61 is '='
			GetCharWidth32(hdc, 119, 119, NoteWidth);//119 is 'w'
			GetCharWidth32(hdc, 38, 38, GClefWidth);//38 is '&'
			GetCharWidth32(hdc, 63, 63, FClefWidth);//63 is '?'
			NumOfStaffs = rect.right / StaffWidth[0];
			if (NumOfStaffs > 128)
				NumOfStaffs = 128;
			for (x = 0; x < NumOfStaffs; x++)
				Staff[x] = '=';
			Staff[x] = 0;
			SelectObject(hdc, hSmallFont);
			for (x = 0; x < 22;x++)
			{
				GetCharWidth32(hdc, ComputerKeys1[x], ComputerKeys1[x], &Widths1[x]);
				GetCharWidth32(hdc, ComputerKeys2[x], ComputerKeys2[x], &Widths2[x]);
			}
			SelectObject(hdc, hFont);
			for (x = 0; x < 7;x++)
				GetCharWidth32(hdc, CDEFGAB[x], CDEFGAB[x], &Widths[x]);

			hdcMem = CreateCompatibleDC(hdc);
			hMemBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			SelectObject(hdcMem, hMemBitmap);
		}
		SelectObject(hdc, hMaestroFont);
		TextOut(hdc, ExtraSpace, top-212,Staff, NumOfStaffs);
		TextOut(hdc, ExtraSpace, top-121, Staff, NumOfStaffs);
		SetBkMode(hdc, TRANSPARENT);
		TextOut(hdc, ExtraSpace, top-224, "&", 1);
		TextOut(hdc, ExtraSpace-1, top-212, "\\", 1);
		TextOut(hdc, ExtraSpace, top-158, "?", 1);
		TextOut(hdc, ExtraSpace-1, top-121, "\\", 1);
		TextOut(hdc, ExtraSpace-1, top-169, "\\", 1);
//char Keys[12][3] = {"C ","F ","Bb","Eb","Ab","Db","Gb","B ","E ","A ","D ","G "};
//char Keys[12][3] = {"C ","G ","D ","A ","E ","B ","Gb","Db","Ab","Eb","Bb","F "};
		if ((showkeys) && (KeyName))
		{
			switch (KeyName)
			{
			case 5:
				TextOut(hdc, ExtraSpace+100, top-230, "#", 1);
				TextOut(hdc, ExtraSpace+100, top-127, "#", 1);
			case 4:
				TextOut(hdc, ExtraSpace+85, top-248, "#", 1);
				TextOut(hdc, ExtraSpace+85, top-145, "#", 1);
			case 3:
				TextOut(hdc, ExtraSpace+70, top-266, "#", 1);
				TextOut(hdc, ExtraSpace+70, top-163, "#", 1);
			case 2:
				TextOut(hdc, ExtraSpace+55, top-242, "#", 1);
				TextOut(hdc, ExtraSpace+55, top-139, "#", 1);
			case 1:
				TextOut(hdc, ExtraSpace+40, top-260, "#", 1);
				TextOut(hdc, ExtraSpace+40, top-157, "#", 1);
				break;
			case 6:
				if (accidental == 2)
				{
					TextOut(hdc, ExtraSpace+115, top-242, "b", 1);
					TextOut(hdc, ExtraSpace+115, top-139, "b", 1);
				}
				else//if (accidental == 1)
				{
					TextOut(hdc, ExtraSpace+115, top-254, "#", 1);
					TextOut(hdc, ExtraSpace+115, top-151, "#", 1);

					TextOut(hdc, ExtraSpace+100, top-230, "#", 1);
					TextOut(hdc, ExtraSpace+100, top-127, "#", 1);

					TextOut(hdc, ExtraSpace+85, top-248, "#", 1);
					TextOut(hdc, ExtraSpace+85, top-145, "#", 1);

					TextOut(hdc, ExtraSpace+70, top-266, "#", 1);
					TextOut(hdc, ExtraSpace+70, top-163, "#", 1);

					TextOut(hdc, ExtraSpace+55, top-242, "#", 1);
					TextOut(hdc, ExtraSpace+55, top-139, "#", 1);

					TextOut(hdc, ExtraSpace+40, top-260, "#", 1);
					TextOut(hdc, ExtraSpace+40, top-157, "#", 1);
					break;
				}
			case 7:
				TextOut(hdc, ExtraSpace+100, top-224, "b", 1);
				TextOut(hdc, ExtraSpace+100, top-121, "b", 1);
			case 8:
				TextOut(hdc, ExtraSpace+85, top-248, "b", 1);
				TextOut(hdc, ExtraSpace+85, top-145, "b", 1);
			case 9:
				TextOut(hdc, ExtraSpace+70, top-230, "b", 1);
				TextOut(hdc, ExtraSpace+70, top-127, "b", 1);
			case 10:
				TextOut(hdc, ExtraSpace+55, top-254, "b", 1);
				TextOut(hdc, ExtraSpace+55, top-151, "b", 1);
			case 11:
				TextOut(hdc, ExtraSpace+40, top-236, "b", 1);
				TextOut(hdc, ExtraSpace+40, top-133, "b", 1);
			}
		}
//		SetTextColor(hdc, 0xD0D0D0);
//		SelectObject(hdc, hFont);
//		if (showkeys == FALSE)
//			for (x = 0; x < 35; x++)
//				TextOut(hdc, (x*WhiteKeyWidth) + ExtraSpace + (WhiteKeyWidth-Widths[x%7])/2, top-139, &CDEFGAB[x % 7], 1);
		SelectObject(hdc, hSmallFont);
		if (showkeys == FALSE)
		{
			SetTextColor(hdc, 0x6060D0);
			TextOut(hdc, 50, top-200, "F", 1);
			TextOut(hdc, 50, top-188, "D", 1);
			TextOut(hdc, 50, top-176, "B", 1);
			TextOut(hdc, 50, top-164, "G", 1);
			TextOut(hdc, 50, top-152, "E", 1);

			TextOut(hdc, 65, top-205, "G", 1);
			TextOut(hdc, 65, top-193, "E", 1);
			TextOut(hdc, 65, top-181, "C", 1);
			TextOut(hdc, 65, top-169, "A", 1);
			TextOut(hdc, 65, top-157, "F", 1);
			TextOut(hdc, 65, top-145, "D", 1);

			TextOut(hdc, 50, top-109, "A", 1);
			TextOut(hdc, 50, top-97, "F", 1);
			TextOut(hdc, 50, top-85, "D", 1);
			TextOut(hdc, 50, top-73, "B", 1);
			TextOut(hdc, 50, top-61, "G", 1);

			TextOut(hdc, 65, top-114, "B", 1);
			TextOut(hdc, 65, top-102, "G", 1);
			TextOut(hdc, 65, top-90, "E", 1);
			TextOut(hdc, 65, top-78, "C", 1);
			TextOut(hdc, 65, top-66, "A", 1);
			TextOut(hdc, 65, top-54, "F", 1);

			TextOut(hdc, 50, top-130, "C", 1);
		}
		SetTextColor(hdc, 0x6060D0);
		for (x = ExtraSpace + (4*WhiteKeyWidth), z = 0; z < 10; x += WhiteKeyWidth, z++)
			TextOut(hdc, x+((WhiteKeyWidth-Widths1[z])/2), top + 155, &ComputerKeys1[z], 1);
		SetTextColor(hdc, 0xFF0000);//blue
		for ( ; z < 22; x += WhiteKeyWidth, z++)
			TextOut(hdc, x+((WhiteKeyWidth-Widths1[z])/2), top + 155, &ComputerKeys1[z], 1);
		SetBkMode(hdc, OPAQUE);

		for (x = (4*WhiteKeyWidth) + ExtraSpace-(WhiteKeyWidth/2), z = 0; z < 10; x += WhiteKeyWidth, z++)
		{
			if ((z == 3) || (z == 6) || (z == 10) || (z == 13) || (z == 17) || (z == 20))
				SetTextColor(hdc, 0xD0D0D0);
			else
				SetTextColor(hdc, 0x6060D0);//reddish
			TextOut(hdc, x+((WhiteKeyWidth-Widths2[z])/2), top - 22, &ComputerKeys2[z], 1);
		}
		SetTextColor(hdc, 0x6060D0);
		for ( ; z < 22; x += WhiteKeyWidth, z++)
		{
			if ((z == 3) || (z == 6) || (z == 10) || (z == 13) || (z == 17) || (z == 20))
				SetTextColor(hdc, 0xD0D0D0);
			else
				SetTextColor(hdc, 0xFF0000);//blue
			TextOut(hdc, x+((WhiteKeyWidth-Widths2[z])/2), top - 22, &ComputerKeys2[z], 1);
		}

		hOldPen = SelectObject(hdc, hPen);
		hOldBrush = SelectObject(hdc, hWhiteBrush);

		for (x = ExtraSpace; x < (35*WhiteKeyWidth); x += WhiteKeyWidth)
			Rectangle(hdc, x, top, x+WhiteKeyWidth, top+150);
		SelectObject(hdc, hBlackBrush);
		for (x = ExtraSpace; x < (ExtraSpace+(35*WhiteKeyWidth)); x += (7*WhiteKeyWidth))
		{
			left = x + BlackKeyWidth;
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
			left = x + (WhiteKeyWidth*5/3);
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
			left = x + (WhiteKeyWidth*11/3);
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
			left = x + (WhiteKeyWidth*14/3);
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
			left = x + (WhiteKeyWidth*17/3);
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
			left = -1;
		}
		if (showkeys == FALSE)
		{
			SetTextColor(hdc, 0xE4E4E4);
			SelectObject(hdc, hFont);
			for (x = 0; x < 35; x++)
				TextOut(hdc, (x*WhiteKeyWidth) + ExtraSpace + (WhiteKeyWidth-Widths[x%7])/2, top+110, &CDEFGAB[x % 7], 1);
		}
		SelectObject(hdc, hOldPen);
		SelectObject(hdc, hOldBrush);
		/////////////////////////////////////////////////////////////////
		BitBlt(hdcMem, 0, top260, rect.right, rect.bottom, hdc, 0, top260, SRCCOPY);
		/////////////////////////////////////////////////////////////////
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		RemoveFontResource("MAESTRO_.TTF");
		if (midi_in)
		{
			midiInStop(hMidiIn);
			midiInReset(hMidiIn);
			midiInClose(hMidiIn);
		}
		midiOutShortMsg(hMidiOut, 0xB0 | (123 << 8));//set controller for channel 0 to all voices off
		midiOutReset(hMidiOut);
		midiOutClose(hMidiOut);
		DeleteObject(hPen);
		DeleteObject(hWhiteBrush);
		DeleteObject(hBlackBrush);
		DeleteObject(hBlueBrush);
		DeleteObject(hFont);
		DeleteObject(hSmallFont);
		DeleteObject(hMaestroFont);
		DeleteObject(hMemBitmap);
		DeleteObject(hOldFont);
		DeleteObject(hOldPen);
		DeleteObject(hOldBrush);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

//SUBROUTINES////////////////////////////////////////////////////////////
void DrawKey(void)
{
	hdc = GetDC(hwnd);
//	{
//		char asdf[10];
//		_itoa(Note, asdf, 10);
//		TextOut(hdc, 0, 0, asdf, lstrlen(asdf));
//	}
	hOldPen = SelectObject(hdc, hPen);
	hOldFont = SelectObject(hdc, hMaestroFont);
	if (fromkeydown == FALSE)//clear notation
	{
		/////////////////////////////////////////////////////////////////
		BitBlt(hdc, 0, top260, rect.right, rect.bottom, hdcMem, 0, top260, SRCCOPY);
		BitBlt(hdc, rect.right-80, top260-20, 80, 20, hdc, rect.right-80, top260-40, SRCCOPY);//to clear parts of high accidentals
		/////////////////////////////////////////////////////////////////
		if ((showtest) && (Note == RandomNote))
			NoteTest();
	}
	left = (xKey[ScanCode] * WhiteKeyWidth) + ExtraSpace;
	saveleft = left;
	BracketingKeys = yKey[ScanCode];
	if (BracketingKeys == NEITHER)
	{//black keys
		if (fromkeydown)
		{
			SetTextColor(hdc, 0xFF0000);
			SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			if (accidental == 2)
				TextOut(hdc, left + (WhiteKeyWidth-Widths[xKey[ScanCode]%7])/2, top-139, &CDEFGAB[xKey[ScanCode]%7], 1);
			else//if (accidental == 1)
				if (ScanCode != 3) // not middle C
				TextOut(hdc, left - (WhiteKeyWidth+Widths[(xKey[ScanCode]-1)%7])/2, top-139, &CDEFGAB[(xKey[ScanCode]-1)%7], 1);
			SelectObject(hdc, hMaestroFont);
			if (accidental == 1)
				TextOut(hdc, left-(WhiteKeyWidth/3)-26, yStaff[Note-36], "#w", 2);
			else
				TextOut(hdc, left-(WhiteKeyWidth/3)+5, yStaff[Note-35], "bw", 2);
			SetBkMode(hdc, OPAQUE);
			SelectObject(hdc, hBlueBrush);
		}
		else
			SelectObject(hdc, hBlackBrush);
		SetTextColor(hdc, 0);//black
		left -= WhiteKeyWidth/3;
		Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
		SelectObject(hdc, hOldBrush);
		if (fromkeydown)
		{
			if (accidental == 1)
			{
				switch (xKey[ScanCode])
				{
				case 15://middle C#
//					MoveToEx(hdc, left-(BlackKeyWidth/3), top-122, NULL);//-1
//					LineTo(hdc, left+(BlackKeyWidth*4/3), top-122);//+29
					MoveToEx(hdc, left-BlackKeyWidth, top-122, NULL);//-1
					LineTo(hdc, left+WhiteKeyWidth-BlackKeyWidth, top-122);//+29
					break;
				case 1://low C#
					MoveToEx(hdc, left-BlackKeyWidth, top-28, NULL);
					LineTo(hdc, left+WhiteKeyWidth-BlackKeyWidth, top-28);
				case 2://low D#
					MoveToEx(hdc, left-BlackKeyWidth, top-40, NULL);
					LineTo(hdc, left+WhiteKeyWidth-BlackKeyWidth, top-40);
					break;
				case 33://very high G#
				case 34://very high A#
					MoveToEx(hdc, left-BlackKeyWidth, top-238, NULL);
					LineTo(hdc, left+WhiteKeyWidth-BlackKeyWidth, top-238);
				case 32://very high F#
					MoveToEx(hdc, left-BlackKeyWidth, top-226, NULL);
					LineTo(hdc, left+WhiteKeyWidth-BlackKeyWidth, top-226);
				case 30://high C#
				case 29://high D#
					MoveToEx(hdc, left-BlackKeyWidth, top-214, NULL);
					LineTo(hdc, left+WhiteKeyWidth-BlackKeyWidth, top-214);
				case 27://high A#
					MoveToEx(hdc, left-BlackKeyWidth, top-202, NULL);
					LineTo(hdc, left+WhiteKeyWidth-BlackKeyWidth, top-202);
					break;
				}
			}
			else//if (accidental == 2)
			{
				switch (Note)
				{
					case 37://low Db
					case 39://low Eb
//						MoveToEx(hdc, left-(BlackKeyWidth/3), top-40, NULL);
//						LineTo(hdc, left+(BlackKeyWidth*4/3), top-40);
						MoveToEx(hdc, left, top-40, NULL);
						LineTo(hdc, left+WhiteKeyWidth, top-40);
						break;
						MoveToEx(hdc, left, top-202, NULL);
						LineTo(hdc, left+WhiteKeyWidth, top-202);
						break;
					case 94://very high Bb
						MoveToEx(hdc, left, top-250, NULL);
						LineTo(hdc, left+WhiteKeyWidth, top-250);
					case 92://very high Ab
					case 90://high Gb
						MoveToEx(hdc, left, top-238, NULL);
						LineTo(hdc, left+WhiteKeyWidth, top-238);
					case 87://high Db
						MoveToEx(hdc, left, top-226, NULL);
						LineTo(hdc, left+WhiteKeyWidth, top-226);
					case 85://high Cb
						MoveToEx(hdc, left, top-214, NULL);
						LineTo(hdc, left+WhiteKeyWidth, top-214);
					case 80://high Ab
					case 82://high Bb
						MoveToEx(hdc, left, top-202, NULL);
						LineTo(hdc, left+WhiteKeyWidth, top-202);
						break;

				}
			}
		}
	}
	else if (BracketingKeys != -1)
	{//white keys
		if (fromkeydown)
		{
			SetTextColor(hdc, 0xFF0000);
			SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			if (ScanCode != 16) // not middle C
				TextOut(hdc, left + (WhiteKeyWidth-Widths[xKey[ScanCode]%7])/2, top-139, &CDEFGAB[xKey[ScanCode]%7], 1);
			SetBkMode(hdc, OPAQUE);

			SetTextColor(hdc, 0);
			SelectObject(hdc, hMaestroFont);
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, 0xFF0000);
//			if (ScanCode != 16) // not middle C
				TextOut(hdc, left + (WhiteKeyWidth-Widths[xKey[ScanCode]%7])/2, yStaff[Note-36], "w", 1);
			SelectObject(hdc,hMaestroFont);
			SetTextColor(hdc, 0);
			switch (xKey[ScanCode])
			{
			case 14://middle C
				MoveToEx(hdc, left, top-122, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-122);
				break;
			case 0://low C
				MoveToEx(hdc, left, top-28, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-28);
				MoveToEx(hdc, left, top-40, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-40);
				break;
			case 1://low D
			case 2://low E
				MoveToEx(hdc, left, top-40, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-40);
				break;
			case 34://very high B
				MoveToEx(hdc, left, top-250, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-250);
			case 32://very high G
			case 33://very high A
				MoveToEx(hdc, left, top-238, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-238);
			case 30://high E
			case 31://high F
				MoveToEx(hdc, left, top-226, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-226);
			case 28://high C
			case 29://high D
				MoveToEx(hdc, left, top-214, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-214);
			case 26://high A
			case 27://high B
				MoveToEx(hdc, left, top-202, NULL);
				LineTo(hdc, left+WhiteKeyWidth, top-202);
				break;
			}
			SetBkMode(hdc, OPAQUE);
			SelectObject(hdc, hOldFont);
			SelectObject(hdc, hBlueBrush);
		}
		Rectangle(hdc, left, top, left+WhiteKeyWidth, top+150);
		if ((!fromkeydown) && (showkeys == FALSE))
		{
			SetTextColor(hdc, 0xE4E4E4);
			SelectObject(hdc, hFont);
			x = xKey[ScanCode];
			TextOut(hdc, (x*WhiteKeyWidth) + ExtraSpace + (WhiteKeyWidth-Widths[x%7])/2, top+110, &CDEFGAB[x % 7], 1);
		}
		if (PreviousNote != (Note-1))
			SelectObject(hdc, hBlackBrush);
		else
			SelectObject(hdc, hBlueBrush);
		if (BracketingKeys == LEFT)
		{
			left -= (WhiteKeyWidth/3);
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
		}
		else if (BracketingKeys == RIGHT)
		{
			left += (WhiteKeyWidth*2/3);
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
		}
		else if (BracketingKeys == BOTH)
		{
			left -= (WhiteKeyWidth/3);
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
			left += WhiteKeyWidth;
			Rectangle(hdc, left, top, left+BlackKeyWidth, top+100);
		}
		SelectObject(hdc, hOldBrush);
	}
	SelectObject(hdc, hOldPen);
	SelectObject(hdc, hOldFont);
	ReleaseDC(hwnd, hdc);
}

void StartShowAndPlay(void)
{
	midiOutShortMsg(hMidiOut, 0x90 | (Velocity << 16) | (Note << 8));
	fromkeydown = TRUE;
	if ((Note >= 36) && (Note <= 96))
	{
		DrawKey();
		for (x = 0; (x < 40) && (ScanCodes[x] != 0); x++)
			;
		if (x < 40)
			ScanCodes[x] = ScanCode;
	}
}

void StartNote(void)
{//play note
	if (showscale)//All Notes in Scale
	{
		saveAccidental = accidental;
		if ((SavedNote == 41) || (SavedNote == 53) || (SavedNote == 65) || (SavedNote == 77) || (SavedNote == 89))
			accidental = 2;//Key of F
		else if ((SavedNote != 42) && (SavedNote != 54) && (SavedNote != 66) && (SavedNote != 78) && (SavedNote != 90))
		{
			accidental = 1;
			for (x = 0; x < 25; x++)
			{
				if (SavedNote == BlackKeyNotes[x])
				{
					accidental = 2;//if it's a flat key
					break;
				}
			}
		}
		for (x = 0; x < 7; x++)
		{
			Note = SavedNote + Interval[x];
			ScanCode = Codes[Note-36];
			fromkeydown = TRUE;
			if ((Note >= 36) && (Note <= 96))
				DrawKey();
			PreviousNote = Note;
		}
		PreviousNote = -1;
		accidental = saveAccidental;
		return;
	}

//	if ((showtest) && (SavedNote == 95))
//		SendMessage(hwnd, WM_COMMAND, IDM_TEST, 0);
	Note = SavedNote;
	StartShowAndPlay();
	if (ChordType == 1)//Major Triad
	{
		Note = SavedNote + 4;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			StartShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				StartShowAndPlay();
			}
		}
	}
	else if (ChordType == 2)//Minor Triad
	{
		Note = SavedNote + 3;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			StartShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				StartShowAndPlay();
			}
		}
	}
	else if (ChordType == 3)//Diminished Triad
	{
		Note = SavedNote + 3;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			StartShowAndPlay();
			Note = SavedNote + 6;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				StartShowAndPlay();
			}
		}
	}
	else if (ChordType == 4)//Augmented Triad
	{
		Note = SavedNote + 4;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			StartShowAndPlay();
			Note = SavedNote + 8;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				StartShowAndPlay();
			}
		}
	}
	else if (ChordType == 5)//Dominant 7th
	{
		Note = SavedNote + 4;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			StartShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				StartShowAndPlay();
				Note = SavedNote + 10;
				if ((Inversion == 1) || (Inversion == 2) || (Inversion == 3))
					Note -= 12;
				if (Note <= 96)
				{
					ScanCode = Codes[Note-36];
					StartShowAndPlay();
				}
			}
		}
	}
	else if (ChordType == 6)//Major 7th
	{
		Note = SavedNote + 4;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			StartShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				StartShowAndPlay();
				Note = SavedNote + 11;
				if ((Inversion == 1) || (Inversion == 2) || (Inversion == 3))
					Note -= 12;
				if (Note <= 96)
				{
					ScanCode = Codes[Note-36];
					StartShowAndPlay();
				}
			}
		}
	}
	else if (ChordType == 7)//Minor 7th
	{
		Note = SavedNote + 3;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			StartShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				StartShowAndPlay();
				Note = SavedNote + 10;
				if ((Inversion == 1) || (Inversion == 2) || (Inversion == 3))
					Note -= 12;
				if (Note <= 96)
				{
					ScanCode = Codes[Note-36];
					StartShowAndPlay();
				}
			}
		}
	}
	else if (ChordType == 8)//Diminished 7th
	{
		Note = SavedNote + 3;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			StartShowAndPlay();
			Note = SavedNote + 6;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				StartShowAndPlay();
				Note = SavedNote + 9;
				if ((Inversion == 1) || (Inversion == 2) || (Inversion == 3))
					Note -= 12;
				if (Note <= 96)
				{
					ScanCode = Codes[Note-36];
					StartShowAndPlay();
				}
			}
		}
	}
}

void EndShowAndPlay(void)
{
	midiOutShortMsg(hMidiOut, 0x90 | (Note << 8));//0 Velocity
	fromkeydown = FALSE;
	if ((Note >= 36) && (Note <= 96))
		DrawKey();
	for (x = 0; (x < 40) && (ScanCodes[x] != ScanCode); x++)
		;
	if (x < 40)
		ScanCodes[x] = 0;
	else
		for (x = 0; x < 40; x++)
			ScanCodes[x] = 0;
	fromkeydown = TRUE;
	if (ChordType == 0)
	{
		for (x = 0; x < 40; x++)
		{
			if (ScanCodes[x] != 0)
			{
				ScanCode = ScanCodes[x];
				Note = Notes[ScanCode];
				DrawKey();//doesn't use x
			}
		}
	}
}

void EndNote(void)
{
	if (showscale)//All Notes in Scale
	{
		saveAccidental = accidental;
		accidental = 1;
		for (x = 0; x < 25; x++)
			if (SavedNote == BlackKeyNotes[x])
			{
				accidental = 2;//if it's a flat key
				break;
			}
		for (x = 0; x < 7; x++)
		{
			Note = SavedNote + Interval[x];
			ScanCode = Codes[Note-36];
			fromkeydown = FALSE;
			if (Note <= 96)
				DrawKey();
			PreviousNote = Note;
		}
		PreviousNote = -1;
		accidental = saveAccidental;
		return;
	}

	Note = SavedNote;
	EndShowAndPlay();
	if (ChordType == 1)
	{
		Note = SavedNote + 4;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			EndShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				EndShowAndPlay();
			}
		}
	}
	else if (ChordType == 2)
	{
		Note = SavedNote + 3;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			EndShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				EndShowAndPlay();
			}
		}
	}
	else if (ChordType == 3)
	{
		Note = SavedNote + 3;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			EndShowAndPlay();
			Note = SavedNote + 6;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				EndShowAndPlay();
			}
		}
	}
	else if (ChordType == 4)
	{
		Note = SavedNote + 4;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			EndShowAndPlay();
			Note = SavedNote + 8;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				EndShowAndPlay();
			}
		}
	}
	else if (ChordType == 5)
	{
		Note = SavedNote + 4;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			EndShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				EndShowAndPlay();
				Note = SavedNote + 10;
				if ((Inversion == 1) || (Inversion == 2) || (Inversion == 3))
					Note -= 12;
				if (Note <= 96)
				{
					ScanCode = Codes[Note-36];
					EndShowAndPlay();
				}
			}
		}
	}
	else if (ChordType == 6)
	{
		Note = SavedNote + 4;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			EndShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				EndShowAndPlay();
				Note = SavedNote + 11;
				if ((Inversion == 1) || (Inversion == 2) || (Inversion == 3))
					Note -= 12;
				if (Note <= 96)
				{
					ScanCode = Codes[Note-36];
					EndShowAndPlay();
				}
			}
		}
	}
	else if (ChordType == 7)//Minor 7th
	{
		Note = SavedNote + 3;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			EndShowAndPlay();
			Note = SavedNote + 7;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				EndShowAndPlay();
				Note = SavedNote + 10;
				if ((Inversion == 1) || (Inversion == 2) || (Inversion == 3))
					Note -= 12;
				if (Note <= 96)
				{
					ScanCode = Codes[Note-36];
					EndShowAndPlay();
				}
			}
		}
	}
	else if (ChordType == 8)//Diminished 7th
	{
		Note = SavedNote + 3;
		if (Inversion == 1)
			Note -= 12;
		if (Note <= 96)
		{
			ScanCode = Codes[Note-36];
			EndShowAndPlay();
			Note = SavedNote + 6;
			if ((Inversion == 1) || (Inversion == 2))
				Note -= 12;
			if (Note <= 96)
			{
				ScanCode = Codes[Note-36];
				EndShowAndPlay();
				Note = SavedNote + 9;
				if ((Inversion == 1) || (Inversion == 2) || (Inversion == 3))
					Note -= 12;
				if (Note <= 96)
				{
					ScanCode = Codes[Note-36];
					EndShowAndPlay();
				}
			}
		}
	}
}

void CALLBACK MidiInProc(HMIDIIN hMidiIn, WORD wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{//from midiInOpen (if a MIDI keyboard is attached)
	if (wMsg == MIM_DATA)
		PostMessage(hwnd, WM_USER, (WPARAM)dwParam1 & 0xFF, (LPARAM)(dwParam1 >> 8) & 0xFFFF);//dwParam1 contains velocity, note, and status bytes
}

int Atoi(char *ptr)
{
	int x;

	for (x = 0; (*ptr >= '0') && (*ptr <= '9'); ptr++)
	{
		x *= 10;
		x += *ptr - '0';
	}
	return x;
}

void WriteIni(void)
{
	hFile = CreateFile(SimplePianoIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (iInDevice != 0)
	{
		WriteFile(hFile, InDev, 13, &dwBytesWritten, NULL);
		_itoa(iInDevice, temp, 10);
		WriteFile(hFile, temp, lstrlen(temp),&dwBytesWritten, NULL);
		WriteFile(hFile, "\r\n", 2, &dwBytesWritten, NULL);
	}
	WriteFile(hFile, OutDev, 14, &dwBytesWritten, NULL);
	_itoa(iOutDevice, temp, 10);
	WriteFile(hFile, temp, lstrlen(temp),&dwBytesWritten, NULL);
	WriteFile(hFile, "\r\n", 2, &dwBytesWritten, NULL);
	WriteFile(hFile, Vel, 17, &dwBytesWritten, NULL);
	_itoa(DefaultVelocity, temp, 10);
	WriteFile(hFile, temp, lstrlen(temp),&dwBytesWritten, NULL);
	WriteFile(hFile, "\r\n", 2, &dwBytesWritten, NULL);
	CloseHandle(hFile);
}

double uniform_deviate(int seed)
{
	return seed * (1.0 / (RAND_MAX + 1.0));
}

void NoteTest(void)
{
	saveAccidental = accidental;
	do
	{
		RandomNote = (int)(36.0 + uniform_deviate(rand()) * (85.0-36.0));//for Notes 36 thru 84 (from Julienne Walker's Eternally Confuzzled website)
		if (RandomNote & 1)//odd number
			accidental = 1;
		else
			accidental = 2;
	} while (RandomNote == PreviousRandomNote);
	PreviousRandomNote = RandomNote;
	RandomNoteLoc = yStaff[RandomNote-36] - 250;

	FillRect(hdc, &testRect, hWhiteBrush);
	SetBkMode(hdc, TRANSPARENT);
	TextOut(hdc, top260min30 + 325, top-462, "==", 2);
	TextOut(hdc, top260min30 + 325, top-371, "==", 2);
	TextOut(hdc, top260min30 + 325, top-462-12, "&", 1);
	TextOut(hdc, top260min30 + 325, top-371-37, "?", 1);
	if (RandomNote <= 40)
		TextOut(hdc, top260min30 + 325 + StaffWidth[0], top-371+12, "__", 2);
	if ((RandomNote == 36) || (RandomNote == 37))
		TextOut(hdc, top260min30 + 325 + StaffWidth[0], top-371+24, "__", 2);
	if ((RandomNote == 60) || (RandomNote == 61))
		TextOut(hdc, top260min30 + 325 + StaffWidth[0], top-462+21, "__", 2);
	if (RandomNote >= 80)
		TextOut(hdc, top260min30 + 325 + StaffWidth[0], top-462-59, "__", 2);
	if (RandomNote == 84)
		TextOut(hdc, top260min30 + 325 + StaffWidth[0], top-462-71, "__", 2);

	for (x = 0; x < 25; x++)
	{
		if (RandomNote == BlackKeyNotes[x])
		{
			if (accidental == 1)
				TextOut(hdc, top260min30 + 325 + (StaffWidth[0]+(NoteWidth[0]/2)), RandomNoteLoc, "#w", 2);
			else
				TextOut(hdc, top260min30 + 325 + (StaffWidth[0]+(NoteWidth[0]/2)), RandomNoteLoc-6, "bw", 2);
			break;
		}
	}
	if (x == 25)//not a # or b
		TextOut(hdc, top260min30 + 325 + (StaffWidth[0]+(NoteWidth[0]/2)), RandomNoteLoc, "w", 1);
	SetBkMode(hdc, OPAQUE);
}
