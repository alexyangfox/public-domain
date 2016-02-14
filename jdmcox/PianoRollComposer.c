//’ using Microsoft's Visual C/C++ 6.0 (with the Feb 2003 MSDN library of API's)
// PianoRollComposer.rc and PianoRollComposer.h (rename to resource.h) can also be downloaded from jdmcox.com
#include <windows.h>
#include <mmsystem.h>// add winmm.lib to Project -Settings -Link
#include <mmreg.h> // where WAVEFORMATEXTENSIBLE is defined
#include <ks.h> // required by KsMedia.h
#include <KsMedia.h> // mulitmedia GUID info
#include <commctrl.h>// for sliders (put comctl32.lib in Project/Settings/Link)
#include <math.h>// for modf
#include <wininet.h>//put wininet.lib in Project/Settings/Link
#include "resource.h"// rename PianoRollComposer.h to resource.h (at jdmcox.com, along with PianoRollComposer.rc)

#define PI 3.141592653589793
#define WM_USER2 WM_USER+1
#define WM_USER3 WM_USER2+1
#define WM_USER4 WM_USER3+1
#define WM_USER5 WM_USER4+1
#define WM_USER6 WM_USER5+1
#define WM_USER7 WM_USER6+1
#define WM_USER8 WM_USER7+1
#define WM_USER9 WM_USER8+1
#define WM_USER10 WM_USER9+1
#define WM_USER11 WM_USER10+1
#define WM_USER12 WM_USER11+1
#define WM_USER13 WM_USER12+1
#define WM_USER14 WM_USER13+1
#define WM_USER20 WM_USER14+1
#define WM_USER21 WM_USER20+1
#define WM_USER22 WM_USER21+1
#define MAX_MIDI 500000
#define MAX_EVENT 100000
#define TIMER_RESOLUTION 1
#define BLACK 0
#define WHITE 0xFCFCFC
#define RED 0x6878F8
#define BLUE 0xF0A080
#define GREEN 0x00E000
#define YELLOW 0xF8F8
#define BLUEGREEN 0xD8D800
#define VIOLET 0xFF80FF
#define ORANGE 0x80C0FF
#define BROWN 0x60A0D0
#define GREY 0xA0A0A0
#define DIALOGCOLOR 0xD8EDEF // 0xD8E9EC
#define WHOLE 160
#define HALF 80
#define QUARTER 40
#define EIGHTH 20
#define SIXTEENTH 10
#define THIRTYSECOND 5
#define NOCOPY 0
#define COPY 1
#define COPYBUTTONDOWN 2
#define COPYBUTTONUP 3
#define MAJOR_TRIAD 1
#define MAJOR_7TH 2
#define MAJOR_13TH 3
#define TOTAL_CHORDS 156
#define TENTHOUSAND 10000
#define THOUSAND 1000
#define HUNDRED 100
#define TEN 10
#define IDM_INPUT 50000
#define IDM_OUTPUT 60000
#define initialFilterFc 8
#define modEnvToFilterFc 11
#define delayVolEnv 33
#define attackVolEnv 34
#define holdVolEnv 35
#define decayVolEnv 36
#define sustainVolEnv 37
#define releaseVolEnv 38
#define Instrument 41
#define keyRange 43
#define velRange 44
#define coarseTune 51
#define fineTune 52
#define sampleID 53
#define overridingRootKey 58
#define BLOCK_SIZE 8192
#define BLOCK_COUNT 2
#define VOICES 36
#define SIXTEENTWELVE 1612
#define TOTALINES 26
#define PLAYING 2
#define PAUSED 1
#define STOPPED 0
#define MAX_PLAYLIST 36

int current, RealNote;
DWORD Data1, Data2, EWQLfileSize[16];
DWORD FromChannel, ToChannel, FromPort, ToPort;
UINT StopID;
SHORT s; // for merging wave files in MixerProc
BYTE VirtualActiveChannel = 0xFF, NumberOfPorts = 0, Port = 0, port, OnlyActiveChannel, ol, no = 0; // for NotesOn when virtualkeyboard
BYTE OldChannel, ReplacementChannel, OldPort = 0, ReplacementPort = 0, PortamentoRate, NoteEvery3, NewVelocity;
BYTE Sustenuto[108]; // 88 notes + 21 because note numbers begin at 21
BOOL EWQL = FALSE, inifile, shiftpressed, ctrlpressed, changingvelocity = FALSE, sustenuto = FALSE, sustenutoDown = FALSE;
char ChannelNumber[256] = "Channel xx ";
char InstrumentBufs[16][512]; // 16 ports max and 512 chars max and 32 chars/instrument
char Ports[] = "Port       ";

struct {
	char name[32];
	int start;
	int end;
	int octaveshift;
	int textstart;
	int textlength;
	BYTE channel;
	BYTE port;
	BYTE firstnote;
	BYTE lastnote;
} AssignedInstruments[8];
int ai = 0;

int KeyWidth, ExtraSpace, note, velocity = -1, KeyboardOffset = 12;
int BlackKeyNotes[52] = {0,22,0,25,27,0,30,32,34,0,37,39,0,42,44,46,0,49,51,0,54,56,58,0,61,63,0,66,68,70,0,73,75,0,78,80,82,0,85,87,0,90,92,94,0,97,99,0,102,104,106};
int WhiteKeyNotes[52] = {21,23,24,26,28,29,31,33,35,36,38,40,41,43,45,47,48,50,52,53,55,57,59,60,62,64,65,67,69,71,72,74,76,77,79,81,83,84,86,88,89,91,93,95,96,98,100,101,103,105,107,108};
unsigned char ComputerKeysDown1[23];
unsigned char ComputerKeysDown2[23];
unsigned char ComputerKeysNote1[23];
unsigned char ComputerKeysNote2[23];
char KeyLetter[52] = "ABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABCDEFGABC";
int KeyLoc[88], RealKeyLoc[88];
HDC hdc3, hdc33;
PAINTSTRUCT ps3;
RECT rect3, rect4; // rect3 is for VirtualKeyboard and rect4 is for ActiveInstruments
HBRUSH hBrush3;

int MetronomeChannel = 10, MetronomePort = 0, MetronomeNote = 31; // 31 is PercussionNote
int PixelsBetweenLines = 12, PixelsInGrandStaff, MyExtraSpace, PixelsInGrandStaffAndMyExtraSpace, thisRow, newY, NoteTop, NoteMiddle, BottomNoteTop, Bank;
int Row1Top, Row2Top, Row3Top, RowTop, MinPitchBend, OrigPixelsBetweenLines, Beginning, Ending, BeginEnd, PTR, MeasureNumber, playlist, screenend, PreviousNote;
int InstrumentNum;
DWORD LinesPageRowsPixel, nco, ptrNoteCarryOver;
DWORD NoteCarryOver[100][256]; // Page < 98 and nco < 256
WORD volBegin, volEnd;
BOOL blackbackground, inwhite, onegrandstaff = FALSE, onlynotenames = FALSE, musicflag = FALSE, fromuser14 = FALSE, overlapped, showaspace = FALSE;
BOOL noending, badending, pianoMetronome = FALSE, beginning, fromeditmywave = FALSE, firstkeysignature = TRUE, myPercussion = FALSE;
BOOL virtualkeyboard, playflag = STOPPED, showingstop = FALSE, button2pressed = FALSE, fromplaylist = FALSE;
//char Diatonic[12][12][5]; // for ChordsProc
char UserBuf[5000];
char InstrumentBuf[6000];
char myInstruments[130][32];
char InstrumentRanges[128][8]; // e.g. 36-108
char PercussionBuf[1024];
char ActiveInstruments[] = "Active Instrument";
char VirtualKeyboard[] = "Virtual Keyboard";

struct {
	int note;
	int pixel;
} NoEnding[10];
int ne;
struct {
	int note;
	int pixel;
} BadEnding[10];
int be;
char lineSpace[4];
char Left[] = "1";
char Right[] = "1";
COLORREF PrevColor;

char About[] = "Updated at 7:30am June 1, 2012\n\nhttp://jdmcox.com/\n\nDoug Cox\n\njdmcox@jdmcox.com";
char szAppName[] = "PianoRollComposer 2.47";
char TitleName[256];

char MidiIn[6][256];
char MidiOut[6][256];
char MidiOutIni[256];
char MidiInIni[256];

BOOL sustain = FALSE, Sustained[VOICES], onstaff;
HBRUSH hB1, hB2, hB3, hBlueBrush, Brushes[128];
LPDRAWITEMSTRUCT lpdis, lpdis2;

int NewChannel, AddedVelocity, ConstantVelocity = 0, MetronomeVolume = 40, MIDImenu;
int XLoc, YLoc, shownotesX, shownotesY, LastInputDevice = 0, LastOutputDevice;
DWORD oldTickptr, newTickptr;
BYTE hex1, hex2, SysExLen;
BYTE SysEx[128];
char cSysEx[128];
char Char[2];
char metronomeVolume[4];
BOOL shownotesplayed = FALSE, sysex = FALSE, onelesspixel = FALSE, ctrlA = FALSE, wasctrlA = FALSE, alterbeatperminute = FALSE;

int Beats, Chords2Notes[12], cn;
int Notes2Chords[12], nc;
int ThisX[2], tx, Frame, TitleAndMenu, Title, Menu;
int Editing = -1, EditingBeginX = -1, EditingBeginY, EditingEndX, EditingEndY;
int vol, volchange = 0, origVol[16];
DWORD message, instrument, tickptr, thisTickptr;
BYTE chan;
double oldMilliSecondsPerTick, originalMilliSecondsPerTick, TempoChange = 0.0, alpha;
BOOL newTempo = FALSE, keepgoing = FALSE, fromoptions = FALSE, fromsave = FALSE, bigopen = FALSE, playingwave = FALSE, playing2wavefiles = FALSE;
BOOL gotc[16], gotchannel[16], overNote = FALSE, chords2enter = FALSE, keyboardpercus = FALSE, saveit = FALSE, recordingtowave = FALSE, recordedtowave = TRUE, mute = FALSE;

struct {
	DWORD pixel;// beginning of line pixel (equates to an Event[x].pixel at the beginning of a line)
	DWORD PixelsPerPage;
	DWORD FirstMeasureNumber;
	WORD PixelsPerLine;
	WORD rowonpage;
	BYTE BeatsPerMeasure[64];// e.g.6
	BYTE PixelsPerBeat[64];// = Pixels/Measure / Beats/Measure
	WORD PixelsPerMeasure[64];// = Beats/Measure * Pixels/Beat
	BYTE KeySignature[64];
} Lines[300];
int line, l, L;
WORD LinePixels, ThisLinePixels;

int TicksPerPixel = 12;
BYTE PixelsPerBeat = 40;// Beat = Quarter Note
WORD TicksPerQuarterNote = 480;
WORD MidiFormat, MidiTracks, tracks, ticksinnote;
BYTE MetaEventType, MidiNote, MidiVelocity, ch, counter, Channel;
BYTE Controller, ControllerValue, MidiInstrument, Aftertouch, ChannelEvent;
BYTE TimeNumerator, BeatNoteType = 4, BeatsPerMeasure = 4, ClocksPerBeat, ThirtysecondNotesPer24Clocks;
BYTE prevBeatNoteType, prevPixelsPerBeat;
BYTE Powers[] = {1, 2, 4, 8, 16, 32};
BYTE Midi[MAX_MIDI];
BYTE thisInstrument;
BYTE KeySignature;
char notetype[] = " Q   ";//spaces at [3] and [4] are to clean up changed [2]
double dMilliSecondsPerTick, dMultiplier, d, d2, d3, d4;

// The numbers below are multiplied by PixelsBetweenLines, then divided by 2,
// and then subtracted from (PixelsBetweenLines * TOTALINES) + MyExtraSpace - (PixelsBetweenLines/2) - (PixelsBetweenLines/4)
// to get the numbers for NoteLoc when PixelsBetweenLines is changed in OptionsProc.
int UpFrom21[]={  0,  0,  1,  2,  2,  3,  3,  4,  5,  5,  6,  6,  7,  7,  8,  9,  9, 10, 10, 11, 12, 12, 13, 13, 14, 14, 15, 16, 16, 17, 17, 18, 19, 19, 20, 20, 21, 21, 22, 23, 23, 24, 24, 25, 26, 26, 27, 27, 28, 28, 29, 30, 30, 31, 31, 32, 33, 33, 34, 34, 35, 35, 36, 37, 37, 38, 38, 39, 40, 40, 41, 41, 42, 42, 43, 44, 44, 45, 45, 46, 47, 47, 48, 48, 49, 49, 50, 51};
//MIDI note num: 21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97 98  99  100  101 102 103 104 105 106 107 108
int NoteLoc[] ={337,337,331,325,325,319,319,313,307,307,301,301,295,295,289,283,283,277,277,271,265,265,259,259,253,253,247,241,241,235,235,229,223,223,217,217,211,211,205,199,199,193,193,187,181,181,175,175,169,169,163,157,157,151,151,145,139,139,133,133,127,127,121,115,115,109,109,103, 97, 97, 91, 91, 85, 85, 79, 73, 73, 67, 67, 61, 55, 55, 49, 49, 43, 43, 37, 31};
char Letter[] ={'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C'};// 88 notes

WORD SharpSigs[5];
WORD FlatSigs[5];

char Lyric[64];

struct EVENT {
	DWORD pixel;
	DWORD pixelsinnote;
	DWORD tickptr;
	DWORD ticksinnote;
	DWORD message;
	int time;
	double dMilliSecondsPerTick;
	BYTE note;
	BYTE velocity;
	BYTE sharporflat;
	BYTE channel;
	BYTE port;
	BYTE BeatsPerMeasure;
	BYTE BeatNoteType;
	BYTE KeySignature;
	BYTE type;// MetaEventType  also used as flag
	BYTE overlapped;
	WORD len;// text length
	WORD ptr;// text
	char finger[2];
} Event[MAX_EVENT], copyEvent[MAX_EVENT], TempEvent, ChordEvent[12], *UndoEvent[10000], resortEvent[200];

DWORD e, timePtr, savetimePtr, ptr, lastE, copySize = 0, BeginSustain, lastNote, u, pUndo, LastEventInUndo[10000], Loop, TempPixel, originalE, EventPixels[MAX_EVENT], EventMessage;
int LyricLen;
WORD dataptr;
DWORD dataptr24, dp24;
BYTE Data[0xFFFF];// for lyrics, etc

struct {
	DWORD tickptr;
	DWORD xOn;
	BYTE note;
	BYTE channel;
} OnOff[640];
int on;

int channel, InstrumentNum;
DWORD InstrumentOffset[16][16];
BYTE ActiveChannel = 0, ActiveChannels[16], ActiveInstrument = 0, ChannelInstruments[16][16], StereoLocations[16], saveActiveChannel, OrigActiveChannel, OrigActiveInstrument;

int n, v, w, x, y, saveX, saveX2, IniBufSize = 0, insertNote, extraX, start, end, thisX, xOffset, DrumSet = 0, ChosenDrumSet, Keysig, offset, bt, lb, firstLB;
int mSecondsPerBeat, InitialBeatsPerMinute = 120, beatsperminute, BPM, BPMchange, insertedN, PageBegin = 0, lastRowLoc, Rows = 1, iNumDevs, middle;
int notebottom, notend, Row, tempRow, ButtonDownRow, ButtonUpRow, ButtonDownPage, Page = 0, PedalDownPage, PedalDownl, Note, OriginalNote, previousNote = 0, UpDownNote, notelen, NoteLen, NoteEvery = 5, PercussionNote = 31, Velocity, Volume = 100, overnote = 0, ordinalNote;
int NotesOn[64], TotalNotesOn[64], Increase, Decrease, copying = NOCOPY, sustainflag = 0, chordnameflag = 0, PitchBendLoc;
int firstX = 0, lastX = 0, copyX, offsetX, PedalDownX, PedalDownY, HighestNote, LowestNote, name, lbptr, smallest;
int most, most3, most4, Cmost3, Cmost4, Fmost3, Fmost4, decimal, sign, firstN = 0, lastN = 0, index, thisIndex;
int biggest3, biggest4, Cbiggest3, Cbiggest4, Fbiggest3, Fbiggest4, RightSide, LoopCols = 0, MaxLoopCols, CurrentNotes = -1, PreviousNotes = 0, previousVolume;
int colors[16] = {RED, ORANGE, YELLOW, GREEN, BLUEGREEN, BLUE, VIOLET, BROWN, RED, GREY, BROWN, YELLOW, GREEN, BLUEGREEN, BLUE, VIOLET};
int pixel, Pixel, PreviousWrongEvent;
unsigned int uTimerID = 0, uTimer2ID = 0, uTimer3ID, uTimer4ID = 0, uTimer5ID = 0, uTimer6ID, uTimer7ID, uTimer8ID, uTimer9ID, uTimer0ID, uTimer10ID, uTimerElevenID, uTimer13ID, time, PauseID, chord = 0, DeviceOut = 0, DeviceIn = 0;
DWORD MagicNumber, XXX;
DWORD MeasureBarPixel, PedalDownLineEnd;
DWORD z, saveZ, newZ, diff, CursorLoc, copyBegin, i, j, fileSize, dwBytesRead, dwBytesWritten, fileSize2, UserFileSize;
DWORD Time = 0, Startime, milliSecondsPerBeat, timeFromBegin;
DWORD TicksPerSecond, TrackLen, TrackBegin, DeltaTicks, MetaEventLen, TotalTicks;
DWORD X, Y, Tickptr, Tick, Tock;
DWORD TopLeftPtr, FirstTopLeftPtr, PixelsPerPage, FirstPixelOnPage;
DWORD totalActiveChannels, saveE, PitchBend, PitchBend1, PitchBend2;
DWORD beginE, endE, beginZ, endZ, srcBeginX, srcBeginY, srcEndX, srcEndY;
DWORD EventIndex[MAX_EVENT];
WORD xLoc, yLoc, PixelsPerLine, PixelsPerMeasure;
BYTE oldBeatsPerMeasure, oldPixelsPerBeat, oldKeySignature, ModWheel[16];
BYTE Keys[] = {200,255,254,253,252,251,250,1,2,3,4,5,6};
char keys[] = "C F BbEbAbDbGbG D A E B F#";
BYTE IniBuf[1024];
BYTE LogBuf[4000000];
BYTE LoopNotes[47][120];
BYTE LoopNoteVol, sharporflat;
BYTE *lbPtr;
BYTE GoodNumbersInChords[TOTAL_CHORDS], biggest;
BYTE b;
WORD BadNumbersInChords[TOTAL_CHORDS], baddest;
BYTE ChordBuf[14];
int BoxWidth, ScanCode;

//				      1  2  3  4  5  6  7  8  9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32 33 34 35 36 37 38 39 40 41 42 43 44 45 46 47 48 49 50 51 52 53
int ScanCodes[] = {0, 0,70,71,72,73,74,75,76,77,78,79,80,81, 0, 0,56,57,58,59,60,61,62,63,64,65,66,67, 0, 0,45,46,47,48,49,50,51,52,53,54,55,69, 0,68,35,36,37,38,39,40,41,42,43,44};// percussion numbers for scan codes 0 thru 53
char Keyboard1[] = {'`','1','2','3','4','5','6','7','8','9','0','-','='};
char Keyboard2[] = {'Q','W','E','R','T','Y','U','I','O','P','[',']','\\'};
char Keyboard3[] = {'A','S','D','F','G','H','J','K','L',';','\''};
char Keyboard4[] = {'Z','X','C','V','B','N','M',',','.','/'};
char *PercInstr1[] = {"","","Short","Long","Short","Long","","Hi Wood","Low Wood","Mute","Open","Mute","Open"};
char *PercInstr1x[] = {"Cabasa","Maracas","Whistle","Whistle","Guiro","Guiro","Claves","Block","Block","Cuica","Cuica","Triangle","Triangle"};
char *PercInstr2[] = {"","Crash","","Ride","Hi","Low","Mute Hi","Open Hi","Low","Hi","Low","Hi","Low"};
char *PercInstr2x[] = {"Cowbell","Cymbal 2","Vibraslap","Cymbal","Bongo","Bongo","Conga","Conga","Conga","Timbale","Timbale","Agogo","Agogo"};
char *PercInstr3[] = {"Low","Open","Low-Mid","Hi-Mid","Crash","High","Ride","Chinese","Ride","","Splash"};
char *PercInstr3x[] = {"Tom","Hi-Hat","Tom","Tom","Cymbal 1","Tom","Cymbal 1","Cymbal","Bell","Tamborine","Cymbal"};
char *PercInstr4[] = {"Acoustic","Bass","","Acoustic","","Electric","Low Floor","Closed","High Floor","Pedal"};
char *PercInstr4x[] = {"Bass Drum","Drum 1","Side Stick","Snare","Hand Clap","Snare","Tom","Hi-Hat","Tom","Hi-Hat"};

char NotesInBox[14*2];
char Number[4];
char CM[] =   "C E G ";
char Cm[] =   "C EbG ";
char Cdim[] = "C EbGb";
char Caug[] = "C E Ab";
char C7[] =   "C E G Bb";
char CM7[] =  "C E G B ";
char Cm7[] =  "C EbG Bb";
char Cdim7[] ="C EbGbA ";
char FM[] =   "F A C ";
char Fm[] =   "F AbC ";
char Fdim[] = "F AbB ";
char Faug[] = "F A Db";
char F7[] =    "F A C Eb";
char FM7[] =   "F A C E ";
char Fm7[] =   "F AbC Eb";
char Fdim7[] = "F AbB D ";
char GM[] =    "G B D ";
char Gm[] =    "G BbD ";
char Gdim[] =  "G BbDb";
char Gaug[] =  "G B Eb";
char G7[] =    "G B D F ";
char GM7[] =   "G B D Gb";
char Gm7[] =   "G BbD F ";
char Gdim7[] = "G BbDbE ";
char BbM[] =   "BbD F ";
char Bbm[] =   "BbDbF ";
char Bbdim[] = "BbDbE ";
char Bbaug[] = "BbD Gb";
char Bb7[] =   "BbD F Ab";
char BbM7[] =  "BbD F A ";
char Bbm7[] =  "BbDbF Ab";
char Bbdim7[] ="BbDbE G ";
char DM[] =    "D GbA ";
char Dm[] =    "D F A ";
char Ddim[] =  "D F Ab";
char Daug[] =  "D GbBb";
char D7[] =    "D GbA C ";
char DM7[] =   "D GbA Db";
char Dm7[] =   "D F A C ";
char Ddim7[] = "D F AbB ";
char EbM[] =   "EbG Bb";
char Ebm[] =   "EbGbBb";
char Ebdim[] = "EbGbA ";
char Ebaug[] = "EbG B ";
char Eb7[] =   "EbG BbDb";
char EbM7[] =  "EbG BbD ";
char Ebm7[] =  "EbGbBbDb";
char Ebdim7[] ="EbGbA C ";
char AM[] =    "A DbE ";
char Am[] =    "A C E ";
char Adim[] =  "A C Eb";
char Aaug[] =  "A DbF ";
char A7[] =    "A DbE G ";
char AM7[] =   "A DbE Ab";
char Am7[] =   "A C E G ";
char Adim7[] = "A C EbGb";
char AbM[] =   "AbC Eb";
char Abm[] =   "AbB Eb";
char Abdim[] = "AbB D ";
char Abaug[] = "AbC E ";
char Ab7[] =   "AbC EbGb";
char AbM7[] =  "AbC EbG ";
char Abm7[] =  "AbB EbGb";
char Abdim7[] ="AbB D F ";
char EM[] =    "E AbB ";
char Em[] =    "E G B ";
char Edim[] =  "E G Bb";
char Eaug[] =  "E AbC ";
char E7[] =    "E AbB D ";
char EM7[] =   "E AbB Eb";
char Em7[] =   "E G B D ";
char Edim7[] = "E G BbDb";
char DbM[] =   "DbF Ab";
char Dbm[] =   "DbE Ab";
char Dbdim[] = "DbE G ";
char Dbaug[] = "DbF A ";
char Db7[] =   "DbF AbB ";
char DbM7[] =  "DbF AbC ";
char Dbm7[] =  "DbE AbB ";
char Dbdim7[] ="DbE G Bb";
char BM[] =    "B EbGb";
char Bm[] =    "B D Gb";
char Bdim[] =  "B D F ";
char Baug[] =  "B EbG ";
char B7[] =    "B EbGbA ";
char BM7[] =   "B EbGbBb";
char Bm7[] =   "B D GbA ";
char Bdim7[] = "B D F Ab";
char GbM[] =   "GbBbDb";
char Gbm[] =   "GbA Db";
char Gbdim[] = "GbA C ";
char Gbaug[] = "GbBbD ";
char Gb7[] =   "GbBbDbE ";
char GbM7[] =  "GbBbDbF ";
char Gbm7[] =  "GbA DbE ";
char Gbdim7[] ="GbA C Eb";
//maj min 7 min7 maj7 sus4 sus2 6 min6 aug dim7 7-5 7+5 min/maj7 maj7-5 maj7+5 9 min9 maj9 11 min11 13 min13 maj13 7-5-9 7-5+9 7+5-9
char C9[] =    "C E G BbD ";
char CM9[] =   "C E G B D ";
char Cm9[] =   "C EbG BbD ";
char Db9[] =   "DbF AbB Eb";
char DbM9[] =  "DbF AbC Eb";
char Dbm9[] =  "DbE AbB Eb";
char D9[] =    "D GbA C E ";
char DM9[] =   "D GbA DbE ";
char Dm9[] =   "D F A C E ";
char Eb9[] =   "EbG BbDbF ";
char EbM9[] =  "EbG BbD F ";
char Ebm9[] =  "EbGbBbDbF ";
char E9[] =    "E GbB D Gb";
char EM9[] =   "E AbB EbGb";
char Em9[] =   "E G B D Gb";
char F9[] =    "F A C EbG ";
char FM9[] =   "F A C E G ";
char Fm9[] =   "F AbC EbG ";
char Gb9[] =   "GbBbDbE Ab";
char GbM9[] =  "GbBbDbF Ab";
char Gbm9[] =  "GbA DbE Ab";
char G9[] =    "G B D F A ";
char GM9[] =   "G B D GbA ";
char Gm9[] =   "G BbD F A ";
char Ab9[] =   "AbC EbGbBb";
char AbM9[] =  "AbC EbG Bb";
char Abm9[] =  "AbB EbGbBb";
char A9[] =    "A DbE G B ";
char AM9[] =   "A DbE AbB ";
char Am9[] =   "A C E G B ";
char Bb9[] =   "BbD F AbC ";
char BbM9[] =  "BbD F A C ";
char Bbm9[] =  "BbDbF AbC ";
char B9[] =    "B EbGbA Db";
char BM9[] =   "B EbGbBbDb";
char Bm9[] =   "B D GbA Db";

char CM11[] =   "C E G B D F ";
char DbM11[] =  "DbF AbC EbGb";
char DM11[] =   "D GbA DbE G ";
char EbM11[] =  "EbG BbD F Ab";
char EM11[] =   "E AbB EbGbA ";
char FM11[] =   "F A C E G Bb";
char GbM11[] =  "GbBbDbF AbB ";
char GM11[] =   "G B D GbA C ";
char AbM11[] =  "AbC EbG BbDb";
char AM11[] =   "A DbE AbB D ";
char BbM11[] =  "BbD F A C Eb";
char BM11[] =   "B EbGbBbDbE ";
char CM13[] =   "C E G B D F A ";
char DbM13[] =  "DbF AbC EbGbBb";
char DM13[] =   "D GbA DbE G B ";
char EbM13[] =  "EbG BbD F AbC ";
char EM13[] =   "E AbB EbGbA Db";
char FM13[] =   "F A C E G BbD ";
char GbM13[] =  "GbBbDbF AbB Eb";
char GM13[] =   "G B D GbA C E ";
char AbM13[] =  "AbC EbG BbDbF ";
char AM13[] =   "A DbE AbB D Gb";
char BbM13[] =  "BbD F A C EbG ";
char BM13[] =   "B EbGbBbDbE Ab";
char *ChordNotes[] = {CM,Cm,Cdim,Caug,C7,CM7,Cm7,Cdim7,FM,Fm,Fdim,Faug,F7,FM7,Fm7,Fdim7,GM,Gm,Gdim,Gaug,G7,GM7,Gm7,Gdim7,BbM,Bbm,Bbdim,Bbaug,Bb7,BbM7,Bbm7,Bbdim7,DM,Dm,Ddim,Daug,D7,DM7,Dm7,Ddim7,EbM,Ebm,Ebdim,Ebaug,Eb7,EbM7,Ebm7,Ebdim7,AM,Am,Adim,Aaug,A7,AM7,Am7,Adim7,AbM,Abm,Abdim,Abaug,Ab7,AbM7,Abm7,Abdim7,EM,Em,Edim,Eaug,E7,EM7,Em7,Edim7,DbM,Dbm,Dbdim,Dbaug,Db7,DbM7,Dbm7,Dbdim,BM,Bm,Bdim,Baug,B7,BM7,Bm7,Bdim7,GbM,Gbm,Gbdim,Gbaug,Gb7,GbM7,Gbm7,Gbdim7,\
C9,CM9,Cm9,Db9,DbM9,Dbm9,D9,DM9,Dm9,Eb9,EbM9,Ebm9,E9,EM9,Em9,F9,FM9,Fm9,Gb9,GbM9,Gbm9,G9,GM9,Gm9,Ab9,AbM9,Abm9,A9,AM9,Am9,Bb9,BbM9,Bbm9,B9,BM9,Bm9,CM11,DbM11,DM11,EbM11,EM11,FM11,GbM11,GM11,AbM11,AM11,BbM11,BM11,CM13,DbM13,DM13,EbM13,EM13,FM13,GbM13,GM13,AbM13,AM13,BbM13,BM13};
char *ChordPtr[] = {"CM","Cm","Cdim","Caug","C7","CM7","Cm7","Cdim7","FM","Fm","Fdim","Faug","F7","FM7","Fm7","Fdim7","GM","Gm","Gdim","Gaug","G7","GM7","Gm7","Gdim7","BbM","Bbm","Bbdim","Bbaug","Bb7","BbM7","Bbm7","Bbdim7","DM","Dm","Ddim","Daug","D7","DM7","Dm7","Ddim7","EbM","Ebm","Ebdim","Ebaug","Eb7","EbM7","Ebm7","Ebdim7","AM","Am","Adim","Aaug","A7","AM7","Am7","Adim7","AbM","Abm","Abdim","Abaug","Ab7","AbM7","Abm7","Abdim","EM","Em","Edim","Eaug","E7","EM7","Em7","Edim7","DbM","Dbm","Dbdim","Dbaug","Db7","DbM7","Dbm7","Dbdim7","BM","Bm","Bdim","Baug","B7","BM7","Bm7","Bdim7","GbM","Gbm","Gbdim","Gbaug","Gb7","GbM7","Gbm7","Gbdim7",\
"C9","CM9","Cm9","Db9","DbM9","Dbm9","D9","DM9","Dm9","Eb9","EbM9","Ebm9","E9","EM9","Em9","F9","FM9","Fm9","Gb9","GbM9","Gbm9","G9","GM9","Gm9","Ab9","AbM9","Abm9","A9","AM9","Am9","Bb9","BbM9","Bbm9","B9","BM9","Bm9","CM11","DbM11","DM11","EbM11","EM11","FM11","GbM11","GM11","AbM11","AM11","BbM11","BM11","CM13","DbM13","DM13","EbM13","EM13","FM13","GbM13","GM13","AbM13","AM13","BbM13","BM13"};
char SharpChordNotes[16];

char ListBuf[512], PreviousIndex[512], NextIndex[512];
char temp[10], temp2[10];
BYTE tempBuf[16];
char NoteName[4];
char SharpFlat[2];
char FKey[14] = "    (         ";
char activeChannels[16];
char IniFile[] = "PianoRollComposer.ini";
char IniCopy[] = "Copyright=";
char IniTex[] = "Text=";
char IniAccidental[] = "Accidental=";
char IniShow[] = "ShowParameters=";
char IniVolume[] = "Volume=";
char IniBeatsPerMinute[] = "Beats/Minute=";
char IniOneLessPixel[] = "OneLessPixel=";
char IniNotePixels[] = "NoteEvery=";
char IniShowKey[] = "ShowKey=";
char IniShowTime[] = "ShowTime=";
char IniShowNoteNames[] = "ShowNoteNames=";
char IniShowBPMs[] = "ShowBPMs=";
char IniShowMeasureNums[] = "ShowMeasureNumbers=";
char IniShowNoteVelocities[] = "ShowNoteVelocities=";
char IniShowNameAtPointer[] = "ShowNameAtPointer=";
char IniShowNewInstrument[] = "ShowNewInstrument=";
char IniShowFingers[] = "ShowFingers=";
char IniShowNumbers[] = "ShowNumbers=";
char IniAddedVelocity[] = "AddedVelocity=";
char IniRecordQuality[] = "RecordQuality=";
char IniBigOpen[] = "BigOpen=";
char IniPorts[] = "Ports=";
char SoundFont[] = "SoundFont=";
char MidiOutput[] = "MidiOutput=";
char MidiInput[] = "MidiInput=";
char Constantvelocity[] = "ConstantVelocity=";
char OnlyNoteNames[] = "OnlyNoteNames=";
char Pixelsbetweenlines[] = "PixelsBetweenLines=";
char ShowASpace[] = "ShowASpace=";
char IniSysEx[] = "SysEx=";
char LogFile[] = "PianoRollComposer.log";
char Highlight[] = "High&light";
char Highlighting[] = "HIGH&LIGHTING";
char Play[] = "&Play";
char Pause[] = "&PAUSE";
char ResumePlay[] = "PLAY";
char Stop2[] = "STO&P";
char RecordMIDIKeyboard[] = "&Record";
char StopRecording[] = "STOP &Recording";
char Filename[MAX_PATH];
char FullFilename[MAX_PATH];
char FullFilename2[MAX_PATH];
char PlayList[MAX_PLAYLIST][MAX_PATH];
char *Help;
char DotLog[] = ".log";
char BigText[4096];
char Name[48];
char text1[23] = " Beats/Minute ";
char text2[21] = " Velocity ";
char text3[21] = " Note Begin ";
char text4[19] = " Note End ";
char textBPM[10] = "120", textVolume[10], textStart[10], textEnd[10], textBeatsMeasure[10], textKeySig[10];
char textName[64], textCopyright[64], textText[512];
char addedVelocity[4];
char constantVelocity[4];
char Limit[] = "Exceeds the limit of measures per line (64).";
char ticksPerBeat[] = "Ticks/Beat = ";
char ShowAndPlay[] = "&Play Active Instrument";
char DontShowAndPlay[] = "Don't &Play Active Instrument";
char Chan[] = "Channel ";
char ChanBank[] = " Bank = ";
char Chan2[] = " (channel xx)";
char Millisecondspertick[] = "MilliSeconds/Tick = ";
char beatsPerMinute[] = "Beats/Minute = ";
char keysig[] = "Key Signature = ";
char TimeSignature[] = "Time Signature = ";
char Perc[] = "Percussion";
char unknownMessage[] = "Unknown Message = ";
char unknownControl[] = "Unknown Control = ";
char unknownEntry[] = "Unknown Entry";
char Sustain[] = "Sustain = ";
char Pan[] = "Pan = ";
char ChanVol[] = "Volume = ";
char modWheel[] = "Modulation = ";
char Reverb[] = "Reverb = ";
char Chorus[] = "Chorus = ";
char Expression[] = "Expression = ";
char AllControllersOff[] = "All Controllers Off";
char AllNotesOff[] = "All Notes Off";
char RPN[] = "RPN (coarse) = ";
char RPN2[] = "RPN (fine) = ";
char NRPN[] = "NRPN (coarse) = ";
char NRPN2[] = "NRPN (fine) = ";
char DataEntry[] = "Data Entry Slider(coarse) = ";
char DataEntry2[] = "Data Entry Slider(fine) = ";
char SoftPedal[] = "Soft Pedal = ";
char pitchBend[] = "Pitch Bend = ";
char KeyAftertouch[] = "Key Aftertouch = ";
char AfterTouch[] = "AfterTouch = ";
char FineBankSelect[] = "BankSelect(fine) = ";
char portamentoRate[] =  "Portamento Rate = ";
char portamentoRate2[] = "Portamento Rate2 = ";
char portamento[] = "Portamento = ";
char ToDelete[] = "one of the overlapping notes\nhas to be from the ACTIVE instrument.";
char NoteLimit[] = "A note has reached its limit.";
char page[13] = "page ";
char PercussionLoop[] = "Percussion Loop";
char EditMyWave[] = "View Wave";
char Phase[] = "Phase";
char Chords[] = "Chords";
char Keyboardpercus[] = "Keyboard Percussion";
char RecordingToWave[] = "Press Esc to end RECORDING TO WAVE";
char PlayingWave[] = "Press Esc to end PLAYING WAVE";
char ShowingNotesPlayed[] = "Showing Notes Played";
char CheckforUpdate[] = "http://jdmcox.com/";
char UpdateAvailable[100] = "\nhttp://jdmcox.com";
char Accept[] = "Accept: */*\r\n\r\n";
char Dynamics[] = "Dynamics & Effects";

BOOL goodread;
DWORD ptrInstrument;
BOOL first = TRUE, midi_in = FALSE, usingsharp = FALSE, show = TRUE, sharpflat, showkey = FALSE, showtime = FALSE, event, gotit, firstnoteplayed = TRUE, othernotesplaying;
BOOL playing = FALSE, keyboardactive = FALSE, good, firstnote, fromP, ctrl, transposectrl, atnote, overlap, fromrename = FALSE, showingoptions = FALSE, showports = FALSE;
BOOL paused = FALSE, highlighting = FALSE, shownotenames = FALSE, startatp = FALSE, gotc0, gotc9, gottime, gotkey, gotpiano, gotdrums, itsbad, transposeshift;
BOOL helping = FALSE, done, movingleft, playing2 = FALSE, fromuser = FALSE, clearscreen = FALSE, showinginstruments = FALSE, showingdynamics = FALSE;
BOOL continuousustain[16], reverb[16], chorus[16], metronome = FALSE, showvolumes = FALSE, showbeatsperminute = FALSE, showlist = FALSE, showmeasurenumber = FALSE;
BOOL showvolume = FALSE, showexpression = FALSE, showmodulation = FALSE, showreverb = FALSE, showchorus = FALSE, showpitchbend = FALSE, showportamento = FALSE, fromkeyboard = FALSE;
BOOL everything, nonotes, nonoteoffs, volumeonly, expressiononly, panonly, reverbonly, chorusonly, modulationonly, pitchbendonly, sustainonly, bpmonly, chaninst, bank;
BOOL button3pressed = FALSE, button4pressed = FALSE, setvolume, settempo, dynamicexpression = FALSE, shownameatpointer = FALSE, shownewinstrument = FALSE, showfingers = FALSE, shownumbers = FALSE;

  //                       R    I    F    F  chunk size            W    A    V    E    f    m    t       subchunk1 size      PMC       channels  sample rate         byte rate          blockalign bits/sample d    a    t    a  subchunk2 size
//BYTE WaveHeader[44] = {0x52,0x49,0x46,0x46,0x00,0x00,0x00,0x00,0x57,0x41,0x56,0x45,0x66,0x6D,0x74,0x20,0x10,0x00,0x00,0x00,0x01,0x00,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x02,0x00,0x10,0x00,0x64,0x61,0x74,0x61,0x00,0x00,0x00,0x00};
  //                        0    1    2    3    4    5    6    7    8    9   10   11   12   13   14   15   16   17   18   19   20   21   22   23   24   25   26   27   28   29   30   31   32   33   34   35   36   37   38   39   40   41   42   43
BYTE WaveHeader[] = {\
0x52,0x49,0x46,0x46, // 0 "RIFF"
0x00,0x00,0x00,0x00, // 4 chunk size
0x57,0x41,0x56,0x45, // 8 "WAVE"
0x66,0x6D,0x74,0x20, //12 "fmt"
0x28,0x00,0x00,0x00, //16 Format chunk size (40 for WAVE_FORMAT_EXTENSIBLE)
0xFE,0xFF,				//20 WAVE_FORMAT_EXTENSIBLE
0x01,0x00,				//22 channels
0x00,0x00,0x00,0x00, //24 samples/sec 
0x00,0x00,0x00,0x00, //28 average bytes/sec
0x00,0x00,				//32 block align (data block size)
0x00,0x00,				//34 bits/sample
0x16,0x00,				//36 size of extension
0x00,0x00,				//38 valid bits/sample
0x03,0x00,0x00,0x00, //40 channel mask (3 = left & right speakers)
0x01,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71, //44 SubFormat
0x64,0x61,0x74,0x61, //60 "data" (at 36 if PCM)
0x00,0x00,0x00,0x00};//64 Data chunk size
BYTE SubFormat[] = {0x01,0x00,0x00,0x00,0x00,0x00,0x10,0x00,0x80,0x00,0x00,0xaa,0x00,0x38,0x9b,0x71};
DWORD WaveHeaderSize;

int m, play, record, indexPlay = 0, WaveOptionsIndex = 3, BytesPerSample;
int wp, dp, wavePtr, wave0, prevwP, vert, horiz, BytesPer40MilliSeconds;
DWORD subchunksize, nextchunk, SourceOfDestination, ComponentType;
DWORD BufferSize, WaveExtension, nSamplesPerSec = 22050, nSamplesPer40MilliSeconds, prevTickptr;
DWORD mixerNumber, NumOfMixers, Destination, WaveVolume, control, MixerSliders;
DWORD mixerMaxVol, mixerMinVol, Entries;
UINT mixerID, indexRecord = 0;
WORD nChannels = 1, nRecordChannels = 1, wBitsPerSample = 16;
BYTE *WaveBuf, *WaveBuf2;
BYTE Header[2048];
BOOL wavemixer = FALSE, fromwavemixer = FALSE, fromup = FALSE, fromdown = FALSE;
char MasterVolume[10] = "vol xx0%  ";
char mixertemp[32];
char Sound[] = "Sound";
char WaveMixer[] = "Mixer";
char AudioMixer[] = "Audio Mixer";
char RecordPlay;
char Playback[8][32];
char Record[8][32];
char Selected[32];

struct {
	DWORD mixerNumber;
	DWORD dwControlID;
	DWORD muteDestination;
	DWORD Volume;
	DWORD checkID;
	char mixercaps[32];
	char mixer[32];
	char type[64];
	char volume[11];
	char RecordPlay;
	BYTE muteselected;
	BYTE basstreble;
	BYTE bass;
	BYTE treble;
} MixerBuf[48];

DWORD Mixer, SliderPos;
BYTE BassOrTreble;
HWND hBassSlider, hTrebleSlider, hBassTreble;

struct {
	DWORD mixer;
	HWND hSlider;
	int checks;
} Slider[16];

double prevMilliSecondsPer, dMilliSeconds, mixerVolTick = 6553.5;// 1/10 of 0xFFFF

int sliders[16] = {IDC_SLIDER1, IDC_SLIDER2, IDC_SLIDER3, IDC_SLIDER4, IDC_SLIDER5, IDC_SLIDER6, IDC_SLIDER7, IDC_SLIDER8, IDC_SLIDER9, IDC_SLIDER10, IDC_SLIDER11, IDC_SLIDER12, IDC_SLIDER13, IDC_SLIDER14, IDC_SLIDER15, IDC_SLIDER16};
int mixers[16] = {IDC_EDIT1, IDC_EDIT2, IDC_EDIT3, IDC_EDIT4, IDC_EDIT5, IDC_EDIT6, IDC_EDIT7, IDC_EDIT8, IDC_EDIT9, IDC_EDIT10, IDC_EDIT11, IDC_EDIT12, IDC_EDIT13, IDC_EDIT14, IDC_EDIT15, IDC_EDIT16};
int volumes[16] = {IDC_STATIC1, IDC_STATIC2, IDC_STATIC3, IDC_STATIC4, IDC_STATIC5, IDC_STATIC6, IDC_STATIC7, IDC_STATIC8, IDC_STATIC9, IDC_STATIC10, IDC_STATIC11, IDC_STATIC12, IDC_STATIC13, IDC_STATIC14, IDC_STATIC15, IDC_STATIC16};
int checks[16] = {IDC_CHECK1, IDC_CHECK2, IDC_CHECK3, IDC_CHECK4, IDC_CHECK5, IDC_CHECK6, IDC_CHECK7, IDC_CHECK8, IDC_CHECK9, IDC_CHECK10, IDC_CHECK11, IDC_CHECK12, IDC_CHECK13, IDC_CHECK14, IDC_CHECK15, IDC_CHECK16};
HWND hSlider[16], hwndCombo1, hwndCombo2;
HWND hEdit[16];
HWND hStatic[16];
HWND hCheck[16];
HWND hwndMeter;

MIDIHDR midiHdr;
LPMIDIHDR pMidiHdr;
HWAVEIN hWaveIn;
HWAVEOUT hWaveOut, hWaveOut2, hWaveOut3;
HWAVEOUT hTemp, hTemp2;
WAVEHDR WaveInHdr, WaveOutHdr, WaveOutHdr2;
WIN32_FIND_DATA fd;

//WAVEFORMATEX WaveFormat, WaveFormat2;
//typedef struct {
//	WAVEFORMATEX Format;
//	union {
//		WORD wValidBitsPerSample; // this is for 24 bit sound
//		WORD wSamplesPerBlock;
//		WORD wReserved;
//	} Samples;
//	DWORD dwChannelMask;
//	GUID SubFormat;
//} WAVEFORMATEXTENSIBLE;
WAVEFORMATEXTENSIBLE WaveFormat, WaveFormat2;

HMIXER hMixer, hMixer2;
MIXERCAPS mixerCaps;
MIXERLINE mixerLine, mixerLine2, mixerLine3;
MIXERLINECONTROLS mixerLineControls, mixerLineControls2;
MIXERCONTROL mixerControl, mixerControl2[8];
MIXERCONTROLDETAILS mixerControlDetails, mixerControlDetails2;
MIXERCONTROLDETAILS_UNSIGNED mixerControlDetailsVolume, mixerControlDetailsVolume2, *pSelected, mixerControlDetailsOther;
MIXERCONTROLDETAILS_BOOLEAN mixerControlDetailsMute, mixerControlDetailsMasterMute;
MIXERCONTROLDETAILS_LISTTEXT *pList;
MMRESULT MixerError;
HMIDIIN hMidiIn;
MIDIINCAPS mic;
MMTIME mmTime;

char WaveHelp[] = "\
Record either MIDI music thru the sound card when it's played, or thru a mic.\r\n\
If MIDI music is showing on the staffs, it will play as you sing (use headphones).\r\n\
\r\n\
Volume Control should work fine, but you can cross-check it by going to\r\n\
Sounds and Audio Devices in the Windows Control Panel, and in the Audio tab,\r\n\
click on Volume in Sound Recording and Volume in Sound Playback.\r\n\
\r\n\
Four volume controls affect the recorded sound from MIDI music.\r\n\
One is in Recording, and the three others are in Playback: main volume, SW Synth, and Wave.\r\n\
\r\n\
You can record to a WAVE file with no music for 5 minutes at 44.1 kHz 16 bits and Stereo,\r\n\
or 10, 20, or 40 minutes at lower kHz and/or Monaural (the buffer is 52,920,000 bytes).\r\n\
Or you can record to the end of the music.\r\n\
48 kHz and 24 bits sounds the best, but a SoundFont or VST can limit the sound quality.\r\n\
\r\n\
Get winLAME (free) from the Internet to create an MP3 file from a Wave file.\r\n\
\r\n\
RINGTONES\r\n\
To trim an MP3 file to make it small enough for a cell phone's ringtone,\r\n\
get mpTrim (free) from the Internet. Most cell phones will only use a 10 second MP3 ringtone.\r\n\
\r\n\
To send a ringtone to a bluetooth-capable cellphone, buy a USB Bluetooth thing.\r\n\
To change Bluetooth settings in your computer, open Bluetooth Devices in Control Panel.\r\n\
To send ringtones from your computer, select:\r\n\
   Start\r\n\
   All Programs\r\n\
   Accessories\r\n\
   Communications\r\n\
   Bluetooth File Transfer Wizard";

char NewInstrumentHelp[] = "\
If MIDI music contains instrument changes,\n\
an X will show at Middle C where it occurs.\n\
\n\
You could look at Advanced -List View to see\n\
which instrument(s) changed at that pixel.";

char PercussionHelp[] = "\r\n\
Percussion instruments are associated with note locations on the staffs.\r\n\
For example, if PERCUSSION is selected in the Instruments window,\r\n\
and is selected as the Active Instrument, then if you click on middle C,\r\n\
a Bongo will sound and a note will be placed there. Clicking on other locations\r\n\
on the staffs will sound other percussion notes, and will place other there.\r\n\
Short percussion sounds can be placed closer to each other if you make them sixty-fourth notes.\r\n\
\r\n\
The General MIDI Specification states that there will be 47 percussion instruments,\r\n\
beginning with Acoustic Bass Drum and ending with Open Triangle, but this number has been\r\n\
expanded by some MIDI programs to 61, and Windows recognizes the expanded list.\r\n\
You can enter 61 percussion instruments here, from the lowest Eb/D# to the second highest Eb/D#.\r\n\
\r\n\
Another (un-documented) change to the General MIDI Specification is a method to change the sound\r\n\
of the types of percussion instruments specified. Since the MIDI sounds in Creative sound cards\r\n\
can adapt to those new sounds, you can select either Default Sounds, Alternate 1, Alternate 2,\r\n\
or Alternate 3 to get those sets of drum sounds, if you have a Creative sound card. Windows XP\r\n\
seems to support Alternate 1 as well as the Default Sounds. Some MIDI files use Alternate 1 sounds,\r\n\
and of course if you select it, your MIDI music will include a message to use it. If you open the\r\n\
Percussion Instruments window with MIDI music showing, the sound in the MIDI file will be indicated.";

char ChordsHelp[] = "Click on a Key, Chord type,\r\n\
Mode, Degree, or Inversion,\r\n\
or use the 4 Arrow keys.\r\n\
\r\n\
For Octave Up/Down, press PgUp/PgDn.\r\n\
To Play, press Enter or the Spacebar.\r\n\
To exit, Press Esc.\r\n\
\r\n\
To enter a chord into the music,\r\n\
click on where you want it to go.\r\n\
Select Sharps or Flats in Options\r\n\
to match the chord key.\r\n\
\r\n\
The Grey M or m when the 13th chord\r\n\
is choses indicate Major or minor Thirds.\r\n\
\r\n\
Read http://jdmcox.com/Music Theory.txt";

char LoopHelp[] = "\r\n\
Only the 47 standard MIDI percussion instruments show.\r\n\
\r\n\
The greyed characters at the left are the keyboard keys\r\n\
to use if you select Keyboard Percussion.\r\n\
\r\n\
The double vertical lines indicate a quarter note width.\r\n\
Percussion notes from here are all thirty-second notes.\r\n\
\r\n\
Left-click on a square to create a percussion note.\r\n\
Press Delete or left-click on a percussion note to remove it.\r\n\
\r\n\
Right-click on a percussion note to change its velocity (volume).\r\n\
Initial percussion velocity is Initial Note Velocity in Options.\r\n\
\r\n\
Press the Space bar to play.\r\n\
Press the Space bar or Esc to stop playing.\r\n\
\r\n\
If you select Files -Save, the percussion\r\n\
music will be saved as a MIDI file.\r\n\
\r\n\
If you select Files -Open, the percussion part\r\n\
of the MIDI file you select (that fits) will be shown.\r\n\
\r\n\
If you select Use as Metronome on the Menu,\r\n\
you should also select 1 Quarter Note\\Loop.\r\n\
Press Esc to stop the Metronome.\r\n\
\r\n\
If you want to Paste Into Existing Music, but there isn't any,\r\n\
just put a note where you want the percussion loop to end.\r\n\
\r\n\
See Instruments -PERCUSSION -HELP for more info.";

char DynamicEffectsHelp[] = "\r\n\
Dynamics and Effects can be embedded in a MIDI music file.\r\n\
This program graphically represents their values as vertical locations on the screen.\r\n\
\r\n\
Volume and Expression values are depicted by offsets down from high C.\r\n\
A maximum value of 127 will show at high C.\r\n\
\r\n\
Reverberation, Chorus Effect, and Modulation values are depicted by offsets up from low A.\r\n\
A minimum value of 0 will show at low A.\r\n\
\r\n\
Pitch Bend is neutral at Middle C. Minimum Pitch Bend is at the lowest D,\r\n\
and maximum Pitch Bend is at the second highest B. These are each 2 semitones from neutral.\r\n\
The G above the top of the Treble Clef and the F below the bottom of the Bass Clef\r\n\
are one semitone from neutral. Pitch Bend can be used to slur from one note to another.\r\n\
Neutral Pitch Bend is 8192, max Pitch Bend is 16384, and min Pitch Bend is 0.\r\n\
\r\n\
Portamento probably only works with EastWest/Quantum Leap instruments (and ony marginally).\r\n\
                             Portamento Off occurs an eighth note spacing after Portamento On.\r\n\
Selecting EDIT allows you to add a dynamic or effect for the ACTIVE instrument.\r\n\
After selecting a dynamic/effect, move the mouse pointer to where you want it to begin,\r\n\
then press the left mouse button, and release it for a single entry, or hold it down\r\n\
while moving the mouse pointer to where you want it to end.\r\n\
(a dynamic/effect's possible value is shown at the cursor as you move the cursor within its range)\r\n\
\r\n\
The first letter of the selected item will appear with the instrument's color as background.\r\n\
\r\n\
The top left of the letter's colored background is the location of the item.\r\n\
Except for Pitch Bend, values are equivalent to pixels. Notes are 6 vertical pixels apart.\r\n\
\r\n\
Press the Delete key while the mouse pointer is over a V, E, R, C, M, or P\r\n\
to delete an item from the ACTIVE instrument.\r\n\
If you hold the delete key down, you can delete multiple letters while moving the mouse.\r\n\
\r\n\
Only dynamics and effects for instruments selected to SHOW & PLAY will show.";

//+00140 Piano[1] C 60 (85) 19
char ListHelp[] = "\r\n\
This list only shows those instruments selected to SHOW & PLAY.\r\n\
\r\n\
The number at the beginning of each line is the offset in pixels from the beginning of the music.\r\n\
If there's a + before the pixel number, it's for Note On; Note Off has a - at the beginning of the line.\r\n\
The number in brackets is the channel number.\r\n\
Next is the note name.\r\n\
Next is its MIDI note number.\r\n\
If it's a Note On, the number in parenthese is the note velocity/volume, and the last number is the pixel width of the note.\r\n\
\r\n\
Changes can ONLY be made to: Channel instrument assignments, Volume, Expression,\r\n\
Pan, Reverb, Chorus, Sustain, Modulation, Pitch Bend, Beats/Minute, Portamento Rate, and All Controllers (or Notes) Off.\r\n\
The format must be correct.\r\n\
Examples:\r\n\
 00000 Channel 1 Harpsichord\r\n\
 00000 Channel 1 7   (the instrument's number works, too)\r\n\
 00000 Channel 10   (channel 10 is always percussion, so it doesn't need a name or number)\r\n\
 00000 Piano[1] Volume = 90\r\n\
 00000 Piano[1] Pitch Bend = 8192\r\n\
 00000 Piano[1] All Controllers Off\r\n\
Values range from 0 thru 127, except for Pitch Bend numbers and Beats/Minute. Beats/Minute range is 24 thru 500.\r\n\
If you see something like 00000 Channel 1 Bank = 1, it's meaningless, because Windows only has 1 Bank.\r\n\
\r\n\
Pitch Bend is for moving the pitch of a note up or down. Its range is 0 thru 16383.  8192 is neutral (no Pitch Bend).\r\n\
Pan means Stereo Location. 0 for full left and 127 for full right.\r\n\
Sustain On value is 127, and Sustain Off value is 0.\r\n\
\r\n\
When List View is selected, the pixel offset at the mouse pointer and the MIDI note number shows at the top middle of the screen.\r\n\
If you're looking for a MIDI note number for a sharp or flat, you'll have to hold down the Ctrl key while moving the mouse.\r\n\
\r\n\
When List View is selected, if you move the mouse pointer over a note and press the right mouse button,\r\n\
a button that says Show in List View will show in that note's parameters window. Click on it to go to that note in List View.";

char KeyboardHelp[] = "\
Press the Left or Right Arrow Key to\nmove the computer keyboard an octave.\n\
\n\
Press the Up or Down Arrow Key\nto change the keyboard volume.\n\
\n\
Press the Tab Key to focus on the main window\n\
if the Instruments window isn't showing,\n\
otherwise focus on the Instruments window.\n\
\n\
You can assign different instruments\n\
to different sections of the keyboard.\n\
Hold the right mouse button down\n\
while moving the mouse to define a section.\n\
\n\
Press Delete to delete a section that\nthe mouse is pointing to.\n\
\n\
If you select Octave Shift\n\
after creating an instrument section,\n\
you can make that instrument play\n\
higher or lower and thus can play\n\
the same note with 2 or 3 instruments.";

char New[] = "\
Clears the screen of any music.\n\
Makes the first Instrument the piano.\n\
Clears all other Instruments.\n\
Clears all MIDI Control Messages.\n\
Sets Beats/Minute to 120.\n\
Sets time signature to 4/4,\n\
and key signature to C Major.";

//INSTRUMENTS
char Piano[] = "Piano";
char Piano2[] = "Bright Piano";
char Piano3[] = "Electric Piano";
char Piano4[] = "Honky Tonk Piano";
char Piano5[] = "Electric Piano 1";
char Piano6[] =	"Electric Piano 2";
char Piano7[] = "Harpsichord";
char Piano8[] = "Clavinet";
char ChromaticPercussion[] = "Celesta";
char ChromaticPercussion2[] = "Glockenspiel";
char ChromaticPercussion3[] = "Music Box";
char ChromaticPercussion4[] = "Vibraphone";
char ChromaticPercussion5[] = "Marimba";
char ChromaticPercussion6[] = "Xylophone";
char ChromaticPercussion7[] = "Tubular Bells";
char ChromaticPercussion8[] = "Dulcimer";
char Organ[] = "Drawbar Organ";
char Organ2[] = "Percussive Organ";
char Organ3[] = "Rock Organ";
char Organ4[] = "Church Organ";
char Organ5[] = "Reed Organ";
char Organ6[] = "Accordian";
char Organ7[] = "Harmonica";
char Organ8[] = "Tango Accordian";
char Guitar[] = "Nylon Acoustic Guitar";
char Guitar2[] = "Steel Acoustic Guitar";
char Guitar3[] = "Jazz Electric Guitar";
char Guitar4[] = "Clean Electric Guitar";
char Guitar5[] = "Muted Electric Guitar";
char Guitar6[] = "Overdrive Guitar";
char Guitar7[] = "Distorted Guitar";
char Guitar8[] = "Harmonica Guitar";
char Bass[] = "Acoustic Bass";
char Bass2[] = "Electric Fingered Bass";
char Bass3[] = "Electric Picked Bass";
char Bass4[] = "Fretless Bass";
char Bass5[] = "Slap Bass 1";
char Bass6[] = "Slap Bass 2";
char Bass7[] = "Syn Bass 1";
char Bass8[] = "Syn Bass 2";
char Strings[] = "Violin";
char Strings2[] = "Viola";
char Strings3[] = "Cello";
char Strings4[] = "Contrabass";
char Strings5[] = "Tremolo Strings";
char Strings6[] = "Pizzicato Strings";
char Strings7[] = "Orchestral Harp";
char Strings8[] = "Timpani";
char Ensemble[] = "String Ensemble 1";
char Ensemble2[] = "String Ensemble 2 (Slow)";
char Ensemble3[] = "Syn Strings 1";
char Ensemble4[] = "Syn Strings 2";
char Ensemble5[] = "Choir Aahs";
char Ensemble6[] = "Voice Ohs";
char Ensemble7[] = "Syn Choir";
char Ensemble8[] = "Orchestral Hit";
char Brass[] = "Trumpet";
char Brass2[] = "Trombone";
char Brass3[] = "Tuba";
char Brass4[] = "Muted Trumpet";
char Brass5[] = "French Horn";
char Brass6[] = "Brass Section";
char Brass7[] = "Syn Brass 1";
char Brass8[] = "Syn Brass 2";
char Reed[] = "Soprano Sax";
char Reed2[] = "Alto Sax";
char Reed3[] = "Tenor Sax";
char Reed4[] = "Baritone Sax";
char Reed5[] = "Oboe";
char Reed6[] = "English Horn";
char Reed7[] = "Bassoon";
char Reed8[] = "Clarinet";
char Pipe[] = "Piccolo";
char Pipe2[] = "Flute";
char Pipe3[] = "Recorder";
char Pipe4[] = "Pan Flute";
char Pipe5[] = "Bottle Blow";
char Pipe6[] = "Shakuhachi";
char Pipe7[] = "Whistle";
char Pipe8[] = "Ocarina";
char SynthLead[] = "Syn Square Wave";
char SynthLead2[] = "Syn Sawtooth Wave";
char SynthLead3[] = "Syn Calliope";
char SynthLead4[] = "Syn Chiff";
char SynthLead5[] = "Syn Chrang";
char SynthLead6[] = "Syn Voice";
char SynthLead7[] = "Syn Fifths Sawtooth Wave";
char SynthLead8[] = "Syn Brass & Lead";
char SynthPad[] = "New Age Syn Pad";
char SynthPad2[] = "Warm Syn Pad";
char SynthPad3[] = "Polysynth Syn Pad";
char SynthPad4[] = "Choir Syn Pad";
char SynthPad5[] = "Bowed Syn Pad";
char SynthPad6[] = "Metal Syn Pad";
char SynthPad7[] = "Halo Syn Pad";
char SynthPad8[] = "Sweep Syn Pad";
char SynthEffects[] = "SFX Rain";
char SynthEffects2[] = "SFX Soundtrack";
char SynthEffects3[] = "SFX Crystal";
char SynthEffects4[] = "SFX Atmosphere";
char SynthEffects5[] = "SFX Brightness";
char SynthEffects6[] = "SFX Goblins";
char SynthEffects7[] = "SFX Echoes";
char SynthEffects8[] = "SFX Sci-Fi";
char Ethnic[] = "Sitar";
char Ethnic2[] = "Banjo";
char Ethnic3[] = "Shamisen";
char Ethnic4[] = "Koto";
char Ethnic5[] = "Kalimba";
char Ethnic6[] = "Bag Pipe";
char Ethnic7[] = "Fiddle";
char Ethnic8[] = "Shanai";
char Percussive[] = "Tinkle Bell";
char Percussive2[] = "Agogo";
char Percussive3[] = "Steel Drum";
char Percussive4[] = "Woodblock";
char Percussive5[] = "Taiko Drum";
char Percussive6[] = "Melodic Tom";
char Percussive7[] = "Syn Drum";
char Percussive8[] = "Reverse Cymbal";
char SoundEffects[] = "Guitar Fret Noise";
char SoundEffects2[] = "Breath Noise";
char SoundEffects3[] = "Seashore";
char SoundEffects4[] = "Bird Tweet";
char SoundEffects5[] = "Telephone Ring";
char SoundEffects6[] = "Helicopter";
char SoundEffects7[] = "Applause";
char SoundEffects8[] = "Gun Shot";
char None[] = "none";

DWORD *Instruments[] = {(DWORD*)Piano, (DWORD*)Piano2, (DWORD*)Piano3, (DWORD*)Piano4, (DWORD*)Piano5, (DWORD*)Piano6, (DWORD*)Piano7, (DWORD*)Piano8,\
(DWORD*)ChromaticPercussion, (DWORD*)ChromaticPercussion2, (DWORD*)ChromaticPercussion3, (DWORD*)ChromaticPercussion4, (DWORD*)ChromaticPercussion5, (DWORD*)ChromaticPercussion6, (DWORD*)ChromaticPercussion7, (DWORD*)ChromaticPercussion8,\
(DWORD*)Organ, (DWORD*)Organ2, (DWORD*)Organ3, (DWORD*)Organ4, (DWORD*)Organ5, (DWORD*)Organ6, (DWORD*)Organ7, (DWORD*)Organ8,\
(DWORD*)Guitar, (DWORD*)Guitar2, (DWORD*)Guitar3, (DWORD*)Guitar4, (DWORD*)Guitar5, (DWORD*)Guitar6, (DWORD*)Guitar7, (DWORD*)Guitar8,\
(DWORD*)Bass, (DWORD*)Bass2, (DWORD*)Bass3, (DWORD*)Bass4, (DWORD*)Bass5, (DWORD*)Bass6, (DWORD*)Bass7, (DWORD*)Bass8,\
(DWORD*)Strings, (DWORD*)Strings2, (DWORD*)Strings3, (DWORD*)Strings4, (DWORD*)Strings5, (DWORD*)Strings6, (DWORD*)Strings7, (DWORD*)Strings8,\
(DWORD*)Ensemble, (DWORD*)Ensemble2, (DWORD*)Ensemble3, (DWORD*)Ensemble4, (DWORD*)Ensemble5, (DWORD*)Ensemble6, (DWORD*)Ensemble7, (DWORD*)Ensemble8,\
(DWORD*)Brass, (DWORD*)Brass2, (DWORD*)Brass3, (DWORD*)Brass4, (DWORD*)Brass5, (DWORD*)Brass6, (DWORD*)Brass7, (DWORD*)Brass8,\
(DWORD*)Reed, (DWORD*)Reed2, (DWORD*)Reed3, (DWORD*)Reed4, (DWORD*)Reed5, (DWORD*)Reed6, (DWORD*)Reed7, (DWORD*)Reed8,\
(DWORD*)Pipe, (DWORD*)Pipe2, (DWORD*)Pipe3, (DWORD*)Pipe4, (DWORD*)Pipe5, (DWORD*)Pipe6, (DWORD*)Pipe7, (DWORD*)Pipe8,\
(DWORD*)SynthLead, (DWORD*)SynthLead2, (DWORD*)SynthLead3, (DWORD*)SynthLead4, (DWORD*)SynthLead5, (DWORD*)SynthLead6, (DWORD*)SynthLead7, (DWORD*)SynthLead8,\
(DWORD*)SynthPad, (DWORD*)SynthPad2, (DWORD*)SynthPad3, (DWORD*)SynthPad4, (DWORD*)SynthPad5, (DWORD*)SynthPad6, (DWORD*)SynthPad7, (DWORD*)SynthPad8,\
(DWORD*)SynthEffects, (DWORD*)SynthEffects2, (DWORD*)SynthEffects3, (DWORD*)SynthEffects4, (DWORD*)SynthEffects5, (DWORD*)SynthEffects6, (DWORD*)SynthEffects7, (DWORD*)SynthEffects8,\
(DWORD*)Ethnic, (DWORD*)Ethnic2, (DWORD*)Ethnic3, (DWORD*)Ethnic4, (DWORD*)Ethnic5, (DWORD*)Ethnic6, (DWORD*)Ethnic7, (DWORD*)Ethnic8,\
(DWORD*)Percussive, (DWORD*)Percussive2, (DWORD*)Percussive3, (DWORD*)Percussive4, (DWORD*)Percussive5, (DWORD*)Percussive6, (DWORD*)Percussive7, (DWORD*)Percussive8,\
(DWORD*)SoundEffects, (DWORD*)SoundEffects2, (DWORD*)SoundEffects3, (DWORD*)SoundEffects4, (DWORD*)SoundEffects5, (DWORD*)SoundEffects6, (DWORD*)SoundEffects7, (DWORD*)SoundEffects8,\
(DWORD*)None};
/*
char StudioBreakKit[24][19] = {\
"Kick",//36 C
"SideStick",//37 C#
"Snare",//38 D
"Snare Flam",//39 D#
"Snare Rim Shot",//40 E
"Tom Floor",//41 F
"Hihat Closed",//42 F#
"Tom Low",//43 G
"Hihat Pedal",//44 G#
"Tom Mid",//45 A
"Hihat Open",//46 A#
"Tom High",//47 B
"Snare Roll",//48 C
"Crash High",//49 C#
"Snare Ghost",//50 D
"Ride,"//51 D#
"China",//52 E
"Ride Bell",//53 F
"Tambourine Accent",//54 F#
"Splash",//55 G
"Cowbell Low",//56 G#
"Crash Low",//57 A
"Shaker Low",//58 A#
"Ride Edge"//59 B
};
*/
char Percussion[61][20] = {\
"High Q",//27 D#
"Slap",//28 E
"RecordScratch 2",//29 F
"RecordScratch 1",//30 F#
"Sticks",//31 G
"Click 1",//32 G#
"Click 2",//33 A
"Small Bell",//34 A#
"AcousticBassDrum",//35 B Beginning of General MIDI percussion
"Bass Drum 1",//36 C
"Side Stick",//37 C#
"Acoustic Snare",//38 D
"Hand Clap",//39 D#
"Electric Snare",//40 E
"Low Floor Tom",//41 F
"Closed Hi-Hat",//42 F#
"High Floor Tom",//43 G
"Pedal Hi-Hat",//44 G#
"Low Tom",//45 A
"Open Hi-Hat",//46 A#
"Low-Mid Tom",//47 B
"Hi-Mid Tom",//48 C
"Crash Cymbal 1",//49 C#
"High Tom",//50 D
"Ride Cymbal 1",//51 D#
"Chinese Cymbal",//52 E
"Ride Bell",//53 F
"Tambourine",//54 F#
"Splash Cymbal",//55 G
"Cowbell",//56 G#
"Crash Cymbal 2",//57 A
"Vibraslap",//58 A#
"Ride Cymbal 2",//59 B
"Hi Bongo",//60 C
"Low Bongo",//61 C#
"Mute Hi Conga",//62 D
"Open Hi Conga",//63 D#
"Low Conga",//64 E
"High Timbale",//65 F
"Low Timbale",//66 F#
"High Agogo",//67 G
"Low Agogo",//68 G#
"Cabasa",//69 A
"Maracas",//70 A#
"Short Whistle",//71 B
"Long Whistle",//72 C
"Short Guiro",//73 C#
"Long Guiro",//74 D
"Claves",//75 D#
"Hi Wood Block",//76 E
"Low Wood Block",//77 F
"Mute Cuica",//78 F#
"Open Cuica",//79 G
"Mute Triangle",//80 G#
"Open Triangle",//81 A Last of General MIDI percussion
"Shaker",//82 A#
"Sleigh Bells",//83 B
"Bell Tree",//84 C
"Castanets",//85 C#
"Mute Surdo",//86 D
"Open Surdo",//87 D#
};
char *ptrPercussion;

HWND hwnd, hwndOptionsDlg, hwndInstruments, hwndSustain = NULL, hwndChords = NULL, hwndChordName = NULL, hwndHelp, hwndLoop, hwndKeyboardPercus, hwndOpenList;
HWND hwndDynamics, hwndList, hwndEditList, hwndSound, hwndMixer = NULL, hwndSF, hwndPhase, hwndEditMyWave, hwndPlayList, hwndUser, hwndHook, hwndParent;
HWND hwndVirtualKeyboard, hwndVelocities;
HINSTANCE hInst;
HGLOBAL hResource;
HMENU hMenu, hMenuPopup, hMenu2, hMenu3, hMenu4, hMenu5;
HANDLE hFile, hFile2, hFile3;
RECT rect, blueRect0, blueRect1, blueRect2, blueRect3, InstrumentRect, loopRect, hiliteRect, Rect, Rect2, WorkArea, meterRect;
HDC hdc, hdcMem, hdcMem2, hdcMem3;
HBITMAP hBitmap, hBitmap2, hBitmap3;
PAINTSTRUCT ps;
HBRUSH hBrush, hBrush2, hGreyBrush, hBlackBrush, hInstrumentBrush[9], hInstrumentBrush16[16], hChords2BlueBrush, hDialogBrush, hDarkGreyBrush;
HPEN hPen, hGreyPen, hEditPen, hInstrumentPen[8], hDialogPen, hLimitPen, hWhitePen;
HPEN hBlackPen, hDarkGreyPen, hNullPen;
HPEN hRedPen, hRedSharpPen, hRedFlatPen;
HPEN hOrangePen, hOrangeSharpPen, hOrangeFlatPen;
HPEN hYellowPen, hYellowSharpPen, hYellowFlatPen;
HPEN hGreenPen, hGreenSharpPen, hGreenFlatPen;
HPEN hBlueGreenPen, hBlueGreenSharpPen, hBlueGreenFlatPen;
HPEN hBluePen, hBlueSharpPen, hBlueFlatPen;
HPEN hVioletPen, hVioletSharpPen, hVioletFlatPen;
HPEN hBrownPen, hBrownSharpPen, hBrownFlatPen;
HPEN hGreyNaturalPen, hGreySharpPen, hGreyFlatPen;
HFONT hFont, hSmallFont, hHelpFont, hLyricFont, hSharpFlatFont, hDialogFont, hNoteFont;
HGDIOBJ hOldPen, hOldFont, hOldBrush;
HCURSOR hCursor, hWaitingCursor;
LOGFONT lf, lf2, lf3, lf4, lf5, lf6, lf7;
char ComicSansMS[] = "Comic Sans MS";
char SansSerif[] = "Microsoft Sans Serif";
char Arial[] = "Arial";
char DialogLight[] = "Microsoft Dialog Light";
SIZE size, recordingSize, playingSize, showingnotesSize; //, NoteNameSize;
POINT point;
OPENFILENAME ofn, ofn2, ofn3;
HMIDIOUT hMidiOut, hMidisOut[16];
MIDIOUTCAPS moc;
HMIDIIN hMidiIn;
CRITICAL_SECTION csTickptr;
HINTERNET hOpen, hInternet, hConnectHandle, hResourceHandle;
MENUITEMINFO mii;

void ReadMidi(void);
void WriteMidi(void);
void ShowNaturalFlatSharp(int);
void GetTicksInNote(void);
void ChangeNoteType(char ch);
void Uncheck(UINT id);
void Uncheck2(void);
void StopTimers(void);
void StopTimer(void);
void Resort(void);
void ZeroEvents(void);
void ZeroEvent(int);
void DeleteDuplicateNotes(void);
void SaveText(int);
void MoveNoteUpDown(char);
void MoveNoteLeftRight(char);
void NewEvent(void);
void SaveEvents(void);
void ChangeInstrument(BYTE channel);
void ShowControl(char*);
void FillLogBuf(void);
BOOL CheckListBuf(void);
BOOL CheckForPortFile(void);
DWORD Atoi(char *ptr);
//void Itoa(int number, char *ch);
void ResetContent(DWORD*);
void DrawNote(int, int);
void ChangeVolume(void);
void UpdateVolChange(void);
void EditedLine(void);
void TitleBar(void);
void MixerTitleBar(void);
void GetEventTickptr(void);
void SetMasterVolume(void);
void RecordWave(void);
void PlayWave(void);
void FillMixerBuf(void);
void ClearMeter(void);
void GetWaveQuality(void);
void ShowQuality(void);
void ShowUnplayedNote(int);
void CheckForNewInstrument(void);
void FillLines(void);
void GetPixelsBetweenLines(void);
void Metronome(void);
void PlayPlayListProc(void);
int CALLBACK OptionsProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK ParametersProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK InstrumentsProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK InstrumProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK PercussionProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Percussion3Proc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK AllBeatsPerMinuteProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK MasterVolumeProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK RecordProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK RecordInstructions1(HWND, UINT, WPARAM, LPARAM);
int CALLBACK RecordInstructions2(HWND, UINT, WPARAM, LPARAM);
int CALLBACK KeyTimeProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK HelpProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Help2Proc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK SustainProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK MixerProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK SoundsProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK NoteVolumeProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK ListProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK NewProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK AdvancedInstrumentProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK DynamicEffectsProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK LyricProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK OpeningTextProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK OptionalTextProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK WaveOptionsProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK NewChannelProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK PlayListProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK UserProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK OctaveShiftProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK ChangeInstrumentProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK PortamentoProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK ChangeVelocitiesProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK LoopProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditMyWaveProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK ChordsProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK KeyboardPercusProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK SoundProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK VirtualKeyboardProc(HWND, UINT, WPARAM, LPARAM);
void CALLBACK waveInProc(HWAVEIN, UINT, DWORD, DWORD, DWORD);
void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
void CALLBACK waveOutProc4(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
UINT CALLBACK OFNHookProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK ShowNoteProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK CutSectionProc(HWND, UINT, WPARAM, LPARAM);
//void GetDiatonicNotes(int, int);

WNDPROC pHelpProc, pListProc, pMeterProc, pSFProc;
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
MSG          msg;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR CmdLine, int iCmdShow)
{
	WNDCLASS     wndclass;

	hInst = hInstance;
	hBrush = CreateSolidBrush(WHITE);

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE (IDI_ICON1));
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = hBrush;
	wndclass.lpszMenuName  = "MENU";
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	wndclass.lpfnWndProc = EditMyWaveProc;
	wndclass.hIcon = NULL;
	wndclass.lpszClassName = EditMyWave;
	RegisterClass(&wndclass);

	wndclass.lpfnWndProc = LoopProc;
	wndclass.hIcon = NULL;
	wndclass.lpszClassName = PercussionLoop;
	RegisterClass(&wndclass);

	wndclass.lpfnWndProc = ChordsProc;
	wndclass.hIcon = NULL;
	wndclass.lpszClassName = Chords;
	RegisterClass(&wndclass);

	wndclass.lpfnWndProc = KeyboardPercusProc;
	wndclass.hIcon = NULL;
	wndclass.lpszClassName = Keyboardpercus;
	RegisterClass(&wndclass);

	wndclass.lpfnWndProc = SoundProc;
	wndclass.hIcon = NULL;
	wndclass.lpszClassName = Sound;
	RegisterClass(&wndclass);

	wndclass.lpfnWndProc = VirtualKeyboardProc;
	wndclass.hIcon = NULL;
	wndclass.lpszClassName = VirtualKeyboard;
	RegisterClass(&wndclass);

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInst, NULL);

	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if ((hwndInstruments) && IsDialogMessage(hwndInstruments, &msg)) { // showinginstruments
			if (msg.message == WM_KEYDOWN) {
				if (msg.wParam == VK_TAB)
					PostMessage(hwndInstruments, WM_USER20, VK_TAB, msg.lParam);
				else if ((EWQL) && (msg.wParam >= 0x21) && (msg.wParam == VK_NEXT))
					PostMessage(hwndInstruments, WM_USER21, 0, 0);
				else if ((EWQL) && (msg.wParam >= 0x21) && (msg.wParam == VK_PRIOR))
					PostMessage(hwndInstruments, WM_USER22, 0, 0);
			}
		}
		else if (((hwndOptionsDlg == NULL) && (hwndDynamics == NULL) && (hwndMixer == NULL))
		|| ((!IsDialogMessage(hwndOptionsDlg, &msg)) && (!IsDialogMessage(hwndDynamics, &msg)) && (!IsDialogMessage(hwndMixer, &msg)))) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return msg.wParam;
}


int Compare(const void *x, const void *y)
{//for qsort
	if (((struct EVENT*)x)->tickptr >= ((struct EVENT*)y)->tickptr)
		return 1;
	else if (((struct EVENT*)y)->tickptr >= ((struct EVENT*)x)->tickptr)
		return -1;
	else
		return 0;
}

void Menus(void)
{
	DWORD badDeviceIn;

	MIDImenu = 1;
	z = midiOutGetNumDevs();
	if (z) {
		if (z >= 1) {
			hMenuPopup = CreatePopupMenu();
			InsertMenu(hMenu, MIDImenu, MF_BYPOSITION|MF_POPUP, (UINT)hMenuPopup, "&MIDI Output");
		}
		for (x = 0; x < (int)z; x++) {
			if (MMSYSERR_NOERROR == midiOutGetDevCaps(x, &moc, sizeof(moc))) {
				AppendMenu(hMenuPopup, MF_BYPOSITION|MF_POPUP|MF_STRING, IDM_OUTPUT + x, moc.szPname);
				strcpy(MidiOut[x], moc.szPname);
			}
		}
		LastOutputDevice = x;
		for (y = 0; y <= LastOutputDevice; y++) {
			for (x = 0; MidiOutIni[x] != 0; x++) {
				if (MidiOut[y][x] != MidiOutIni[x]) {
					break;
				}
			}
			if (MidiOutIni[x] == 0) {
				DeviceOut = y;
				break;
			}
		}
		CheckMenuItem(hMenuPopup, IDM_OUTPUT + DeviceOut, MF_CHECKED);
	}
	z = midiInGetNumDevs();
	if (z) {
		hMenuPopup = CreatePopupMenu();
		InsertMenu(hMenu, MIDImenu, MF_BYPOSITION|MF_POPUP, (UINT)hMenuPopup, "&MIDI Input");
		MIDImenu++;
		for (x = 0; x < (int)z; x++) {
			if (MMSYSERR_NOERROR == midiInGetDevCaps(x, &mic, sizeof(mic))) {
				if (0 == strcmp(mic.szPname, "LoopBe Internal MIDI"))
					badDeviceIn = x;
				AppendMenu(hMenuPopup, MF_BYPOSITION|MF_POPUP|MF_STRING, IDM_INPUT + x, mic.szPname);
				strcpy(MidiIn[x], mic.szPname);
			}
		}
		AppendMenu(hMenuPopup, MF_BYPOSITION|MF_POPUP|MF_STRING, IDM_INPUT + x, "No Input");
		strcpy(MidiIn[x], "No Input");
		x++;
		LastInputDevice = x;
		for (y = 0; y < LastInputDevice; y++) {
			for (x = 0; MidiInIni[x] != 0; x++) {
				if (MidiIn[y][x] != MidiInIni[x]) {
					break;
				}
			}
			if (MidiInIni[x] == 0) {
				DeviceIn = y;
				break; // got DeviceIn from MidiInIni
			}
		}
		if ((DeviceIn == badDeviceIn) && (DeviceIn == 0))
			DeviceIn++;
		else if ((DeviceIn == badDeviceIn) && (DeviceIn != 0))
			DeviceIn--;
		CheckMenuItem(hMenuPopup, IDM_INPUT + DeviceIn, MF_CHECKED);
		
	}
	CheckMenuItem(hMenu, ID_NOTETYPE_QUARTER, MF_CHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME0, MF_CHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO0, MF_CHECKED);
}

void CALLBACK ChordTimerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	for ( ; timePtr < lastNote; timePtr++) {
		if (!EWQL)
			midiOutShortMsg(hMidiOut, (0x90|ChordEvent[timePtr].channel) | (ChordEvent[timePtr].note << 8));// end note
		else
			midiOutShortMsg(hMidisOut[Event[timePtr].port], (0x90|ChordEvent[timePtr].channel) | (ChordEvent[timePtr].note << 8));// end note
	}
	timeKillEvent(uTimer9ID);
	timeEndPeriod(TIMER_RESOLUTION);
	uTimer9ID = 0;
}

void CALLBACK Chords2TimerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	int x;

	if (!EWQL)
		for (x = 0; x < cn; x++)
			midiOutShortMsg(hMidiOut, (0x90|ActiveChannel) | (Chords2Notes[x] << 8));// end note
	else
		for (x = 0; x < cn; x++)
			midiOutShortMsg(hMidisOut[Port], (0x90|ActiveChannel) | (Chords2Notes[x] << 8));// end note
	timeKillEvent(uTimer10ID);
	timeEndPeriod(TIMER_RESOLUTION);
	uTimer10ID = 0;
}

void CALLBACK TimerFunc11(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	PostMessage(hwnd, WM_USER11, 0, 0);
}

void CALLBACK TimerFunc12(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	PostMessage(hwnd, WM_USER12, 0, 0);
}

void CALLBACK TimerFunc13(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{ // Metronome
	midiOutShortMsg(hMidiOut, 0x80 | (40 << 16) | (90 << 8)); // note off, velocity 40, note number 90
	midiOutShortMsg(hMidiOut, 0x90 | (40 << 16) | (90 << 8));
}

void CALLBACK TimerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{// ***from PLAY***
	do {
		if ((Event[timePtr].note) && (ActiveChannels[Event[timePtr].channel])) {
			if (!EWQL)
				midiOutShortMsg(hMidiOut, Event[timePtr].message);
			else {
				midiOutShortMsg(hMidisOut[Event[timePtr].port], Event[timePtr].message);
			}
			if ((Event[timePtr].velocity))
				PostMessage(hwnd, WM_USER2, (WPARAM)timePtr, 0);
			else if (virtualkeyboard) // note off
				PostMessage(hwnd, WM_USER8, (WPARAM)timePtr, 0);
			if (keyboardactive) {
				Tick = Event[timePtr].tickptr;
				EnterCriticalSection(&csTickptr); Tickptr = Tick; LeaveCriticalSection(&csTickptr);// for WM_USER
			}
		}
		else if ((Event[timePtr].note == 0) && (Event[timePtr].message)){
			message = Event[timePtr].message;
			if ((message & 0xFFF0) == 0x07B0) {
				chan = (BYTE)(message & 0xF);
				origVol[chan] = message >> 16;
				vol = origVol[chan] + volchange;
				if (vol > 127)
					vol = 127;
				else if (vol < 0)
					vol = 0;
				message &= 0x00FFFF;
				message |= (vol << 16);
			}
			if (!EWQL)
				midiOutShortMsg(hMidiOut, message);
			else
				midiOutShortMsg(hMidisOut[Event[timePtr].port], message);
		}
		else if (Event[timePtr].dMilliSecondsPerTick) {
			oldMilliSecondsPerTick = Event[timePtr].dMilliSecondsPerTick;
			dMilliSecondsPerTick = oldMilliSecondsPerTick + (oldMilliSecondsPerTick * TempoChange);
		}
		if (newTempo) {
			newTempo = FALSE;
			dMilliSecondsPerTick = oldMilliSecondsPerTick + (oldMilliSecondsPerTick * TempoChange);
		}
 		timePtr++;
	} while ((Event[timePtr].tickptr - Event[timePtr-1].tickptr) <= MagicNumber); // start notes that are within 4 tickptrs of a beginning or ending of another note

	if ((timePtr < lastE) && (!done)) {// e
		time = (int)(dMilliSecondsPerTick * (double)(Event[timePtr].tickptr - Event[timePtr-1].tickptr)); // time is length of note in milliseconds
		uTimerID = timeSetEvent(time, TIMER_RESOLUTION, TimerFunc, 0, TIME_ONESHOT);
	}

	else {
		done = TRUE;
		PostMessage(hwnd, WM_USER3, 0, 0);// not SendMessage because WM_USER2 needs to finish before WM_USER3
	}
}

void CALLBACK TimerFunc2(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{// from ID_RECORDMIDIKEYBOARD
	if (!EWQL) {
//		if (MMSYSERR_NOERROR != midiOutShortMsg(hMidiOut, 0x90 | MetronomeChannel-1 | (MetronomeVolume << 16) | (MetronomeNote << 8))) {
//			midiOutShortMsg(hMidiOut, 0x80 | (10 << 16) | (90 << 8)); // note off, velocity 10, note number 90
//			midiOutShortMsg(hMidiOut, 0x90 | (10 << 16) | (90 << 8));
//		}
		midiOutShortMsg(hMidiOut, 0x90 | MetronomeChannel-1 | (MetronomeVolume << 16) | (MetronomeNote << 8));
	}
	else
		midiOutShortMsg(hMidisOut[MetronomePort], 0x90 | MetronomeChannel-1 | (MetronomeVolume << 16) | (MetronomeNote << 8));
	if (counter != 0xFF)
		counter--;
	if (counter == 0) {
		counter = 0xFF;
		firstnote = FALSE;
		if (Startime == 0) {
			timeBeginPeriod(0);
			Startime = timeGetTime();
			timeEndPeriod(0);
		}
		Pixel = Tickptr * 40 / TicksPerQuarterNote;// where beat should start
		pixel = Pixel;
		Beats = 0;
	}
	if (!firstnote) {
		if ((pixel - FirstPixelOnPage) < PixelsPerPage) {
			l = line;
			while (pixel > (int)(Lines[l].pixel + Lines[l].PixelsPerLine)) {
				l++;
			}
			if (l >= 300) { // should never happen
				SendMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
				return;
			}
			X = pixel % Lines[l].PixelsPerLine;
			Y = Lines[l].rowonpage + MyExtraSpace - NoteMiddle;
			hdc = GetDC(hwnd);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
 			hOldPen = SelectObject(hdc, hPen);
			MoveToEx(hdc, X, Y, NULL);
			LineTo(hdc, X, Y + PixelsInGrandStaff);// vertical line
			SelectObject(hdc, hOldPen);
			ReleaseDC(hwnd, hdc);
		}
		else {// new page
			PageBegin += lastRowLoc;
			FirstPixelOnPage = pixel;
			TopLeftPtr = e;
			Page++;
			_itoa(Page+1, &page[5], 10);
			line = Page * Rows;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
		}
		pixel += 40;
		Beats++;
	}
}

void EndIt(DWORD message)
{
	if (!EWQL) {
		midiOutShortMsg(hMidiOut, message);
		midiOutReset(hMidiOut);
	}
	else {
		midiOutShortMsg(hMidisOut[TempEvent.port], message);
		midiOutReset(hMidisOut[TempEvent.port]);
	}
}

//dwUser = TempEvent.message = (0x90|ActiveChannel) | (TempEvent.velocity << 16) | (TempEvent.note << 8);
void CALLBACK TimerFunc3(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{// from WM_LBUTTONUP
	timeKillEvent(uTimer3ID);
	timeEndPeriod(TIMER_RESOLUTION);
	uTimer3ID = 0;
	EndIt(dwUser);
}
void CALLBACK TimerFunc7(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{// from WM_LBUTTONUP
	timeKillEvent(uTimer7ID);
	timeEndPeriod(TIMER_RESOLUTION);
	uTimer7ID = 0;
	EndIt(dwUser);
}
void CALLBACK TimerFunc8(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{// from WM_LBUTTONUP
	timeKillEvent(uTimer8ID);
	timeEndPeriod(TIMER_RESOLUTION);
	uTimer8ID = 0;
	EndIt(dwUser);
}


void CALLBACK MidiInProc(HMIDIIN hMidiIn, WORD wMsg, DWORD dwInstance, DWORD dwParam1, DWORD dwParam2)
{//from midiInOpen (if a MIDI keyboard is attached)
	if (wMsg == MIM_DATA)
		PostMessage(hwnd, WM_USER, (WPARAM)(dwParam1 & 0xFF), (LPARAM)((dwParam1 >> 8) & 0xFFFF));//dwParam1 contains velocity, note, and status bytes
}

//sub-class procedure
LRESULT CALLBACK HProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_KEYDOWN) {
		if (wParam == VK_ESCAPE) // || (wParam == VK_SPACE || (wParam == VK_RETURN)
			SendMessage(hwndHelp, WM_CLOSE, 0, 0);
	}
	return CallWindowProc(pHelpProc, hwnd2, message, wParam, lParam);
}

//sub-class procedure
LRESULT CALLBACK MeterProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdcMeter;
	PAINTSTRUCT psMeter;

	if (message == WM_PAINT) {
		hdcMeter = BeginPaint(hwndMeter, &psMeter);
		hOldPen = SelectObject(hdcMeter, hDialogPen);
		MoveToEx(hdcMeter, 2, meterRect.bottom, NULL);
		LineTo(hdcMeter, 2, meterRect.top);
		MoveToEx(hdcMeter, 3, meterRect.bottom, NULL);
		LineTo(hdcMeter, 3, meterRect.top);
		MoveToEx(hdcMeter, 4, meterRect.bottom, NULL);
		LineTo(hdcMeter, 4, meterRect.top);
		MoveToEx(hdcMeter, 5, meterRect.bottom, NULL);
		LineTo(hdcMeter, 5, meterRect.top);
		MoveToEx(hdcMeter, 6, meterRect.bottom, NULL);
		LineTo(hdcMeter, 6, meterRect.top);

		SelectObject(hdcMeter, hOldPen);
		MoveToEx(hdcMeter, 2, meterRect.bottom, NULL);
		LineTo(hdcMeter, 2, vert);
		MoveToEx(hdcMeter, 3, meterRect.bottom, NULL);
		LineTo(hdcMeter, 3, vert);
		MoveToEx(hdcMeter, 4, meterRect.bottom, NULL);
		LineTo(hdcMeter, 4, vert);
		MoveToEx(hdcMeter, 5, meterRect.bottom, NULL);
		LineTo(hdcMeter, 5, vert);
		MoveToEx(hdcMeter, 6, meterRect.bottom, NULL);
		LineTo(hdcMeter, 6, vert);
		EndPaint(hwndMeter, &psMeter);
	}
	return CallWindowProc(pMeterProc, hwnd2, message, wParam, lParam);
}

//sub-class procedure
LRESULT CALLBACK listProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_CHAR) {
		if (wParam == 24)// Ctrl-X
			SendMessage(hwndList, WM_COMMAND, EDIT_CUT, 0);
		else if (wParam == 3)// Ctrl-C
			SendMessage(hwndEditList, LB_GETTEXT, index, (LPARAM)ListBuf);
		else if (wParam == 22)// Ctrl-V
			SendMessage(hwndList, WM_COMMAND, EDIT_PASTE, 0);

		else if (wParam == 25) {// Ctrl-Y
			if ((pUndo < 9999) && (LastEventInUndo[pUndo+1] != -1)) {
				pUndo++;
				for (x = 0; x < (int)LastEventInUndo[pUndo]; x++)
					Event[x] = UndoEvent[pUndo][x];
				e = LastEventInUndo[pUndo];
				FillRect(hdcMem, &rect, hBrush);
//				TextOut(hdcMem, rect.right >> 1, 0, _itoa(pUndo, temp, 10), lstrlen(temp));
				ResetContent(&everything);
			}
		}

		else if (wParam == 26) {// Ctrl-Z
			if ((pUndo == 0) && (Loop != 0))
				pUndo = 10000;
			if ((pUndo) && (LastEventInUndo[pUndo-1] != -1)) {
				pUndo--;
				e = LastEventInUndo[pUndo];
				for (x = 0; x < (int)e; x++)
					Event[x] = UndoEvent[pUndo][x];
				FillRect(hdcMem, &rect, hBrush);
//				TextOut(hdcMem, rect.right >> 1, 0, _itoa(pUndo, temp, 10), lstrlen(temp));
				ResetContent(&everything);
			}
		}

		else if ((wParam == 'H') || (wParam == 'h')) {
			Help = ListHelp;
			DialogBox(hInst, "LISTHELP", hwnd, Help2Proc);
			SetFocus(hwndList);
		}
	}
	else if (message == WM_KEYDOWN) {
		if (wParam == VK_ESCAPE)
			SendMessage(hwndList, WM_CLOSE, 0, 0);
		else if (wParam == VK_DELETE)
			SendMessage(hwndList, WM_COMMAND, EDIT_DELETE, 0);
		else if (wParam == VK_F2)
			SendMessage(hwndList, WM_COMMAND, EDIT_RENAME, 0);
		else if (wParam == VK_INSERT)
			SendMessage(hwndList, WM_COMMAND, EDIT_NEW, 0);
	}
	return CallWindowProc(pListProc, hwnd2, message, wParam, lParam);
}



LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		ol = 0;
		virtualkeyboard = FALSE;
		WaveBuf = WaveBuf2 = NULL;

		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.hwndOwner         = hwnd;
		ofn.hInstance         = hInst;
		ofn.lpstrFilter       = " MIDI files\0""*.mid;*.midi\0\0";
		ofn.lpstrFile         = FullFilename;
		ofn.lpstrFileTitle    = Filename;
//		ofn.Flags =					OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_ENABLESIZING|OFN_EXPLORER|OFN_ENABLEHOOK;
		ofn.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_ENABLESIZING|OFN_EXPLORER;
		ofn.lpstrTitle        = NULL;
		ofn.lpstrDefExt       = "mid";
		ofn.nMaxFile          = MAX_PATH;
		ofn.lpstrCustomFilter = 0;
		ofn.nMaxCustFilter    = 0;
		ofn.nFilterIndex      = 0;
		ofn.nMaxFileTitle     = MAX_PATH;
		ofn.lpstrInitialDir   = NULL;
		ofn.nFileOffset       = 0;
		ofn.nFileExtension    = 0;
		ofn.lCustData         = 0;
		ofn.lpfnHook          = &OFNHookProc;// see bigopen
		ofn.lpTemplateName    = NULL;

		ofn2.lStructSize       = sizeof(OPENFILENAME);
		ofn2.hwndOwner         = hwnd;
		ofn2.hInstance         = hInst;
		ofn2.lpstrFilter       = " WAVE files\0""*.wav;*.wave\0\0";
		ofn2.lpstrFile         = FullFilename2;
		ofn2.lpstrFileTitle    = Filename;
		ofn2.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_ENABLESIZING;
		ofn2.lpstrTitle        = NULL;
		ofn2.lpstrDefExt       = "wav";
		ofn2.nMaxFile          = MAX_PATH;
		ofn2.lpstrCustomFilter = NULL;
		ofn2.nMaxCustFilter    = 0;
		ofn2.nFilterIndex      = 0;
		ofn2.nMaxFileTitle     = MAX_PATH;
		ofn2.lpstrInitialDir   = NULL;
		ofn2.nFileOffset       = 0;
		ofn2.nFileExtension    = 0;
		ofn2.lCustData         = 0;
		ofn2.lpfnHook          = NULL;
		ofn2.lpTemplateName    = NULL;
/*
		ofn3.lStructSize       = sizeof(OPENFILENAME);
		ofn3.hwndOwner         = hwnd;
		ofn3.hInstance         = hInst;
		ofn3.lpstrFilter       = " TEXT files\0""*.txt\0\0";
		ofn3.lpstrFile         = FullFilename;
		ofn3.lpstrFileTitle    = Filename;
		ofn3.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_ENABLESIZING|OFN_ALLOWMULTISELECT|OFN_EXPLORER;
		ofn3.lpstrTitle        = NULL;
		ofn3.lpstrDefExt       = "txt";
		ofn3.nMaxFile          = MAX_PATH;
		ofn3.lpstrCustomFilter = NULL;
		ofn3.nMaxCustFilter    = 0;
		ofn3.nFilterIndex      = 0;
		ofn3.nMaxFileTitle     = MAX_PATH;
		ofn3.lpstrInitialDir   = NULL;
		ofn3.nFileOffset       = 0;
		ofn3.nFileExtension    = 0;
		ofn3.lCustData         = 0;
		ofn3.lpfnHook          = NULL;
		ofn3.lpTemplateName    = NULL;
*/
		lf.lfHeight = -15;
		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 3;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x22;
		for (x = 0; ComicSansMS[x] != 0; x++)
			lf.lfFaceName[x] = ComicSansMS[x];
		lf.lfFaceName[x] = 0;
		hFont = CreateFontIndirect(&lf);

		lf2.lfHeight = -11;
		lf2.lfWeight = 700;
		lf2.lfItalic = 0;
		lf2.lfUnderline = 0;
		lf2.lfStrikeOut = 0;
		lf2.lfCharSet = 0;
		lf2.lfOutPrecision = 1;
		lf2.lfClipPrecision = 2;
		lf2.lfQuality = 1;
		lf2.lfPitchAndFamily = 0x22;
		for (x = 0; DialogLight[x] != 0; x++)
			lf2.lfFaceName[x] = DialogLight[x];
		lf2.lfFaceName[x] = 0;
		hSmallFont = CreateFontIndirect(&lf2);

		lf3.lfHeight = -15;
		lf3.lfWeight = 700;
		lf3.lfItalic = 0;
		lf3.lfUnderline = 0;
		lf3.lfStrikeOut = 0;
		lf3.lfCharSet = 0;
		lf3.lfOutPrecision = 3;
		lf3.lfClipPrecision = 2;
		lf3.lfQuality = 1;
		lf3.lfPitchAndFamily = 0x22;
		for (x = 0; Arial[x] != 0; x++)
			lf3.lfFaceName[x] = Arial[x];
		lf3.lfFaceName[x] = 0;
		hHelpFont = CreateFontIndirect(&lf3);

		lf4.lfEscapement = lf4.lfOrientation = 0;
		lf4.lfHeight = -13;
		lf4.lfWeight = 400;
		lf4.lfItalic = 0;
		lf4.lfUnderline = 0;
		lf4.lfStrikeOut = 0;
		lf4.lfCharSet = 0;
		lf4.lfOutPrecision = 3;
		lf4.lfClipPrecision = 2;
		lf4.lfQuality = 1;
		lf4.lfPitchAndFamily = 0x22;
		for (x = 0; SansSerif[x] != 0; x++)
			lf4.lfFaceName[x] = SansSerif[x];
		lf4.lfFaceName[x] = 0;
		hLyricFont = CreateFontIndirect(&lf4);

		lf5.lfHeight = -12;
		lf5.lfWeight = 400;
		lf5.lfItalic = 0;
		lf5.lfUnderline = 0;
		lf5.lfStrikeOut = 0;
		lf5.lfCharSet = 0;
		lf5.lfOutPrecision = 1;
		lf5.lfClipPrecision = 2;
		lf5.lfQuality = 1;
		lf5.lfPitchAndFamily = 0x22;
		for (x = 0; SansSerif[x] != 0; x++)
			lf5.lfFaceName[x] = SansSerif[x];
		lf5.lfFaceName[x] = 0;
		hDialogFont = CreateFontIndirect(&lf5);

		lf6.lfHeight = -13;
		lf6.lfWeight = 400;
		lf6.lfItalic = 1;
		lf6.lfUnderline = 0;
		lf6.lfStrikeOut = 0;
		lf6.lfCharSet = 0;
		lf6.lfOutPrecision = 3;
		lf6.lfClipPrecision = 2;
		lf6.lfQuality = 1;
		lf6.lfPitchAndFamily = 0x22;
		for (x = 0; SansSerif[x] != 0; x++)
			lf6.lfFaceName[x] = SansSerif[x];
		lf6.lfFaceName[x] = 0;
		hSharpFlatFont = CreateFontIndirect(&lf6);

		lf7.lfHeight = -15;
		lf7.lfWeight = 700;
		lf7.lfItalic = 0;
		lf7.lfUnderline = 0;
		lf7.lfStrikeOut = 0;
		lf7.lfCharSet = 0;
		lf7.lfOutPrecision = 3;
		lf7.lfClipPrecision = 2;
		lf7.lfQuality = 1;
		lf7.lfPitchAndFamily = 0x22;
		for (x = 0; Arial[x] != 0; x++)
			lf7.lfFaceName[x] = Arial[x];
		lf7.lfFaceName[x] = 0;
		hNoteFont = CreateFontIndirect(&lf7);

		hFile = CreateFile("Percussion.txt", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			if (fileSize = GetFileSize(hFile, NULL)) {
				ReadFile(hFile, &PercussionBuf, fileSize, &dwBytesRead, NULL);
				myPercussion = TRUE;
				for (x = 0, y = 0; x < 61; x++) {
					if ((x < 61) && (y >= (int)fileSize)) // Percussion has to have 61 entries
						Percussion[x][0] = 0;
					else {
						for (z = 0; (y < (int)fileSize) && (PercussionBuf[y] != '\r'); y++, z++)
							Percussion[x][z] = PercussionBuf[y]; // char Percussion[61][20]
						Percussion[x][z] = 0;
						y += 2;
					}
				}
			}
			CloseHandle(hFile);
		}

		if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0)) {
			WorkArea.right = GetSystemMetrics(SM_CXFULLSCREEN);// backup
			WorkArea.bottom = GetSystemMetrics(SM_CYFULLSCREEN);// backup
		}
		hWaitingCursor = LoadCursor(NULL, IDC_WAIT);
		hResource = LoadResource(hInst, FindResource(hInst, "USING", "TEXT"));// see PianoRollComposer.rc for this
		hBrush2 = CreateSolidBrush(0xF0A0A0);
		hGreyBrush = CreateSolidBrush(0xE8E8E8);
		hBlackBrush = CreateSolidBrush(0);
		hDarkGreyBrush = CreateSolidBrush(0x909090);
		hDialogBrush = GetSysColorBrush(COLOR_MENUBAR);
		if (NULL == hDialogBrush)
			hDialogBrush = GetSysColorBrush(DIALOGCOLOR);
		hChords2BlueBrush = CreateSolidBrush(0xE9A982);
		hBlueBrush = CreateSolidBrush(0xF00000);
		hB1 = CreateSolidBrush(WHITE);		
		hB2 = CreateSolidBrush(0xF0F0F0);
		for (channel = 0; channel < 10; channel++)
			hInstrumentBrush[channel] = CreateSolidBrush(colors[channel]);
		for (x = 0, channel = 8; channel < 16; x++, channel++)
				hInstrumentPen[x] = CreatePen(PS_SOLID, 0, colors[channel]);
		for (x = 0; x < 16; x++)
			hInstrumentBrush16[x] = CreateSolidBrush(colors[x]);
		InstrumentRect.left = 0;
		InstrumentRect.top = 0;
		InstrumentRect.right = 20;
		InstrumentRect.bottom = 15;
		for (x = 0; x < 16; x++) {
			continuousustain[x] = 0;
			reverb[x] = 0;
			chorus[x] = 0;
			ModWheel[x] = 0;
		}

		inifile = blackbackground = inwhite = FALSE;
		hFile = CreateFile(IniFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			inifile = TRUE;
			if (IniBufSize = GetFileSize(hFile, NULL)) {
				ReadFile(hFile, IniBuf, IniBufSize, &dwBytesRead, NULL);

				for (x = 0; x < IniBufSize; x++) {
					if (IniBuf[x] != '=')
						continue;
					if ((IniBuf[x-5] == 'B') && (IniBuf[x-4] == 'l') && (IniBuf[x-3] == 'a') && (IniBuf[x-2] == 'c') && (IniBuf[x-1] == 'k')) {
						blackbackground = inwhite = TRUE;
						DeleteObject(hBrush);
						hBrush = CreateSolidBrush(BLACK);
					}
					else if ((IniBuf[x-1] == 'l') && (IniBuf[x-10] == 'A')) {// Accidental=
						x++;
						if (IniBuf[x] == '#')
							usingsharp = TRUE;
						else
							usingsharp = FALSE;
					}
					else if ((IniBuf[x-1] == 's') && (IniBuf[x-10] == 'P')) {// ShowParameters=
						x++;
						if (IniBuf[x] == 'Y')
							show = TRUE;
						else
							show = FALSE;
					}
					else if ((IniBuf[x-1] == 'e') && (IniBuf[x-6] == 'V')) {// Volume=
						x++;
						Volume = atoi(&IniBuf[x]);
					}
					else if ((IniBuf[x-6] == 'M') && (IniBuf[x-12] == 'B')) {// Beats/Minute=
						x++;
						InitialBeatsPerMinute = atoi(&IniBuf[x]);
					}
					else if ((IniBuf[x-2] == 'r') && (IniBuf[x-5] == 'E')) {// NoteEvery=
						x++;
						NoteEvery = atoi(&IniBuf[x]);
					}
					else if ((IniBuf[x-1] == 'y') && (IniBuf[x-3] == 'K')) {// ShowKey=
						x++;
						if (IniBuf[x] == 'Y')
							showkey = TRUE;
						else
							showkey = FALSE;
					}
					else if ((IniBuf[x-2] == 'm') && (IniBuf[x-4] == 'T')) {// ShowTime=
						x++;
						if (IniBuf[x] == 'Y')
							showtime = TRUE;
						else
							showtime = FALSE;
					}
					else if ((IniBuf[x-2] == 'e') && (IniBuf[x-5] == 'N') && (IniBuf[x-13] == 'S')) {// ShowNoteNames=
						x++;
						if (IniBuf[x] == 'Y')
							shownotenames = TRUE;
						else
							shownotenames = FALSE;
					}
					else if ((IniBuf[x-2] == 'M') && (IniBuf[x-4] == 'B')) {// ShowBPMs=
						x++;
						if (IniBuf[x] == 'Y')
							showbeatsperminute = TRUE;
						else
							showbeatsperminute = FALSE;
					}
					else if ((IniBuf[x-7] == 'N') && (IniBuf[x-14] == 'M')) {// ShowMeasureNumbers=
						x++;
						if (IniBuf[x] == 'Y')
							showmeasurenumber = TRUE;
						else
							showmeasurenumber = FALSE;
					}
					else if ((IniBuf[x-10] == 'V') && (IniBuf[x-9] == 'e')) {// ShowNoteVelocities=
						x++;
						if (IniBuf[x] == 'Y')
							showvolumes = TRUE;
						else
							showvolumes = FALSE;
					}
					else if ((IniBuf[x-7] == 'P') && (IniBuf[x-9] == 'A')) {// ShowNameAtPointer=
						x++;
						if (IniBuf[x] == 'Y')
							shownameatpointer = TRUE;
						else
							shownameatpointer = FALSE;
					}
					else if ((IniBuf[x-1] == 't') && (IniBuf[x-10] == 'I')) {// ShowNewInstrument=
						x++;
						if (IniBuf[x] == 'Y')
							shownewinstrument = TRUE;
					}
					else if ((IniBuf[x-7] == 'N') && (IniBuf[x-6] == 'u')) {// ShowNumbers=
						x++;
						if (IniBuf[x] == 'Y')
							shownumbers = TRUE;
						else
							shownumbers = FALSE;
					}
					else if ((IniBuf[x-7] == 'F') && (IniBuf[x-11] == 'S')) {// ShowFingers=
						x++;
						if (IniBuf[x] == 'Y')
							showfingers = TRUE;
						else
							showfingers = FALSE;
					}

					else if ((IniBuf[x-8] == 'V') && (IniBuf[x-13] == 'A')) {// AddedVelocity=
						for (x++, y = 0; (x < IniBufSize) && (IniBuf[x] != '\r'); x++, y++)
							addedVelocity[y] = IniBuf[x];
						addedVelocity[y] = 0;
						AddedVelocity = Atoi(addedVelocity);
					}

					else if ((IniBuf[x-7] == 'Q') && (IniBuf[x-1] == 'y')) {// RecordQuality=
						WaveOptionsIndex = IniBuf[x+1] - '1';
						GetWaveQuality();
					}
					else if ((IniBuf[x-9] == 'M') && (IniBuf[x-5] == 'I')) {// MidiInput=
						saveX = x;
						for (x++, y = 0; (x < IniBufSize) && (IniBuf[x] != '\r'); x++, y++)
							MidiInIni[y] = IniBuf[x];
						MidiInIni[y] = 0;
					}
					else if ((IniBuf[x-10] == 'M') && (IniBuf[x-6] == 'O')) {// MidiOutput=
						saveX = x;
						for (x++, y = 0; (x < IniBufSize) && (IniBuf[x] != '\r'); x++, y++)
							MidiOutIni[y] = IniBuf[x];
						MidiOutIni[y] = 0;
					}
					else if ((IniBuf[x-8] == 'V') && (IniBuf[x-9] == 't') && (IniBuf[x-16] == 'C')) { // ConstantVelocity=
						if (IniBuf[x+1] != '\r') {
							ConstantVelocity = Atoi(&IniBuf[x+1]);
							for (x++, y = 0; (y < 4) && (IniBuf[x] >= '0') && (IniBuf[x] <= '9'); x++, y++)
								constantVelocity[y] = IniBuf[x];
						}
					}
					else if ((IniBuf[x-5] == 'N') && (IniBuf[x-9] == 'N') && (IniBuf[x-13] == 'O')) { // OnlyNoteNames=
						if (IniBuf[x+1] == 'Y')
							onlynotenames = TRUE;
					}
					else if ((IniBuf[x-5] == 'L') && (IniBuf[x-12] == 'B') && (IniBuf[x-18] == 'P')) { // PixelsBetweenLines=
						if (IniBuf[x+1] != '\r') {
							PixelsBetweenLines = Atoi(&IniBuf[x+1]);
							for (x++, y = 0; (y < 4) && (IniBuf[x] >= '0') && (IniBuf[x] <= '9'); x++, y++)
								lineSpace[y] = IniBuf[x];
						}
					}
					else if ((IniBuf[x-5] == 'S') && (IniBuf[x-6] == 'A') && (IniBuf[x-10] == 'S')) { // ShowASpace=
						if (IniBuf[x+1] == 'Y')
							showaspace = TRUE;
					}

					else if ((IniBuf[x-1] == 'x') && (IniBuf[x-2] == 'E') && (IniBuf[x-5] == 'S')) {// SysEx=F0 00 20 63 00 0B 00 03 F7
						sysex = TRUE;
						for (x++, y = 0, SysExLen = 0; (x < IniBufSize) && (IniBuf[x] != '\r'); x++, y++) {
							cSysEx[y] = IniBuf[x];
							if ((y % 3) == 0) {
								if ((IniBuf[x+2] == ' ') || (IniBuf[x+2] == '\r') || ((x+2) == IniBufSize)) {
									hex1 = IniBuf[x];
									if ((hex1 >= 'a') && (hex1 <= 'f'))
										hex1 -= 0x20;
									if ((hex1 >= 'A') && (hex1 <= 'F'))
										hex1 -= 7;
									hex1 -= '0';
									hex2 = IniBuf[x+1];
									if ((hex2 >= 'a') && (hex2 <= 'f'))
										hex2 -= 0x20;
									if ((hex2 >= 'A') && (hex2 <= 'F'))
										hex2 -= 7;
									hex2 -= '0';
									if ((hex1 <= 0x0F) && (hex2 <= 0x0F))
										SysEx[SysExLen++] = (hex1 << 4) | hex2;
									else {
										sysex = FALSE;
										MessageBox(hwnd, cSysEx, "A bad hexadecimal number follows this:", MB_OK);
										break;
									}
								}
								else {
									sysex = FALSE;
									MessageBox(hwnd, "doesn't have a space between each hexadecimal number.", "The SysEx line in PianoRollComposer.ini", MB_OK);
									break;
								}
							}
						}
						cSysEx[y] = 0;
					}
					else if ((IniBuf[x-5] == 'P') && (IniBuf[x-4] == 'o') && (IniBuf[x-3] == 'r') && (IniBuf[x-2] == 't') && (IniBuf[x-1] == 's')) {
						EWQL = TRUE;
/*
						NumberOfPorts = IniBuf[x+1] - '0';
						if ((IniBuf[x+2] >= '0') && (IniBuf[x+2] <= '9'))
							NumberOfPorts += ((IniBuf[x+2] - '0') * 10);
						if ((IniBuf[x+3] >= '0') && (IniBuf[x+3] <= '9'))
							NumberOfPorts += ((IniBuf[x+3] - '0') * 100);
*/
					}
					else if ((IniBuf[x-4] == 'O') && (IniBuf[x-7] == 'B'))// BigOpen= (just for me)
						bigopen = TRUE;
				}
			}
			CloseHandle(hFile);
			hFile = NULL;
		}

		if (EWQL) {
			hFile2 = FindFirstFile("Port*.txt", &fd);
			if (hFile2 != INVALID_HANDLE_VALUE) {
				NumberOfPorts++;
				while (FindNextFile(hFile2, &fd)) {
					NumberOfPorts++;
				};
				FindClose(hFile2);
				for (x = 0; x < NumberOfPorts; x++) {
					y = 4;
					if (x < 9)
						Ports[y++] = x +'1';
					else {
						Ports[y++] = '1';
						Ports[y++] = ((x+1) % 10) + '0';
					}
					Ports[y++] = '.';
					Ports[y++] = 't';
					Ports[y++] = 'x';
					Ports[y++] = 't';
					Ports[y++] = 0;
					hFile = CreateFile(Ports, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
					if (hFile != INVALID_HANDLE_VALUE) {
						if (EWQLfileSize[x] = GetFileSize(hFile, NULL))
							ReadFile(hFile, &InstrumentBufs[x], EWQLfileSize[x], &dwBytesRead, NULL);
						CloseHandle(hFile);
					}
				}
				myInstruments[0][0] = '0';
				myInstruments[0][1] = '0';
				myInstruments[0][2] = '0';
				myInstruments[0][3] = ' ';
				myInstruments[0][4] = ' ';
				for (y = 0, z = 5; (y < (int)EWQLfileSize) && (InstrumentBufs[0][y] != 0) && (InstrumentBufs[0][y] != '(') && (InstrumentBufs[0][y] != '\r'); y++, z++)
					myInstruments[0][z] = InstrumentBufs[0][y];
				myInstruments[0][z] = 0;
				for (x = 0; x < 16; x++)
					ActiveChannels[x] = TRUE;
			}
			else
				EWQL = FALSE;
		}
		if (!EWQL) { // because of else EWQL = FALSE, above
			hFile = CreateFile("Instruments.txt", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				if (fileSize = GetFileSize(hFile, NULL)) {
					ReadFile(hFile, &InstrumentBuf, fileSize, &dwBytesRead, NULL);
					w = fileSize / 32; // number of instruments
					for (x = 0, y = 0; y < (int)fileSize; x++) {
						myInstruments[x][0] = ((x+1) / 100) + '0';
						myInstruments[x][1] = (((x+1) % 100) / 10) + '0';
						myInstruments[x][2] = ((x+1) % 10) + '0';
						myInstruments[x][3] = ' ';
						myInstruments[x][4] = ' ';
						for (z = 5; (y < (int)fileSize) && (InstrumentBuf[y] != '(') && (InstrumentBuf[y] != '\r'); y++, z++)
							myInstruments[x][z] = InstrumentBuf[y];
						myInstruments[x][z] = 0;
						if (InstrumentBuf[y] == '(') {
							y++; // to instrument ranges
							for (z = 0; InstrumentBuf[y] != ')'; y++, z++) {
								InstrumentRanges[x][z] = InstrumentBuf[y];
							}
							y++; // to '\r'
							InstrumentRanges[x][z] = 0;
						}
						y += 2; // to next line
					}
					myInstruments[x][0] = ((x+1) / 100) + '0';
					myInstruments[x][1] = (((x+1) % 100) / 10) + '0';
					myInstruments[x][2] = ((x+1) % 10) + '0';
					myInstruments[x][3] = ' ';
					myInstruments[x][4] = ' ';
					myInstruments[x][5] = 'n';
					myInstruments[x][6] = 'o';
					myInstruments[x][7] = 'n';
					myInstruments[x][8] = 'e';
					myInstruments[x][9] = '*';
					myInstruments[x][10] = 0;
				}
				CloseHandle(hFile);
			}
			else {
				for (x = 0; x <= 128; x++) {
					myInstruments[x][0] = ((x+1) / 100) + '0';
					myInstruments[x][1] = (((x+1) % 100) / 10) + '0';
					myInstruments[x][2] = ((x+1) % 10) + '0';
					myInstruments[x][3] = ' ';
					myInstruments[x][4] = ' ';
					ptrInstrument = *(DWORD*)&Instruments[x];
					strcpy(&myInstruments[x][5], (char*)ptrInstrument);
				}
			}
		}
		for (x = 0; x < 16; x++) {
			for (y = 0; y < 16; y++)
				InstrumentOffset[x][y] = 0;
		}

		InitializeCriticalSection(&csTickptr);

		hMenu = GetMenu(hwnd);
		Menus();
		DrawMenuBar(hwnd);

		Midi[0] = 0;//flag to do certain tasks only with music created in this program

		TempEvent.dMilliSecondsPerTick = 0;// never changes, but must be 0

		midiOutSetVolume(0, 0xFFFFFFFF);// sets SW Synth Volume in both stereo channels
		if (EWQL) {
			for (x = 0; x < NumberOfPorts; x++) {
				if (MMSYSERR_NOERROR == midiOutOpen(&hMidisOut[x], x+1, 0, 0, 0)) {// MIDIMAPPER = -1 (pointer to driver for default Windows MIDI sounds)
					midiOutShortMsg(hMidisOut[x], 0xC0); // Program Change: channel 0 and instrument 0 (piano)
					midiOutShortMsg(hMidisOut[x], 0xB0 | (121 << 8));// all controllers off
					midiOutShortMsg(hMidisOut[x], 0xB0 | (0x65 << 8));// coarse RPN (0 is Pitch Bend)
					midiOutShortMsg(hMidisOut[x], 0xB0 | (0x64 << 8));// fine RPN (0 is Pitch Bend)
					midiOutShortMsg(hMidisOut[x], 0xB0 | (0x06 << 8) | (0x02 << 16)); // coarse Data Entry (2 is +/- 2 semitones) DEFAULT
					midiOutShortMsg(hMidisOut[x], 0xB0 | (0x26 << 8));// fine Data Entry (0 is 0 cents)
				}
			}
		}
		else if (MMSYSERR_NOERROR == midiOutOpen(&hMidiOut, DeviceOut, 0, 0, 0)) {// MIDIMAPPER = -1 (pointer to driver for default Windows MIDI sounds)
			midiOutShortMsg(hMidiOut, 0xC0); // Program Change: channel 0 and instrument 0 (piano)
//			midiOutShortMsg(hMidiOut, 0xC9 | (40 << 8));// trick! (changes some drums in Windows!)// 0-39 40-48 49-55 56-126 give different Hand Claps, etc
			midiOutShortMsg(hMidiOut, 0xB0 | (121 << 8));// all controllers off
			midiOutShortMsg(hMidiOut, 0xB0 | (0x65 << 8));// coarse RPN (0 is Pitch Bend)
			midiOutShortMsg(hMidiOut, 0xB0 | (0x64 << 8));// fine RPN (0 is Pitch Bend)
			midiOutShortMsg(hMidiOut, 0xB0 | (0x06 << 8) | (0x02 << 16)); // coarse Data Entry (2 is +/- 2 semitones) DEFAULT
			midiOutShortMsg(hMidiOut, 0xB0 | (0x26 << 8));// fine Data Entry (0 is 0 cents)
		}
		else
			MessageBox(hwnd, "Select a different Midi Output device.", ERROR, MB_OK);
		if (MMSYSERR_NOERROR == midiInOpen((LPHMIDIIN)&hMidiIn, DeviceIn, (DWORD)MidiInProc, 0, CALLBACK_FUNCTION | MIDI_IO_STATUS)) {
			midi_in = TRUE;
			midiInStart(hMidiIn);
		}

		ActiveChannels[0] = TRUE;
		StereoLocations[0] = 0x40;
		if (EWQL) {
			for (x = 0; x < 16; x++) {
				for (y = 0; y < 16; y++) {
					if (y < NumberOfPorts) {
						ChannelInstruments[x][y] = x; // to show EWQL instruments
						midiOutShortMsg(hMidisOut[y], (0xB0|x) | (121 << 8)); // all controllers off
						midiOutShortMsg(hMidisOut[y], (0xB0|x) | (7 << 8) | (127 << 16)); // full Volume (not 100)
					}
					else
						ChannelInstruments[x][y] = 0xFF; // flag
				}
				StereoLocations[x] = 0x40;
			}
		}
		else {
			ChannelInstruments[0][0] = 0;
			for (y = 1; y < 16; y++) {
				ChannelInstruments[0][y] = 0xFF;// flag (biggest MIDI instrument number is 127)
			}
			for (x = 1; x < 16; x++) {
				ActiveChannels[x] = FALSE;
				for (y = 0; y < 16; y++) {
					ChannelInstruments[x][y] = 0xFF;// flag (biggest MIDI instrument number is 127)
				}
				StereoLocations[x] = 0x40;
				midiOutShortMsg(hMidiOut, (0xB0|x) | (121 << 8)); // all controllers off
				midiOutShortMsg(hMidiOut, (0xB0|x) | (7 << 8) | (127 << 16)); // full Volume (not 100)
				midiOutShortMsg(hMidiOut, 0xC0|x); // Program Change: instrument 0 (piano)
			}
			ChannelInstruments[9][0] = 30 + (DrumSet*10);
		}
//		midiOutShortMsg(hMidiOut, 0xC9 | ((30 + (DrumSet*10)) << 8));
		TempoChange = 0.0;
		for (x = 0; x < 16; x++)
			origVol[x] = 127;
		for (y = 0; y < 47; y++)
			for (x = 0; x < 120; x++)
				LoopNotes[y][x] = 0;
		for (x = 0; x < 64; x++)
			TotalNotesOn[x] = 0;
		Frame = GetSystemMetrics(SM_CXSIZEFRAME);
		Menu = GetSystemMetrics(SM_CYMENU);
		TitleAndMenu = GetSystemMetrics(SM_CYCAPTION) + Menu;
		for (x = 0; x < 300; x++) {
			Lines[x].pixel = 0;
			Lines[x].PixelsPerPage = 0;
			Lines[x].rowonpage = 0;
			Lines[x].FirstMeasureNumber = 0;
			for (y = 0; y < 64; y++) {
				Lines[x].BeatsPerMeasure[y] = 4;
				Lines[x].PixelsPerBeat[y] = 40;
				Lines[x].PixelsPerMeasure[y] = 160;
				Lines[x].KeySignature[y] = 200;
			}
		}
		for (y = 0; y < MAX_PLAYLIST; y++) {
			for (x = 0; x < MAX_PATH; x++)
				PlayList[y][x] = 0;
		}
		for (x = 0; x < 8; x++) {
			AssignedInstruments[x].name[0] = 0;
			AssignedInstruments[x].channel = 0;
			AssignedInstruments[x].port = 0;
			AssignedInstruments[x].start = 0;
			AssignedInstruments[x].end = 0;
			AssignedInstruments[x].octaveshift = 0;
			AssignedInstruments[x].textstart = 0;
			AssignedInstruments[x].textlength = 0;
			AssignedInstruments[x].firstnote = 0;
			AssignedInstruments[x].lastnote = 0;
		}
		ai = 0;
		for (x = 21; x <= 108; x++)
			Sustain[x] = 0;
		return 0;

	case WM_SIZE:
		if (first == FALSE) {
			rect.left = rect.top = 0;
			rect.right = LOWORD(lParam);
			rect.bottom = HIWORD(lParam);
			////////////////////////
			GetPixelsBetweenLines();
			////////////////////////
			PageBegin = 0;
			FirstPixelOnPage = 0;
			FirstTopLeftPtr = 0;
			TopLeftPtr = 0;
			Page = 0;
			_itoa(Page+1, &page[5], 10);
			line = 0;
			for (y = 0; y < 98; y++) {
				for (x = 0; x < 256; x++)
					NoteCarryOver[y][x] = 0;
			}
			PedalDownX = -1;
			FillRect(hdcMem, &rect, hBrush);
		}
		return 0;

	case WM_COMMAND:
		if (wParam == StopID) { // STOP on Menu
			playflag = STOPPED;
			if (showingstop) {
				showingstop = FALSE;
				RemoveMenu(hMenu, 9, MF_BYPOSITION);
			}
			ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Play);
			DrawMenuBar(hwnd);
			savetimePtr = 0;
			keyboardactive = FALSE;
			done = TRUE;
			SendMessage(hwnd, WM_USER3, 0, 0);// stop playing
		}
		else if ((LOWORD(wParam) >= IDM_INPUT) && (LOWORD(wParam) < (IDM_INPUT+LastInputDevice))) {
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			CheckMenuItem(hMenu, IDM_INPUT + DeviceIn, MF_UNCHECKED);
			DeviceIn = LOWORD(wParam) - IDM_INPUT;
			CheckMenuItem(hMenu, IDM_INPUT + DeviceIn, MF_CHECKED);
			midiInStop(hMidiIn);
			midiInReset(hMidiIn);
			midiInClose(hMidiIn);
			if (MMSYSERR_NOERROR == midiInOpen((LPHMIDIIN)&hMidiIn, DeviceIn, (DWORD)MidiInProc, 0, CALLBACK_FUNCTION | MIDI_IO_STATUS))
				midiInStart(hMidiIn);
			break;
		}
		else if ((LOWORD(wParam) >= IDM_OUTPUT) && (LOWORD(wParam) <= (IDM_OUTPUT+LastOutputDevice))) {
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			CheckMenuItem(hMenu, IDM_OUTPUT + DeviceOut, MF_UNCHECKED);
			DeviceOut = LOWORD(wParam) - IDM_OUTPUT;
			CheckMenuItem(hMenu, IDM_OUTPUT + DeviceOut, MF_CHECKED);
			midiOutReset(hMidiOut);
			midiOutClose(hMidiOut);
			if (DeviceOut != (DWORD)LastOutputDevice) { // not PianoRollComposer Synthesizer
				if (MMSYSERR_NOERROR == midiOutOpen(&hMidiOut, DeviceOut, 0, 0, 0)) {
					midiOutShortMsg(hMidiOut, 0xC0);//channel 0 and instrument 0 (piano)
					midiOutShortMsg(hMidiOut, 0xB0 | (121 << 8));// all controllers off
					midiOutShortMsg(hMidiOut, 0xB0 | (0x65 << 8));// coarse RPN (0 is Pitch Bend)
					midiOutShortMsg(hMidiOut, 0xB0 | (0x64 << 8));// fine RPN (0 is Pitch Bend)
					midiOutShortMsg(hMidiOut, 0xB0 | (0x06 << 8) | (0x02 << 16)); // coarse Data Entry (2 is +/- 2 semitones) DEFAULT
					midiOutShortMsg(hMidiOut, 0xB0 | (0x26 << 8));// fine Data Entry (0 is 0 cents)
					if ((ActiveChannel != 9) || (EWQL))
						midiOutShortMsg(hMidiOut, (0xC0|ActiveChannel) | (ActiveInstrument << 8));
					if (WaveBuf)
						VirtualFree(WaveBuf, 0, MEM_RELEASE);
					WaveBuf = NULL;
				}
			}
			break;
		}

		switch (LOWORD(wParam))
		{
		case ID_FILE_MULTIOPEN:
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			if ((!keyboardactive) && (!sustainflag)) {

				hwndPlayList = CreateDialog(hInst, "PLAYLIST", hwnd, PlayListProc);

				for (x = 0; x < MAX_PATH; x++)
					FullFilename[x] = 0;
				for (y = 0; y < MAX_PLAYLIST; y++) {
					for (x = 0; x < MAX_PATH; x++)
						PlayList[y][x] = 0;
				}
				ofn.lpstrTitle = "Add to Play List (36 max)";
				ofn.Flags = OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_ENABLESIZING|OFN_EXPLORER|OFN_ENABLEHOOK;
				fromplaylist = TRUE; // for OFNHookProc
				for (playlist = 0; (playlist < MAX_PLAYLIST) && (GetOpenFileName(&ofn)); playlist++) {
					for (x = 0; FullFilename[x] != 0; x++)
						PlayList[playlist][x] = FullFilename[x];
					for (x = 0; x < MAX_PATH; x++)
						FullFilename[x] = 0;
					SendMessage(hwndPlayList, 0xABCD, 0, 0);
				}
				ofn.lpstrTitle = NULL;
//				SendMessage(hwndPlayList, WM_CLOSE, 0, 0);
			}
			break;
			
		case ID_FILES_OPEN:
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			if ((!keyboardactive) && (!sustainflag)) {
				if (bigopen)
					ofn.Flags = OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_ENABLESIZING|OFN_EXPLORER|OFN_ENABLEHOOK;
//				for (x = 0; x < MAX_PATH; x++)
//					FullFilename[x] = 0;
				if (GetOpenFileName(&ofn)) {
					hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
					if (hFile != INVALID_HANDLE_VALUE) {
						if (fileSize = GetFileSize(hFile, NULL)) {
							if (fileSize > MAX_MIDI) {
								MessageBox(hwnd, "That file is too big.", "", MB_OK);
								CloseHandle(hFile);
							}
							else {
								for (y = 0; y < 98; y++) {
									for (x = 0; x < 256; x++)
										NoteCarryOver[y][x] = 0;
								}
								SendMessage(hwnd, WM_COMMAND, WM_USER4, 0);//most of ID_NEW
								ReadFile(hFile, Midi, fileSize, &dwBytesRead, NULL);
								CloseHandle(hFile);
								if (*(DWORD*)&Midi[0] == 0x46464952) {//"RIFF"
									if (*(DWORD*)&Midi[8] == 0x45564157) {// "WAVE"
										MessageBox(hwnd, "That's a WAVE file", "Oops", MB_OK);
										break;
									}
									for (x = 0, y = 20; y < (int)fileSize; x++, y++)
										Midi[x] = Midi[y];
									fileSize -= 20;
								}
								if (*(DWORD*)&Midi[0] == 0x6468544D) {//"MThd" - MIDI files are in Big Endian format (not Intel format)
									BOOL noinstruments = TRUE;
									for (y = 0; y < MAX_PLAYLIST; y++) {
										for (x = 0; x < MAX_PATH; x++)
											PlayList[y][x] = 0;
									}
									TitleBar();
									for (y = 0; y < 16; y++) {
										for (x = 0; x < 16; x++)
											ChannelInstruments[y][x] = 0xFF;
									}
									if (!EWQL) {
										for (x = 0; x < 16; x++) {
											midiOutShortMsg(hMidiOut, (0xB0|x) | (121 << 8));// all controllers off
											midiOutShortMsg(hMidiOut, (0xB0|x) | (7 << 8) | (127 << 16));// full Volume (not default of 100)
											origVol[x] = 127;
										}
									}
									else {
										for (y = 0; y < NumberOfPorts; y++) {
											for (x = 0; x < 16; x++) {
												midiOutShortMsg(hMidisOut[y], (0xB0|x) | (121 << 8));// all controllers off
												midiOutShortMsg(hMidisOut[y], (0xB0|x) | (7 << 8) | (127 << 16));// full Volume (not default of 100)
												origVol[x] = 127;
											}
										}
									}
									TempoChange = 0.0;
									BigText[bt] = 0;
									//////////
									ReadMidi();
									//////////
									if (!EWQL) {
										for (x = 0; x < 16; x++) {
											if (ChannelInstruments[x][0] != 0xFF) { // not empty channel
												midiOutShortMsg(hMidiOut, 0xC0 | x | (ChannelInstruments[x][0] << 8)); // to play channels' instruments
												ActiveChannels[x] = TRUE;
												noinstruments = FALSE;
											}
										}
									}
									else { // if (EWQL)
										for (x = 0; x < 16; x++) {
											ActiveChannels[x] = TRUE;
											for (y = 0; y < NumberOfPorts; y++)
												ChannelInstruments[x][y] = x;
										}
									}
									if (noinstruments) {
										ChannelInstruments[0][0] = 0;// need at least a piano
										ActiveChannels[0] = TRUE;
									}
									ActiveChannel = 0;
									ActiveInstrument = 0;
									originalE = e;
									for (x = 0; x < (int)e; x++)
										EventPixels[x] = Event[x].pixel;// for SAVE at CLOSE
									SaveEvents();
									FillRect(hdcMem, &rect, hBrush);
									InvalidateRect(hwnd, &rect, FALSE);
								}
								else {
									MessageBox(hwnd, "That's not a MIDI file.", ERROR, MB_OK);
								}
							}
						}// end of if (fileSize = GetFileSize(hFile, NULL))
						else
							CloseHandle(hFile);
					}
				}
			}
			break;

		case ID_FILES_SAVE:
			if (Midi[0] != 0)
				MessageBox(hwnd, "Please respect the rights of the\noriginal creator of this MIDI file.", "", MB_OK);
			if (e > 4) {
				if (!playing) {
					if (keyboardactive) {
						keyboardactive = FALSE;
						ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, RecordMIDIKeyboard);
						DrawMenuBar(hwnd);
						DeleteDuplicateNotes();
					}
					fromsave = TRUE;
					StopTimers();
savit:			WriteMidi();
					hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
					if (hFile != INVALID_HANDLE_VALUE) {
						Midi[0] = 'M';
						WriteFile(hFile, Midi, i, &dwBytesWritten, NULL);// i from WriteMidi
						Midi[0] = 0;
						CloseHandle(hFile);
						TitleBar();

						originalE = e;
						for (x = 0; x < (int)e; x++)
							EventPixels[x] = Event[x].pixel;
					}
					else if (GetSaveFileName(&ofn)) {
						int x = ofn.nFileExtension;
						ofn.lpstrFile[x] = 'm';
						ofn.lpstrFile[x+1] = 'i';
						ofn.lpstrFile[x+2] = 'd';
						ofn.lpstrFile[x+3] = 0;
						goto savit;
					}
				}
			}
			else
				MessageBox(hwnd, "Save Nothing?!?", "", MB_OK);
			return 0;

		case ID_FILES_SAVE_AS:
			if (Midi[0] != 0)
				MessageBox(hwnd, "Please respect the rights of the\noriginal creator of this MIDI file.", "", MB_OK);
			if (e > 4) {
				if (!playing) {
					if (keyboardactive) {
						keyboardactive = FALSE;
						ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, RecordMIDIKeyboard);
						DrawMenuBar(hwnd);
						DeleteDuplicateNotes();
					}
					fromsave = TRUE;
					StopTimers();
					if (GetSaveFileName(&ofn)) {
						int x = ofn.nFileExtension;
						ofn.lpstrFile[x] = 'm';
						ofn.lpstrFile[x+1] = 'i';
						ofn.lpstrFile[x+2] = 'd';
						ofn.lpstrFile[x+3] = 0;
						WriteMidi();
						hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
						if (hFile != INVALID_HANDLE_VALUE) {
							Midi[0] = 'M';
							WriteFile(hFile, Midi, i, &dwBytesWritten, NULL);// i from WriteMidi
							Midi[0] = 0;
							CloseHandle(hFile);
							TitleBar();

							originalE = e;
							for (x = 0; x < (int)e; x++)
								EventPixels[x] = Event[x].pixel;
						}
					}
				}
			}
			else
				MessageBox(hwnd, "Save Nothing?!?", "", MB_OK);
			return 0;

		case ID_FILE_EXPORTHIGHLIGHTED:
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			if (copySize) {
				hFile = CreateFile("Highlighted.dta", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					WriteFile(hFile, &copyBegin, 4, &dwBytesWritten, NULL);
					WriteFile(hFile, &totalActiveChannels, 4, &dwBytesWritten, NULL);
					WriteFile(hFile, &activeChannels, 16, &dwBytesWritten, NULL);
					for (x = 0, y = sizeof(copyEvent[0]); x < (int)copySize; x++)
						WriteFile(hFile, &copyEvent[x], y, &dwBytesWritten, NULL);
					CloseHandle(hFile);
					MessageBox(hwnd, "Exported to\n\"Highlighted.dta\"", "Highlighted data", MB_OK);
				}
			}
			else
				MessageBox(hwnd, "There's no Highlighted data\nthat was Cut or Copied.", ERROR, MB_OK);
			break;
			  
		case ID_FILE_IMPORTHIGHLIGHTED:
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			hFile = CreateFile("Highlighted.dta", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				if (fileSize = GetFileSize(hFile, NULL)) {
					if (0 != (fileSize-24) % sizeof(copyEvent[0])) {
						MessageBox(hwnd, "That's not a compatible Highlighted file", ERROR, MB_OK);
						break;
					}
					ReadFile(hFile, &copyBegin, 4, &dwBytesRead, NULL);
					ReadFile(hFile, &totalActiveChannels, 4, &dwBytesRead, NULL);
					ReadFile(hFile, &activeChannels, 16, &dwBytesRead, NULL);
					y = sizeof(copyEvent[0]);
					copySize = (fileSize-24) / y;
					for (x = 0; x < (int)copySize; x++)
						ReadFile(hFile, &copyEvent[x], y, &dwBytesRead, NULL);
					NewChannel = 0xFF; // flag
				}
				CloseHandle(hFile);
				MessageBox(hwnd, "Imported from\nHighlighted.dta\n\nPress Ctrl-V to Paste it\nat the cursor location.", "Highlighted data", MB_OK);
			}
			break;

//		case ID_FILES_LOADINSTRUMENTS:
//			if (GetOpenFileName(&ofn3)) {
//				if ((0 == stricmp(ofn3.lpstrFileTitle, "Instruments.txt")) || (CheckForPortFile())) {
//					x=x;
//				}
//				else 
//					MessageBox(hwnd, "is for Kontakt or EastWest/Quantum Leap users\nto load a file of instrument names that match\nsampled instrument names in those programs.", "Load Instruments", MB_OK);
//			}
//			break;

		case ID_FILES_EXIT:
			SendMessage(hwnd, WM_CLOSE, 0, 0);
			break;

		case ID_UNDO:
			if ((pUndo == 0) && (Loop != 0))
				pUndo = 10000;
			if ((pUndo) && (LastEventInUndo[pUndo-1] != -1)) {
				pUndo--;
				e = LastEventInUndo[pUndo];
				for (x = 0; x < (int)e; x++)
					Event[x] = UndoEvent[pUndo][x];
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case ID_EDIT_UNDOUNDOCTRLY:
			if ((pUndo < 9999) && (LastEventInUndo[pUndo+1] != -1)) {
				pUndo++;
				for (x = 0; x < (int)LastEventInUndo[pUndo]; x++)
					Event[x] = UndoEvent[pUndo][x];
				e = LastEventInUndo[pUndo];
				FillRect(hdcMem, &rect, hBrush);
//				TextOut(hdcMem, rect.right >> 1, 0, _itoa(pUndo, temp, 10), lstrlen(temp));
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case ID_EDIT_CHANGEINSTRUMENT:
			if (DialogBox(hInst, "CHANGEINSTRUMENT", hwnd, ChangeInstrumentProc)) {
				OldChannel = (BYTE)(FromChannel - 1);
				ReplacementChannel = (BYTE)(ToChannel - 1);
				if (ActiveChannels[OldChannel] && ActiveChannels[ReplacementChannel])
				{
					if (EWQL) {
						OldPort = (BYTE)(FromPort - 1);
						ReplacementPort = (BYTE)(ToPort - 1);
					}
					if ((OldChannel < 16) && (ReplacementChannel < 16)) {
						for (x = 0; x < (int)e; x++) {
							if ((Event[x].message) && (Event[x].channel == OldChannel) && (Event[x].port == OldPort)) {
								Event[x].message &= 0xFFFFFFF0;
								Event[x].message |= ReplacementChannel;
								Event[x].channel = ReplacementChannel;
								Event[x].port = ReplacementPort;
							}
						}
						FillRect(hdcMem, &rect, hBrush);
						InvalidateRect(hwnd, &rect, FALSE);
					}
					else
						MessageBox(hwnd, "Bad channel number", "", MB_OK);
				}
				else
					MessageBox(hwnd, "Both channels have to be active", "", MB_OK);
			}
			break;

		case ID_EDIT_CHANGEVELOCITIES:
			hwndVelocities = CreateDialog(hInst, "VELOCITIES", hwnd, ChangeVelocitiesProc);
			break;

		case ID_MIDILIST:
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			if ((!keyboardactive) && (!showlist))
				hwndList = CreateDialog(hInst, "LIST", hwnd, ListProc);
			break;

		case ID_HIGHLIGHT:
			if ((!playing) && (!keyboardactive) && (!sustainflag)) {
				if ((copying == NOCOPY) && (!highlighting)) {
					for (x = 0; x < (int)e; x++)
						if (Event[x].velocity)
							break;
					if (x == (int)e) {
						ctrlA = FALSE;
						MessageBox(hwnd, "There are no notes to highlight.", Highlight, MB_OK);
					}
					else {
						highlighting = TRUE;
						copying = COPY;
						ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlighting);
						DrawMenuBar(hwnd);
					}
				}
				else {
					ctrlA = FALSE;
					highlighting = FALSE;
					copying = NOCOPY;
					ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
					DrawMenuBar(hwnd);
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			else
				ctrlA = FALSE;
			break;

		case ID_NEW:
			if ((!playing) && (!keyboardactive) && (!sustainflag)) {
				if (IDOK == MessageBox(hwnd, New, "New", MB_OKCANCEL|MB_DEFBUTTON2)) {
					for (y = 0; y < 98; y++) {
						for (x = 0; x < 256; x++)
							NoteCarryOver[y][x] = 0;
					}
					SetWindowText(hwnd, szAppName);
					Filename[0] = 0;
					FullFilename[0] = 0;
					FullFilename2[0] = 0;
					//fall thru...
		case WM_USER4:// from ID_FILES_OPEN
					for (x = 0; x < 300; x++) {
						Lines[x].pixel = 0;
						Lines[x].PixelsPerPage = 0;
						Lines[x].rowonpage = 0;
						Lines[x].FirstMeasureNumber = 0;
						for (y = 0; y < 64; y++) {
							Lines[x].BeatsPerMeasure[y] = 4;
							Lines[x].PixelsPerBeat[y] = 40;
							Lines[x].PixelsPerMeasure[y] = 160;
							Lines[x].KeySignature[y] = 200;
						}
					}
					FirstTopLeftPtr = 0;
					StopTimers();
					if (keyboardactive) {
						keyboardactive = FALSE;
						ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, RecordMIDIKeyboard);
						DrawMenuBar(hwnd);
						DeleteDuplicateNotes();
					}
					ActiveChannels[0] = TRUE;
					ChannelInstruments[0][0] = 0;
					for (y = 1; y < 16; y++) {
						ChannelInstruments[0][y] = 0xFF;// flag (biggest MIDI instrument number is 127)
					}
					StereoLocations[0] = 0x40;
					if (!EWQL) {
						for (x = 1; x < 16; x++) {
							ActiveChannels[x] = FALSE;
							for (y = 0; y < 16; y++)
								ChannelInstruments[x][y] = 0xFF;// flag (biggest MIDI instrument number is 127)
							midiOutShortMsg(hMidiOut,0x40B0|x); // sustain off
							midiOutShortMsg(hMidiOut, (0xB0|x) | (121 << 8));// all controllers off
							midiOutShortMsg(hMidiOut, (0xB0|x) | (7 << 8) | (127 << 16));// full Volume (not default of 100)
							StereoLocations[x] = 0x40;
							origVol[x] = 127;
						}
					}
					else {
						for (x = 0; x < 16; x++) {
							for (y = 0; y < NumberOfPorts; y++) {
								midiOutShortMsg(hMidisOut[y], 0x40B|x); // sustain off
								midiOutShortMsg(hMidisOut[y], 0xB|x | (121 << 8));// all controllers off
								midiOutShortMsg(hMidisOut[y], (0xB0|x) | (7 << 8) | (127 << 16));// full Volume (not default of 100)
							}
							StereoLocations[x] = 0x40;
							origVol[x] = 127;
						}
					}
					TempoChange = 0.0;
					DrumSet = ChosenDrumSet;
					if (!EWQL)
						ChannelInstruments[9][0] = 30 + (DrumSet*10);
					ActiveChannel = 0;
					ActiveInstrument = 0;
//					midiOutShortMsg(hMidiOut, 0xC9 | ((30 + (DrumSet*10)) << 8));
					midiOutShortMsg(hMidiOut, 0xC0);//channel 0 and piano
					Time = 0;
					Midi[0] = 0;
					TicksPerPixel = 12;
					TicksPerQuarterNote = 480;
					BeatsPerMeasure = 4;
					BeatNoteType = 4;
					KeySignature = 200;// C
					Port = 0;
					ZeroEvents();
					originalE = 0;
					bt = 0;
					BigText[0] = 0;

					PageBegin = 0;
					FirstPixelOnPage = 0;
					FirstTopLeftPtr = 0;
					TopLeftPtr = 0;
					Page = 0;
					_itoa(Page+1, &page[5], 10);
					line = 0;

					for (x = 0; x < 300; ) {
						if (Lines[x].rowonpage == 0) {
							PixelsPerPage = 0;
							PixelsPerPage += Lines[x].PixelsPerLine;
							for (z = x+1; Lines[z].rowonpage != 0; z++)
								PixelsPerPage += Lines[z].PixelsPerLine;
							Lines[x].PixelsPerPage = PixelsPerPage;
							for (x++; Lines[x].rowonpage != 0; x++)
								Lines[x].PixelsPerPage = PixelsPerPage;
						}
					}

					Event[0].BeatsPerMeasure = 4;
					Event[0].BeatNoteType = 4;
					Event[1].KeySignature = 200;// C
					InitialBeatsPerMinute = 120; // NEW 1/30/2012
					dMilliSecondsPerTick = 60000.0 / (double)(InitialBeatsPerMinute * TicksPerQuarterNote);
					Event[2].dMilliSecondsPerTick = dMilliSecondsPerTick;
//					if (!EWQL) {
						Event[3].message = 0xC0;
						Event[3].channel = 0;
//					}
//					else {
//						Event[3].message = 0x05B0; // portamento rate of 0 (slowest)
//						Event[3].channel = 0;
//					}
					e = 4;
					////////////////////////
					GetPixelsBetweenLines(); // calls FillLines
					////////////////////////
					for (u = 0; u < 10000; u++) {
						if (LastEventInUndo[u] != -1)
							free(UndoEvent[u]);
						LastEventInUndo[u] = -1;// flag
					}
					pUndo = 0;
					Loop = -1;

					if (showinginstruments)
						DestroyWindow(hwndInstruments);
					if (virtualkeyboard)
						DestroyWindow(hwndVirtualKeyboard);
					VirtualActiveChannel = 0xFF; // flag
					for (x = 0; x < 8; x++) {
						AssignedInstruments[x].name[0] = 0;
						AssignedInstruments[x].channel = 0;
						AssignedInstruments[x].port = 0;
						AssignedInstruments[x].start = 0;
						AssignedInstruments[x].end = 0;
						AssignedInstruments[x].octaveshift = 0;
						AssignedInstruments[x].textstart = 0;
						AssignedInstruments[x].textlength = 0;
						AssignedInstruments[x].firstnote = 0;
						AssignedInstruments[x].lastnote = 0;
					}
					ai = 0;

					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
					UpdateWindow(hwnd);
					SaveEvents();// ID_NEW
				}//end of if (IDOK == MessageBox(hwnd, New, "New", MB_OKCANCEL|MB_DEFBUTTON2))
			}//end of if ((!playing) && (!keyboardactive) && (!sustainflag))
			break;

		case ID_INSTRUMENTS:
			if (!showinginstruments)
				hwndInstruments = CreateDialog(hInst, "INSTRUMENTS", hwnd, InstrumentsProc);
			else
				SetFocus(hwndInstruments);
			break;

		case ID_VIRTUALKEYBOARD:
			if (virtualkeyboard)
				SendMessage(hwndVirtualKeyboard, WM_CLOSE, 0, 0);
GetClientRect(hwnd, &rect);
			rect3.left = rect3.top = 0;
			rect3.right = rect.right;
			rect3.bottom = TitleAndMenu+Frame+(24*16);
			virtualkeyboard = TRUE;
			hMenu5 = LoadMenu(hInst, "MENU5");
			hwndVirtualKeyboard = CreateWindow(VirtualKeyboard, NULL,// see VirtualKeyboardProc
				WS_POPUP | WS_CAPTION | WS_SYSMENU | WS_VISIBLE,
				rect3.left, rect3.top, rect3.right, rect3.bottom,
				hwnd, hMenu5, hInst, NULL);
			SetMenu(hwndVirtualKeyboard, hMenu5);
			break;

		case ID_OPTIONS:
			if (!showingoptions)
				hwndOptionsDlg = CreateDialog(hInst, "OPTIONS", hwnd, OptionsProc);
			break;

		case PLAYBACKVOLUME100:
			volchange = 100;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME100, MF_CHECKED);
			break;
		case PLAYBACKVOLUME80:
			volchange = 80;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME80, MF_CHECKED);
			break;
		case PLAYBACKVOLUME60:
			volchange = 60;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME60, MF_CHECKED);
			break;
		case PLAYBACKVOLUME40:
			volchange = 40;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME40, MF_CHECKED);
			break;
		case PLAYBACKVOLUME20:
			volchange = 20;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME20, MF_CHECKED);
			break;
		case PLAYBACKVOLUME0:
			volchange = 0;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME0, MF_CHECKED);
			break;
		case PLAYBACKVOLUME_20:
			volchange = -20;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME_20, MF_CHECKED);
			break;
		case PLAYBACKVOLUME_40:
			volchange = -40;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME_40, MF_CHECKED);
			break;
		case PLAYBACKVOLUME_60:
			volchange = -60;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME_60, MF_CHECKED);
			break;
		case PLAYBACKVOLUME_80:
			volchange = -80;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME_80, MF_CHECKED);
			break;
		case PLAYBACKVOLUME_100:
			volchange = -100;
			ChangeVolume();
			CheckMenuItem(hMenu, PLAYBACKVOLUME_100, MF_CHECKED);
			break;

//		case PLAYBACKTEMPO100:
//			TempoChange = -1.0; // -0.50;
//			newTempo = TRUE;
//			Uncheck2();
//			CheckMenuItem(hMenu, PLAYBACKTEMPO100, MF_CHECKED);
//			break;
		case PLAYBACKTEMPO80:
			TempoChange = -0.80; // -0.40;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO80, MF_CHECKED);
			break;
		case PLAYBACKTEMPO60:
			TempoChange = -0.60; // -0.30;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO60, MF_CHECKED);
			break;
		case PLAYBACKTEMPO40:
			TempoChange = - 0.40; // -0.20;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO40, MF_CHECKED);
			break;
		case PLAYBACKTEMPO20:
			TempoChange = -0.20; // -0.10;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO20, MF_CHECKED);
			break;
		case PLAYBACKTEMPO0:
			TempoChange = 0.0;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO0, MF_CHECKED);
			break;
		case PLAYBACKTEMPO_20:
			TempoChange = 0.20; // 0.10;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO_20, MF_CHECKED);
			break;
		case PLAYBACKTEMPO_40:
			TempoChange = 0.40; // 0.20;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO_40, MF_CHECKED);
			break;
		case PLAYBACKTEMPO_60:
			TempoChange = 0.60; // 0.30;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO_60, MF_CHECKED);
			break;
		case PLAYBACKTEMPO_80:
			TempoChange = 0.80; //0.40;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO_80, MF_CHECKED);
			break;
		case PLAYBACKTEMPO_100:
			TempoChange = 1.0; // 0.50;
			newTempo = TRUE;
			Uncheck2();
			CheckMenuItem(hMenu, PLAYBACKTEMPO_100, MF_CHECKED);
			break;

		case ID_TRANSPOSEUP:
			if (GetKeyState(VK_CONTROL) & 0x8000)// Ctrl is pressed
				transposectrl = TRUE;
			else
				transposectrl = FALSE;
			if (GetKeyState(VK_SHIFT) & 0x8000)// Shift is pressed
				transposeshift = TRUE;
			else
				transposeshift = FALSE;
			if (IDYES == MessageBox(hwnd, "", "Transpose All Notes Up?", MB_YESNO)) {
				if ((!playing) && (!keyboardactive) && (!highlighting) && (!sustainflag)) {
					for (x = 0; x < (int)e; x++) {//check for upper limit
						if (((ActiveChannels[Event[x].channel]) && (Event[x].note == 108))
						 || ((transposectrl) && (Event[x].note >= (108-12)))
						 || ((transposeshift) && (Event[x].note >= (108-7)))) {
							MessageBox(hwnd, NoteLimit, "", MB_OK);
							break;
						}
					}
					if (x == (int)e) {
						for (x = 0; x < (int)e; x++) {
							if ((ActiveChannels[Event[x].channel]) && (Event[x].note) && ((Event[x].channel != 9) || (EWQL))) {
								if (transposectrl)
									Event[x].note += 12;
								else if (transposeshift)
									Event[x].note += 7;
								else
									Event[x].note++;
								if (Event[x].note <= 108) {
									if (!Letter[Event[x].note - 21])
										Event[x].sharporflat = 1;
									else
										Event[x].sharporflat = 0;
								}
							}
						}
						FillRect(hdcMem, &rect, hBrush);
						InvalidateRect(hwnd, &rect, FALSE);
					}
				}
				else if (copying == COPYBUTTONUP) {
					for (x = firstX; x < lastX; x++) {//check for upper limit
						if ((ActiveChannels[Event[x].channel]) && (Event[x].note == 108)) {
							MessageBox(hwnd, NoteLimit, "", MB_OK);
							break;
						}
					}
					if (x == lastX) {
						for (x = firstX; x <= lastX; x++) {
							if ((ActiveChannels[Event[x].channel]) && (Event[x].note) && ((Event[x].channel != 9) || (EWQL))) {
								if (transposectrl)
									Event[x].note += 12;
								else if (transposeshift)
									Event[x].note += 7;
								else
									Event[x].note++;
								if (Event[x].note <= 108) {
									if (!Letter[Event[x].note - 21])
										Event[x].sharporflat = 1;
									else
										Event[x].sharporflat = 0;
								}
							}
						}
					}
					lParam = xLoc + (yLoc << 0x10);
					SendMessage(hwnd, WM_MOUSEMOVE, 0, (LPARAM)lParam);
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			break;

		case ID_TRANSPOSEDOWN:
			if (GetKeyState(VK_CONTROL) & 0x8000)// Ctrl is pressed
				transposectrl = TRUE;
			else
				transposectrl = FALSE;
			if (GetKeyState(VK_SHIFT) & 0x8000)// Shift is pressed
				transposeshift = TRUE;
			else
				transposeshift = FALSE;
			if (IDYES == MessageBox(hwnd, "", "Transpose All Notes Down?", MB_YESNO)) {
				if ((!playing) && (!keyboardactive) && (!highlighting) && (!sustainflag)) {
					for (x = 0; x < (int)e; x++) {//check for lower limit
						if (((ActiveChannels[Event[x].channel]) && (Event[x].note == 21))
						 || ((transposectrl) && (Event[x].note) && (Event[x].note <= (21+12)))
						 || ((transposeshift) && (Event[x].note) && (Event[x].note <= (21+7)))) {
							MessageBox(hwnd, NoteLimit, "", MB_OK);
							break;
						}
					}
					if (x == (int)e) {
						for (x = 0; x < (int)e; x++) {
							if ((ActiveChannels[Event[x].channel]) && (Event[x].note) && ((Event[x].channel != 9) || (EWQL))) {
								if (transposectrl)
									Event[x].note -= 12;
								else if (transposeshift)
									Event[x].note -= 7;
								else
									Event[x].note--;
								if (Event[x].note >= 21) {
									if (!Letter[Event[x].note - 21])
										Event[x].sharporflat = 1;
									else
										Event[x].sharporflat = 0;
								}
							}
						}
					}
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
				}
				else if ((!playing) && (!keyboardactive) && (copying == COPYBUTTONUP)) {
					for (x = firstX; x < lastX; x++) {//check for lower limit
						if ((ActiveChannels[Event[x].channel]) && (Event[x].note == 21)) {
							MessageBox(hwnd, NoteLimit, "", MB_OK);
							break;
						}
					}
					if (x == lastX) {
						for (x = firstX; x <= lastX; x++) {
							if ((ActiveChannels[Event[x].channel]) && (Event[x].note) && ((Event[x].channel != 9) || (EWQL))) {
								if (transposectrl)
									Event[x].note -= 12;
								else if (transposeshift)
									Event[x].note -= 7;
								else
									Event[x].note--;
								if (Event[x].note >= 21) {
									if (!Letter[Event[x].note - 21])
										Event[x].sharporflat = 1;
									else
										Event[x].sharporflat = 0;
								}
							}
						}
					}
					lParam = xLoc + (yLoc << 0x10);
					SendMessage(hwnd, WM_MOUSEMOVE, 0, (LPARAM)lParam);
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			break;

		case ID_SUSTAIN:
			if ((hwndSustain == NULL) && (!playing) && (!keyboardactive) && (!highlighting)) {
				sustainflag = 0;
				hwndSustain = CreateDialog(hInst, "SUSTAIN1", hwnd, SustainProc); // "SUSTAIN1" because "SUSTAIN" doesn't work
			}
			break;

		case ID_CHORDS:
			if ((chord == 0) && (!highlighting)) {// && (!playing) && (!keyboardactive)
				if ((ActiveChannel != 9) || (EWQL)) {
					hMenu3 = CreateMenu();//to override parent menu bar
					hwndChords = CreateWindow(Chords, Chords,// see ChordsProc
						WS_POPUPWINDOW | WS_VISIBLE | WS_CAPTION,
						(rect.right/2)-340, rect.bottom-305, 680, 350,
						hwnd, hMenu3, hInst, NULL);
				}
				else
					MessageBox(hwnd, "", "Percussion Chords?!?", MB_OK);
			}
			break;

		case ID_NOTETYPE_WHOLE:
			if ((!playing) && (!keyboardactive)) {
				CheckMenuItem(hMenu, ID_NOTETYPE_WHOLE, MF_CHECKED);
				Uncheck(ID_NOTETYPE_WHOLE);
				ChangeNoteType('W');
				NoteEvery = 160;
			}
			break;
		case ID_NOTETYPE_HALF:
			if ((!playing) && (!keyboardactive)) {
				CheckMenuItem(hMenu, ID_NOTETYPE_HALF, MF_CHECKED);
				Uncheck(ID_NOTETYPE_HALF);
				ChangeNoteType('H');
				NoteEvery = 80;
			}
			break;
		case ID_NOTETYPE_QUARTER:
			if ((!playing) && (!keyboardactive)) {
				CheckMenuItem(hMenu, ID_NOTETYPE_QUARTER, MF_CHECKED);
				Uncheck(ID_NOTETYPE_QUARTER);
				ChangeNoteType('Q');
				NoteEvery = 40;
			}
			break;
		case ID_NOTETYPE_EIGHTH:
			if ((!playing) && (!keyboardactive)) {
				CheckMenuItem(hMenu, ID_NOTETYPE_EIGHTH, MF_CHECKED);
				Uncheck(ID_NOTETYPE_EIGHTH);
				ChangeNoteType('E');
				NoteEvery = 20;
			}
			break;
		case ID_NOTETYPE_SIXTEENTH:
			if ((!playing) && (!keyboardactive)) {
				CheckMenuItem(hMenu, ID_NOTETYPE_SIXTEENTH, MF_CHECKED);
				Uncheck(ID_NOTETYPE_SIXTEENTH);
				ChangeNoteType('S');
				NoteEvery = 10;
			}
			break;
		case ID_NOTETYPE_THIRTYSECOND:
			if ((!playing) && (!keyboardactive)) {
				CheckMenuItem(hMenu, ID_NOTETYPE_THIRTYSECOND, MF_CHECKED);
				Uncheck(ID_NOTETYPE_THIRTYSECOND);
				ChangeNoteType('T');
				NoteEvery = 5;
			}
			break;
		case ID_NOTETYPE_SIXTYFOURTH:
			if ((!playing) && (!keyboardactive)) {
				CheckMenuItem(hMenu, ID_NOTETYPE_SIXTYFOURTH, MF_CHECKED);
				Uncheck(ID_NOTETYPE_SIXTYFOURTH);
				ChangeNoteType('F');
			}
			break;

		case ID_PERCUSSIONLOOP:
			if ((!playing) && (!keyboardactive) && (!highlighting)) {
				if (metronome) {
					metronome = FALSE;
					if (uTimer0ID) {
						timeKillEvent(uTimer0ID);
						uTimer0ID = 0;
						timeEndPeriod(TIMER_RESOLUTION);
						midiOutShortMsg(hMidiOut, 0xB9 | (123 << 8));// All Channel 9 Notes Off
					}
					CurrentNotes = -1;
					PreviousNotes = 0;
				}
				ActiveChannels[9] = TRUE;
				ChannelInstruments[9][0] = 30 + (DrumSet*10);
				saveActiveChannel = ActiveChannel;
				ActiveChannel = 9;
				saveE = e;
				copySize = 0;// so copyEvent isn't used with Ctrl-V after this
				for (x = 0; x < (int)e; x++)
					copyEvent[x] = Event[x];
				hMenu3 = CreateMenu();//to override parent menu bar
				hwndLoop = CreateWindow(PercussionLoop, PercussionLoop,// see LoopProc
					WS_POPUP | WS_VISIBLE,
					0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
					NULL, hMenu3, hInst, NULL);
				DestroyMenu(hMenu);
				hMenu2 = LoadMenu(hInst, "MENU2");
				SetMenu(hwndLoop, hMenu2);
			}
			break;

		case DYNAMICSEFFECTS:
			if (showingdynamics == FALSE)
				hwndDynamics = CreateDialog(hInst, "DYNAMICEFFECTS", hwnd, DynamicEffectsProc);
			break;

		case PLAY:
			if (playing) {
				if (keyboardactive) {
					keyboardactive = FALSE;
					ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, RecordMIDIKeyboard);
					DrawMenuBar(hwnd);
					if (uTimer2ID) {
						timeKillEvent(uTimer2ID);
						timeEndPeriod(TIMER_RESOLUTION);
						uTimer2ID = 0;
					}
					qsort(Event, e, sizeof(Event[0]), Compare);
					Resort();
					DeleteDuplicateNotes();
					SaveEvents();
				}
				SendMessage(hwnd, WM_USER3, 0, 0);// to stop playing
			}
			else if (playingwave) {
				waveOutReset(hWaveOut);
				if (playing2wavefiles) {
					waveOutReset(hWaveOut2);
					waveInReset(hWaveIn);
				}
			}
			else if (highlighting) {
				MessageBox(hwnd, "still HIGHLIGHTING...", "Oops", MB_OK);
				break;
			}

			else if ((e > 4) && (!keyboardactive) && (!sustainflag)) {
				for (x = 0; x < (int)e; x++)
					if (Event[x].note)
						break;
				if (x == (int)e)// no notes
					break;
				if (uTimer4ID) {
					keyboardactive = FALSE;
					ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, RecordMIDIKeyboard);
					DeleteDuplicateNotes();
				}

				WriteMidi();
				ReadMidi();

				playing = TRUE;
				playflag = PAUSED;
				othernotesplaying = TRUE;
				timePtr = 0;
/*
FLAGS: showingstop, done, playing
playflag = PLAYING, PAUSED, or STOPPED
if Play pressed,
	if (!stopshowing)
		insert STOP
	modify Play to PAUSE
if PAUSE pressed
	save info
	modify PAUSE to PLAY
if PLAY pressed
	get info
	if (!stopshowing)
		insert STOP
	modify PLAY to PAUSE
if STOP pressed (see WM_COMMAND)
	delete STOP
	modify PAUSE to Play
*/
				if (!fromP) {
					if (!showingstop) {
						showingstop = TRUE;
						InsertMenu(hMenu, 9, MF_BYPOSITION, (UINT)hMenu, "STOP");
						StopID = GetMenuItemID(hMenu, 9);

						timeFromBegin = 0;
						PageBegin = 0;
						FirstPixelOnPage = 0;
 						TopLeftPtr = FirstTopLeftPtr;
						Page = 0;
						_itoa(Page+1, &page[5], 10);
						line = 0;
					}
					if ((playflag == PAUSED) && ((!fromP))) {
						playflag = PLAYING;
						timePtr = savetimePtr;
						for (y = 0; y < (int)timePtr; y++) {
							if ((Event[y].note == 0) && (Event[y].message) && ((Event[y].message & 0xFFF0) != 0x40B0)) { // not sustain
								if (!EWQL)
									midiOutShortMsg(hMidiOut, Event[y].message);
								else
									midiOutShortMsg(hMidisOut[Event[y].port], Event[y].message);
							}
							if (Event[y].dMilliSecondsPerTick)
								dMilliSecondsPerTick = Event[y].dMilliSecondsPerTick;
						}
					}
					ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Pause);
					DrawMenuBar(hwnd);
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
				}

				else { // if (fromP)
					ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Stop2);
					DrawMenuBar(hwnd);
					for (x = 0; x < (int)e; x++) {
						if ((Event[x].velocity) && (Event[x].pixel >= CursorLoc)) {
							timePtr = x;
							for (y = 0; y < x; y++) {
								if ((Event[y].note == 0) && (Event[y].message) && ((Event[y].message & 0xFFF0) != 0x40B0)) {
									if (!EWQL)
										midiOutShortMsg(hMidiOut, Event[y].message);
									else
										midiOutShortMsg(hMidisOut[Event[y].port], Event[y].message);
								}
								if (Event[y].dMilliSecondsPerTick)
									dMilliSecondsPerTick = Event[y].dMilliSecondsPerTick;
							}
//							Sleep(100); // may be needed...
							if (startatp) {
								startatp = FALSE;
								keyboardactive = TRUE;
								ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, StopRecording);
								DrawMenuBar(hwnd);
							}
							break;
						}
					}
					if (x == (int)e) { // no notes after cursor location
						fromP = FALSE;
						playing = FALSE;
						playflag = STOPPED;
						ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Play);
						DrawMenuBar(hwnd);
						break;
					}
					if (volchange)
						ChangeVolume();
					if (TempoChange != 0.0)
						newTempo = TRUE;

					d = prevMilliSecondsPer = 0.0;
					prevTickptr = 0;
					for (y = 0; y < x; y++) {
						if (Event[y].dMilliSecondsPerTick) {
							d += (prevMilliSecondsPer * (double)(Event[y].tickptr - prevTickptr));
							prevMilliSecondsPer = Event[y].dMilliSecondsPerTick;
							prevTickptr = Event[y].tickptr;
						}
					}
					d += (prevMilliSecondsPer * (double)(Event[x].tickptr - prevTickptr));
					d2 = modf(d, &d3);// d2 is decimal part and d3 is integer part
					if (d2 > 0.5)
						d3++;
					timeFromBegin = (int)d3;
				} // end of if (fromP)

				MagicNumber = TicksPerQuarterNote / 120;// it seems to work
				lastE = e;
				done = FALSE;
				timeBeginPeriod(0);
				Startime = timeGetTime();
				Startime -= timeFromBegin;
				timeEndPeriod(0);
				timeBeginPeriod(TIMER_RESOLUTION);
				uTimerID = timeSetEvent(1, TIMER_RESOLUTION, TimerFunc, 0, TIME_ONESHOT);
			}
			break;

		case ID_RECORDMIDIKEYBOARD: // Record (on Menu)
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			if (!midi_in) {
				midiInStop(hMidiIn);
				midiInReset(hMidiIn);
				midiInClose(hMidiIn);
				if (MMSYSERR_NOERROR == midiInOpen((LPHMIDIIN)&hMidiIn, DeviceIn, (DWORD)MidiInProc, 0, CALLBACK_FUNCTION | MIDI_IO_STATUS)) {
					midi_in = TRUE;
					midiInStart(hMidiIn);
//					Menus(); // this adds another Midi In and Midi Out!
//					DrawMenuBar(hwnd);
				}
			}
			if ((!sustainflag) && ((midi_in) || (virtualkeyboard))) {
				if (keyboardactive)
					SendMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);
				else if (DialogBox(hInst, "RECORD", hwnd, RecordProc)) {
					if (metronome) { // metronome from Advanced -Percussion
						metronome = FALSE;
						if (uTimer0ID) {
							timeKillEvent(uTimer0ID);
							uTimer0ID = 0;
							timeEndPeriod(TIMER_RESOLUTION);
							midiOutShortMsg(hMidiOut, 0xB9 | (123 << 8));// All Channel 9 Notes Off
						}
						CurrentNotes = -1;
						PreviousNotes = 0;
					}
//					for (x = 0; x < (int)e; x++)
//						if (Event[x].velocity)
//							break;
//					if (x == (int)e) { // no notes entered yet
					if (startatp == FALSE) { // no notes entered yet
						if (pianoMetronome) { // Metronome selected in Options
							pianoMetronome = FALSE;
							timeKillEvent(uTimer13ID);
							timeEndPeriod(TIMER_RESOLUTION);
							uTimer13ID = 0;
						}
						keyboardactive = TRUE;
						Startime = 0;
						ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, StopRecording);
						firstnote = TRUE;
						DrawMenuBar(hwnd);

						othernotesplaying = FALSE;
						timePtr = 0;
						Tickptr = 0;
						counter = BeatsPerMeasure;
						mSecondsPerBeat = 60000000 / InitialBeatsPerMinute;
						milliSecondsPerBeat = mSecondsPerBeat / 1000;
						dMilliSecondsPerTick = 60000.0 / (double)(InitialBeatsPerMinute * TicksPerQuarterNote);// for Event[e].tickptr in WM_USER
						timeBeginPeriod(TIMER_RESOLUTION);// for Metronome
						uTimer2ID = timeSetEvent(milliSecondsPerBeat, TIMER_RESOLUTION, TimerFunc2, 0, TIME_PERIODIC);
						if (virtualkeyboard)
							SetFocus(hwndVirtualKeyboard); // here only
					}
//					else if (startatp) { // PLAY will check for startatp
//						SendMessage(hwnd, WM_COMMAND, PLAY, 0);
//						keyboardactive = TRUE; // AFTER PLAY sees it as (!keyboardactive)
//						ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, StopRecording);
//						DrawMenuBar(hwnd);
//					}
				}
			}
			if ((playflag == STOPPED) && (!midi_in) && (!virtualkeyboard))
				MessageBox(hwnd, "You need to connect a MIDI keyboard\nor select Virtual Keyboard.", &RecordMIDIKeyboard[1], MB_OK);
			break;

		case ID_ADVANCED_WAVE:
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			if (wavemixer == FALSE) {
				//////
				FillMixerBuf();
				//////
				hwndMixer = CreateDialog(hInst, "MIXER", hwnd, MixerProc);
				ShowQuality();
			}
			break;

		case ID_SHOWNOTESPLAYED:
			if (playflag != STOPPED)
				SendMessage(hwnd, WM_COMMAND, StopID, 0);
			if (DialogBox(hInst, "SHOWNOTESPLAYED", hwnd, ShowNoteProc)) {
				shownotesplayed = TRUE;
				for (x = 0; x < (int)e; x++)
					Event[x].time = 0;
				GetTextExtentPoint32(hdcMem, ShowingNotesPlayed, strlen(ShowingNotesPlayed), &showingnotesSize);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else {
				shownotesplayed = FALSE;
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case HELP_USING:
			if (!helping) {
				hwndHelp = CreateDialog(hInst, "HELP1", hwnd, HelpProc);
			}
			break;

		case ID_HELP_CHECKFORNEWERVERSION:
			SetCursor(hWaitingCursor);
			hOpen = InternetOpen(szAppName, INTERNET_OPEN_TYPE_PRECONFIG, NULL, NULL, 0);
			if (hInternet = InternetOpenUrl(hOpen, CheckforUpdate, Accept, -1, INTERNET_FLAG_NO_CACHE_WRITE|INTERNET_FLAG_KEEP_CONNECTION, 0))
			{
				char *MyWebPage;
				MyWebPage = (char*)malloc(10000);//shouldn't get any bigger than this
				for (fileSize = 0; (goodread = InternetReadFile(hInternet, MyWebPage, 10000, &dwBytesRead)) && (dwBytesRead); fileSize += dwBytesRead)
					;
				InternetCloseHandle(hInternet);
				if ((goodread) && (dwBytesRead == 0))
				{
					for (x = 0; x < (int)fileSize; x++)
					{
						if ((MyWebPage[x] == 'P') && (MyWebPage[x+1] == 'R') && (MyWebPage[x+2] == 'C'))// <!--PRC--><br>
						{
							z = x+12;
							x += 20;//to 9:00am Oct 26, 2009
							for (y = 11; About[y] != '\n'; x++, y++)
								if (MyWebPage[x] != About[y])
									break;
							if (About[y] != '\n') {// different
								for (x = z; MyWebPage[x] != '.'; x++)
									;
								MyWebPage[x-1] = 0;
								strcat(&MyWebPage[z], UpdateAvailable);
								MessageBox(hwnd, &MyWebPage[z], szAppName, MB_OK);
							}
							else {
								MessageBox(hwnd, "No new updates...", szAppName, MB_OK);
							}
							break;
						}
					}
				}
			}
			SetCursor(hCursor);
			break;

		case ID_HELP_ABOUT:
			MessageBox(hwnd, About, szAppName, MB_OK);
			break;
		}
		return 0;

	case WM_CHAR:
		if ((!playing) && (!keyboardactive)) {
			if (wParam == '#') {
				usingsharp = TRUE;
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else if ((wParam == 'b') || (wParam == 'B')) {
				usingsharp = FALSE;
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			if ((wParam == '~') && (sysex)) {
				midiHdr.lpData = SysEx;
				midiHdr.dwBufferLength = SysExLen;
				midiHdr.dwBytesRecorded = SysExLen;
				midiHdr.dwUser = 0;
				midiHdr.dwFlags = MHDR_ISSTRM;
				pMidiHdr = &midiHdr;
				MixerError = midiOutPrepareHeader(hMidiOut, pMidiHdr, sizeof(midiHdr));
				x = midiOutLongMsg(hMidiOut, pMidiHdr, sizeof(midiHdr));
				midiOutUnprepareHeader(hMidiOut, pMidiHdr, sizeof(midiHdr));
				if ((MixerError == 0) && (x == 0))
					MessageBox(hwnd, cSysEx, "Sent SysEx message", MB_OK);
			}
			if (wParam == 25) {// Ctrl-Y
				if ((pUndo < 9999) && (LastEventInUndo[pUndo+1] != -1)) {
					pUndo++;
					for (x = 0; x < (int)LastEventInUndo[pUndo]; x++)
						Event[x] = UndoEvent[pUndo][x];
					e = LastEventInUndo[pUndo];
					FillRect(hdcMem, &rect, hBrush);
//					TextOut(hdcMem, rect.right >> 1, 0, _itoa(pUndo, temp, 10), lstrlen(temp));
					InvalidateRect(hwnd, &rect, FALSE);
				}
				break;
			}

			else if (wParam == 26) {// Ctrl-Z
				SendMessage(hwnd, WM_COMMAND, ID_UNDO, 0);
//				StopTimers(); // to clear stuck notes
				break;
			}

			else if ((highlighting) && (wParam == 24)) {// Ctrl-X
				highlighting = FALSE;
				copying = NOCOPY;
				ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
				DrawMenuBar(hwnd);
				copySize = 0;

				if (ctrlA) {
					ctrlA = FALSE;
					wasctrlA = TRUE;
				}
				else
					wasctrlA = FALSE;
				while (lastX >= firstX) {
					if ((ActiveChannels[Event[firstX].channel]) || (Event[firstX].type)) {
						if (Event[firstX].velocity) {
							copyEvent[copySize++] = Event[firstX];
							for (z = firstX+1; z < e; z++) {
								if ((Event[z].note == Event[firstX].note) && (Event[z].pixel == (Event[firstX].pixel + Event[firstX].pixelsinnote)) && (Event[z].velocity == 0) && (Event[z].channel == Event[firstX].channel)) {
									copyEvent[copySize++] = Event[z];
									if (Event[z].pixel <= Event[lastX].pixel)
										lastX--;
									for ( ; z < e; z++)// remove Note Off Event
										Event[z] = Event[z+1];
									e--;
									break;
								}
							}
							for (z = firstX; z < e; z++)// remove Note On Event
								Event[z] = Event[z+1];
							e--;
							lastX--;
						}
						else if (Event[firstX].note == 0) {
							copyEvent[copySize++] = Event[firstX];
							if (((Event[firstX].message & 0xFFF0) == (DWORD)0x40B0) && ((Event[firstX].message & 0xFF0000) >= 0x400000)) {// sustain begin
								channel = Event[firstX].message & 0xF;// find the end and delete them both
								for (x = firstX+1; x < (int)e; x++) {
									if ((Event[firstX].message & 0xFFFF) == (Event[x].message & (0x40B0|channel)) && ((Event[x].message & 0xFF0000) < 0x400000)) {
										if (Event[x].pixel <= Event[lastX].pixel)
											lastX--;
										for (z = x; z < e; z++)// remove Other Events
											Event[z] = Event[z+1];
										e--;
										break;
									}
								}
							}
							else if (((Event[firstX].message & 0xFFF0) == (DWORD)0x40B0) && ((Event[firstX].message & 0xFF0000) < 0x400000)) {// sustain end
								channel = Event[firstX].message & 0xF;// find the beginning and delete them both
								for (x = firstX-1; x ; x--) {
									if ((Event[firstX].message & 0xFFFF) == (Event[x].message & (0x40B0|channel)) && ((Event[x].message & 0xFF0000) >= 0x400000)) {
										for (z = x; z < e; z++)// remove Other Events
											Event[z] = Event[z+1];
										e--;
										break;
									}
								}
							}
							for (z = firstX; z < e; z++)// remove Other Events
								Event[z] = Event[z+1];
							e--;
							lastX--;
						}
						else
							firstX++;
					}
					else
						firstX++;
				}

				NewChannel = 0xFF;
				CheckForNewInstrument();

				SaveEvents();
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}

			else if ((highlighting) && (wParam == 3)) {// Ctrl-C
				highlighting = FALSE;
				copying = NOCOPY;
				ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
				DrawMenuBar(hwnd);
				if (ctrlA) {
					ctrlA = FALSE;
					wasctrlA = TRUE;
				}
				else
					wasctrlA = FALSE;
				if (lastX >= firstX) {
					for (y = 0, x = firstX; x <= lastX; x++) {
						if ((ActiveChannels[Event[x].channel]) || (Event[x].type)) {
							if (Event[x].velocity) {
								copyEvent[y++] = Event[x];// Note On
								for (z = x+1; z < e; z++) {
									if ((Event[z].pixel == (Event[x].pixel + Event[x].pixelsinnote)) && (Event[z].note == Event[x].note) && (Event[z].channel == Event[x].channel) && (Event[z].velocity == 0)) {
										copyEvent[y++] = Event[z];// Note Off
										break;
									}
								}
							}
							else if (Event[x].note == 0)
								copyEvent[y++] = Event[x];// anything else
						}
					}
					copySize = y;

					NewChannel = 0xFF;
					CheckForNewInstrument();
				}
				else
					MessageBox(hwnd, "An empty highlighted area won't be cut or copied", "", MB_OK);
				InvalidateRect(hwnd, &rect, FALSE);
			}

			else if (wParam == 22) {// Ctrl-V
				if (copySize) {
					if (highlighting) {
						highlighting = FALSE;
						copying = NOCOPY;
						ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
						DrawMenuBar(hwnd);
					}
					l = (Page * Rows) + Row;
					if (wasctrlA) {
						copyX = 0;
					}
					else
						copyX = xLoc - (xLoc % 5) + Lines[l].pixel;
					if (copyBegin >= (copyEvent[0].pixel - 40))
						copyBegin = copyEvent[0].pixel;// if "just before" (within 1 quarter note)
					for (x = 0; x < (int)copySize; x++) {
						if (copyEvent[x].velocity) {// .note
							for (y = 0; y < (int)totalActiveChannels; y++) {
								if (copyEvent[x].channel == activeChannels[y]) {
									if (NewChannel != 0xFF) {
										copyEvent[x].channel = NewChannel;
										if ((copyEvent[x].message & 0x000000F0) == 0x90) {
											copyEvent[x].message &= 0xFFFFFFF0;
											copyEvent[x].message |= NewChannel;
										}
									}
									beginE = copyEvent[x].pixel - copyBegin + copyX;
									endE = beginE + copyEvent[x].pixelsinnote;
									for (z = 0; z < e; z++) {
										beginZ = Event[z].pixel;
										endZ = Event[z].pixel + Event[z].pixelsinnote;
										if ((copyEvent[x].note == Event[z].note) && (copyEvent[x].channel == Event[z].channel) && (Event[z].velocity)) {
											if (((beginE >= beginZ) && (endE <= endZ))
											 || ((endE > beginZ) && (endE <= endZ))
											 || ((beginE >= beginZ) && (beginE < endZ))
											 || ((beginE <= beginZ) && (endE >= endZ)))
											{// a collision
												break;
											}
										}
									}
									if (z == e) {// no collisions
										Event[e] = copyEvent[x];
										Event[e].pixel = beginE;
										Event[e].tickptr = beginE * TicksPerQuarterNote / 40;
										e++;
										Event[e] = Event[e-1];
										Event[e].velocity = 0;
										Event[e].pixel = Event[e-1].pixel + Event[e-1].pixelsinnote;
										Event[e].tickptr = Event[e].pixel * TicksPerQuarterNote / 40;
										Event[e].pixelsinnote = 0;
										Event[e].ticksinnote = 0;
										Event[e].message = Event[e-1].message & 0xFF00FFFF;
										e++;
									}
								}
							}
						}// end of if (copyEvent[x].velocity)
						else if ((copyEvent[x].note == 0) && (copyEvent[x].message & 0xFFF0 != 0x40B0)) {// don't paste sustain - too much trouble
							Event[e] = copyEvent[x];
							Event[e].pixel = beginE;
							Event[e].tickptr = beginE * TicksPerQuarterNote / 40;
							e++;
						}
					}
					SaveEvents();// Ctrl-V
					qsort(Event, e, sizeof(Event[0]), Compare);
					Resort();
					FillRect(hdcMem, &rect, hBrush);
				}
				else
					MessageBox(hwnd, "First press Ctrl-C or Ctrl-X\nafter highlighting an area.", "", MB_OK);
				InvalidateRect(hwnd, &rect, FALSE);
				break;
			}
		}
		return 0;

	case WM_SYSKEYDOWN:
		if (wParam == VK_F10) {
			ChangeInstrument(9);
			return 0;
		}
		break;

	case WM_KEYDOWN:
		if (chordnameflag) {
			chordnameflag = 0;
			hdc = GetDC(hwnd);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY); // get original screen
			ReleaseDC(hwnd, hdc);
		}
		switch (wParam)
		{
		case 'G':
			hFile = CreateFile("User.txt", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				if (UserFileSize = GetFileSize(hFile, NULL)) {
					if (UserFileSize > 5000)
						UserFileSize = 5000;
					ReadFile(hFile, UserBuf, UserFileSize, &dwBytesRead, NULL);
					hwndUser = CreateDialog(hInst, "USER", hwnd, UserProc);
				}
				CloseHandle(hFile);
			}
			break;

		case 'A':
			if (GetKeyState(VK_CONTROL) & 0x8000) {// Ctrl-A
				SendMessage(hwnd, WM_KEYDOWN, VK_HOME, 0);
				highlighting = FALSE;
				ctrlA = TRUE;
				SendMessage(hwnd, WM_COMMAND, ID_HIGHLIGHT, 0);
				if (ctrlA) {
					copying = COPYBUTTONUP;
					copyBegin = 0;
					for (x = 0, totalActiveChannels = 0; x < 16; x++)
						if (ActiveChannels[x])
							activeChannels[totalActiveChannels++] = x;
					for (x = 0; x < (int)e; x++) {
						if (Event[x].velocity) {
							for (y = 0; y < (int)totalActiveChannels; y++) {
								if (Event[x].channel == activeChannels[y]) {
									firstX = x;
									x = e;// to break out of outer loop
									break;
								}
							}
						}
					}
					for (x = firstX, lastX = firstX; x < (int)e; x++) {
						if ((Event[x].note) && (Event[x].velocity == 0)) {
							lastX = x;
						}
					}
				}
			}
			break;

		case 191: // '/' or '?'
			hwndHelp = CreateDialog(hInst, "HELP1", hwnd, HelpProc);
			break;
		case 187:// '+'
			if (GetKeyState(VK_CONTROL) & 0x8000) {// Ctrl is pressed
				DeleteObject(hLyricFont);
				lf4.lfEscapement -= 50;
				lf4.lfOrientation -= 50;
				hLyricFont = CreateFontIndirect(&lf4);
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else {
				if (volchange < 100)
					volchange += 20;
				ChangeVolume();
				UpdateVolChange();
			}
			break;
		case 189:// '-'
			if (GetKeyState(VK_CONTROL) & 0x8000) {// Ctrl is pressed
				DeleteObject(hLyricFont);
				lf4.lfEscapement += 50;
				lf4.lfOrientation += 50;
				hLyricFont = CreateFontIndirect(&lf4);
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else {
				if (volchange > -100)
					volchange -= 20;
				ChangeVolume();
				UpdateVolChange();
			}
			break;
		case VK_LEFT:
			movingleft = TRUE;
			if ((notetype[1] != 'F') && (notetype[2] != '3')) // sixty-fourth note or part of a triplet
				MoveNoteLeftRight(-5);
			else
				MoveNoteLeftRight(-1);
			break;

		case VK_RIGHT:
			movingleft = FALSE;
			if ((notetype[1] != 'F') && (notetype[2] != '3')) // sixty-fourth note or part of a triplet
				MoveNoteLeftRight(5);
			else
				MoveNoteLeftRight(1);
			break;

		case VK_UP:
			if ((!playing) && (!keyboardactive) && (!highlighting) && (Note < 108))
				MoveNoteUpDown(1);
			break;

		case VK_DOWN:
			if ((!playing) && (!keyboardactive) && (!highlighting) && (Note > 21))
				MoveNoteUpDown(-1);
			break;

		case VK_END:
			if ((!playing) && (!keyboardactive) && ((!highlighting) || (ctrlA)) && (Page < 98)) {
				while ((Page < 98) && (ptr < e)) {
					Page++;
					line = Page * Rows;
					TopLeftPtr = ptr;
					if (Event[TopLeftPtr].pixel > Lines[line].pixel)
						FirstPixelOnPage = Event[TopLeftPtr].pixel;
					else
						FirstPixelOnPage = Lines[line].pixel;
					PageBegin += lastRowLoc;
					_itoa(Page+1, &page[5], 10);
					FillRect(hdcMem, &rect, hBrush);
					for ( ; (ptr < e) && ((Event[ptr].pixel) < (Lines[Page*Rows].pixel + Lines[l].PixelsPerPage)); ptr++)
						;
				}
				InvalidateRect(hwnd, &rect, FALSE);
			}
			if (Page > 98)
				MessageBox(hwnd, "Music is over 99 pages", "", MB_OK);
			break;

		case VK_NEXT:
			if ((Page < 98) && (playflag == STOPPED) && (!keyboardactive) && ((!highlighting) || (ctrlA))) {
				Page++;
				line = Page * Rows;
				TopLeftPtr = ptr;
				if (Event[TopLeftPtr].pixel > Lines[line].pixel)
					FirstPixelOnPage = Event[TopLeftPtr].pixel;
				else
					FirstPixelOnPage = Lines[line].pixel;
				PageBegin += lastRowLoc;
				_itoa(Page+1, &page[5], 10);
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case VK_PRIOR:
			if ((playflag == STOPPED) && (!keyboardactive) && ((!highlighting) || (ctrlA)) && (Page)) {
				Page--;
				ptr = 0;
				for (z = 0; z < (DWORD)Page; z++)
					for ( ; (ptr < e) && ((Event[ptr].pixel) < (Lines[z*Rows].pixel + Lines[z*Rows].PixelsPerPage)); ptr++)
						;
				TopLeftPtr = ptr;
				FirstPixelOnPage = Event[TopLeftPtr].pixel;
				PageBegin -= lastRowLoc;
				_itoa(Page+1, &page[5], 10);
				line = Page * Rows;
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case VK_F1:
			ChangeInstrument(0);
			break;
		case VK_F2:
			ChangeInstrument(1);
			break;
		case VK_F3:
			ChangeInstrument(2);
			break;
		case VK_F4:
			ChangeInstrument(3);
			break;
		case VK_F5:
			ChangeInstrument(4);
			break;
		case VK_F6:
			ChangeInstrument(5);
			break;
		case VK_F7:
			ChangeInstrument(6);
			break;
		case VK_F8:
			ChangeInstrument(7);
			break;
		case VK_F9:
			ChangeInstrument(8);
			break;
		case VK_F11:
			ChangeInstrument(10);
			break;
		case VK_F12:
			ChangeInstrument(11);
			break;
////////////////////////////////
		case 'M':
			if (GetKeyState(VK_CONTROL) < 0) // Ctrl key down
				Metronome();
			else if (mixerControlDetailsMasterMute.fValue)
				mixerControlDetailsMasterMute.fValue = 0;
			else
				mixerControlDetailsMasterMute.fValue = 1;
			mute = TRUE;
			SetMasterVolume();
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '1':
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = (int)mixerVolTick;
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = ' ';
			MasterVolume[5] = '1';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '2':
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = (int)(mixerVolTick * 2.0);
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = ' ';
			MasterVolume[5] = '2';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '3':
			if (GetKeyState(VK_SHIFT) >= 0) {// Shift key not down
				if (mixerControlDetailsMasterMute.fValue)
					SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
				if (GetKeyState(VK_CONTROL) < 0) {// Ctrl key down (for a Third note)
					if ((notetype[2] == ' ') && (notetype[1] != 'S') && (notetype[1] != 'T') && (notetype[1] != 'F')) {
						notetype[2] = '3';
						TextOut(hdcMem, 0, 0, notetype, 4);
					}
					else {
						notetype[2] = ' ';
						TextOut(hdcMem, 0, 0, notetype, 4);
					}
					if (!playing)
						InvalidateRect(hwnd, &rect, FALSE);
				}
				else {
					mixerControlDetailsVolume.dwValue = (int)(mixerVolTick * 3.0);
					fromkeyboard = TRUE;
					SetMasterVolume();
					MasterVolume[4] = ' ';
					MasterVolume[5] = '3';
					if (!playing)
						InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			break;
		case '4':// for triplet note length (1/3 of notetype)
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = (int)(mixerVolTick * 4.0);
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = ' ';
			MasterVolume[5] = '4';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '5':
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = (int)(mixerVolTick * 5.0);
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = ' ';
			MasterVolume[5] = '5';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '6':
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = (int)(mixerVolTick * 6.0);
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = ' ';
			MasterVolume[5] = '6';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '7':
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = (int)(mixerVolTick * 7.0);
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = ' ';
			MasterVolume[5] = '7';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '8':
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = (int)(mixerVolTick * 8.0);
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = ' ';
			MasterVolume[5] = '8';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '9':
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = (int)(mixerVolTick * 9.0);
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = ' ';
			MasterVolume[5] = '9';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '0':
			if (mixerControlDetailsMasterMute.fValue)
				SendMessage (hwnd, WM_KEYDOWN, 'M', 0);
			mixerControlDetailsVolume.dwValue = 0xFFFF;
			fromkeyboard = TRUE;
			SetMasterVolume();
			MasterVolume[4] = '1';
			MasterVolume[5] = '0';
			if (!playing)
				InvalidateRect(hwnd, &rect, FALSE);
			break;
////////////////////////////////
		case 190:// a period (dot)
			if (notetype[2] == ' ') {
				notetype[2] = '.';
				TextOut(hdcMem, 0, 0, notetype, 4);
			}
			else {
				notetype[2] = ' ';
				TextOut(hdcMem, 0, 0, notetype, 4);
			}
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case VK_ESCAPE:
			ctrlA = FALSE;
			if (playingwave) {
				waveOutReset(hWaveOut);
				if (playing2wavefiles) {
					waveOutReset(hWaveOut2);
				}
				SetFocus(hwndMixer);
			}
			if (recordingtowave) {
				recordingtowave = FALSE;
				playing = FALSE;
				playflag = STOPPED;

				mmTime.wType = TIME_SAMPLES;
				waveInGetPosition(hWaveIn, &mmTime, sizeof(mmTime));
				BufferSize = mmTime.u.sample * nRecordChannels * BytesPerSample;

				waveInUnprepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
				waveInReset(hWaveIn);
				waveInClose(hWaveIn); // this will activate waveInProc, which will active WM_USER6
				SetFocus(hwndMixer);
			}
			if (copying != NOCOPY) {
				highlighting = FALSE;
				copying = NOCOPY;
				ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
				DrawMenuBar(hwnd);
				break;
			}
			if (keyboardactive) {
				keyboardactive = FALSE;
				ModifyMenu(hMenu, ID_RECORDMIDIKEYBOARD, MF_BYCOMMAND|MF_STRING, ID_RECORDMIDIKEYBOARD, RecordMIDIKeyboard);
				DrawMenuBar(hwnd);
				if (uTimer2ID) {
					timeKillEvent(uTimer2ID);
					timeEndPeriod(TIMER_RESOLUTION);
					uTimer2ID = 0;
				}
				qsort(Event, e, sizeof(Event[0]), Compare);
				Resort();
				DeleteDuplicateNotes();
				SaveEvents();
				SendMessage(hwnd, WM_USER3, 0, 0);// stop playing
			}
			else if ((playing) || (playflag == PAUSED) || (metronome)) {
				if (fromP)
					SendMessage(hwnd, WM_USER3, 0, 0);// stop playing
				else
					SendMessage(hwnd, WM_COMMAND, StopID, 0);
			}
			else
				StopTimers();
			break;

		case VK_SPACE:
			SendMessage(hwnd, WM_COMMAND, PLAY, 0);
			break;

		case VK_HOME:
			if ((playflag == STOPPED) && (!keyboardactive) && (!highlighting)) {
				PageBegin = 0;
				FirstPixelOnPage = 0;
 				TopLeftPtr = FirstTopLeftPtr;
				Page = 0;
				_itoa(Page+1, &page[5], 10);
				line = 0;
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;

		case 'W':
			SendMessage(hwnd, WM_COMMAND, ID_NOTETYPE_WHOLE, 0);
			break;
		case 'H':
			SendMessage(hwnd, WM_COMMAND, ID_NOTETYPE_HALF, 0);
			break;
		case 'Q':
			SendMessage(hwnd, WM_COMMAND, ID_NOTETYPE_QUARTER, 0);
			break;
		case 'E':
			SendMessage(hwnd, WM_COMMAND, ID_NOTETYPE_EIGHTH, 0);
			break;
		case 'S':
			SendMessage(hwnd, WM_COMMAND, ID_NOTETYPE_SIXTEENTH, 0);
			break;
		case 'T':
			SendMessage(hwnd, WM_COMMAND, ID_NOTETYPE_THIRTYSECOND, 0);
			break;
		case 'F':
			SendMessage(hwnd, WM_COMMAND, ID_NOTETYPE_SIXTYFOURTH, 0);
			break;
//		case 'C':// Ctrl-C interfers
		case 'N':
			SendMessage(hwnd, WM_COMMAND, ID_NEW, 0);
			break;
		case 'O':
			SendMessage(hwnd, WM_COMMAND, ID_OPTIONS, 0);
			break;
		case 'I':
			SendMessage(hwnd, WM_COMMAND, ID_INSTRUMENTS, 0);
			break;
		case 'P':// to start playing from mouse pointer
			if (playflag == STOPPED) {
				line = (Page * Rows);
				l = line + Row;
				CursorLoc = xLoc + Lines[l].pixel;// for PLAY
				fromP = TRUE;
				SendMessage(hwnd, WM_COMMAND, PLAY, 0);
			}
			break;

		case 'U':
			SendMessage(hwnd, WM_COMMAND, ID_TRANSPOSEUP, 0);
			SaveEvents();
			break;
		case 'D':
			SendMessage(hwnd, WM_COMMAND, ID_TRANSPOSEDOWN, 0);
			SaveEvents();
			break;
		case 'R':
			SendMessage(hwnd, WM_COMMAND, ID_RECORDMIDIKEYBOARD, 0);
			break;
		case 'K':
			SendMessage(hwnd, WM_COMMAND, ID_VIRTUALKEYBOARD, 0);
			break;

		case VK_DELETE:
			if ((!playing) && (!keyboardactive)) {
				if (GetKeyState(VK_SHIFT) & 0x8000) // Shift is pressed
					shiftpressed = TRUE;
				if (GetKeyState(VK_CONTROL) & 0x8000) // Control is pressed
					ctrlpressed = TRUE;
				atnote = TRUE;
				if (highlighting) {
					highlighting = FALSE;
					copying = NOCOPY;
					ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
					DrawMenuBar(hwnd);

					while (lastX >= firstX) {
						if ((ActiveChannels[Event[firstX].channel]) || (Event[firstX].type)) {
 							if ((Event[firstX].velocity)) {
								for (z = firstX+1; z < e; z++) {
									if ((Event[z].note == Event[firstX].note) && (Event[z].pixel == (Event[firstX].pixel + Event[firstX].pixelsinnote)) && (Event[z].velocity == 0) && (Event[z].channel == Event[firstX].channel) && (Event[z].overlapped == Event[firstX].overlapped)) {
										if (Event[z].pixel <= Event[lastX].pixel)
											lastX--;
										for ( ; z < e; z++)// remove Note Off Event
											Event[z] = Event[z+1];
										e--;
										break;
									}
								}
								for (z = firstX; z < e; z++)// remove Note On Event
									Event[z] = Event[z+1];
								e--;
								lastX--;
							}
							else if (Event[firstX].note == 0) {
								if (((Event[firstX].message & 0xFFF0) == (DWORD)0x40B0) && ((Event[firstX].message & 0xFF0000) >= 0x400000)) {// sustain begin
									channel = Event[firstX].message & 0xF;// find the end and delete them both
									for (x = firstX+1; x < (int)e; x++) {
										if ((Event[firstX].message & 0xFFFF) == (Event[x].message & (0x40B0|channel)) && ((Event[x].message & 0xFF0000) < 0x400000)) {
											if (Event[x].pixel <= Event[lastX].pixel)
												lastX--;
											for (z = x; z < e; z++)// remove Other Events
												Event[z] = Event[z+1];
											e--;
											break;
										}
									}
								}
								else if (((Event[firstX].message & 0xFFF0) == (DWORD)0x40B0) && ((Event[firstX].message & 0xFF0000) < 0x400000)) {// sustain end
									channel = Event[firstX].message & 0xF;// find the beginning and delete them both
									for (x = firstX-1; x ; x--) {
										if ((Event[firstX].message & 0xFFFF) == (Event[x].message & (0x40B0|channel)) && ((Event[x].message & 0xFF0000) >= 0x400000)) {
											for (z = x; z < e; z++)// remove Other Events
												Event[z] = Event[z+1];
											e--;
											break;
										}
									}
								}
								for (z = firstX; z < (DWORD)lastX; z++)// remove Other Events
									Event[z] = Event[z+1];
								e--;
								lastX--;
							}
							else
								firstX++;
						}
						else
							firstX++;
					}
					FillRect(hdcMem, &rect, hBrush);
				}// end of if (highlighting)

				else if (yLoc > ((Row+1) * PixelsInGrandStaffAndMyExtraSpace)) {// DELETE lyric
					z = xLoc + Lines[l].pixel;
					for (x = 0; x < (int)e; x++) {
						if ((Event[x].type == 5) && (z < (Event[x].pixel+100))) {
							hOldFont = SelectObject(hdcMem, hLyricFont);
							GetTextExtentPoint32(hdcMem, &Data[Event[x].ptr], Event[x].len, &size);
							SelectObject(hdcMem, hOldFont);
							if ((z > Event[x].pixel) && (z < Event[x].pixel + size.cx)) {
								for ( ; x < (int)e; x++)
									Event[x] = Event[x+1];// delete entry
								e--;
								break;
							}
						}
					}
				}

				else if ((showvolume) || (showexpression) || (showmodulation) || (showreverb) || (showchorus) || (showpitchbend) || (showportamento)) {
					l = (Page * Rows) + Row;
					z = xLoc + Lines[l].pixel;
					atnote = FALSE;

					for (x = 0; x < (int)e; x++) {
						if ((Event[x].pixel < z) && ((Event[x].pixel+Event[x].pixelsinnote) > z) && (Event[x].note))
							atnote = TRUE;
						if ((Event[x].pixel < z) && ((Event[x].pixel+size.cx) > z)) {
							if ((showvolume) && ((Event[x].message & 0xFFFF) == (DWORD)(0x07B0|ActiveChannel))) {
								y = ((Event[x].message & 0xFF0000) >> 0x10);
								w =  ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (127-y);
								if ((yLoc > w) && (yLoc < (w + size.cy))) {
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];
									e--;
									atnote = FALSE;
									if (!EWQL)
										midiOutShortMsg(hMidiOut, (0xB0|ActiveChannel) | (7 << 8) | (127 << 16)); // volume to 127
									else
										midiOutShortMsg(hMidisOut[Event[x].port], (0xB0|ActiveChannel) | (7 << 8) | (127 << 16));
									break;
								}
							}
							else if ((showexpression) && ((Event[x].message & 0xFFFF) == (DWORD)(0x0BB0|ActiveChannel))) {
								y = ((Event[x].message & 0xFF0000) >> 0x10);
								w =  ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (127-y);
								if ((yLoc > w) && (yLoc < (w + size.cy))) {
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];
									e--;
									atnote = FALSE;
									if (!EWQL)
										midiOutShortMsg(hMidiOut, (0xB0|ActiveChannel) | (11 << 8) | (127 << 16)); // expression to 127
									else
										midiOutShortMsg(hMidisOut[Event[x].port], (0xB0|ActiveChannel) | (11 << 8) | (127 << 16));
									break;
								}
							}
							else if ((showmodulation) && ((Event[x].message & 0xFFFF) == (DWORD)(0x01B0|ActiveChannel))) {
								y = ((Event[x].message & 0xFF0000) >> 0x10);
								w = ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + PixelsInGrandStaffAndMyExtraSpace - y;
								if ((yLoc > w) && (yLoc < (w + size.cy))) {
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];
									e--;
									atnote = FALSE;
									if (!EWQL)
										midiOutShortMsg(hMidiOut, (0xB0|ActiveChannel) | (1 << 8)); // modulation to 0
									else
										midiOutShortMsg(hMidisOut[Event[x].port], (0xB0|ActiveChannel) | (1 << 8));
									break;
								}
							}
							else if ((showreverb) && ((Event[x].message & 0xFFFF) == (DWORD)(0x5BB0|ActiveChannel))) {
								y = ((Event[x].message & 0xFF0000) >> 0x10);
								w = ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + PixelsInGrandStaffAndMyExtraSpace - y;
								if ((yLoc > w) && (yLoc < (w + size.cy))) {
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];
									e--;
									atnote = FALSE;
									if (!EWQL)
										midiOutShortMsg(hMidiOut, (0xB0|ActiveChannel) | (91 << 8)); // reverb to 0
									else
										midiOutShortMsg(hMidisOut[Event[x].port], (0xB0|ActiveChannel) | (91 << 8));
									break;
								}
							}
							else if ((showchorus) && ((Event[x].message & 0xFFFF) == (DWORD)(0x5DB0|ActiveChannel))) {
								y = ((Event[x].message & 0xFF0000) >> 0x10);
								w = ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + PixelsInGrandStaffAndMyExtraSpace - y;
								if ((yLoc > w) && (yLoc < (w + size.cy))) {
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];
									e--;
									atnote = FALSE;
									if (!EWQL)
										midiOutShortMsg(hMidiOut, (0xB0|ActiveChannel) | (93 << 8)); // chorus to 0
									else
										midiOutShortMsg(hMidisOut[Event[x].port], (0xB0|ActiveChannel) | (93 << 8));
									break;
								}
							}
							else if ((showpitchbend) && ((Event[x].message & 0xFF) == (DWORD)(0xE0|ActiveChannel))) {
								y = (Event[x].message & 0xFFFF00) >> 8;
								PitchBend1 = y & 0x7F;
								PitchBend2 = (WORD)((y >> 1) & (0x7F << 7));
								PitchBend = (PitchBend2 | PitchBend1);
								PitchBendLoc = PitchBend * PixelsBetweenLines * 20 / 0x4000;
								w = ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (PixelsBetweenLines * 24) - PitchBendLoc;
								if ((yLoc > w) && (yLoc < (w + size.cy))) {
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];
									e--;
									atnote = FALSE;
									if (!EWQL)
										midiOutShortMsg(hMidiOut, (0xE0|ActiveChannel) | (8192 << 8));
									else
										midiOutShortMsg(hMidisOut[Event[x].port], (0xE0|ActiveChannel) | (8192 << 8));
									break;
								}
							}
							else if ((showportamento) && ((Event[x].message & 0xFFFF) == (DWORD)(0x41B0|ActiveChannel))) {
								w = (((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (PixelsBetweenLines*27/2)); // down 13 1/2 lines from top
								if ((yLoc > w) && (yLoc < (w + size.cy))) {
									saveX = x;
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];
									e--;
									for (x = saveX-1; x < (int)e; x++)
										if ((Event[x].message & 0xFFFFFF) == (DWORD)(0x0041B0|ActiveChannel)) {
											for ( ; x < (int)e; x++)
												Event[x] = Event[x+1];
											e--;
										}
									atnote = FALSE;
									break;
								}
							}
						}
					}
				}

				if (atnote) {// VK_DELETE
					ordinalNote = -1; // flag for MoveNoteUpDown
					OriginalNote = 1;// trick for MoveNoteUpDown
					y = yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace);
					for (x = 0; x < 88; x++)
						if (y >= NoteLoc[x])
							break;
					if (GetKeyState(VK_CONTROL) < 0) {
						if (usingsharp)
							x++;
						else
							x--;
					}
					Note = x + 21;
					l = (Page * Rows) + Row;
					z = xLoc + Lines[l].pixel;
					tx = 0;
					ThisX[1] = 0xFF;
					for (x = 0; x < (int)e; x++) {
						if ((z >= Event[x].pixel) && (z <= (Event[x].pixel + Event[x].pixelsinnote))) {
							if ((Event[x].note == Note)
							|| ((!usingsharp) && ((!Letter[Note-1-21]) && (Event[x].note == Note-1)))
							|| ((usingsharp) && ((!Letter[Note+1-21]) && (Event[x].note == Note+1)))) {
								if (Event[x].channel == ActiveChannel) {
 									for (y = x+1; y < (int)e; y++) {
										if ((Event[y].note == Event[x].note) && (Event[y].velocity == 0) && (Event[y].channel == ActiveChannel) && (Event[y].overlapped == Event[x].overlapped)) {
											for ( ; y < (int)e; y++)
												Event[y] = Event[y+1];// delete note off
											e--;
										}
									}
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];// delete note on
									e--;
									break;
								}
								else if (tx < 2)
									ThisX[tx++] = x;
							}
						}
					}
					if (x == (int)e) {
						if (ThisX[1] != 0xFF) {// more than one instrument and neither is ACTIVE
							OriginalNote = 0;// flag
							MessageBox(hwnd, ToDelete, "To Delete", MB_OK);
							shiftpressed = ctrlpressed = FALSE;
							return 0;
						}
						else if (tx) {
							x = ThisX[0];// a single non-ACTIVE note
 							for (y = x+1; y < (int)e; y++) {
								if ((Event[y].note == Event[x].note) && (Event[y].velocity == 0) && (Event[y].channel == Event[x].channel) && (Event[y].overlapped == Event[x].overlapped)) {
									for ( ; y < (int)e; y++)
										Event[y] = Event[y+1];// delete note off
									e--;
								}
							}
							for ( ; x < (int)e; x++)
								Event[x] = Event[x+1];// delete note on
							e--;
						}
						else {// deleting a space
							if ((!shiftpressed) && (!ctrlpressed))
								GetTicksInNote();// TempEvent.ticksinnote = ([Quarter Note or whatever] * TicksPerQuarterNote / 40);
							else
								TempEvent.ticksinnote = 12;
							TempEvent.pixelsinnote = TempEvent.ticksinnote * 40 / TicksPerQuarterNote;
							for (x = 0; x < (int)e; x++)
								if ((Event[x].pixel >= z) && ((Event[x].velocity) || (Event[x].note == 0)))
									break;
							good = TRUE;
							for (y = x-1; y; y--) {
								if ((Event[y].velocity) && ((Event[y].pixel + Event[y].pixelsinnote) > (Event[x].pixel - TempEvent.pixelsinnote))) {
									good = FALSE;
									break;
								}
							}
							if (good) {
								for (y = x-1; y >= 0; y--)// go towards 0
									if ((Event[y].pixel == Event[x].pixel) && (Event[y].velocity) && (Event[y].note == Event[x].note) && (Event[y].sharporflat == Event[x].sharporflat))
										break;
								if ((y == -1) && (TempEvent.ticksinnote <= Event[x].tickptr)) {// at 0 and there's room to delete space
									for ( ; x < (int)e; x++) {
										Event[x].tickptr -= TempEvent.ticksinnote;
										Event[x].pixel -= TempEvent.pixelsinnote;
									}
								}
								else if ((Event[y].tickptr + Event[y].ticksinnote + TicksPerPixel + TempEvent.ticksinnote) <= Event[x].tickptr) {
									for ( ; x < (int)e; x++) {
										Event[x].tickptr -= TempEvent.ticksinnote;
										Event[x].pixel -= TempEvent.pixelsinnote;
									}
								}
							}
						}
					}
				}// end of if (atnote)
				SaveEvents();// VK_DELETE
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			shiftpressed = ctrlpressed = FALSE;
			break;

		case VK_INSERT:// insert a space
			if ((!playing) && (!keyboardactive)) {
				if (GetKeyState(VK_SHIFT) & 0x8000) // Shift is pressed
					shiftpressed = TRUE;
				if (GetKeyState(VK_CONTROL) & 0x8000) // Control is pressed
					ctrlpressed = TRUE;
				l = (Page * Rows) + Row;
				z = xLoc + Lines[l].pixel;
				if ((!shiftpressed) && (!ctrlpressed))
					GetTicksInNote();// get size of space using current notetype
				else
					TempEvent.ticksinnote = 12;
				TempEvent.pixelsinnote = TempEvent.ticksinnote * 40 / TicksPerQuarterNote;
				for (x = 0; x < (int)e; x++)
					if ((Event[x].pixel >= z) && (Event[x].velocity))
						break; // start inserting a space at cursor (and not until the first note)
				for (n = 0; x < (int)e; x++) {
					if (Event[x].velocity) {// note on
						NotesOn[n++] = Event[x].note;
						Event[x].tickptr += TempEvent.ticksinnote;
						Event[x].pixel += TempEvent.pixelsinnote;
					}
					else if (Event[x].note) {// note off
						for (y = 0; y < n; y++) {// this is for notes that start before insert point and end after it
							if (Event[x].note == NotesOn[y]) {
								for ( ; y < n; y++)
									NotesOn[y] = NotesOn[y+1];
								n--;
								Event[x].tickptr += TempEvent.ticksinnote;
								Event[x].pixel += TempEvent.pixelsinnote;
								break;
							}
						}
					}
					else {// BPM change
						Event[x].tickptr += TempEvent.ticksinnote;
						Event[x].pixel += TempEvent.pixelsinnote;
					}
				}
				SaveEvents();// VK_INSERT
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			shiftpressed = ctrlpressed = FALSE;
			break;
		}
		return 0;


	case WM_LBUTTONDOWN:
		if ((playing) || (keyboardactive))
			return 0;
		shiftpressed = ctrlpressed = FALSE;
		if (GetKeyState(VK_SHIFT) & 0x8000) // Shift is pressed
			shiftpressed = TRUE;
		if ((wParam == MK_CONTROL) || (wParam == (MK_LBUTTON | MK_CONTROL))) // move or look for sharp/flat notes
			ctrlpressed = TRUE;
		xLoc = LOWORD(lParam);
		yLoc = HIWORD(lParam);
		l = (Page * Rows) + Row; // Row from WM_MOUSEMOVE
		z = xLoc + Lines[l].pixel;
		saveZ = z;
		ButtonDownRow = Row;
		ButtonDownPage = Page;
//		ordinalNote = -1; // flag for MoveNoteUpDown
//		OriginalNote = 1;// trick for MoveNoteUpDown
//		OriginalNote = 0;// trick for MoveNoteUpDown
		if (changingvelocity) {
			Event[current].velocity = NewVelocity;
			InvalidateRect(hwnd, &rect, FALSE);
			return 0;
		}
		if (copying == COPY) {
			copying = COPYBUTTONDOWN;
			srcBeginX = xLoc;
			srcBeginY = Row * BottomNoteTop;
			for (x = 0, totalActiveChannels = 0; x < 16; x++)
				if (ActiveChannels[x])
					activeChannels[totalActiveChannels++] = x;
			copyBegin = z;
			for (x = 0; x < (int)e; x++) {
				if ((Event[x].pixel >= z) && (Event[x].velocity)) {
					for (y = 0; y < (int)totalActiveChannels; y++) {
						if (Event[x].channel == activeChannels[y]) {
							firstX = x;
							x = e;// to break out of outer loop
							break;
						}
					}
				}
			}
			if (x == (int)e) {
				highlighting = FALSE;
				copying = NOCOPY;
				ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
				DrawMenuBar(hwnd);
			}
			return 0;
		}
		else if (copying == COPYBUTTONUP) {// disregard the copying
			highlighting = FALSE;
			copying = NOCOPY;
			ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
			DrawMenuBar(hwnd);
			return 0;
		}
		else if (Editing != -1) {
			EditingBeginX = xLoc;
			EditingBeginY = yLoc;
			if ((Editing == 0) || (Editing == 1)) { // Volume or Expression
				if ((yLoc >= RowTop) && ((yLoc - RowTop) <= 127))
					volBegin = RowTop - yLoc + 127; // 127 at RowTop and 0 at RowTop - 127
				else
					volBegin = 0xFFFF; // flag
			}
			else if ((Editing >= 2) && (Editing <= 4)) { // Reverb, Chorus, or Modulation
				if ((yLoc < (RowTop + PixelsInGrandStaff)) && ((RowTop + PixelsInGrandStaff - yLoc) <= 127))
					volBegin = RowTop + PixelsInGrandStaff - yLoc;
				else
					volBegin = 0xFFFF;
			}
			else if (Editing == 5)
				volBegin = yLoc;
			else if (Editing == 6) // Portamento (WM_LBUTTONDOWN)
				volBegin = yLoc;
			return 0;
		}

		if (sustainflag) {
			if (sustainflag == 1) {// creating
				itsbad = FALSE;
				for (x = 0; x < (int)e; x++) {
					if ((Event[x].message & 0x007FFFFF) == (DWORD)(0x007F40B0 | ActiveChannel)) {
						for (y = x+1; y < (int)e && (Event[y].message != (DWORD)(0x000040B0 | ActiveChannel)); y++)
							;
						if ((z >= Event[x].pixel) && (z <= Event[y].pixel)) {
							itsbad = TRUE;
						}
					}
				}
				if (itsbad == FALSE) {
					BeginSustain = z;
					srcBeginX = xLoc;
					srcBeginY = (Row+1) * PixelsInGrandStaffAndMyExtraSpace;
					hdc = GetDC(hwnd);
					MoveToEx(hdc, srcBeginX, srcBeginY, NULL);
					LineTo(hdc, xLoc, srcBeginY);
					ReleaseDC(hwnd, hdc);
				}
				else
					srcBeginX = -1;// flag
			}
		}

		// else if over music staves (WM_LBUTTONDOWN)
		else if ((xLoc < Lines[l].PixelsPerLine) && (yLoc > ((Row * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace - 6) && (yLoc < ((Row * PixelsInGrandStaffAndMyExtraSpace) + PixelsInGrandStaffAndMyExtraSpace)))) {
			if (ChannelInstruments[ActiveChannel][0] < 128) {// in case none is selected  //|| (ActiveChannel == 9)
				if (!chords2enter)
					sharpflat = GetKeyState(VK_CONTROL) & 0x80000000;
				else
					chords2enter = FALSE;
				y = yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace);
				for (x = 0; x < 88; x++) {
					if (y >= NoteLoc[x]) {
						if ((ActiveChannel == 9) && (!EWQL) && (((x+21) < 27) || ((x+21) > 87)))
							return 0;// percussion limits
						TempEvent.note = x + 21;
						TempEvent.sharporflat = 0;
						if (sharpflat) {
							if ((usingsharp) && (TempEvent.note < 108) && (0 == Letter[x+1])) {
								TempEvent.note++;// a sharped note
								TempEvent.sharporflat = 1;
							}
							else if ((!usingsharp) && (TempEvent.note > 21) && (0 == Letter[x-1])) {
								TempEvent.note--;// a flatted note
								TempEvent.sharporflat = 1;
							}
						}
						TempEvent.channel = ActiveChannel;
						TempEvent.port = Port;
						TempEvent.velocity = Volume;
						TempEvent.type = 0;
						TempEvent.len = 0;
						TempEvent.ptr = 0;
						break;
					}
				}
				if (notetype[1] != 'F') {// if not sixty-fourth notes
					if (!shiftpressed)
						w = xLoc % NoteEvery;
					else
						w = xLoc % 5; // to put quarter note at any 32nd note location
					x = xLoc - w;// ticks/thirtysecond note = 60 (5 pixels)
					if (notetype[2] == '3') { // triplet
						NoteEvery3 = (NoteEvery/3) + 1;
						if ((xLoc - x) >= (NoteEvery3*2))
							x += (NoteEvery3*2);
						else if ((xLoc - x) >= NoteEvery3)
							x += NoteEvery3;
					}
				}
				else // (notetype[1] == 'F')
					x = xLoc;
				x += Lines[l].pixel; // l = (Page * Rows) + Row;
				TempEvent.pixel = x;
				TempEvent.tickptr = x * TicksPerQuarterNote / 40;

				GetTicksInNote();// get TempEvent.ticksinnote

				TempEvent.pixelsinnote = TempEvent.ticksinnote * 40 / TicksPerQuarterNote;
				for (x = 1; x < (int)e; x++) {
					if ((TempEvent.note == Event[x].note) && (TempEvent.channel == Event[x].channel) && (TempEvent.port == Event[x].port) && (Event[x].velocity)) {
						if ((TempEvent.pixel >= Event[x].pixel) && (TempEvent.pixel < (Event[x].pixel + Event[x].pixelsinnote)))
							break;
						else if (((TempEvent.pixel + TempEvent.pixelsinnote) > Event[x].pixel) && ((TempEvent.pixel + TempEvent.pixelsinnote) <= (Event[x].pixel + Event[x].pixelsinnote)))
							break;
						else if ((TempEvent.pixel <= Event[x].pixel) && ((TempEvent.pixel + TempEvent.pixelsinnote) >= (Event[x].pixel + Event[x].pixelsinnote)))
							break;
					}
				}
				if (x == (int)e) {// not covering another note
					TempEvent.message = (0x90|ActiveChannel) | (TempEvent.velocity << 16) | (TempEvent.note << 8);
					if (!chord) {
						Tickptr = TempEvent.tickptr;
						Event[e] = TempEvent;

						if (!EWQL)
							midiOutShortMsg(hMidiOut, Event[e].message);// start the note
						else
							midiOutShortMsg(hMidisOut[Event[e].port], Event[e].message);// start the note

						if (FirstPixelOnPage > Event[e].pixel)
							FirstPixelOnPage = Event[e].pixel;// necessary
						for (x = 1; x < (int)e; x++) {// sort Event[e]
							if (Event[x].tickptr > Event[e].tickptr) {
								for (y = e; y > x; y--)
									Event[y] = Event[y-1];
								Event[x] = TempEvent;
								break;
							}
						}
						e++;
						Event[e].velocity = 0;// note end
						Event[e].tickptr = Tickptr + Event[x].ticksinnote;
						Event[e].pixelsinnote = 0;
						Event[e].ticksinnote = 0;
						Event[e].pixel = Event[e].tickptr * 40 / TicksPerQuarterNote;
						Event[e].note = Event[x].note;
						Event[e].channel = ActiveChannel;
						Event[e].port = Port;
						Event[e].message = (0x90|ActiveChannel) | (Event[e].note << 8);
						TempEvent = Event[e];
						time = (unsigned int)(dMilliSecondsPerTick * (double)(Event[e].tickptr - Tickptr));
						StopTimer();
						for (x = 1; x < (int)e; x++) {// sort Event[e]
							if (Event[x].tickptr > Event[e].tickptr) {
								for (y = e; y > x; y--)
									Event[y] = Event[y-1];
								Event[x] = TempEvent;
								break;
							}
						}
						e++;
					} // end of if (!chord)
					else if ((ActiveChannel != 9) || (EWQL)) {
						SetFocus(hwndChords);
						timePtr = lastNote = 0;
						for (nc = 0; Notes2Chords[nc] != 0; nc++) {
							TempEvent.note = Notes2Chords[nc];
							NewEvent();
						}						
					}// end of else if (ActiveChannel != 9) {// if (chord)
					qsort(Event, e, sizeof(Event[0]), Compare);
					Resort();
					SaveEvents();
					InvalidateRect(hwnd, &rect, FALSE);
					UpdateWindow(hwnd);
					GetTicksInNote();// get TempEvent.ticksinnote
					timeBeginPeriod(TIMER_RESOLUTION);
					uTimer9ID = timeSetEvent((UINT)(dMilliSecondsPerTick * (double)TempEvent.ticksinnote), TIMER_RESOLUTION, ChordTimerFunc, 0, TIME_ONESHOT);
				}// end of if (x == (int)e) {// not covering another note
			}// end of else if (ChannelInstruments[ActiveChannel][0] < 128)
		}// end of else if ((xLoc < Lines[l].PixelsPerLine)
		else if (yLoc > (Row * PixelsInGrandStaffAndMyExtraSpace)) {// WM_LBUTTONDOWN
			if (DialogBox(hInst, "LYRIC", hwnd, LyricProc)) {
				if ((LyricLen) && ((dataptr+LyricLen) < 0xFFFF)) {
					x = saveZ % 5;
					if (x <= 2) saveZ -= x;// ticks/thirtysecond note = 60 (5 pixels)
					else saveZ += (5-x);
					Event[e].pixel = saveZ;
					Event[e].tickptr = saveZ * TicksPerQuarterNote / 40;
					Event[e].type = 5;
					Event[e].len = LyricLen;
					Event[e].ptr = dataptr;
					e++;
					for (x = 0; x < LyricLen; dataptr++, x++)
						Data[dataptr] = Lyric[x];
					qsort(Event, e, sizeof(Event[0]), Compare);
					Resort();
					SaveEvents();
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
		}
		return 0;

	case WM_LBUTTONUP: // Row from WM_MOUSEMOVE
		if ((!playing) && (!keyboardactive)) {
			xLoc = LOWORD(lParam);
			yLoc = HIWORD(lParam);
			chordnameflag = 0;
			if ((copying == COPYBUTTONDOWN)) {
				if ((xLoc < srcBeginX) && (Row == ButtonDownRow)) {
					highlighting = FALSE;
					copying = NOCOPY;
					ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
					DrawMenuBar(hwnd);
					return 0;
				}
				copying = COPYBUTTONUP;
				if ((xLoc < srcBeginX) && (Row == ButtonDownRow))
					return 0;
				srcEndX = xLoc;
				srcEndY = (Row * PixelsInGrandStaffAndMyExtraSpace) + PixelsInGrandStaffAndMyExtraSpace;
				ButtonUpRow = Row;

				l = (Page * Rows) + Row;
				z = xLoc + Lines[l].pixel;
				for (x = firstX, lastX = firstX; x < (int)e; x++) {
					if ((Event[x].note) && (Event[x].velocity == 0)) {
						if ((Event[x].pixel + Event[x].pixelsinnote) < z)
							lastX = x;
						else
							break;
					}
				}
				if (lastX == firstX) {// no note highlighted
					highlighting = FALSE;
					copying = NOCOPY;
					ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
					DrawMenuBar(hwnd);
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			else if ((copying == NOCOPY)) {
				if (sustainflag) {
					l = (Page * Rows) + Row;
					z = xLoc + Lines[l].pixel;
					if ((sustainflag == 1) && (z > BeginSustain)) {// creating
						itsbad = FALSE;
						for (x = 0; x < (int)e; x++) {
							if ((Event[x].message & 0x007FFFFF) == (DWORD)(0x007F40B0 | ActiveChannel)) {
								for (y = x+1; y < (int)e && (Event[y].message != (DWORD)(0x000040B0 | ActiveChannel)); y++)
									;
								if ((z >= Event[x].pixel) && (z <= Event[y].pixel))
									itsbad = TRUE;
								else if ((BeginSustain <= Event[x].pixel) && (z >= (Event[x].pixel + Event[x].pixelsinnote)))
									itsbad = TRUE;
							}
						}
						if (itsbad == FALSE) {
							Event[e].pixel = BeginSustain;
							Event[e].tickptr = Event[e].pixel * TicksPerQuarterNote / 40;
							Event[e].message = (0xB0|ActiveChannel) | (0x7F << 16) | (0x40 << 8);// sustain pedal down
							e++;
							Event[e].pixel = z;
							Event[e].tickptr = Event[e].pixel * TicksPerQuarterNote / 40;
							Event[e].message = (0xB0|ActiveChannel) | (0x40 << 8);// sustain pedal up
							e++;
						}
					}
					else if (sustainflag == 2) {// deleting
						for (x = 0; x < (int)e; x++) {
							if ((Event[x].message & 0x007FFFFF) == (DWORD)(0x007F40B0 | ActiveChannel)) {
								for (y = x+1; y < (int)e && (Event[y].message != (DWORD)(0x000040B0 | ActiveChannel)); y++)
									;
								if ((z >= Event[x].pixel) && (z <= Event[y].pixel)) {
									for ( ; y < (int)e; y++)
										Event[y] = Event[y+1];
									e--;
									for ( ; x < (int)e; x++)
										Event[x] = Event[x+1];
									e--;
								}
							}
						}
					}
					qsort(Event, e, sizeof(Event[0]), Compare);
					Resort();
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
					SaveEvents();// WM_LBUTTONUP
				} // end of if (sustainflag)
//chorus too steep when above middle c
//chorus thing added after ctrl-z when note entered
//chorus up goes down (under note?)
//ctrl-z sometimes erases notes along with effects
				else if ((Editing != -1) && (EditingBeginX != -1)) { // showingdynamics
					EditingEndX = xLoc;
					EditingEndY = yLoc;
					l = (Page * Rows) + Row;
					z = EditingBeginX + Lines[l].pixel;
					if (Editing < 5) {
						if ((Editing == 0) || (Editing == 1)) { // Volume or Expression
							if ((yLoc >= RowTop) && ((yLoc - RowTop) <= 127))
								volEnd = RowTop - yLoc + 127; // 127 at RowTop and 0 at RowTop - 127
							else
								volEnd = 0xFFFF; // flag
						}
						else if ((Editing >= 2) && (Editing <= 4)) { // Reverb, Chorus, or Modulation
							if ((yLoc < (RowTop + PixelsInGrandStaff)) && ((RowTop + PixelsInGrandStaff - yLoc) <= 127))
								volEnd = RowTop + PixelsInGrandStaff - yLoc;
							else
								volEnd = 0xFFFF;
						}
						if ((volBegin == 0xFFFF) || (volEnd == 0xFFFF)) {
							MessageBox(hwnd, "Either the beginning or the end\nis out of limits.\n\nYou need to see numbers next to\nthe cursor.", "Oops", MB_OK);
							return 0;
						}
						if (Editing <= 1) {// Volume or Expression
							if (Editing == 0)
								Controller = 7;
							else // if (Editing == 1)
								Controller = 11;
						}
						else if (Editing < 5) {// Reverb, Chorus, Mod
							if (Editing == 2)
								Controller = 91;
							else if (Editing == 3)
								Controller = 93;
							else if (Editing == 4)
								Controller = 1;
						}
						if (EditingEndX != EditingBeginX) { // showingdynamics
							d2 = (double)(volEnd-volBegin);
							d3 = (double)(EditingEndX-EditingBeginX);
							alpha = tan(d2 / d3); // alpha = tan(opposite side / adjacent side)
							if (((d2 > 0) && (alpha > 0.0)) || ((d2 < 0) && (alpha < 0.0))) { // not too steep
								for (x = 10, vol = volBegin; z <= (EditingEndX + Lines[l].pixel); x += 10, z += 10) {// put a new entry every 10 pixels
									Event[e].pixel = z;
									Event[e].tickptr = z * TicksPerQuarterNote / 40;
									Event[e].message = (0xB0|ActiveChannel) | (Controller << 8) | (vol << 16);
									Event[e].channel = ActiveChannel;
									Event[e].port = 0;
									e++;
									d = (double)x * atan(alpha); // adjacent side = opposide side * atan(alpha);
									vol = volBegin + (int)d;
									if (vol > 127)
										vol = 127;
									else if (vol < 0)
										vol = 0;
								}
							}
							else
								MessageBox(hwnd, "Too Steep", "", MB_OK);
						}
						else {
							Event[e].pixel = z;
							Event[e].tickptr = z * TicksPerQuarterNote / 40;
							Event[e].message = (0xB0|ActiveChannel) | (Controller << 8) | (vol << 16);
							Event[e].channel = ActiveChannel;
							Event[e].port = 0;
							e++;
						}
					}
					else if (Editing == 5) {// Pitch Bend
						volEnd = yLoc;
						if (EditingEndX != EditingBeginX) {
							d2 = (double)(EditingEndX-EditingBeginX);
							d3 = (double)(volEnd-volBegin);
							alpha = tan(d2 / d3); // alpha = tan(opposite side / adjacent side)
							if (((d3 > 0) && (alpha > 0.0)) || ((d3 < 0) && (alpha < 0.0))) { // not steep enough
								for (x = 2, vol = volBegin; z <= (EditingEndX + Lines[l].pixel); x += 2, z += 2) {// put a new entry every 2 pixels
									d = (double)x / atan(alpha); // adjacent side = opposide side * atan(alpha);
									vol = volBegin + (int)d;
									PitchBend = (MinPitchBend - vol) * 0x4000 / (PixelsBetweenLines * 20);
									if (PitchBend > 0xFFFF)// less than 0
										PitchBend = 0;
									else if (PitchBend > 16383)
										PitchBend = 16383;
									PitchBend1 = PitchBend & 0x7F;
									PitchBend2 = (WORD)((PitchBend << 1) & 0x7FFF);
									Event[e].pixel = z;
									Event[e].tickptr = z * TicksPerQuarterNote / 40;
									Event[e].message = (0xE0|ActiveChannel) | (PitchBend2 << 8) | (PitchBend1 << 8);
									Event[e].channel = ActiveChannel;
									Event[e].port = 0;
									e++;
								}
							}
							else
								MessageBox(hwnd, "Not Steep Enough", "", MB_OK);
						}
						else {
							PitchBend = (MinPitchBend - volEnd) * 0x4000 / (PixelsBetweenLines * 20); // range is from 10 below middle C to 10 above (0 - 0x4000)
							if (PitchBend > 0xFFFF)// less than 0
								PitchBend = 0;
							else if (PitchBend > 16383)
								PitchBend = 16383;
							PitchBend1 = PitchBend & 0x7F;
							PitchBend2 = (WORD)((PitchBend << 1) & 0x7FFF);
							Event[e].pixel = z;
							Event[e].tickptr = z * TicksPerQuarterNote / 40;
							Event[e].message = (0xE0|ActiveChannel) | (PitchBend2 << 8) | (PitchBend1 << 8);
							Event[e].channel = ActiveChannel;
							Event[e].port = 0;
							e++;
						}
					}
					else if (Editing == 6) { // Portamento (WM_LBUTTONUP)
							Event[e].pixel = z;
							Event[e].tickptr = z * TicksPerQuarterNote / 40;
							Event[e].message = (0xB0|ActiveChannel) | (0x41 << 8) | (0x7F << 0x10); // portamento on
							Event[e].channel = ActiveChannel;
							Event[e].port = 0;
							e++;
							z += 20;
							Event[e].pixel = z;
							Event[e].tickptr = z * TicksPerQuarterNote / 40;
							Event[e].message = (0xB0|ActiveChannel) | (0x41 << 8); // portamento off
							Event[e].channel = ActiveChannel;
							Event[e].port = 0;
							e++;

							for (x = 0; (x < (int)e) && (Event[x].pixel == 0); x++) {
								if (Event[x].message == (DWORD)(0x05B0|ActiveChannel))
									break; // already there
							}
							if (Event[x].pixel){
								for (y = e+1; y > x; y--)
									Event[y] = Event[y-1];
								e++;
								ZeroEvent(x);
								Event[x].message = (0x05B0|ActiveChannel);
								Event[x].channel = ActiveChannel;
								x++;
								for (y = e+1; y > x; y--)
									Event[y] = Event[y-1];
								e++;
								ZeroEvent(x);
								Event[x].message = (0x25B0|ActiveChannel);
								Event[x].channel = ActiveChannel;
							}
//						}
					}
					qsort(Event, e, sizeof(Event[0]), Compare);
					Resort();
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
					SaveEvents();// WM_LBUTTONUP
					EditingBeginX = -1;
					if (FALSE == showingdynamics)
						Editing = -1;
				}
			}
		}
//		return 0;
//		fall thru...

	case WM_MOUSEMOVE:
		if (shownotesplayed) {
			for (x = 0; x < (int)e; x++)
				Event[x].time = 0;
		}
		xLoc = LOWORD(lParam);
		yLoc = HIWORD(lParam);
		if (Rows == 1) {
			Row = 0;
			RowTop = Row1Top;
		}
		else if ((Rows == 2) && (yLoc < Row2Top)) {
			Row = 0;
			RowTop = Row1Top;
		}
		else if (Rows == 2) {
			Row = 1;
			RowTop = Row2Top;
		}
		else if ((Rows == 3) && (yLoc < Row2Top)) {
			Row = 0;
			RowTop = Row1Top;
		}
		else if ((Rows == 3) && (yLoc < Row3Top)) {
			Row = 1;
			RowTop = Row2Top;
		}
		else if (Rows == 3) {
			Row = 2;
			RowTop = Row3Top;
		}
		y = yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace);
		l = (Page * Rows) + Row;
		z = xLoc + Lines[l].pixel;
		for (x = 0; x < 88; x++)
			if (y >= NoteLoc[x])
				break;// got note
		ordinalNote = x + 21;// won't be a sharp or flat note
		Note = ordinalNote; // THIS IS WHERE Note	IS OBTAINED (NOT WM_LBUTTONDOWN OR WM_LBUTTONUP)
		OriginalNote = 0;// flag for VK_UP and VK_DOWN and VK_LEFT and VK_RIGHT

		ctrl = 0;
		if ((wParam == MK_CONTROL) || (wParam == (MK_LBUTTON | MK_CONTROL))) {// move or look for sharp/flat notes
			if ((usingsharp) && (Note < 108))
				Note++;
			else if (Note > 21)
				Note--;
			ctrl = 1;
		}

		if (chordnameflag == 1) {
			chordnameflag = 2;
			if ((yLoc > (PixelsInGrandStaff + RowTop)) || (yLoc < RowTop)) {
				chordnameflag = 0;
				return 0;
			}
			srcBeginX = xLoc;
			srcBeginY = yLoc;
			for (x = 0; x < 88; x++)
				if (y >= NoteLoc[x])
					break;// got note
			HighestNote = x + 21;// won't be a sharp or flat note
			l = (Page * Rows) + Row;
			beginE = xLoc + Lines[l].pixel;
			BitBlt(hdcMem2, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY); // save original screen
		}
		if (chordnameflag == 2) {
			hdc = GetDC(hwnd);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY); // get original screen
			MoveToEx(hdc, srcBeginX, srcBeginY, NULL);
			LineTo(hdc, srcBeginX, yLoc);
			LineTo(hdc, xLoc, yLoc);
			LineTo(hdc, xLoc, srcBeginY);
			LineTo(hdc, srcBeginX, srcBeginY);
			ReleaseDC(hwnd, hdc);
			return 0;
		}

		if (((copying == COPYBUTTONDOWN) || (copying == COPYBUTTONUP)) && (yLoc < ((Rows*BottomNoteTop)+34))) {
			if ((xLoc < srcBeginX) && (Row == ButtonDownRow)) {// can't move cursor left
				return 0;
			}
			else if (ButtonDownPage != Page) {
				highlighting = FALSE;
				copying = NOCOPY;
				ModifyMenu(hMenu, ID_HIGHLIGHT, MF_BYCOMMAND|MF_STRING, ID_HIGHLIGHT, Highlight);
				DrawMenuBar(hwnd);
				return 0;
			}
			hdc = GetDC(hwnd);
			if (ctrlA) {
				FillRect(hdcMem2, &rect, hBrush2);
			}
			else {
				BitBlt(hdcMem2, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
				////////////////////////////////
				if (copying == COPYBUTTONDOWN) {
					tempRow = Row;

					blueRect0.top = Row1Top;
					blueRect0.bottom = blueRect0.top + PixelsInGrandStaff;
					blueRect1.top = Row2Top;
					blueRect1.bottom = blueRect1.top + PixelsInGrandStaff;
					blueRect2.top = Row3Top;
					blueRect2.bottom = blueRect2.top + PixelsInGrandStaff;
					blueRect3.top = (PixelsInGrandStaff*3) + (MyExtraSpace*4);
					blueRect3.bottom = blueRect3.top + PixelsInGrandStaff;

					if (ButtonDownRow == 0)
						blueRect0.left = srcBeginX;
					else if (ButtonDownRow == 1)
						blueRect1.left = srcBeginX;
					else if (ButtonDownRow == 2)
						blueRect2.left = srcBeginX;
					else if (ButtonDownRow == 3)
						blueRect3.left = srcBeginX;

					if (Row == 0) {
						blueRect0.right = xLoc;
					}
					else if (Row == 1) {
						blueRect0.right = rect.right;
						blueRect1.right = xLoc;
					}
					else if (Row == 2) {
						blueRect0.right = rect.right;
						blueRect1.right = rect.right;
						blueRect2.right = xLoc;
					}
					else if (Row == 3) {
						blueRect0.right = rect.right;
						blueRect1.right = rect.right;
						blueRect2.right = rect.right;
						blueRect3.right = xLoc;
					}
				}
				else
					tempRow = ButtonUpRow;

				if (ButtonDownRow == 0) {
					FillRect(hdcMem2, &blueRect0, hBrush2);
					if (tempRow >= 1)
						FillRect(hdcMem2, &blueRect1, hBrush2);
					if (tempRow >= 2)
						FillRect(hdcMem2, &blueRect2, hBrush2);
					if (tempRow >= 3)
						FillRect(hdcMem2, &blueRect3, hBrush2);
				}
				else if (ButtonDownRow == 1) {
					FillRect(hdcMem2, &blueRect1, hBrush2);
					if (tempRow >= 2)
						FillRect(hdcMem2, &blueRect2, hBrush2);
					if (tempRow >= 3)
						FillRect(hdcMem2, &blueRect3, hBrush2);
				}
				else if (ButtonDownRow == 2) {
					FillRect(hdcMem2, &blueRect2, hBrush2);
					if (tempRow >= 3)
						FillRect(hdcMem2, &blueRect3, hBrush2);
				}
				else if (ButtonDownRow == 3)
					FillRect(hdcMem2, &blueRect3, hBrush2);
				////////////////////////////////
			}
			BitBlt(hdcMem2, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCAND);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
			ReleaseDC(hwnd, hdc);
			return 0;
		}

		else if (((shownameatpointer) || (showlist)) && (!playing) && (!keyboardactive)) {
			hdc = GetDC(hwnd);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
			SetBkMode(hdc, TRANSPARENT);
			if (showlist) {
				TextOut(hdc, rect.right >> 1, 0, _itoa(Note, temp, 10), lstrlen(temp));
				TextOut(hdc, (rect.right >> 1) - 50, 0, _itoa(z, temp2, 10), lstrlen(temp2));
			}
			if ((shownameatpointer) && (yLoc <= (PixelsInGrandStaff + RowTop)) && (yLoc >= RowTop)) {
				if ((ActiveChannel == 9) && (!EWQL)) {
					if ((Note >= 27) && (Note <= 87)) {// 61 percussion note limits
						GetTicksInNote();
						if (Letter[Note-21] == 0) {
							if (usingsharp) {
								NoteName[0] = Letter[Note-21-1];
								NoteName[1] = '#';
							}
							else {
								NoteName[0] = Letter[Note-21+1];
								NoteName[1] = 'b';
							}
							if (shownameatpointer)
								TextOut(hdc, xLoc-12, yLoc-8, NoteName, 2);
						}
						else if (!ctrl) {
							if (shownameatpointer)
								TextOut(hdc, xLoc-12, yLoc-8, &Letter[Note-21], 1);
						}
						else
							previousNote = Note;// trick to not play sound
					}
				}
				else {// ActiveChannel != 9
					GetTicksInNote();// get TempEvent.ticksinnote
					if ((ActiveInstrument < 128) && (Note >= 21) && (Note <= 108)) {// not 0xFF flag
						if (Letter[Note-21] == 0) {
							if (usingsharp) {
								NoteName[0] = Letter[Note-21-1];
								NoteName[1] = '#';
							}
							else {
								NoteName[0] = Letter[Note-21+1];
								NoteName[1] = 'b';
							}
							if (shownameatpointer)
								TextOut(hdc, xLoc-12, yLoc-8, NoteName, 2);
						}
						else if (!ctrl) {
							if (shownameatpointer)
								TextOut(hdc, xLoc-12, yLoc-8, &Letter[Note-21], 1);
						}
						else
							previousNote = Note;// trick to not play sound
					}
				}
			}
			SetBkMode(hdc, OPAQUE);
			ReleaseDC(hwnd, hdc);
		}// end of if ((showlist) && (!playing) && (!keyboardactive))

		if (show) { // show note parameters at top left
			overNote = FALSE;
			overnote = 0;
			xOffset = 0;
			hdc = GetDC(hwnd);
			for (x = 0; x < (int)e; x++) {
				if ((Event[x].velocity) && (z >= Event[x].pixel) && (z <= (Event[x].pixel + Event[x].pixelsinnote - 2)) && (ActiveChannels[Event[x].channel] == TRUE)) {// -2 is for notes next to each other
					if ((Event[x].note == Note) || ((!usingsharp) && ((!Letter[Note-1-21]) && (Event[x].note == Note-1))) || ((usingsharp) && ((!Letter[Note+1-21]) && (Event[x].note == Note+1)))) {
						overNote = TRUE;
						current = x;
						if (overnote != Note) {
							overnote = Note;
							BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
							xOffset = 0;
						}
						if ((Event[x].channel != 9) || (EWQL)) {
							//ChannelInstruments[channel][y] contains instrument change for y (instrument change is Program Change (0xC0))
							//  InstrumentOffset[channel][y] contains instrument change offset pixel for y
							// any note after change is changed instrument
							for (y = 0; (ChannelInstruments[Event[x].channel][y] != 0xFF) && (InstrumentOffset[Event[x].channel][y] <= Event[x].pixel) && (y < 16); y++)
								;
							thisInstrument = ChannelInstruments[(Event[x].channel)][y-1];
							if (thisInstrument < 128) {// not 0xFF flag
								SetBkColor(hdc, colors[Event[x].channel]);
								if (!EWQL)
									TextOut(hdc, xOffset, 15, &myInstruments[thisInstrument][5], strlen(myInstruments[thisInstrument])-5);
								else { // if (EWQL)
									for (y = 0, v = 0; v < thisInstrument; y++, v++) {
										for (w = y; (InstrumentBufs[Event[x].port][w] != '\r') && (InstrumentBufs[Event[x].port][w] != 0); w++)
											;
										y = w+1;
									}
									for (w = 0;(InstrumentBufs[Event[x].port][y] != '\r') && (InstrumentBufs[Event[x].port][y] != 0) && (InstrumentBufs[Event[x].port][y] != '('); w++, y++)
										TitleName[w] = InstrumentBufs[Event[x].port][y];
									TitleName[w] = 0;
									TextOut(hdc, xOffset, 15, TitleName, strlen(TitleName));
								}
								SetBkColor(hdc, WHITE);
							}
						}
						else if ((Note >= 27) && (Note <= 87)) {// percussion instrument
							SetBkColor(hdc, 0xD0D0D0);
							TextOut(hdc, xOffset, 15, Percussion[Event[x].note-27], lstrlen(Percussion[Event[x].note-27]));
							SetBkColor(hdc, WHITE);
						}
						else {
							SetBkColor(hdc, WHITE);
							ReleaseDC(hwnd, hdc);
							return 0;
						}
						SetBkMode(hdc, TRANSPARENT);
						FKey[1] = Letter[ordinalNote-21];
						if (Event[x].note == Note+1) {
							FKey[2] = '#';
							_itoa(ordinalNote+1, &FKey[5], 10);
						}
						else if (Event[x].note == Note-1) {
							FKey[2] = 'b';
							_itoa(ordinalNote-1, &FKey[5], 10);
						}
						else {
							FKey[2] = ' ';
							_itoa(ordinalNote, &FKey[5], 10);
						}
						y = 5 + strlen(&FKey[5]);
						FKey[y++] = ' ';
						FKey[y++] = 'F';
						_itoa(Event[x].channel+1, &FKey[y], 10);
						y += strlen(&FKey[y]);
						FKey[y++] = ')';
						if (blackbackground)
							SetTextColor(hdc, WHITE);
						TextOut(hdc, xOffset, 30, FKey, y);
						_itoa(Event[x].velocity, &text2[10], 10);
						TextOut(hdc, xOffset, 45, text2, lstrlen(text2));
						for (y = x; y >= 0; y--) {
							if (Event[y].dMilliSecondsPerTick) {
								d = 60000.0 / (Event[y].dMilliSecondsPerTick * (double)TicksPerQuarterNote);
								d2 = modf(d, &d3);// d2 is decimal part and d3 is integer part
								if (d2 > 0.50)
									d3++;
								BPM = (int)d3;
								_itoa(BPM, &text1[14], 10);
								TextOut(hdc, xOffset, 60, text1, lstrlen(text1));
								break;
							}
						}
						_itoa(Event[x].pixel, &text3[12], 10);
						TextOut(hdc, xOffset, 75, text3, lstrlen(text3));
						_itoa((Event[x].pixel + Event[x].pixelsinnote), &text4[10], 10);
						TextOut(hdc, xOffset, 90, text4, lstrlen(text4));
						SetBkMode(hdc, OPAQUE);
						xOffset += 120;
					}
				}
			}// end of for (x = 0; x < (int)e; x++)
			if ((xOffset == 0) && (!shownameatpointer) && (!showlist))// if no note info written
				BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);

			if (Editing != -1) { // Dynamics & Effects
				if ((Editing == 0) || (Editing == 1)) { // Volume or Expression
					if ((yLoc >= RowTop) && ((yLoc - RowTop) <= 127))
						vol = RowTop - yLoc + 127; // 127 at RowTop and 0 at RowTop - 127
					else
						vol = 0; // flag
				}
				else if ((Editing >= 2) && (Editing <= 4)) { // Reverb, Chorus, or Modulation
					if ((yLoc < (RowTop + PixelsInGrandStaff)) && ((RowTop + PixelsInGrandStaff - yLoc) <= 127))
						vol = RowTop + PixelsInGrandStaff - yLoc;
					else
						vol = 0;
				}
				else if (Editing == 5) { // Pitch Bend
					MinPitchBend = RowTop + (PixelsBetweenLines * 24);
					vol = (MinPitchBend - yLoc) * 0x4000 / (PixelsBetweenLines * 20); // range is from 10 lines below middle C to 10 above (0 - 0x4000)
					if ((vol >= 0x4000) || (vol < 0))
						vol = 0;
				}
				else
					vol = 0;
				if (vol) {
					if (vol < 128) {
						temp[0] = (vol / 100) + '0';
						if (temp[0] == '0')
							temp[0] = ' ';
						temp[1] = ((vol % 100) / 10) + '0';
						if ((temp[1] == '0') && (temp[0] == ' '))
							temp[1] = ' ';
						temp[2] = (vol % 10) + '0';
						SetBkMode(hdc, TRANSPARENT);
						TextOut(hdc, xLoc+10, yLoc-8, temp, 3); // Dynamics & Effects Volume (127 at high C)
						SetBkMode(hdc, OPAQUE);
					}
					else { // if Pitch Bend
						_itoa(vol, temp, 10);
						SetBkMode(hdc, TRANSPARENT);
						for (x = 0; temp[x] != 0; x++)
							;
						TextOut(hdc, xLoc-24, yLoc-8, temp, x); // Dynamics & Effects Volume (127 at high C)
						SetBkMode(hdc, OPAQUE);
					}
				}
			}
			BitBlt(hdcMem2, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY);
			ReleaseDC(hwnd, hdc);
		}// end of if (show)
		return 0;

	case WM_RBUTTONDOWN:
		if ((!playing) && (!keyboardactive)) {
			xLoc = LOWORD(lParam);
			yLoc = HIWORD(lParam);
			chordnameflag = 1;
			y = yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace);
			for (x = 0; x < 88; x++) {
				if (y >= NoteLoc[x]) {
					Note = x + 21;
					break;
				}
			}
			ctrl = 0;
			if (wParam == (MK_CONTROL|MK_RBUTTON)) {// Ctrl is down
				ctrl = 1;
				if (usingsharp)
					Note++;
				else
					Note--;
			}
			l = (Page * Rows) + Row;
			Pixel = xLoc + Lines[l].pixel;
			atnote = FALSE;
			for (x = 0; x < (int)e; x++) {
				if ((Event[x].velocity) && ((int)(Event[x].pixel+Event[x].pixelsinnote) >= Pixel) && ((int)Event[x].pixel <= Pixel) && (ActiveChannels[Event[x].channel] == TRUE)) {
					if ((Event[x].note == Note) || ((!usingsharp) && ((!Letter[Note-1-21]) && (Event[x].note == Note-1))) || ((usingsharp) && ((!Letter[Note+1-21]) && (Event[x].note == Note+1)))) {
						atnote = TRUE;
						beatsperminute = 0;
						for (y = x; y >= 0; y--) {
							if (Event[y].dMilliSecondsPerTick) {
								thisTickptr = Event[y].tickptr;
								d = 60000.0 / (Event[y].dMilliSecondsPerTick * (double)TicksPerQuarterNote);
								d2 = modf(d, &d3);// if d == 1.2, then d3 = 1.0 and d2 = 0.2
								if (d2 > 0.50)
									d3++;
								beatsperminute = (int)d3;
								_itoa(beatsperminute, textBPM, 10);// initializes ParametersProc, which might change it
								break;
							}
						}
						_itoa(Event[x].velocity, textVolume, 10);
						_itoa(Event[x].pixel, textStart, 10);
						_itoa(Event[x].pixel + Event[x].pixelsinnote, textEnd, 10);
						if ((Event[x].channel != 9) || (EWQL)) {
							for (y = 0; (ChannelInstruments[Event[x].channel][y] != 0xFF) && (InstrumentOffset[Event[x].channel][y] <= Event[x].pixel) && (y < 16); y++)
								;
							thisInstrument = ChannelInstruments[(Event[x].channel)][y-1];
						}
						else
							thisInstrument = 0xFF;// necessary because music read in might have a 0xC9 naming an instrument (like the trick instrument 40)
						thisX = x;
						if (DialogBox(hInst, "PARAMETERS", hwnd, ParametersProc)) {
							x = thisX;
							if (atoi(textBPM) <= 500)
								BPM = atoi(textBPM);
							else
								BPM = 500;
							if (BPM < 24)
								BPM = 24;
							if (beatsperminute != BPM) {// if new beatsperminute
								beatsperminute = BPM;
								mSecondsPerBeat = 60000000 / beatsperminute;
								for (y = x-1; y >= 0; y--) {
									if ((Event[y].pixel == Event[x].pixel) && (Event[y].dMilliSecondsPerTick)) {
										x = y;
										break;
									}
								}
								if (y < 0) {
									for (y = e; y >= x; y--)
										Event[y+1] = Event[y];// make room for a dMilliSecondsPerTick Event
									e++;
									thisX++;
								}
								Event[x].dMilliSecondsPerTick = (((double)mSecondsPerBeat / (double)TicksPerQuarterNote)) / 1000.0;
								Event[x].pixelsinnote = 0;
								Event[x].ticksinnote = 0;
								Event[x].message = 0;
								Event[x].note = 0;
								Event[x].velocity = 0;
								Event[x].sharporflat = 0;
								Event[x].channel = 0;
								Event[x].port = 0;
								Event[x].time = 0;
								x++;
							}
							if (Event[x].velocity != atoi(textVolume)) {
								Event[x].velocity = atoi(textVolume);
								if (Event[x].velocity > 127)
									Event[x].velocity = 127;
								else if (Event[x].velocity == 0)
									Event[x].velocity = 1;
							}

							if ((Event[x].pixel != (DWORD)start) || ((Event[x].pixel+Event[x].pixelsinnote) != (DWORD)end)) {
								TempEvent.pixel = start;
								TempEvent.pixelsinnote = end - start;
								TempEvent.tickptr = TempEvent.pixel * TicksPerQuarterNote / 40;
								TempEvent.ticksinnote = TempEvent.pixelsinnote * TicksPerQuarterNote / 40;
								TempEvent.channel = Event[x].channel;
								for (y = 1; y < (int)e; y++) {
									if ((y != x) && (Event[y].note == Event[x].note) && (Event[y].velocity) && (Event[y].channel == TempEvent.channel) && (Event[y].overlapped == Event[x].overlapped)) {
										if ((TempEvent.pixel >= Event[y].pixel) && (TempEvent.pixel < (Event[y].pixel + (Event[y].pixelsinnote))))
											break;
										else if ((TempEvent.pixel + (TempEvent.pixelsinnote) > Event[y].pixel) && (TempEvent.pixel <= Event[y].pixel))
											break;
									}
								}
								if (y == (int)e) {// not covering another note
									Event[x].pixel = TempEvent.pixel;
									Event[x].pixelsinnote = TempEvent.pixelsinnote;
									Event[x].tickptr = TempEvent.tickptr;
									Event[x].ticksinnote = TempEvent.ticksinnote;
									Event[x].channel = TempEvent.channel;
									for (y = x; y < (int)e; y++) {// change note end Event
										if ((Event[y].velocity == 0) && (Event[y].note == Event[x].note) && (Event[y].channel == Event[x].channel) && (Event[y].overlapped == Event[x].overlapped)) {
											Event[y].pixel = Event[x].pixel + Event[x].pixelsinnote;
											Event[y].tickptr = Event[y].pixel * TicksPerQuarterNote / 40;
											break;
										}
									}
								}
							}

							SaveEvents();
						}// end of if (DialogBox
						chordnameflag = 0;
						FillRect(hdcMem, &rect, hBrush);
						InvalidateRect(hwnd, &rect, FALSE);
						UpdateWindow(hwnd);
						x = thisX;
						// don't break because of possible multiple notes
					}// end of if ((Event[x].note == Note) && (Event[x].velocity) && (ActiveChannels[Event[x].channel] == TRUE))
				}// end of if ((Event[x].pixel+Event[x].pixelsinnote >= Pixel) ...
			}// end of for (x = 0; x < (int)e; x++) {

			if (atnote == FALSE) {// check if at measure bar
				l = line;
				for (x = PixelsInGrandStaffAndMyExtraSpace; yLoc > x; x += PixelsInGrandStaffAndMyExtraSpace)
					l++;
				for (x = 0, z = 0; x < Lines[l].PixelsPerLine; x += Lines[l].PixelsPerMeasure[z], z++) {
					if ((x >= (xLoc-3)) && (x <= (xLoc+3))) {// 3 for "close" -- x is at xLoc of measure bar
						chordnameflag = 0;
						saveX = x;// **** x is at xLoc of measure bar ****
						saveZ = z;// *** z is measure number at xLoc of measure bar ***
						oldBeatsPerMeasure = BeatsPerMeasure = Lines[l].BeatsPerMeasure[z]; // possibly changed in KeyTimeProc
						oldPixelsPerBeat = PixelsPerBeat = Lines[l].PixelsPerBeat[z]; // possibly changed in KeyTimeProc
						oldKeySignature = KeySignature = Lines[l].KeySignature[z]; // possibly changed in KeyTimeProc
						prevBeatNoteType = BeatNoteType;
						prevPixelsPerBeat = PixelsPerBeat;
						////
						if (DialogBox(hInst, "KEYTIME", hwnd, KeyTimeProc)) {
						////
							MeasureBarPixel = Lines[l].pixel + saveX;
							event = FALSE;
							for (x = 0; (x < (int)e) && (Event[x].pixel <= MeasureBarPixel); x++) {
								if (Event[x].pixel == MeasureBarPixel) {
									event = TRUE;// an Event is at the measure bar
									break;
								}
							}
							saveX2 = x;

							if (oldKeySignature != KeySignature) {
								for (z = saveZ, w = l; w < 300; w++) {
									for ( ; z < 64; z++) 
										Lines[w].KeySignature[z] = KeySignature;
									z = 0;
								}
								if (Event[x].pixel >= MeasureBarPixel) {
									for (v = x; v < (int)e; ) {
										if ((Event[v].KeySignature) && (Event[v].pixel != MeasureBarPixel)) {
											for (w = v; w < (int)e; w++)
												Event[w] = Event[w+1];// remove subsequent key signatures
											e--;
										}
										else
											v++;
									}

									for (y = x-1; y >= 0; y--) {
										if (Event[y].KeySignature) {
											if (Event[y].KeySignature == KeySignature) {
												for (y = x; y < (int)e; y++)
													Event[y] = Event[y+1];// remove subsequent key signatures
												e--;
												goto replacedkey;
											}
											break;
										}
									}

									if (event) {
										for ( ; Event[x].pixel == MeasureBarPixel; x--)
											;
										x++;
										for (y = x;Event[y].pixel == MeasureBarPixel; y++) {
											if (Event[y].KeySignature) {
												Event[y].KeySignature = KeySignature;
												goto replacedkey;
											}
										}
									}
									for (v = (int)e; v > x; v--)
										Event[v] = Event[v-1];// open a space for this time signature
								}
								Event[x].pixel = MeasureBarPixel;
								Event[x].pixelsinnote = 0;
								Event[x].tickptr = MeasureBarPixel * TicksPerQuarterNote / 40;
								Event[x].ticksinnote = 0;
								Event[x].message = 0;
								Event[x].time = 0;
								Event[x].dMilliSecondsPerTick = 0;
								Event[x].note = 0;
								Event[x].velocity = 0;
								Event[x].sharporflat = 0;
								Event[x].channel = 17;//flag to ignore Event[e] (except for midiOut) (it's not in ActiveChannels)
								Event[x].port = 0;
								Event[x].BeatsPerMeasure = 0;
								Event[x].BeatNoteType = 0;
								Event[x].KeySignature = KeySignature;
								Event[x].type = 0;
								Event[x].len = 0;
								Event[x].ptr = 0;
								e++;
							}
replacedkey:
							if ((oldBeatsPerMeasure != BeatsPerMeasure) || (oldPixelsPerBeat != PixelsPerBeat)) {
								x = saveX;
								z = saveZ;
								for (LinePixels = x; LinePixels <= (rect.right-PixelsPerMeasure); z++) {
									Lines[l].PixelsPerBeat[z] = PixelsPerBeat;
									Lines[l].BeatsPerMeasure[z] = BeatsPerMeasure;
									Lines[l].PixelsPerMeasure[z] = PixelsPerMeasure;
									LinePixels += PixelsPerMeasure;
								}
								Lines[l].PixelsPerLine = LinePixels;
								Lines[l].rowonpage = (l % Rows) * PixelsInGrandStaffAndMyExtraSpace;
								for (ThisLinePixels = 0; ThisLinePixels <= (rect.right-PixelsPerMeasure); ThisLinePixels += PixelsPerMeasure)
									;
								for (l++; l < 300; l++) {//put latest beat & beatnote in rest of Lines
									Lines[l].pixel = Lines[l-1].pixel + Lines[l-1].PixelsPerLine;
									Lines[l].PixelsPerLine = ThisLinePixels;
									Lines[l].rowonpage = (l % Rows) * PixelsInGrandStaffAndMyExtraSpace;
									for (z = 0; z < 64; z++) {
										Lines[l].PixelsPerBeat[z] =	PixelsPerBeat;
										Lines[l].BeatsPerMeasure[z] = BeatsPerMeasure;
										Lines[l].PixelsPerMeasure[z] = PixelsPerMeasure;
									}
								}
								for (l = 0; l < 300; ) {
									PixelsPerPage = 0;
									if (Lines[l].rowonpage == 0) {
										PixelsPerPage += Lines[l].PixelsPerLine;
										for (z = l+1; Lines[z].rowonpage != 0; z++)
											PixelsPerPage += Lines[z].PixelsPerLine;
										Lines[l].PixelsPerPage = PixelsPerPage;
										for (l++; Lines[l].rowonpage != 0; l++)
											Lines[l].PixelsPerPage = PixelsPerPage;
									}
								}
								x = saveX2;
								if (Event[x].pixel >= MeasureBarPixel) {
									for (v = x; v < (int)e; v++) {
										if ((Event[v].BeatsPerMeasure) && (Event[v].pixel != MeasureBarPixel)) {
											for (w = v; w < (int)e; w++)
												Event[w] = Event[w+1];// remove subsequent time signatures
											e--;
										}
									}
									if (event) {
										for (y = x-1; y >= 0; y--) {
											if (Event[y].BeatsPerMeasure) {
												if ((Event[y].BeatsPerMeasure == BeatsPerMeasure) && (Event[y].BeatNoteType == BeatNoteType)) {
													for (y = x; y < (int)e; y++)
														Event[y] = Event[y+1];// remove subsequent time signatures
													e--;
													goto replacedbeat;
												}
												break;
											}
										}
										gotit = FALSE;
										for ( ; Event[x].pixel == MeasureBarPixel; x--)
											;
										x++;
										for (y = x;Event[y].pixel == MeasureBarPixel; y++) {
											if (Event[y].BeatsPerMeasure) {
												Event[y].BeatsPerMeasure = BeatsPerMeasure;
												gotit = TRUE;
											}
											if (Event[y].BeatNoteType) {
												Event[y].BeatNoteType = BeatNoteType;
												gotit = TRUE;
											}
											if (gotit)
												goto replacedbeat;
										}
									}
									for (v = (int)e; v > x; v--)
										Event[v] = Event[v-1];// open a space for this beats/measure and beat note type 
								}
								Event[x].pixel = MeasureBarPixel;
								Event[x].pixelsinnote = 0;
								Event[x].tickptr = MeasureBarPixel * TicksPerQuarterNote / 40;
								Event[x].ticksinnote = 0;
								Event[x].message = 0;
								Event[x].time = 0;
								Event[x].dMilliSecondsPerTick = 0;
								Event[x].note = 0;
								Event[x].velocity = 0;
								Event[x].sharporflat = 0;
								Event[x].channel = 17;//flag to ignore Event[e] (except for midiOut) (it's not in ActiveChannels)
								Event[x].port = 0;
								Event[x].BeatsPerMeasure = BeatsPerMeasure;
								Event[x].BeatNoteType = BeatNoteType;
								Event[x].KeySignature = 0;
								Event[x].type = 0;
								Event[x].len = 0;
								Event[x].ptr = 0;
 								e++;
							}
replacedbeat:;
							if ((prevBeatNoteType != BeatNoteType) || (prevPixelsPerBeat != PixelsPerBeat)) {
								MeasureNumber = 0;
								for (l = 0; l < 300; l++) {
									for (x = 0, y = 0; x < Lines[l].PixelsPerLine; x += Lines[l].PixelsPerMeasure[y], y++)
										;
									MeasureNumber += y;
									Lines[l+1].FirstMeasureNumber = MeasureNumber;
								}
							}
							FillRect(hdcMem, &rect, hBrush);
							InvalidateRect(hwnd, &rect, FALSE);
						}// end of if (DialogBox
						chordnameflag = 0;
						x = saveX;
						z = saveZ;
						break;
					}// end of if ((x >= (xLoc-3)) && (x <= (xLoc+3)))
				}// end of for (x = 0, z = 0; x < Lines[l].PixelsPerLine; x += Lines[l].PixelsPerMeasure[z], z++)
			}// end of if (atnote == FALSE)
		}// end of if ((!playing) && (!keyboardactive) && (e > 4))
		return 0;

	case WM_RBUTTONUP:
		if ((playing) || (keyboardactive))
			return 0;
		xLoc = LOWORD(lParam);
		yLoc = HIWORD(lParam);
		if (chordnameflag == 1)
			return 0;
		if (chordnameflag == 2) {
			chordnameflag = 0;
			y = yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace);
			for (x = 0; x < 88; x++)
				if (y >= NoteLoc[x])
					break;// got note
			LowestNote = x + 21;// won't be a sharp or flat note
			if (LowestNote > HighestNote) {
				y = HighestNote;
				HighestNote = LowestNote;
				LowestNote = y;
			}
			l = (Page * Rows) + Row;
			z = xLoc + Lines[l].pixel;
			if (beginE > z) {
				y = z;
				z = beginE;
				beginE = y;
			}
			for (x = 0; x < 8; x++)
				ChordBuf[x] = 0;
			for (i = 0, name = 0, x = 0; (x < (int)e) && (i < 8); x++) {
				if ((((beginE >= Event[x].pixel) && (beginE <= (Event[x].pixel + Event[x].pixelsinnote)))
				 || ((z >= Event[x].pixel) && (z <= (Event[x].pixel + Event[x].pixelsinnote))
				 || ((beginE <= Event[x].pixel) && (z >= (Event[x].pixel + Event[x].pixelsinnote)))))) {
					if (((Event[x].channel != 9) || (EWQL)) && (Event[x].velocity) && (Event[x].note <= HighestNote) && (Event[x].note >= LowestNote)) {
						ChordBuf[i] = Event[x].note;
						y = ChordBuf[i]-21;
						NotesInBox[name] = Letter[y];
						if (NotesInBox[name])
							NotesInBox[name+1] = ' ';
						else
						{
							NotesInBox[name] = Letter[y+1];
							NotesInBox[name+1] = 'b';
						}
						name += 2;
						i++;
					}
				}
			}
			for (x = 0; x < TOTAL_CHORDS; x++) {
				GoodNumbersInChords[x] = 0;
				BadNumbersInChords[x] = 0;
			}
			for (x = 0; (x < 28) && (NotesInBox[x]); x += 2) {
				for (z = 0; z < TOTAL_CHORDS; z++) {
					for (y = 0; ChordNotes[z][y]; y += 2) {
						if (*(WORD*)&NotesInBox[x] == *(WORD*)&ChordNotes[z][y])
							GoodNumbersInChords[z]++;
						else
							BadNumbersInChords[z]++;
					}
				}
			}
			biggest = 0;
			for (x = TOTAL_CHORDS-1; x >= 0; x--) {
				if (GoodNumbersInChords[x] >= biggest) {
					biggest = GoodNumbersInChords[x];
				}
			}
			baddest = 0xFFFF;
			for (y = 0; y < TOTAL_CHORDS; y++) {
				if ((GoodNumbersInChords[y] == biggest) && (BadNumbersInChords[y] < baddest)) {
					baddest = BadNumbersInChords[y];
				}
			}
			if (biggest >= 2) {
				int X = 0;

				hdc = GetDC(hwnd);
				if (blackbackground) {
					SetBkColor(hdc, BLACK);
					SetTextColor(hdc, WHITE);
				}
				for (y = 0; y < TOTAL_CHORDS; y++) {
					if ((GoodNumbersInChords[y] == biggest) && (BadNumbersInChords[y] == baddest)) {
						TextOut(hdc, X, 0, ChordPtr[y], lstrlen(ChordPtr[y]));
						if ((ChordPtr[y][1] != 'b') && (ChordPtr[y][0] != 'C') && (ChordPtr[y][0] != 'F')) {
							for (x = 0; ChordNotes[y][x] != 0; x++)
								SharpChordNotes[x] = ChordNotes[y][x];
							SharpChordNotes[x] = 0;
							for (x = 3; x < 15; x += 2) {
								if (SharpChordNotes[x] == 'b') {
									SharpChordNotes[x] = '#';
									SharpChordNotes[x-1]--;
									if (SharpChordNotes[x-1] == '@')
										SharpChordNotes[x-1] = 'G';
								}
							}
							TextOut(hdc, X, 20, SharpChordNotes, lstrlen(SharpChordNotes));
						}
						else
							TextOut(hdc, X, 20, ChordNotes[y], lstrlen(ChordNotes[y]));
						X += 80;
					}
				}
				ReleaseDC(hwnd, hdc);
			}
			for (x = 0; x < 28; x++)
				NotesInBox[x] = 0;
		}
		return 0;

	case WM_USER:
		saveActiveChannel = ActiveChannel; // because of possible Assigned Instruments
		if (wParam == 0x80) {
			wParam = 0x90;
			lParam &= 0x00FF;// make velocity 0
		}
		if (wParam == (0x90 & 0xF0)) {//Note On or Off
			Note = lParam & 0xFF;
			if ((Note < 21) || (Note > 108)) { // || ((ActiveChannel == 9) && Note > 87)
				ActiveChannel = saveActiveChannel;
				return 0;
			}
			if (VirtualActiveChannel != 0xFF)
				ActiveChannel = VirtualActiveChannel; // for EWQL
			port = Port; // for EWQL
			RealNote = 0;
			for (x = 0; x < 8; x++) { // check for Assigned Instruments
				if ((Note >= AssignedInstruments[x].firstnote) && (Note <= AssignedInstruments[x].lastnote)) {
					RealNote = Note;
					ActiveChannel = AssignedInstruments[x].channel;
					port = AssignedInstruments[x].port; // port, as opposed to Port, is only used in WM_USER
					Note += (AssignedInstruments[x].octaveshift*12);
					break;
				}
			}
			Velocity = (lParam >> 8) & 0xFF;
			if (Velocity == 0) {// Note Off
				if ((0 == Sustenuto[Note]) || (!sustenutoDown)) {
					if (EWQL)
						midiOutShortMsg(hMidisOut[port], (0x90|ActiveChannel) | (Note << 8)); // different port for every 16 Instruments
					else
						midiOutShortMsg(hMidiOut, (0x90|ActiveChannel) | (Note << 8));// 0 Velocity
				}
				if (keyboardactive) {
					timeBeginPeriod(0);
					Event[e].time = timeGetTime();
					timeEndPeriod(0);
					Event[e].pixelsinnote = 0;
					Event[e].ticksinnote = 0;
					Event[e].note = Note;
					Event[e].velocity = 0;
					Event[e].channel = ActiveChannel;
					Event[e].port = port;
					Event[e].message = (0x90|ActiveChannel) | (Note << 8);
					for (x = e-1; x; x--) {// x will be at Note On Event
						if ((Event[x].velocity) && (Event[x].note == Event[e].note) && (Event[x].channel == Event[e].channel) && (Event[x].port == Event[e].port)) {
							y = (int)((double)((Event[e].time - Event[x].time) / dMilliSecondsPerTick));
							if (y >= ((320 * TicksPerQuarterNote / 40) - ((320 * TicksPerQuarterNote / 40)>>2)))
								notelen = 320 * TicksPerQuarterNote / 40;
							else if (y >= ((160*TicksPerQuarterNote/40) - ((160*TicksPerQuarterNote/40)>>2)))
								notelen = 160*TicksPerQuarterNote/40;
							else if (y >= ((80*TicksPerQuarterNote/40) - ((80*TicksPerQuarterNote/40)>>2)))
								notelen = 80*TicksPerQuarterNote/40;
							else if (y >= ((40*TicksPerQuarterNote/40) - ((40*TicksPerQuarterNote/40)>>2)))
								notelen = 40*TicksPerQuarterNote/40;
							else if (y >= ((20*TicksPerQuarterNote/40) - ((20*TicksPerQuarterNote/40)>>2)))
								notelen = 20*TicksPerQuarterNote/40;
							else if (y >= ((10*TicksPerQuarterNote/40) - ((10*TicksPerQuarterNote/40)>>2)))
								notelen = 10*TicksPerQuarterNote/40;
							else
								notelen = 5 * TicksPerQuarterNote / 40;
							oldTickptr = Event[x].tickptr;
							if (NoteLen) {// NoteLen is from RecordProc
								y = Event[x].tickptr;
								y %= NoteLen;
								if (y) {
									Event[x].tickptr -= y;
									if (y > (NoteLen>>1))
										Event[x].tickptr += NoteLen;
									Event[x].pixel = Event[x].tickptr * 40 / TicksPerQuarterNote;
								}
							}
							newTickptr = Event[x].tickptr;
							if ((Event[x].channel != 9) || (EWQL))
								Event[x].ticksinnote = notelen;
							else
								Event[x].ticksinnote = (5*TicksPerQuarterNote/40);
							Event[x].pixelsinnote = Event[x].ticksinnote * 40 / TicksPerQuarterNote;
							Event[e].tickptr = Event[x].tickptr + Event[x].ticksinnote;
							Event[e].pixel = Event[e].tickptr * 40 / TicksPerQuarterNote;//Event[x].pixel + Event[x].pixelsinnote;

							if ((e > 4) && (alterbeatperminute)) {
								originalMilliSecondsPerTick = 60000.0 / (double)(InitialBeatsPerMinute * TicksPerQuarterNote);
								for (z = x-1; z > 0; z--) {
									if ((Event[z].velocity) && (Event[z].tickptr < (Event[x].tickptr-4))) {// 4 is MagicNumber here
										for (v = z-1; v >= 0; v--)
											if (Event[v].dMilliSecondsPerTick)
												break;
										if (Event[v].tickptr >= (Event[x].tickptr-4))
											Event[v].dMilliSecondsPerTick = originalMilliSecondsPerTick * newTickptr / oldTickptr;
										else {
											e++;
											x++;
											for (w = e; w > (int)z; w--)// e hasn't been incremented yet
												Event[w] = Event[w-1];// make a space for new dMilliSecondsPerTick
											Event[z].dMilliSecondsPerTick = originalMilliSecondsPerTick * newTickptr / oldTickptr;
											Event[z].message = 0;
											Event[z].note = 0;
											Event[z].velocity = 0;
											Event[z].sharporflat = 0;
											Event[z].channel = 17;
											Event[z].time = 0;
											Event[z].BeatsPerMeasure = 0;
											Event[z].BeatNoteType = 0;
											Event[z].KeySignature = 0;
											Event[z].type = 0;
											Event[z].len = 0;
											Event[z].ptr = 0;
										}
										break;
									}
								}
							}// end of if ((e > 4) && (alterbeatperminute))
							break;
						}
					}
					e++;
					if (!othernotesplaying)
						InvalidateRect(hwnd, &rect, FALSE);// shows new notes (at end of possible composition (they might not show))
				}
				else {// if (!keyboardactive)
					if ((!shownotesplayed) || (e <= 4) || ((ActiveChannel == 9) && (!EWQL))) {
						hdc = GetDC(hwnd);
						BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY); // clear all notes
						for (x = 0; x < 64; x++) {
							if (TotalNotesOn[x]) {
								if (TotalNotesOn[x] != Note) {
									y = NoteLoc[TotalNotesOn[x]-21];
									if (!virtualkeyboard)
										y += Row * PixelsInGrandStaffAndMyExtraSpace;
									notebottom = (PixelsBetweenLines / 2);
									if ((!Letter[TotalNotesOn[x]-21]) && (usingsharp == FALSE))
										y -= NoteMiddle;
									if ((ActiveChannel != 9) || (EWQL)) {
										if (!onlynotenames) {
											GetTicksInNote();// get TempEvent.ticksinnote
											if ((!virtualkeyboard) && (yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace) > MyExtraSpace) && (yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace) < PixelsInGrandStaffAndMyExtraSpace))
												Rectangle(hdc, xLoc, y, xLoc+(TempEvent.ticksinnote*40/TicksPerQuarterNote), y+notebottom);
											else
												Rectangle(hdc, 480, y, 480+(TempEvent.ticksinnote*40/TicksPerQuarterNote), y+notebottom);
										}
									}
									else {
										SetBkMode(hdc, TRANSPARENT);
										TextOut(hdc, xLoc+8, y-6, Percussion[Note-27], lstrlen(Percussion[Note-27]));
										SetBkMode(hdc, OPAQUE);
									}
									if (((shownotenames) || (onlynotenames)) && (!shownotesplayed)) {
										NoteName[0] = Letter[TotalNotesOn[x]-21];
										NoteName[1] = ' ';
										NoteName[2] = ' ';
										if (NoteName[0] == 0) {
											if (usingsharp) {
												NoteName[0] = Letter[TotalNotesOn[x]-22];
												NoteName[1] = '#';
											}
											else {
												NoteName[0] = Letter[TotalNotesOn[x]-20];
												NoteName[1] = 'b';
											}
										}
										SetBkMode(hdc, TRANSPARENT);
										hOldFont = SelectObject(hdc, hNoteFont);
										if (onstaff) {
											if (onlynotenames)
												TextOut(hdc, xLoc, y-4, NoteName, 3);
											else
												TextOut(hdc, xLoc-15, y-4, NoteName, 3);
										}
										else {
											if (onlynotenames)
												TextOut(hdc, 480, y-4, NoteName, 3);
											else
												TextOut(hdc, 480-15, y-4, NoteName, 3);
										}
										SetBkMode(hdc, OPAQUE);
										SelectObject(hdc, hOldFont);
									}
								}
								else// if (TotalNotesOn[x] == Note)
									TotalNotesOn[x] = 0;
							}
						}
						ReleaseDC(hwnd, hdc);
					}
					if (virtualkeyboard) { // note off
						if (Letter[Note-21]) {
							hdc3 = GetDC(hwndVirtualKeyboard);
							hOldFont = SelectObject(hdc3, hFont);
							SetTextColor(hdc3, 0xFFFFFF); // WHITE to erase
							TextOut(hdc3, KeyLoc[Note-21]+(KeyWidth/4), rect3.top+94, &KeyLetter[KeyLoc[Note-21]/KeyWidth], 1);
							if (RealNote)
								TextOut(hdc3, RealKeyLoc[RealNote-21]+(KeyWidth/4), rect3.top+94, "x", 1);
							SelectObject(hdc3, hOldFont);
							ReleaseDC(hwndVirtualKeyboard, hdc3);
						}
						else {
							hdc3 = GetDC(hwndVirtualKeyboard);
							hBrush3 = CreateSolidBrush(BLACK);
							hOldBrush = SelectObject(hdc3, hBrush3);
							for (z = 0; z < 52; z++)	{
								if (Note == BlackKeyNotes[z]) {
									KeyLoc[Note-21] = ExtraSpace + (KeyWidth * z);
									Rectangle(hdc3, KeyLoc[Note-21]-(KeyWidth/3), rect3.top+20, KeyLoc[Note-21]+(KeyWidth/3), rect3.bottom-62);
									break;
								}
							}
							SelectObject(hdc3, hOldFont);
							ReleaseDC(hwndVirtualKeyboard, hdc3);
						}
					}
				}
			}
			else {// Note On (Velocity != 0)
				if ((sustenuto) && (FALSE == sustenutoDown))
					Sustenuto[Note] = 1; // note on before sustenuto pedal pressed
				if (!ConstantVelocity) {
					Velocity += AddedVelocity;
					if (Velocity > 127)
						Velocity = 127;
				}
				else {
					Velocity = ConstantVelocity;
				}
				if (EWQL)
					midiOutShortMsg(hMidisOut[port], (0x90|ActiveChannel) | (Velocity << 16) | (Note << 8)); // different port for every 16 Instruments
				else
					midiOutShortMsg(hMidiOut, (0x90|ActiveChannel) | (Velocity << 16) | (Note << 8));

				if (keyboardactive) {

					GetEventTickptr();

					Event[e].note = Note;
					if (!Letter[Event[e].note - 21])
						Event[e].sharporflat = 1;
					Event[e].velocity = Velocity;
					Event[e].message = (0x90|ActiveChannel) | (Velocity << 16) | (Note << 8);
////////////////////////////////////////////////////////
					if (othernotesplaying == TRUE) {
						x = Event[e].pixel - Lines[l].pixel; // l is changed in TimerFunc2
						y = (NoteLoc[Event[e].note - 21]) + Lines[l].rowonpage;
						notebottom = (PixelsBetweenLines / 2);
						if ((!Letter[Note-21]) && (usingsharp == FALSE))
							y -= NoteMiddle;
						hdc = GetDC(hwnd);
						Rectangle(hdc, x, y, x+20, y+notebottom);
						ReleaseDC(hwnd, hdc);
					}
////////////////////////////////////////////////////////
					e++;
				} // end of if (keyboardactive)

				else {// if (!keyboardactive)
					XLoc = -1;
					if ((shownotesplayed) && (e > 4) && ((ActiveChannel != 9) || (EWQL))) {
						L = (Page * Rows) + Row;
						CursorLoc = xLoc + Lines[L].pixel;
						for (x = 0; x < (int)e; x++)
							if ((Event[x].velocity) && (Event[x].pixel >= CursorLoc))
								break;// at first note after mouse pointer
						if (Event[PreviousWrongEvent].note == Note) {
							x = PreviousWrongEvent;
							PreviousWrongEvent = 0; // so it's not used again
							goto there;
						}
						y = -1;
						for ( ; (x < (int)e) && (Event[x].pixel < (Lines[L].pixel + Lines[L].PixelsPerPage)); x++) {
							if ((Event[x].velocity) && (Event[x].time == 0) && (Event[x].channel == ActiveChannel)) { //
								if (Event[x].note == Note) {
there:							for (z = x+1; z < e; z++)
										if (Event[z].velocity)
											break;
									if (z == e) {
										SetCursorPos(xLoc, yLoc+TitleAndMenu);// cursor position from WM_MOUSEMOVE (start again)
										for (x = 0; x < (int)e; x++)
											Event[x].time = 0;
									}
									else if (Event[z].pixel >= (Lines[Page*Rows].pixel + Lines[L].PixelsPerPage)) {
										if (y == -1) {// not wrong note
											SendMessage(hwnd, WM_KEYDOWN, VK_NEXT, 0);// turn page
											SetCursorPos(0, 248);
											break;
										}
									}
									if ((y != -1) && ((Event[x].pixel-Event[y].pixel) > 5)) {// WRONG NOTE!
										ShowUnplayedNote(y);
										ActiveChannel = saveActiveChannel;
										return 0;
									}
									else {// the right note
										PreviousWrongEvent = 0; // so it's not used again
										Event[x].time = 1;// flag to show played
										if (Event[x].pixel >= (Lines[L].pixel + Lines[L].PixelsPerLine)) {
											L++;
											Row++;
										}
										XLoc = (WORD)(Event[x].pixel - Lines[L].pixel);
										y = NoteLoc[Note-21];
										if (!virtualkeyboard)
											y += Row * PixelsInGrandStaffAndMyExtraSpace;
										if ((!Letter[Note-21]) && (usingsharp == FALSE))
											y -= NoteMiddle;
										notebottom = (PixelsBetweenLines / 2);
										hdc = GetDC(hwnd);
										Rectangle(hdc, XLoc, y, XLoc+(Event[x].ticksinnote*40/TicksPerQuarterNote), y+notebottom);
										ReleaseDC(hwnd, hdc);
										break;
									}
								}
								else if (y == -1) {
									y = x;
								}
							}
						}
						if ((x == (int)e) || (Event[x].pixel >= (Lines[L].pixel + Lines[L].PixelsPerPage))) {// WRONG NOTE!
							ShowUnplayedNote(y);
							ActiveChannel = saveActiveChannel;
							return 0;
						}
					} // end of if ((shownotesplayed) && (e > 4) && (ActiveChannel != 9))
					else {
						for (x = 0; x < 64; x++) {
							if (TotalNotesOn[x] == 0) {
								if (((ActiveChannel != 9) || (EWQL)) || ((Note >= 27) && (Note <= 87)))
									TotalNotesOn[x] = Note;// so notes left in TotalNotesOn won't be turned off with note off
								break;
							}
						}
					}
					y = NoteLoc[Note-21];
					if (!virtualkeyboard)
						y += Row * PixelsInGrandStaffAndMyExtraSpace;
					if (y < rect.bottom) {
						if ((!Letter[Note-21]) && (usingsharp == FALSE))
							y -= NoteMiddle;
						notebottom = (PixelsBetweenLines / 2);
						hdc = GetDC(hwnd);
						if ((ActiveChannel != 9) || (EWQL)) {
							if (!shownotesplayed) {
								GetTicksInNote();// get TempEvent.ticksinnote
 								if ((!virtualkeyboard) && (XLoc == -1) && (yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace) > MyExtraSpace) && (yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace) < PixelsInGrandStaffAndMyExtraSpace)) {
									onstaff = TRUE;
									if (!onlynotenames)
										Rectangle(hdc, xLoc, y, xLoc+(TempEvent.ticksinnote*40/TicksPerQuarterNote), y+notebottom);
								}
								else {
									onstaff = FALSE;
									if (!onlynotenames) {
										Rectangle(hdc, 480, y, 480+(TempEvent.ticksinnote*40/TicksPerQuarterNote), y+notebottom);
									}
								}
							}
						}
						else if ((Note >= 27) && (Note <= 87)) {
							SetBkMode(hdc, TRANSPARENT);
							TextOut(hdc, xLoc+8, y-6, Percussion[Note-27], lstrlen(Percussion[Note-27]));
							SetBkMode(hdc, OPAQUE);
						}
					}
					if (((shownotenames) || (onlynotenames)) && (!shownotesplayed)) {
						NoteName[0] = Letter[Note-21];
						NoteName[1] = ' ';
						NoteName[2] = ' ';
						if (NoteName[0] == 0) {
							if (usingsharp) {
								NoteName[0] = Letter[Note-22];
								NoteName[1] = '#';
							}
							else {
								NoteName[0] = Letter[Note-20];
								NoteName[1] = 'b';
							}
						}
						SetBkMode(hdc, TRANSPARENT);
						hOldFont = SelectObject(hdc, hNoteFont);
						if ((onstaff) || ((ActiveChannel == 9) && (!EWQL))) {
							if (onlynotenames)
								TextOut(hdc, xLoc, y-3, NoteName, 3);
							else
								TextOut(hdc, xLoc-15, y-3, NoteName, 3);
						}
						else {
							if (onlynotenames)
								TextOut(hdc, 480, y-3, NoteName, 3);
							else
								TextOut(hdc, 480-15, y-3, NoteName, 3);
						}
						SetBkMode(hdc, OPAQUE);
						SelectObject(hdc, hOldFont);
					}
					else if (showvolumes) {
						if (Velocity / 100) {
							NoteName[0] = (Velocity / 100) + '0';
							NoteName[1] = ((Velocity % 100) / 10) + '0';
							NoteName[2] = (Velocity % 10) + '0';
							z = 3;
						}
						else if (Velocity / 10) {
							NoteName[0] = (Velocity / 10) + '0';
							NoteName[1] = (Velocity % 10) + '0';
							z = 2;
						}
						else {
							NoteName[0] = (Velocity % 10) + '0';
							z = 1;
						}
						hOldFont = SelectObject(hdc, hSmallFont);
						SetBkMode(hdc, TRANSPARENT);
						if ((onstaff) || ((ActiveChannel == 9) && (!EWQL)))
							TextOut(hdc, xLoc-20, y-4, NoteName, z);
						else
							TextOut(hdc, 480-20, y-4, NoteName, z);
						SetBkMode(hdc, OPAQUE);
						SelectObject(hdc, hOldFont);
					}
					else if (shownumbers) {
						if (Note / 100) {
							NoteName[0] = (Note / 100) + '0';
							NoteName[1] = ((Note % 100) / 10) + '0';
							NoteName[2] = (Note % 10) + '0';
							z = 3;
						}
						else { // if (Note / 10)
							NoteName[0] = (Note / 10) + '0';
							NoteName[1] = (Note % 10) + '0';
							z = 2;
						}
						hOldFont = SelectObject(hdc, hSmallFont);
						SetBkMode(hdc, TRANSPARENT);
						if ((onstaff) || ((ActiveChannel == 9) && (!EWQL)))
							TextOut(hdc, xLoc-20, y-4, NoteName, z);
						else
							TextOut(hdc, 480-20, y-4, NoteName, z);
						SetBkMode(hdc, OPAQUE);
						SelectObject(hdc, hOldFont);
					}
					ReleaseDC(hwnd, hdc);

					if (virtualkeyboard) { // note on
						if (Letter[Note-21]) {
							for (z = 0; (z < 52) && (Note != WhiteKeyNotes[z]); z++)
								;
							KeyLoc[Note-21] = (z * KeyWidth) + ExtraSpace;
							hdc3 = GetDC(hwndVirtualKeyboard);
							hOldFont = SelectObject(hdc3, hFont);
							TextOut(hdc3, KeyLoc[Note-21]+(KeyWidth/4), rect3.top+94, &KeyLetter[z], 1);
							if (RealNote) {
								for (z = 0; (z < 52) && (RealNote != WhiteKeyNotes[z]); z++)
									;
								RealKeyLoc[RealNote-21] = (z * KeyWidth) + ExtraSpace;
								TextOut(hdc3, RealKeyLoc[RealNote-21]+(KeyWidth/4), rect3.top+94, "x", 1);
							}
							SelectObject(hdc3, hOldFont);
							ReleaseDC(hwndVirtualKeyboard, hdc3);
						}
						else {
							for (z = 0; (z < 52) && (Note != BlackKeyNotes[z]); z++)
								;
							KeyLoc[Note-21] = (z * KeyWidth) + ExtraSpace;
							hdc3 = GetDC(hwndVirtualKeyboard);
							hOldFont = SelectObject(hdc3, hFont);
							SetBkMode(hdc3, TRANSPARENT);
							SetTextColor(hdc3, 0xFFFFFF);
							if (usingsharp) {
								TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+50, &KeyLetter[z-1], 1);
								TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+66, "#", 1);
							}
							else {
								TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+50, &KeyLetter[z], 1);
								TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+66, "b", 1);
							}
							SetTextColor(hdc3, 0);
							SetBkMode(hdc3, OPAQUE);
							SelectObject(hdc3, hOldFont);
							ReleaseDC(hwndVirtualKeyboard, hdc3);
						}
					}
				} // end of else if (!keyboardactive)
			} // end of else //Note On
		}// end of if (wParam == 0x90)

		else if (wParam == 0xB0) {
			if (keyboardactive) {

				GetEventTickptr();

				Event[e].message = (0xB0|ActiveChannel) | (lParam << 8);
				e++;
			}
			if (sustenuto) {
				if (lParam == 0x7F40)
					sustenutoDown = TRUE;
				else if (lParam == 0x0040) {
					sustenutoDown = FALSE;
					for (x = 21; x <= 108; x++)
						if (Sustenuto[x]) {
							Sustenuto[x] = 0;
							if (EWQL)
								midiOutShortMsg(hMidisOut[port], (0x90|ActiveChannel) | (x << 8)); // different port for every 16 Instruments
							else
								midiOutShortMsg(hMidiOut, (0x90|ActiveChannel) | (x << 8));// 0 Velocity
						}
				}
			}
			else {
				if (!EWQL)
					midiOutShortMsg(hMidiOut, (0xB0|ActiveChannel) | (lParam << 8)); // if sustain input, lParam is 0x7F40 or 0x0040
				else
					midiOutShortMsg(hMidisOut[Event[e-1].port], (0xB0|ActiveChannel) | (lParam << 8)); // if sustain input, lParam is 0x7F40 or 0x0040
			}
		}

		else if ((wParam == 0xE0) || (wParam == 0xA0)) {// Pitch Bend or Aftertouch
			if (keyboardactive) {

				GetEventTickptr();

				Event[e].message = (wParam|ActiveChannel) | (lParam << 8);// the Synthesizer will convert lParam to a 14 bit value
				e++;
			}
			midiOutShortMsg(hMidiOut, (wParam|ActiveChannel) | (lParam << 8));
		}
		ActiveChannel = saveActiveChannel;
		return 0;


	case WM_USER2:// from TimerFunc
		if (!done) {
			if (FirstPixelOnPage > Event[wParam].pixel) {
				return 0;// TimerFunc is behind...
			}
			if (Event[wParam].pixel  < (Lines[Page*Rows].pixel + Lines[line].PixelsPerPage)) {
				XXX = wParam;// for WaveBuf
				l = line;
				while (Event[wParam].pixel >= (Lines[l].pixel + Lines[l].PixelsPerLine))
					l++;
				X = Event[wParam].pixel - Lines[l].pixel;
				Y = Lines[l].rowonpage + MyExtraSpace - NoteMiddle;
				hdc = GetDC(hwnd);
				if (!overNote)
					BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
				else
					BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
				hOldPen = SelectObject(hdc, hPen);
				MoveToEx(hdc, X, Y, NULL);
				LineTo(hdc, X, Y + PixelsInGrandStaff);// vertical line
				SelectObject(hdc, hOldPen);
				ReleaseDC(hwnd, hdc);
			}
			else {// new page
				overNote = FALSE;
				TopLeftPtr = ptr;
				FirstPixelOnPage = Event[TopLeftPtr].pixel;
				PageBegin += lastRowLoc;
				Page++;
				_itoa(Page+1, &page[5], 10);
				line = Page * Rows;
				FillRect(hdcMem, &rect, hBrush);
				InvalidateRect(hwnd, &rect, FALSE);
			}
			if (virtualkeyboard) {
				Note = Event[wParam].note;
				NotesOn[no] = Note;
				if (no < 63)
					no++;
				if (Letter[Note-21]) {
					for (z = 0; (z < 52) && (Note != WhiteKeyNotes[z]); z++)
						;
					KeyLoc[Note-21] = (z * KeyWidth) + ExtraSpace;
					hdc3 = GetDC(hwndVirtualKeyboard);
					hOldFont = SelectObject(hdc3, hFont);
					TextOut(hdc3, KeyLoc[Note-21]+(KeyWidth/4), rect3.top+92, &KeyLetter[z], 1);
					SelectObject(hdc3, hOldFont);
					ReleaseDC(hwndVirtualKeyboard, hdc3);
				}
				else {
					for (z = 0; (z < 52) && (Note != BlackKeyNotes[z]); z++)
						;
					KeyLoc[Note-21] = (z * KeyWidth) + ExtraSpace;
					hdc3 = GetDC(hwndVirtualKeyboard);
//					hBrush3 = CreateSolidBrush(BLUE);
//					hOldBrush = SelectObject(hdc3, hBrush3);
//					Rectangle(hdc3, KeyLoc[Note-21]-(KeyWidth/3), rect3.top+20, KeyLoc[Note-21]+(KeyWidth/3), rect3.bottom-62);
//					SelectObject(hdc3, hOldBrush);
					hOldFont = SelectObject(hdc3, hFont);
					SetBkMode(hdc3, TRANSPARENT);
					SetTextColor(hdc3, 0xFFFFFF);
					if (usingsharp) {
//						TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+50, &KeyLetter[KeyLoc[Note-21]/KeyWidth-1], 1);
						TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+50, &KeyLetter[z], 1);
						TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+66, "#", 1);
					}
					else {
//						TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+50, &KeyLetter[KeyLoc[Note-21]/KeyWidth], 1);
						TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+50, &KeyLetter[z], 1);
						TextOut(hdc3, KeyLoc[Note-21]-(KeyWidth/3)+3, rect3.top+66, "b", 1);
					}
					SetTextColor(hdc3, 0);
					SetBkMode(hdc3, OPAQUE);
					SelectObject(hdc3, hOldFont);
					ReleaseDC(hwndVirtualKeyboard, hdc3);
				}
			}
		}
		return 0;

	case WM_USER3:// to stop playing
		if (keyboardactive) { // recording while existing music was playing
			for (x = timePtr-1; x; x--) {
				if (Event[x].velocity)
					break;
			}
			Tickptr = Event[x].tickptr - (Event[x].tickptr % TicksPerQuarterNote) + (TicksPerQuarterNote*2);
			timePtr = 0;
			counter = 1;// to get Startime
			milliSecondsPerBeat = (DWORD)(dMilliSecondsPerTick * (double)TicksPerQuarterNote);
			timeBeginPeriod(TIMER_RESOLUTION);// for Metronome
			uTimer2ID = timeSetEvent(milliSecondsPerBeat, TIMER_RESOLUTION, TimerFunc2, 0, TIME_PERIODIC);
		}
		if (recordingtowave) {
			recordingtowave = FALSE;
			if (done) { // from TimerFunc
				done = FALSE;
				do {
					mmTime.wType = TIME_SAMPLES;
					waveInGetPosition(hWaveIn, &mmTime, sizeof(mmTime));
				} while (mmTime.u.sample < (BufferSize / (nRecordChannels * BytesPerSample)));
			}
			else {
				mmTime.wType = TIME_SAMPLES;
				waveInGetPosition(hWaveIn, &mmTime, sizeof(mmTime));
				BufferSize = mmTime.u.sample * nRecordChannels * BytesPerSample;
			}
			waveInUnprepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
			waveInReset(hWaveIn);
			waveInClose(hWaveIn); // this will activate waveInProc, which will active WM_USER6
			done = TRUE;
		}
		if (playingwave) {
			playingwave = FALSE;
			waveOutReset(hWaveOut);
			if (playing2wavefiles) {
				playing2wavefiles = FALSE;
				waveOutReset(hWaveOut2);
			}
		}
		othernotesplaying = FALSE;
/*
FLAGS: showingstop, done, playing
playflag = PLAYING, PAUSED, or STOPPED
if Play pressed,
	if (!stopshowing)
		insert STOP
	modify Play to PAUSE
if PAUSE pressed
	save info
	modify PAUSE to PLAY
if PLAY pressed
	get info
	if (!stopshowing)
		insert STOP
	modify PLAY to PAUSE
if STOP pressed (see WM_COMMAND)
	delete STOP
	modify PAUSE to Play
*/
		if (playing) {
			playing = FALSE;
			if ((!done) && (!fromP)) {
				playflag = PAUSED;
				ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, ResumePlay);
				savetimePtr = timePtr;
			}
			else {
				savetimePtr = 0;
				ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Play);
//				if (done) {
					if (showingstop) {
						showingstop = FALSE;
						RemoveMenu(hMenu, 9, MF_BYPOSITION);
					}
					playflag = STOPPED;
//				}
			}
			DrawMenuBar(hwnd);
		}
		if (uTimerID) { // should never be NULL!
			done = TRUE;// to not change line in WM_USER2
			timeKillEvent(uTimerID);
			uTimerID = 0;
			timeEndPeriod(TIMER_RESOLUTION);
			if (!EWQL) {
				for (x = 0; x < 16; x++) {
					midiOutShortMsg(hMidiOut, 0x40B0|x); // sustain off (because Kontakt needs it)
					midiOutShortMsg(hMidiOut, (0xB0|x) | (121 << 8));// all controllers off
				}
			}
			else {
				for (y = 0; y < NumberOfPorts; y++) {
					for (x = 0; x < 16; x++) {
						midiOutShortMsg(hMidisOut[y], 0x40B0|x); // sustain off (because Kontakt needs it)
						midiOutShortMsg(hMidisOut[y], (0xB0|x) | (121 << 8));// all controllers off
					}
				}
			}

//			if (keyboardactive) { // recording while existing music was playing
//				for (x = timePtr-1; x; x--) {
//					if (Event[x].velocity)
//						break;
//				}
//				Tickptr = Event[x].tickptr - (Event[x].tickptr % TicksPerQuarterNote) + (TicksPerQuarterNote*2);
//				timePtr = 0;
//				counter = 1;// to get Startime
//				milliSecondsPerBeat = (DWORD)(dMilliSecondsPerTick * (double)TicksPerQuarterNote);
//				timeBeginPeriod(TIMER_RESOLUTION);// for Metronome
//				uTimer2ID = timeSetEvent(milliSecondsPerBeat, TIMER_RESOLUTION, TimerFunc2, 0, TIME_PERIODIC);
//				return 0;
//			}
			if (virtualkeyboard) {
				for (x = 0; (x < 64) && (x < no); x++) {
					Note = NotesOn[x];
					if (Letter[Note-21]) {
						for (z = 0; z < 52; z++)	{
							if (Note == WhiteKeyNotes[z])
								KeyLoc[Note-21] = ExtraSpace + (KeyWidth * z);
						}
						hdc3 = GetDC(hwndVirtualKeyboard);
						hOldFont = SelectObject(hdc3, hFont);
						SetTextColor(hdc3, 0xFFFFFF); // WHITE to erase
						TextOut(hdc3, KeyLoc[Note-21]+(KeyWidth/4), rect3.top+92, &KeyLetter[z], 1);
						SelectObject(hdc3, hOldFont);
						ReleaseDC(hwndVirtualKeyboard, hdc3);
					}
					else {
						hdc3 = GetDC(hwndVirtualKeyboard);
						hBrush3 = CreateSolidBrush(BLACK);
						hOldBrush = SelectObject(hdc3, hBrush3);
						for (z = 0; z < 52; z++)	{
							if (Note == BlackKeyNotes[z]) {
								KeyLoc[Note-21] = ExtraSpace + (KeyWidth * z);
								Rectangle(hdc3, KeyLoc[Note-21]-(KeyWidth/3), rect3.top+20, KeyLoc[Note-21]+(KeyWidth/3), rect3.bottom-62);
								break;
							}
						}
						SelectObject(hdc3, hOldFont);
						ReleaseDC(hwndVirtualKeyboard, hdc3);
					}
				}
				no = 0;
			}
		}
		timePtr = 0;

		if (!keyboardactive)
			StopTimers();
		if ((playflag != PAUSED) && (PlayList[playlist+1][0])) {
			playlist++;
			PlayPlayListProc();
		}
		else {
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
		}
		return 0;

	case WM_USER5:
		fromuser = TRUE;
		InvalidateRect(hwndLoop, &loopRect, FALSE);
		return 0;

	case WM_USER6: // from waveInProc after waveInClose
		timeKillEvent(uTimerElevenID);
		timeEndPeriod(TIMER_RESOLUTION);
		uTimerElevenID = 0;

		dMilliSeconds = prevMilliSecondsPer = 0.0;
		prevTickptr = 0;
		if (XXX) { // otherwise BufferSize comes from waveInGetPosition
			for (x = 0; x < (int)XXX; x++) {
				if (Event[x].dMilliSecondsPerTick) {
					dMilliSeconds += (prevMilliSecondsPer * (double)(Event[x].tickptr - prevTickptr));
					prevMilliSecondsPer = Event[x].dMilliSecondsPerTick;
					prevTickptr = Event[x].tickptr;
				}
			}
			dMilliSeconds += (prevMilliSecondsPer * (double)(Event[XXX-1].tickptr - prevTickptr));
			if (wBitsPerSample != 24)
				d = dMilliSeconds * (double)(nSamplesPerSec * nRecordChannels * 2) / 1000.0;
			else
				d = dMilliSeconds * (double)(nSamplesPerSec * nRecordChannels * 3) / 1000.0;
			d2 = modf(d, &d3);
			BufferSize = (int)d3 + (BLOCK_SIZE * 12); // for unknown reason
			if (d2 > 0.1)
				BufferSize += 2;
		}
		if (WaveFormat.Format.wFormatTag == WAVE_FORMAT_PCM)
			*(WORD*)&WaveHeader[16] = 16;
		else if (WaveFormat.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE)
			*(WORD*)&WaveHeader[16] = 40;
		*(WORD*)&WaveHeader[20] = WaveFormat.Format.wFormatTag;
		*(WORD*)&WaveHeader[22] = nRecordChannels;
		*(DWORD*)&WaveHeader[24] = nSamplesPerSec;
		*(DWORD*)&WaveHeader[28] = WaveFormat.Format.nAvgBytesPerSec; // byte rate
		*(WORD*)&WaveHeader[32] = WaveFormat.Format.nBlockAlign;
		*(WORD*)&WaveHeader[34] = WaveFormat.Format.wBitsPerSample;
		if (WaveFormat.Format.wFormatTag == WAVE_FORMAT_PCM) {
			*(DWORD*)&WaveHeader[36] = 0x61746164; // 0x64,0x61,0x74,0x61 "data"
			*(DWORD*)&WaveHeader[40] = BufferSize;
			*(DWORD*)&WaveHeader[4] = 36 + BufferSize;
			WaveHeaderSize = 44;
		}
		else {
			*(DWORD*)&WaveHeader[36] = 22; // size of extension
			*(DWORD*)&WaveHeader[38] = WaveFormat.Samples.wValidBitsPerSample;
			*(DWORD*)&WaveHeader[40] = WaveFormat.dwChannelMask; 
			for (x = 0, y = 44; x < 16; x++, y++)
				WaveHeader[y] = SubFormat[x];
			*(DWORD*)&WaveHeader[60] = 0x61746164; // 0x64,0x61,0x74,0x61 "data"
			*(DWORD*)&WaveHeader[64] = BufferSize;
			*(DWORD*)&WaveHeader[4] = 60 + BufferSize;
			WaveHeaderSize = 68;
		}
		FullFilename2[0] = 0;
		Filename[0] = 0;
		if (GetSaveFileName(&ofn2)) {
			int x = ofn2.nFileExtension;
			ofn2.lpstrFile[x] = 'w';
			ofn2.lpstrFile[x+1] = 'a';
			ofn2.lpstrFile[x+2] = 'v';
			ofn2.lpstrFile[x+3] = 0;
			hFile2 = CreateFile(FullFilename2, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			if (hFile2 != INVALID_HANDLE_VALUE) {
				WriteFile(hFile2, WaveHeader, WaveHeaderSize, &dwBytesWritten, NULL);
				WriteFile(hFile2, WaveBuf, BufferSize, &dwBytesWritten, NULL);
				CloseHandle(hFile2);
			}
		}
		recordedtowave = TRUE;
		if (WaveBuf)
			VirtualFree(WaveBuf, 0, MEM_RELEASE);
		WaveBuf = NULL;
		ClearMeter();
		SetFocus(hwndMixer);
		return 0;

	case WM_USER7: // from waveOutProc
		playingwave = FALSE;
		playing = FALSE;
		playflag = STOPPED;
		recordingtowave = FALSE;
		waveOutClose(hWaveOut);
		timeKillEvent(uTimerElevenID);
		timeEndPeriod(TIMER_RESOLUTION);
		uTimerElevenID = 0;
		ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Play);
		DrawMenuBar(hwnd);
		FillRect(hdcMem, &rect, hBrush);
		InvalidateRect(hwnd, &rect, FALSE);
		ClearMeter();
		GetWaveQuality();
		ShowQuality();
		SetFocus(hwndMixer);
		return 0;

	case WM_USER8: // if (virtualkeyboard), erase note name
		Note = Event[wParam].note;
		if (Letter[Note-21]) {
			for (z = 0; z < 52; z++)	{
				if (Note == WhiteKeyNotes[z])
					KeyLoc[Note-21] = ExtraSpace + (KeyWidth * z);
			}
			hdc3 = GetDC(hwndVirtualKeyboard);
			hOldFont = SelectObject(hdc3, hFont);
			SetTextColor(hdc3, 0xFFFFFF); // WHITE to erase
			TextOut(hdc3, KeyLoc[Note-21]+(KeyWidth/4), rect3.top+92, &KeyLetter[z], 1);
			SelectObject(hdc3, hOldFont);
			ReleaseDC(hwndVirtualKeyboard, hdc3);
		}
		else {
			hdc3 = GetDC(hwndVirtualKeyboard);
			hBrush3 = CreateSolidBrush(BLACK);
			hOldBrush = SelectObject(hdc3, hBrush3);
			for (z = 0; z < 52; z++)	{
				if (Note == BlackKeyNotes[z]) {
					KeyLoc[Note-21] = ExtraSpace + (KeyWidth * z);
					Rectangle(hdc3, KeyLoc[Note-21]-(KeyWidth/3), rect3.top+20, KeyLoc[Note-21]+(KeyWidth/3), rect3.bottom-62);
					break;
				}
			}
			SelectObject(hdc3, hOldFont);
			ReleaseDC(hwndVirtualKeyboard, hdc3);
		}
		return 0;

	case WM_USER10:
		waveOutClose(hWaveOut);
		fromeditmywave = FALSE;
		playingwave = FALSE;
		SetFocus(hwndEditMyWave);
		InvalidateRect(hwndEditMyWave, &rect, FALSE);
		return 0;

	case WM_USER11: // show record volume for 16 bits
		if (wavemixer) {
			dataptr = *(WORD*)&WaveBuf[wavePtr];
			if (dataptr > 32768)
				dataptr -= 32768;
			else
				dataptr = 32768 - dataptr;
			if (wavePtr > (BytesPer40MilliSeconds+wave0))
				prevwP = wavePtr-BytesPer40MilliSeconds;
			else
				prevwP = 0;
			for (wp = wavePtr; wp > prevwP; wp -= 2) {
				dp = *(WORD*)&WaveBuf[wp];
				if (dp > 32768)
					dp -= 32768;
				else
					dp = 32768 - dp;
				if (dataptr > dp)
					dataptr = dp;
			}
			d = (double)dataptr * dMultiplier;
			vert = (int)d;
			if (wavePtr < (int)BufferSize-BytesPer40MilliSeconds)
				wavePtr += BytesPer40MilliSeconds;
			InvalidateRect(hwndMeter, &meterRect, FALSE);
		}
		return 0;

	case WM_USER12: // show record volume for 24 bits
		if (wavemixer) {
			dataptr24 = *(DWORD*)&WaveBuf[wavePtr] & 0xFFFFFF; // make 24 bits out of 32 bits
			if (dataptr24 > 0x800000)
				dataptr24 -= 0x800000;
			else
				dataptr24 = 0x800000 - dataptr24;
			if (wavePtr > (BytesPer40MilliSeconds+wave0))
				prevwP = wavePtr-BytesPer40MilliSeconds;
			else
				prevwP = 0;
			for (wp = wavePtr; wp > prevwP; wp -= 3) {
				dp24 = *(DWORD*)&WaveBuf[wp] & 0xFFFFFF;
				if (dp24 > 0x800000)
					dp24 -= 0x800000;
				else
					dp24 = 0x800000 - dp24;
				if (dataptr24 > dp24)
					dataptr24 = dp24;
			}
			dataptr = (WORD)(dataptr24 >> 8); // slightly trick
			d = (double)dataptr * dMultiplier;
			vert = (int)d;
			if (wavePtr < (int)BufferSize-BytesPer40MilliSeconds)
				wavePtr += BytesPer40MilliSeconds;
			InvalidateRect(hwndMeter, &meterRect, FALSE);
		}
		return 0;

	case WM_USER13: // for Edit Wave routine
		hMenu3 = CreateMenu();//to override parent menu bar
		hwndEditMyWave = CreateWindow(EditMyWave, EditMyWave,// see EditMyWaveProc
			WS_POPUP | WS_VISIBLE,
			0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN),
			hwndMixer, hMenu3, hInst, NULL);
		DestroyMenu(hMenu);
		hMenu2 = LoadMenu(hInst, "MENU6");
		SetMenu(hwndEditMyWave, hMenu2);
		return 0;

	case WM_USER14:
		fromuser14 = TRUE;
		InvalidateRect(hwndEditMyWave, &rect, FALSE);
		return 0;
	

	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (blackbackground)
			PrevColor = WHITE;
		else
			PrevColor = BLACK;
		SetTextColor(hdcMem, PrevColor);

		if (first) {
			first = FALSE;
			//GetClientRect(hwnd, &rect);
			rect = ps.rcPaint;
			hdcMem = CreateCompatibleDC(hdc);
			hBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			SelectObject(hdcMem, hBitmap);
			hdcMem2 = CreateCompatibleDC(hdc);
			hBitmap2 = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
			SelectObject(hdcMem2, hBitmap2);
			GetTextExtentPoint32(hdcMem, RecordingToWave, strlen(RecordingToWave), &recordingSize);
			GetTextExtentPoint32(hdcMem, PlayingWave, strlen(PlayingWave), &playingSize);
			GetTextExtentPoint32(hdcMem, "M", 1, &size);// probably the widest of Advanced Options letters

			hDialogPen = CreatePen(PS_SOLID, 1, DIALOGCOLOR);// for MeterProc
			if (blackbackground)
				hPen = CreatePen(PS_SOLID, 1, WHITE);
			else
				hPen = CreatePen(PS_SOLID, 1, BLACK);
			hGreyPen = CreatePen(PS_SOLID, 1, 0xC0C0C0);
			hDarkGreyPen = CreatePen(PS_SOLID, 1, 0x99A8AC); // using WhatClr.exe to get one that works

			hBlackPen = CreatePen(PS_SOLID, 1, 0);
			hWhitePen = CreatePen(PS_SOLID, 1, 0xFFFFFF);

			hNullPen = CreatePen(PS_NULL, 0, 0);

			hRedPen = CreatePen(PS_SOLID, 1, RED);
			hRedSharpPen = CreatePen(PS_SOLID, 1, 0xD0);
			hRedFlatPen = CreatePen(PS_SOLID, 1, 0xA0A0FF);

			hOrangePen = CreatePen(PS_SOLID, 1, ORANGE);
			hOrangeSharpPen = CreatePen(PS_SOLID, 1, 0x0080FF);
			hOrangeFlatPen = CreatePen(PS_SOLID, 1, 0xB0E0FF);

			hYellowPen = CreatePen(PS_SOLID, 1, YELLOW);
			hYellowSharpPen = CreatePen(PS_SOLID, 1, 0xE8F0);
			hYellowFlatPen = CreatePen(PS_SOLID, 1, 0xC0FFFF);

			hGreenPen = CreatePen(PS_SOLID, 1, GREEN);
			hGreenSharpPen = CreatePen(PS_SOLID, 1, 0x20A020);
			hGreenFlatPen = CreatePen(PS_SOLID, 1, 0xB0FFD0);

			hBlueGreenPen = CreatePen(PS_SOLID, 1, BLUEGREEN);
			hBlueGreenSharpPen = CreatePen(PS_SOLID, 1, 0xA0A000);
			hBlueGreenFlatPen = CreatePen(PS_SOLID, 1, 0xF0F000);

			hBluePen = CreatePen(PS_SOLID, 1, BLUE);
			hBlueSharpPen = CreatePen(PS_SOLID, 1, 0xD00000);
			hBlueFlatPen = CreatePen(PS_SOLID, 1, 0xF0D0B0);

			hVioletPen = CreatePen(PS_SOLID, 1, VIOLET);
			hVioletSharpPen = CreatePen(PS_SOLID, 1, 0xC000C0);
			hVioletFlatPen = CreatePen(PS_SOLID, 1, 0xFFC0FF);

			hBrownPen = CreatePen(PS_SOLID, 1, BROWN);
			hBrownSharpPen = CreatePen(PS_SOLID, 1, 0x006890);
			hBrownFlatPen = CreatePen(PS_SOLID, 1, 0x98D4F0);

			hGreyNaturalPen = CreatePen(PS_SOLID, 1, GREY);
			hGreySharpPen = CreatePen(PS_SOLID, 1, 0x606060);
			hGreyFlatPen = CreatePen(PS_SOLID, 1, 0xE8E8E8);

			FillRect(hdcMem, &rect, hBrush);
			FillRect(hdcMem2, &rect, hBrush);

			Time = 0;
			originalE = 0;
			Filename[0] = 0;
			bt = 0;
			BigText[0] = 0;

			PageBegin = 0;
			FirstTopLeftPtr = 0;
			TopLeftPtr = 0;
			FirstPixelOnPage = 0;
			Page = 0;
			_itoa(Page+1, &page[5], 10);
			line = 0;

			ZeroEvents();
			Event[0].BeatsPerMeasure = 4;
			Event[0].BeatNoteType = 4;
			Event[1].KeySignature = 200;// C
			dMilliSecondsPerTick = 60000.0 / (double)(InitialBeatsPerMinute * TicksPerQuarterNote);
			Event[2].dMilliSecondsPerTick = dMilliSecondsPerTick;
			Event[3].message = 0xC0;
			Event[3].channel = 0;
			e = 4;
			for (u = 0; u < 10000; u++)
				LastEventInUndo[u] = -1;// flag
			pUndo = 0;
			Loop = -1;
			SaveEvents();// WM_PAINT
			////////////////////////
			GetPixelsBetweenLines();
			////////////////////////
//			if (hFile == INVALID_HANDLE_VALUE)// no .ini file
			if (!inifile)
				hwndHelp = CreateDialog(hInst, "HELP1", hwnd, HelpProc);

			hwndSound = CreateWindow(Sound, Sound,// see SoundProc
				WS_CHILD,
				0,0,0,0,
				hwnd, hMenu3, hInst, NULL);
			mixerOpen(&hMixer, 0, (DWORD)hwndSound, 0, MIXER_OBJECTF_HMIXER|CALLBACK_WINDOW);// open 0 id
			SendMessage(hwndSound, MM_MIXM_CONTROL_CHANGE, (WPARAM)hMixer, 0);// so SoundProc will get the current Master Volume (lParam is MixerControl.dwControlID)
		}//end of if (first)

		for (thisRow = 0, newY = 0, l = line; thisRow < Rows; thisRow++, l++) {
			SelectObject(hdcMem, hGreyPen);
			for (x = 0, z = 0, w = Lines[l].BeatsPerMeasure[z]; x < Lines[l].PixelsPerLine; x += Lines[l].PixelsPerBeat[z], w--) {
				if (w == 0) {
					z++;
					w = Lines[l].BeatsPerMeasure[z];
				}
				MoveToEx(hdcMem, x, newY + MyExtraSpace + (PixelsBetweenLines*9), NULL); // grey vertical lines (beat width)
				LineTo(hdcMem, x, newY + MyExtraSpace + (PixelsBetweenLines*19));
				for (v = 0, y = newY + MyExtraSpace; v < 9; v++, y += PixelsBetweenLines) {
					MoveToEx(hdcMem, x-4, y, NULL); // lines above and below staffs
					LineTo(hdcMem, x+4, y);
				}

				y += (PixelsBetweenLines * 5);
				MoveToEx(hdcMem, x-4, y, NULL);
				LineTo(hdcMem, x+4, y);
				for (v = 0, y += (PixelsBetweenLines * 6); v < 6; v++, y += PixelsBetweenLines) {
					MoveToEx(hdcMem, x-4, y, NULL);
					LineTo(hdcMem, x+4, y);
				}
				lastRowLoc = y;
			}
			SelectObject(hdcMem, hPen);
			for (x = 0, z = 0; x <= Lines[l].PixelsPerLine; x += Lines[l].PixelsPerMeasure[z], z++) {// measure bars
				MoveToEx(hdcMem, x, newY + MyExtraSpace + (PixelsBetweenLines*9), NULL);
				LineTo(hdcMem, x, newY + MyExtraSpace + (PixelsBetweenLines*19));
				if ((showmeasurenumber) && (x != Lines[l].PixelsPerLine)) {
					_itoa(Lines[l].FirstMeasureNumber+z+1, Number, 10);
					SetBkMode(hdcMem, TRANSPARENT);
					TextOut(hdcMem, x, newY + MyExtraSpace + (PixelsBetweenLines*9) - size.cy, Number, strlen(Number));
					SetBkMode(hdcMem, OPAQUE);
				}
			}
			for (v = 0, y = newY + MyExtraSpace + (PixelsBetweenLines * 9); v < 5; v++, y += PixelsBetweenLines) {
				MoveToEx(hdcMem, 0, y, NULL); // G clef staff lines
				LineTo(hdcMem, Lines[l].PixelsPerLine, y);
			}
			for (v = 0, y += PixelsBetweenLines; v < 5; v++, y += PixelsBetweenLines) {
				MoveToEx(hdcMem, 0, y, NULL); // F clef staff lines
				LineTo(hdcMem, Lines[l].PixelsPerLine, y);
			}
			newY += PixelsInGrandStaffAndMyExtraSpace;
		}

		overNote = FALSE;
		BitBlt(hdcMem2, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
		PixelsPerPage = Lines[line].PixelsPerLine * Rows;
		PixelsPerLine = Lines[line].PixelsPerLine;

//asdf
		if (TicksPerQuarterNote) {// draw all notes on page
			l = line; // line = Page*Rows (they both start at 0)
			LinesPageRowsPixel = Lines[Page*Rows].pixel;
			nco = 0;
			for (screenend = 0, x = line; x < (line+Rows); x++)
				screenend += Lines[x].PixelsPerLine;
			for (ptr = TopLeftPtr; (ptr < e) && (Event[ptr].pixel) < (FirstPixelOnPage + Lines[l].PixelsPerPage); ptr++) { // Lines[Page*Rows].pixel
				if ((Event[ptr].note >= 21) && (Event[ptr].note <= 108)) {

					Channel = Event[ptr].channel;
					if (ActiveChannels[Channel]) {
						if (Event[ptr].velocity) { // note on
							while (Event[ptr].pixel >= Lines[l].pixel + Lines[l].PixelsPerLine)
								l++; // to next line
							if (l >= (line + Rows))// e.g. if l goes to the next page
								break; // can happen if Lines[l].PixelsPerPage != sum of Lines[all l's on page].PixelsPerLine
							x = Event[ptr].pixel - Lines[l].pixel;
							notend = x + Event[ptr].pixelsinnote;
							y = (NoteLoc[Event[ptr].note - 21]) + Lines[l].rowonpage;
							notebottom = y + (PixelsBetweenLines / 2);
							sharporflat = Event[ptr].sharporflat;
							///////////////
							DrawNote(x, y);
							///////////////
							L = l;
							while ((notend > (int)Lines[L].PixelsPerLine) && ((L+1) < (line+Rows))) { // L+1 because L is incremented below
								L++;
								notend = Event[ptr].pixel + Event[ptr].pixelsinnote - Lines[L].pixel;
								x = 0;
								y = (NoteLoc[Event[ptr].note - 21]) + Lines[L].rowonpage;
								notebottom = y + (PixelsBetweenLines / 2);
								sharporflat = Event[ptr].sharporflat;
								///////////////
								DrawNote(x, y);
								///////////////
							}

							if (notend > Lines[L].PixelsPerLine) { // if note goes to next page (but hopefully not into page after that)
								for (nco = 0; (NoteCarryOver[Page+1][nco] != 0); nco++) {
									if (NoteCarryOver[Page+1][nco] == ptr) // copy over it
										break;
								}
								NoteCarryOver[Page+1][nco] = ptr;
							}

						}
					}
				}
			}// end of for (ptr = TopLeftPtr; (ptr < e) && ((Event[ptr].pixel) < (FirstPixelOnPage + Lines[l].PixelsPerPage)); ptr++)

			// show parts of notes that carry to next line
			nco = 0;
			while (NoteCarryOver[Page][nco]) {
				l = line;
				ptrNoteCarryOver = NoteCarryOver[Page][nco];
				if (Event[ptrNoteCarryOver].pixel < FirstPixelOnPage) {
					Channel = Event[ptrNoteCarryOver].channel;
					if (ActiveChannels[Channel]) {
						notend = Event[ptrNoteCarryOver].pixel + Event[ptrNoteCarryOver].pixelsinnote - Lines[l].pixel; // LinesPageRowsPixel
						do {
							if (notend > (int)(Lines[l].PixelsPerLine)) {
								l++;
								notend = Event[ptrNoteCarryOver].pixel + Event[ptrNoteCarryOver].pixelsinnote - Lines[l].pixel;
							}
							x = 0;
							y = (NoteLoc[Event[ptrNoteCarryOver].note - 21]) + Lines[l].rowonpage;
							notebottom = y + (PixelsBetweenLines / 2);
							sharporflat = Event[ptrNoteCarryOver].sharporflat;
							///////////////
							DrawNote(x, y);
							///////////////
						} while (notend > (int)(Lines[l].PixelsPerLine));
					}
				}
				else
					NoteCarryOver[Page][nco] = 0;
				nco++;
			}
			//

			l = line;

			for (ptr = TopLeftPtr; (ptr < e) && (Event[ptr].pixel) < (FirstPixelOnPage + Lines[l].PixelsPerPage); ptr++) { // Lines[Page*Rows].pixel
//			for (ptr = TopLeftPtr; (ptr < e) && ((Event[ptr].pixel) < (Lines[Page*Rows].pixel + Lines[l].PixelsPerPage)); ptr++) {
				while (Event[ptr].pixel >= (Lines[l].pixel + Lines[l].PixelsPerLine)) {
					l++;
				}
				if (l >= (line + Rows))// e.g. if l goes to the next page
					break;

				if (Event[ptr].type == 5) {
					hOldFont = SelectObject(hdcMem, hLyricFont);
					SetBkMode(hdcMem, TRANSPARENT);
					x = Event[ptr].pixel - Lines[l].pixel;
					y = PixelsInGrandStaffAndMyExtraSpace + Lines[l].rowonpage;
					z = Event[ptr].ptr;
					TextOut(hdcMem, x, y, &Data[z], Event[ptr].len);
					SetBkMode(hdcMem, OPAQUE);
					SelectObject(hdcMem, hOldFont);
				}

				if ((shownewinstrument == TRUE) && ((Event[ptr].message & 0xF0) == 0xC0) && (Event[ptr].pixel)) {
					SetBkMode(hdcMem, TRANSPARENT);
					TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (PixelsBetweenLines*27/2), "X", 1); // down 13 1/2 lines from top
					SetBkMode(hdcMem, OPAQUE);
				}

				if ((Event[ptr].message & 0x0000FFFF) == (DWORD)(0x000040B0 | ActiveChannel)) {// sustain pedal
					x = Event[ptr].pixel - Lines[l].pixel;
					notend = x + Event[ptr].pixelsinnote;
 					y = Lines[l].rowonpage + PixelsInGrandStaffAndMyExtraSpace;
					SelectObject(hdcMem, hGreyNaturalPen);
					if ((Event[ptr].message & 0x00FF0000) >= 0x40) {// pedal down
						PedalDownX = x;
						PedalDownY = y;
						PedalDownl = l;
						PedalDownLineEnd = Lines[l].PixelsPerLine;
						PedalDownPage = Page;
						MoveToEx(hdcMem, PedalDownX, PedalDownY-5, NULL);
						LineTo(hdcMem, PedalDownX, PedalDownY);
						LineTo(hdcMem, PedalDownX+4, PedalDownY);// to show at least something at end of page

						for (z = ptr; z < e; z++) {// extend line if it goes to next page
							if ((Event[z].message & 0x0000FFFF) == (DWORD)(0x000040B0 | ActiveChannel) && ((Event[z].message & 0xFF0000) < 0x400000)) {
								if (Event[z].pixel >= (Lines[l].pixel + PedalDownLineEnd)) {
									MoveToEx(hdcMem, PedalDownX, PedalDownY, NULL);
									LineTo(hdcMem, PedalDownLineEnd, PedalDownY);
								}
								break;
							}
						}

					}
					else if (PedalDownX != -1) {// pedal up
						if (PedalDownPage == Page) {
							MoveToEx(hdcMem, PedalDownX+3, PedalDownY, NULL);
							if (PedalDownl != l) {
								LineTo(hdcMem, PedalDownLineEnd, PedalDownY);
								MoveToEx(hdcMem, 0, y, NULL);
							}
						}
						else {// new page
							MoveToEx(hdcMem, 0, y, NULL);
						}
						LineTo(hdcMem, x, y);
						LineTo(hdcMem, x, y-5);
						PedalDownX = -1;
					}
				}

				if (((shownotenames) || (onlynotenames)) && ((Event[ptr].velocity))) {// && ((Event[ptr].message & 0xF0) == 0x90))) {
					if (((Event[ptr].channel != 9) || (!EWQL)) && (ActiveChannels[Event[ptr].channel]) && (Event[ptr].note >= 21) && (Event[ptr].note <= 108)) {
						NoteName[0] = Letter[Event[ptr].note-21];
						NoteName[1] = ' ';
						NoteName[2] = ' ';
						offset = 3;
						if (NoteName[0] == 0) {
							if (usingsharp) {
								NoteName[0] = Letter[Event[ptr].note-22];
								NoteName[1] = '#';
							}
							else {
								NoteName[0] = Letter[Event[ptr].note-20];
								NoteName[1] = 'b';
								offset += NoteMiddle;
							}
						}
						SetBkMode(hdcMem, TRANSPARENT);
						if (onlynotenames) {
							SetTextColor(hdcMem, 0xF0);
						}
						else {
							if (inwhite)
								SetTextColor(hdcMem, WHITE);
						}
						hOldFont = SelectObject(hdcMem, hNoteFont);
						TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, NoteLoc[Event[ptr].note-21] + ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) - offset, NoteName, 2);
						SetTextColor(hdcMem, PrevColor);
						SetBkMode(hdcMem, OPAQUE);
						SelectObject(hdcMem, hOldFont);
					}
				}

				else if ((showvolumes) && ((Event[ptr].velocity) && ((Event[ptr].message & 0xF0) == 0x90)) && (Event[ptr].note >= 21) && (Event[ptr].note <= 108)) {
					x = Event[ptr].velocity;
					if (x / 100) {
						NoteName[0] = (x / 100) + '0';
						NoteName[1] = ((x % 100) / 10) + '0';
						NoteName[2] = (x % 10) + '0';
						y = 3;
					}
					else if (x / 10) {
						NoteName[0] = (x / 10) + '0';
						NoteName[1] = (x % 10) + '0';
						y = 2;
					}
					else {
						NoteName[0] = (x % 10) + '0';
						y = 1;
					}
					offset = 3;
					if ((Event[ptr].sharporflat) && (!usingsharp))
						offset += NoteMiddle;
					hOldFont = SelectObject(hdcMem, hNoteFont);
					SetBkMode(hdcMem, TRANSPARENT);
					if (inwhite)
						SetTextColor(hdcMem, WHITE);
					TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, NoteLoc[Event[ptr].note-21] + ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) - offset, NoteName, y); // 336
					SetTextColor(hdcMem, PrevColor);
					SetBkMode(hdcMem, OPAQUE);
					SelectObject(hdcMem, hOldFont);
				}
				else if ((shownumbers) && ((Event[ptr].velocity) && ((Event[ptr].message & 0xF0) == 0x90)) && (Event[ptr].note >= 21) && (Event[ptr].note <= 108)) {
					x = Event[ptr].note;
					if (x / 100) {
						NoteName[0] = (x / 100) + '0';
						NoteName[1] = ((x % 100) / 10) + '0';
						NoteName[2] = (x % 10) + '0';
						y = 3;
					}
					else if (x / 10) {
						NoteName[0] = (x / 10) + '0';
						NoteName[1] = (x % 10) + '0';
						y = 2;
					}
					else {
						NoteName[0] = (x % 10) + '0';
						y = 1;
					}
					offset = 3;
					if ((Event[ptr].sharporflat) && (!usingsharp))
						offset += NoteMiddle;
					hOldFont = SelectObject(hdcMem, hNoteFont);
					SetBkMode(hdcMem, TRANSPARENT);
					if (inwhite)
						SetTextColor(hdcMem, WHITE);
					TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, NoteLoc[Event[ptr].note-21] + ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) - offset, NoteName, y); // 336
					SetTextColor(hdcMem, PrevColor);
					SetBkMode(hdcMem, OPAQUE);
					SelectObject(hdcMem, hOldFont);
				}
				else if ((showfingers) && (Event[ptr].finger[0])) {
					offset = 3;
					if ((Event[ptr].sharporflat) && (!usingsharp))
						offset += NoteMiddle;
					hOldFont = SelectObject(hdcMem, hNoteFont);
					SetBkMode(hdcMem, TRANSPARENT);
					if (inwhite)
						SetTextColor(hdcMem, WHITE);
					TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel + 2, NoteLoc[Event[ptr].note-21] + ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) - offset, Event[ptr].finger, strlen(Event[ptr].finger)); //336
					SetTextColor(hdcMem, PrevColor);
					SetBkMode(hdcMem, OPAQUE);
					SelectObject(hdcMem, hOldFont);
				}

				else if ((showports) && (Event[ptr].velocity)) {
					offset = 3;
					if ((Event[ptr].sharporflat) && (!usingsharp))
						offset += NoteMiddle;
					hOldFont = SelectObject(hdcMem, hNoteFont);
					SetBkMode(hdcMem, TRANSPARENT);
					if (inwhite)
						SetTextColor(hdcMem, WHITE);
					temp[0] = Event[ptr].port + '1';
					TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel + 2, NoteLoc[Event[ptr].note-21] + ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) - offset, temp, 1);
					SetTextColor(hdcMem, PrevColor);
					SetBkMode(hdcMem, OPAQUE);
					SelectObject(hdcMem, hOldFont);
				}

				if ((showbeatsperminute) && (Event[ptr].dMilliSecondsPerTick)) {
					SetBkMode(hdcMem, TRANSPARENT);
					TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (PixelsBetweenLines*27/2), "I", 1); // down 13 1/2 lines from top
					SetBkMode(hdcMem, OPAQUE);
				}

//////////////
				if (ActiveChannels[Event[ptr].channel]) {
					if ((showvolume) && ((Event[ptr].message & 0xFFFF) == (DWORD)(0x07B0|Event[ptr].channel))) {
						y = ((Event[ptr].message & 0xFF0000) >> 0x10);
						SetBkColor(hdcMem, colors[Event[ptr].message & 0xF]);
						TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (127-y), "V", 1);
						SetBkColor(hdcMem, WHITE);
					}

					if ((showexpression) && ((Event[ptr].message & 0xFFFF) == (DWORD)(0x0BB0|Event[ptr].channel))) {
						y = ((Event[ptr].message & 0xFF0000) >> 0x10);
						SetBkColor(hdcMem, colors[Event[ptr].message & 0xF]);
						TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (127-y), "E", 1);
						SetBkColor(hdcMem, WHITE);
					}

					if ((showmodulation) && ((Event[ptr].message & 0xFFFF) == (DWORD)(0x01B0|Event[ptr].channel))) {
						y = ((Event[ptr].message & 0xFF0000) >> 0x10);
						SetBkColor(hdcMem, colors[Event[ptr].message & 0xF]);
						TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + PixelsInGrandStaffAndMyExtraSpace - y, "M", 1);
						SetBkColor(hdcMem, WHITE);
					}

					if ((showreverb) && ((Event[ptr].message & 0xFFFF) == (DWORD)(0x05BB0|Event[ptr].channel))) {
						y = ((Event[ptr].message & 0xFF0000) >> 0x10);
						SetBkColor(hdcMem, colors[Event[ptr].message & 0xF]);
						TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + PixelsInGrandStaffAndMyExtraSpace - y, "R", 1);
						SetBkColor(hdcMem, WHITE);
					}
					if ((showchorus) && ((Event[ptr].message & 0xFFFF) == (DWORD)(0x05DB0|Event[ptr].channel))) {
						y = ((Event[ptr].message & 0xFF0000) >> 0x10);
						SetBkColor(hdcMem, colors[Event[ptr].message & 0xF]);
						TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + PixelsInGrandStaffAndMyExtraSpace - y, "C", 1);
						SetBkColor(hdcMem, WHITE);
					}
					if ((showpitchbend) && ((Event[ptr].message & 0xFF) == (DWORD)(0xE0|Event[ptr].channel))) {
						y = (Event[ptr].message & 0xFFFF00) >> 8;
						PitchBend1 = y & 0x7F;
						PitchBend2 = (WORD)((y >> 1) & (0x7F << 7));
						PitchBend = PitchBend2 | PitchBend1;
						PitchBendLoc = PitchBend * PixelsBetweenLines * 20 / 0x4000;
						SetBkColor(hdcMem, colors[Event[ptr].message & 0xF]);
						TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (PixelsBetweenLines * 24) - PitchBendLoc, "P", 1);
						SetBkColor(hdcMem, WHITE);
					}
					if ((showportamento) && ((Event[ptr].message & 0xFFFFFF) == (DWORD)(0x07F41B0|Event[ptr].channel))) {
						SetBkColor(hdcMem, colors[Event[ptr].message & 0xF]);
TextOut(hdcMem, Event[ptr].pixel-Lines[l].pixel, ((l % Rows) * PixelsInGrandStaffAndMyExtraSpace) + MyExtraSpace + (PixelsBetweenLines*27/2), "?", 1); // down 13 1/2 lines from top
						SetBkColor(hdcMem, WHITE);
					}
				}
/////////////
				if (((showkey) && (Event[ptr].KeySignature)) || ((showtime) && (Event[ptr].BeatsPerMeasure))) {
					hOldFont = SelectObject(hdcMem, hFont);
					SetBkMode(hdcMem, TRANSPARENT);
					l = line;
					while (Event[ptr].pixel >= (Lines[l].pixel + Lines[l].PixelsPerLine)) {
						l++;
					}
					x = Event[ptr].pixel - Lines[l].pixel;
 					y = Lines[l].rowonpage + MyExtraSpace;// puts y at Row1Top, Row2Top, etc.
					if (Event[ptr].KeySignature) {
						Keysig = Event[ptr].KeySignature;
						offsetX = 1;
						if ((Keysig >= 250) && (Keysig <= 255)) {
							for (z = 0; z < (DWORD)(256-Keysig); z++) {
								TextOut(hdcMem, x + offsetX, FlatSigs[z] + y - (size.cy * 5 / 8), "b", 1);
								TextOut(hdcMem, x + offsetX, FlatSigs[z] + (PixelsBetweenLines * 7) + y - (size.cy * 5 / 8), "b", 1);
								offsetX += 10;
							}
						}
						else if ((Keysig >= 1) && (Keysig <= 6)) {
							for (z = 0; z < (DWORD)Keysig; z++) {
								TextOut(hdcMem, x + offsetX, SharpSigs[z] + y - (size.cy * 5 / 8), "#", 1);
								TextOut(hdcMem, x + offsetX, SharpSigs[z] + (PixelsBetweenLines * 7) + y - (size.cy * 5 / 8), "#", 1);
								offsetX += 10;
							}
						}
					}
					else {// if (Event[ptr].BeatsPerMeasure)
						if (Event[ptr].BeatsPerMeasure >= 10) {
							temp[0] = (Event[ptr].BeatsPerMeasure / 10) + '0';
							temp[1] = (Event[ptr].BeatsPerMeasure % 10) + '0';
							TextOut(hdcMem, x, y + (PixelsBetweenLines * 14) - size.cy - 5, temp, 2);
						}
						else if (Event[ptr].BeatsPerMeasure) {
							temp[0] = Event[ptr].BeatsPerMeasure + '0';
							TextOut(hdcMem, x, y + (PixelsBetweenLines * 14) - size.cy - 5, temp, 1); // x = 1280!
						}
						if (Event[ptr].BeatNoteType >= 10) {
							temp[0] = (Event[ptr].BeatNoteType / 10) + '0';
							temp[1] = (Event[ptr].BeatNoteType % 10) + '0';
							TextOut(hdcMem, x, y + (PixelsBetweenLines * 14), temp, 2);
						}
						else if (Event[ptr].BeatNoteType) {
							temp[0] = Event[ptr].BeatNoteType + '0';
							TextOut(hdcMem, x, y + (PixelsBetweenLines * 14), temp, 1);
						}
					}
					SelectObject(hdcMem, hOldFont);
					SetBkMode(hdcMem, OPAQUE);
				}
			}
		}// end of if (TicksPerQuarterNote)

		SetBkMode(hdcMem, TRANSPARENT);
		TextOut(hdcMem, rect.right-70, 0, page, lstrlen(page));
		if (ActiveChannel < 8)
			FillRect(hdcMem, &InstrumentRect, hInstrumentBrush[ActiveChannel]); // put channel color at top left
		else if ((EWQL) && (ActiveChannel == 9))
			FillRect(hdcMem, &InstrumentRect, hInstrumentBrush[ActiveChannel]); // put channel color at top left
		else {
			hOldPen = SelectObject(hdcMem, hInstrumentPen[ActiveChannel-8]);// Channel color
			if ((ActiveChannel != 9) || (EWQL)) {
				for (y = 0; y < 15; y += 2) { // put striped channel color at top left
					MoveToEx(hdcMem, 0, y, NULL);
					LineTo(hdcMem, 20, y);
				}
			}
			else {
				for (y = 0; y < 15; y++) { // put channel color at top left
					MoveToEx(hdcMem, 0, y, NULL);
					LineTo(hdcMem, 20, y);
				}
			}
			SelectObject(hdcMem, hOldPen);
		}
		TextOut(hdcMem, 0, 0, notetype, 4);
		if (recordingtowave) {
			SetTextColor(hdcMem, 0xF0);
			TextOut(hdcMem, (rect.right/2) - (recordingSize.cx/2), 0, RecordingToWave, strlen(RecordingToWave));
			SetTextColor(hdcMem, PrevColor);
		}
		else if (playingwave) {
			SetTextColor(hdcMem, 0xF0);
			TextOut(hdcMem, (rect.right/2) - (playingSize.cx/2), 0, PlayingWave, strlen(PlayingWave));
			SetTextColor(hdcMem, PrevColor);
		}
		else if (shownotesplayed) {
			TextOut(hdcMem, (rect.right/2) - (showingnotesSize.cx/2), 0, ShowingNotesPlayed, strlen(ShowingNotesPlayed));
		}
		SetBkMode(hdcMem, OPAQUE);
		if (!blackbackground)
			SetBkColor(hdcMem, WHITE);
		else
			SetBkColor(hdcMem, BLACK);
		hOldFont = SelectObject(hdcMem, hSharpFlatFont);
		if (usingsharp)
			TextOut(hdcMem, rect.right-160, 0, "#", 1);
		else
			TextOut(hdcMem, rect.right-160, 0, "b", 1);
		SelectObject(hdcMem, hOldFont);
		if (mixerControlDetailsMasterMute.fValue)
			TextOut(hdcMem, rect.right-145, 0, "   MUTED! ", 10);
		else // if (MasterVolume[4] != 'x')
			TextOut(hdcMem, rect.right-145, 0, MasterVolume, 10);
		if (!ctrlA)
			//////////////////////////////////////////////////////////////////
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
			//////////////////////////////////////////////////////////////////
		else {
			FillRect(hdcMem2, &rect, hBrush2);
			BitBlt(hdcMem2, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCAND);
			BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
		}
		EndPaint(hwnd, &ps);
		return 0;


	case WM_CLOSE:
		if (!playing) {
			if ((Midi[0] == 0) && (e > 4)) {
				if (originalE != e)
					goto savit2;
				else {
					for (x = 0; x < (int)e; x++) {
						if (EventPixels[x] != Event[x].pixel) {
savit2:						x = MessageBox(hwnd, "SAVE CHANGES?", szAppName, MB_YESNOCANCEL);
							if (x == IDYES) {
								WriteMidi();
								if (GetSaveFileName(&ofn)) {
									int x = ofn.nFileExtension;
									ofn.lpstrFile[x] = 'm';
									ofn.lpstrFile[x+1] = 'i';
									ofn.lpstrFile[x+2] = 'd';
									ofn.lpstrFile[x+3] = 0;
									hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
									if (hFile != INVALID_HANDLE_VALUE) {
										Midi[0] = 'M';
										WriteFile(hFile, Midi, i, &dwBytesWritten, NULL);
										Midi[0] = 0;
										CloseHandle(hFile);
									}
								}
							}
							else if (x == IDCANCEL)
								return 0;
							break;
						}
					}
				}
			}
		}
		DestroyWindow(hwnd);
		break;

	case WM_DESTROY:
		if (midi_in) {
			midiInStop(hMidiIn);
			midiInReset(hMidiIn);
			midiInClose(hMidiIn);
		}
		midiOutReset(hMidiOut);
		midiOutClose(hMidiOut);
		if (EWQL) {
			for (x = 0; x < NumberOfPorts; x++) {
				midiOutReset(hMidisOut[x]);
				midiOutClose(hMidisOut[x]);
			}
		}
		if (timePtr)
			timeEndPeriod(TIMER_RESOLUTION);
		DeleteObject(hPen);
		DeleteObject(hOldPen);
		DeleteObject(hBrush);
		for (u = 0; u < 10000; u++)
			if (LastEventInUndo[u] != -1)
				free(UndoEvent[u]);
		mixerClose(hMixer);
		if (WaveBuf)
			VirtualFree(WaveBuf, 0, MEM_RELEASE);
		if (WaveBuf2)
			VirtualFree(WaveBuf2, 0, MEM_RELEASE);
		DeleteCriticalSection(&csTickptr);
		PostQuitMessage(0);
		break;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

//SUBROUTINES//////////////////////////////////////////////////////////////

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
	noending = FALSE;
	badending = FALSE;
	for (x = 0; x < 10; x++) {
		NoEnding[x].note = 0;
		BadEnding[x].note = 0;
	}
	ne = be = 0;
	InitialBeatsPerMinute = 0;
	dataptr = 0;
	gotc0 = 0;
	gotc9 = 0;
	gotpiano = 0;
	gotdrums = 0;
	gottime = 0;
	gotkey = 0;
	DrumSet = 0;
	for (x = 0; x < 16; x++)
		StereoLocations[x] = 0x40;// flag
	PedalDownX = -1;
	bt = 0;
	BigText[0] = 0;
	textName[0] = 0;
	textCopyright[0] = 0; 
	textText[0] = 0;
	ZeroEvents();
	e = 0;
	Time = 0;
	ChannelEvent = 0;
	TicksPerSecond = 0;
	uTimerID = 0;
	for (x = 0; x < 16; x++) {
		continuousustain[x] = 0;
		reverb[x] = 0;
		chorus[x] = 0;
		ModWheel[x] = 0;
		gotc[x] = FALSE;
		gotchannel[x] = FALSE;
		for (y = 0; y < 16; y++)
			InstrumentOffset[x][y] = 0;
	}
	if (!EWQL)
		ChannelInstruments[9][0] = 30 + (DrumSet*10);

	MidiFormat = (Midi[8] << 8) | Midi[9];
	MidiTracks = (Midi[10] << 8) | Midi[11];
	TicksPerQuarterNote = (Midi[12] << 8) | Midi[13];//not computer ticks! doesn't change!
	TicksPerPixel = TicksPerQuarterNote / 40;
	if (TicksPerQuarterNote & 0x8000) { //if hi-bit is set
		TicksPerSecond = (*(BYTE*)&Midi[12] & 0x7FFF) * (*(BYTE*)&Midi[13]);//alternate format
		MessageBox(hwnd, "TicksPerSecond used!", "What the...!", MB_OK);
	}
	i = 14;
	for (tracks = 0; tracks < MidiTracks; tracks++) {
		if (*(DWORD*)&Midi[i] != 0x6B72544D) {//"MTrk"
			MessageBox(hwnd, "bad MTrk", ERROR, MB_OK);
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
			Event[e].pixel = Event[e].tickptr * 40 / TicksPerQuarterNote;
			switch (Midi[i] & 0xF0)//status byte without channel info
			{
			case 0x80://note off
				Event[e].channel = Midi[i++] & 0x0F;
				Channel = Event[e].channel;// for shortened Channel Events
				Event[e].note = Midi[i++];
				i++;
				Event[e].velocity = 0;
				ChannelEvent = 0x80;
				Event[e].message = (0x90|Channel) | (Event[e].note << 8);
				if (!Letter[Event[e].note - 21])
					Event[e].sharporflat = 1;
				break;
			case 0x90://note on (or note off with 0 velocity)
				Event[e].channel = Midi[i++] & 0x0F;
				Channel = Event[e].channel;// for shortened Channel Events
				Event[e].note = Midi[i++];
				Event[e].velocity = Midi[i++];
				ChannelEvent = 0x90;
				Event[e].message = (0x90|Channel) | (Event[e].velocity << 16) | (Event[e].note << 8);
				if (!Letter[Event[e].note - 21])
					Event[e].sharporflat = 1;
				gotchannel[Channel] = TRUE;
				break;
			case 0xA0:
				Channel = Midi[i++] & 0x0F;
				Event[e].channel = Channel;
				Note = Midi[i++];
				Aftertouch = Midi[i++];
				ChannelEvent = 0xA0;
				Event[e].message = (0xA0|Channel) | (Aftertouch << 16) | (Note << 8);
				break;
			case 0xB0:// Controller
				Channel = Midi[i++] & 0x0F;
				Event[e].channel = Channel;
				Controller = Midi[i++];
				ControllerValue = Midi[i++];
				if ((Controller == 10) && (StereoLocations[Channel] == 0x40))
					StereoLocations[Channel] = ControllerValue;
				else if (Controller == 1)
					ModWheel[Channel] = ControllerValue;
				else if (Controller == 91)// reverb
					reverb[Channel] = TRUE;
				else if (Controller == 93)// chorus
					chorus[Channel] =  TRUE;
				ChannelEvent = 0xB0;
				Event[e].message = (0xB0|Channel) | (ControllerValue << 16) | (Controller << 8);
				break;
			case 0xC0: // Program Control
				Channel = Midi[i++] & 0x0F;
				Event[e].channel = Channel;
				MidiInstrument = Midi[i++];
				for (x = 0; x < 16; x++) {
					if (ChannelInstruments[Channel][x] == 0xFF) {
						if (!EWQL) // EWQL puts port there instead of MidiInstrument
							ChannelInstruments[Channel][x] = MidiInstrument;
						for (y = 0; y < 16; y++) {
							if (Event[e].pixel < InstrumentOffset[Channel][y]) {
								for (z = 15; z > (DWORD)y; z--) // open up a space
									InstrumentOffset[Channel][z] = InstrumentOffset[Channel][z-1];
								InstrumentOffset[Channel][z] = Event[e].pixel;
								break;
							}
							else {
								InstrumentOffset[Channel][x] = Event[e].pixel;
								break;
							}
						}
						break;
					}
				}
				if ((Channel == 9) && (!EWQL))
				{
					if (MidiInstrument <= 39)
						DrumSet = 0;
					else if (MidiInstrument <= 48)
						DrumSet = 1;
					else if (MidiInstrument <= 55)
						DrumSet = 2;
					else if (MidiInstrument <= 126)
						DrumSet = 3;
				}
				ChannelEvent = 0xC0;
				Event[e].message = (0xC0|Channel) | (MidiInstrument << 8);
				gotc[Channel] = TRUE;
				break;
			case 0xD0:
				Channel = Midi[i++] & 0x0F;
				Event[e].channel = Channel;
				Aftertouch = Midi[i++];
				ChannelEvent = 0xD0;
				Event[e].message = (0xD0|Channel) | (Aftertouch << 8);
				break;
			case 0xE0:
				Channel = Midi[i++] & 0x0F;
				Event[e].channel = Channel;
				PitchBend1 = Midi[i++];// PitchBend1 is least significant
				PitchBend2 = Midi[i++];
				PitchBend = (PitchBend2 << 8) | PitchBend1;
				ChannelEvent = 0xE0;
				Event[e].message = (0xE0|Channel) | (PitchBend << 8);// the Synthesizer will convert PitchBend to a 14 bit value
				break;

			case 0xF0:
				if (Midi[i] == 0xFF) {//Meta Events
					i++;//to MetaEventType
					MetaEventType = Midi[i];
					i++;//to MetaEventLen
					switch (MetaEventType)
					{
					case 0:// Sequence Number
						MetaEventLen = ReadVLV();
						Event[e].type = MetaEventType;
						Event[e].len = 2;
						Event[e].ptr = dataptr;
						for (x = dataptr + (int)MetaEventLen; dataptr < x; dataptr++)
							Data[dataptr] = Midi[i++];
						break;
					case 1://Text Event
						MetaEventLen = ReadVLV();
						if (*(DWORD*)&Midi[i] == 0x434D444A)//"JDMC"
							Midi[0] = 0;//flag
						else if (Midi[0] == 0) {
							for (x = (int)i, y = 0; (y < 511) && (x  < (int)(i + MetaEventLen)); x++, y++)
								textText[y] = Midi[x];
							textText[y] = 0;
						}
						if (Event[e].tickptr != 0) {
							Midi[i-2] = 5;// it must be a lyric if it's not at the beginning
							saveit = TRUE;
						}
						SaveText(i);
						break;
					case 2:// Copyright
						MetaEventLen = ReadVLV();
						if (Midi[0] == 0) {
							for (x = (int)i, y = 0; x  < (int)(i + MetaEventLen); x++, y++)
								textCopyright[y] = Midi[x];
							textCopyright[y] = 0;
						}
						SaveText(i);
						break;
					case 3:// Sequence/Track Name
						MetaEventLen = ReadVLV();
						if (Midi[0] == 0) {
							for (x = (int)i, y = 0; x  < (int)(i + MetaEventLen); x++, y++)
								textName[y] = Midi[x];
							textName[y] = 0;
						}
						SaveText(i);
						break;
					case 4:// Instrument Name
						MetaEventLen = ReadVLV();
						SaveText(i);
						break;
					case 5:// Lyric
						MetaEventLen = ReadVLV();
						Event[e].type = MetaEventType;
						Event[e].len = (WORD)MetaEventLen;
						Event[e].ptr = dataptr;
						for (x = dataptr + (int)MetaEventLen; dataptr < x; dataptr++)
							Data[dataptr] = Midi[i++];
						break;
					case 6:// Marker
					case 7:// Cue Point
					case 8:// ?
					case 9:// ?
					case 10:
					case 11:
					case 12:
					case 13:
					case 14:
					case 15:
						MetaEventLen = ReadVLV();
						SaveText(i);
						break;
					case 0x20:// MIDI Channel Prefix
					case 0x21:// Major/Minor (obsolete)
						MetaEventLen = ReadVLV();
						Event[e].type = (BYTE)MetaEventType;
						Event[e].len = (WORD)MetaEventLen;
						Event[e].ptr = dataptr;
						for (x = dataptr + (int)MetaEventLen; dataptr < x; dataptr++)
							Data[dataptr] = Midi[i++];
						break;
					case 0x2F:
						//not at length, but at 0 (this is track end id)
						i++;
						e--;// because of e++;
						break;
					case 0x51://tempo
						i++;
						mSecondsPerBeat = (Midi[i] << 16) | (Midi[i+1] << 8) | Midi[i+2];// microseconds/beat
						dMilliSecondsPerTick = (((double)mSecondsPerBeat / (double)TicksPerQuarterNote)) / 1000.0;
						Event[e].dMilliSecondsPerTick = dMilliSecondsPerTick;
						oldMilliSecondsPerTick = dMilliSecondsPerTick;
						if (InitialBeatsPerMinute == 0)
							InitialBeatsPerMinute = 60000000 / mSecondsPerBeat;
						i += 3;
						break;
					case 0x54://SMPTE offset
						i++;
						Event[e].type = MetaEventType;
						Event[e].len = 5;
						Event[e].ptr = dataptr;
						for (x = dataptr + 5; dataptr < x; dataptr++)
							Data[dataptr] = Midi[i++];
						break;
					case 0x58://time signature
						i++;// past the number 4
						Event[e].BeatsPerMeasure = Midi[i++];
						Event[e].BeatNoteType = Powers[Midi[i++]];
						switch (Event[e].BeatNoteType)
						{
						case 32:
							PixelsPerBeat = 5;
							break;
						case 16:
							PixelsPerBeat = 10;
							break;
						case 8:
							PixelsPerBeat = 20;
							break;
						case 4:
							PixelsPerBeat = 40;
							break;
						case 2:
							PixelsPerBeat = 80;
							break;
						case 1:
							PixelsPerBeat = 160;
							break;
						default:
							PixelsPerBeat = 40;
						}			
						ClocksPerBeat = Midi[i++];//normally 24 clocks/beat
						ThirtysecondNotesPer24Clocks = Midi[i++];//number of thirty-second notes per 24 midi clocks (normally 8) - so normally 1 quarter note = 1 beat
						gottime = TRUE;
						break;
					case 0x59://key signature
						i++;//past the number 2
						if (Midi[i] == 0) // key of C Major
							Event[e].KeySignature = 200;// trick
						else
							Event[e].KeySignature = Midi[i]; // positive numbers are number of sharps and negative numbers are number of flats
						if (gotkey == FALSE) {
							if (Midi[i] < 128)
								usingsharp = TRUE;
							else
								usingsharp = FALSE;
						}
						i++;
						i++; // skip Major/Minor Key BYTE
						gotkey = TRUE;
						break;
					case 0x7F:// words to: As Time Goes By, Heart and Soul, Memory, and Some Enchanted Evening
						MetaEventLen = ReadVLV();
						if (*(DWORD*)&Midi[i+1] == 0x434D444A) {//"JDMC"
							Midi[0] = 0;//flag
						}
						if (Midi[i-1] == 2) {
							if ((Midi[i+1] < 0x10) && (Midi[i+1] != 0))
								Event[e-1].finger[0] = Midi[i+1] + '0';
							else { // if (EWQL) ChannelInstruments[channel][port] = channel;
								Event[e-1].port = Midi[i+1] >> 4; // PORT
								ChannelInstruments[Event[e-1].channel][Event[e-1].port] = Event[e-1].channel;
							}
						}
						else if (Midi[i-1] == 3) {
							Event[e-1].finger[0] = Midi[i+1];
							if (Midi[i+1] <= 9) // a number
								Event[e-1].finger[0] += '0';
							Event[e-1].finger[1] = Midi[i+2];
							if (Midi[i+2] <= 9) // a number
								Event[e-1].finger[1] += '0';
						}
						i += MetaEventLen;
						e--;// because of e++;
						break;
					default:
						MessageBox(hwnd, "Unknown Meta Event", "", MB_OK);
						e--;// because of e++;
					}//end of switch (MetaEventType)
					break;//to get out of switch that this switch is in
				}//end of if (Midi[i] == 0xFF)
				else {//SysEx 0xF0
					Event[e].type = 0xF0;
					Event[e].ptr = dataptr;
					x = i;
					for ( ; Midi[i] != 0xF7; dataptr++)
						Data[dataptr] = Midi[i++];
					Data[dataptr++] = Midi[i++];
					Event[e].len = (WORD)(i - x);
				}
				break;// out of case 0xF0:
			default://it must be a shortened Channel Event
				switch (ChannelEvent)
				{
				case 0x80:
					Event[e].channel = Channel;
					Event[e].note = Midi[i++];
					i++;
					Event[e].velocity = 0;
					Event[e].message = (0x90|Channel) | (Event[e].note << 8);
					if (!Letter[Event[e].note - 21])
						Event[e].sharporflat = 1;
					break;
				case 0x90:
					Event[e].channel = Channel;
					Event[e].note = Midi[i++];
					Event[e].velocity = Midi[i++];
					Event[e].message = (0x90|Channel) | (Event[e].velocity << 16) | (Event[e].note << 8);
					if (!Letter[Event[e].note - 21])
						Event[e].sharporflat = 1;
					break;
				case 0xA0:
					Event[e].channel = Channel;
					MidiNote = Midi[i++];
					Aftertouch = Midi[i++];
					Event[e].message = (0xA0|Channel) | (Aftertouch << 16) | (MidiNote << 8);
					break;
				case 0xB0:
					Event[e].channel = Channel;
					Controller = Midi[i++];
					ControllerValue = Midi[i++];
					if ((Controller == 10) && (StereoLocations[Channel] == 0x40))
						StereoLocations[Channel] = ControllerValue;
					else if (Controller == 1)
						ModWheel[Channel] = ControllerValue;
					else if (Controller == 91)// reverb
						reverb[Channel] = TRUE;
					else if (Controller == 93)// chorus
						chorus[Channel] = TRUE;
					Event[e].message = (0xB0|Channel) | (ControllerValue << 16) | (Controller << 8);
					break;
				case 0xC0: // Program Control
					Event[e].channel = Channel;
					MidiInstrument = Midi[i++];
					for (x = 0; x < 16; x++) {
						if (ChannelInstruments[Channel][x] == 0xFF) {
							if ((Channel != 0) && (x != 1)) // if not default program control
								ChannelInstruments[Channel][x] = MidiInstrument;
							for (y = 0; y < 16; y++) {
								if (Event[e].pixel < InstrumentOffset[Channel][y]) {
									for (z = 15; z > (DWORD)y; z--) // open up a space
										InstrumentOffset[Channel][z] = InstrumentOffset[Channel][z-1];
									InstrumentOffset[Channel][z] = Event[e].pixel;
									break;
								}
								else {
									InstrumentOffset[Channel][x] = Event[e].pixel;
									break;
								}
							}
							break;
						}
					}
					if ((Channel == 9) && (!EWQL))
					{
						if (MidiInstrument <= 39)
							DrumSet = 0;
						else if (MidiInstrument <= 48)
							DrumSet = 1;
						else if (MidiInstrument <= 55)
							DrumSet = 2;
						else if (MidiInstrument <= 126)
							DrumSet = 3;
					}
					Event[e].message = (0xC0|Channel) | (MidiInstrument << 8);
					gotc[Channel] = TRUE;
					break;
				case 0xD0:
					Event[e].channel = Channel;
					Aftertouch = Midi[i++];
					Event[e].message = (0xD0|Channel) | (Aftertouch << 8);
					break;
				case 0xE0:
					Event[e].channel = Channel;
					PitchBend1 = Midi[i++];// PitchBend1 is least significant
					PitchBend2 = Midi[i++];
					PitchBend = (PitchBend2 << 8) | PitchBend1;
					ChannelEvent = 0xE0;
					Event[e].message = (0xE0|Channel) | (PitchBend << 8);// the Synthesizer will convert PitchBend to a 14 bit value
					break;
				default:
					MessageBox(hwnd, "Unknown MIDI data", "", MB_OK);
					return;
				}
			}//end of switch (Midi[i] & 0xF0)
			if (e == MAX_EVENT) {
				MessageBox(hwnd, "MIDI Event array wasn't big enough!", ERROR, MB_OK);
				return;
			}
			if ((Event[e].message  & 0xFF) == 0xC0)
				gotc0 = TRUE;
			else if (((Event[e].message  & 0xFF) == 0xC9) && (!EWQL))
				gotc9 = TRUE;
			if (Event[e].channel == 0)
				gotpiano = TRUE;
			else if ((Event[e].channel == 9) && (!EWQL))
				gotdrums = TRUE;
			//////
			e++;//
			//////
		}// end of while (i < (TrackBegin + TrackLen))
	}// end of for (tracks = 0

	BigText[bt] = 0;
	if (gottime == FALSE) {
		Event[e].tickptr = 0;
		Event[e].pixel = 0;
		Event[e].BeatNoteType = 4;
		Event[e++].BeatsPerMeasure = 4;
	}
	if (gotkey == FALSE) {
		Event[e].tickptr = 0;
		Event[e].pixel = 0;
		Event[e++].KeySignature = 200;// trick
	}
	if ((gotpiano == TRUE) && (gotc0 == FALSE) && (!EWQL)) {// to correct for bad MIDI writer
		gotc[0] = TRUE;
		Event[e].tickptr = 0;
		Event[e].pixel = 0;
		Event[e++].message = 0xC0;
		ChannelInstruments[0][0] = 0;
	}
	if ((gotdrums == TRUE) && (gotc9 == FALSE)) {// to correct for bad MIDI writer
		gotc[9] = TRUE;
		Event[e].tickptr = 0;
		Event[e].pixel = 0;
		Event[e++].message = 0xC9;
		ChannelInstruments[9][0] = 30;
	}
	if (!EWQL) {
		for (x = 0; x < 16; x++) {
			if ((gotchannel[x]) && (!gotc[x])) {
				Event[e].tickptr = 0;
				Event[e].pixel = 0;
				Event[e++].message = 0xC0 | x;
				ChannelInstruments[x][0] = 0;
			}
		}
	}
	if (dMilliSecondsPerTick == 0.0) {
		InitialBeatsPerMinute = 120;
		dMilliSecondsPerTick = 60000.0 / (double)(InitialBeatsPerMinute * TicksPerQuarterNote);
		for (x = e; x >= 2; x--)
			Event[x+1] = Event[x];
		Event[2].dMilliSecondsPerTick = dMilliSecondsPerTick;
		Event[2].pixel = 0;
		Event[2].pixelsinnote = 0;
		Event[2].tickptr = 0;
		Event[2].ticksinnote = 0;
		Event[2].message = 0;//this is only changed with note on or off
		Event[2].note = 0;
		Event[2].velocity = 0;
		Event[2].sharporflat = 0;
		Event[2].channel = 17;//flag to ignore Event[2] (except for midiOut) (it's not in ActiveChannels)
		Event[2].port = 0;
		Event[2].time = 0;
		Event[2].BeatsPerMeasure = 0;
		Event[2].BeatNoteType = 0;
		Event[2].KeySignature = 0;
		Event[2].type = 0;
		Event[2].len = 0;
		Event[2].ptr = 0;
		Event[2].finger[0] = 0;
		Event[2].overlapped = 0;
		e++;
	}

	qsort(Event, e, sizeof(Event[0]), Compare);
	Resort();

//fdsa
	if (saveit) {
		saveit = FALSE;
		hFile2 = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (hFile2 != INVALID_HANDLE_VALUE) {
			Midi[0] = 'M';
			WriteFile(hFile2, Midi, fileSize, &dwBytesWritten, NULL);// i from WriteMidi
			Midi[0] = 0;
			CloseHandle(hFile2);
		}
	}
	tickptr = 0xFFFFFFFF;// flag
	dMilliSecondsPerTick = 0.0;
	overlapped = FALSE;

	for (x = 0; x < (int)e; x++) {// check for errors
		if ((Event[x].note) && (Event[x].velocity)) {
			for (y = x+1; y < (int)e; y++) {
				if ((Event[y].note == Event[x].note) && (Event[y].channel == Event[x].channel)) {
					if (Event[y].velocity == 0) {
						if (0x7FFFFFFF == (Event[y].message | 0x7FFFFFFF)) { // if Event[y] not used yet (no same-note overlap)
							Event[y].message |= 0x80000000; // flag to catch endings with no beginning
							Event[x].pixelsinnote = Event[y].pixel - Event[x].pixel;
							Event[x].ticksinnote = Event[x].pixelsinnote * TicksPerQuarterNote / 40;
							Event[y].overlapped = Event[x].overlapped;
							break; // note has ending
						}
						else {
							Event[x].overlapped = ol++; // if ol was 255, it will become 0
							overlapped = TRUE;
						}
					}
				}
			} // end of for (y = x+1; y < (int)e; y++)

			if (y == (int)e) { // note doesn't have ending
				noending = TRUE;
				if (ne < 10) {
					NoEnding[ne].note = Event[x].note;
					NoEnding[ne].pixel = Event[x].pixel;
					ne++;
				}
				for ( ; x < (int)e; x++) // remove Note On Event
					Event[x] = Event[x+1];
				e--;
			}
			if ((Event[x].velocity) && (Event[x].message == 0)) {
				for (y = x+1; y < (int)e; y++) {// delete bad note
					if ((Event[y].note == Event[x].note) && (Event[y].channel == Event[x].channel)) {
						for ( ; y < (int)e; y++ )
							Event[y] = Event[y+1];// delete note off
						e--;
						x--;
					}
				}
				for (y = x; y < (int)e; y++)
					Event[y] = Event[y+1];// delete note on
				e--;
				x--;
				MessageBox(hwnd, "A note with no message\nhas been deleted.", "", MB_OK);
			}
		} // end of if ((Event[x].note) && (Event[x].velocity))
		else if (Event[x].dMilliSecondsPerTick) {// delete unnecessary BPM
			if ((Event[x].dMilliSecondsPerTick == dMilliSecondsPerTick) || (Event[x].tickptr == tickptr)) {
				for (y = x; y < (int)e; y++)
					Event[y] = Event[y+1];
				e--;
				x--;
			}
			else {
				dMilliSecondsPerTick = Event[x].dMilliSecondsPerTick;
				tickptr = Event[x].tickptr;
			}
		}
	} // end of for (x = 0; x < (int)e; x++)

	for (y = 0; y < (int)e; y++) { // check for note ending without a beginning
//if ((Event[y].note) && (Event[y].velocity) && (Event[y].pixelsinnote == 0))
//	x=x;
		if ((Event[y].note) && (Event[y].velocity == 0)) {
			if (0x7FFFFFFF == (Event[y].message | 0x7FFFFFFF)) { // an ending without a beginning (flag not set)
				badending = TRUE;
				if (be < 10) {
					BadEnding[be].note = Event[y].note;
					BadEnding[be].pixel = Event[y].pixel;
					be++;
				}
				for ( ; y < (int)e; y++) // remove Note Off Event
					Event[y] = Event[y+1];
				e--;
			}
			else
				Event[y].message &= 0x7FFFFFFF; // remove flag
		}
	}
	if (dMilliSecondsPerTick == 0.0)
		MessageBox(hwnd, "There's no Beats/Minute!", ERROR, MB_OK);

	///////////
	FillLines();
	///////////
}

void WriteVLV(void)

{
	DWORD x = DeltaTicks & 0x7F;

	while (DeltaTicks >>= 7) {
		x <<= 8;
		x |= ((DeltaTicks & 0x7F) | 0x80);
	}
	while (TRUE) {
		Midi[i++] = (BYTE)x;
		if (x & 0x80)
			x >>= 8;
		else
			break;
	}
}

void WriteMidi(void)
{//use data in Event to write .mid file
	BOOL gotmidi0;

	if (Midi[0] == 0)
		gotmidi0 = TRUE;
	else
		gotmidi0 = FALSE;
	*(DWORD*)&Midi[0] = 0x6468544D;//"MThd"
	*(DWORD*)&Midi[4] = 0x06000000;//bytes go in in reverse order because of Big Endian
	*(WORD*)&Midi[8] = 0;// type 0 track data
	*(WORD*)&Midi[0xA] = 0x0100;//1 track
	Midi[0xC] = TicksPerQuarterNote >> 8;// reverse bytes
	Midi[0xD] = TicksPerQuarterNote & 0xFF;// reverse bytes
	*(DWORD*)&Midi[0xE] = 0x6B72544D;//"MTrk"
	//*(DWORD*)&Midi[18] will hold TrackLen
	i = 22;//beginning of track data

	if (gotmidi0) {
		Midi[i++] = 0;//delta tick
		Midi[i++] = 0xFF;
		Midi[i++] = 0x7F;// Sequencer-specific data
		Midi[i++] = 7;// length
		Midi[i++] = 0;
		Midi[i++] = 'J';
		Midi[i++] = 'D';
		Midi[i++] = 'M';
		Midi[i++] = 'C';
		Midi[i++] = 'o';
		Midi[i++] = 'x';

		if (textName[0]) {
			y = lstrlen(textName);
			Midi[i++] = 0;//delta tick
			Midi[i++] = 0xFF;
			Midi[i++] = 3;// Sequence/Track Name
			Midi[i++] = y;
			for (x = 0; x < y; x++)
				Midi[i++] = textName[x];
		}
		if (textCopyright[0]) {
			y = lstrlen(textCopyright);
			Midi[i++] = 0;//delta tick
			Midi[i++] = 0xFF;
			Midi[i++] = 2;// Copyright
			Midi[i++] = y;
			for (x = 0; x < y; x++)
				Midi[i++] = textCopyright[x];
		}
		if (textText[0]) {
			y = lstrlen(textText);
			Midi[i++] = 0;//delta tick
			Midi[i++] = 0xFF;
			Midi[i++] = 1;// Text
			Midi[i++] = y;
			for (x = 0; x < y; x++)
				Midi[i++] = textText[x];
		}
	}// end of if (gotmidi0)
	for (Channel = 0; Channel < 16; Channel++) {
		if (ChannelInstruments[Channel][0] == 128) {// none
			for (x = 0; x < (int)e; ) {
				if (Event[x].channel == Channel) {
					for (y = x; y < (int)e; y++)
						Event[y] = Event[y+1];
					e--;
				}
				else
					x++;
			}	
		}
	}

	qsort(Event, e, sizeof(Event[0]), Compare);
	Resort();

	for (x = 0; x < (int)e; x++) {
		if ((EWQL) && (Event[x].message == 0x00B0)) // Bank Select
			continue; // because it's added at end, and we don't want it added every time the music is played
		if (Event[x].note) {
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			Midi[i++] = (0x90|Event[x].channel);
			Midi[i++] = Event[x].note;
			Midi[i++] = Event[x].velocity;
			if (Event[x].finger[0]) {
				WriteVLV();
				Midi[i++] = 0xFF;
				Midi[i++] = 0x7F;
				if (Event[x].finger[1] == 0) {
					Midi[i++] = 2; // length of ID + data
					y = Event[x].finger[0] - '0';
					z = 0;
				}
				else {
					Midi[i++] = 3; // length of ID + data
					if ((Event[x].finger[0] >= '0') && (Event[x].finger[0] <= '9'))
						y = Event[x].finger[0] - '0';
					else
						y = Event[x].finger[0];
					if ((Event[x].finger[1] >= '0') && (Event[x].finger[1] <= '9'))
						z = Event[x].finger[1] - '0';
					else
						z = Event[x].finger[1];
				}
				Midi[i++] = 0; // ID
				if (z == 0)
					Midi[i++] = (BYTE)y;
				else {
					Midi[i++] = (BYTE)y;
					Midi[i++] = (BYTE)z;
				}
			}
			if (EWQL) { // PORT
				WriteVLV();
				Midi[i++] = 0xFF;
				Midi[i++] = 0x7F;
				Midi[i++] = 2; // length of ID + data
				Midi[i++] = 0; // ID
				Midi[i++] = (BYTE)(Event[x].port << 4); // finger might be in lower nibble
			}
		}

		else if ((Event[x].message & 0xF0) == 0xA0) {
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			Midi[i++] = (BYTE)(Event[x].message & 0xFF);// includes channel
			Midi[i++] = (BYTE)(Event[x].message >> 8) & 0xFF;// note
			Midi[i++] = (BYTE)(Event[x].message >> 16) & 0xFF;// aftertouch pressure
		}

		else if ((Event[x].message & 0xF0) == 0xB0) {
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			Midi[i++] = (BYTE)(Event[x].message & 0xFF);
			Midi[i++] = (BYTE)(Event[x].message >> 8) & 0xFF;// controller
			Midi[i++] = (BYTE)(Event[x].message >> 16) & 0xFF;// controller value
		}
		else if ((Event[x].message & 0xF0) == 0xC0) {//((Event[x].pixel) && 
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			if (!EWQL) {
				Midi[i++] = (BYTE)(Event[x].message & 0xFF);
				if (((Event[x].message & 0xF) != 9) || (!EWQL))
					Midi[i++] = (BYTE)(Event[x].message >> 8) & 0xFF;// instrument
				else
					Midi[i++] = 30 + (DrumSet*10);
			}
			else { // if (EWQL)
				Midi[i++] = 0xFF; // Meta Event
				Midi[i++] = 6; // MetaEventType (marker)
				Midi[i++] = 1; // MetaEventLen
				Midi[i++] = 0;
			}
		}
		else if ((Event[x].message & 0xF0) == 0xD0) {
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			Midi[i++] = (BYTE)(Event[x].message & 0xFF);// includes channel
			Midi[i++] = (BYTE)(Event[x].message >> 16) & 0xFF;// aftertouch pressure
		}
		else if ((Event[x].message & 0xF0) == 0xE0) {
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			Midi[i++] = (BYTE)(Event[x].message & 0xFF);
			Midi[i++] = (BYTE)(Event[x].message >> 8) & 0x7F;// PitchBend1 (least significant)
			Midi[i++] = (BYTE)(Event[x].message >> 16) & 0x7F;// PitchBend2
		}
		else if (Event[x].dMilliSecondsPerTick) {
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			Midi[i++] = 0xFF;
			Midi[i++] = 0x51;
			Midi[i++] = 3;// bytes
			mSecondsPerBeat = (int)(Event[x].dMilliSecondsPerTick * 1000.0 * (double)TicksPerQuarterNote);
			Midi[i++] = (mSecondsPerBeat >> 16) & 0xFF;
			Midi[i++] = (mSecondsPerBeat >> 8) & 0xFF;
			Midi[i++] = mSecondsPerBeat & 0xFF;
		}
		else if (Event[x].KeySignature) {
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			Midi[i++] = 0xFF;
			Midi[i++] = 0x59;// Key Signature
			Midi[i++] = 2;
			if (Event[x].KeySignature == 200)
				Midi[i] = 0;
			else
				Midi[i] = Event[x].KeySignature;
			i++;
			Midi[i++] = 0;
		}
		else if (Event[x].BeatsPerMeasure) {
			if (x)
				DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
			else
				DeltaTicks = 0;
			WriteVLV();
			Midi[i++] = 0xFF;
			Midi[i++] = 0x58;// Time Signature
			Midi[i++] = 4;
			Midi[i++] = Event[x].BeatsPerMeasure;
			switch (Event[x].BeatNoteType)
			{
			case 2:
				y = 1;
				break;
			case 4:
				y = 2;
				break;
			case 8:
				y = 3;
				break;
			case 16:
				y = 4;
				break;
			case 32:
				y = 5;
				break;
			}
			Midi[i++] = y;
			Midi[i++] = 24;// standard
			Midi[i++] = 8;// standard
		}
		else if (Event[x].type) {
			if ((!gotmidi0) || ((Event[x].type != 1) && (Event[x].type != 2) && (Event[x].type != 3))) {
				if (x)
					DeltaTicks = (Event[x].tickptr - Event[x-1].tickptr);
				else
					DeltaTicks = 0;
				WriteVLV();
				if (Event[x].type != 0xF0) {
					Midi[i++] = 0xFF;
					Midi[i++] = Event[x].type;
					y = Event[x].len;
					z = (y & 0x7F);// Variable Length
					while (y >>= 7)
					{
						z <<= 8;
						z |= ((y & 0x7F) | 0x80);
					}
					while (TRUE)
					{
						Midi[i++] = (BYTE)z;
						if (z & 0x80)
							z >>= 8;
						else
							break;
					}
				}
				z = Event[x].ptr;
				for (y = 0; y < Event[x].len; y++, z++)
					Midi[i++] = Data[z];
			}
		}
	}
	if (EWQL) { // to add something past last note to get clean break in EWQL
		DeltaTicks = 1200; // to end without that bad sound in EWQL
		WriteVLV();
		Midi[i++] = 0xB0;
		Midi[i++] = 0; // Bank Select
		Midi[i++] = 0;
	}
	Midi[i++] = 0;//delta tick (yes, 0)
	Midi[i++] = 0xFF;
	Midi[i++] = 0x2F;//end of track
	Midi[i++] = 0;
	TrackLen = i - 22;
	_asm mov eax, TrackLen
	_asm bswap eax
	_asm mov TrackLen, eax
	*(DWORD*)&Midi[0x12] = TrackLen;
	if (gotmidi0)
		Midi[0] = 0;
	return;
}

void ChangeNoteType(char ch)
{
	notetype[1] = ch;
	notetype[2] = ' ';// to clean up notetype[1] change and to erase dot
	TextOut(hdcMem, 0, 0, notetype, 4);
	InvalidateRect(hwnd, &rect, FALSE);
}

void GetTicksInNote(void)
{
	if ((notetype[1] == 'W') && (notetype[2] == ' '))
		TempEvent.ticksinnote = (160 * TicksPerQuarterNote / 40);
	else if ((notetype[1] == 'W') && (notetype[2] == '.'))
		TempEvent.ticksinnote = (160 * TicksPerQuarterNote / 40) + ((160 * TicksPerQuarterNote / 40) / 2);
	else if ((notetype[1] == 'W') && (notetype[2] == '3'))
		TempEvent.ticksinnote = ((160 * TicksPerQuarterNote / 40) / 3);
	else if ((notetype[1] == 'H') && (notetype[2] == ' '))
		TempEvent.ticksinnote = (80 * TicksPerQuarterNote / 40);
	else if ((notetype[1] == 'H') && (notetype[2] == '.'))
		TempEvent.ticksinnote = (80 * TicksPerQuarterNote / 40) + ((80 * TicksPerQuarterNote / 40) / 2);
	else if ((notetype[1] == 'H') && (notetype[2] == '3'))
		TempEvent.ticksinnote = ((80 * TicksPerQuarterNote / 40) / 3);
	else if ((notetype[1] == 'Q') && (notetype[2] == ' '))
		TempEvent.ticksinnote = (TicksPerQuarterNote);
	else if ((notetype[1] == 'Q') && (notetype[2] == '.'))
		TempEvent.ticksinnote = (TicksPerQuarterNote) + ((TicksPerQuarterNote) / 2);
	else if ((notetype[1] == 'Q') && (notetype[2] == '3'))
		TempEvent.ticksinnote = ((TicksPerQuarterNote) / 3);
	else if ((notetype[1] == 'E') && (notetype[2] == ' '))
		TempEvent.ticksinnote = (20 * TicksPerQuarterNote / 40);
	else if ((notetype[1] == 'E') && (notetype[2] == '.'))
		TempEvent.ticksinnote = (20 * TicksPerQuarterNote / 40) + ((20 * TicksPerQuarterNote / 40) / 2);
	else if ((notetype[1] == 'E') && (notetype[2] == '3'))
		TempEvent.ticksinnote = ((20 * TicksPerQuarterNote / 40) / 3);
	else if ((notetype[1] == 'S') && (notetype[2] == ' '))
		TempEvent.ticksinnote = (10 * TicksPerQuarterNote / 40);
	else if ((notetype[1] == 'S') && (notetype[2] == '.'))
		TempEvent.ticksinnote = (10 * TicksPerQuarterNote / 40) + ((10 * TicksPerQuarterNote / 40) / 2);
	else if ((notetype[1] == 'T') && (notetype[2] == ' '))
		TempEvent.ticksinnote = (5 * TicksPerQuarterNote / 40);
	else if ((notetype[1] == 'T') && (notetype[2] == '.'))
		TempEvent.ticksinnote = (5 * TicksPerQuarterNote / 40) + ((5 * TicksPerQuarterNote / 40) / 2);
	else if ((notetype[1] == 'F') && (notetype[2] == ' '))
		TempEvent.ticksinnote = (3 * TicksPerQuarterNote / 40);
	else if ((notetype[1] == 'F') && (notetype[2] == '.'))
		TempEvent.ticksinnote = (3 * TicksPerQuarterNote / 40) + ((5 * TicksPerQuarterNote / 40) / 2);
}

void Uncheck(UINT id)
{
	if (id != ID_NOTETYPE_WHOLE)
		CheckMenuItem(hMenu, ID_NOTETYPE_WHOLE, MF_UNCHECKED);
	if (id != ID_NOTETYPE_HALF)
		CheckMenuItem(hMenu, ID_NOTETYPE_HALF, MF_UNCHECKED);
	if (id != ID_NOTETYPE_QUARTER)
		CheckMenuItem(hMenu, ID_NOTETYPE_QUARTER, MF_UNCHECKED);
	if (id != ID_NOTETYPE_EIGHTH)
		CheckMenuItem(hMenu, ID_NOTETYPE_EIGHTH, MF_UNCHECKED);
	if (id != ID_NOTETYPE_SIXTEENTH)
		CheckMenuItem(hMenu, ID_NOTETYPE_SIXTEENTH, MF_UNCHECKED);
	if (id != ID_NOTETYPE_THIRTYSECOND)
		CheckMenuItem(hMenu, ID_NOTETYPE_THIRTYSECOND, MF_UNCHECKED);
	if (id != ID_NOTETYPE_SIXTYFOURTH)
		CheckMenuItem(hMenu, ID_NOTETYPE_SIXTYFOURTH, MF_UNCHECKED);
}

void Uncheck2(void)
{
	CheckMenuItem(hMenu, PLAYBACKTEMPO100, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO80, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO60, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO40, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO20, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO0, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO_20, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO_40, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO_60, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO_80, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKTEMPO_100, MF_UNCHECKED);
}

void ChangeLength(HWND hwndDlg, int length, HWND hwndStart, HWND hwndEnd)
{
	int x;

	GetWindowText(hwndStart, textStart, 10);
	x = atoi(textStart);
	_itoa(x+length, textEnd, 10);
	SetWindowText(hwndEnd, textEnd);
}


void CALLBACK waveInProc(HWAVEIN hWaveIn, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam)
{
	if (message == WIM_CLOSE)
		PostMessage(hwnd, WM_USER6, 0, 0);
}

void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam)
{

	if (message == WOM_DONE) {
		PostMessage(hwnd, WM_USER7, 0, 0);
	}
}

void CALLBACK waveOutProc4(HWAVEOUT hWaveOut4, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam)
{
	if (message == WOM_DONE) {
		PostMessage(hwnd, WM_USER10, 0, 0);
	}
}

int CALLBACK ParametersProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	double dBeatsPerMinute;
	static int Length, New;
	static char temp[10];
	static HWND hwndBPM, hwndVolume, hwndStart, hwndEnd, hwndKey, hwndListButton, hwndFinger;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndListButton = GetDlgItem(hwndDlg, IDC_BUTTON9);
		if (showlist)
			ShowWindow(hwndListButton, SW_SHOWNORMAL);
		Length = 0;
		hwndBPM = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndVolume = GetDlgItem(hwndDlg, IDC_EDIT2);
		hwndStart = GetDlgItem(hwndDlg, IDC_EDIT3);
		hwndEnd = GetDlgItem(hwndDlg, IDC_EDIT4);
		hwndFinger = GetDlgItem(hwndDlg, IDC_EDIT5);
		hwndKey = GetDlgItem(hwndDlg, IDC_EDIT6);
		SetWindowText(hwndBPM, textBPM);
		SetWindowText(hwndVolume, textVolume);
		SetWindowText(hwndStart, textStart);
		SetWindowText(hwndEnd, textEnd);
		if (Event[thisX].finger[0])
			SetWindowText(hwndFinger, Event[thisX].finger);
		if (thisInstrument < 128) {// not 0xFF flag
			if (!EWQL) {
				strcpy(TitleName, myInstruments[thisInstrument]);
				if (Event[thisX].channel < 10) {
					Chan2[10] = Event[thisX].channel + '1';
					Chan2[11] = ')';
					Chan2[12] = ' ';
				}
				else {
					Chan2[10] = (Event[thisX].channel / 10) + '0';
					Chan2[11] = (Event[thisX].channel % 10) + '1';
					Chan2[12] = ')';
				}
				strcat(TitleName, Chan2);
			}
			else { // if (EWQL)
				int x, y, z; // can't use the global ones

				for (y = 0, z = 0; z < thisInstrument; y++, z++) {
					for (x = y; (InstrumentBufs[Event[thisX].port][x] != '\r') && (InstrumentBufs[Event[thisX].port][x] != 0); x++)
						;
					y = x+1;
				}
				for (x = 0; (InstrumentBufs[Event[thisX].port][y] != 0) && (InstrumentBufs[Event[thisX].port][y] != '\r') && (InstrumentBufs[Event[thisX].port][y] != '('); x++, y++)
					TitleName[x] = InstrumentBufs[Event[thisX].port][y];
				TitleName[x] = 0;
			}
			SetWindowText(hwndDlg, TitleName);
		}
		else if ((Note >= 27) && (Note <= 87)) {// percussion instrument
			strcpy(TitleName, Percussion[Event[thisX].note-27]);
			Chan2[10] = '1';
			Chan2[11] = '0';
			Chan2[12] = ')';
			strcat(TitleName, Chan2);
			SetWindowText(hwndDlg, TitleName);
		}
		SetFocus(hwndDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:
			Length = WHOLE - onelesspixel;
			ChangeLength(hwndDlg, WHOLE, hwndStart, hwndEnd);
			SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
			break;
		case IDC_BUTTON2:
			Length = HALF - onelesspixel;
			ChangeLength(hwndDlg, HALF, hwndStart, hwndEnd);
			SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
			break;
		case IDC_BUTTON3:
			Length = QUARTER - onelesspixel;
			ChangeLength(hwndDlg, QUARTER, hwndStart, hwndEnd);
			SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
			break;
		case IDC_BUTTON4:
			Length = EIGHTH - onelesspixel;
			ChangeLength(hwndDlg, EIGHTH, hwndStart, hwndEnd);
			SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
			break;
		case IDC_BUTTON5:
			Length = SIXTEENTH - onelesspixel;
			ChangeLength(hwndDlg, SIXTEENTH, hwndStart, hwndEnd);
			SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
			break;
		case IDC_BUTTON6:
			Length = THIRTYSECOND - onelesspixel;
			ChangeLength(hwndDlg, THIRTYSECOND, hwndStart, hwndEnd);
			SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
			break;
		case IDC_BUTTON7:
			if (Length == 0)
				Length = Event[thisX].pixelsinnote;
			Length += onelesspixel;
			ChangeLength(hwndDlg, Length + (Length/2) - onelesspixel, hwndStart, hwndEnd);
			Length = 0;
			SendMessage(hwndDlg, WM_COMMAND, IDOK, 0);
			break;
		case IDC_BUTTON8:
			if (DialogBox(hInst, "MASTERVOLUME", hwndDlg, MasterVolumeProc)) {
				for (x = thisX; x < (int)e; x++) {
					if ((Event[x].channel == Event[thisX].channel) && (Event[x].velocity)) {
						if ((Event[x].channel == 9) && (!EWQL) && (Event[x].note != Event[thisX].note))
							continue;
						New = Event[x].velocity;
						if (Increase)
							New += Increase;
						if (Decrease)
							New -= Decrease;
						if (New > 127)
							Event[x].velocity = 127;
						else if (New <= 0)
							Event[x].velocity = 1;
						else
							Event[x].velocity = New;
					}
				}
				SetWindowText(hwndVolume, _itoa(Event[thisX].velocity, temp, 10));
				SetFocus(hwndDlg);
			}
			break;
		case IDC_BUTTON9:// Show in List View
			for (x = 0; x < (int)e; x++) {
				if ((Event[thisX].pixel == Event[EventIndex[x]].pixel) && (Event[thisX].message == Event[EventIndex[x]].message)) {
					SendMessage(hwndEditList, LB_SETCURSEL, x, 0);
					break;
				}
			}
			SetFocus(hwndDlg);
			break;

		case IDC_BUTTON10:// Change All Beats/Minute
			if (DialogBox(hInst, "ALLBEATSPERMINUTE", hwndDlg, AllBeatsPerMinuteProc)) {
				InitialBeatsPerMinute += BPMchange;
				if (InitialBeatsPerMinute > 500)
					InitialBeatsPerMinute = 500;
				if (InitialBeatsPerMinute < 24)
					InitialBeatsPerMinute = 24;
				for (x = 0; x < (int)e; x++) {
					if (Event[x].dMilliSecondsPerTick) {
						dBeatsPerMinute = 60000.0 / (Event[x].dMilliSecondsPerTick * (double)TicksPerQuarterNote);
						d2 = modf(dBeatsPerMinute, &d3);// d2 is decimal part and d3 is integer part
						if (d2 > 0.50)
							d3++;
						dBeatsPerMinute = (double)(d3 + BPMchange);
						if (dBeatsPerMinute > 500.0)
							dBeatsPerMinute = 500.0;
						else if ((dBeatsPerMinute) && (dBeatsPerMinute < 24.0))
							dBeatsPerMinute = 24.0;
						Event[x].dMilliSecondsPerTick = 60000.0 / (dBeatsPerMinute * TicksPerQuarterNote);
						if ((Event[x].tickptr == thisTickptr)) {
							_itoa((int)dBeatsPerMinute, textBPM, 10);
							SetWindowText(hwndBPM, textBPM);
						}
					}
				}
				SetFocus(hwndDlg);
			}
			break;

		case IDC_BUTTON11:// Change Instrument
			channel = Event[thisX].channel;
			DialogBox(hInst, "INSTRUM", hwnd, InstrumProc);
			SetFocus(hwndDlg);
			break;

		case IDC_BUTTON12: // ?
			MessageBox(hwndDlg, "If you enter a finger number to use (for piano students),\nand Show Fingers is checked in Options, it'll show on the note.\n\nFinger number examples: 1, 2, 3, 4, 5, L5, R5, 5L, 5R.", "Fingers", MB_OK);
			break;

		case IDOK:
			if (0 == GetWindowText(hwndBPM, textBPM, 10)) {
				SetFocus(hwndBPM);
				break;
			}
			if (0 == GetWindowText(hwndVolume, textVolume, 10)) {
				SetFocus(hwndVolume);
				break;
			}
			GetWindowText(hwndStart, textStart, 10);
			if (0 == GetWindowText(hwndEnd, textEnd, 10)) {
				SetFocus(hwndEnd);
				break;
			}
			GetWindowText(hwndFinger, Event[thisX].finger, 3);
			start = Atoi(textStart);
			end = Atoi(textEnd);
			if (start >= end)
				break;
			if (end < (start+2)) // 4
				end = start + 2; // 4
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK Percussion3Proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int x, y, z, changing, OldNote, LineSpacing;
	static HWND hwndEdit;
	static HWND hwndRadio[4];
	static HWND hwndButton[62];
	static RECT Prect;
	static HDC hdc;
	static PAINTSTRUCT ps;
	static int radios[4] = {IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4};
	static int buttons[] = {IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4, IDC_BUTTON5, IDC_BUTTON6, IDC_BUTTON7, IDC_BUTTON8, IDC_BUTTON9, IDC_BUTTON10, IDC_BUTTON11, IDC_BUTTON12, IDC_BUTTON13, IDC_BUTTON14, IDC_BUTTON15, IDC_BUTTON16, IDC_BUTTON17, IDC_BUTTON18, IDC_BUTTON19, IDC_BUTTON20, IDC_BUTTON21, IDC_BUTTON22, IDC_BUTTON23, IDC_BUTTON24, IDC_BUTTON25, IDC_BUTTON26, IDC_BUTTON27, IDC_BUTTON28, IDC_BUTTON29, IDC_BUTTON30, IDC_BUTTON31, IDC_BUTTON32, IDC_BUTTON33, IDC_BUTTON34, IDC_BUTTON35, IDC_BUTTON36, IDC_BUTTON37, IDC_BUTTON38, IDC_BUTTON39, IDC_BUTTON40, IDC_BUTTON41, IDC_BUTTON42, IDC_BUTTON43, IDC_BUTTON44, IDC_BUTTON45, IDC_BUTTON46, IDC_BUTTON47, IDC_BUTTON48, IDC_BUTTON49, IDC_BUTTON50, IDC_BUTTON51, IDC_BUTTON52, IDC_BUTTON53, IDC_BUTTON54, IDC_BUTTON55, IDC_BUTTON56, IDC_BUTTON57, IDC_BUTTON58, IDC_BUTTON59, IDC_BUTTON60, IDC_BUTTON61, IDC_BUTTON62};

	if (message == WM_INITDIALOG) {
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		for (x = 0; x < 4; x++) {
			hwndRadio[x] = GetDlgItem(hwndDlg, radios[x]);
			if (x == DrumSet) {
				CheckDlgButton (hwndDlg, radios[x], BST_CHECKED);
				break;
			}
		}
		ChannelInstruments[9][0] = 30 + (DrumSet*10);
		Note = 0;
		changing = 0;
		if (myPercussion) {
			for (x = 0; x < 61; x++) {
				hwndButton[x] = GetDlgItem(hwndDlg, buttons[x]);
		 		SetWindowText(hwndButton[x], Percussion[x]);
			}
		}
	}
	else if (message == WM_COMMAND) {
		if (wParam == IDOK)
			EndDialog (hwndDlg, TRUE);
		else if (wParam == IDCANCEL)
			EndDialog (hwndDlg, FALSE);
		else {
			if (IDC_BUTTON63 == (int)wParam) {// HELP
				Help = PercussionHelp;
				DialogBox(hInst, "PERCUSSIONHELP", hwnd, Help2Proc);
				SetFocus(hwndDlg);
			}
			else if (IDC_BUTTON64 == (int)wParam) {
				changing = 1;
				MessageBox(hwndDlg, "Click on a Percussion Instrument to change FROM.\n\nThis Percussion Instrument is presumed\nto have been entered on the staffs already.", "Change a Percussion Instrument", MB_OK);
			}
			else if ((IDC_BUTTON65 == (int)wParam) && (Note)) {// Select the Percussion Instrument clicked-on to use as a beat
				MessageBox(hwndDlg, "selected for the beat", Percussion[Note-27], MB_OK);
				PercussionNote = Note;
			}
			else {
				for (x = 0; x < 4; x++) {
					if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, radios[x])) {
						ChosenDrumSet = x;
						DrumSet = ChosenDrumSet;
						ChannelInstruments[channel][0] = 30 + (DrumSet*10);
						midiOutShortMsg(hMidiOut, 0xC9 | ((30 + (DrumSet*10)) << 8));
						break;
					}
				}
				for (x = 0; x < 62; x++) {
					if (buttons[x] == (int)wParam) {
						SetFocus(hwndDlg);
						Note = x + 27;// G is lowest percussion instrument (right now)
						midiOutShortMsg(hMidiOut, 0x099 | (Volume << 16) | (Note << 8));
						TempEvent.message = 0x99 | (Note << 8);
						GetTicksInNote();
						time = (int)(dMilliSecondsPerTick * TempEvent.ticksinnote);
						timeBeginPeriod(TIMER_RESOLUTION); 
						uTimer3ID = timeSetEvent(time, TIMER_RESOLUTION, TimerFunc7, TempEvent.message, TIME_ONESHOT);// end the note
						if (changing == 2) {
							if (Note != 88) {// not none
								for (x = 0; x < (int)e; x++) {
									if ((Event[x].channel == 9) && (Event[x].note == OldNote))
										Event[x].note = Note;
								}
							}
							else {// delete the percussion instrument's notes
								for (x = 0; x < (int)e; ) {
									if ((Event[x].channel == 9) && (Event[x].note == OldNote)) {
										for (y = x; y < (int)e; y++)
											Event[y] = Event[y+1];
										e--;
									}
									else
										x++;
								}	
								FillRect(hdcMem, &rect, hBrush);
								InvalidateRect(hwnd, &rect, FALSE);
							}
							MessageBox(hwndDlg, "Done", "", MB_OK);
							changing = 0;
						}
						else if (changing == 1) {
							for (x = 0; x < (int)e; x++) {
								if ((Event[x].channel == 9) && (Event[x].note == Note)) {
									OldNote = Note;
									changing = 2;
									MessageBox(hwndDlg, "Click on a Percussion Instrument to change TO.", "Change a Percussion Instrument", MB_OK);
									break;
								}
							}
							if (changing != 2) {
								changing = 0;
								MessageBox(hwndDlg, "hasn't been written(entered)\nso how can it be changed?", Percussion[Note-27], MB_OK);
							}
						}
						break;
					}
				}
			}
		}
	}
	return 0;
}

int CALLBACK PercussionProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int x, y, z, changing, OldNote, LineSpacing;
	static HWND hwndEdit;
	static HWND hwndRadio[4];
	static HWND hwndButton[62];
	static RECT Prect;
	static HDC hdc;
	static PAINTSTRUCT ps;
	static int radios[4] = {IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4};
	static int buttons[] = {IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4, IDC_BUTTON5, IDC_BUTTON6, IDC_BUTTON7, IDC_BUTTON8, IDC_BUTTON9, IDC_BUTTON10, IDC_BUTTON11, IDC_BUTTON12, IDC_BUTTON13, IDC_BUTTON14, IDC_BUTTON15, IDC_BUTTON16, IDC_BUTTON17, IDC_BUTTON18, IDC_BUTTON19, IDC_BUTTON20, IDC_BUTTON21, IDC_BUTTON22, IDC_BUTTON23, IDC_BUTTON24, IDC_BUTTON25, IDC_BUTTON26, IDC_BUTTON27, IDC_BUTTON28, IDC_BUTTON29, IDC_BUTTON30, IDC_BUTTON31, IDC_BUTTON32, IDC_BUTTON33, IDC_BUTTON34, IDC_BUTTON35, IDC_BUTTON36, IDC_BUTTON37, IDC_BUTTON38, IDC_BUTTON39, IDC_BUTTON40, IDC_BUTTON41, IDC_BUTTON42, IDC_BUTTON43, IDC_BUTTON44, IDC_BUTTON45, IDC_BUTTON46, IDC_BUTTON47, IDC_BUTTON48, IDC_BUTTON49, IDC_BUTTON50, IDC_BUTTON51, IDC_BUTTON52, IDC_BUTTON53, IDC_BUTTON54, IDC_BUTTON55, IDC_BUTTON56, IDC_BUTTON57, IDC_BUTTON58, IDC_BUTTON59, IDC_BUTTON60, IDC_BUTTON61, IDC_BUTTON62};

	if (message == WM_INITDIALOG) {
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		for (x = 0; x < 4; x++) {
			hwndRadio[x] = GetDlgItem(hwndDlg, radios[x]);
			if (x == DrumSet) {
				CheckDlgButton (hwndDlg, radios[x], BST_CHECKED);
				break;
			}
		}
		ChannelInstruments[9][0] = 30 + (DrumSet*10);
		Note = 0;
		changing = 0;
		if (myPercussion) {
			for (x = 0; x < 62; x++) {
				hwndButton[x] = GetDlgItem(hwndDlg, buttons[x]);
		 		SetWindowText(hwndButton[x], Percussion[x]);
			}
		}
	}
	else if (message == WM_COMMAND) {
		if (wParam == IDOK)
			EndDialog (hwndDlg, TRUE);
		else if (wParam == IDCANCEL)
			EndDialog (hwndDlg, FALSE);
		else {
			if (IDC_BUTTON63 == (int)wParam) {// HELP
				Help = PercussionHelp;
				DialogBox(hInst, "PERCUSSIONHELP", hwnd, Help2Proc);
				SetFocus(hwndDlg);
			}
			else if (IDC_BUTTON64 == (int)wParam) {
				changing = 1;
				MessageBox(hwndDlg, "Click on a Percussion Instrument to change FROM.\n\nThis Percussion Instrument is presumed\nto have been entered on the staffs already.", "Change a Percussion Instrument", MB_OK);
			}
			else if ((IDC_BUTTON65 == (int)wParam) && (Note)) {// Select the Percussion Instrument clicked-on to use as a beat
				MessageBox(hwndDlg, "selected for the beat", Percussion[Note-27], MB_OK);
				PercussionNote = Note;
			}
			else {
				for (x = 0; x < 4; x++) {
					if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, radios[x])) {
						ChosenDrumSet = x;
						DrumSet = ChosenDrumSet;
						ChannelInstruments[channel][0] = 30 + (DrumSet*10);
						midiOutShortMsg(hMidiOut, 0xC9 | ((30 + (DrumSet*10)) << 8));
						break;
					}
				}
				for (x = 0; x < 62; x++) {
					if (buttons[x] == (int)wParam) {
						Note = x + 27;// G is lowest percussion instrument (right now)
						midiOutShortMsg(hMidiOut, 0x099 | (Volume << 16) | (Note << 8));
						TempEvent.message = 0x99 | (Note << 8);
//						if (Note != 72)// Long Whistle
//							TempEvent.ticksinnote = 5 * TicksPerQuarterNote / 40;
//						else
//							TempEvent.ticksinnote = 40 * TicksPerQuarterNote / 40;
						GetTicksInNote();
						time = (int)(dMilliSecondsPerTick * TempEvent.ticksinnote);
						timeBeginPeriod(TIMER_RESOLUTION); 
						uTimer3ID = timeSetEvent(time, TIMER_RESOLUTION, TimerFunc7, TempEvent.message, TIME_ONESHOT);// end the note
						if (changing == 2) {
							if (Note != 88) {// not none
								for (x = 0; x < (int)e; x++) {
									if ((Event[x].channel == 9) && (Event[x].note == OldNote))
										Event[x].note = Note;
								}
							}
							else {// delete the percussion instrument's notes
								for (x = 0; x < (int)e; ) {
									if ((Event[x].channel == 9) && (Event[x].note == OldNote)) {
										for (y = x; y < (int)e; y++)
											Event[y] = Event[y+1];
										e--;
									}
									else
										x++;
								}	
								FillRect(hdcMem, &rect, hBrush);
								InvalidateRect(hwnd, &rect, FALSE);
							}
							MessageBox(hwndDlg, "Done", "", MB_OK);
							changing = 0;
						}
						else if (changing == 1) {
							for (x = 0; x < (int)e; x++) {
								if ((Event[x].channel == 9) && (Event[x].note == Note)) {
									OldNote = Note;
									changing = 2;
									MessageBox(hwndDlg, "Click on a Percussion Instrument to change TO.", "Change a Percussion Instrument", MB_OK);
									break;
								}
							}
							if (changing != 2) {
								changing = 0;
								MessageBox(hwndDlg, "hasn't been written(entered)\nso how can it be changed?", Percussion[Note-27], MB_OK);
							}
						}
						break;
					}
				}
			}
		}
	}
	else if (message == WM_PAINT) {
		GetClientRect(hwndEdit, &Prect);
		LineSpacing = Prect.bottom / 21;
		hdc = BeginPaint(hwndEdit, &ps);
		z = LineSpacing + 4;
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // E5
		}
		z += LineSpacing;
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // C5
		}
		z += LineSpacing;
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // A5
		}
		z += (6 * LineSpacing);
		for (y = (LineSpacing * 4) + 4; y < z; y += 16) {//68
			MoveToEx(hdc, 0, y, NULL);
			LineTo(hdc, Prect.right, y); // Treble Clef
		}
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // C4
		}
		z += LineSpacing;
		y = z;
		z += (5 * LineSpacing);
		for ( ; y < z; y += 16) {
			MoveToEx(hdc, 0, y, NULL);
			LineTo(hdc, Prect.right, y); // Bass Clef
		}
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // E3
		}
		z += LineSpacing;
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // C3
		}
		z += LineSpacing;
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // A3
		}
		z += LineSpacing;
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // F2
		}
		z += LineSpacing;
		for (x = 0; x < Prect.right; x += 40) {
			MoveToEx(hdc, x, z, NULL);
			LineTo(hdc, x+10, z); // D2
		}
		EndPaint(hwndEdit, &ps);
		return 0;
	}
	return 0;
}

void PlayInstrum(void)
{
	midiOutShortMsg(hMidiOut, (0xB0|channel) | (123 << 8));// All Channel Notes Off
	midiOutShortMsg(hMidiOut, (0xC0|channel) | (InstrumentNum << 8));
	midiOutShortMsg(hMidiOut, (0x90|channel) | (Volume << 16) | (60 << 8));// start the note (middle C)
	TempEvent.message = (0x90|channel) | (60 << 8);// Note Off
	if (uTimer3ID) {
		timeBeginPeriod(TIMER_RESOLUTION);
		uTimer3ID = timeSetEvent(0, TIMER_RESOLUTION, TimerFunc3, TempEvent.message, TIME_ONESHOT);// end the note now
	}
	if (uTimer7ID) {
		timeBeginPeriod(TIMER_RESOLUTION);
		uTimer7ID = timeSetEvent(0, TIMER_RESOLUTION, TimerFunc7, TempEvent.message, TIME_ONESHOT);// end the note now
	}
	if (uTimer8ID) {
		timeBeginPeriod(TIMER_RESOLUTION);
		uTimer8ID = timeSetEvent(0, TIMER_RESOLUTION, TimerFunc8, TempEvent.message, TIME_ONESHOT);// end the note now
	}
	time = 500; // (int)(dMilliSecondsPerTick * (double)(TempEvent.ticksinnote));
	StopTimer();
}

int CALLBACK InstrumProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static DWORD x, y, i;
	static int ItemHeight;
	static char Buffer[128];
	static HWND hwndInstrumList;
	static TEXTMETRIC tm;
	static RECT rect;

	switch (message)
	{
	case WM_INITDIALOG:
		if ((channel != 9) || (EWQL))
			GetTicksInNote();// get TempEvent.ticksinnote
		else
			TempEvent.ticksinnote = 5 * TicksPerQuarterNote / 40;
		hB3 = hB2;
		for (y = 0; y < 121; y += 8) {
			if (hB3 == hB1)
				hB3 = hB2;
			else
				hB3 = hB1;
			for (x = y; x < (y+8); x++)
				Brushes[x] = hB3;
		}
		hwndInstrumList = GetDlgItem(hwndDlg, IDC_LIST1);
		for (x = 0; x < 129; x++)// includes "none"
			SendMessage(hwndInstrumList, LB_ADDSTRING, 0, (LPARAM)myInstruments[x]);
		InstrumentNum = ChannelInstruments[channel][0];
		if (InstrumentNum == 0xFF)
			InstrumentNum = 0;
		SendMessage(hwndInstrumList, LB_SETCURSEL, (WPARAM)InstrumentNum, 0);
		ItemHeight = SendMessage(hwndInstrumList, LB_GETITEMHEIGHT, 0, 0);
		SetFocus(hwndInstrumList);
		if (InstrumentNum != 128)
			PlayInstrum();
		break;

	case WM_DRAWITEM:
		lpdis = (LPDRAWITEMSTRUCT)lParam;
		if ((lpdis->itemAction == ODA_SELECT) || (lpdis->itemAction == ODA_DRAWENTIRE)) {
			SendMessage(lpdis->hwndItem, LB_GETTEXT, lpdis->itemID, (LPARAM)Buffer);
			if (lpdis->itemState & ODS_SELECTED) {
				FillRect(lpdis->hDC, &lpdis->rcItem, hBlueBrush);
				SetTextColor(lpdis->hDC, WHITE);
			}
			else {
				FillRect(lpdis->hDC, &lpdis->rcItem, Brushes[lpdis->itemID]);
				SetTextColor(lpdis->hDC, BLACK);
			}
			SetBkMode(lpdis->hDC, TRANSPARENT);
			TextOut(lpdis->hDC, 0, lpdis->rcItem.top + 1, Buffer, strlen(Buffer)); 
			SetBkMode(lpdis->hDC, OPAQUE);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LIST1:
			if (HIWORD (wParam) == LBN_DBLCLK)
				SendMessage(hwndDlg, WM_COMMAND, (WPARAM)IDOK, 0);
			else if (HIWORD (wParam) == LBN_SELCHANGE) {
				InstrumentNum = SendMessage(hwndInstrumList, LB_GETCURSEL, 0, 0);
				if (InstrumentNum != 128)
					PlayInstrum();
			}
			break;

		case IDOK:
			SendMessage(hwndInstrumList, LB_GETTEXT, InstrumentNum, (LPARAM)myInstruments[InstrumentNum]);
			if (InstrumentNum < 128) {
				if (thisX == -1) { // flag from InstrumentsProc
					ChannelInstruments[channel][0] = InstrumentNum;
					for (y = 0; (Event[y].velocity == 0) && (y < e); y++) {
						if ((Event[y].message & 0xFF) == (DWORD)(0xC0|channel)) {
							if (((Event[y].message >> 8) & 0xFF) != (DWORD)InstrumentNum) {
								Event[y].message &= 0xFF;
								Event[y].message |= (InstrumentNum << 8);
							}
							break;
						}
					}
					if ((Event[y].velocity != 0) || (y == e)) {// not found or not before a note played
						for (y = 0; ((Event[y].message & 0xF0) != 0xC0) && (Event[y].velocity == 0) && (y < e); y++)
							;
						for (i = e; i >= y; i--)
							Event[i+1] = Event[i];
						Event[y].pixel = 0;
						Event[y].tickptr = 0;
						Event[y].message = (0xC0|channel) | (InstrumentNum << 8);
						Event[y].channel = channel;
						Event[y].port = 0;
						Event[y].pixelsinnote = 0;//in case Event[y].tickptr != 0
						Event[y].ticksinnote = 0;
						Event[y].dMilliSecondsPerTick = 0;
						Event[y].note = 0;
						Event[y].velocity = 0;
						Event[y].sharporflat = 0;
						Event[y].time = 0;
						Event[y].BeatsPerMeasure = 0;
						Event[y].BeatNoteType = 0;
						Event[y].KeySignature = 0;
						Event[y].type = 0;
						Event[y].len = 0;
						Event[y].ptr = 0;
						Event[y].finger[0] = 0;
						Event[y].overlapped = 0;
						e++;
					}
				}
				else { // from ParametersProc
					for (y = e; y > (DWORD)thisX; y--)
						Event[y] = Event[y-1]; // open a space in front of note
					Event[thisX].message = (0xC0|channel) | (InstrumentNum << 8); // Program Change
					Event[thisX].note = 0;
					Event[thisX].velocity = 0;
					e++;
					for (y = 0; (ChannelInstruments[channel][y] != 0xFF) && (y < 16); y++)
						;
					ChannelInstruments[channel][y] = InstrumentNum;
					InstrumentOffset[channel][y] = Event[thisX].pixel;
				}
			}
			else
				ChannelInstruments[channel][0] = 128; // none
			EndDialog (hwndDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			break;
		}
	}
	return 0;
}

void ChangeLocationsAndInstruments(HWND hSlider[16])
{
	DWORD y, i;
	BYTE x;

	if ((ActiveChannel != OrigActiveChannel) || (ActiveInstrument != OrigActiveInstrument))
		midiOutShortMsg(hMidiOut, (0xC0|ActiveChannel) | (ActiveInstrument << 8));
	for (x = 0; x < 16; x++) {
		y = SendMessage(hSlider[x], TBM_GETPOS, 0, 0);
		z = y * 0x10;
		if (z == 0x80)
			z = 0x7F;
		if (z != StereoLocations[x]) {// new Stereo Location
			StereoLocations[x] = (BYTE)z;
			midiOutShortMsg(hMidiOut, (0xB0|x) | (z << 16) | (10 << 8));

			for (y = 0; (Event[y].tickptr == 0) && (y < e); y++) {
				if ((Event[y].message & 0xFFFF) == (DWORD)(0x0AB0|x)) {
						if (((Event[y].message >> 16) & 0xFF) != z) {
							if (z != 0x40) {
								Event[y].message &= 0x00FFFF;
								Event[y].message |= (z << 16);
							}
							else {
								for (i = y; i < e; i++)
									Event[i] = Event[i+1];//delete it
								e--;
							}
						}
						break;
				}
			}
			if ((Event[y].tickptr != 0) || (y == e)) {// not found
				for (y = 0; ((Event[y].message & 0xFFF0) != 0x0AB0) && (Event[y].tickptr == 0) && (y < e); y++)
					;
				for (i = e; i >= y; i--)
					Event[i+1] = Event[i];
				Event[y].pixel = 0;
				Event[y].tickptr = 0;
				Event[y].message = (DWORD)(0x0AB0|x) | (z << 16);
				Event[y].channel = x;
				Event[y].port = 0;
				Event[y].pixelsinnote = 0;//in case Event[y].tickptr != 0
				Event[y].ticksinnote = 0;
				Event[y].dMilliSecondsPerTick = 0;
				Event[y].note = 0;
				Event[y].velocity = 0;
				Event[y].sharporflat = 0;
				Event[y].time = 0;
				Event[y].BeatsPerMeasure = 0;
				Event[y].BeatNoteType = 0;
				Event[y].KeySignature = 0;
				Event[y].type = 0;
				Event[y].len = 0;
				Event[y].ptr = 0;
				Event[y].finger[0] = 0;
				Event[y].overlapped = 0;
				e++;
			}
		}
	}
	FillRect(hdcMem, &rect, hBrush);
	InvalidateRect(hwnd, &rect, FALSE);
//	frominstrumentproc = TRUE;
//	SendMessage(hwnd, WM_KEYDOWN, VK_DOWN, 0);// trick
}

int CALLBACK InstrumentsProc(HWND hwndInstruments, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int chan;
	static char Buffer[128];
	static char cPort[4];

	int x, y;
	static int sliders[16] = {IDC_SLIDER1, IDC_SLIDER2, IDC_SLIDER3, IDC_SLIDER4, IDC_SLIDER5, IDC_SLIDER6, IDC_SLIDER7, IDC_SLIDER8, IDC_SLIDER9, IDC_SLIDER10, IDC_SLIDER11, IDC_SLIDER12, IDC_SLIDER13, IDC_SLIDER14, IDC_SLIDER15, IDC_SLIDER16};
	static int buttons[17] = {IDC_BUTTON1, IDC_BUTTON2, IDC_BUTTON3, IDC_BUTTON4, IDC_BUTTON5, IDC_BUTTON6, IDC_BUTTON7, IDC_BUTTON8, IDC_BUTTON9, IDC_BUTTON10, IDC_BUTTON11, IDC_BUTTON12, IDC_BUTTON13, IDC_BUTTON14, IDC_BUTTON15, IDC_BUTTON16};
	static int checks[16] = {IDC_CHECK1, IDC_CHECK2, IDC_CHECK3, IDC_CHECK4, IDC_CHECK5, IDC_CHECK6, IDC_CHECK7, IDC_CHECK8, IDC_CHECK9, IDC_CHECK10, IDC_CHECK11, IDC_CHECK12, IDC_CHECK13, IDC_CHECK14, IDC_CHECK15, IDC_CHECK16};
	static int radios[16] = {IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4, IDC_RADIO5, IDC_RADIO6, IDC_RADIO7, IDC_RADIO8, IDC_RADIO9, IDC_RADIO10, IDC_RADIO11, IDC_RADIO12, IDC_RADIO13, IDC_RADIO14, IDC_RADIO15, IDC_RADIO16};
	static HWND hSlider[16];
	static HWND hwndEdit[16];
	static HWND hwndButton[16];
	static HWND hwndCheck[16];
	static HWND hwndRadio[16];
	static HWND hwndOK;
	static HWND hwndStaticPort, hwndMyPort, hwndHelp;
	static HDC hdc;
	static PAINTSTRUCT ps;

	switch (message)
	{
	case WM_INITDIALOG: // buttons get drawn in case WM_DRAWITEM
		if (EWQL) {
			hwndStaticPort = GetDlgItem(hwndInstruments, IDC_STATIC_PORT);
			ShowWindow(hwndStaticPort, SW_SHOWNORMAL);
			hwndMyPort = GetDlgItem(hwndInstruments, IDC_EDIT1);
			ShowWindow(hwndMyPort, SW_SHOWNORMAL);
			_itoa((Port+1), cPort, 10);
			SendMessage(hwndMyPort, WM_SETTEXT, 0, (LPARAM)&cPort); // causes WM_COMMAND else if ((LOWORD(wParam) == IDC_EDIT1) && (HIWORD(wParam) == EN_CHANGE))
			hwndHelp = GetDlgItem(hwndInstruments, IDC_BUTTON17);
			ShowWindow(hwndHelp, SW_SHOWNORMAL);
			for (channel = 0; myInstruments[channel][0]; channel++) {
				ChannelInstruments[channel][Port] = channel; // to show EWQL instruments
				if (ActiveChannels[channel])
					CheckDlgButton (hwndInstruments, checks[channel], BST_CHECKED);
			}
			for ( ; channel < 16; channel++)
				ChannelInstruments[channel][Port] = 0xFF;
		}
		showinginstruments = TRUE;
		OrigActiveChannel = ActiveChannel;
		OrigActiveInstrument = ChannelInstruments[ActiveChannel][0];
		hwndOK = GetDlgItem(hwndInstruments, IDOK);

		for (channel = 0; channel < 16; channel++) {
			hwndButton[channel] = GetDlgItem(hwndInstruments, buttons[channel]);
			hwndCheck[channel] = GetDlgItem(hwndInstruments, checks[channel]);
			hwndRadio[channel] = GetDlgItem(hwndInstruments, radios[channel]);
			hSlider[channel] = GetDlgItem(hwndInstruments, sliders[channel]);
			SendMessage(hSlider[channel], TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 8));
			SendMessage(hSlider[channel], TBM_SETTICFREQ, (WPARAM)1, (LPARAM)0);
			SendMessage(hSlider[channel], TBM_SETLINESIZE, (WPARAM)0, (LPARAM)1);
			SendMessage(hSlider[channel], TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)1);
			if (ActiveChannels[channel])
				CheckDlgButton (hwndInstruments, checks[channel], BST_CHECKED);
			if (channel == ActiveChannel) {
				CheckDlgButton (hwndInstruments, radios[channel], BST_CHECKED);
				for (x = 0; x < 16; x++)
					if (x != channel)
						CheckDlgButton (hwndInstruments, radios[x], BST_UNCHECKED);
			}
			if (ChannelInstruments[channel][0] == 0xFF)
				StereoLocations[channel] = 0x40;// center
			x = StereoLocations[channel];
			if (x == 0x7F)
				x = 0x80;
			SendMessage(hSlider[channel], TBM_SETPOS, (WPARAM)TRUE, (LPARAM)(x / 0x10));
		}
		SetFocus(hwndRadio[ActiveChannel]);
		break;

	case WM_USER20:
		if (virtualkeyboard) {
			SendMessage(hwndVirtualKeyboard, WM_CLOSE, 0, 0);
			SendMessage(hwnd, WM_COMMAND, ID_VIRTUALKEYBOARD, 0);
			SetFocus(hwndVirtualKeyboard);
		}
		else
			SetFocus(hwnd);
		break;
	case WM_USER21:
		if (Port < 16) {
			Port++;
			_itoa((Port+1), cPort, 10);
			SetWindowText(hwndMyPort, cPort);
		}
		break;

	case WM_USER22:
		if (Port) {
			Port--;
			_itoa((Port+1), cPort, 10);
			SetWindowText(hwndMyPort, cPort);
		}
		break;

	case WM_COMMAND:
		if (wParam == IDC_BUTTON17) // only shows with EWQL
			MessageBox(hwndInstruments, "PageDown to increment the Port,\nor PageUp to decrement it.", "Active Port", MB_OK);
			
		else if ((LOWORD(wParam) == IDC_EDIT1) && (HIWORD(wParam) == EN_CHANGE)) {
			SendMessage(hwndMyPort, EM_SETSEL, 0, -1);
			if (GetWindowText(hwndMyPort, cPort, 10)) {
				Port = (BYTE)(Atoi(cPort)) - 1;
				if (Port < NumberOfPorts) {
					for (x = 0; x < 130; x++)
						myInstruments[x][0] = 0;
					for (x = 0; x < 128; x++)
						InstrumentRanges[x][0] = 0;
					for (x = 0, y = 0; y < (int)EWQLfileSize[Port]; x++) { // fill myInstruments with Port1 instruments
						ChannelInstruments[x][Port] = x;
						myInstruments[x][0] = ((x+1) / 100) + '0';
						myInstruments[x][1] = (((x+1) % 100) / 10) + '0';
						myInstruments[x][2] = ((x+1) % 10) + '0';
						myInstruments[x][3] = ' ';
						myInstruments[x][4] = ' ';
						for (z = 5; (y < (int)EWQLfileSize[Port]) && (InstrumentBufs[Port][y] != '(') && (InstrumentBufs[Port][y] != 0) && (InstrumentBufs[Port][y] != '\r'); y++, z++)
							myInstruments[x][z] = InstrumentBufs[Port][y];
						myInstruments[x][z] = 0;

						if (InstrumentBufs[Port][y] == '(') {
							y++; // to instrument ranges
							for (z = 0; (y < (int)EWQLfileSize[Port]) && (InstrumentBufs[Port][y] != ')'); y++, z++) {
								InstrumentRanges[x][z] = InstrumentBufs[Port][y];
							}
							y++; // to '\r'
							InstrumentRanges[x][z] = 0;
						}
						y += 2; // to next line
					}
					InvalidateRect(hwndInstruments, NULL, FALSE);
					for ( ; x < 16; x++)
						ChannelInstruments[x][Port] = 0xFF;
					if ((virtualkeyboard) && (playflag == STOPPED)) {
						SendMessage(hwndVirtualKeyboard, WM_CLOSE, 0, 0);
						SendMessage(hwnd, WM_COMMAND, ID_VIRTUALKEYBOARD, 0);
						SetFocus(hwndRadio[ActiveChannel]);
					}
				}
				else {
					Port--;
					_itoa((Port+1), cPort, 10);
					SetWindowText(hwndMyPort, cPort);
				}
			}
		}

		else if ((LOWORD(wParam) == IDOK) || (LOWORD(wParam) == IDCANCEL)) {
			if ((virtualkeyboard) && (playflag == STOPPED)) {
				SendMessage(hwndVirtualKeyboard, WM_CLOSE, 0, 0);
				SendMessage(hwnd, WM_COMMAND, ID_VIRTUALKEYBOARD, 0);
			}
			ChangeLocationsAndInstruments(hSlider);
			SendMessage(hwndInstruments, WM_CLOSE, 0, 0);
		}

		else if (wParam == IDC_BUTTON18) { // Select All to Show & Play
			for (x = 0; x < 16; x++) {
				ActiveChannels[x] = TRUE;
				CheckDlgButton (hwndInstruments, checks[x], BST_CHECKED);
			}
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
		}
		else if (wParam == IDC_BUTTON19) { // Select None to Show & Play
			for (x = 0; x < 16; x++) {
				ActiveChannels[x] = FALSE;
				CheckDlgButton (hwndInstruments, checks[x], BST_UNCHECKED);
			}
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
		}

		else for (channel = 0; channel < 16; channel++) {
			if (buttons[channel] == (int)wParam) {
				if (!EWQL) { // can't change name on button if EWQL
					if (channel != 9) {// not percussion
						thisX = -1; // flag for InstrumProc
						if (DialogBox(hInst, "INSTRUM", hwndInstruments, InstrumProc)) {
							if (ChannelInstruments[channel][0] != 0xFF) {
								if (ChannelInstruments[channel][0] != 128) {
									CheckDlgButton (hwndInstruments, checks[channel], BST_CHECKED);
									CheckDlgButton (hwndInstruments, radios[channel], BST_CHECKED);
									for (x = 0; x < 16; x++)
										if (x != channel)
											CheckDlgButton (hwndInstruments, radios[x], BST_UNCHECKED);
									ActiveChannel = channel;
									ActiveChannels[channel] = TRUE;
									ActiveInstrument = ChannelInstruments[channel][0];
								}
								else if (ActiveChannel == channel) {// && (ChannelInstruments[channel][0] == 128)
									ActiveChannels[channel] = FALSE;
									for (x = 0; x < 16; x++) {
										if (ChannelInstruments[x][0] < 128) {
											ActiveChannel = x;
											ActiveChannels[x] = TRUE;
											ActiveInstrument = ChannelInstruments[x][0];
											CheckDlgButton (hwndInstruments, checks[channel], BST_UNCHECKED);
											CheckDlgButton (hwndInstruments, radios[channel], BST_UNCHECKED);
											CheckDlgButton (hwndInstruments, checks[x], BST_CHECKED);
											CheckDlgButton (hwndInstruments, radios[x], BST_CHECKED);
											break;
										}
									}
								}
								else if (BST_CHECKED == IsDlgButtonChecked(hwndInstruments, checks[channel])) {// checking box)
									CheckDlgButton (hwndInstruments, checks[channel], BST_UNCHECKED);
									CheckDlgButton (hwndInstruments, radios[channel], BST_UNCHECKED);
									ActiveChannels[channel] = FALSE;
									if (channel == ActiveChannel) {
										for (x = 0; x < 16; x++) {
											if (ActiveChannels[x]) {
												ActiveChannel = x;
												ActiveInstrument = ChannelInstruments[x][0];
												CheckDlgButton (hwndInstruments, checks[x], BST_CHECKED);
												CheckDlgButton (hwndInstruments, radios[x], BST_CHECKED);
												break;
											}
										}
										if (x == 16) {// no active channels
											ActiveChannels[0] = TRUE;
											ActiveChannel = 0;
											ActiveInstrument = 0;// piano
											ChannelInstruments[0][0] = 0;
											CheckDlgButton (hwndInstruments, checks[0], BST_CHECKED);
											CheckDlgButton (hwndInstruments, radios[0], BST_CHECKED);
										}
									}
								}
							}
						}
					}
					else {// if (channel == 9
						if (DialogBox(hInst, "PERCUSSION3", hwndInstruments, Percussion3Proc)) {
							CheckDlgButton (hwndInstruments, checks[channel], BST_CHECKED);
							CheckDlgButton (hwndInstruments, radios[channel], BST_CHECKED);
							for (x = 0; x < 16; x++)
								if (x != channel)
									CheckDlgButton (hwndInstruments, radios[x], BST_UNCHECKED);
							ActiveChannel = channel;
							ActiveChannels[channel] = TRUE;
							ActiveInstrument = ChannelInstruments[channel][0];
						}
					}
					SetFocus(hwndRadio[ActiveChannel]);
				}
				else { // if (EWQL)
					goto radiobutton;
				}
			} // end of if (buttons[channel] == (int)wParam)

			else if (checks[channel] == (int)wParam) {
				if ((ChannelInstruments[channel][0] < 128)) {
					if (BST_CHECKED == IsDlgButtonChecked(hwndInstruments, checks[channel])) {// checking box
						for (x = 0; x < 16; x++)
							CheckDlgButton (hwndInstruments, radios[x], BST_UNCHECKED);
						CheckDlgButton (hwndInstruments, radios[channel], BST_CHECKED);
						ActiveChannel = channel;
						ActiveChannels[channel] = TRUE;
						ActiveInstrument = ChannelInstruments[channel][0];
					}
					else {// un-checking box
						CheckDlgButton (hwndInstruments, radios[channel], BST_UNCHECKED);
						ActiveChannels[channel] = FALSE;
						if (channel == ActiveChannel) {
							for (x = 0; x < 16; x++) {
								if (ActiveChannels[x]) {
									ActiveChannel = x;
									ActiveInstrument = ChannelInstruments[x][0];
									CheckDlgButton (hwndInstruments, checks[x], BST_CHECKED);
									CheckDlgButton (hwndInstruments, radios[x], BST_CHECKED);
									break;
								}
							}
							if (x == 16) {// no active channels
								ActiveChannels[0] = TRUE;
								ActiveChannel = 0;
								ActiveInstrument = 0;// piano
								ChannelInstruments[0][0] = 0;
								CheckDlgButton (hwndInstruments, checks[0], BST_CHECKED);
								CheckDlgButton (hwndInstruments, radios[0], BST_CHECKED);
							}
						}
					}
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
				}
				else// can't check a channel without an instrument
					CheckDlgButton (hwndInstruments, checks[channel], BST_UNCHECKED);
			}

			else if (radios[channel] == (int)wParam) {
radiobutton:if (ChannelInstruments[channel][0] < 128) {
					CheckDlgButton (hwndInstruments, radios[ActiveChannel], BST_UNCHECKED);
					ActiveChannel = channel;
					ActiveChannels[channel] = TRUE;
					if (!EWQL)
						ActiveInstrument = ChannelInstruments[channel][0];
					else
						ActiveInstrument = ChannelInstruments[channel][Port];
					CheckDlgButton (hwndInstruments, checks[channel], BST_CHECKED);
					CheckDlgButton (hwndInstruments, radios[channel], BST_CHECKED);
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);

					if ((virtualkeyboard) && (playflag == STOPPED)) {
						SendMessage(hwndVirtualKeyboard, WM_CLOSE, 0, 0);
						SendMessage(hwnd, WM_COMMAND, ID_VIRTUALKEYBOARD, 0);
					}
					SetFocus(hwndRadio[channel]); // in case it's from goto radiobutton
				}
				else {
					CheckDlgButton (hwndInstruments, radios[ActiveChannel], BST_CHECKED);
					CheckDlgButton (hwndInstruments, radios[channel], BST_UNCHECKED);
				}
			}
		}
		break;

	case WM_DRAWITEM:
		lpdis2 = (LPDRAWITEMSTRUCT)lParam;
		for (chan = 0; (chan < 16) && (wParam != (UINT)buttons[chan]); chan++)
			; // equate channel with button ID
 		hOldPen = SelectObject(lpdis2->hDC, hWhitePen);
		MoveToEx(lpdis2->hDC, lpdis2->rcItem.left, lpdis2->rcItem.bottom, NULL);
		LineTo(lpdis2->hDC, lpdis2->rcItem.left, lpdis2->rcItem.top);
		LineTo(lpdis2->hDC, lpdis2->rcItem.right-2, lpdis2->rcItem.top);
		SelectObject(lpdis2->hDC, hDarkGreyPen);
		MoveToEx(lpdis2->hDC, lpdis2->rcItem.right-2, lpdis2->rcItem.top+2, NULL);
 		LineTo(lpdis2->hDC, lpdis2->rcItem.right-2, lpdis2->rcItem.bottom-2);
		LineTo(lpdis2->hDC, lpdis2->rcItem.left, lpdis2->rcItem.bottom-2);
		SelectObject(lpdis2->hDC, hOldPen);
		MoveToEx(lpdis2->hDC, lpdis2->rcItem.left, lpdis2->rcItem.bottom-1, NULL);
		LineTo(lpdis2->hDC, lpdis2->rcItem.right-1, lpdis2->rcItem.bottom-1);
		LineTo(lpdis2->hDC, lpdis2->rcItem.right-1, lpdis2->rcItem.top);

		SelectObject(lpdis2->hDC, hNullPen);
		SelectObject(lpdis2->hDC, hInstrumentBrush16[chan]);
		Rectangle(lpdis2->hDC, lpdis2->rcItem.left+5, lpdis2->rcItem.top+3, lpdis2->rcItem.right-5, lpdis2->rcItem.bottom-3);
		if ((chan >= 8) && (chan != 9)) {
			SelectObject(lpdis2->hDC, hWhitePen);
			if (chan != 9) {
				for (y = 4; y < (lpdis2->rcItem.bottom-6); y += 2) {
					MoveToEx(lpdis2->hDC, lpdis2->rcItem.left+5, y, NULL);
					LineTo(lpdis2->hDC, lpdis2->rcItem.right-5, y);
				}
				MoveToEx(lpdis2->hDC, 0, y, NULL);
				LineTo(lpdis2->hDC, lpdis2->rcItem.right-5, y);
			}
		}
		if (ChannelInstruments[chan][0] != 0xFF) {
			SetBkMode(lpdis2->hDC, TRANSPARENT);
			if ((chan != 9) || (EWQL))
				TextOut(lpdis2->hDC, 10, lpdis2->rcItem.top+3, &myInstruments[ChannelInstruments[chan][Port]][5], strlen(&myInstruments[ChannelInstruments[chan][Port]][5]));
			else
				TextOut(lpdis2->hDC, 10, lpdis2->rcItem.top+3, "PERCUSSION", 10);
			SetBkMode(lpdis2->hDC, OPAQUE);
		}
		SelectObject(lpdis2->hDC, hOldPen);
		break;
	
	case WM_CLOSE:
		DestroyWindow(hwndInstruments);
		break;

	case WM_DESTROY:
		showinginstruments = FALSE;
		hwndInstruments = NULL;
		break;
	}
	return 0;
}

int CALLBACK OctaveShiftProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	HWND hwndOK;
	switch (message)
	{
	case WM_INITDIALOG:
		CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO5, IDC_RADIO1);
		hwndOK = GetDlgItem(hwndDlg, IDOK);
		SetFocus(hwndOK);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO1))
				AssignedInstruments[ai].octaveshift = 0;
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO2))
				AssignedInstruments[ai].octaveshift = 1;
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO3))
				AssignedInstruments[ai].octaveshift = -1;
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO4))
				AssignedInstruments[ai].octaveshift = 2;
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO5))
				AssignedInstruments[ai].octaveshift = -2;
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

LRESULT CALLBACK VirtualKeyboardProc(HWND hwndVirtualKeyboard, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int x, y, z, xPos, yPos, xBegin, yBegin, xEnd, yEnd, Start, End, smallestStart, smallestEnd, sizeY;
	static int BlackKeys[36] =		 {1,3,4,6,7,8,10,11,13,14,15,17,18,20,21,22,24,25,27,28,29,31,32,34,35,36,38,39,41,42,43,45,46,48,49,50};
	static int BlackKeyNotesfromBlackKeys[36] = {22,25,27,30,32,34,37,39,42,44,46,49,51,54,56,58,61,63,66,68,70,73,75,78,80,82,85,87,90,92,94,97,99,102,104,106};
	static int Widths1[22];
	static int Widths2[22];
	static unsigned char ComputerKeys1[23] = "ASDFGHJKL;'234567890-= "; // space to get size of space for erasing
	static unsigned char ComputerKeys2[23] = "ZXCVBNM,./QWERTYUIOP[] ";
	static unsigned char ComputerKeyCodes1[23];
	static unsigned char ComputerKeyCodes2[23];
	static char Blanks[4] = "    ";
	static char vol[] = "   velocity    ";
	static char TitleBar[128];
	RECT rectKeyboard;
	static HDC hdcMem3;
	static HBITMAP hdcBitmap3;
	static BOOL keyboardfirst, outside;

	switch (message)
	{
	case WM_CREATE:
		keyboardfirst = outside = TRUE;
		xBegin = 0;
		if (velocity == -1)
			velocity = Volume;
		_itoa(velocity, &vol[12], 10);
		GetWindowRect(hwnd, &rectKeyboard); // if PianoRollComposer not full-screen
		MoveWindow(hwndVirtualKeyboard, rectKeyboard.left, rectKeyboard.bottom-Frame-200, rectKeyboard.right-rectKeyboard.left, 200, FALSE);
		GetClientRect(hwndVirtualKeyboard, &rect3); // rect3 is changed to this Window's frame of reference
		if (rect3.right > rect.right)
			rect3.right = rect.right;
		ExtraSpace = (rect3.right % 52) / 2;
		KeyWidth = (rect3.right - (ExtraSpace*2)) / 52;
		hdc3 = GetDC(hwndVirtualKeyboard);
		for (x = 0; x < 23;x++) { // also get " " width
			GetCharWidth32(hdc3, ComputerKeys1[x], ComputerKeys1[x], &Widths1[x]);
			GetCharWidth32(hdc3, ComputerKeys2[x], ComputerKeys2[x], &Widths2[x]);
			ComputerKeyCodes1[x] = (unsigned char)VkKeyScan(ComputerKeys1[x]);
			ComputerKeyCodes2[x] = (unsigned char)VkKeyScan(ComputerKeys2[x]);
			ComputerKeysDown1[x] = 0;
			ComputerKeysDown2[x] = 0;
			ComputerKeysNote1[x] = 0;
			ComputerKeysNote2[x] = 0;
		}
		hdcMem3 = CreateCompatibleDC(hdc3);
		hBitmap3 = CreateCompatibleBitmap(hdc3, rect3.right, rect3.bottom);
		SelectObject(hdcMem3, hBitmap3);
		ReleaseDC(hwndVirtualKeyboard, hdc3);
		VirtualActiveChannel = ActiveChannel;
		break;

	case WM_SETFOCUS:
		hdc3 = GetDC(hwndVirtualKeyboard);
		BitBlt(hdc3, 0, 0, rect3.right, rect3.bottom, hdcMem3, 0, 0, SRCCOPY); // get original screen
		ReleaseDC(hwnd, hdc3);
		InvalidateRect(hwndVirtualKeyboard, & rect3, FALSE);
		break;

	case WM_COMMAND:
		if (LOWORD(wParam) == ID_EXIT)
			DestroyWindow(hwndVirtualKeyboard);
		else if (LOWORD(wParam) == ID_HELP_PERCUS)
			MessageBox(hwndVirtualKeyboard, KeyboardHelp, VirtualKeyboard, MB_OK);
		break;

	case WM_LBUTTONDOWN:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		if ((xPos < (rect3.right - ExtraSpace)) && (yPos > (rect3.top + 20))) {
			if (yPos > rect3.bottom-58) {
				note = WhiteKeyNotes[(xPos - ExtraSpace) / KeyWidth];
			}
			else {
				z = KeyWidth/3;
				for (x = 0, y = ExtraSpace; x < 36; x++) { // 36 black keys
					y = ExtraSpace + (KeyWidth * BlackKeys[x]);
					if ((xPos > (y-z)) && (xPos < (y+z))) {
						note = BlackKeyNotesfromBlackKeys[x];
						break;
					}
				}
				if (x == 36) { // not a black key
					note = WhiteKeyNotes[(xPos - ExtraSpace) / KeyWidth];
				}
			}
			PostMessage(hwnd, WM_USER, (WPARAM)0x90, (LPARAM)(note | (velocity << 8)));
		}
		break;

	case WM_LBUTTONUP:
		PostMessage(hwnd, WM_USER, (WPARAM)0x90, (LPARAM)note);
		break;

	case WM_RBUTTONDOWN: // to Assign Instrument
		xBegin = LOWORD(lParam);
		yBegin = HIWORD(lParam);
		smallestStart = 0xFFFF;
		smallestEnd = 0xFFFF;
		break;

	case WM_RBUTTONUP:
		xEnd = LOWORD(lParam);
		yEnd = HIWORD(lParam);
		if (xBegin > xEnd) {
			x = xBegin;
			xBegin = xEnd;
			xEnd = x;
		}
		for (x = 0; x < 8; x++) { // check for overlap
			if ((xBegin >= AssignedInstruments[x].start) && (xBegin < AssignedInstruments[x].end)) {
				xBegin = 0;
				break;
			}
			if ((xEnd > AssignedInstruments[x].start) && (xEnd <= AssignedInstruments[x].end)) {
				xBegin = 0;
				break;
			}
			if ((xBegin <= AssignedInstruments[x].start) && (xEnd >= AssignedInstruments[x].end)) {
				xBegin = 0;
				break;
			}
		}
		if ((xEnd - xBegin) < KeyWidth)
			xBegin = 0;
		if (xBegin == 0) { // flag
			hdc3 = GetDC(hwndVirtualKeyboard);
			BitBlt(hdc3, 0, 0, rect3.right, rect3.bottom, hdcMem3, 0, 0, SRCCOPY); // get original screen
			ReleaseDC(hwndVirtualKeyboard, hdc3);
			InvalidateRect(hwndVirtualKeyboard, &rect3, FALSE);
			break;
		}
		Start = End = 0;
		for (x = ExtraSpace; x < (KeyWidth*53); x += KeyWidth) { // 52 white keys
			if ((Start == 0) && (x > xBegin))
				Start = x-KeyWidth;
			if ((End == 0) && (x > xEnd))
				End = x;
		}
		if (DialogBox(hInst, "OCTAVESHIFT", hwndVirtualKeyboard, OctaveShiftProc)) {
			strcpy(AssignedInstruments[ai].name, &myInstruments[ActiveInstrument][5]);
			AssignedInstruments[ai].textlength = strlen(&myInstruments[ActiveInstrument][4]) - 1;
			if (AssignedInstruments[ai].octaveshift) {
				x = AssignedInstruments[ai].textlength;
				if (AssignedInstruments[ai].octaveshift >= 1) {
					AssignedInstruments[ai].name[x++] = '+';
					AssignedInstruments[ai].name[x] = AssignedInstruments[ai].octaveshift + '0';
				}
				else {
					AssignedInstruments[ai].name[x++] = '-';
					AssignedInstruments[ai].name[x] = -AssignedInstruments[ai].octaveshift + '0';
				}
				AssignedInstruments[ai].textlength += 2;
			}
			AssignedInstruments[ai].channel = ActiveChannel;
			AssignedInstruments[ai].port = Port;
			AssignedInstruments[ai].start = Start;
			AssignedInstruments[ai].end = End;
			AssignedInstruments[ai].firstnote = WhiteKeyNotes[(xBegin - ExtraSpace) / KeyWidth];
			AssignedInstruments[ai].lastnote = WhiteKeyNotes[(xEnd - ExtraSpace) / KeyWidth];
			hdc3 = GetDC(hwndVirtualKeyboard);
			GetTextExtentPoint32(hdc3, &myInstruments[ActiveInstrument][4], AssignedInstruments[ai].textlength, &size);
			sizeY = size.cy/2;
			if (End-Start > size.cx)
				AssignedInstruments[ai].textstart = Start + ((End-Start)/2) - (size.cx/2);
			else
				AssignedInstruments[ai].textstart = Start;
			if (ai < 7)
				ai++;
		}
		xBegin = 0; // to end assigning instrument to section of keyboard
		BitBlt(hdc3, 0, 0, rect3.right, rect3.bottom, hdcMem3, 0, 0, SRCCOPY); // get original screen
		ReleaseDC(hwndVirtualKeyboard, hdc3);
		InvalidateRect(hwndVirtualKeyboard, &rect3, FALSE);
		break;

	case WM_MOUSEMOVE:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		if (yPos > 20)
			outside = FALSE;
		else
			outside = TRUE;
		if (xBegin) {
			hdc3 = GetDC(hwndVirtualKeyboard);
//			BitBlt(hdc3, 0, 0, rect3.right, rect3.bottom, hdcMem3, 0, 0, SRCCOPY); // get original screen
			MoveToEx(hdc3, xBegin, rect3.bottom-30, NULL);
			LineTo(hdc3, xPos, rect3.bottom-30);
			ReleaseDC(hwndVirtualKeyboard, hdc3);
		}
		break;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(hwndVirtualKeyboard);
		else if (wParam == VK_TAB) {
			if (showinginstruments)
				SetFocus(hwndInstruments);
			else
				SetFocus(hwnd);
		}
		else if ((wParam == VK_DELETE) && (!outside)) {
			for (x = 0; x < 8; x++) {
				if ((xPos >= AssignedInstruments[x].start) && (xPos < AssignedInstruments[x].end)) {
					for ( ; x < 7; x++)
						AssignedInstruments[x] = AssignedInstruments[x+1];
					AssignedInstruments[x].name[0] = 0;
					AssignedInstruments[x].channel = 0;
					AssignedInstruments[x].port = 0;
					AssignedInstruments[x].start = 0;
					AssignedInstruments[x].end = 0;
					AssignedInstruments[x].octaveshift = 0;
					AssignedInstruments[x].textstart = 0;
					AssignedInstruments[x].textlength = 0;
					AssignedInstruments[x].firstnote = 0;
					AssignedInstruments[x].lastnote = 0;
					InvalidateRect(hwndVirtualKeyboard, &rect3, FALSE);
					break;
				}
			}
		}
		else if (wParam == VK_RIGHT) {
			if (KeyboardOffset > 47)
				break;
			hdc3 = GetDC(hwndVirtualKeyboard);
			BitBlt(hdc3, 0, 0, rect3.right, rect3.bottom, hdcMem3, 0, 0, SRCCOPY); // get original screen
			for (x = 0; x < 22; x++) { // erase existing letters/numbers
				TextOut(hdc3, ExtraSpace + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths1[22]*2))), rect3.top+2, Blanks, 4);
				TextOut(hdc3, ExtraSpace + (KeyWidth/2) + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths2[22]*2))), rect3.bottom-18, Blanks, 4);
			}
			KeyboardOffset += 7;
			for (x = 0; x < 22; x++) {
				if ((x == 3) || (x == 6) || (x == 10) || (x == 13) || (x == 17) || (x == 20))
					SetTextColor(hdc3, 0xB0B0B0);
				else if (x < 10)
					SetTextColor(hdc3, 0x4040D0);//reddish
				else 
					SetTextColor(hdc3, 0xFF0000);//blue
				TextOut(hdc3, ExtraSpace + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths1[x]/2))), rect3.top+2, &ComputerKeys1[x], 1);
				// now show bottom computer keys
				if (x < 10)
					SetTextColor(hdc3, 0x4040D0);//reddish
				else 
					SetTextColor(hdc3, 0xFF0000);//blue
				TextOut(hdc3, ExtraSpace + (KeyWidth/2) + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths2[x]/2))), rect3.bottom-18, &ComputerKeys2[x], 1);
			}
			BitBlt(hdcMem3, 0, 0, rect3.right, rect3.bottom, hdc3, 0, 0, SRCCOPY); // save original screen
			ReleaseDC(hwndVirtualKeyboard, hdc3);
			InvalidateRect(hwndVirtualKeyboard, & rect3, FALSE);
		}
		else if (wParam == VK_LEFT) {
			if (KeyboardOffset+19 <= 0)
				break;
			hdc3 = GetDC(hwndVirtualKeyboard);
			BitBlt(hdc3, 0, 0, rect3.right, rect3.bottom, hdcMem3, 0, 0, SRCCOPY); // get original screen
			for (x = 0; x < 22; x++) { // erase existing letters/numbers
				TextOut(hdc3, ExtraSpace + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths1[22]*2))), rect3.top+2, Blanks, 4);
				TextOut(hdc3, ExtraSpace + (KeyWidth/2) + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths2[22]*2))), rect3.bottom-18, Blanks, 4);
			}
			KeyboardOffset -= 7;
			for (x = 0; x < 22; x++) {
				if ((x == 3) || (x == 6) || (x == 10) || (x == 13) || (x == 17) || (x == 20))
					SetTextColor(hdc3, 0xB0B0B0);
				else if (x < 10)
					SetTextColor(hdc3, 0x4040D0);//reddish
				else 
					SetTextColor(hdc3, 0xFF0000);//blue
				TextOut(hdc3, ExtraSpace + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths1[x]/2))), rect3.top+2, &ComputerKeys1[x], 1);
				// now show bottom computer keys
				if (x < 10)
					SetTextColor(hdc3, 0x4040D0);//reddish
				else 
					SetTextColor(hdc3, 0xFF0000);//blue
				TextOut(hdc3, ExtraSpace + (KeyWidth/2) + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths2[x]/2))), rect3.bottom-18, &ComputerKeys2[x], 1);
			}
			BitBlt(hdcMem3, 0, 0, rect3.right, rect3.bottom, hdc3, 0, 0, SRCCOPY); // save original screen
			ReleaseDC(hwndVirtualKeyboard, hdc3);
			InvalidateRect(hwndVirtualKeyboard, &rect3, FALSE);
		}
		else if ((wParam == VK_UP) && (velocity < 127)) {
			velocity++;
			_itoa(velocity, &vol[12], 10);
			InvalidateRect(hwndVirtualKeyboard, &rect3, FALSE);
		}
		else if ((wParam == VK_DOWN) && (velocity > 0)) {
			velocity--;
			_itoa(velocity, &vol[12], 10);
			InvalidateRect(hwndVirtualKeyboard, &rect3, FALSE);
		}

		else { // computer keyboard keys
			if (0x40000000 & lParam)//bit 30
				break;//ignore typematics
			for (x = 0; x < 22; x++) {
				if (wParam == ComputerKeyCodes2[x]) { // white keys
					if ((x+KeyboardOffset+1 >= 0) && (x+KeyboardOffset+1) < 52) {
						ComputerKeysDown2[x] = wParam;
						note = WhiteKeyNotes[x+KeyboardOffset+1];
						ComputerKeysNote2[x] = note;
						PostMessage(hwnd, WM_USER, (WPARAM)0x90, (LPARAM)(note | (velocity << 8)));
					}
					break;
				}
				else if (wParam == ComputerKeyCodes1[x]) { // black keys
					if ((x+KeyboardOffset+1 > 0) && (x+KeyboardOffset+1 < 52) && (BlackKeyNotes[x+KeyboardOffset+1])) {
						ComputerKeysDown1[x] = wParam;
						note = BlackKeyNotes[x+KeyboardOffset+1];
						ComputerKeysNote1[x] = note;
						PostMessage(hwnd, WM_USER, (WPARAM)0x90, (LPARAM)(note | (velocity << 8)));
					}
					break;
				}
			}
		}
		break;

	case WM_KEYUP:
		for (x = 0; x < 22; x++) {
			if (wParam == ComputerKeysDown1[x]) {
				ComputerKeysDown1[x] = 0;
				PostMessage(hwnd, WM_USER, (WPARAM)0x90, (LPARAM)ComputerKeysNote1[x]);
				break;
			}
			else if (wParam == ComputerKeysDown2[x]) {
				ComputerKeysDown2[x] = 0;
				PostMessage(hwnd, WM_USER, (WPARAM)0x90, (LPARAM)ComputerKeysNote2[x]); // WM_KEYDOWN info is erased in WM_USER
				break;
			}
		}
		break;

	case WM_PAINT:
		hdc3 = BeginPaint(hwndVirtualKeyboard, &ps3);
		MoveToEx(hdc3, ExtraSpace, rect3.top+20, NULL);//top+20
		LineTo(hdc3, (ExtraSpace)+(KeyWidth*52), rect3.top+20);
		MoveToEx(hdc3, ExtraSpace, rect3.bottom-20, NULL);
		LineTo(hdc3, (ExtraSpace)+(KeyWidth*52), rect3.bottom-20);
		for (x = ExtraSpace; x <= (rect3.right - ExtraSpace); x += KeyWidth) { // 52 white keys
			MoveToEx(hdc3, x, rect3.top+20, NULL);
			LineTo(hdc3, x, rect3.bottom-20);
		}
		hBrush3 = CreateSolidBrush(BLACK);
		SelectObject(hdc3, hBrush3);
		for (x = 0; x < 36; x++) { // 36 black keys
			y = ExtraSpace + (KeyWidth * BlackKeys[x]);
			Rectangle(hdc3, y-(KeyWidth/3), rect3.top+20, y+(KeyWidth/3), rect3.bottom-62);
		}
		for (x = 0; x < 22; x++) {
			if ((x == 3) || (x == 6) || (x == 10) || (x == 13) || (x == 17) || (x == 20))
				SetTextColor(hdc3, 0xB0B0B0);
			else if (x < 10)
				SetTextColor(hdc3, 0x4040D0);//reddish
			else 
				SetTextColor(hdc3, 0xFF0000);//blue
			TextOut(hdc3, ExtraSpace + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths1[x]/2))), rect3.top+2, &ComputerKeys1[x], 1);
			// now show bottom computer keys
			if (x < 10)
				SetTextColor(hdc3, 0x4040D0);//reddish
			else 
				SetTextColor(hdc3, 0xFF0000);//blue
			TextOut(hdc3, ExtraSpace + (KeyWidth/2) + ((x+KeyboardOffset)*KeyWidth)+((KeyWidth-(Widths2[x]/2))), rect3.bottom-18, &ComputerKeys2[x], 1);
		}
		if (keyboardfirst) {
			keyboardfirst = FALSE;
			BitBlt(hdcMem3, 0, 0, rect3.right, rect3.bottom, hdc3, 0, 0, SRCCOPY); // save original screen
		}
		BitBlt(hdc3, 0, 0, rect3.right, rect3.bottom, hdcMem3, 0, 0, SRCCOPY); // get original screen
		if (InstrumentRanges[ActiveInstrument][0]) {
			x = (InstrumentRanges[ActiveInstrument][0] - '0') * 10;
			x += (InstrumentRanges[ActiveInstrument][1] - '0');
			for (y = 0; WhiteKeyNotes[y] < x; y++)
				;
			if (WhiteKeyNotes[y] == x)
				x = ExtraSpace+(y*(KeyWidth));
			else
				x = ExtraSpace+(y*KeyWidth) - (KeyWidth/3);
			hLimitPen = CreatePen(PS_SOLID, 4, 0xFF); // red vertical line
			hOldPen = SelectObject(hdc3, hLimitPen);
			MoveToEx(hdc3, x, rect3.top+20, NULL);
			LineTo(hdc3, x, rect3.bottom-20);
			SelectObject(hdc3, hOldPen);

			x = (InstrumentRanges[ActiveInstrument][3] - '0') * 10;
			x += (InstrumentRanges[ActiveInstrument][4] - '0');
			if (InstrumentRanges[ActiveInstrument][5] != 0) {
				x *= 10;
				x += (InstrumentRanges[ActiveInstrument][5] - '0');
			}
			for (y = 0; WhiteKeyNotes[y] < x; y++)
				;
			if (WhiteKeyNotes[y] == x)
				x = ExtraSpace+((y+1)*(KeyWidth));
			else
				x = ExtraSpace+(y*KeyWidth) + (KeyWidth/3);
			hLimitPen = CreatePen(PS_SOLID, 4, 0xFF); // red vertical line
			hOldPen = SelectObject(hdc3, hLimitPen);
			MoveToEx(hdc3, x, rect3.top+20, NULL);
			LineTo(hdc3, x, rect3.bottom-20);
			SelectObject(hdc3, hOldPen);
		}
		for (x = 0; x < ai; x++) { // show Assigned Instruments
			MoveToEx(hdc3, AssignedInstruments[x].start, rect3.bottom-28, NULL);
			LineTo(hdc3, AssignedInstruments[x].end, rect3.bottom-28);
			if (AssignedInstruments[x].start) {
				MoveToEx(hdc3, AssignedInstruments[x].start+5, rect3.bottom-28-5, NULL); // arrow
				LineTo(hdc3, AssignedInstruments[x].start, rect3.bottom-28);
				LineTo(hdc3, AssignedInstruments[x].start+5, rect3.bottom-28+6);

				MoveToEx(hdc3, AssignedInstruments[x].end-5, rect3.bottom-28+5, NULL); // arrow
				LineTo(hdc3, AssignedInstruments[x].end, rect3.bottom-28);
				LineTo(hdc3, AssignedInstruments[x].end-5, rect3.bottom-28-6);
				TextOut(hdc3, AssignedInstruments[x].textstart, rect3.bottom-28-sizeY, AssignedInstruments[x].name, AssignedInstruments[x].textlength);
			}
		}
		if ((ActiveChannel != 9) || (EWQL))
			strcpy(TitleBar, &myInstruments[ActiveInstrument][5]);
		else
			strcpy(TitleBar, "PERCUSSION");
		strcat(TitleBar, vol);
		SetWindowText(hwndVirtualKeyboard, TitleBar);
		EndPaint(hwndVirtualKeyboard, &ps3);
		break;

		case WM_CLOSE:
			DestroyWindow(hwndVirtualKeyboard);
			break;

		case WM_DESTROY:
			VirtualActiveChannel = 0xFF; // flag
			if ((keyboardactive) || (playing))
				SendMessage(hwnd, WM_KEYDOWN, VK_ESCAPE, 0);// to stop playing and recording
			virtualkeyboard = FALSE;
			DeleteObject(hdcMem3);
			DeleteObject(hBitmap3);
			if (showinginstruments)
				SetFocus(hwndInstruments);
			break;
	}
	return DefWindowProc(hwndVirtualKeyboard, message, wParam, lParam);
}

int CALLBACK KeyTimeProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int KeySig[] = {IDC_RADIO1,IDC_RADIO2,IDC_RADIO3,IDC_RADIO4,IDC_RADIO5,IDC_RADIO6,IDC_RADIO7,IDC_RADIO8,IDC_RADIO9,IDC_RADIO10,IDC_RADIO11,IDC_RADIO12,IDC_RADIO13};
	static HWND hwndBeatsMeasure;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndBeatsMeasure = GetDlgItem(hwndDlg, IDC_EDIT1);
		_itoa(BeatsPerMeasure, textBeatsMeasure, 10);
		SetWindowText(hwndBeatsMeasure, textBeatsMeasure);
		switch (PixelsPerBeat)
		{
		case 5:
			CheckRadioButton(hwndDlg, IDC_RADIO15, IDC_RADIO19, IDC_RADIO19);
			break;
		case 10:
			CheckRadioButton(hwndDlg, IDC_RADIO15, IDC_RADIO19, IDC_RADIO18);
			break;
		case 20:
			CheckRadioButton(hwndDlg, IDC_RADIO15, IDC_RADIO19, IDC_RADIO17);
			break;
		case 40:
			CheckRadioButton(hwndDlg, IDC_RADIO15, IDC_RADIO19, IDC_RADIO16);
			break;
		case 80:
			CheckRadioButton(hwndDlg, IDC_RADIO15, IDC_RADIO19, IDC_RADIO15);
			break;
		}

		switch (KeySignature)
		{
		case 200:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO1);
			break;
		case 255:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO2);
			break;
		case 254:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO3);
			break;
		case 253:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO4);
			break;
		case 252:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO5);
			break;
		case 251:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO6);
			break;
		case 250:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO7);
			break;
		case 1:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO8);
			break;
		case 2:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO9);
			break;
		case 3:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO10);
			break;
		case 4:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO11);
			break;
		case 5:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO12);
			break;
		case 6:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO13, IDC_RADIO13);
			break;
		}

//		if ((KeySignature >= 250) && (KeySignature <= 255))
//			usingsharp = FALSE;
//		else if ((KeySignature >= 1) && (KeySignature <= 6))
//			usingsharp = TRUE;
		if (usingsharp)
			CheckDlgButton(hwndDlg, IDC_CHECK4, BST_CHECKED);
		else
			CheckDlgButton(hwndDlg, IDC_CHECK4, BST_UNCHECKED);
		if (showkey)
			CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
		if (showtime)
			CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);
		SetFocus(hwndDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (0 == GetWindowText(hwndBeatsMeasure, textBeatsMeasure, 10))
				break;
			BeatsPerMeasure = atoi(textBeatsMeasure);
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO19)) {
				BeatNoteType = 32;
				PixelsPerBeat = 5;
			}
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO18)) {
				BeatNoteType = 16;
				PixelsPerBeat = 10;
			}
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO17)) {
				BeatNoteType = 8;
				PixelsPerBeat = 20;
			}
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO16)) {
				BeatNoteType = 4;
				PixelsPerBeat = 40;
			}
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO15)) {
				BeatNoteType = 2;
				PixelsPerBeat = 80;
			}
			PixelsPerMeasure = BeatsPerMeasure * PixelsPerBeat;
			if ((rect.right / PixelsPerMeasure) > 64) {
				MessageBox(hwndDlg, Limit, ERROR, MB_OK);
				break;
			}
			for (x = 0; x < 13; x++)
				if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, KeySig[x]))
					break;
			KeySignature = Keys[x];
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK1))
				showkey = TRUE;
			else
				showkey = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK2))
				showtime = TRUE;
			else
				showtime = FALSE;
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK OptionalTextProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndName, hwndText, hwndCopyright;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndName = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndCopyright = GetDlgItem(hwndDlg, IDC_EDIT2);
		hwndText = GetDlgItem(hwndDlg, IDC_EDIT3);
		SetWindowText(hwndName, textName);
		if ((textCopyright[0] == 0) && (textCopyright[1] == 0))
			textCopyright[0] = '\xA9';// put copyright symbol there as an aid
		SetWindowText(hwndCopyright, textCopyright);
		SetWindowText(hwndText, textText);
		SetFocus(hwndName);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetWindowText(hwndName, textName, 63);
			GetWindowText(hwndText, textText, 511);
			GetWindowText(hwndCopyright, textCopyright, 63);
			if ((textCopyright[0] == '\xA9') && (textCopyright[1] == 0))// copyright symbol
				textCopyright[0] = 0;// it was just there as an aid
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			if ((textCopyright[0] == '\xA9') && (textCopyright[1] == 0))// copyright symbol
				textCopyright[0] = 0;// it was just there as an aid
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK OptionsProc(HWND hwndOptionsDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
//	static int radio[] = {IDC_RADIO1, IDC_RADIO2};
	static HWND hwndName, hwndVolume, hwndBPM, hwndOptional, hwndOpening, hwndAddedVelocity;
	static HWND hwndConstantVelocity, hwndMidiIn, hwndMidiOut, hwndLineSpace;
	static BOOL save = FALSE;
	static PAINTSTRUCT ps;
	static HDC hdc;

	switch (message)
	{
	case WM_INITDIALOG:
		OrigPixelsBetweenLines = PixelsBetweenLines;
		showingoptions = TRUE;
		if (Midi[0] == 0) {
			hwndOptional = GetDlgItem(hwndOptionsDlg, IDC_BUTTON3);
			ShowWindow(hwndOptional, SW_SHOWNORMAL);
		}
		if (BigText[0]) {
			hwndOpening = GetDlgItem(hwndOptionsDlg, IDC_BUTTON4);
			ShowWindow(hwndOpening, SW_SHOWNORMAL);
		}
		hwndVolume = GetDlgItem(hwndOptionsDlg, IDC_EDIT4);
		hwndBPM = GetDlgItem(hwndOptionsDlg, IDC_EDIT5);
		_itoa(Volume, textVolume, 10);
		SetWindowText(hwndVolume, textVolume);
		_itoa(InitialBeatsPerMinute, textBPM, 10);
		SetWindowText(hwndBPM, textBPM);
		hwndAddedVelocity = GetDlgItem(hwndOptionsDlg, IDC_EDIT1);
		hwndConstantVelocity = GetDlgItem(hwndOptionsDlg, IDC_EDIT2);
		SetWindowText(hwndConstantVelocity, constantVelocity);
		SetWindowText(hwndAddedVelocity, addedVelocity);
		hwndMidiIn = GetDlgItem(hwndOptionsDlg, IDC_EDIT3);
		SetWindowText(hwndMidiIn, MidiIn[DeviceIn]);
		_itoa(PixelsBetweenLines, lineSpace, 10);
		hwndLineSpace = GetDlgItem(hwndOptionsDlg, IDC_EDIT7);
		SetWindowText(hwndLineSpace, lineSpace);
		hwndMidiOut = GetDlgItem(hwndOptionsDlg, IDC_EDIT6);
		SetWindowText(hwndMidiOut, MidiOut[DeviceOut]);

		if (NoteEvery == 5)
			CheckRadioButton(hwndOptionsDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
		else
			CheckRadioButton(hwndOptionsDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);

		if (inwhite)
			CheckRadioButton(hwndOptionsDlg, IDC_RADIO5, IDC_RADIO6, IDC_RADIO6);
		else
			CheckRadioButton(hwndOptionsDlg, IDC_RADIO5, IDC_RADIO6, IDC_RADIO5);
		if (usingsharp)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK4, BST_CHECKED);
		else
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK4, BST_UNCHECKED);
		if (!show)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK1, BST_CHECKED);
		if (showtime)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK2, BST_CHECKED);
		if (showkey)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK3, BST_CHECKED);
		if (shownotenames)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_CHECKED);
		if (showmeasurenumber)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK6, BST_CHECKED);
		if (showvolumes)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_CHECKED);
		if (showbeatsperminute)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK8, BST_CHECKED);
		if (shownameatpointer)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK9, BST_CHECKED);
		if (shownewinstrument)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK10, BST_CHECKED);
		if (showfingers)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK11, BST_CHECKED);
		if (shownumbers)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK12, BST_CHECKED);
		if (onlynotenames)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK13, BST_CHECKED);
		if (showaspace)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK14, BST_CHECKED);
		if (pianoMetronome)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK15, BST_CHECKED);
		if (showports)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK16, BST_CHECKED);
		if (sustenuto)
			CheckDlgButton(hwndOptionsDlg, IDC_CHECK17, BST_CHECKED);
		SetFocus(hwndOptionsDlg);
		break;
 
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_EDIT1:
			GetWindowText(hwndAddedVelocity, addedVelocity, 10);
			AddedVelocity = Atoi(addedVelocity);
			break;

		case IDC_EDIT2:
			GetWindowText(hwndConstantVelocity, constantVelocity, 10);
			ConstantVelocity = Atoi(constantVelocity);
			break;

		case IDC_EDIT4:
			if (HIWORD(wParam) == EN_CHANGE) {
				GetWindowText(hwndVolume, textVolume, 10);
				Volume = atoi(textVolume);
				if (Volume == 0) {
					Volume = 1;
					_itoa(Volume, textVolume, 10);
				}
				if (Volume > 127) {
					Volume = 127;
					_itoa(Volume, textVolume, 10);
				}
			}
			return 0;

		case IDC_EDIT5:
			if (HIWORD(wParam) == EN_CHANGE) {
				GetWindowText(hwndBPM, textBPM, 10);
				InitialBeatsPerMinute = atoi(textBPM);
				if (InitialBeatsPerMinute < 24) {
					InitialBeatsPerMinute = 24;
					_itoa(InitialBeatsPerMinute, textBPM, 10);
				}
				if (InitialBeatsPerMinute > 500) {
					InitialBeatsPerMinute = 500;
					_itoa(InitialBeatsPerMinute, textBPM, 10);
				}
				for (x = 0; x < (int)e; x++) {
					if (Event[x].dMilliSecondsPerTick) {// change first InitialBeatsPerMinute to Options InitialBeatsPerMinute
						Event[x].dMilliSecondsPerTick = 60000.0 / (double)(InitialBeatsPerMinute * TicksPerQuarterNote);
						dMilliSecondsPerTick = Event[x].dMilliSecondsPerTick;
						break;
					}
				}
			}
			return 0;

		case IDC_CHECK4:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK4))
				usingsharp = TRUE;
			else
				usingsharp = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDC_CHECK1:
			if (BST_UNCHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK1))
				show = TRUE;
			else
				show = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK2:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK2))
				showtime = TRUE;
			else
				showtime = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK3:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK3))
				showkey = TRUE;
			else
				showkey = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK5: // Note Names
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK5)) {
				shownotenames = TRUE;
				showvolumes = FALSE;
				showfingers = FALSE;
				shownumbers = FALSE;
				onlynotenames = FALSE;
				showports = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK11, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK12, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK13, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK16, BST_UNCHECKED);
			}
			else
				shownotenames = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK6:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK6))
				showmeasurenumber = TRUE;
			else
				showmeasurenumber = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK7:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK7)) {
				showvolumes = TRUE;
				shownotenames = FALSE;
				showfingers = FALSE;
				shownumbers = FALSE;
				onlynotenames = FALSE;
				showports = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK11, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK12, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK13, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK16, BST_UNCHECKED);
			}
			else
				showvolumes = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK8:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK8))
				showbeatsperminute = TRUE;
			else
				showbeatsperminute = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK9:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK9))
				shownameatpointer = TRUE;
			else
				shownameatpointer = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK10:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK10))
				shownewinstrument = TRUE;
			else
				shownewinstrument = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK11:// showfingers
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK11)) {
				showfingers = TRUE;
				showvolumes = FALSE;
				shownotenames = FALSE;
				shownumbers = FALSE;
				onlynotenames = FALSE;
				showports = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK12, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK13, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK16, BST_UNCHECKED);
			}
			else
				showfingers = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK12:
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK12)) {
				shownumbers = TRUE;
				showvolumes = FALSE;
				shownotenames = FALSE;
				showfingers = FALSE;
				onlynotenames = FALSE;
				showports = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK11, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK13, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK16, BST_UNCHECKED);
			}
			else
				shownumbers = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDC_CHECK13: // Show Note Names Without the Notes
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK13)) {
				onlynotenames = TRUE;
				shownumbers = FALSE;
				showvolumes = FALSE;
				shownotenames = FALSE;
				showfingers = FALSE;
				showports = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK11, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK12, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK16, BST_UNCHECKED);
			}
			else
				onlynotenames = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDC_CHECK14:
			if (showaspace)
				showaspace = FALSE;
			else
				showaspace = TRUE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDC_CHECK15: // Metronome
			Metronome();
			break;
		case IDC_CHECK16: // Ports
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK16)) {
				showports = TRUE;
				onlynotenames = FALSE;
				shownumbers = FALSE;
				showvolumes = FALSE;
				shownotenames = FALSE;
				showfingers = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK11, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK12, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK13, BST_UNCHECKED);
			}
			else
				showports = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDC_CHECK17:
			if (sustenuto)
				sustenuto = FALSE;
			else
				sustenuto = TRUE;
			break;

		case IDC_RADIO1:
			CheckRadioButton(hwndOptionsDlg, IDC_RADIO1, IDC_RADIO4, IDC_RADIO1);
			NoteEvery = 5;
			break;
		case IDC_RADIO2:
			CheckRadioButton(hwndOptionsDlg, IDC_RADIO1, IDC_RADIO4, IDC_RADIO2);
			switch (notetype[1])
			{
				case 'W':
					NoteEvery = 160;
					break;
				case 'H':
					NoteEvery = 80;
					break;
				case 'Q':
					NoteEvery = 40;
					break;
				case 'E':
					NoteEvery = 20;
					break;
				case 'S':
					NoteEvery = 10;
					break;
				case 'T':
					NoteEvery = 5;
					break;
			}
			break;

		case IDC_RADIO5:
			CheckRadioButton(hwndOptionsDlg, IDC_RADIO5, IDC_RADIO6, IDC_RADIO5);
			inwhite = FALSE;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_RADIO6:
			CheckRadioButton(hwndOptionsDlg, IDC_RADIO5, IDC_RADIO6, IDC_RADIO6);
			inwhite = TRUE;
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDC_BUTTON8: // Metronome help
			MessageBox(hwndOptionsDlg, "The metronome plays at the Initial Beats/Minute.\nIt plays the Channel 1 instrument's\nnote number 90 at a volume of 40.\n\nOutside of Options, press Ctrl-M to start/stop it.", "Metronome", MB_OK);
			SetFocus(hwndOptionsDlg);
			break;
		case IDC_BUTTON7:
			MessageBox(hwndOptionsDlg, "The EastWest/Quantum Leap Player\nuses ports to enable more instruments.", "Ports", MB_OK);
			SetFocus(hwndOptionsDlg);
			break;
		case IDC_BUTTON6:
			MessageBox(hwndOptionsDlg, "Keep it at Entered Velocity\n(if a velocity is entered)", "Keyboard Velocity", MB_OK);
			SetFocus(hwndOptionsDlg);
			break;
		case IDC_BUTTON5:
			MessageBox(hwndOptionsDlg, NewInstrumentHelp, "", MB_OK);
			SetFocus(hwndOptionsDlg);
			break;

		case IDC_BUTTON4:
			for (x = 0; (x < 48) && (Filename[x] != '.'); x++)
				Name[x] = Filename[x];
			Name[x] = 0;
			if (BigText[0]) {
				fromoptions = TRUE;
				DialogBox(hInst, "OPENINGTEXT", hwnd, OpeningTextProc);
			}
			break;

		case IDC_BUTTON3:
			if (Midi[0] == 0)
				DialogBox(hInst, "OPTIONALTEXT", hwndOptionsDlg, OptionalTextProc);
			break;

		case IDC_BUTTON2:
			MessageBox(hwndOptionsDlg, "Beats/Minute changes are indicated by black vertical lines over Middle C.\nLook at a following note to see the new value.", "", MB_OK);
			SetFocus(hwndOptionsDlg);
			break;

		case IDC_BUTTON1:// Save to .ini file
			GetWindowText(hwndVolume, textVolume, 10);
			GetWindowText(hwndBPM, textBPM, 10);

			Volume = atoi(textVolume);
			if (Volume == 0) {
				Volume = 1;
				_itoa(Volume, textVolume, 10);
				SetWindowText(hwndVolume, textVolume);
			}
			if (Volume > 127) {
				Volume = 127;
				_itoa(Volume, textVolume, 10);
				SetWindowText(hwndVolume, textVolume);
			}

			InitialBeatsPerMinute = atoi(textBPM);
			if (InitialBeatsPerMinute < 24) {
				InitialBeatsPerMinute = 24;
				_itoa(InitialBeatsPerMinute, textBPM, 10);
				SetWindowText(hwndVolume, textBPM);
			}
			if (InitialBeatsPerMinute > 500) {
				InitialBeatsPerMinute = 500;
				_itoa(InitialBeatsPerMinute, textBPM, 10);
				SetWindowText(hwndVolume, textBPM);
			}

			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK4))
				usingsharp = TRUE;
			else
				usingsharp = FALSE;
			if (BST_UNCHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK1))
				show = TRUE;
			else
				show = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK2))
				showtime = TRUE;
			else
				showtime = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK3))
				showkey = TRUE;
			else
				showkey = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK5)) {
				shownotenames = TRUE;
				showvolumes = FALSE;
				showfingers = FALSE;
				shownumbers = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK11, BST_UNCHECKED);
			}
			else
				shownotenames = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK6))
				showmeasurenumber = TRUE;
			else
				showmeasurenumber = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK7)) {
				showvolumes = TRUE;
				shownotenames = FALSE;
				showfingers = FALSE;
				shownumbers = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK11, BST_UNCHECKED);
			}
			else
				showvolumes = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK8))
				showbeatsperminute = TRUE;
			else
				showbeatsperminute = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK9))
				shownameatpointer = TRUE;
			else
				shownameatpointer = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK10))
				shownewinstrument = TRUE;
			else
				shownewinstrument = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK11)) {
				showfingers = TRUE;
				showvolumes = FALSE;
				shownotenames = FALSE;
				shownumbers = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_UNCHECKED);
			}
			else
				showfingers = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndOptionsDlg, IDC_CHECK12)) {
				shownumbers = TRUE;
				showfingers = FALSE;
				showvolumes = FALSE;
				shownotenames = FALSE;
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK5, BST_UNCHECKED);
				CheckDlgButton(hwndOptionsDlg, IDC_CHECK7, BST_UNCHECKED);
			}
			else
				shownumbers = FALSE;

			for (x = 0, y = 0; IniAccidental[y] != 0; x++, y++)
				IniBuf[x] = IniAccidental[y];
			if (usingsharp)
				IniBuf[x++] = '#';
			else
				IniBuf[x++] = 'b';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShow[y] != 0; x++, y++)
				IniBuf[x] = IniShow[y];
			if (show)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniVolume[y] != 0; y++)
				IniBuf[x++] = IniVolume[y];
			_itoa(Volume, &IniBuf[x], 10);
			for ( ; IniBuf[x] != 0; x++)
				;
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';

			for (y = 0; IniBeatsPerMinute[y] != 0; y++)
				IniBuf[x++] = IniBeatsPerMinute[y];
			_itoa(InitialBeatsPerMinute, &IniBuf[x], 10);
			for ( ; IniBuf[x] != 0; x++)
				;
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';

			for (y = 0; IniNotePixels[y] != 0; y++)
				IniBuf[x++] = IniNotePixels[y];
			if (NoteEvery != 5)
				NoteEvery = 40;
			_itoa(NoteEvery, &IniBuf[x], 10);
			for ( ; IniBuf[x] != 0; x++)
				;
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowKey[y] != 0; y++)
				IniBuf[x++] = IniShowKey[y];
			if (showkey)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowTime[y] != 0; y++)
				IniBuf[x++] = IniShowTime[y];
			if (showtime)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowNoteNames[y] != 0; y++)
				IniBuf[x++] = IniShowNoteNames[y];
			if (shownotenames)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowBPMs[y] != 0; y++)
				IniBuf[x++] = IniShowBPMs[y];
			if (showbeatsperminute)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowMeasureNums[y] != 0; y++)
				IniBuf[x++] = IniShowMeasureNums[y];
			if (showmeasurenumber)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowNoteVelocities[y] != 0; y++)
				IniBuf[x++] = IniShowNoteVelocities[y];
			if (showvolumes)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowNameAtPointer[y] != 0; y++)
				IniBuf[x++] = IniShowNameAtPointer[y];
			if (shownameatpointer)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowNewInstrument[y] != 0; y++)
				IniBuf[x++] = IniShowNewInstrument[y];
			if (shownewinstrument)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowNumbers[y] != 0; y++)
				IniBuf[x++] = IniShowNumbers[y];
			if (shownumbers)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniShowFingers[y] != 0; y++)
				IniBuf[x++] = IniShowFingers[y];
			if (showfingers)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniAddedVelocity[y] != 0; x++, y++)
				IniBuf[x] = IniAddedVelocity[y];
			for (y = 0; addedVelocity[y] != 0; x++, y++)
				IniBuf[x] = addedVelocity[y];
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; IniRecordQuality[y] != 0; x++, y++)
				IniBuf[x] = IniRecordQuality[y];
			IniBuf[x++] = WaveOptionsIndex + '1';
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; MidiInput[y] != 0; x++, y++)
				IniBuf[x] = MidiInput[y];
			for (y = 0; MidiIn[DeviceIn][y] != 0; x++, y++)
				IniBuf[x] = MidiIn[DeviceIn][y];
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; MidiOutput[y] != 0; x++, y++)
				IniBuf[x] = MidiOutput[y];
			for (y = 0; MidiOut[DeviceOut][y] != 0; x++, y++)
				IniBuf[x] = MidiOut[DeviceOut][y];

			if (ConstantVelocity) {
				IniBuf[x++] = '\r';
				IniBuf[x++] = '\n';
				for (y = 0; Constantvelocity[y] != 0; x++, y++)
					IniBuf[x] = Constantvelocity[y];
				for (y = 0; constantVelocity[y] != 0; x++, y++)
					IniBuf[x] = constantVelocity[y];
			}
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';

			for (y = 0; OnlyNoteNames[y] != 0; x++, y++)
				IniBuf[x] = OnlyNoteNames[y];
			if (onlynotenames)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';

			if (PixelsBetweenLines) {
				IniBuf[x++] = '\r';
				IniBuf[x++] = '\n';
				for (y = 0; Pixelsbetweenlines[y] != 0; x++, y++)
					IniBuf[x] = Pixelsbetweenlines[y];
				GetWindowText(hwndLineSpace, lineSpace, 10);
				PixelsBetweenLines = Atoi(lineSpace);
				if (PixelsBetweenLines < 8)
					 PixelsBetweenLines = 8;
				else if (PixelsBetweenLines > 26)
					PixelsBetweenLines = 26;
				_itoa(PixelsBetweenLines, lineSpace, 10);
				for (y = 0; lineSpace[y] != 0; x++, y++)
					IniBuf[x] = lineSpace[y];
				FillRect(hdcMem, &rect, hBrush);
			}

			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			for (y = 0; ShowASpace[y] != 0; x++, y++)
				IniBuf[x] = ShowASpace[y];
			if (showaspace)
				IniBuf[x++] = 'Y';
			else
				IniBuf[x++] = 'N';

			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			if (sysex) {
				for (y = 0; IniSysEx[y] != 0; x++, y++)
					IniBuf[x] = IniSysEx[y];
				for (y = 0; cSysEx[y] != 0; x++, y++)
					IniBuf[x] = cSysEx[y];
				IniBuf[x++] = '\r';
				IniBuf[x++] = '\n';
			}
			if (EWQL) {
				for (y = 0; IniPorts[y] != 0; y++)
					IniBuf[x++] = IniPorts[y];
				IniBuf[x++] = NumberOfPorts + '0';
				IniBuf[x++] = '\r';
				IniBuf[x++] = '\n';
			}
			if (bigopen) {
				for (y = 0; IniBigOpen[y] != 0; y++)
					IniBuf[x++] = IniBigOpen[y];
				IniBuf[x++] = '\r';
				IniBuf[x++] = '\n';
			}
			IniBuf[x] = 0; // for MessageBox, below

			hFile = CreateFile(IniFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
			WriteFile(hFile, IniBuf, x, &dwBytesWritten, NULL);
			CloseHandle(hFile);
			MessageBox(hwndOptionsDlg, IniBuf, "PianoRollComposer.ini", MB_OK);
			SetFocus(hwndOptionsDlg);
			//fall thru...

		case IDOK:
		case IDCANCEL:
			GetWindowText(hwndLineSpace, lineSpace, 10);
			PixelsBetweenLines = Atoi(lineSpace);
			if (PixelsBetweenLines < 8)
				 PixelsBetweenLines = 8;
			else if (PixelsBetweenLines > 26)
				PixelsBetweenLines = 26;
			////////////////////////
			GetPixelsBetweenLines();
			////////////////////////
			if (OrigPixelsBetweenLines != PixelsBetweenLines)
				SendMessage(hwnd, WM_KEYDOWN, VK_HOME, 0);
			SendMessage(hwndOptionsDlg, WM_CLOSE, 0, 0);
FillRect(hdcMem, &rect, hBrush);
InvalidateRect(hwnd, &rect, FALSE);
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndOptionsDlg);
		hwndOptionsDlg = NULL;
		showingoptions = FALSE;
		break;
	}
	return 0;
}

void HiliteKey(HDC hdc3)
{
	if ((ScanCodes[ScanCode] >= 70) && (ScanCodes[ScanCode] <= 81)) {
		x = ScanCodes[ScanCode] - 70;
		Rectangle(hdc3, BoxWidth+(x*BoxWidth), 0, (BoxWidth*2)+(x*BoxWidth)+1, 61);
		TextOut(hdc3, 5 + BoxWidth + (x*BoxWidth), 2, &Keyboard1[x+1], 1);
		TextOut(hdc3, 5 + BoxWidth + (x*BoxWidth), 22, PercInstr1[x+1], strlen(PercInstr1[x+1]));
		TextOut(hdc3, 5 + BoxWidth + (x*BoxWidth), 42, PercInstr1x[x+1], strlen(PercInstr1x[x+1]));
	}
	else if ((ScanCodes[ScanCode] >= 56) && (ScanCodes[ScanCode] <= 67)) {
		x = ScanCodes[ScanCode] - 56;
		Rectangle(hdc3, BoxWidth+(x*BoxWidth), 60, (BoxWidth*2)+(x*BoxWidth)+1, 121);
		TextOut(hdc3, 5 + ((x+1)*BoxWidth), 62, &Keyboard2[x], 1);
		TextOut(hdc3, 5 + ((x+1)*BoxWidth), 82, PercInstr2[x], strlen(PercInstr2[x]));
		TextOut(hdc3, 5 + ((x+1)*BoxWidth), 102, PercInstr2x[x], strlen(PercInstr2x[x]));
	}
	else if ((ScanCodes[ScanCode] >= 45) && (ScanCodes[ScanCode] <= 55)) {
		x = ScanCodes[ScanCode] - 45;
		Rectangle(hdc3, BoxWidth+(BoxWidth/4)+(x*BoxWidth), 120, (BoxWidth*2)+(BoxWidth/4)+(x*BoxWidth)+1, 181);
		TextOut(hdc3, 5 + (BoxWidth/4) + ((x+1)*BoxWidth), 122, &Keyboard3[x], 1);
		TextOut(hdc3, 5 + (BoxWidth/4) + ((x+1)*BoxWidth), 142, PercInstr3[x], strlen(PercInstr3[x]));
		TextOut(hdc3, 5 + (BoxWidth/4) + ((x+1)*BoxWidth), 162, PercInstr3x[x], strlen(PercInstr3x[x]));
	}
	else if ((ScanCodes[ScanCode] >= 35) && (ScanCodes[ScanCode] <= 44)) {
		x = ScanCodes[ScanCode] - 35;
		Rectangle(hdc3, BoxWidth+(BoxWidth*3/4)+(x*BoxWidth), 180, (BoxWidth*2)+(BoxWidth*3/4)+(x*BoxWidth)+1, 241);
		TextOut(hdc3, 5 + (BoxWidth*3/4) + ((x+1)*BoxWidth), 182, &Keyboard4[x], 1);
		TextOut(hdc3, 5 + (BoxWidth*3/4) + ((x+1)*BoxWidth), 202, PercInstr4[x], strlen(PercInstr4[x]));
		TextOut(hdc3, 5 + (BoxWidth*3/4) + ((x+1)*BoxWidth), 222, PercInstr4x[x], strlen(PercInstr4x[x]));
	}
	else if (ScanCodes[ScanCode] == 68) {// |
		Rectangle(hdc3, 13*BoxWidth, 60, (14*BoxWidth)+1, 121);
		TextOut(hdc3, 5 + ((12+1)*BoxWidth), 62, &Keyboard2[12], 1);
		TextOut(hdc3, 5 + ((12+1)*BoxWidth), 82, PercInstr2[12], strlen(PercInstr2[12]));
		TextOut(hdc3, 5 + ((12+1)*BoxWidth), 102, PercInstr2x[12], strlen(PercInstr2x[12]));
	}
	else if (ScanCodes[ScanCode] == 69) {// ~
		Rectangle(hdc3, 0, 0, BoxWidth+1, 61);
		TextOut(hdc3, 5, 2, &Keyboard1[0], 1);
		TextOut(hdc3, 5, 22, PercInstr1[0], strlen(PercInstr1[0]));
		TextOut(hdc3, 5, 42, PercInstr1x[0], strlen(PercInstr1x[0]));
	}
}

LRESULT CALLBACK KeyboardPercusProc(HWND hwndKeyboardPercus, UINT message, WPARAM wParam, LPARAM lParam)
{
	HDC hdc3;
	PAINTSTRUCT ps3;

	switch (message)
	{
	case WM_CREATE:
		keyboardpercus = TRUE;
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_EXIT:
			SendMessage(hwndKeyboardPercus, WM_CLOSE, 0, 0);
			break;
		}
		case ID_HELP_PERCUS:
			MessageBox(hwndKeyboardPercus, "", "WHAT???", MB_OK);
		break;

	case WM_CHAR:
		ScanCode = (lParam >> 16) & 0xFF;
		if (ScanCode == 1)
			SendMessage(hwndKeyboardPercus, WM_CLOSE, 0, 0);
		else if ((ScanCode <= 53) && (ScanCodes[ScanCode] != 0)) {
			PercussionNote = ScanCodes[ScanCode];
			midiOutShortMsg(hMidiOut, 0x99 | (Volume << 16) | (PercussionNote << 8));
			hdc3 = GetDC(hwndKeyboardPercus);
			SelectObject(hdc3, hChords2BlueBrush);
			SetBkMode(hdc3, TRANSPARENT);
			HiliteKey(hdc3);
			SetBkMode(hdc3, OPAQUE);
			ReleaseDC(hwndKeyboardPercus, hdc3);
		}
		break;

	case WM_KEYUP:
		ScanCode = (lParam >> 16) & 0xFF;
		if ((ScanCode <= 53) && (ScanCodes[ScanCode] != 0)) {
			x = ScanCodes[ScanCode] - 70;
			y = 2;
			hdc3 = GetDC(hwndKeyboardPercus);
			SelectObject(hdc3, hDialogBrush);
			SetBkMode(hdc3, TRANSPARENT);
			HiliteKey(hdc3);
			SetBkMode(hdc3, OPAQUE);
			ReleaseDC(hwndKeyboardPercus, hdc3);
		}
		break;

	case WM_PAINT:
		BoxWidth = Rect2.right / 14;
		hdc3 = BeginPaint(hwndKeyboardPercus, &ps3);
		FillRect(hdc3, &Rect2, hDialogBrush);
		SetBkMode(hdc3, TRANSPARENT);
		MoveToEx(hdc3, 0, 60, NULL);
		LineTo(hdc3, BoxWidth * 14, 60);
		for (x = BoxWidth; x < Rect2.right; x += BoxWidth) {
			MoveToEx(hdc3, x, 0, NULL);
			LineTo(hdc3, x, 60);
		}
		x = BoxWidth;
		MoveToEx(hdc3, x, 120, NULL);
		LineTo(hdc3, rect.right, 120);
		for ( ; x < Rect2.right; x += BoxWidth) {
			MoveToEx(hdc3, x, 60, NULL);
			LineTo(hdc3, x, 120);
		}
		for (x = BoxWidth + (BoxWidth/4); x < (BoxWidth * 13); x += BoxWidth) {
			MoveToEx(hdc3, x, 120, NULL);
			LineTo(hdc3, x, 180);
		}
		MoveToEx(hdc3,BoxWidth + (BoxWidth/4), 180, NULL);
		LineTo(hdc3, BoxWidth + (BoxWidth/4) + (BoxWidth * 11), 180);
		for (x = BoxWidth + (BoxWidth*3/4); x < (BoxWidth * 12); x += BoxWidth) {
			MoveToEx(hdc3, x, 180, NULL);
			LineTo(hdc3, x, 240);
		}
		MoveToEx(hdc3,BoxWidth + (BoxWidth*3/4), 240, NULL);
		LineTo(hdc3, BoxWidth + (BoxWidth*3/4) + (BoxWidth * 10), 240);

		for (x = 0; x < 13; x++) {
			TextOut(hdc3, 5 + (x*BoxWidth), 2, &Keyboard1[x], 1);
			TextOut(hdc3, 5 + (x*BoxWidth), 22, PercInstr1[x], strlen(PercInstr1[x]));
			TextOut(hdc3, 5 + (x*BoxWidth), 42, PercInstr1x[x], strlen(PercInstr1x[x]));
		}
		for (x = 0; x < 13; x++) {
			TextOut(hdc3, 5 + ((x+1)*BoxWidth), 62, &Keyboard2[x], 1);
			TextOut(hdc3, 5 + ((x+1)*BoxWidth), 82, PercInstr2[x], strlen(PercInstr2[x]));
			TextOut(hdc3, 5 + ((x+1)*BoxWidth), 102, PercInstr2x[x], strlen(PercInstr2x[x]));
		}
		for (x = 0; x < 11; x++) {
			TextOut(hdc3, 5 + (BoxWidth/4) + ((x+1)*BoxWidth), 122, &Keyboard3[x], 1);
			TextOut(hdc3, 5 + (BoxWidth/4) + ((x+1)*BoxWidth), 142, PercInstr3[x], strlen(PercInstr3[x]));
			TextOut(hdc3, 5 + (BoxWidth/4) + ((x+1)*BoxWidth), 162, PercInstr3x[x], strlen(PercInstr3x[x]));
		}
		for (x = 0; x < 10; x++) {
			TextOut(hdc3, 5 + (BoxWidth*3/4) + ((x+1)*BoxWidth), 182, &Keyboard4[x], 1);
			TextOut(hdc3, 5 + (BoxWidth*3/4) + ((x+1)*BoxWidth), 202, PercInstr4[x], strlen(PercInstr4[x]));
			TextOut(hdc3, 5 + (BoxWidth*3/4) + ((x+1)*BoxWidth), 222, PercInstr4x[x], strlen(PercInstr4x[x]));
		}
		SetBkMode(hdc3, OPAQUE);
		EndPaint(hwndKeyboardPercus, &ps3);
		break;

	case WM_CLOSE:
		DestroyWindow(hwndKeyboardPercus);
		keyboardpercus = FALSE;
		break;
	}
	return DefWindowProc(hwndKeyboardPercus, message, wParam, lParam);
}


int CALLBACK SustainProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG) {
		sustainflag = 1;
		CheckDlgButton(hwndDlg, IDC_RADIO1, BST_CHECKED);// create
	}

	else if (message == WM_COMMAND) {
		if (LOWORD(wParam) == IDC_BUTTON1)
			SendMessage(hwndDlg, WM_CLOSE, 0, 0);
		else if ((LOWORD(wParam) == IDC_RADIO1) && (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO1))) {// create
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
			sustainflag = 1;
		}
		else if ((LOWORD(wParam) == IDC_RADIO2) && (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO2))) {// delete
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
			sustainflag = 2;
		}
	}
	else if (message == WM_CLOSE) {
		DestroyWindow(hwndSustain);
		hwndSustain = NULL;
		sustainflag = 0;
	}
	return 0;
}

void ShowSliders(HWND hwndMixer)
{
	int x, y, z;

	for (x = 0, y = 0, z = 8; x < (int)Entries; x++) {
		if (MixerBuf[x].mixer[0]) { // || (MixerBuf[x].type[0]))
			if ((MixerBuf[x].RecordPlay == 'P') && (0 == strcmp(MixerBuf[x].mixercaps, Playback[indexPlay]))) {
				MixerBuf[x].checkID = checks[y];
				SendMessage(hSlider[y], TBM_SETPOS, 1, 100-MixerBuf[x].Volume);
				ShowWindow(hSlider[y], SW_SHOW);
				ShowWindow(hCheck[y], SW_SHOW);
				Slider[y].hSlider = hSlider[y];
				Slider[y].checks = checks[y];
				Slider[y].mixer = x;
				SetWindowText(hEdit[y], MixerBuf[x].type);
				SetWindowText(hStatic[y], MixerBuf[x].volume);
				if (MixerBuf[x].muteselected)
					CheckDlgButton(hwndMixer, checks[y], BST_CHECKED);
				else
					CheckDlgButton(hwndMixer, checks[y], BST_UNCHECKED);
				y++;

				if (MixerBuf[x].basstreble) {
					Mixer = x;
					ShowWindow(hBassTreble, SW_SHOW);
					ShowWindow(hBassSlider, SW_SHOW);
					SendMessage(hBassSlider, TBM_SETPOS, 1, 100-MixerBuf[x].bass);
					ShowWindow(hTrebleSlider, SW_SHOW);
					SendMessage(hTrebleSlider, TBM_SETPOS, 1, 100-MixerBuf[x].treble);
				}
			}
			if ((MixerBuf[x].RecordPlay == 'R') && (0 == strcmp(MixerBuf[x].mixercaps, Record[indexRecord]))) {
				MixerBuf[x].checkID = checks[z];
				SendMessage(hSlider[z], TBM_SETPOS, 1, 100-MixerBuf[x].Volume);
				ShowWindow(hSlider[z], SW_SHOW);
				ShowWindow(hCheck[z], SW_SHOW);
				Slider[z].hSlider = hSlider[z];
				Slider[z].checks = checks[z];
				Slider[z].mixer = x;
				SetWindowText(hEdit[z], MixerBuf[x].type);
				SetWindowText(hStatic[z], MixerBuf[x].volume);
				if (0 == strcmp(Selected, MixerBuf[x].type))
					CheckDlgButton(hwndMixer, checks[z], BST_CHECKED);
				else
					CheckDlgButton(hwndMixer, checks[z], BST_UNCHECKED);
				z++;
			}
		}
	}
}

// if (hBassSlider OR if (hTrebleSlider
void SetControl(DWORD ControlType)
{
	MixerError = mixerOpen(&hMixer2, MixerBuf[Mixer].mixerNumber, 0, 0, 0);
	mixerLine2.cbStruct = sizeof(MIXERLINE);
	mixerLine2.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	MixerError = mixerGetLineInfo((HMIXEROBJ)hMixer2, &mixerLine2, MIXER_GETLINEINFOF_COMPONENTTYPE);
	if (!MixerError) {
		mixerLineControls2.cbStruct = sizeof(MIXERLINECONTROLS);
		mixerLineControls2.dwLineID = mixerLine2.dwLineID;
		mixerLineControls2.cControls = mixerLine2.cControls;
		mixerLineControls2.cbmxctrl = sizeof(mixerControl2[0]);
		mixerLineControls2.pamxctrl = mixerControl2;
		MixerError = mixerGetLineControls((HMIXEROBJ)hMixer2, &mixerLineControls2, MIXER_GETLINECONTROLSF_ALL);
		if (!MixerError) {
			for (control = 0; control < mixerLineControls2.cControls; control++) {
				if (mixerControl2[control].dwControlType == ControlType) {
					mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
					mixerControlDetails.cMultipleItems = mixerControl2[control].cMultipleItems;
					mixerControlDetails.dwControlID = (DWORD)mixerControl2[control].dwControlID;
					mixerControlDetails.cChannels = 1;// mixerLine2.cChannels;
					d = (double)SliderPos * 655.35;//SliderPos is a percentage of 0xFFFF
					d2 = modf(d, &d3);
					mixerControlDetailsOther.dwValue = (int)d3;
					if (d2 > 0.50)
						mixerControlDetailsOther.dwValue++;
					mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
					mixerControlDetails.paDetails = &mixerControlDetailsOther;
					/////
					MixerError = mixerSetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails, MIXER_SETCONTROLDETAILSF_VALUE);
					/////
				}
			}
		}
	}
	mixerClose(hMixer2);
}

int CALLBACK MixerProc(HWND hwndMixer, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y, z, Height;
	static HDC hdc;
	static PAINTSTRUCT ps;
	static RECT thisRect, thatRect;
	static RECT rectText;

	switch (message)
	{
	case WM_INITDIALOG:
		wavemixer = TRUE;
		hwndMeter = GetDlgItem(hwndMixer, IDC_STATIC20);
		for (x = 0; x < 16; x++) {
			hSlider[x] = GetDlgItem(hwndMixer, sliders[x]);
			SendMessage(hSlider[x], TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));
			SendMessage(hSlider[x], TBM_SETTICFREQ, (WPARAM)10, (LPARAM)0);
			SendMessage(hSlider[x], TBM_SETLINESIZE, (WPARAM)0, (LPARAM)1);
			SendMessage(hSlider[x], TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)5);
			hEdit[x] = GetDlgItem(hwndMixer, mixers[x]);
			hStatic[x] = GetDlgItem(hwndMixer, volumes[x]);
			hCheck[x] = GetDlgItem(hwndMixer, checks[x]);
		}
		hBassSlider = GetDlgItem(hwndMixer, IDC_SLIDER17);
		SendMessage(hBassSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));
		SendMessage(hBassSlider, TBM_SETTICFREQ, (WPARAM)10, (LPARAM)0);
		SendMessage(hBassSlider, TBM_SETLINESIZE, (WPARAM)0, (LPARAM)1);
		SendMessage(hBassSlider, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)5);
		hTrebleSlider = GetDlgItem(hwndMixer, IDC_SLIDER18);
		SendMessage(hTrebleSlider, TBM_SETRANGE, (WPARAM)TRUE, (LPARAM)MAKELONG(0, 100));
		SendMessage(hTrebleSlider, TBM_SETTICFREQ, (WPARAM)10, (LPARAM)0);
		SendMessage(hTrebleSlider, TBM_SETLINESIZE, (WPARAM)0, (LPARAM)1);
		SendMessage(hTrebleSlider, TBM_SETPAGESIZE, (WPARAM)0, (LPARAM)5);
		hBassTreble = GetDlgItem(hwndMixer, IDC_STATIC17);
		hwndCombo1 = GetDlgItem(hwndMixer, IDC_COMBO1);// IMPORTANT: in the dialog editor, click on the down-pointing thing on the right of the combobox, and then expand the box that appears downward!
		for (x = 0; x < play; x++)
			SendMessage(hwndCombo1, CB_ADDSTRING, 0, (LPARAM)Playback[x]);
		SendMessage(hwndCombo1,	CB_SETCURSEL, (WPARAM)indexPlay, 0);

		hwndCombo2 = GetDlgItem(hwndMixer, IDC_COMBO2); // recording device
		for (x = 0; x < record; x++)
			SendMessage(hwndCombo2, CB_ADDSTRING, 0, (LPARAM)Record[x]);
		SendMessage(hwndCombo2,	CB_SETCURSEL, (WPARAM)indexRecord, 0);

		if (!SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0)) {
			WorkArea.right = GetSystemMetrics(SM_CXFULLSCREEN);// backup
			WorkArea.bottom = GetSystemMetrics(SM_CYFULLSCREEN);// backup
		}
		y = WorkArea.bottom;
		GetWindowRect(hwndMixer, &thisRect);
		Height = thisRect.bottom-thisRect.top;
		thisRect.bottom = y;
		thisRect.top = y - Height;
		MoveWindow(hwndMixer, thisRect.left, thisRect.top, thisRect.right-thisRect.left, thisRect.bottom-thisRect.top, FALSE);

		GetClientRect(hwndMeter, &meterRect);
		meterRect.bottom--;
		vert = meterRect.bottom;
		pMeterProc = (WNDPROC)SetWindowLong(hwndMeter, GWL_WNDPROC, (LONG)MeterProc);

		ShowSliders(hwndMixer);
		ShowQuality();

		SetFocus(hwndMixer);
		break;

	case WM_VSCROLL:
		x = wParam & 0xFFFF;
		if ((x == TB_THUMBPOSITION) || (x == TB_THUMBTRACK))
			SliderPos = 100-(wParam >> 0x10);
		else
			SliderPos = 100-SendMessage((HWND)lParam, TBM_GETPOS, 0, 0);
		for (z = 0; z < 16; z++) {
			if (Slider[z].hSlider == (HWND)lParam) {
				x = Slider[z].mixer;
				MixerError = mixerOpen(&hMixer2, MixerBuf[x].mixerNumber, 0, 0, 0);
				d = (double)SliderPos * 655.35;//SliderPos is a percentage of 0xFFFF
				d2 = modf(d, &d3);
				mixerControlDetailsVolume2.dwValue = (int)d3;
				if (d2 > 0.50)
					mixerControlDetailsVolume2.dwValue++;
				mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
				mixerControlDetails.dwControlID = MixerBuf[x].dwControlID;
				mixerControlDetails.cChannels = 1;
				mixerControlDetails.cMultipleItems = 0;
				mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
				mixerControlDetails.paDetails = &mixerControlDetailsVolume2;
				/////
				MixerError = mixerSetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails, MIXER_SETCONTROLDETAILSF_VALUE);
				/////
				mixerClose(hMixer2);
				break;
			}
		}
		if (hBassSlider == (HWND)lParam) {
			SetControl(MIXERCONTROL_CONTROLTYPE_BASS);
		}
		else if (hTrebleSlider == (HWND)lParam) {
			SetControl(MIXERCONTROL_CONTROLTYPE_TREBLE);
		}
		break;


	case WM_COMMAND:
		if ((LOWORD(wParam) == IDOK) && (!recordingtowave) && (!playingwave) && (!playing2wavefiles))
			SendMessage(hwndMixer, WM_CLOSE, 0, 0); 
		else if (LOWORD(wParam) == IDCANCEL) {
			if ((recordingtowave) || (playingwave) || (playing2wavefiles))
				SendMessage(hwnd, WM_USER3, 0, 0);// stop playing
			else
				SendMessage(hwndMixer, WM_CLOSE, 0, 0); 
		}

		else if ((lParam == (LONG)hwndCombo1) && (HIWORD(wParam) == CBN_SELCHANGE)) {// else
			for (x = 0, y = 0; x < (int)Entries; x++) {
				if ((MixerBuf[x].mixer[0]) && (MixerBuf[x].type[0])) {
					if ((MixerBuf[x].RecordPlay == 'P') && (0 == strcmp(MixerBuf[x].mixercaps, Playback[indexPlay]))) {
						ShowWindow(hSlider[y], SW_HIDE);
						ShowWindow(hCheck[y], SW_HIDE);
						SetWindowText(hEdit[y], "                                             ");
						SetWindowText(hStatic[y], "     ");
						y++;
					}
					ShowWindow(hBassSlider, SW_HIDE);
					ShowWindow(hTrebleSlider, SW_HIDE);
					ShowWindow(hBassTreble, SW_HIDE);
				}
			}
			indexPlay = SendMessage(hwndCombo1, CB_GETCURSEL, 0, 0);

			ShowSliders(hwndMixer);
		}

		else if ((lParam == (LONG)hwndCombo2) && (HIWORD(wParam) == CBN_SELCHANGE)) {
			for (x = 0, z = 8; x < (int)Entries; x++) {
				if ((MixerBuf[x].mixer[0]) && (MixerBuf[x].type[0])) {
					if ((MixerBuf[x].RecordPlay == 'R') && (0 == strcmp(MixerBuf[x].mixercaps, Record[indexRecord]))) {
						ShowWindow(hSlider[z], SW_HIDE);
						ShowWindow(hCheck[z], SW_HIDE);
						SetWindowText(hEdit[z], "                                             ");
						SetWindowText(hStatic[z], "     ");
						z++;
					}
				}
			}
			indexRecord = SendMessage(hwndCombo2, CB_GETCURSEL, 0, 0);

			ShowSliders(hwndMixer);
		}

		else if (HIWORD(wParam) == BN_CLICKED) {
			if (LOWORD(wParam) == IDC_BUTTON1) {// Record to Wave
				if ((!recordingtowave) && (!playingwave) && (!playing2wavefiles) && (recordedtowave))
					RecordWave();
			}
			else if (LOWORD(wParam) == IDC_BUTTON2) {
				if ((!recordingtowave) && (!playingwave) && (!playing2wavefiles) && (!button2pressed)) {// Play Wave
					button2pressed = TRUE;
					if (playing2wavefiles)
						ofn2.lpstrTitle = "Second WAVE file";
					if (GetOpenFileName(&ofn2)) {
						button2pressed = FALSE;
						///////////
						PlayWave();
						///////////
					}
					else
						playing2wavefiles = FALSE;
					ofn2.lpstrTitle = NULL;
				}
			}
////////////////
			else if (LOWORD(wParam) == IDC_BUTTON3) { // Merge 2 Wave files
				if ((!recordingtowave) && (!playingwave) && (!playing2wavefiles) && (!button3pressed)) {
					button3pressed = TRUE;
					ofn2.lpstrTitle = "First WAVE file";
					if (GetOpenFileName(&ofn2)) {
						button3pressed = FALSE;
						hFile2 = CreateFile(FullFilename2, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
						if (hFile2 != INVALID_HANDLE_VALUE) {
							if (fileSize2 = GetFileSize(hFile2, NULL)) {
								if (WaveBuf2)
									VirtualFree(WaveBuf2, 0, MEM_RELEASE);
								WaveBuf2 = VirtualAlloc(NULL, fileSize2, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
								ReadFile(hFile2, WaveBuf2, fileSize2, &dwBytesRead, NULL);
								CloseHandle(hFile2);
								if (*(DWORD*)&WaveBuf2[8] == 0x45564157) {// "WAVE"
									if ((*(WORD*)&WaveBuf2[20] == 1) || (*(WORD*)&WaveBuf2[20] == 0xFFFE)) {// PCM or WAVEFORMATEXTENSIBLE
										subchunksize = *(DWORD*)&WaveBuf2[16];
										nextchunk = subchunksize + 20;
										if (*(DWORD*)&WaveBuf2[nextchunk] == 0x74636166)// "fact"
											nextchunk += 12;
										if (*(DWORD*)&WaveBuf2[nextchunk] == 0x61746164) {// "data"
											Data2 = nextchunk+8;
											playing2wavefiles = TRUE;
										}
										else {
											ofn2.lpstrTitle = NULL;
											MessageBox(hwndMixer, "Can't play Wave file\n", "", MB_OK);
										}
									}
									else {
										ofn2.lpstrTitle = NULL;
										MessageBox(hwndMixer, "This program can't play\nthat kind of Wave file.", "", MB_OK);
									}
								}
								else {
									ofn2.lpstrTitle = NULL;
									MessageBox(hwndMixer, "That's not a Wave file.", "Oops", MB_OK);
								}
							}
							else {
								ofn2.lpstrTitle = NULL;
								CloseHandle(hFile2);
								MessageBox(hwndMixer, "That file is empty", ERROR, MB_OK);
							}
						}
						else {
							ofn2.lpstrTitle = NULL;
							playing2wavefiles = FALSE;
							MessageBox(hwndMixer, FullFilename2, "Unable to open", MB_OK);
						}
					}
					else {
						button3pressed = FALSE;
						ofn2.lpstrTitle = NULL;
						SetFocus(hwndMixer);
						break;
					}
					if (playing2wavefiles)
						ofn2.lpstrTitle = "Second WAVE file";
					if (GetOpenFileName(&ofn2)) {
						///////////
//						PlayWave();
						///////////
						button3pressed = FALSE;
						playing2wavefiles = FALSE;
						hFile = CreateFile(FullFilename2, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
						if (hFile != INVALID_HANDLE_VALUE) {
							if (fileSize = GetFileSize(hFile, NULL)) {
								BufferSize = fileSize;
								if (WaveBuf)
									VirtualFree(WaveBuf, 0, MEM_RELEASE);
								WaveBuf = VirtualAlloc(NULL, BufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
								ReadFile(hFile, WaveBuf, fileSize, &dwBytesRead, NULL);
								CloseHandle(hFile);
								if (*(DWORD*)&WaveBuf[8] == 0x45564157) { // "WAVE"
									if ((*(WORD*)&WaveBuf[20] == 1) || (*(WORD*)&WaveBuf[20] == 0xFFFE)) {// PCM or WAVEFORMATEXTENSIBLE
										subchunksize = *(DWORD*)&WaveBuf[16];
										nextchunk = subchunksize + 20;
										if (*(DWORD*)&WaveBuf[nextchunk] == 0x74636166)// "fact"
											nextchunk += 12; // ignore fact chunk
										if (*(DWORD*)&WaveBuf[nextchunk] == 0x61746164) {// "data"
// MERGE WAVE FILES
											Data1 = nextchunk+8;
											if ((*(WORD*)&WaveBuf[20] == *(WORD*)&WaveBuf2[20]) // wave type
											 && (*(WORD*)&WaveBuf[22] == *(WORD*)&WaveBuf2[22]) // nChannels
											 && (*(DWORD*)&WaveBuf[24] == *(DWORD*)&WaveBuf2[24]) // nSamplesPerSec
											 && (*(WORD*)&WaveBuf[34] == *(WORD*)&WaveBuf2[34])) // wBitsPerSample
											{
												if (*(WORD*)&WaveBuf[34] == 16) { // wBitsPerSample
													for ( ;(Data1 < fileSize) && (Data2 < fileSize2); Data1 += 2, Data2 += 2) {
														if ((*(SHORT*)&WaveBuf[Data1] & 1) && (*(SHORT*)&WaveBuf2[Data2] & 1))
															*(SHORT*)&WaveBuf[Data1] += 1; // because of dividing both numbers by 2
														s = (*(SHORT*)&WaveBuf[Data1] / 2) + (*(SHORT*)&WaveBuf2[Data2] / 2);
														*(SHORT*)&WaveBuf[Data1] = s;
													}
												}
												else if (*(WORD*)&WaveBuf[34] == 24) { // wBitsPerSample
													if (fileSize <= fileSize2) {
														for ( ;Data1 < fileSize; Data1 += 3, Data2 += 3) {
															x = (*(DWORD*)&WaveBuf[Data1] & 0x00FFFFFF) + (*(DWORD*)&WaveBuf2[Data2] & 0x00FFFFFF);
															*(int*)&WaveBuf[Data1] = x | (*(DWORD*)&WaveBuf[Data1] & 0xFF000000);
														}
													}
													else {
														for ( ;Data2 < fileSize2; Data1 += 3, Data2 += 3) {
															x = (*(DWORD*)&WaveBuf[Data1] & 0x00FFFFFF) + (*(DWORD*)&WaveBuf2[Data2] & 0x00FFFFFF);
															*(int*)&WaveBuf2[Data2] = x | (*(DWORD*)&WaveBuf2[Data2] & 0xFF000000);
														}
													}
												}
												FullFilename2[0] = 0;
												Filename[0] = 0;
												ofn2.lpstrTitle = NULL;
												if (GetSaveFileName(&ofn2)) {
													int x = ofn2.nFileExtension;
													ofn2.lpstrFile[x] = 'w';
													ofn2.lpstrFile[x+1] = 'a';
													ofn2.lpstrFile[x+2] = 'v';
													ofn2.lpstrFile[x+3] = 0;
													hFile = CreateFile(FullFilename2, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
													if (hFile != INVALID_HANDLE_VALUE) {
														if (fileSize <= fileSize2)
															WriteFile(hFile, WaveBuf, fileSize, &dwBytesWritten, NULL);
														else
															WriteFile(hFile, WaveBuf2, fileSize2, &dwBytesWritten, NULL);
														CloseHandle(hFile);
														MessageBox(hwndMixer, "", "Done", MB_OK);
													}
												}
											}
											else {
												MessageBox(hwndMixer, "Either the WAVE files don't have\nthe same wave type\nor the same number of channels\nor the same kHz\nor the same bits/sample.", ERROR, MB_OK);
											}
										}
									}
								}
							}
							else
								CloseHandle(hFile);
						}
					}
					else {
						button3pressed = FALSE;
						playing2wavefiles = FALSE;
					}
					if (WaveBuf)
						VirtualFree(WaveBuf, 0, MEM_RELEASE);
					if (WaveBuf2)
						VirtualFree(WaveBuf2, 0, MEM_RELEASE);
					WaveBuf = WaveBuf2 = NULL;
					ofn2.lpstrTitle = NULL;
					SetFocus(hwndMixer);
				}
			}
////////////////
			else if (LOWORD(wParam) == IDC_BUTTON4) {
				if ((!recordingtowave) && (!playingwave) && (!playing2wavefiles) && (!button4pressed)) {// Recording Quality
					button4pressed = TRUE;
					DialogBox(hInst, "WAVEOPTIONS", hwnd, WaveOptionsProc);
					ShowQuality();
					button4pressed = FALSE;
					SetFocus(hwndMixer);
				}
			}
			else if (LOWORD(wParam) == IDC_BUTTON5) {// Help
				Help = WaveHelp;
				DialogBox(hInst, "WAVEHELP", hwnd, Help2Proc);
				SetFocus(hwndMixer);
			}
			else if (LOWORD(wParam) == IDC_BUTTON6) {// Edit Wave
				SendMessage(hwnd, WM_USER13, 0, 0);
			}


			else {// mute/select buttons
				for (y = 0; y < 16; y++)
					if (LOWORD(wParam) == Slider[y].checks)
						break;
				if (y == 16) {
					MessageBox(hwndMixer,"", "!", MB_OK);
					break;
				}
				mixerControlDetailsMute.fValue = IsDlgButtonChecked(hwndMixer, checks[y]);
				if (y == 0)
					mixerControlDetailsMasterMute.fValue = mixerControlDetailsMute.fValue;
				x = Slider[y].mixer;
				if (x == 0) {
					mute = TRUE;
					SetMasterVolume();
				}
				else if (y < 8) {// mute
					mixerOpen(&hMixer2, MixerBuf[x].mixerNumber, 0, 0, 0);
					mixerLine2.cbStruct = sizeof(MIXERLINE);
					mixerLine2.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
					MixerError = mixerGetLineInfo((HMIXEROBJ)hMixer2, &mixerLine2, MIXER_GETLINEINFOF_COMPONENTTYPE);
					if (!MixerError) {
						for (i = 0; i < mixerLine2.cConnections; i++) {
							mixerLine3.cbStruct = sizeof(MIXERLINE);
							mixerLine3.dwDestination = MixerBuf[x].muteDestination;
							mixerLine3.dwSource = i;
							MixerError = mixerGetLineInfo((HMIXEROBJ)hMixer2, &mixerLine3, MIXER_GETLINEINFOF_SOURCE);
							if (!MixerError) {
								if ((0 == strcmp(mixerLine3.szName, MixerBuf[x].type)) && (0 == strcmp(mixerLine3.Target.szPname, MixerBuf[x].mixer))) {
									mixerLineControls2.cbStruct = sizeof(MIXERLINECONTROLS);
									mixerLineControls2.dwLineID = mixerLine3.dwLineID;
									mixerLineControls2.cControls = mixerLine3.cControls;
									mixerLineControls2.cbmxctrl = sizeof(mixerControl2[0]);
									mixerLineControls2.pamxctrl = mixerControl2;
									MixerError = mixerGetLineControls((HMIXEROBJ)hMixer2, &mixerLineControls2, MIXER_GETLINECONTROLSF_ALL);
									if (!MixerError) {
										for (control = 0; control < mixerLineControls2.cControls; control++) {
											mixerControlDetails2.cbStruct = sizeof(MIXERCONTROLDETAILS);
											mixerControlDetails2.cMultipleItems = mixerControl2[control].cMultipleItems;
											mixerControlDetails2.dwControlID = (DWORD)mixerControl2[control].dwControlID;
											mixerControlDetails2.cChannels = 1;// mixerLine3.cChannels;
											if (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE) {
												mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
												mixerControlDetails2.paDetails = &mixerControlDetailsMute;
												MixerError = mixerSetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_VALUE);
											}
										}
									}
								}
							}
						}
					}
				}
				else {// select
					mixerOpen(&hMixer2, MixerBuf[x].mixerNumber, 0, 0, 0);
					mixerLine2.cbStruct = sizeof(MIXERLINE);
					mixerLine2.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_WAVEIN;
					MixerError = mixerGetLineInfo((HMIXEROBJ)hMixer2, &mixerLine2, MIXER_GETLINEINFOF_COMPONENTTYPE);
					if (!MixerError) {
						mixerLineControls2.cbStruct = sizeof(MIXERLINECONTROLS);
						mixerLineControls2.dwLineID = mixerLine2.dwLineID;
						mixerLineControls2.cControls = mixerLine2.cControls;
						mixerLineControls2.cbmxctrl = sizeof(mixerControl2[0]);
						mixerLineControls2.pamxctrl = mixerControl2;
						MixerError = mixerGetLineControls((HMIXEROBJ)hMixer2, &mixerLineControls2, MIXER_GETLINECONTROLSF_ALL);
						if (!MixerError) {
							for (control = 0; control < mixerLineControls2.cControls; control++) {
								mixerControlDetails2.cbStruct = sizeof(MIXERCONTROLDETAILS);
								mixerControlDetails2.cMultipleItems = mixerControl2[control].cMultipleItems;
								mixerControlDetails2.dwControlID = (DWORD)mixerControl2[control].dwControlID;
								mixerControlDetails2.cChannels = 1;// mixerLine2.cChannels;
								if ((mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MUX) || (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MIXER)) {
									pList = (MIXERCONTROLDETAILS_LISTTEXT*)malloc(mixerControlDetails2.cMultipleItems * sizeof(MIXERCONTROLDETAILS_LISTTEXT));
									mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
									mixerControlDetails2.paDetails = pList;
									MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_LISTTEXT);
									if (!MixerError) {
										pSelected = (MIXERCONTROLDETAILS_UNSIGNED*)malloc(mixerControlDetails2.cMultipleItems * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
										for (i = 0; i < mixerControlDetails2.cMultipleItems; i++) {
											if (0 == strcmp(pList[i].szName, MixerBuf[x].type))
												pSelected[i].dwValue = 1;
											else
												pSelected[i].dwValue = 0;
										}
										mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
										mixerControlDetails2.paDetails = pSelected;// look at pSelected[0], pSelected[1], etc
										MixerError = mixerSetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_SETCONTROLDETAILSF_VALUE);
										free(pSelected);
									}
									free(pList);
								}
							}
						}
					}
				}
			}
		}
		break;
					
	case WM_CLOSE:
		DestroyWindow(hwndMixer);
		hwndMixer = NULL;
		wavemixer = FALSE;
	}
	return 0;
}

int CALLBACK SoundsProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		if (continuousustain[ActiveChannel])
			CheckDlgButton(hwndDlg, IDC_CHECK1, BST_CHECKED);
		if (reverb[ActiveChannel])
			CheckDlgButton(hwndDlg, IDC_CHECK2, BST_CHECKED);
		if (chorus[ActiveChannel])
			CheckDlgButton(hwndDlg, IDC_CHECK3, BST_CHECKED);
		if (ModWheel[ActiveChannel] == 0)
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO5, IDC_RADIO5);
		else if (ModWheel[ActiveChannel] == 32)
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO5, IDC_RADIO1);
		else if (ModWheel[ActiveChannel] == 64)
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO5, IDC_RADIO2);
		else if (ModWheel[ActiveChannel] == 96)
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO5, IDC_RADIO3);
		else if (ModWheel[ActiveChannel] == 127)
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO5, IDC_RADIO4);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK1))
				continuousustain[ActiveChannel] = TRUE;
			else
				continuousustain[ActiveChannel] = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK2))
				reverb[ActiveChannel] = TRUE;
			else
				reverb[ActiveChannel] = FALSE;
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK3))
				chorus[ActiveChannel] = TRUE;
			else
				chorus[ActiveChannel] = FALSE;

			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO5))
				ModWheel[ActiveChannel] = 0;
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO1))
				ModWheel[ActiveChannel] = 32;
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO2))
				ModWheel[ActiveChannel] = 64;
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO3))
				ModWheel[ActiveChannel] = 96;
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO4))
				ModWheel[ActiveChannel] = 127;
			EndDialog (hwndDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			break;
		}
	}
	return 0;
}

int CALLBACK AllBeatsPerMinuteProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char textIncrease[10], textDecrease[10];
	static HWND hwndEdit1, hwndEdit2;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit1 = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndEdit2 = GetDlgItem(hwndDlg, IDC_EDIT2);
		SetFocus(hwndEdit1);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (GetWindowText(hwndEdit1, textIncrease, 10))
				BPMchange = atoi(textIncrease);
			else if (GetWindowText(hwndEdit2, textDecrease, 10))
				BPMchange = -(atoi(textDecrease));
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK MasterVolumeProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char Channel[] = "Channel    Instrument";
	static char textIncrease[10], textDecrease[10];
	static HWND hwndEdit1, hwndEdit2;

	switch (message)
	{
	case WM_INITDIALOG:
		Channel[8] = ((Event[thisX].channel+1) / 10) + '0';
		if (Channel[8] == '0')
			Channel[8] = ' ';
		Channel[9] = ((Event[thisX].channel+1) % 10) + '0';
		SetWindowText(hwndDlg, &myInstruments[ChannelInstruments[Event[thisX].channel][0]][5]);
		hwndEdit1 = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndEdit2 = GetDlgItem(hwndDlg, IDC_EDIT2);
		SetFocus(hwndEdit1);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			GetWindowText(hwndEdit1, textIncrease, 10);
			Increase = atoi(textIncrease);
			GetWindowText(hwndEdit2, textDecrease, 10);
			Decrease = atoi(textDecrease);
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK WaveOptionsProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	BOOL gotit;
	static HWND hwndCombo;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndCombo = GetDlgItem(hwndDlg, IDC_COMBO1);
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)"  8.000 kHz, 16 Bit, Mono ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 11.025 kHz, 16 Bit, Mono ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 11.025 kHz, 16 Bit, Stereo ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 22.050 kHz, 16 Bit, Mono ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 22.050 kHz, 16 Bit, Stereo ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 44.100 kHz, 16 Bit, Mono ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 44.100 kHz, 16 Bit, Stereo ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 44.100 kHz, 24 Bit, Mono ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 44.100 kHz, 24 Bit, Stereo ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 48.000 kHz, 24 Bit, Mono ");
		SendMessage(hwndCombo, CB_ADDSTRING, 0, (LPARAM)" 48.000 kHz, 24 Bit, Stereo ");
		SendMessage(hwndCombo,	CB_SETCURSEL, (WPARAM)WaveOptionsIndex, 0);
		SetFocus(hwndDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:
			hFile = CreateFile(IniFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				if (IniBufSize = GetFileSize(hFile, NULL)) {
					gotit = FALSE;
					ReadFile(hFile, IniBuf, IniBufSize, &dwBytesRead, NULL);
					SetFilePointer(hFile, 0, 0, FILE_BEGIN);
					for (x = 0; x < IniBufSize; x++) {
						if ((IniBuf[x] == '=') && (IniBuf[x-7] == 'Q') && (IniBuf[x-1] == 'y')) {// RecordQuality=
							IniBuf[x+1] = WaveOptionsIndex + '1';
							gotit = TRUE;
						}
					}
					if (gotit == FALSE) {// "RecordQuality=" not found
						for (y = 0; IniRecordQuality[y] != 0; x++, y++)
							IniBuf[x] = IniRecordQuality[y];
						IniBuf[x++] = WaveOptionsIndex + '1';
						IniBuf[x++] = '\r';
						IniBuf[x] = '\n';
					}
				}
			}
			else {
				hFile = CreateFile(IniFile, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				for (x = 0, y = 0; IniRecordQuality[y] != 0; x++, y++)
					IniBuf[x] = IniRecordQuality[y];
				IniBuf[x++] = WaveOptionsIndex + '1';
				IniBuf[x++] = '\r';
				IniBuf[x] = '\n';
			}
			WriteFile(hFile, IniBuf, x, &dwBytesWritten, NULL);
			CloseHandle(hFile);
			MessageBox(hwndMixer, IniBuf, "PianoRollComposer.ini", MB_OK);
			SetFocus(hwndDlg);
			break;

		case IDC_COMBO1:
			if (HIWORD(wParam) == CBN_SELCHANGE)
				WaveOptionsIndex = SendMessage(hwndCombo, CB_GETCURSEL, 0, 0);
			break;

		case IDC_BUTTON2:
		case IDOK:
			GetWaveQuality();
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK LyricProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			LyricLen = GetWindowText(hwndEdit, Lyric, 64);
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK ChangeInstrumentProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char fromChannel[2], toChannel[2], fromPort[2], toPort[2];
	static HWND hwndEdit1, hwndEdit2, hwndEdit3, hwndEdit4, hwndStatic1, hwndStatic2;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit1 = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndEdit2 = GetDlgItem(hwndDlg, IDC_EDIT2);
		if (EWQL) {
			hwndStatic1 = GetDlgItem(hwndDlg, IDC_STATIC1);
			ShowWindow(hwndStatic1, SW_SHOWNORMAL);
			hwndStatic2 = GetDlgItem(hwndDlg, IDC_STATIC2);
			ShowWindow(hwndStatic2, SW_SHOWNORMAL);
			hwndEdit3 = GetDlgItem(hwndDlg, IDC_EDIT3);
			ShowWindow(hwndEdit3, SW_SHOWNORMAL);
			hwndEdit4 = GetDlgItem(hwndDlg, IDC_EDIT4);
			ShowWindow(hwndEdit4, SW_SHOWNORMAL);
			SetFocus(hwndEdit3);
		}
		else
			SetFocus(hwndEdit1);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (GetWindowText(hwndEdit1, fromChannel, 3))
				FromChannel = Atoi(fromChannel);
			else
				break;
			if (GetWindowText(hwndEdit2, toChannel, 3))
				ToChannel = Atoi(toChannel);
			else
				break;
			if (EWQL) {
				if (GetWindowText(hwndEdit3, fromPort, 3))
					FromPort = Atoi(fromPort);
				else
					break;
				if (GetWindowText(hwndEdit4, toPort, 3))
					ToPort = Atoi(toPort);
				else
					break;
			}
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK OpeningTextProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		SendMessage(hwndEdit, WM_SETFONT, (UINT)hFont, TRUE);
		SetWindowText(hwndEdit, BigText);
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (fromoptions) {
				fromoptions = FALSE;
				SetFocus(hwndOptionsDlg);
			}
			EndDialog (hwndDlg, TRUE);
			return TRUE;
		case IDCANCEL:
			if (fromoptions) {
				fromoptions = FALSE;
				SetFocus(hwndOptionsDlg);
			}
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}

int CALLBACK RecordInstructions1(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog (hwndDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			break;
		}
	}
	return 0;
}

int CALLBACK RecordInstructions2(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog (hwndDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			break;
		}
	}
	return 0;
}

int CALLBACK PortamentoProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char cPortamentoRate[4];
	static HWND hwndEdit;

	if (message == WM_INITDIALOG) {
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);		
		SetFocus(hwndEdit);
	}

	else if (message == WM_COMMAND) {
		if (LOWORD(wParam) == IDOK) {
			if (GetWindowText(hwndEdit, cPortamentoRate, 4)) {
				PortamentoRate = (BYTE)Atoi(cPortamentoRate);
				EndDialog(hwndDlg, TRUE);
			}
		}
		else if (wParam == IDCANCEL) {
			EndDialog(hwndDlg, FALSE);
		}
	}
	return 0;
}

int CALLBACK ChangeVelocitiesProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int dlgWidth;
	RECT dlgRect;
	static char cNewVelocity[4];
	static HWND hwndEdit;

	if (message == WM_INITDIALOG) {
		GetWindowRect(hwndDlg, &dlgRect);
		dlgWidth = dlgRect.right-dlgRect.left;
		MoveWindow(hwndDlg, (rect.right/2)-(dlgWidth/2), TitleAndMenu, dlgWidth, dlgRect.bottom-dlgRect.top, TRUE);
		changingvelocity = TRUE;
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);		
		SetFocus(hwndEdit);
	}
	else if (message == WM_COMMAND) {
		if ((LOWORD(wParam) == IDC_EDIT1) && (HIWORD(wParam) == EN_CHANGE)) {
			if (GetWindowText(hwndEdit, cNewVelocity, 4)) {
				NewVelocity = (BYTE)Atoi(cNewVelocity);
			}
		}
		else if (wParam == IDCANCEL) {
			SendMessage(hwndVelocities, WM_CLOSE, 0, 0);
		}
	}
	else if (message == WM_CLOSE) {
		changingvelocity = FALSE;
		DestroyWindow(hwndVelocities);
		hwndVelocities = NULL;
	}
	return 0;
}

int CALLBACK RecordProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int PossibleNote;
	static int NoteLengths[] = {0, 60, 120, 240, 480};
	static int radio[] = {IDC_RADIO1, IDC_RADIO2, IDC_RADIO3, IDC_RADIO4, IDC_RADIO5};
	static char metronomeChannel[4], metronomeNote[4], metronomePort[4];
	static HWND hwnd1, hwnd2, hwnd3, hwndButton, hwndVolume, hwndChannel, hwndNote, hwndStaticPort, hwndMyPort;

	switch (message)
	{
	case WM_INITDIALOG:
		NoteLengths[3] = TicksPerQuarterNote >> 1;
		NoteLengths[2] = TicksPerQuarterNote >> 2;
		NoteLengths[1] = TicksPerQuarterNote >> 3;
		NoteLengths[0] = 0;
		hwnd1 = GetDlgItem(hwndDlg, IDC_STATIC1); // Wait for a full measure of beats...
		hwnd2 = GetDlgItem(hwndDlg, IDC_STATIC2); // help
		hwnd3 = GetDlgItem(hwndDlg, IDC_STATIC3); // help
		hwndButton = GetDlgItem(hwndDlg, IDC_BUTTON1);
		hwndVolume = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndChannel = GetDlgItem(hwndDlg, IDC_EDIT2);
		hwndNote = GetDlgItem(hwndDlg, IDC_EDIT3);
		_itoa(MetronomeVolume, metronomeVolume, 10);
		SetWindowText(hwndVolume, metronomeVolume);
		_itoa(MetronomeChannel, metronomeChannel, 10);
		SetWindowText(hwndChannel, metronomeChannel);
		_itoa(MetronomeNote, metronomeNote, 10);
		SetWindowText(hwndNote, metronomeNote);
		if (EWQL) {
			hwndStaticPort = GetDlgItem(hwndDlg, IDC_STATIC_PORT);
			ShowWindow(hwndStaticPort, SW_SHOWNORMAL);
			hwndMyPort = GetDlgItem(hwndDlg, IDC_EDIT4);
			ShowWindow(hwndMyPort, SW_SHOWNORMAL);
			_itoa(MetronomePort+1, metronomePort, 10);
			SetWindowText(hwndMyPort, metronomePort);
		}
		for (PossibleNote = 0; PossibleNote < (int)e; PossibleNote++) {
			if (Event[PossibleNote].velocity)
				break;
		}
		if (PossibleNote != (int)e) {
			startatp = TRUE;
			SetWindowText(hwnd1, "Start at the cursor when you press \"P\"");
		}
		else
			startatp = FALSE;
		NoteLen = TicksPerQuarterNote >> 3;// 60;
		CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO5, IDC_RADIO2);
		if (alterbeatperminute)
			CheckDlgButton (hwndDlg, IDC_CHECK1, BST_CHECKED);
		SetFocus(hwndDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_CHECK1:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK1))
				alterbeatperminute = TRUE;
			else
				alterbeatperminute = FALSE;
			break;
//		case IDC_BUTTON2:
//			MessageBox(hwndDlg, "Select Instruments on the Menu\nto see the Instrument Channels.\nSelect the Active Instrument\nto use as a Metronome.\n\nThis includes the Percussion Channel.", "Instrument Channel", MB_OK);
//			break;
//		case IDC_BUTTON3:
//			MessageBox(hwndDlg, "Select Instruments on the Menu\nand select Play Active Instrument\nto hear its different notes.\n\nSelect Options on the Menu\nand select SHOW: Note Numbers.\nThen left-click to play that note\nand see its note number.\n\nThis works for Percussion as well.", "Instrument Note", MB_OK);
//			break;

		case IDOK:
			for (x = 0; x < 5; x++) {
				if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, radio[x])) {
					NoteLen = NoteLengths[x];
					break;
				}
			}
			GetWindowText(hwndVolume, metronomeVolume, 4);
			MetronomeVolume = Atoi(metronomeVolume);
			if (MetronomeVolume > 127)
				MetronomeVolume = 127;
			GetWindowText(hwndChannel, metronomeChannel, 4);
			MetronomeChannel = Atoi(metronomeChannel);
			if ((MetronomeChannel-1) > 15)
				MetronomeChannel = 1;
			GetWindowText(hwndNote, metronomeNote, 4);
			MetronomeNote = Atoi(metronomeNote);
			if ((MetronomeNote > 108) || (MetronomeNote < 21))
				MetronomeNote = 60;
			if (EWQL) {
				GetWindowText(hwndMyPort, metronomePort, 4);
				MetronomePort = Atoi(metronomePort) - 1;
			}
//			if (PossibleNote == (int)e) { // no notes entered yet
			if (startatp == FALSE) { // no notes entered yet
				if (DialogBox(hInst, "WITHNOMUSIC", hwndDlg, RecordInstructions1))
					EndDialog (hwndDlg, TRUE);
				else {
//					startatp = FALSE;
					EndDialog (hwndDlg, FALSE);
				}
			}
			else { // music was entered
				if (DialogBox(hInst, "WITHMUSIC", hwndDlg, RecordInstructions2))
					EndDialog (hwndDlg, TRUE);
				else {
//					startatp = FALSE;
					EndDialog (hwndDlg, FALSE);
				}
			}
			break;

		case IDCANCEL:
			startatp = FALSE;
			EndDialog (hwndDlg, FALSE);
			break;
		}
	}
	return 0;
}

int CALLBACK HelpProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	char *pText;
	int dlgWidth;
	static HWND hwndEdit;
	static RECT dlgRect;
	static HDC hdc;
	static PAINTSTRUCT ps;

	switch (message)
	{
	case WM_INITDIALOG:
		helping = TRUE;
		GetWindowRect(hwndDlg, &dlgRect);
		dlgWidth = dlgRect.right-dlgRect.left;
		MoveWindow(hwndDlg, (rect.right/2)-(dlgWidth/2), TitleAndMenu, dlgWidth, rect.bottom, TRUE);
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		GetClientRect(hwndDlg, &dlgRect);
		MoveWindow(hwndEdit, 0, 0, dlgRect.right, dlgRect.bottom, TRUE);
		SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hHelpFont, (LPARAM)MAKELPARAM(TRUE, 0));
		dlgRect.left += 5;
		dlgRect.right -= 15;
		SendMessage(hwndEdit, EM_SETRECTNP, 0, (LPARAM)(LPRECT)&dlgRect);
		pText = (char*)LockResource(hResource);
		SetWindowText(hwndEdit, pText);
		SetFocus(hwndEdit);
		pHelpProc = (WNDPROC)SetWindowLong(hwndEdit, GWL_WNDPROC, (LONG)HProc);
		break;

	case WM_CLOSE:
		DestroyWindow(hwndHelp);
		hwndHelp = NULL;
		helping = FALSE;
		break;
	}
	return 0;
}

int CALLBACK Help2Proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetWindowText(hwndEdit, Help);
		SetFocus(hwndEdit);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog (hwndDlg, TRUE);
			break;
		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			break;
		}
	}
	return 0;
}

BOOL CheckListBuf(void)
{
	DWORD PreviousPixel, NextPixel;
	DWORD instrument;

	if (ListBuf[0] != ' ')// + or -
		return FALSE;// can't edit notes

	TempEvent.pixel = Atoi(&ListBuf[1]);

	PreviousPixel = 0;
	NextPixel = 0xFFFFFFFF;
	if (index) {
		SendMessage(hwndEditList, LB_GETTEXT, index-1, (LPARAM)PreviousIndex);
		PreviousPixel = Atoi(&PreviousIndex[1]);
	}
//	if (index < (int)e) {
//		SendMessage(hwndEditList, LB_GETTEXT, index+1, (LPARAM)NextIndex);
//		NextPixel = Atoi(&NextIndex[1]);
//	}
	if ((TempEvent.pixel < PreviousPixel) || (TempEvent.pixel > NextPixel))
		return FALSE;

	if ((ListBuf[7] == 'C') && (ListBuf[8] == 'h') && (ListBuf[9] == 'a') && (ListBuf[14] == ' ')) {// Channel
		if (ListBuf[16] == ' ') { // Channel 1 thru 9
			channel = (ListBuf[15] - '0') - 1;
			instrument = 17; // e.g. ListBuf[instrument]
		}
		else if ((ListBuf[16] >= '0') && (ListBuf[16] <= '9')) { // Channel 10 thru 16
			channel = (((ListBuf[15] - '0') * 10) + (ListBuf[16] - '0')) - 1;
			instrument = 18;
		}
		else
			return FALSE;
		if ((channel != 9) || (EWQL)) {// not percussion
			for (x = 0; x < 128; x++) {
				if ((ListBuf[instrument] >= '0') && (ListBuf[instrument] <= '9')) { // instrument number
					InstrumentNum = ListBuf[instrument] - '0';
					if ((ListBuf[instrument+1] >= '0') && (ListBuf[instrument+1] <= '9')) {
						InstrumentNum *= 10;
						InstrumentNum += ListBuf[instrument+1] - '0';
					}
					if ((ListBuf[instrument+2] >= '0') && (ListBuf[instrument+2] <= '9')) {
						InstrumentNum *= 10;
						InstrumentNum += ListBuf[instrument+2] - '0';
					}
					if (InstrumentNum)
						InstrumentNum--;
					TempEvent.message = (0xC0|channel) | (InstrumentNum << 8);
					for (y = 0; ChannelInstruments[channel][y] != 0xFF; y++)
						;
					if (y == 0) {
						ChannelInstruments[channel][0] = InstrumentNum;
						InstrumentOffset[channel][0] = TempEvent.pixel;
					}
					else if ((y) && (InstrumentOffset[channel][y-1] == TempEvent.pixel))
						ChannelInstruments[channel][y-1] = InstrumentNum;// change previous location instrument
					else if ((y) && (ChannelInstruments[Channel][y-1] != InstrumentNum)) {
						ChannelInstruments[channel][y] = InstrumentNum;
						InstrumentOffset[channel][y] = TempEvent.pixel;
					}
					ActiveChannels[channel] = TRUE;
					break;
				}
				else { // it's the name of the instrument
					for (y = 0; ListBuf[instrument+y] != 0; y++) {
						if (myInstruments[x][y+5] != ListBuf[instrument+y])
							break;
					}
					if (ListBuf[instrument+y] == 0) {// found it
						TempEvent.message = (0xC0|channel) | (x << 8);
						for (y = 0; ChannelInstruments[channel][y] != 0xFF; y++)
							;
						if (y == 0) {
							ChannelInstruments[channel][0] = x;
							InstrumentOffset[channel][0] = TempEvent.pixel;
						}
						else if ((y) && (InstrumentOffset[channel][y-1] == TempEvent.pixel))
							ChannelInstruments[channel][y-1] = x;// change previous location instrument
						else if ((y) && (ChannelInstruments[Channel][y-1] != x)) {
							ChannelInstruments[channel][y] = x;
							InstrumentOffset[channel][y] = TempEvent.pixel;
						}
						ActiveChannels[channel] = TRUE;
						break;
					}
				}
			}
			if (x == 128) {
				MessageBox(hwndList, "isn't a listed instrument.\nCheck the spelling.", &ListBuf[instrument], MB_OK);
				return FALSE;
			}
		}
		else {// if (channel == 9)
			TempEvent.message = (0xC9) | ((30 + (DrumSet*10)) << 8);
			ChannelInstruments[9][0] = 30 + (DrumSet*10);
			ActiveChannels[9] = TRUE;
		}
	} // endof if (ListBuf == "Channel = ")

	else {
		x = 7;
		if ((ListBuf[x] == 'B') && (ListBuf[x+1] == 'e') && (ListBuf[x+2] == 'a')) {// "Beats/Minute = "
			y = Atoi(&ListBuf[22]);
			if ((y >= 24) && (y <= 500)) {
				TempEvent.dMilliSecondsPerTick = 60000.0 / (double)(y * TicksPerQuarterNote);
				TempEvent.message = 0;// flag
				return TRUE;
			}
		}
		for ( ; ListBuf[x] != 0; x++) {
			if (ListBuf[x] == '[')
				break;
		}
		if (ListBuf[x] == 0)// [ not found
			return FALSE;// all other entries have to be Volume, Expression, Controllers Off, Notes Off, Pan, Reverb, Chorus, Modulation, or Pitch Bend

		x++;// to channel
		if (ListBuf[x+1] == ']') {
			channel = (ListBuf[x] - '0') - 1;
			x += 3;// to type of change
		}
		else {
			channel = (((ListBuf[x] - '0') * 10) + (ListBuf[x+1] - '0')) - 1;
			x += 4;// to type of change
		}
		
		if ((ListBuf[x] == 'V') && (ListBuf[x+1] == 'o') && (ListBuf[x+2] == 'l')) {// "Volume = "
			x += 9;// past ' = '
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (7 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'E') && (ListBuf[x+1] == 'x') && (ListBuf[x+2] == 'p')) {// "Expression = "
			x += 13;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (11 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'A') && (ListBuf[x+4] == 'C') && (ListBuf[x+5] == 'o')) {// "All Controllers Off = "
			x += 18;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (121 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'A') && (ListBuf[x+4] == 'N') && (ListBuf[x+5] == 'o')) {// "All Notes Off = "
			x += 12;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (123 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'P') && (ListBuf[x+1] == 'a') && (ListBuf[x+2] == 'n')) {// "Pan = "
			x += 6;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (10 << 8) | (y << 16);
			StereoLocations[channel] = y;
		}
		else if ((ListBuf[x] == 'R') && (ListBuf[x+1] == 'e') && (ListBuf[x+2] == 'v')) {// "Reverb = "
			x += 9;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (91 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'C') && (ListBuf[x+1] == 'h') && (ListBuf[x+2] == 'o')) {// "Chorus = "
			x += 9;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (93 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'S') && (ListBuf[x+1] == 'u') && (ListBuf[x+2] == 's')) {// "Sustain = "
			x += 10;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (64 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'M') && (ListBuf[x+1] == 'o') && (ListBuf[x+2] == 'd')) {// "Modulation = "
			x += 13;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (1 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'P') && (ListBuf[x+1] == 'i') && (ListBuf[x+2] == 't')) {// "Pitch Bend = "
			x += 13;
			y = Atoi(&ListBuf[x]);
			PitchBend1 = y & 0x7F;
			PitchBend2 = (y << 1) & 0xFF00;
			PitchBend = PitchBend2 | PitchBend1;
			TempEvent.message = (0xE0|channel) | (PitchBend << 8);
		}
		else if ((ListBuf[x] == 'P') && (ListBuf[x+1] == 'o') && (ListBuf[x+2] == 'r') && (ListBuf[x+11] == 'R') && (ListBuf[x+15] == ' ')) {// "Portamento Rate = "
			x += 18;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (5 << 8) | (y << 16);
		}
		else if ((ListBuf[x] == 'P') && (ListBuf[x+1] == 'o') && (ListBuf[x+2] == 'r') && (ListBuf[x+11] == 'R') && (ListBuf[x+15] == '2')) {// "Portamento Rate2 = "
			x += 19;
			y = Atoi(&ListBuf[x]);
			TempEvent.message = (0xB0|channel) | (37 << 8) | (y << 16);
		}
		else
			return FALSE;
	}
	return TRUE;
}

int CALLBACK ShowNoteProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	if (message == WM_INITDIALOG)
		SetFocus(hwndDlg);
	else if (message == WM_COMMAND) {
		if (LOWORD(wParam) == IDOK) {
			EndDialog(hwndDlg, TRUE);
		}
		else if (LOWORD(wParam) == IDCANCEL) {
			EndDialog(hwndDlg, FALSE);
		}
	}
	return 0;
}

int CALLBACK NewChannelProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x;
	static int index = 0;
	static HWND hwndEditList;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEditList = GetDlgItem(hwndDlg, IDC_LIST1);
		for (x = 0; (x < 16) && (tempBuf[x] != 0xFF); x++) { // tempBuf[x] contains channel number
			if (tempBuf[x] < 10) {
				ChannelNumber[8] = tempBuf[x] + '1';
				ChannelNumber[9] = ' ';
			}
			else {
				ChannelNumber[8] =( tempBuf[x] / 10) + '1';
				ChannelNumber[9] =( tempBuf[x] % 10) + '1';
			}
			thisInstrument = ChannelInstruments[tempBuf[x]][0];
			strcpy(&ChannelNumber[11], &myInstruments[thisInstrument][5]);
			SendMessage(hwndEditList, LB_ADDSTRING, 0, (LPARAM)ChannelNumber);
		}
		SetFocus(hwndEditList);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LIST1:
			index = SendMessage(hwndEditList, LB_GETCURSEL, 0, 0);
			NewChannel = tempBuf[index];
			break;
		case IDOK:
			for (x = 0; x < 16; x++) {// show all channels again
				if (ChannelInstruments[x][0] < 128) {
					CheckDlgButton(hwndInstruments, checks[x], BST_CHECKED);
					ActiveChannels[x] = TRUE;
				}
			}
			EndDialog(hwndDlg, TRUE);
			break;
		case IDCANCEL:
			NewChannel = 0xFF; // flag
			EndDialog(hwndDlg, FALSE);
			break;
		}
		break;
	}
	return 0;
}

int CALLBACK NewProc(HWND hwndNewDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x = 0, len;
	static HWND hwndText;
	RECT thisRect;

	switch (message)
	{
	case WM_INITDIALOG:
		GetWindowRect(hwndNewDlg, &thisRect);
		MoveWindow(hwndNewDlg, thisRect.left+100, thisRect.top + ((index-thisIndex) * Rect.bottom), Rect.right+1, Rect.bottom, FALSE);
		hwndText = GetDlgItem(hwndNewDlg, IDC_EDIT1);
		if (fromrename) {
			fromrename = FALSE;
			SetWindowText(hwndText, ListBuf);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			len = GetWindowText(hwndText, ListBuf, 512);
			if (len == 0)
				break;
			if ((ListBuf[0] != ' ') && (ListBuf[0] != '+') && (ListBuf[0] != '-')) {
				for (x = len; x >= 0; x--)
					ListBuf[x+1] = ListBuf[x];
				ListBuf[0] = ' ';
			}
			EndDialog (hwndNewDlg, TRUE);
			break;

		case IDCANCEL:
			EndDialog (hwndNewDlg, FALSE);
			break;
		}
	}
	return 0;
}

void ResetContent(DWORD *only)
{
	nonotes = nonoteoffs = volumeonly = expressiononly = panonly = reverbonly = chorusonly = modulationonly = pitchbendonly = sustainonly = bpmonly = chaninst = bank = FALSE;
	*only = TRUE;
	/////////////
	FillLogBuf();
	/////////////
	SendMessage(hwndEditList, LB_RESETCONTENT, 0, 0);
	SendMessage(hwndEditList, LB_INITSTORAGE, (int)e, (DWORD)lb);
	SetCursor(hWaitingCursor);
	for (x = 0, index = 0; x < lb; index++) {
		SendMessage(hwndEditList, LB_ADDSTRING, 0, (LPARAM)&LogBuf[x]);
		for ( ; LogBuf[x] != 0; x++)
			;
		x++;
		EventIndex[index] = *(DWORD*)&LogBuf[x];// contains the Event number for the index - necessary when list shows only a part of Event data
		x += 4;
	}
	EventIndex[index] = index;
	x = SendMessage(hwndEditList, LB_GETCOUNT, 0, 0);
	EventIndex[x] = x;
	SendMessage(hwndEditList, LB_ADDSTRING, 0, (LPARAM)"");
	SetCursor(hCursor);
	index = SendMessage(hwndEditList, LB_SETCURSEL, 0, 0);
}

int CALLBACK ListProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	RECT dlgRect;

	switch (message)
	{
	case WM_INITDIALOG:
		GetWindowRect(hwndDlg, &dlgRect);
		MoveWindow(hwndDlg, rect.right-dlgRect.right, dlgRect.top, dlgRect.right-dlgRect.left, dlgRect.bottom-dlgRect.top, FALSE);
		TempEvent.pixel = 0;
		TempEvent.pixelsinnote = 0;
		TempEvent.tickptr = 0;
		TempEvent.ticksinnote = 0;
		TempEvent.message = 0;//this is only changed with note on or off
		TempEvent.dMilliSecondsPerTick = 0;
		TempEvent.note = 0;
		TempEvent.velocity = 0;
		TempEvent.sharporflat = 0;
		TempEvent.channel = 17;//flag to ignore Event[e] (except for midiOut) (it's not in ActiveChannels)
		TempEvent.port = 0;
		TempEvent.time = 0;
		TempEvent.BeatsPerMeasure = 0;
		TempEvent.BeatNoteType = 0;
		TempEvent.KeySignature = 0;
		TempEvent.type = 0;
		TempEvent.len = 0;
		TempEvent.ptr = 0;

		showlist = TRUE;
		hwndEditList = GetDlgItem(hwndDlg, IDC_LIST1);
		//////////////////////////
		ResetContent(&everything);
		//////////////////////////
		SetFocus(hwndEditList);
		pListProc = (WNDPROC)SetWindowLong(hwndEditList, GWL_WNDPROC, (LONG)listProc);
		GetClientRect(hwndEditList, &Rect);
		Rect.bottom = SendMessage(hwndEditList, LB_GETITEMHEIGHT, 0, 0);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_LIST1:
			index = SendMessage(hwndEditList, LB_GETCURSEL, 0, 0);
			thisIndex = SendMessage(hwndEditList, LB_GETTOPINDEX, 0, 0);
			break;

		case ID_ESCTOEXIT:
			SendMessage(hwndList, WM_CLOSE, 0, 0);

		case EDIT_COPY:
			SendMessage(hwndEditList, LB_GETTEXT, index, (LPARAM)ListBuf);
			break;
		case EDIT_PASTE:
			x = Event[EventIndex[index]].pixel;
			ListBuf[1] = (x / TENTHOUSAND) + '0';
			ListBuf[2] = ((x % TENTHOUSAND) / THOUSAND) + '0';
			ListBuf[3] = ((x % THOUSAND) / HUNDRED) + '0';
			ListBuf[4] = ((x % HUNDRED) / TEN) + '0';
			ListBuf[5] = (x % TEN) + '0';
			x = SendMessage(hwndEditList, LB_INSERTSTRING, index, (LPARAM)ListBuf);
			if (CheckListBuf()) {
				SendMessage(hwndEditList, LB_SETCURSEL, index, 0);
				x = EventIndex[index];
				for (y = (int)e; y > x; y--)
					Event[y] = Event[y-1];// open up Event at x
				e++;
				TempEvent.pixel = Event[x].pixel;
				TempEvent.tickptr = Event[x].tickptr;
				Event[x] = TempEvent;
				Event[x].channel = channel;
				SaveEvents();
			}
			else
				SendMessage(hwndEditList, LB_DELETESTRING, index, 0); 
			break;
		case EDIT_DELETE:
			SendMessage(hwndEditList, LB_GETTEXT, index, (LPARAM)ListBuf);
			if ((CheckListBuf()) || (GetKeyState(VK_CONTROL) & 0x8000)) { // ctrl key down to not check entry!
				SendMessage(hwndEditList, LB_DELETESTRING, index, 0);
				SendMessage(hwndEditList, LB_SETCURSEL, index, 0);
				x = EventIndex[index];
				for ( ; x < (int)e; x++)
					Event[x] = Event[x+1];// delete Event[x]
				e--;
				SaveEvents();
			}
			break;
		case EDIT_CUT:
			SendMessage(hwndEditList, LB_GETTEXT, index, (LPARAM)ListBuf);
			if (CheckListBuf()) {
				x = SendMessage(hwndEditList, LB_DELETESTRING, index, 0);
				SendMessage(hwndEditList, LB_SETCURSEL, index, 0);
				x = EventIndex[index];
				for ( ; x < (int)e; x++)
					Event[x] = Event[x+1];// delete Event[x]
				e--;
				SaveEvents();
			}
			break;
		case EDIT_RENAME:
			// first, save original setting for Event[x].message comparison
			SendMessage(hwndEditList, LB_GETTEXT, index, (LPARAM)ListBuf);
			fromrename = TRUE;
			if (DialogBox(hInst, "NEW", hwndEditList, NewProc)) {
				if (CheckListBuf()) {
					SendMessage(hwndEditList, LB_DELETESTRING, index, 0);
					SendMessage(hwndEditList, LB_INSERTSTRING, index, (LPARAM)ListBuf);
					SendMessage(hwndEditList, LB_SETCURSEL, index, 0);
					if (Event[EventIndex[index]].pixel != TempEvent.pixel) {
						Event[EventIndex[index]].pixel = TempEvent.pixel;
						Event[EventIndex[index]].tickptr = TempEvent.pixel * TicksPerQuarterNote / 40;
					}
					if (TempEvent.message)
						Event[EventIndex[index]].message = TempEvent.message;
					else if (TempEvent.dMilliSecondsPerTick)
						Event[EventIndex[index]].dMilliSecondsPerTick = TempEvent.dMilliSecondsPerTick;
					else
						MessageBox(hwndList, "Huh?", ERROR, MB_OK);
					if (EventIndex[index] == e)
						e++;
				}
				else {
					MessageBox(hwndList, "Bad entry", "", MB_OK);
					Help = ListHelp;
					DialogBox(hInst, "LISTHELP", hwnd, Help2Proc);
					SetFocus(hwndList);
				}
				SaveEvents();
			}
			break;
		case EDIT_NEW:
			SendMessage(hwndEditList, LB_INSERTSTRING, index, (LPARAM)"");
			SendMessage(hwndEditList, LB_SETCURSEL, index, 0);
			if (DialogBox(hInst, "NEW", hwndEditList, NewProc)) {
				channel = 17; // this will change in Program Change
				///////////////////
				if (CheckListBuf()) {
				///////////////////
					SendMessage(hwndEditList, LB_DELETESTRING, index, 0);
					SendMessage(hwndEditList, LB_INSERTSTRING, index, (LPARAM)ListBuf);
					SendMessage(hwndEditList, LB_SETCURSEL, index, 0);
					for (x = 0; x < (int)e; x++)
						if (Event[x].pixel == TempEvent.pixel)
							break;
					for (y = e; y > x; y--)
						Event[y] = Event[y-1];// open up Event at index
					e++;
					Event[x] = TempEvent;
					Event[x].tickptr = Event[x].pixel * TicksPerQuarterNote / 40;
					Event[x].channel = channel;
					x = SendMessage(hwndEditList, LB_GETCOUNT, 0, 0) - 1;
					EventIndex[x] = x;
				}
				else {
					SendMessage(hwndEditList, LB_DELETESTRING, index, 0);
					SendMessage(hwndEditList, LB_SETCURSEL, index, 0);
					MessageBox(hwndList, "Bad entry", "", MB_OK);
					Help = ListHelp;
					DialogBox(hInst, "LISTHELP", hwnd, Help2Proc);
					SetFocus(hwndList);
				}
				SaveEvents();
			}
			else {
				SendMessage(hwndEditList, LB_DELETESTRING, index, 0);
				SendMessage(hwndEditList, LB_SETCURSEL, index, 0);
			}
			break;

		case ALL:
			ResetContent(&everything);
			break;
		case NONOTES:
			ResetContent(&nonotes);
			break;
		case NONOTEOFFS:
			ResetContent(&nonoteoffs);
			break;
		case VOLUME:
			ResetContent(&volumeonly);
			break;
		case EXPRESSION:
			ResetContent(&expressiononly);
			break;
		case PAN:
			ResetContent(&panonly);
			break;
		case REVERB:
			ResetContent(&reverbonly);
			break;
		case CHORUS:
			ResetContent(&chorusonly);
			break;
		case MODULATION:
			ResetContent(&modulationonly);
			break;
		case ID_SHOW_SUSTAIN:
			ResetContent(&sustainonly);
			break;
		case PITCHBEND:
			ResetContent(&pitchbendonly);
			break;
		case BEATSPERMINUTE:
			ResetContent(&bpmonly);
			break;
		case CHANNELINSTRUMENTS:
			ResetContent(&chaninst);
			break;
		case BANK:
			ResetContent(&bank);
			break;

		case HELP3:
			Help = ListHelp;
			DialogBox(hInst, "LISTHELP", hwnd, Help2Proc);
			SetFocus(hwndList);
			break;
		}
		break;

	case WM_CLOSE:
		qsort(Event, e, sizeof(Event[0]), Compare);
		Resort();
		DestroyWindow(hwndList);
		hwndList = NULL;
		showlist = FALSE;
		break;
	}
	return 0;
}

void InsertPercussion(void)
{
	DWORD LoopOffset = 0;

	do {
		for (y = 0; y < 47; y++) {
			for (x = 0; x < (LoopCols * 8); x++) {
				if (LoopNotes[y][x]) {
					Event[e].pixel = (x+LoopOffset) * 5;
					Event[e].pixelsinnote = 4;
					Event[e].tickptr = (x+LoopOffset) * 5 * TicksPerQuarterNote / 40;
					Event[e].ticksinnote = 48;
					Event[e].note = 81 - y;
					Event[e].message = 0x99 | (LoopNotes[y][x] << 16) | (Event[e].note << 8);
					Event[e].velocity = LoopNotes[y][x];
					Event[e].channel = 9;
					if (!Letter[Event[e].note - 21])
						Event[e].sharporflat = 1;
					else
						Event[e].sharporflat = 0;
					e++;
					Event[e].pixel = ((x+LoopOffset) * 5) + 4;
					Event[e].pixelsinnote = 1;
					Event[e].tickptr = ((x+LoopOffset) * 5 * TicksPerQuarterNote / 40) + (4 * TicksPerQuarterNote / 40);
					Event[e].ticksinnote = 12;
					Event[e].note = 81 - y;
					Event[e].message = 0x99 | (Event[e].note << 8);
					Event[e].velocity = 0;
					Event[e].channel = 9;
					e++;
				}
			}
		}
		LoopOffset += (LoopCols * 8);
	} while ((LoopOffset * 5) < Event[saveE-1].pixel);
}

void ConvertFromEvent(void)
{// Open
	if (GetOpenFileName(&ofn)) {
		hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			if (fileSize = GetFileSize(hFile, NULL)) {
				if (fileSize < MAX_MIDI) {
					ReadFile(hFile, Midi, fileSize, &dwBytesRead, NULL);
					if (*(DWORD*)&Midi[0] == 0x6468544D) {//"MThd" - MIDI files are in Big Endian format (not Intel format)
						ReadMidi();
						for (y = 0; y < 47; y++)
							for (x = 0; x < 120; x++)
								LoopNotes[y][x] = 0;
						for (x = 0; x < (int)e; x++) {
							if (Event[x].pixel >= (DWORD)(MaxLoopCols * 8 * 5))// 8 thirtysecond notes/quarter note and 5 pixels/column
								break;
							if ((Event[x].velocity) && (Event[x].channel == 9)) {
								LoopNotes[81 - Event[x].note][Event[x].pixel / 5] = Event[x].velocity;
							}
						}
					}
					else
						MessageBox(hwndLoop, "That's not a MIDI file.", ERROR, MB_OK);
				}
				else
					MessageBox(hwndLoop, "That MIDI file is too big.", "", MB_OK);
			}
			CloseHandle(hFile);
		}
	}
	e = saveE;
	copySize = 0;// so copyEvent isn't used with Ctrl-V after this
	for (x = 0; x < (int)e; x++)
		Event[x] = copyEvent[x];
	SetFocus(hwndLoop);
	clearscreen = TRUE;
	InvalidateRect(hwndLoop, &loopRect, FALSE);
}

void ConvertoEvent(void)
{// Save
	ZeroEvents();
	e = 0;
	Event[e].BeatsPerMeasure = 4;
	Event[e++].BeatNoteType = 4;
	Event[e++].KeySignature = 200;// C
	Event[e++].dMilliSecondsPerTick = 60000.0 / (double)(InitialBeatsPerMinute * TicksPerQuarterNote);
	for (y = 0; y < 47; y++) {
		for (x = 0; x < (LoopCols * 8); x++) {
			if (LoopNotes[y][x]) {
				Event[e].pixel = x * 5;
				Event[e].pixelsinnote = 4;
				Event[e].tickptr = x * 60;
				Event[e].ticksinnote = 48;
				Event[e].note = 81 - y;
				Event[e].message = 0x99 | (LoopNotes[y][x] << 16) | (Event[e].note << 8);
				Event[e].velocity = LoopNotes[y][x];
				Event[e].channel = 9;
				if (!Letter[Event[e].note - 21])
					Event[e].sharporflat = 1;
				Event[e].type = 0;
				e++;
				Event[e].pixel = (x * 5) + 4;
				Event[e].pixelsinnote = 1;
				Event[e].tickptr = (x * 60) + 48;
				Event[e].ticksinnote = 12;
				Event[e].note = 81 - y;
				Event[e].message = 0x99 | (Event[e].note << 8);
				Event[e].velocity = 0;
				Event[e].channel = 9;
				Event[e].type = 0;
				e++;
			}
		}
	}
	if (GetSaveFileName(&ofn)) {
		int x = ofn.nFileExtension;
		ofn.lpstrFile[x] = 'm';
		ofn.lpstrFile[x+1] = 'i';
		ofn.lpstrFile[x+2] = 'd';
		ofn.lpstrFile[x+3] = 0;
		WriteMidi();
		hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE) {
			Midi[0] = 'M';
			WriteFile(hFile, Midi, i, &dwBytesWritten, NULL);// i from WriteMidi
			Midi[0] = 0;
			CloseHandle(hFile);
		}
	}
	e = saveE;
	copySize = 0;// so copyEvent isn't used with Ctrl-V after this
	for (x = 0; x < (int)e; x++)
		Event[x] = copyEvent[x];
	SetFocus(hwndLoop);
}

void CALLBACK DrumTimerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	int i;

	if (CurrentNotes != -1) {
		for (i = 0; i < 47; i++)
			if (LoopNotes[i][CurrentNotes])
				midiOutShortMsg(hMidiOut, 0x99 | ((81-i) << 8));// MIDI note 81 is triangle
		PreviousNotes = CurrentNotes;
	}
	CurrentNotes++;
	if (CurrentNotes == (LoopCols * 8))
		CurrentNotes = 0;
	for (i = 0; i < 47; i++)
		if (LoopNotes[i][CurrentNotes])
			midiOutShortMsg(hMidiOut, 0x99 | (LoopNotes[i][CurrentNotes] << 16) | ((81-i) << 8));
	if (metronome == FALSE)
		PostMessage(hwnd, WM_USER5, 0, 0);
}

void DrumBeginSequence(void)
{
	playing2 = TRUE;
	ModifyMenu(hMenu2, PLAY2, MF_BYCOMMAND|MF_STRING, PLAY2, Stop2);
	DrawMenuBar(hwndLoop);
	CurrentNotes = -1;
	timeBeginPeriod(TIMER_RESOLUTION);
	milliSecondsPerBeat = (int)(dMilliSecondsPerTick * (double)TicksPerQuarterNote);
	time = milliSecondsPerBeat / 8;// 8 thirtysecond notes in a quarter note
	timeBeginPeriod(TIMER_RESOLUTION);
	uTimer0ID = timeSetEvent(time, TIMER_RESOLUTION, DrumTimerFunc, 0, TIME_PERIODIC);
}

void DrumEndSequence()
{
	playing2 = FALSE;
	ModifyMenu(hMenu2, PLAY2, MF_BYCOMMAND|MF_STRING, PLAY2, Play);
	DrawMenuBar(hwndLoop);
	if (uTimer0ID) {
		timeKillEvent(uTimer0ID);
		uTimer0ID = 0;
		timeEndPeriod(TIMER_RESOLUTION);
		midiOutShortMsg(hMidiOut, 0xB9 | (123 << 8));// All Channel 9 Notes Off
	}
	CurrentNotes = -1;
	PreviousNotes = 0;
	clearscreen = TRUE;
	InvalidateRect(hwndLoop, &loopRect, FALSE);
}

int CALLBACK NoteVolumeProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char textVol[4];
	static HWND hwndEdit, hwndSpin;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndSpin = GetDlgItem(hwndDlg, IDC_SPIN1);
		_itoa(Volume, textVol, 10);
		SetWindowText(hwndEdit, textVol);
		SendMessage(hwndSpin, UDM_SETRANGE, 0, (LPARAM)MAKELONG((SHORT)127, (SHORT)1));
		SetFocus(hwndDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			if (0 == GetWindowText(hwndEdit, textVol, 4)) {
				SetFocus(hwndEdit);
				break;
			}
			LoopNoteVol = atoi(textVol);
			if (x == 0)
				LoopNoteVol = 1;
			else if (x > 127)
				LoopNoteVol = 127;
			EndDialog (hwndDlg, TRUE);
			return TRUE;
		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return 0;
}
/*
//Circle of Fifths in Major Mode: C G D A E B F# Bb Eb Ab Db F
//in Major Mode: G=1# D=2# A=3# E=4# B=5# F#=6# Db=5b Ab=4b Eb=3b Bb=2b F=1b 
void GetDiatonicNotes(int modefromA, int mode)
{
	int firstnote, u, v, w, x, y, z;
	static int akeys[] = {2,1,2,2,1,2,2}; // semitones between Diatonic notes in Mode A
	static int SharpFlats[12] = {0,1,1,1,1,1,1,2,2,2,2,2}; // Circle of Fifths
	static char Chromatic1[12][5] = {" A  "," Bb "," B  "," C  "," Db "," D  "," Eb "," E  "," F  "," Gb "," G  "," Ab "}; // C mode
	static char Chromatic2[12][5] = {" A  "," Bb "," B  "," C  "," C# "," D  "," Eb "," E  "," F  "," F# "," G  "," G# "}; // D
	static char Chromatic3[12][5] = {" A  "," A# "," B  "," C  "," C# "," D  "," D# "," E  "," F  "," F# "," G  "," G# "}; // E
	static char Chromatic4[12][5] = {" A  "," Bb "," B  "," C  "," Db "," D  "," Eb "," E  "," F  "," Gb "," G  "," Ab "}; // F
	static char Chromatic5[12][5] = {" A  "," Bb "," B  "," C  "," C# "," D  "," Eb "," E  "," F  "," F# "," G  "," Ab "}; // G
	static char Chromatic6[12][5] = {" A  "," Bb "," B  "," C  "," C# "," D  "," D# "," E  "," F  "," F# "," G  "," G# "}; // A
	static char Chromatic7[12][5] = {" A  "," A# "," B  "," C  "," C# "," D  "," D# "," E  "," F  "," F# "," G  "," G# "}; // B

	for (x = 0, y = 0; y < modefromA; y++)
		if (Chromatic2[y][2] == ' ')
			x++; // pointer into akeys
	for (v = 0; v < 12; v++) {
		firstnote = v+modefromA;
		if (firstnote >= 12)
			firstnote -= 12;
		y = firstnote; // pointer into Chromatic array
		for (w = 0, u = 0; u < 7; u++, w++, x++) {
			switch (mode)
			{
			case 1: // C
				for (z = 0; z < 4; z++)
					Diatonic[v][w][z] = Chromatic1[y][z];
				break;
			case 2: // D
				for (z = 0; z < 4; z++)
					Diatonic[v][w][z] = Chromatic2[y][z];
				break;
			case 3: // E
				for (z = 0; z < 4; z++)
					Diatonic[v][w][z] = Chromatic3[y][z];
				break;
			case 4: // F
				for (z = 0; z < 4; z++)
					Diatonic[v][w][z] = Chromatic4[y][z];
				break;
			case 5: // G
				for (z = 0; z < 4; z++)
					Diatonic[v][w][z] = Chromatic5[y][z];
				break;
			case 6: // A
				for (z = 0; z < 4; z++)
					Diatonic[v][w][z] = Chromatic6[y][z];
				break;
			case 7: // B
				for (z = 0; z < 4; z++)
					Diatonic[v][w][z] = Chromatic7[y][z];
				break;
			}
			if (x == 7)
				x = 0;
			y += akeys[x];
			if (y >= 12)
				y -= 12;
		}
	}
}
*/
LRESULT CALLBACK ChordsProc(HWND hwndChords, UINT message, WPARAM wParam, LPARAM lParam)
{
	int w, x, y, z;
	static int xKeyLoc[12], yKeyLoc[12], cp, key, keynames, TotalIntervals, ChordNote, ChordTicks, xPos, yPos, scale;
	static int Intervals1[] = {4,3,0, 4,3,4,0, 4,3,4,3,3,4,0, 2,2,1,2,2,2,1,0}; // C mode - Triad, Major7th, 13th Chord, Diatonic Scale
	static int Intervals2[] = {3,4,0, 3,4,3,0, 3,4,3,4,3,4,0, 2,1,2,2,2,1,2,0}; // D
	static int Intervals3[] = {3,4,0, 3,4,3,0, 3,4,3,3,4,3,0, 1,2,2,2,1,2,2,0}; // E
	static int Intervals4[] = {4,3,0, 4,3,4,0, 4,3,4,3,4,3,0, 2,2,2,1,2,2,1,0}; // F
	static int Intervals5[] = {4,3,0, 4,3,3,0, 4,3,3,4,3,4,0, 2,2,1,2,2,1,2,0}; // G
	static int Intervals6[] = {3,4,0, 3,4,3,0, 3,4,3,4,3,3,0, 2,1,2,2,1,2,2,0}; // A
	static int Intervals7[] = {3,3,0, 3,3,4,0, 3,3,4,3,4,3,0, 1,2,2,1,2,2,2,0}; // B
	static int *tomp[7] = {Intervals1, Intervals2, Intervals3, Intervals4, Intervals5, Intervals6, Intervals7};
	static int *Intervals;
	static int IntervalPtr[] = {0,3,7,14};

	static int DiatonicIntervals1[] = {0,2,4,5,7,9,11,12}; // C mode (additive)
	static int DiatonicIntervals2[] = {0,2,3,5,7,9,10,12}; // D
	static int DiatonicIntervals3[] = {0,1,3,5,7,8,10,12}; // E
	static int DiatonicIntervals4[] = {0,2,4,6,7,9,11,12}; // F
	static int DiatonicIntervals5[] = {0,2,4,5,7,9,10,12}; // G
	static int DiatonicIntervals6[] = {0,2,3,5,7,8,10,12}; // A
	static int DiatonicIntervals7[] = {0,1,3,5,6,8,10,12}; // B
	static int *DiatonicIntervals;

	static int BottomNotes1[12] = {60,67,62,57,64,59,66,61,56,63,58,65}; // C mode - MIDI note numbers for {C, G, D, A, E, B, F#, Db, Eb, Ab, Bb, F}
	static int BottomNotes2[12] = {62,57,64,59,66,61,56,63,58,65,60,67}; // D
	static int BottomNotes3[12] = {64,59,66,61,56,63,58,65,60,67,62,57}; // E
	static int BottomNotes4[12] = {65,60,67,62,57,64,59,66,61,56,63,58}; // F
	static int BottomNotes5[12] = {67,62,57,64,59,66,61,56,63,58,65,60}; // G
	static int BottomNotes6[12] = {57,64,59,66,61,56,63,58,65,60,67,62}; // A
	static int BottomNotes7[12] = {59,66,61,56,63,58,65,60,67,62,57,64}; // B
	static int *BottomNotes;
//MIDI note num:					21  22  23  24  25  26  27  28  29  30  31  32  33  34  35  36  37  38  39  40  41  42  43  44  45  46  47  48  49  50  51  52  53  54  55  56  57  58  59  60  61  62  63  64  65  66  67  68  69  70  71  72  73  74  75  76  77  78  79  80  81  82  83  84  85  86  87  88  89  90  91  92  93  94  95  96  97 98  99  100  101 102 103 104 105 106 107 108
static int BottomNoteLoc[] ={337,337,331,325,325,319,319,313,307,307,301,301,295,295,289,283,283,277,277,271,265,265,259,259,253,253,247,241,241,235,235,229,223,223,217,217,211,211,205,199,199,193,193,187,181,181,175,175,169,169,163,157,157,151,151,145,139,139,133,133,127,127,121,115,115,109,109,103, 97, 97, 91, 91, 85, 85, 79, 73, 73, 67, 67, 61, 55, 55, 49, 49, 43, 43, 37, 31};
//									 {'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C', 0, 'D', 0, 'E','F', 0, 'G', 0, 'A', 0, 'B','C'};// 88 notes
	static double d1, d2, d3, d4;
	static char *ChordsPtr[4] = {" Triad "," 7th Chord "," 13th Chord "," Scale "};
	static char CKeys[12][4] = {" C  "," G  "," D  "," A  "," E  "," B  "," F# "," Db "," Ab "," Eb "," Bb "," F  "};// Major
	static char DKeys[12][4] = {" D  "," A  "," E  "," B  "," F# "," C# "," G# "," Eb "," Bb "," F  "," C  "," G  "};
	static char EKeys[12][4] = {" E  "," B  "," F# "," C# "," G# "," D# "," A# "," F  "," C  "," G  "," D  "," A  "};
	static char FKeys[12][4] = {" F  "," C  "," G  "," D  "," A  "," E  "," B  "," Gb "," Db "," Ab "," Eb "," Bb "};
	static char GKeys[12][4] = {" G  "," D  "," A  "," E  "," B  "," F# "," C# "," Ab "," Eb "," Bb "," F  "," C  "};
	static char AKeys[12][4] = {" A  "," E  "," B  "," F# "," C# "," G# "," D# "," Bb "," F  "," C  "," G  "," D  "};// minor
	static char BKeys[12][4] = {" B  "," F# "," C# "," G# "," D# "," A# "," F  "," C  "," G  "," D  "," A  "," E  "};
	static char *Keys[12];
	static char *Modes[8] = {"     MODE"," C ionian  MAJOR "," D dorian "," E phyrgian "," F lydian "," G mixolydian "," A aeolian  MINOR "," B locrian "};
	static char *Degrees[8] = {"DEGREE"," I "," II "," III "," IV "," V "," VI "," VII "};
	static int KeySigX[12]  = { 365,   375,   385,   395,   405,   415,   425,   415,   405,   395,   385,   375  };
	static int KeySigY[12]  = {  65,    69,    87,    63,    81,    99,    75,   105,    81,    99,    75,    94  };
	static int mode, degree, inversion, modefromA = 3; // C Major
	static int Inversion[4] = {200,220,240,260};
	static char *Inversions[4] = {"  No Inversion"," 1st Inversion"," 2nd Inversion"," 3rd Inversion"};
	static signed char InversionOffsets[4][3] = {0,0,0, -12,0,0, 0,-12,0, 0,0,-12};
	static signed char InversionOffsets2[4][3]= {0,0,0, -12,-12,-12, 0,-12,-12, 0,0,-12};
	static HWND hwndHelp3;
	static HDC hdc3;
	static PAINTSTRUCT ps3;
	static RECT rect3;
	static HBRUSH hChords2BlueBrush;

	switch (message)
	{
	case WM_CREATE:
		hwndHelp3 = CreateWindow("BUTTON", "&HELP",
			WS_CHILD | WS_VISIBLE,
			480, 270, 72, 32, // 480 was 220
			hwndChords, (HMENU)192, hInst, NULL);
		chord = 1;// CM
		GetClientRect(hwndChords, &rect3);
		d1 = (double)110 * cos(60.0 * PI / 180.0);//for circle of fifths
		d2 = (double)110 * sin(60.0 * PI / 180.0);
		d3 = (double)110 * cos(30.0 * PI / 180.0);
		d4 = (double)110 * sin(30.0 * PI / 180.0);
		xKeyLoc[0] = 390;
		yKeyLoc[0] = 20;
		xKeyLoc[1] = (int)d1 + 390;
		yKeyLoc[1] = 130 - (int)d2;
		xKeyLoc[2] = (int)d3 + 390;
		yKeyLoc[2] = 130 - (int)d4;
		xKeyLoc[3] = 510;
		yKeyLoc[3] = 130;
		xKeyLoc[4] = xKeyLoc[2];
		yKeyLoc[4] = 130 + (int)d4;
		xKeyLoc[5] = xKeyLoc[1];
		yKeyLoc[5] = 130 + (int)d2;
		xKeyLoc[6] = 390;
		yKeyLoc[6] = 240; // F#
//		xKeyLoc[7] = 390;
//		yKeyLoc[7] = 260; // Gb
		xKeyLoc[7] = 390 - (int)d1;
		yKeyLoc[7] = 240 - 110 + (int)d2;
		xKeyLoc[8] = 390 - (int)d3;
		yKeyLoc[8] = 240 - 110 + (int)d4;
		xKeyLoc[9] = 290;
		yKeyLoc[9] = 130;
		xKeyLoc[10] = 390 - (int)d3;
		yKeyLoc[10] = 240 - 110 - (int)d4;
		xKeyLoc[11] = 390 - (int)d1;
		yKeyLoc[11] = 240 - 110 - (int)d2;
		xKeyLoc[12] = 390;
		yKeyLoc[12] = 260; // Gb
		d1 = (double)90 * cos(60.0 * PI / 180.0);//for circle of fifths
		d2 = (double)90 * sin(60.0 * PI / 180.0);
		d3 = (double)90 * cos(30.0 * PI / 180.0);
		d4 = (double)90 * sin(30.0 * PI / 180.0);
		cp = key = inversion = 0;
		mode = degree = 1;
		Intervals = tomp[0]; // Intervals1;
		DiatonicIntervals = DiatonicIntervals1;
		BottomNotes = BottomNotes1;
		for (x = 0; x < 12; x++)
			Keys[x] = CKeys[x];
//		GetDiatonicNotes(modefromA, mode);
		break;

	case WM_SYSKEYDOWN:
		if (wParam == 'H') {
			Help = ChordsHelp;
			DialogBox(hInst, "CHORDSHELP", hwnd, Help2Proc);
			SetFocus(hwndChords);
		}
		return 0;// not break;

	case WM_CHAR:
		if ((wParam >= '1') && (wParam <= '3')) {
			inversion = wParam - '0';
			InvalidateRect(hwndChords, &rect3, FALSE);
		}
		else if (wParam == '0') {
			inversion = 0;
			InvalidateRect(hwndChords, &rect3, FALSE);
		}
		break;
		
	case WM_KEYDOWN:
		switch (wParam)
		{
		case VK_LEFT:
			if (key == 0)
				key = 11;
			else
				key--;
			InvalidateRect(hwndChords, &rect3, FALSE);
			break;
		case VK_RIGHT:
			if (key == 11)
				key = 0;
			else
				key++;
			InvalidateRect(hwndChords, &rect3, FALSE);
			break;
		case VK_DOWN:
			if (cp < 3) {
				cp++;
				chord++;
				InvalidateRect(hwndChords, &rect3, FALSE);
			}
			break;
		case VK_UP:
			if (cp) {
				cp--;
				chord--;
				InvalidateRect(hwndChords, &rect3, FALSE);
			}
			break;
		case VK_ESCAPE:
			SendMessage(hwndChords, WM_CLOSE, 0, 0);
			break;
		case VK_RETURN:
		case VK_SPACE:
			if (0 == (lParam & 0x40000000))// bit 30 is set if auto-repeating
				SendMessage(hwndChords, WM_COMMAND, (WPARAM)ID_PLAY, 0);
			break;
		case 'H':
			SendMessage(hwndChords, WM_COMMAND, (WPARAM)ID_CHORDS2HELP, 0);
			break;
		case VK_PRIOR:
			SendMessage(hwndChords, WM_COMMAND, (WPARAM)ID_OCTAVEUP, 0);
			break;
		case VK_NEXT:
			SendMessage(hwndChords, WM_COMMAND, (WPARAM)ID_OCTAVEDOWN, 0);
			break;
		}
		break;

	case WM_LBUTTONDOWN:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		if (xPos < 160) {
			if (yPos > 260)
				inversion = 3;
			else if (yPos > 240)
				inversion = 2;
			else if (yPos > 220)
				inversion = 1;
			else if (yPos > 200)
				inversion = 0;

			else if (yPos < 180) {
				if (yPos > 160) { // B
					mode = 7;
					modefromA = 2; // " A  "," Bb "," B  "," C  "," Db "," D  "," Eb "," E  "," F  "," F# "," G  "," Ab "
					DiatonicIntervals = DiatonicIntervals7;
					BottomNotes = BottomNotes7;
					for (x = 0; x < 12; x++)
						Keys[x] = BKeys[x];
				}
				else if (yPos > 140) { // A
					mode = 6;
					modefromA = 0;
					DiatonicIntervals = DiatonicIntervals6;
					BottomNotes = BottomNotes6;
					for (x = 0; x < 12; x++)
						Keys[x] = AKeys[x];
				}
				else if (yPos > 120) { // G
					mode = 5;
					modefromA = 10;
					DiatonicIntervals = DiatonicIntervals5;
					BottomNotes = BottomNotes5;
					for (x = 0; x < 12; x++)
						Keys[x] = GKeys[x];
				}
				else if (yPos > 100) { // F
					mode = 4;
					modefromA = 8;
					DiatonicIntervals = DiatonicIntervals4;
					BottomNotes = BottomNotes4;
					for (x = 0; x < 12; x++)
						Keys[x] = FKeys[x];
				}
				else if (yPos > 80) { // E
					mode = 3;
					modefromA = 7;
					DiatonicIntervals = DiatonicIntervals3;
					BottomNotes = BottomNotes3;
					for (x = 0; x < 12; x++)
						Keys[x] = EKeys[x];
				}
				else if (yPos > 60) { // D
					mode = 2;
					modefromA = 5;
					DiatonicIntervals = DiatonicIntervals2;
					BottomNotes = BottomNotes2;
					for (x = 0; x < 12; x++)
						Keys[x] = DKeys[x];
				}
				else if (yPos > 40) { // C
					mode = 1;
					modefromA = 3;
					DiatonicIntervals = DiatonicIntervals1;
					BottomNotes = BottomNotes1;
					for (x = 0; x < 12; x++)
						Keys[x] = CKeys[x];
				}
				if ((mode-1+degree-1) < 7)
					Intervals = tomp[mode-1+degree-1];
				else
					Intervals = tomp[(mode-1+degree-1) - 7];
//				GetDiatonicNotes(modefromA, mode);
			}
			InvalidateRect(hwndChords, &rect3, FALSE);
		}
		if ((xPos > 160) && (xPos < 230)) {
			if ((yPos > 200) && (yPos < 280)) {
				chord = (yPos-180) / 20; // chord = 1 for Triad Chord
				cp = chord - 1;
			}
			else if (yPos < 180) {
				if (yPos > 160)
					degree = 7;
				else if (yPos > 140)
					degree = 6;
				else if (yPos > 120)
					degree = 5;
				else if (yPos > 100)
					degree = 4;
				else if (yPos > 80)
					degree = 3;
				else if (yPos > 60)
					degree = 2;
				else if (yPos > 40)
					degree = 1;
				if ((mode-1+degree-1) < 7)
					Intervals = tomp[mode-1+degree-1];
				else
					Intervals = tomp[(mode-1+degree-1) - 7];
			}
			InvalidateRect(hwndChords, &rect3, FALSE);
		}
		for (x = 0; x < 12; x++) {
			if ((xPos >= xKeyLoc[x]) && (xPos <= xKeyLoc[x] + 20) && (yPos >= yKeyLoc[x]) && (yPos <= yKeyLoc[x] + 20)) {
				key = x;
				InvalidateRect(hwndChords, &rect3, FALSE);
				break;
			}
		}
		break;

	case WM_COMMAND:
		if ((HWND)(LPARAM)lParam == hwndHelp3) {
			Help = ChordsHelp;
			DialogBox(hInst, "CHORDSHELP", hwnd, Help2Proc);
			SetFocus(hwndChords);
		}
		switch (LOWORD(wParam))
		{
		case ID_CLOSE:
			SendMessage(hwndChords, WM_CLOSE, 0, 0);
			break;

		case ID_PLAY:
			if (cp < 4) {
				ChordNote = BottomNotes[key]+DiatonicIntervals[degree-1];
				switch (notetype[1])
				{
				case 'Q':
					ChordTicks = TicksPerQuarterNote;
					break;
				case 'E':
					ChordTicks = TicksPerQuarterNote / 2;
					break;
				case 'S':
					ChordTicks = TicksPerQuarterNote / 4;
					break;
				case 'T':
					ChordTicks = TicksPerQuarterNote / 8;
					break;
				case 'F':
					ChordTicks = TicksPerQuarterNote / 16;
					break;
				case 'H':
					ChordTicks = TicksPerQuarterNote * 2;
					break;
				case 'W':
					ChordTicks = TicksPerQuarterNote * 4;
					break;
				}
				cn = 0;
				Chords2Notes[cn++] = ChordNote;
				if (!EWQL)
					midiOutShortMsg(hMidiOut, (0x90|ActiveChannel) | (ChordNote << 8) | (Volume << 16));
				else
					midiOutShortMsg(hMidisOut[Port], (0x90|ActiveChannel) | (ChordNote << 8) | (Volume << 16));
				for (x = IntervalPtr[cp], y = 0; Intervals[x] != 0; x++, y++) {
					ChordNote += (Intervals[x] + InversionOffsets[inversion][y]);
					Chords2Notes[cn++] = ChordNote;
					if (!EWQL)
						midiOutShortMsg(hMidiOut, (0x90|ActiveChannel) | (ChordNote << 8) | (Volume << 16));
					else
						midiOutShortMsg(hMidisOut[Port], (0x90|ActiveChannel) | (ChordNote << 8) | (Volume << 16));
				}
				timeBeginPeriod(TIMER_RESOLUTION);
				uTimer10ID = timeSetEvent((UINT)(dMilliSecondsPerTick * (double)ChordTicks), TIMER_RESOLUTION, Chords2TimerFunc, 0, TIME_ONESHOT);
				SetFocus(hwndChords);
			}
			break;

		case ID_OCTAVEUP:
			for (x = 0; x < 12; x++)
				BottomNotes[x] += 12;
			SetFocus(hwndChords);
			InvalidateRect(hwndChords, &rect3, FALSE);
			break;
		case ID_OCTAVEDOWN:
			for (x = 0; x < 12; x++)
				BottomNotes[x] -= 12;
			SetFocus(hwndChords);
			InvalidateRect(hwndChords, &rect3, FALSE);
			break;

		case ID_CHORDS2HELP:
			Help = ChordsHelp;
			DialogBox(hInst, "CHORDSHELP", hwnd, Help2Proc);
			SetFocus(hwndChords);
			break;
		}
		break;

	case WM_PAINT:
		hdc3 = BeginPaint(hwndChords, &ps3);
		FillRect(hdc3, &rect3, hDialogBrush);
		SelectObject(hdc3, hChords2BlueBrush);
		GetTextExtentPoint32(hdc3, Keys[key], 4, &size);
		Rectangle(hdc3, xKeyLoc[key], yKeyLoc[key], xKeyLoc[key]+size.cx, yKeyLoc[key]+size.cy);
		GetTextExtentPoint32(hdc3, ChordsPtr[cp], strlen(ChordsPtr[cp]), &size);
		Rectangle(hdc3, 160, 200+(20*cp), 160+size.cx, 200+(20*cp)+size.cy);
		SetBkMode(hdc3, TRANSPARENT);
		GetTextExtentPoint32(hdc3, Inversions[0], 5, &size);
		Rectangle(hdc3, 10, Inversion[inversion], 10+size.cx, Inversion[inversion]+17);
		GetTextExtentPoint32(hdc3, Modes[mode], strlen(Modes[mode]), &size);
		Rectangle(hdc3, 10, 20+(mode*20), 10+size.cx, 20+(mode*20)+17);
		GetTextExtentPoint32(hdc3, Degrees[degree], strlen(Degrees[degree]), &size);
		Rectangle(hdc3, 160, 20+(degree*20), 160+size.cx, 20+(degree*20)+17);
		TextOut(hdc3, 387, 265, "KEY", 3);

		for (x = 0, y = 20, z = 0; z < 8; x++, y += 20, z++)
			TextOut(hdc3, 10, y, Modes[x], strlen(Modes[x]));
		for (x = 0, y = 20; x < 8; x++, y += 20, z++)
			TextOut(hdc3, 160, y, Degrees[x], strlen(Degrees[x]));
		for (x = 0, y = 200; x < 4; y += 20, x++)
			TextOut(hdc3, 10, y, Inversions[x], strlen(Inversions[x]));
		for (x = 0, y = 200; x < 4; y += 20, x++)
			TextOut(hdc3, 160, y, ChordsPtr[x], strlen(ChordsPtr[x]));

		for (x = 0; x < 12; x++) {
			TextOut(hdc3, xKeyLoc[x], yKeyLoc[x], Keys[x], 3);
		}
		for (y = 7; y < 108; y += 12) {
			MoveToEx(hdc3, 600, y, NULL);
			LineTo(hdc3, 610, y);
		}
		for (y = 115; y < 175; y += 12) {
			MoveToEx(hdc3, 570, y, NULL);
			LineTo(hdc3, 640, y);
		}
		MoveToEx(hdc3, 600, 175, NULL);
		LineTo(hdc3, 610, 175);
		for (y = 187; y < 247; y += 12) {
			MoveToEx(hdc3, 570, y, NULL);
			LineTo(hdc3, 640, y);
		}
		for (y = 247; y < 319; y += 12) {
			MoveToEx(hdc3, 600, y, NULL);
			LineTo(hdc3, 610, y);
		}
		////////////////////////////////// put note names on staffs
		if (cp == 3)
			scale = 2;
		else
			scale = 0;
		nc = 0;
		if (Letter[BottomNotes[key]+DiatonicIntervals[degree-1]-21]) { // not # or b
			y = BottomNotes[key]+DiatonicIntervals[degree-1]-21; //  NoteLoc starts at MIDI note number 21
			Y = BottomNoteLoc[y]-31; // BottomNoteLoc is the same as the default NoteLoc (but it doesn change like NoteLoc can)
			if (scale == 2) {
				TextOut(hdc3, 590, Y, &Letter[y], 1);
				scale ^= 1;
			}
			else if (scale == 3) {
				TextOut(hdc3, 610, Y, &Letter[y], 1);
				scale ^= 1;
			}
			else
				TextOut(hdc3, 600, Y, &Letter[y], 1); // bottom note (chord root)
		}
		else {
			y = BottomNotes[key]+DiatonicIntervals[degree-1]-21+1;
			Y = BottomNoteLoc[y]-32;
			if (key >= 7) {
				temp[0] = Letter[y];
				temp[1] = 'b';
			}
			else {
				temp[0] = Letter[y-2];
				temp[1] = '#';
			}
			if (scale == 2) {
				TextOut(hdc3, 590, Y, temp, 2);
				scale ^= 1;
			}
			else if (scale == 3) {
				TextOut(hdc3, 610, Y, temp, 2);
				scale ^= 1;
			}
			else
				TextOut(hdc3, 600, Y, temp, 2); // sharped or flatted bottom note
		}
		Notes2Chords[nc++] = BottomNotes[key]+DiatonicIntervals[degree-1];
		TotalIntervals = 0;
		for (x = IntervalPtr[cp], z = 0; Intervals[x] != 0; x++, z++) {
			if (cp == 2) { // 13th chord
				SetTextColor(hdc3, GREY);
				if (Intervals[x] == 4)
					TextOut(hdc3, 620, Y-6, "M", 1); // Major third
				else
					TextOut(hdc3, 620, Y-6, "m", 1); // minor third
				SetTextColor(hdc3, 0);
			}
			TotalIntervals += Intervals[x];
			if (z < 3)
				w = InversionOffsets2[inversion][z];
			else
				w = 0;
			if (Letter[BottomNotes[key]+DiatonicIntervals[degree-1]-21+TotalIntervals]) {
				y = BottomNotes[key]+DiatonicIntervals[degree-1]-21+TotalIntervals+w;
				Y = BottomNoteLoc[y]-32;
				if (scale == 2) {
					TextOut(hdc3, 590, Y, &Letter[y], 1);
					scale ^= 1;
				}
				else if (scale == 3) {
					TextOut(hdc3, 610, Y, &Letter[y], 1);
					scale ^= 1;
				}
				else
					TextOut(hdc3, 600, Y, &Letter[y], 1);
			}
			else if ((key >= 7) || (key == 0)) {// flat notes
				y = BottomNotes[key]+DiatonicIntervals[degree-1]-21+TotalIntervals+1+w;
				Y = BottomNoteLoc[y]-32;
				temp[0] = Letter[y];
				temp[1] = 'b';
				if (scale == 2) {
					TextOut(hdc3, 590, Y, temp, 2);
					scale ^= 1;
				}
				else if (scale == 3) {
					TextOut(hdc3, 610, Y, temp, 2);
					scale ^= 1;
				}
				else
					TextOut(hdc3, 600, Y, temp, 2);
			}
			else {// sharp notes
				y = BottomNotes[key]+DiatonicIntervals[degree-1]-21+TotalIntervals-1+w;
				Y = BottomNoteLoc[y]-32;
				temp[0] = Letter[y];
				temp[1] = '#';
				if (scale == 2) {
					TextOut(hdc3, 590, Y, temp, 2);
					scale ^= 1;
				}
				else if (scale == 3) {
					TextOut(hdc3, 610, Y, temp, 2);
					scale ^= 1;
				}
				else
					TextOut(hdc3, 600, Y, temp, 2);
			}
			Notes2Chords[nc++] = BottomNotes[key]+DiatonicIntervals[degree-1]+TotalIntervals+w;
		} // end of for (x = IntervalPtr[cp],
		Notes2Chords[nc] = 0;
		///////////////////////////////////
		MoveToEx(hdc3, 395, 140, NULL);
		LineTo(hdc3, 405, 140);// middle C
		for (y = 152; y < (152+60); y += 12) {
			MoveToEx(hdc3, 365, y, NULL);
			LineTo(hdc3, 435, y);
		}
		for (y = 128; y > (128-60); y -= 12) {
			MoveToEx(hdc3, 365, y, NULL);
			LineTo(hdc3, 435, y);
		}
		hOldFont = SelectObject(hdc3, hFont);
		if (key <= 6) {
			for (x = key; x; x--) {
				TextOut(hdc3, KeySigX[x], KeySigY[x], "#", 1);
				TextOut(hdc3, KeySigX[x], KeySigY[x]+84, "#", 1);
			}
		}
		else {
			for (x = key; x < 12; x++) {
				TextOut(hdc3, KeySigX[x], KeySigY[x], "b", 1);
				TextOut(hdc3, KeySigX[x], KeySigY[x]+84, "b", 1);
			}
		}
		SelectObject(hdc3, hOldFont);
		EndPaint(hwndChords, &ps3);
		break;

	case WM_CLOSE:
		chord = 0;
		DestroyWindow(hwndChords);
		break;
	}
	return DefWindowProc(hwndChords, message, wParam, lParam);
}

int compare(const void *x, const void *y)
{
	if (*(DWORD*)x < *(DWORD*)y)
		return -1;
	else if (*(DWORD*)x > *(DWORD*)y)
		return 1;
	else
		return 0;
}

int CALLBACK CutSectionProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static char Begin[] = "Beginning at        ";
	static char End[] = "Ending at           ";
	static HWND hwndBegin, hwndEnd;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndBegin = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndEnd = GetDlgItem(hwndDlg, IDC_EDIT2);
		_itoa(Beginning, &Begin[13], 10);
		SetWindowText(hwndBegin, Begin);
		if (Ending != 0x7FFFFFFF) {
			_itoa(Ending, &End[10], 10);
			SetWindowText(hwndEnd, End);
		}
		SetFocus(hwndDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			EndDialog (hwndDlg, TRUE);
			break;
		case IDCANCEL:
			beginning = FALSE;
			Ending = 0x7FFFFFFF;
			EndDialog (hwndDlg, FALSE);
			break;
		}
	}
	return FALSE;
}

void ShowRatios(int ratio)
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

void CALLBACK waveTimerFunc(UINT uID, UINT uMsg, DWORD dwUser, DWORD dw1, DWORD dw2)
{
	PostMessage(hwnd, WM_USER14, 0, 0);
}

LRESULT CALLBACK EditMyWaveProc(HWND hwndEditMyWave, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int x, y, z, WavePtr, WaveLength, WaveStart, divisor, rectrightx3, ratio, fileSize2;
	static DWORD time1, time2, BytesPerSec, SamplesPerSec, BytesPer100MilliSeconds;
	static unsigned int uTimer14ID = 0;
	static WORD w, Channels, BitsPerSample, BytesPerSample = 0, BytesPerSampleXChannels;
	static SHORT s;
	static BYTE *myWaveBuf;
	static BOOL first = TRUE, oddeven;
	static char WavePos[8], Loudness[8];
	static char WaveHeight[] = "Vertical Scale = 1:1";
	static char WaveQuality[MAX_PATH];
	static RECT rect;
	static HDC hdc2, hdcMem2;
	static PAINTSTRUCT ps2;
	static HBITMAP hBitmap2;

	switch (message)
	{
	case WM_CREATE:
		myWaveBuf = NULL;
		PTR = 0;
		Channels = 1;
		WaveLength = 0x7FFFFFFF;
		beginning = FALSE;
		Ending = 0x7FFFFFFF;
		WaveQuality[0] = 0;
		GetClientRect(hwndEditMyWave, &rect);
		rectrightx3 = rect.right * 3;
		middle = rect.bottom / 2;
		DrawMenuBar(hwndEditMyWave);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILES_OPEN:
			if (GetOpenFileName(&ofn2)) {
				SetFocus(hwndEditMyWave);
				hFile2 = CreateFile(FullFilename2, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
				if (hFile2 != INVALID_HANDLE_VALUE) {
					if (fileSize2 = GetFileSize(hFile2, NULL)) {
						if (myWaveBuf)
							VirtualFree(myWaveBuf, 0, MEM_RELEASE);
						myWaveBuf = VirtualAlloc(NULL, fileSize2, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
						ReadFile(hFile2, myWaveBuf, fileSize2, &dwBytesRead, NULL);
						CloseHandle(hFile2);
						if (*(DWORD*)&myWaveBuf[8] == 0x45564157) {// "WAVE"
							subchunksize = *(DWORD*)&myWaveBuf[16];
							nextchunk = subchunksize + 20;
							if (*(DWORD*)&myWaveBuf[nextchunk] == 0x74636166)// "fact"
								nextchunk += 12;
							if (*(DWORD*)&myWaveBuf[nextchunk] == 0x61746164) {// "data"
								PTR = 0;
								fromuser14 = FALSE;
								if (uTimer14ID) {
									timeKillEvent(uTimer14ID);
									uTimer14ID = 0;
									timeEndPeriod(TIMER_RESOLUTION);
								}
								playingwave = FALSE;
								ratio = 16;
								WaveHeight[17] = '1';
								WaveHeight[19] = '1';
								divisor = 0x10000; // to make a WORD a percentage of the maximum WORD
								Channels = *(WORD*)&myWaveBuf[22];
								SamplesPerSec = *(DWORD*)&myWaveBuf[24];
								BytesPerSec = *(DWORD*)&myWaveBuf[28];
								BytesPer100MilliSeconds = BytesPerSec / 10;
								BitsPerSample = *(WORD*)&myWaveBuf[34];
								BytesPerSample = BitsPerSample / 8;
								BytesPerSampleXChannels = BytesPerSample * Channels;
								WaveLength = *(DWORD*)&myWaveBuf[nextchunk+4];
								WaveStart = nextchunk+8;
								if ((WaveLength + WaveStart) == fileSize2) {
									for (y = 0; y < MAX_PATH; y++)
										WaveQuality[y] = 0;
									for (x = 0, y = 0; Filename[x] != 0; x++, y++)
										WaveQuality[y] = Filename[x];
									WaveQuality[y++] = ' ';
									WaveQuality[y++] = '(';
									if (BytesPerSample == 1)
										WaveQuality[y++] = '8';
									else if (BytesPerSample == 2) {
										WaveQuality[y++] = '1';
										WaveQuality[y++] = '6';
									}
									else if (BytesPerSample == 3) {
										WaveQuality[y++] = '2';
										WaveQuality[y++] = '4';
									}
									WaveQuality[y++] = ' ';
									WaveQuality[y++] = 'b';
									WaveQuality[y++] = 'i';
									WaveQuality[y++] = 't';
									WaveQuality[y++] = ' ';
									if (Channels == 1) {
										WaveQuality[y++] = 'm';
										WaveQuality[y++] = 'o';
										WaveQuality[y++] = 'n';
										WaveQuality[y++] = 'o';
									}
									else if (Channels == 2) {
										WaveQuality[y++] = 's';
										WaveQuality[y++] = 't';
										WaveQuality[y++] = 'e';
										WaveQuality[y++] = 'r';
										WaveQuality[y++] = 'e';
										WaveQuality[y++] = 'o';
									}
									WaveQuality[y++] = ')';
								}
								else {
									ofn2.lpstrTitle = NULL;
									VirtualFree(myWaveBuf, 0, MEM_RELEASE);
									myWaveBuf = NULL;
									MessageBox(hwndEditMyWave, "Data size is wrong", ERROR, MB_OK);
									break;
								}
							}
							else {
								ofn2.lpstrTitle = NULL;
								VirtualFree(myWaveBuf, 0, MEM_RELEASE);
								myWaveBuf = NULL;
								MessageBox(hwndEditMyWave, "Can't play Wave file\n", "", MB_OK);
							}
						}
						else {
							ofn2.lpstrTitle = NULL;
							VirtualFree(myWaveBuf, 0, MEM_RELEASE);
							myWaveBuf = NULL;
							MessageBox(hwndEditMyWave, "That's not a Wave file.", "Oops", MB_OK);
						}
					}
					else {
						ofn2.lpstrTitle = NULL;
						CloseHandle(hFile2);
						MessageBox(hwndEditMyWave, "That file is empty", ERROR, MB_OK);
					}
				}
				else {
					ofn2.lpstrTitle = NULL;
					MessageBox(hwndEditMyWave, FullFilename2, "Unable to open", MB_OK);
				}
			}
			break;

		case ID_FILES_SAVEAS: // from Advanced -Wave -View Wave -Save As
			ofn2.lpstrFile[0] = 0; // to not suggest a file name
			if (GetSaveFileName(&ofn2)) {
				hFile = CreateFile(FullFilename2, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					WriteFile(hFile, myWaveBuf, fileSize2, &dwBytesWritten, NULL);
					CloseHandle(hFile);
				}
			}
			break;

		case ID_FILES_EXIT:
			DestroyWindow(hwndEditMyWave);
			break;

		case ID_PLAY:
			if ((myWaveBuf) && (!playingwave)) {
				fromeditmywave = TRUE;
				playingwave = TRUE;

				WaveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
				WaveFormat.Format.nChannels = Channels;
				nSamplesPerSec = *(DWORD*)&myWaveBuf[24];
				WaveFormat.Format.nSamplesPerSec = SamplesPerSec;
				WaveFormat.Format.nAvgBytesPerSec = BytesPerSec;
				WaveFormat.Format.nBlockAlign = *(WORD*)&myWaveBuf[32];
				WaveFormat.Format.wBitsPerSample = BitsPerSample;
				WaveFormat.Format.cbSize = 22;
				WaveFormat.Samples.wValidBitsPerSample = WaveFormat.Format.wBitsPerSample;
				WaveFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
				// the following should be WaveFormat.Subformat = KSDATAFORMAT_SUBTYPE_PCM defined as 00000001-0000-0010-8000-00aa00389b71 (but that doesn't work here)
				WaveFormat.SubFormat.Data1 = 1;
				WaveFormat.SubFormat.Data2 = 0;
				WaveFormat.SubFormat.Data3 = 0x10;
				WaveFormat.SubFormat.Data4[0] = 0x80;
				WaveFormat.SubFormat.Data4[1] = 0;
				WaveFormat.SubFormat.Data4[2] = 0;
				WaveFormat.SubFormat.Data4[3] = 0xAA;
				WaveFormat.SubFormat.Data4[4] = 0;
				WaveFormat.SubFormat.Data4[5] = 0x38;
				WaveFormat.SubFormat.Data4[6] = 0x9B;
				WaveFormat.SubFormat.Data4[7] = 0x71;
				waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat.Format, (DWORD)&waveOutProc4, 0, CALLBACK_FUNCTION);

				WaveOutHdr.lpData = &myWaveBuf[WavePtr+PTR];
				WaveOutHdr.dwBufferLength = fileSize2 - (WavePtr+PTR);
				WaveOutHdr.dwBytesRecorded = 0;
				WaveOutHdr.dwUser = 0;
				WaveOutHdr.dwFlags = 0;
				WaveOutHdr.dwLoops = 0;

				waveOutPrepareHeader(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
				waveOutWrite(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
				timeBeginPeriod(TIMER_RESOLUTION);
				time1 = timeGetTime();

				timeBeginPeriod(TIMER_RESOLUTION);
				uTimer14ID = timeSetEvent(100, TIMER_RESOLUTION, waveTimerFunc, 0, TIME_PERIODIC);
			}
			break;

		case MY_HELP:
			MessageBox(hwndEditMyWave, "Esc, Left Arrow, Right Arrow, Home, PgUp, PgDn, End\nwork as they should.\n\n+ or - changes the vertical scale.\n\nThe number to the right of the cursor is the file location.\nThe number above/below the cursor is the wave point number.\n\nTo play a wave file from the current screen, press the Space bar.\nTo stop playing, press the Space bar again.\n\nTo cut out a section, left-click at the beginning,\nand then left-click again at the end. Be careful.", "View Wave", MB_OK);
			break;
		}
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case 187:// '+'
			if (ratio < 128) {
				divisor >>= 1;
				ratio <<= 1;
				ShowRatios(ratio);
				WaveHeight[17] = Left[0];
				WaveHeight[19] = Right[0];
				InvalidateRect(hwndEditMyWave, &rect, FALSE);
			}
			break;
		case 189:// '-'
			if (ratio > 2) {
				divisor <<= 1;
				ratio >>= 1;
				ShowRatios(ratio);
				WaveHeight[17] = Left[0];
				WaveHeight[19] = Right[0];
				InvalidateRect(hwndEditMyWave, &rect, FALSE);
			}
			break;
		case VK_ESCAPE:
			if (!playingwave)
				DestroyWindow(hwndEditMyWave);
			else {
				playingwave = FALSE;
				waveOutReset(hWaveOut);
				timeKillEvent(uTimer14ID);
				uTimer14ID = 0;
				timeEndPeriod(TIMER_RESOLUTION);
			}
			break;
		case VK_SPACE:
			if (!playingwave)
				SendMessage(hwndEditMyWave, WM_COMMAND, ID_PLAY, 0);
			else {
				playingwave = FALSE;
				waveOutReset(hWaveOut);
				timeKillEvent(uTimer14ID);
				uTimer14ID = 0;
				timeEndPeriod(TIMER_RESOLUTION);
			}
			break;
		case VK_LEFT:
			if (PTR >= BytesPerSampleXChannels)
				PTR -= BytesPerSampleXChannels;
			InvalidateRect(hwndEditMyWave, &rect, FALSE);
			break;
		case VK_RIGHT:
			if (PTR < (WaveLength - (rect.right * BytesPerSampleXChannels)))
				PTR += BytesPerSampleXChannels;
			InvalidateRect(hwndEditMyWave, &rect, FALSE);
			break;
		case VK_NEXT:
			if (PTR < (WaveLength - (rect.right * BytesPerSampleXChannels)))
				PTR += rect.right * BytesPerSampleXChannels;
			InvalidateRect(hwndEditMyWave, &rect, FALSE);
			break;
		case VK_PRIOR:
			if (PTR >= rect.right * BytesPerSampleXChannels)
				PTR -= rect.right * BytesPerSampleXChannels;
			InvalidateRect(hwndEditMyWave, &rect, FALSE);
			break;
		case VK_HOME:
			PTR = 0;
			InvalidateRect(hwndEditMyWave, &rect, FALSE);
			break;
		case VK_END:
			PTR = WaveLength - (rect.right * BytesPerSampleXChannels);
			InvalidateRect(hwndEditMyWave, &rect, FALSE);
			break;
		}
		break;

	case WM_MOUSEMOVE:
		if (myWaveBuf) {
			xLoc = LOWORD(lParam);
			yLoc = HIWORD(lParam);
			InvalidateRect(hwndEditMyWave, &rect, FALSE);
			UpdateWindow(hwndEditMyWave);
		}
		break;

	case WM_LBUTTONDOWN:
		if (myWaveBuf) {
			if (beginning == FALSE) {
				Beginning = WaveStart + PTR + (xLoc * BytesPerSampleXChannels);
				if ((Beginning-WaveStart) % (BytesPerSampleXChannels))
					Beginning += BytesPerSample; // to start with starting channel
			}
			else if (Ending == 0x7FFFFFFF) {
				Ending = WaveStart + PTR + (xLoc * BytesPerSampleXChannels);
				if ((Ending-WaveStart) % (BytesPerSampleXChannels))
					Ending += BytesPerSample;
			}
			if (Ending > Beginning) {
				if (DialogBox(hInst, "CUTSECTION", hwndEditMyWave, CutSectionProc)) {
					SetFocus(hwndEditMyWave);
					if (beginning == FALSE)
						beginning = TRUE;
					else {
						beginning = FALSE;
						if (Ending > WaveLength)
							Ending = WaveLength;
						for (x = Beginning, y = Ending; y < WaveLength; x++, y++)
							*(BYTE*)&myWaveBuf[x] = *(BYTE*)&myWaveBuf[y];
						fileSize2 = x;
						*(DWORD*)&myWaveBuf[nextchunk+4] = x - WaveStart;
						*(DWORD*)&myWaveBuf[4] = x - 8;
					}
				}
				Ending = 0x7FFFFFFF;
			}
			else {
				beginning = FALSE;
				Ending = 0x7FFFFFFF;
			}
		}
		break;

	case WM_PAINT:
		hdc2 = BeginPaint(hwndEditMyWave, &ps2);
		if (first) {
			first = FALSE;
			hdcMem2 = CreateCompatibleDC(hdc2);
			hBitmap2 = CreateCompatibleBitmap(hdc2, rect.right, rect.bottom);
			SelectObject(hdcMem2, hBitmap2);
			SetFocus(hwndEditMyWave);
		}
		if (fromuser14) {
			fromuser14 = FALSE;
			if (PTR < (WaveLength - (int)BytesPer100MilliSeconds))
				PTR += BytesPer100MilliSeconds;
			else {
				PTR = WaveLength - (rect.right * BytesPerSampleXChannels);
				timeKillEvent(uTimer14ID);
				uTimer14ID = 0;
				timeEndPeriod(TIMER_RESOLUTION);
			}
		}
		FillRect(hdcMem2, &rect, hBrush);
		SetBkMode(hdcMem2, TRANSPARENT);
		TextOut(hdcMem2, 0, 0, WaveQuality, strlen(WaveQuality)); // file name
		SetBkMode(hdcMem2, OPAQUE);
		SelectObject(hdcMem2, hPen);
		MoveToEx(hdcMem2, 0, middle, NULL);
		LineTo(hdcMem2, rect.right, middle);
		if (PTR <= WaveLength - (rect.right * BytesPerSampleXChannels)) {
			if (myWaveBuf) {
				oddeven = 1;
				if (BytesPerSample == 1)
					MessageBox(hwndEditMyWave, "This program doesn't deal with 8 bit wave files.", "Low Fidelity!", MB_OK);
				else if (BytesPerSample == 2) {
					if (Channels == 1) {
						SetBkMode(hdcMem2, TRANSPARENT);
						x = WaveStart + PTR + (xLoc * BytesPerSample);
						_itoa(x, WavePos, 10);
						TextOut(hdcMem2, xLoc+10, yLoc, WavePos, strlen(WavePos));
						_itoa(*(SHORT*)&myWaveBuf[x], Loudness, 10);
						TextOut(hdcMem2, xLoc, yLoc+20, Loudness, strlen(Loudness));
						TextOut(hdcMem2, 0, middle - 20, WaveHeight, 20);
						SetBkMode(hdcMem2, OPAQUE);

						SelectObject(hdcMem2, hPen);
						MoveToEx(hdcMem2, 0, middle, NULL);
						for (x = 0, WavePtr = WaveStart; x < rect.right; x++, WavePtr += BytesPerSample) {
							z = middle - (*(SHORT*)&myWaveBuf[WavePtr+PTR] * rect.bottom / divisor);
							LineTo(hdcMem2, x, z);
						}
					}
					else { // stereo
						SetBkMode(hdcMem2, TRANSPARENT);
						x = WaveStart + PTR + (xLoc * BytesPerSampleXChannels);
						_itoa(x, WavePos, 10);
						TextOut(hdcMem2, xLoc+10, yLoc, WavePos, strlen(WavePos));
						_itoa(*(SHORT*)&myWaveBuf[x], Loudness, 10);
						SetTextColor(hdcMem2, 0xE0); // red
						TextOut(hdcMem2, xLoc, yLoc+20, Loudness, strlen(Loudness));
						_itoa(*(SHORT*)&myWaveBuf[x+BytesPerSample], Loudness, 10);
						SetTextColor(hdcMem2, 0xE000); // green
						TextOut(hdcMem2, xLoc, yLoc-20, Loudness, strlen(Loudness));
						TextOut(hdcMem2, 0, middle - 20, WaveHeight, 20);
						SetBkMode(hdcMem2, OPAQUE);
						SetTextColor(hdcMem2, 0);

						SelectObject(hdcMem2, hRedPen); // first entry in myWaveBuf is red
						MoveToEx(hdcMem2, 0, middle, NULL);
						for (x = 0, WavePtr = WaveStart; x < rect.right; x++, WavePtr += BytesPerSampleXChannels) {
							z = middle - (*(SHORT*)&myWaveBuf[WavePtr+PTR] * rect.bottom / divisor);
							LineTo(hdcMem2, x, z);
						}
						SelectObject(hdcMem2, hGreenPen); // second is green
						MoveToEx(hdcMem2, 0, middle, NULL);
						for (x = 0, WavePtr = WaveStart+BytesPerSample; x < rect.right; x++, WavePtr += BytesPerSampleXChannels) {
							z = middle - (*(SHORT*)&myWaveBuf[WavePtr+PTR] * rect.bottom / divisor);
							LineTo(hdcMem2, x, z);
						}
					}
				}
				else if (BytesPerSample == 3) {
					if (Channels == 1) {
						SetBkMode(hdcMem2, TRANSPARENT);
						x = WaveStart + PTR + (xLoc * BytesPerSample);
						_itoa(x, WavePos, 10);
						TextOut(hdcMem2, xLoc+10, yLoc, WavePos, strlen(WavePos));
						z = (*(int*)&myWaveBuf[x] << 8); // left-shift to keep sign
						_itoa(z >> 8, Loudness, 10); // right-shift to keep sign and make z the original 3 bytes
						TextOut(hdcMem2, xLoc, yLoc+20, Loudness, strlen(Loudness));
						TextOut(hdcMem2, 0, middle - 20, WaveHeight, 20);
						SetBkMode(hdcMem2, OPAQUE);

						if (PTR < (WaveLength - rectrightx3)) {
							SelectObject(hdcMem2, hPen);
							MoveToEx(hdcMem2, 0, middle, NULL);
							for (x = 0, WavePtr = WaveStart; x < rect.right; x++, WavePtr += BytesPerSample) {
								s = *(int*)&myWaveBuf[WavePtr+PTR] >> 8;
								y = middle - (s * rect.bottom / divisor);
								LineTo(hdcMem2, x, y);
							}
						}
					}
					else { // stereo
						SetBkMode(hdcMem2, TRANSPARENT);
						x = WaveStart + PTR + (xLoc * BytesPerSampleXChannels);
						_itoa(x, WavePos, 10);
						TextOut(hdcMem2, xLoc+10, yLoc, WavePos, strlen(WavePos));

						SetTextColor(hdcMem2, RED); // red
						z = (*(int*)&myWaveBuf[x] << 8); // left-shift to keep sign
						_itoa(z >> 8, Loudness, 10); // right-shift to keep sign and make z the original 3 bytes
						TextOut(hdcMem2, xLoc, yLoc+20, Loudness, strlen(Loudness));

						SetTextColor(hdcMem2, GREEN); // green
						z = (*(int*)&myWaveBuf[x+BytesPerSample] << 8); // left-shift to keep sign
						_itoa(z >> 8, Loudness, 10); // right-shift to keep sign and make z the original 3 bytes
						TextOut(hdcMem2, xLoc, yLoc-20, Loudness, strlen(Loudness));

						TextOut(hdcMem2, 0, middle - 20, WaveHeight, 20);
						SetBkMode(hdcMem2, OPAQUE);
						SetTextColor(hdcMem2, 0);

						SelectObject(hdcMem2, hRedPen); // first entry in myWaveBuf is red
						MoveToEx(hdcMem2, 0, middle, NULL);
						for (x = 0, WavePtr = WaveStart; x < rect.right; x++, WavePtr += BytesPerSampleXChannels) {
							s = *(int*)&myWaveBuf[WavePtr+PTR] >> 8;
							y = middle - (s * rect.bottom / divisor);
							LineTo(hdcMem2, x, y);
						}
						SelectObject(hdcMem2, hGreenPen); // second is green
						MoveToEx(hdcMem2, 0, middle, NULL);
						for (x = 0, WavePtr = WaveStart+BytesPerSample; x < rect.right; x++, WavePtr += BytesPerSampleXChannels) {
							s = *(int*)&myWaveBuf[WavePtr+PTR] >> 8;
							y = middle - (s * rect.bottom / divisor);
							LineTo(hdcMem2, x, y);
						}
					}
				}
			}
		}
		BitBlt(hdc2, 0, 0, rect.right, rect.bottom, hdcMem2, 0, 0, SRCCOPY);
		EndPaint(hwndEditMyWave, &ps2);
		return 0;

	case WM_DESTROY:
		if (uTimer14ID) {
			timeKillEvent(uTimer14ID);
			uTimer14ID = 0;
			timeEndPeriod(TIMER_RESOLUTION);
		}
		playingwave = FALSE;
		fromuser14 = FALSE;
		fromeditmywave = FALSE;
		if (myWaveBuf)
			VirtualFree(myWaveBuf, 0, MEM_RELEASE);
		myWaveBuf = NULL;
		DestroyMenu(hMenu2);
		hMenu = LoadMenu(hInst, "MENU");
		SetMenu(hwnd, hMenu);
		Menus();
		DrawMenuBar(hwnd);
		break;
	}
	return DefWindowProc(hwndEditMyWave, message, wParam, lParam);
}

LRESULT CALLBACK LoopProc(HWND hwndLoop, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y, z, x16, y16;
	static int LoopRow, LoopCol, xHilite = 0, yHilite = 0, xHiliteOld, yHiliteOld;
	static DWORD QuarterNoteBeats[] = {ID_QUARTERNOTES_1,ID_QUARTERNOTES_2,ID_QUARTERNOTES_3,ID_QUARTERNOTES_4,ID_QUARTERNOTES_5,ID_QUARTERNOTES_6,ID_QUARTERNOTES_7,ID_QUARTERNOTES_8,ID_QUARTERNOTES_9,ID_QUARTERNOTES_10,ID_QUARTERNOTES_11,ID_QUARTERNOTES_12,ID_QUARTERNOTES_13};
	BYTE loopNote;
	static BOOL first2 = TRUE;
	static HDC hdc2;
	static PAINTSTRUCT ps2;

	switch (message)
	{
	case WM_CREATE:
		GetClientRect(hwndLoop, &loopRect);
		hiliteRect.left = 0;
		hiliteRect.right = loopRect.right;
		MaxLoopCols = (loopRect.right - 136) / (8 * 16);
		if (LoopCols == 0)
			LoopCols = MaxLoopCols;
		RightSide = (LoopCols * 8 * 16) + 136;
		DrawMenuBar(hwndLoop);
		break;

	case WM_COMMAND:
		if (wParam == OPEN2)
			ConvertFromEvent();
		else if (wParam == SAVE2)
			ConvertoEvent();
		else if (wParam == EXIT2)
			DestroyWindow(hwndLoop);
		else if (wParam == ID_KEYBOARD) {
			if (!keyboardpercus) {
				GetWindowRect(hwnd, &Rect2);
				hMenu5 = LoadMenu(hInst, "MENU5");
				hwndKeyboardPercus = CreateWindow(Keyboardpercus, Keyboardpercus,// see KeyboardPercusProc
					WS_POPUPWINDOW | WS_CAPTION | WS_VISIBLE,
					0, Rect2.bottom+Menu-300, Rect2.right - (Rect2.right % 14), 300,
					hwndLoop, hMenu5, hInst, NULL);
				SetMenu(hwndKeyboardPercus, hMenu5);
			}
		}		
		else if (wParam == ID_NEW) {
			if (IDYES == MessageBox(hwndLoop, "Remove Existing Notes?", "", MB_YESNO)) {
				for (y = 0; y < 47; y++)
					for (x = 0; x < 120; x++)
						LoopNotes[y][x] = 0;
				clearscreen = TRUE;
				InvalidateRect(hwndLoop, & loopRect, FALSE);
			}
		}
		else if (wParam == PLAY2) {
			if (playing2 == FALSE)
				DrumBeginSequence();
			else
				DrumEndSequence();
		}
		else if ((wParam == ID_USEASMETRONOME) && (!playing2)) {
			metronome = TRUE;
			CurrentNotes = -1;
			timeBeginPeriod(TIMER_RESOLUTION);
			milliSecondsPerBeat = (int)(dMilliSecondsPerTick * (double)TicksPerQuarterNote);
			time = milliSecondsPerBeat / 8;// 8 thirtysecond notes in a quarter note
			timeBeginPeriod(TIMER_RESOLUTION);
			uTimer0ID = timeSetEvent(time, TIMER_RESOLUTION, DrumTimerFunc, 0, TIME_PERIODIC);
			DestroyWindow(hwndLoop);
		}
		else if ((wParam == ID_MERGEWITHEXISTINGPERCUSSION) && (!playing2)) {
			e = saveE;
			for (x = 0; x < (int)e; x++)
				Event[x] = copyEvent[x];
			InsertPercussion();
			qsort(Event, e, sizeof(Event[0]), Compare);
			Resort();
			DeleteDuplicateNotes();
			saveE = e;
			for (x = 0; x < (int)e; x++)
				copyEvent[x] = Event[x];
			DestroyWindow(hwndLoop);
		}
		else if ((wParam == ID_REPLACEEXISTINGPERCUSSION) && (!playing2)) {
			e = saveE;
			for (x = 0; x < (int)e; x++)
				Event[x] = copyEvent[x];

			for (x = 0; x < (int)e; x++) {
				if ((Event[x].channel == 9) && (Event[x].note)) {
					for (y = x; y < (int)e; y++)
						Event[y] = Event[y+1];
					e--;
					x--;
				}
			}
			saveE = e;

			ptr = 0;
			for (z = 0; z < Page; z++)
				for ( ; (ptr < e) && ((Event[ptr].pixel) < (Lines[z*Rows].pixel + Lines[z*Rows].PixelsPerPage)); ptr++)
					;
			TopLeftPtr = ptr;
			FirstPixelOnPage = Event[TopLeftPtr].pixel;

			InsertPercussion();
			qsort(Event, e, sizeof(Event[0]), Compare);
			Resort();
			DeleteDuplicateNotes();
			saveE = e;
			for (x = 0; x < (int)e; x++)
				copyEvent[x] = Event[x];
			DestroyWindow(hwndLoop);
		}
		else if (wParam == HELP2) {
			Help = LoopHelp;
			DialogBox(hInst, "LOOPHELP", hwndLoop, Help2Proc);
		}

		else for (x = 0; x < 13; x++) {// 13 for 1920 screen width
			if (wParam == QuarterNoteBeats[x]) {
				DrumEndSequence();
				if ((x + 1) >= MaxLoopCols)
					LoopCols = MaxLoopCols;
				else
					LoopCols = x + 1;
				RightSide = (LoopCols * 8 * 16) + 136;
				clearscreen = TRUE;
				InvalidateRect(hwndLoop, &loopRect, FALSE);
				break;
			}
		}
		break;

	case WM_KEYDOWN:
		if (wParam == VK_SPACE)
			SendMessage(hwndLoop, WM_COMMAND, PLAY2, 0);
		if (wParam == VK_DELETE) {
			if ((xLoc > 136) && (xLoc < RightSide) && (yLoc < 752)) {//47*16
				LoopCol = (xLoc-136) / 16;
				LoopRow = yLoc / 16;
				hdc2 = GetDC(hwndLoop);
				if (0 == GetPixel(hdc2, xLoc, yLoc))// black square
					LoopNotes[LoopRow][LoopCol] = 0;
				ReleaseDC(hwndLoop, hdc2);
				InvalidateRect(hwndLoop, & loopRect, FALSE);
			}
		}
		if (wParam == VK_ESCAPE) {
			if (playing2)
				DrumEndSequence();
			else
				DestroyWindow(hwndLoop);
		}
		break;

	case WM_LBUTTONDOWN:
		xLoc = LOWORD(lParam);
		yLoc = HIWORD(lParam);
		if ((xLoc > 136) && (xLoc < RightSide) && (yLoc < 752)) {//47*16
			LoopCol = (xLoc-136) / 16;
			LoopRow = yLoc / 16;
			hdc2 = GetDC(hwndLoop);
			if (0 == GetPixel(hdc2, xLoc, yLoc))// black square
				LoopNotes[LoopRow][LoopCol] = 0;
			else {
				LoopNotes[LoopRow][LoopCol] = Volume;

				loopNote = 81 - LoopRow;//81 is triangle
				midiOutShortMsg(hMidiOut, (0x99) | (LoopNotes[LoopRow][LoopCol] << 16) | (loopNote << 8));
				TempEvent.message = (0x99) | (loopNote << 8);// Note Off
				milliSecondsPerBeat = (int)(dMilliSecondsPerTick * (double)TicksPerQuarterNote);
				time = milliSecondsPerBeat / 8;// 8 thirtysecond notes in a quarter note
				StopTimer();
			}
			ReleaseDC(hwndLoop, hdc2);
			InvalidateRect(hwndLoop, & loopRect, FALSE);
		}
		break;

	case WM_RBUTTONDOWN:
		xLoc = LOWORD(lParam);
		yLoc = HIWORD(lParam);
		LoopCol = (xLoc-136) / 16;
		LoopRow = yLoc / 16;
		hdc2 = GetDC(hwndLoop);
		if (0 == GetPixel(hdc2, xLoc, yLoc))// black square
			if (DialogBox(hInst, "NOTEVOLUME", hwndLoop, NoteVolumeProc))
				LoopNotes[LoopRow][LoopCol] = LoopNoteVol;
		ReleaseDC(hwndLoop, hdc2);
		break;

	case WM_MOUSEMOVE:
		xLoc = LOWORD(lParam);
		yLoc = HIWORD(lParam);
		xHiliteOld = xHilite;
		yHiliteOld = yHilite;
		xHilite = ((xLoc) / 16) * 16;
		yHilite = ((yLoc) / 16) * 16;
		if ((yHiliteOld != yHilite) && (yHilite < 752))//47*16
			InvalidateRect(hwndLoop, &loopRect, FALSE);
		break;

	case WM_PAINT:
		hdc2 = BeginPaint(hwndLoop, &ps2);
		hiliteRect.top = yHiliteOld;
		hiliteRect.bottom = yHiliteOld + 17;
		if (clearscreen) {
			clearscreen = FALSE;
			FillRect(hdc2, &loopRect, hBrush);
		}
		if (fromuser) {
			fromuser = FALSE;
			hOldBrush = SelectObject(hdc2, hBrush);
			Rectangle(hdc2, 136+(PreviousNotes*16), 0, 136+(PreviousNotes*16)+17, 752);
			SelectObject(hdc2, hGreyBrush);
			Rectangle(hdc2, 136+(CurrentNotes*16), 0, 136+(CurrentNotes*16)+17, 752);
			SelectObject(hdc2, hOldBrush);
		}
		FillRect(hdc2, &hiliteRect, hBrush);
 		hOldBrush = SelectObject(hdc2, hGreyBrush);
		Rectangle(hdc2, 0, yHilite, RightSide+1, yHilite+17);
		SelectObject(hdc2, hOldBrush);
		for (x = 136; x <= RightSide; x += 16) {
			MoveToEx(hdc2, x, 0, NULL);
			LineTo(hdc2, x, 752);//47*16
			if ((x-136) % 128 == 0) {
				MoveToEx(hdc2, x+1, 0, NULL);
				LineTo(hdc2, x+1, 752);//47*16
			}
		}
		for (x = 54, y = 0; x > 7; x--, y += 16) {
			z = strlen(Percussion[x]);
			SetBkMode(hdc2, TRANSPARENT);
			TextOut(hdc2, 15, y, Percussion[x], z);
			SetBkMode(hdc2, OPAQUE);
			MoveToEx(hdc2, 0, y, NULL);
			LineTo(hdc2, RightSide, y);
		}

		y = 0;
		SetTextColor(hdc2, 0xC0C0C0);
		SetBkMode(hdc2, TRANSPARENT);
		for (x = 12; x >= 0; x--, y += 16) {
			TextOut(hdc2, 2, y, &Keyboard1[x], 1);
		}
		for (x = 12; x >= 0; x--, y += 16) {
			TextOut(hdc2, 2, y, &Keyboard2[x], 1);
		}
		for (x = 10; x >= 0; x--, y += 16) {
			TextOut(hdc2, 2, y, &Keyboard3[x], 1);
		}
		for (x = 9; x >= 0; x--, y += 16) {
			TextOut(hdc2, 2, y, &Keyboard4[x], 1);
		}
		SetBkMode(hdc2, OPAQUE);
		SetTextColor(hdc2, 0);

		MoveToEx(hdc2, 0, y, NULL);
		LineTo(hdc2, RightSide, y);
		for (y = 0; y < 47; y++) {
			for (x = 0; x < (LoopCols * 8); x++) {
				if (LoopNotes[y][x]) {
					x16 = (x * 16) + 136;
					y16 = y * 16;
		 			hOldBrush = SelectObject(hdc2, hBlackBrush);
					Rectangle(hdc2, x16, y16, x16+16, y16+16);
					SelectObject(hdc2, hOldBrush);
				}
			}
		}
		EndPaint(hwndLoop, &ps2);
		break;

	case WM_DESTROY:
		ActiveChannel = saveActiveChannel;
		DestroyMenu(hMenu2);
		hMenu = LoadMenu(hInst, "MENU");
		SetMenu(hwnd, hMenu);
		Menus();
		DrawMenuBar(hwnd);
		break;
	}
	return DefWindowProc(hwndLoop, message, wParam, lParam);
}// end of LoopProc

int CALLBACK DynamicEffectsProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int Checks[6] = {IDC_CHECK1,IDC_CHECK2,IDC_CHECK3,IDC_CHECK4,IDC_CHECK5,IDC_CHECK6};
	static int *FlagPtr[6] = {&showvolume,&showexpression,&showreverb,&showchorus,&showmodulation,&showpitchbend};
	static int Radios[6] = {IDC_RADIO1,IDC_RADIO2,IDC_RADIO3,IDC_RADIO4,IDC_RADIO5,IDC_RADIO6};
	RECT thisRect;

	switch (message)
	{
	case WM_INITDIALOG:
		GetClientRect(hwndDlg, &thisRect);
		MoveWindow(hwndDlg, thisRect.left + 20, thisRect.top + 100, thisRect.right, thisRect.bottom+30, FALSE);
		showingdynamics = TRUE;
		if (Editing == -1)
			CheckDlgButton (hwndDlg, IDC_RADIO8, BST_CHECKED);// NOTHING
		for (x = 0; x < 6; x++) {
			if (*FlagPtr[x])
				CheckDlgButton (hwndDlg, Checks[x], BST_CHECKED);
			else
				CheckDlgButton (hwndDlg, Checks[x], BST_UNCHECKED);
			if (Editing == x)
				CheckDlgButton (hwndDlg, Radios[x], BST_CHECKED);
		}
		SetFocus(hwndDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:
			Help = DynamicEffectsHelp;
			DialogBox(hInst, "DYNAMICSHELP", hwnd, Help2Proc);
			SetFocus(hwndDlg);
			break;

		case IDC_CHECK1:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK1))
				showvolume = TRUE;
			else
				showvolume = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK2:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK2))
				showexpression = TRUE;
			else
				showexpression = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK3:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK3))
				showreverb = TRUE;
			else
				showreverb = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK4:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK4))
				showchorus = TRUE;
			else
				showchorus = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK5:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK5))
				showmodulation = TRUE;
			else
				showmodulation = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK6:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK6))
				showpitchbend = TRUE;
			else
				showpitchbend = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case IDC_CHECK7:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_CHECK7))
				showportamento = TRUE;
			else
				showportamento = FALSE;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDC_RADIO1:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO1);
			CheckDlgButton (hwndDlg, IDC_CHECK1, BST_CHECKED);
			showvolume = TRUE;
			Editing = 0;
			hEditPen = CreatePen(PS_SOLID, 0, colors[ActiveChannel]);
			break;
		case IDC_RADIO2:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO2);
			CheckDlgButton (hwndDlg, IDC_CHECK2, BST_CHECKED);
			showexpression = TRUE;
			Editing = 1;
			hEditPen = CreatePen(PS_SOLID, 0, colors[ActiveChannel]);
			break;
		case IDC_RADIO3:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO3);
			CheckDlgButton (hwndDlg, IDC_CHECK3, BST_CHECKED);
			showreverb = TRUE;
			Editing = 2;
			hEditPen = CreatePen(PS_SOLID, 0, colors[ActiveChannel]);
			break;
		case IDC_RADIO4:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO4);
			CheckDlgButton (hwndDlg, IDC_CHECK4, BST_CHECKED);
			showchorus = TRUE;
			Editing = 3;
			hEditPen = CreatePen(PS_SOLID, 0, colors[ActiveChannel]);
			break;
		case IDC_RADIO5:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO5);
			CheckDlgButton (hwndDlg, IDC_CHECK5, BST_CHECKED);
			showmodulation = TRUE;
			Editing = 4;
			hEditPen = CreatePen(PS_SOLID, 0, colors[ActiveChannel]);
			break;
		case IDC_RADIO6:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO6);
			CheckDlgButton (hwndDlg, IDC_CHECK6, BST_CHECKED);
			showpitchbend = TRUE;
			Editing = 5;
			hEditPen = CreatePen(PS_SOLID, 0, colors[ActiveChannel]);
			break;
		case IDC_RADIO7:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO7);
			CheckDlgButton (hwndDlg, IDC_CHECK7, BST_CHECKED);
			showportamento = TRUE;
			Editing = 6;
			hEditPen = CreatePen(PS_SOLID, 0, colors[ActiveChannel]);
			break;
		case IDC_RADIO8:
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO8);
			Editing = -1;
			hEditPen = CreatePen(PS_SOLID, 0, colors[ActiveChannel]);
			break;

		case IDC_RADIO9:
			CheckRadioButton(hwndDlg, IDC_RADIO9, IDC_RADIO10, IDC_RADIO9);
			for (x = 0; x < 6; x++) {
				CheckDlgButton (hwndDlg, Checks[x], BST_CHECKED);
				*FlagPtr[x] = TRUE;
			}
			if (Editing == -1) {
				CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO8);
			}
			dynamicexpression = TRUE;
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDC_RADIO10:
			CheckRadioButton(hwndDlg, IDC_RADIO9, IDC_RADIO10, IDC_RADIO10);
			CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO8, IDC_RADIO8);
			for (x = 0; x < 6; x++) {
				CheckDlgButton (hwndDlg, Checks[x], BST_UNCHECKED);
				*FlagPtr[x] = FALSE;
			}
			Editing = -1;
			FillRect(hdcMem, &rect, hBrush);
			InvalidateRect(hwnd, &rect, FALSE);
			break;


		case IDCANCEL:
		case IDOK:
			SendMessage(hwndDynamics, WM_CLOSE, 0, 0);
			break;
		}
		break;// out of case WM_COMMAND:

	case WM_CLOSE:
		DestroyWindow(hwndDynamics);
		hwndDynamics = NULL;
		showingdynamics = FALSE;
		Editing = -1;
		break;
	}
	return 0;
}

void StopTimers(void)
{
	if ((!fromsave) && (!fromP) && (playflag != PAUSED)) {
		PageBegin = 0;
		FirstPixelOnPage = 0;
 		TopLeftPtr = FirstTopLeftPtr;
		Page = 0;
		line = 0;
		_itoa(Page+1, &page[5], 10);
	}
	else if (fromP)
		fromP = FALSE;
	else if (fromsave)
		fromsave = FALSE;
	if (uTimer2ID) {
		timeKillEvent(uTimer2ID);
		timeEndPeriod(TIMER_RESOLUTION);
		uTimer2ID = 0;
	}
	if (uTimer4ID) {
		timeKillEvent(uTimer4ID);
		timeEndPeriod(TIMER_RESOLUTION);
		uTimer4ID = 0;
	}
	if (uTimer5ID) {
		timeKillEvent(uTimer5ID);
		timeEndPeriod(TIMER_RESOLUTION);
		uTimer5ID = 0;
	}
	if (uTimer6ID) {
		timeKillEvent(uTimer6ID);
		timeEndPeriod(TIMER_RESOLUTION);
		uTimer6ID = 0;
	}
	if (metronome) {
		metronome = FALSE;
		if (uTimer0ID) {
			timeKillEvent(uTimer0ID);
			timeEndPeriod(TIMER_RESOLUTION);
			midiOutShortMsg(hMidiOut, 0xB9 | (123 << 8));// All Channel 9 Notes Off
		}
		CurrentNotes = -1;
		PreviousNotes = 0;
	}
	if (!EWQL)
		midiOutReset(hMidiOut);// stops all notes
	else {
		for (x = 0; x < NumberOfPorts; x++)
			midiOutReset(hMidisOut[x]);
	}
}

void Resort(void)
{
	static DWORD w, x, y, z;

	for (x = 0; x < (int)(e-1); x++) {
		for (y = x+1; (y < (int)e) && (Event[x].tickptr == Event[y].tickptr); y++) {
			if ((Event[x].note == Event[y].note) && (Event[x].velocity) && (Event[y].velocity) && (Event[x].channel < Event[y].channel)) {
				TempEvent = Event[x];// sort notes at same location by channel
				Event[x] = Event[y];
				Event[y] = TempEvent;
			}
			if ((Event[x].note) && (Event[y].note) && (Event[x].velocity) && (Event[y].velocity == 0)) { 
				TempEvent = Event[x];// put notes off before notes on at same tickptr
				Event[x] = Event[y];
				Event[y] = TempEvent;
			}
			if ((Event[x].velocity) && (Event[y].note == 0)) {
				TempEvent = Event[x];// put non-notes before notes on at same tickptr
				Event[x] = Event[y];
				Event[y] = TempEvent;
			}
			if ((Event[x].note == 0) && (Event[y].note) && (Event[y].velocity == 0)) { 
				TempEvent = Event[x];// put notes off before non-notes at same tickptr
				Event[x] = Event[y];
				Event[y] = TempEvent;
			}
		}
	}
	for (x = 0; (x < e) && (Event[x].tickptr == 0); x++) // sort until tickptr != 0
		resortEvent[x] = Event[x];
	z = 0;
	for (y = 0; (y < x) && (z < x); y++) {// put non-message Events first
		if (resortEvent[y].message == 0)
			Event[z++] = resortEvent[y];
	}
	for (y = 0; (y < x) && (z < x); y++) {
		if ((resortEvent[y].message & 0xFFB0) == 0x79B0)// then Controller Off message
			Event[z++] = resortEvent[y];
	}
	for (w = 0; (w < 16) && (z < x); w++) {// then message Events in order by Channel
		for (y = 0; (y < x) && (z < x); y++) {
			if ((0 == resortEvent[y].velocity) && (resortEvent[y].message & 0xF0) && ((resortEvent[y].message & 0xFF00) != 0x7900) && ((resortEvent[y].message & 0xF) == w))
				Event[z++] = resortEvent[y];
		}
	}
}

void ZeroEvent(int x)
{
	Event[x].pixel = 0;
	Event[x].pixelsinnote = 0;
	Event[x].tickptr = 0;
	Event[x].ticksinnote = 0;
	Event[x].message = 0;
	Event[x].dMilliSecondsPerTick = 0;
	Event[x].note = 0;
	Event[x].velocity = 0;
	Event[x].sharporflat = 0;
	Event[x].channel = 17;//flag to ignore Event[x] (except for midiOut) (it's not in ActiveChannels)
	Event[x].port = 0;
	Event[x].time = 0;
	Event[x].BeatsPerMeasure = 0;
	Event[x].BeatNoteType = 0;
	Event[x].KeySignature = 0;
	Event[x].type = 0;
	Event[x].len = 0;
	Event[x].ptr = 0;
	Event[x].finger[0] = 0;
	Event[x].overlapped = 0;
}
void ZeroEvents(void)
{
	int x;

	for (x = 0; x < MAX_EVENT; x++)
		ZeroEvent(x);
	for (x = 0; x < 0xFFFF; x++)
		Data[x] = 0;
	dataptr = 0;
}

void DeleteDuplicateNotes(void)
{
	for (x = 0; x < (int)e; x++) {
		if (Event[x].velocity) {
			for (y = x+1; (y < (int)e); y++) {
				if ((Event[x].tickptr == Event[y].tickptr)
				 && (Event[x].note == Event[y].note)
				 && (Event[x].channel == Event[y].channel)
				 && (Event[x].port == Event[y].port)
				 && (Event[y].velocity)) {
					for (v = y+1; v < (int)e; v++) {
						if ((Event[v].note == Event[y].note) && (Event[v].velocity == 0)) {
							for (  ; v < (int)e; v++)
								Event[v] = Event[v+1];// remove note off
							e--;
						}
					}
					for (v = y; v < (int)e; v++)
						Event[v] = Event[v+1];// remove note on
					e--;
				}
			}
		}
	}
}

void SaveText(int x)
{
	if (BigText[0] == 0)
		BigText[0] = ' ';// just in case
	y = x + MetaEventLen;
	for ( ; (bt < 4093) && (x < y); x++, bt++) {
		BigText[bt] = Midi[x];
		if (BigText[bt] == 0)
			BigText[bt] = ' ';
	}
	if (BigText[bt-1] == 10)// \n
		bt--;
	if ((BigText[bt] != 10) || (BigText[bt-1] != 13)) {
		BigText[bt++] = 13;// \r
		BigText[bt++] = 10;// \n
	}
	Event[e].type = MetaEventType;
	Event[e].len = (WORD)MetaEventLen;
	Event[e].ptr = dataptr;
	for (x = dataptr + (int)MetaEventLen; dataptr < x; dataptr++)
		Data[dataptr] = Midi[i++];
}

void CheckOverlap(char LeftRight)
{ // thisX is the one moving
	overlap = FALSE;
	for (x = 0; x < (int)e; x++) {// don't overlap other same instrument same notes
		if ((x != thisX) && (Event[x].note == Event[thisX].note) && (Event[x].velocity) && (Event[thisX].channel == Event[x].channel) && (Event[thisX].port == Event[x].port)) {
			if ((Event[thisX].pixel > Event[x].pixel) && (LeftRight < 0)) { // moving left
//				if ((Event[x].pixel + Event[x].pixelsinnote) > (Event[thisX].pixel + LeftRight)) {
				if ((Event[x].pixel + Event[x].pixelsinnote) >= (Event[thisX].pixel)) {
					overlap = TRUE;
					break;
				}
			}
			else if ((Event[thisX].pixel < Event[x].pixel) && (LeftRight > 0)) { // moving right
				if ((Event[thisX].pixel + Event[thisX].pixelsinnote + LeftRight) > (Event[x].pixel)) {
					overlap = TRUE;
					break;
				}
			}
		}
	}
}

void MoveNoteLeftRight(char LeftRight) // LeftRight is either 5 or -5
{
	BOOL left1pixel, right1pixel;

	l = (Page * Rows) + Row;
	z = xLoc + Lines[l].pixel;// xLoc & Note from WM_MOUSEMOVE
	left1pixel = right1pixel = FALSE;
	if (GetKeyState(VK_CONTROL) < 0) { // to move 
		if (movingleft)
			left1pixel = TRUE;
		else
			right1pixel = TRUE;
	}
	if (ordinalNote == -1)
		ordinalNote = Note;
	if (OriginalNote == 0) {// flag
		OriginalNote = ordinalNote;
		tx = 0;
		ThisX[1] = 0xFF;
		for (thisX = 0; thisX < (int)e; thisX++) {
			if ((z >= Event[thisX].pixel) && (z <= (Event[thisX].pixel + Event[thisX].pixelsinnote))) {
				if ((Event[thisX].note == ordinalNote)
				|| ((!usingsharp) && ((!Letter[ordinalNote-1-21]) && (Event[thisX].note == ordinalNote-1)))
				|| ((usingsharp) && ((!Letter[ordinalNote+1-21]) && (Event[thisX].note == ordinalNote+1)))) {
					if (Event[thisX].channel == ActiveChannel)
						break;
					else if (tx < 2)
						ThisX[tx++] = thisX;
				}
			}
		}
		if (thisX == (int)e) {// no ACTIVE note there
			if (ThisX[1] != 0xFF) {// more than one instrument and neither is ACTIVE
				OriginalNote = 0;// flag
				MessageBox(hwnd, ToDelete, "To Move", MB_OK);
				return;
			}
			else if (tx)
				thisX = ThisX[0];// a single non-ACTIVE note
		}
	}
	if (Event[thisX].note == 0)
		return;
	if (ordinalNote == -1) // flag from VK_DELETE, MB_LBUTTONDOWN, or ChangeInstrument
		return;

	if (right1pixel) {
		if (z < (Event[thisX].pixel + (Event[thisX].pixelsinnote / 2))) {
			Event[thisX].pixel++;
			Event[thisX].pixelsinnote--;
		}
		else {
			CheckOverlap(LeftRight);
			if (!overlap) {
				Event[thisX].pixelsinnote++;
				for (x = thisX+1 ; x < (int)e; x++) {// find note end
					if ((Event[x].note == Event[thisX].note) && (Event[x].channel == Event[thisX].channel) && (Event[x].port == Event[thisX].port) && (Event[x].velocity == 0) && (Event[x].overlapped == Event[thisX].overlapped)) {
						Event[x].pixel++;
						Event[x].tickptr = Event[x].pixel * TicksPerQuarterNote / 40;
						break;
					}
				}
			}
		}
		Event[thisX].tickptr = Event[thisX].pixel * TicksPerQuarterNote / 40;
		FillRect(hdcMem, &rect, hBrush);
		InvalidateRect(hwnd, &rect, FALSE);
		return;
	}
	else if (left1pixel) {
		if (z > (Event[thisX].pixel + (Event[thisX].pixelsinnote / 2))) {
			Event[thisX].pixelsinnote--;
			for (x = thisX+1 ; x < (int)e; x++) {// find note end
				if ((Event[x].note == Event[thisX].note) && (Event[x].channel == Event[thisX].channel) && (Event[x].port == Event[thisX].port) && (Event[x].velocity == 0) && (Event[x].overlapped == Event[thisX].overlapped)) {
					Event[x].pixel--;
					Event[x].tickptr = Event[x].pixel * TicksPerQuarterNote / 40;
					break;
				}
			}
		}
		else if (Event[thisX].pixel) {
			CheckOverlap(LeftRight);
			if (!overlap) {
				Event[thisX].pixel--;
				Event[thisX].pixelsinnote++;
			}
		}
		Event[thisX].tickptr = Event[thisX].pixel * TicksPerQuarterNote / 40;
		FillRect(hdcMem, &rect, hBrush);
		InvalidateRect(hwnd, &rect, FALSE);
		return;
	}

	if ((movingleft) && (Event[thisX].pixel < (Lines[0].pixel + 5))) // don't move it to less than 0
		overlap = TRUE;
	else {
		CheckOverlap(LeftRight);
	}

	if (overlap == FALSE) {
		Event[thisX].pixel += LeftRight;
		Event[thisX].tickptr = Event[thisX].pixel * TicksPerQuarterNote / 40;
		for (x = thisX+1 ; x < (int)e; x++) {// find note end
			if ((Event[x].note == Event[thisX].note) && (Event[x].channel == Event[thisX].channel) && (Event[x].port == Event[thisX].port) && (Event[x].velocity == 0) && (Event[x].overlapped == Event[thisX].overlapped)) {
				Event[x].pixel += LeftRight;
				Event[x].tickptr = Event[x].pixel * TicksPerQuarterNote / 40;
		//move Event[thisX] and Event[x] after their .pixels have been changed
				if (movingleft) {
					for (y = thisX-1; (y >= 0) && (Event[y].tickptr > Event[thisX].tickptr); y--)
						;
					if (y < (thisX-1)) {// if y is below thisX
						y++;
						TempEvent = Event[thisX];
						for (z = thisX; z > (DWORD)y; z--)
							Event[z] = Event[z-1];
						Event[y] = TempEvent;
						thisX = y;
					}

					for (y = x-1; (y >= 0) && (Event[y].tickptr > Event[x].tickptr); y--)
						;
					if (y < (x-1)) {// if y is below x
						y++;
						TempEvent = Event[x];
						for (z = x; z > (DWORD)y; z--)
							Event[z] = Event[z-1];
						Event[y] = TempEvent;
					}
				}
				else {// if moving right
					for (y = x+1; (y < (int)e) && (Event[y].tickptr < Event[x].tickptr); y++)
						;
					if (y > (x+1)) {// if y is past x
						y--;
						TempEvent = Event[x];
						for (z = x; z < (DWORD)y; z++)
							Event[z] = Event[z+1];
						Event[y] = TempEvent;
					}

					for (y = thisX+1; (y < (int)e) && (Event[y].tickptr < Event[thisX].tickptr); y++)
						;
					if (y > (thisX+1)) {// if y is past thisX
						y--;
						TempEvent = Event[thisX];
						for (z = thisX; z < (DWORD)y; z++)
							Event[z] = Event[z+1];
						Event[y] = TempEvent;
						thisX = y;
					}
				}
				break;
			}
		}
		SaveEvents();
		FillRect(hdcMem, &rect, hBrush);
		InvalidateRect(hwnd, &rect, FALSE);
	}
}

void MoveNoteUpDown(char UpDown)
{
//	if (frominstrumentproc) {
//		frominstrumentproc = FALSE;
//		FillRect(hdcMem, &rect, hBrush);
//		InvalidateRect(hwnd, &rect, FALSE);
//		return;
//	}
	l = (Page * Rows) + Row;
	z = xLoc + Lines[l].pixel;
	if (ordinalNote == -1)
		ordinalNote = Note;
	if (OriginalNote == 0) {// flag from WM_MOUSEMOVE
		OriginalNote = ordinalNote;
		tx = 0;
		ThisX[1] = 0xFF;
		for (thisX = 0; thisX < (int)e; thisX++) {
			if ((z >= Event[thisX].pixel) && (z <= (Event[thisX].pixel + Event[thisX].pixelsinnote))) {
				if ((Event[thisX].note == ordinalNote)
				|| ((!usingsharp) && ((!Letter[ordinalNote-1-21]) && (Event[thisX].note == ordinalNote-1)))
				|| ((usingsharp) && ((!Letter[ordinalNote+1-21]) && (Event[thisX].note == ordinalNote+1)))) {
					if ((Event[thisX].channel == ActiveChannel) && (Event[thisX].port == Port))
						break; // only occurs at first move
					else if (tx < 2)
						ThisX[tx++] = thisX;
				}
			}
		}
		if (thisX == (int)e) {// no note there
			if (ThisX[1] != 0xFF) {// more than one instrument and neither is ACTIVE
				OriginalNote = 0;// flag
				MessageBox(hwnd, ToDelete, "To Move", MB_OK);
				return;
			}
			else
				thisX = ThisX[0];// a single non-ACTIVE note
		}
	}
	if (Event[thisX].note == 0)
		return;
	if (ordinalNote == -1) // flag from VK_DELETE, MB_LBUTTONDOWN, or ChangeInstrument
		return;
	overlap = FALSE;
	for (x = 0; x < (int)e; x++) {// don't overlap other same instrument same notes
		if ((thisX != x) && (Event[x].velocity) && ((Event[thisX].note + UpDown) == Event[x].note) && (Event[thisX].channel == Event[x].channel) && (Event[thisX].port == Event[x].port) && (Event[x].overlapped == Event[thisX].overlapped))
		{
			if ((Event[thisX].pixel <= Event[x].pixel) && ((Event[thisX].pixel + Event[thisX].pixelsinnote) > Event[x].pixel)) {
				overlap = TRUE;
				break;
			}
			else if (((Event[thisX].pixel + Event[thisX].pixelsinnote) > (Event[x].pixel + Event[x].pixelsinnote))
			 && (Event[thisX].pixel < (Event[x].pixel + Event[x].pixelsinnote))) {
				overlap = TRUE;
				break;
			}
			else if ((Event[thisX].pixel > Event[x].pixel)
			 && ((Event[thisX].pixel + Event[thisX].pixelsinnote) < (Event[x].pixel + Event[x].pixelsinnote))) {
				overlap = TRUE;
				break;
			}
		}
	}
	if (overlap == FALSE) {
		for (x = thisX+1; x < (int)e; x++) {// find note end
			if ((Event[x].pixel == (Event[thisX].pixel + Event[thisX].pixelsinnote)) && (Event[x].note == Event[thisX].note) && (Event[x].channel == Event[thisX].channel) && (Event[thisX].port == Event[x].port) && (Event[x].velocity == 0) && (Event[x].overlapped == Event[thisX].overlapped))
				break;
		}
		if (x == (int)e)
			MessageBox(hwnd, "Note End not found", ERROR, MB_OK);
ordinalNote += UpDown;
		Event[thisX].note += UpDown;
		Note = Event[thisX].note;
		Event[thisX].message += (UpDown << 8);
		Event[x].note += UpDown;
		Event[x].message += (UpDown << 8);
		if (!Letter[Event[thisX].note - 21]) {
			Event[thisX].sharporflat = 1;
			Event[x].sharporflat = 1;
		}
		else {
			Event[thisX].sharporflat = 0;
			Event[x].sharporflat = 0;
		}
		if (!EWQL)
			midiOutShortMsg(hMidiOut, Event[thisX].message);
		else
			midiOutShortMsg(hMidisOut[Event[thisX].port], Event[thisX].message);
		TempEvent.message = (0x90|ActiveChannel) | (Note << 8);// Note Off
		if (uTimer3ID) {
			timeBeginPeriod(TIMER_RESOLUTION);
			uTimer3ID = timeSetEvent(0, TIMER_RESOLUTION, TimerFunc3, TempEvent.message, TIME_ONESHOT);// end the note now
		}
		if (uTimer7ID) {
			timeBeginPeriod(TIMER_RESOLUTION);
			uTimer7ID = timeSetEvent(0, TIMER_RESOLUTION, TimerFunc7, TempEvent.message, TIME_ONESHOT);// end the note now
		}
		if (uTimer8ID) {
			timeBeginPeriod(TIMER_RESOLUTION);
			uTimer8ID = timeSetEvent(0, TIMER_RESOLUTION, TimerFunc8, TempEvent.message, TIME_ONESHOT);// end the note now
		}
		time = (unsigned int)(dMilliSecondsPerTick * (double)(Event[thisX].ticksinnote));
		StopTimer();
		SaveEvents();
		FillRect(hdcMem, &rect, hBrush);
		InvalidateRect(hwnd, &rect, FALSE);
	}
}

void NewEvent(void)
{
	for (x = 4; x < (int)e; x++) {
		if ((TempEvent.note == Event[x].note) && (TempEvent.channel == Event[x].channel) && (TempEvent.port == Event[x].port) && (Event[x].velocity)) {
			if ((TempEvent.pixel > Event[x].pixel) && (TempEvent.pixel < (Event[x].pixel + Event[x].pixelsinnote)))
				break;
			else if (((TempEvent.pixel + TempEvent.pixelsinnote) > Event[x].pixel) && ((TempEvent.pixel + TempEvent.pixelsinnote) < (Event[x].pixel + Event[x].pixelsinnote)))
				break;
			else if ((TempEvent.pixel < Event[x].pixel) && ((TempEvent.pixel + TempEvent.pixelsinnote) > (Event[x].pixel + Event[x].pixelsinnote)))
				break;
		}
	}
	if (x == (int)e) {// not covering another note
		Event[e] = TempEvent;
		if (!Letter[Event[e].note - 21])
			Event[e].sharporflat = 1;
		else
			Event[e].sharporflat = 0;
		ChordEvent[lastNote++] = TempEvent;
		Event[e].message = (0x90|Event[e].channel) | (Event[e].velocity << 16) | (Event[e].note << 8);
		if (!EWQL)
			midiOutShortMsg(hMidiOut, Event[e].message);
		else
			midiOutShortMsg(hMidisOut[Port], Event[e].message);
		e++;
		Event[e].velocity = 0;// note end
		Event[e].tickptr = Event[e-1].tickptr + Event[e-1].ticksinnote;
		Event[e].pixelsinnote = 0;
		Event[e].ticksinnote = 0;
		Event[e].pixel = Event[e].tickptr * 40 / TicksPerQuarterNote;
		Event[e].note = Event[e-1].note;
		Event[e].channel = ActiveChannel;
		Event[e].port = Port;
		Event[e].message = (0x90|ActiveChannel) | (Event[e].note << 8);
		Event[e].dMilliSecondsPerTick = 0;
		Event[e].sharporflat = 0;
		Event[e].type = 0;
		Event[e].len = 0;
		Event[e].ptr = 0;
		e++;
	}
}

void SaveEvents(void)
{
	DWORD x;

	u++;
	if (u > 9999) {
		u = 0;
		Loop++;
	}
	pUndo = u;
	if (LastEventInUndo[u] != -1)
		free(UndoEvent[u]);
	UndoEvent[u] = (struct EVENT*)calloc(1, e * sizeof(Event[0]));
	for (x = 0; x < e; x++)
		UndoEvent[u][x] = Event[x];
	LastEventInUndo[u] = e;
}

void ChangeInstrument(BYTE channel)
{
	DWORD y;

	if (ChannelInstruments[channel][0] < 128) {
//		ordinalNote = -1; // flag for MoveNoteUpDown
//		OriginalNote = 1;// trick for MoveNoteUpDown
		y = yLoc - (Row * PixelsInGrandStaffAndMyExtraSpace);
		l = (Page * Rows) + Row;
		z = xLoc + Lines[l].pixel;
		for (x = 0; x < 88; x++)
			if (y >= (DWORD)NoteLoc[x])
				break;// got note
		ordinalNote = x + 21;// won't be a sharp or flat note
		Note = ordinalNote; // THIS IS WHERE Note	IS OBTAINED (NOT WM_LBUTTONDOWN OR WM_LBUTTONUP)
		OriginalNote = 0;// flag for VK_UP and VK_DOWN and VK_LEFT and VK_RIGHT

		ActiveChannel = channel;
		ActiveChannels[channel] = TRUE;
		ActiveInstrument = ChannelInstruments[channel][0];
		VirtualActiveChannel = 0xFF; // flag
		if (!EWQL)
			midiOutShortMsg(hMidiOut, (0xC0|ActiveChannel) | (ActiveInstrument << 8));
		instrument = ChannelInstruments[channel][0];
		for (y = 0; (Event[y].tickptr == 0) && (y < e); y++) {
			if ((Event[y].message & 0xFF) == (DWORD)(0xC0|channel)) {
					if (((Event[y].message >> 8) & 0xFF) != instrument) {
						Event[y].message &= 0xFF;
						Event[y].message |= (instrument << 8);
					}
					break;
			}
		}
		if ((Event[y].tickptr != 0) || (y == e)) {// not found
			for (y = 0; ((Event[y].message & 0xF0) != 0xC0) && (Event[y].tickptr == 0) && (y < e); y++)
				;
			for (i = e; i >= y; i--)
				Event[i+1] = Event[i];
			Event[y].pixel = 0;
			Event[y].tickptr = 0;
			Event[y].message = (0xC0|channel) | (instrument << 8);
			Event[y].channel = channel;
			Event[y].port = 0;
			Event[y].pixelsinnote = 0;//in case Event[y].tickptr != 0
			Event[y].ticksinnote = 0;
			Event[y].dMilliSecondsPerTick = 0;
			Event[y].note = 0;
			Event[y].velocity = 0;
			Event[y].sharporflat = 0;
			Event[y].time = 0;
			Event[y].BeatsPerMeasure = 0;
			Event[y].BeatNoteType = 0;
			Event[y].KeySignature = 0;
			Event[y].type = 0;
			Event[y].len = 0;
			Event[y].ptr = 0;
			Event[y].finger[0] = 0;
			Event[y].overlapped = 0;
			e++;
		}
		InvalidateRect(hwnd, &rect, FALSE);
	}
}

void ShowControl(char* buffer)
{
	for (y = 0; buffer[y] != 0; y++)
		LogBuf[lb++] = buffer[y];
	b = (BYTE)((Event[x].message & 0xFF0000) >> 0x10);
	if (b / 100)
		LogBuf[lb++] = (BYTE)(b / 100) + '0';
	if (b / 10)
		LogBuf[lb++] = (BYTE)((b % 100) / 10) + '0';
	LogBuf[lb++] = (BYTE)(b % 10) + '0';
}

void FillLogBuf(void)
{
	lb = 0;

	for (x = 0; (x < (int)e) && (lb < 3999900); x++) {
		if (Event[x].type)
			continue;
		if ((nonotes) && (Event[x].note))
			continue;
		if ((nonoteoffs) && (Event[x].note) && ((Event[x].velocity == 0) || ((Event[x].message & 0xF0) == 0x80)))
			continue;
		if ((volumeonly) && ((Event[x].message & 0xFFF0) != 0x07B0))
			continue;
		if ((expressiononly) && ((Event[x].message & 0xFFF0) != 0x0BB0))
			continue;
		if ((panonly) && ((Event[x].message & 0xFFF0) != 0x0AB0))
			continue;
		if ((reverbonly) && ((Event[x].message & 0xFFF0) != 0x5BB0))
			continue;
		if ((chorusonly) && ((Event[x].message & 0xFFF0) != 0x5DB0))
			continue;
		if ((modulationonly) && ((Event[x].message & 0xFFF0) != 0x01B0))
			continue;
		if ((sustainonly) && ((Event[x].message & 0xFFF0) != 0x40B0))
			continue;
		if ((pitchbendonly) && ((Event[x].message & 0xF0) != 0xE0))
			continue;
		if ((bank) && ((Event[x].message & 0xFFF0) != 0x00B0))
			continue;
		if ((chaninst) && ((Event[x].message & 0xF0) != 0xC0))
			continue;
		if ((bpmonly) && (Event[x].dMilliSecondsPerTick == 0))
			continue;

		if ((ActiveChannels[Event[x].message & 0xF]) || (Event[x].message == 0)) {
			firstLB = lb;
			LogBuf[lb++] = ' ';// for possible +
			LogBuf[lb++] = (BYTE)(Event[x].pixel / TENTHOUSAND) + '0';
			LogBuf[lb++] = (BYTE)((Event[x].pixel % TENTHOUSAND) / THOUSAND) + '0';
			LogBuf[lb++] = (BYTE)((Event[x].pixel % THOUSAND) / HUNDRED) + '0';
			LogBuf[lb++] = (BYTE)((Event[x].pixel % HUNDRED) / TEN) + '0';
			LogBuf[lb++] = (BYTE)(Event[x].pixel % TEN) + '0';
			LogBuf[lb++] = ' ';

			channel = (BYTE)(Event[x].message & 0xF);
			if (Event[x].note) {
				if ((channel == 9) && (!EWQL)) {// percussion
					ptrPercussion = Percussion[Event[x].note - 27];
					while (*ptrPercussion)
						LogBuf[lb++] = *ptrPercussion++;
					LogBuf[lb++] = '[';
					if ((channel + 1) / 10)
						LogBuf[lb++] = (BYTE)((channel + 1)/ 10) + '0';
					LogBuf[lb++] = (char)((channel + 1) % 10) + '0';
					LogBuf[lb++] = ']';
					LogBuf[lb++] = ' ';
				}
				else { // channel != 9
					lb--;// because of space at beginning of instrument name
					// get the last ChannelInstruments[channel][y] that's not 0xFF
					if (Event[x].velocity) {
						for (y = 0; (ChannelInstruments[channel][y] != 0xFF) && (InstrumentOffset[channel][y] <= Event[x].pixel) && (y < 16); y++) {
							if (Event[x].port != y)
								continue;
						}
					}
					else { // note off
						for (y = 0; (ChannelInstruments[channel][y] != 0xFF) && (InstrumentOffset[channel][y] < Event[x].pixel) && (y < 16); y++) {
							if (Event[x].port != y)
								continue;
						}
					}
					thisInstrument = ChannelInstruments[channel][y-1];
					if (!EWQL) {
						for (z = 4; myInstruments[thisInstrument][z]; z++)
							LogBuf[lb++] = myInstruments[thisInstrument][z];
					}
					else {
						for (y = 0, v = 0; v < thisInstrument; y++, v++) {
							for (w = y; (InstrumentBufs[Event[x].port][w] != '\r') && (InstrumentBufs[Event[x].port][w] != 0); w++)
								;
							y = w+1;
						}
						LogBuf[lb++] = ' ';
						for ( ;(InstrumentBufs[Event[x].port][y] != '\r') && (InstrumentBufs[Event[x].port][y] != 0) && (InstrumentBufs[Event[x].port][y] != '('); w++, y++)
							LogBuf[lb++] = InstrumentBufs[Event[x].port][y];
					}
					LogBuf[lb++] = '[';
					if ((channel + 1) / 10)
						LogBuf[lb++] = (BYTE)((channel + 1)/ 10) + '0';
					LogBuf[lb++] = (char)((channel + 1) % 10) + '0';
					LogBuf[lb++] = ']';
					LogBuf[lb++] = ' ';
					if (Letter[Event[x].note-21]) {
						LogBuf[lb++] = Letter[Event[x].note-21];
					}
					else if (usingsharp) {
						LogBuf[lb++] = Letter[Event[x].note-22];
						LogBuf[lb++] = '#';
					}
					else {
						LogBuf[lb++] = Letter[Event[x].note-20];
						LogBuf[lb++] = 'b';
					}
				}
				LogBuf[lb++] = ' ';
				if (Event[x].note / 100)
					LogBuf[lb++] = (BYTE)(Event[x].note / 100) + '0';
				LogBuf[lb++] = (BYTE)((Event[x].note % 100) / 10) + '0';
				LogBuf[lb++] = (BYTE)(Event[x].note % 10) + '0';
				LogBuf[lb++] = ' ';
				if ((Event[x].velocity) && ((Event[x].message & 0xF0) == 0x90)) {
					LogBuf[firstLB] = '+';
					LogBuf[lb++] = '(';
					if (Event[x].velocity / 100)
						LogBuf[lb++] = (BYTE)(Event[x].velocity / 100) + '0';
					LogBuf[lb++] = (BYTE)((Event[x].velocity % 100) / 10) + '0';
					LogBuf[lb++] = (BYTE)(Event[x].velocity % 10) + '0';
					LogBuf[lb++] = ')';
					LogBuf[lb++] = ' ';
					if (Event[x].pixelsinnote / 100)
						LogBuf[lb++] = (BYTE)(Event[x].pixelsinnote / 100) + '0';
					LogBuf[lb++] = (BYTE)((Event[x].pixelsinnote % 100) / 10) + '0';
					LogBuf[lb++] = (BYTE)(Event[x].pixelsinnote % 10) + '0';
				}
				else
					LogBuf[firstLB] = '-';
			}

			else if (Event[x].message) {
				if ((Event[x].message & 0xF0) == 0xC0) {// Program Change
					for (y = 0; Chan[y] != 0; y++)
						LogBuf[lb++] = Chan[y];
					ch = (BYTE)((channel + 1) / 10) + '0';
					if (ch != '0')
						LogBuf[lb++] = ch;
					LogBuf[lb++] = (BYTE)((channel + 1) % 10) + '0';

					if ((channel == 9) && (!EWQL)) {// percussion
						LogBuf[lb++] = ' ';
						for (y = 0; Perc[y] != 0; y++)
							LogBuf[lb++] = Perc[y];
					}
					else {
						for (y = 4; myInstruments[(Event[x].message & 0xFF00) >> 8][y]; y++)
							LogBuf[lb++] = myInstruments[(Event[x].message & 0xFF00) >> 8][y];
					}
				}

				else if ((Event[x].message & 0xF0) == 0xB0) {// Control
					if (((Event[x].message & 0xFF00) >> 8) == 0) { // Bank
						for (y = 0; Chan[y] != 0; y++)
							LogBuf[lb++] = Chan[y];
						ch = (BYTE)((channel + 1) / 10) + '0';
						if (ch != '0')
							LogBuf[lb++] = ch;
						LogBuf[lb++] = (BYTE)((channel + 1) % 10) + '0';

						for (y = 0; ChanBank[y] != 0; y++)
							LogBuf[lb++] = ChanBank[y];
						if ((Bank+1) > 9)
							LogBuf[lb++] = (BYTE)((Bank+1) / 10) + '0';
						LogBuf[lb++] = (BYTE)((Bank+1) % 10) + '0';
					}
					else { // all other 0xB0 controls
						if ((channel == 9) && (!EWQL)) {// percussion
							for (y = 0; Perc[y] != 0; y++)
								LogBuf[lb++] = Perc[y];
						}
						else {
							lb--;// because of space at beginning of instrument name
							for (y = 0; (ChannelInstruments[channel][y] != 0xFF) && (InstrumentOffset[channel][y] <= Event[x].pixel) && (y < 16); y++)
								;
							for (z = 4; myInstruments[ChannelInstruments[channel][y-1]][z]; z++)
								LogBuf[lb++] = myInstruments[ChannelInstruments[channel][y-1]][z];
						}
						LogBuf[lb++] = '[';
						if ((channel+1) / 10)
							LogBuf[lb++] = (BYTE)((channel+1)/ 10) + '0';
						LogBuf[lb++] = (char)((channel+1) % 10) + '0';
						LogBuf[lb++] = ']';
						LogBuf[lb++] = ' ';

						if (((Event[x].message & 0xFF00) >> 8) == 0x40)
							ShowControl(Sustain);
						else if (((Event[x].message & 0xFF00) >> 8)  == 10)
							ShowControl(Pan);
						else if (((Event[x].message & 0xFF00) >> 8)  == 7)
							ShowControl(ChanVol);
						else if (((Event[x].message & 0xFF00) >> 8)  == 1)
							ShowControl(modWheel);
						else if (((Event[x].message & 0xFF00) >> 8) == 91)
							ShowControl(Reverb);
						else if (((Event[x].message & 0xFF00) >> 8) == 93)
							ShowControl(Chorus);
						else if (((Event[x].message & 0xFF00) >> 8) == 11)
							ShowControl(Expression);
						else if (((Event[x].message & 0xFF00) >> 8) == 121)
							for (y = 0; AllControllersOff[y] != 0; y++)
								LogBuf[lb++] = AllControllersOff[y];
						else if (((Event[x].message & 0xFF00) >> 8) == 123)
							for (y = 0; AllNotesOff[y] != 0; y++)
								LogBuf[lb++] = AllNotesOff[y];
						else if (((Event[x].message & 0xFF00) >> 8) == 101)
							ShowControl(RPN);
						else if (((Event[x].message & 0xFF00) >> 8) == 100)
							ShowControl(RPN2);
						else if (((Event[x].message & 0xFF00) >> 8) == 99)
							ShowControl(NRPN);
						else if (((Event[x].message & 0xFF00) >> 8) == 98)
							ShowControl(NRPN2);
						else if (((Event[x].message & 0xFF00) >> 8) == 6)
							ShowControl(DataEntry);
						else if (((Event[x].message & 0xFF00) >> 8) == 38)
							ShowControl(DataEntry2);
						else if (((Event[x].message & 0xFF00) >> 8) == 67)
							ShowControl(SoftPedal);
						else if (((Event[x].message & 0xFF00) >> 8) == 32)
							ShowControl(FineBankSelect);
						else if (((Event[x].message & 0xFF00) >> 8) == 5)
							ShowControl(portamentoRate);
						else if (((Event[x].message & 0xFF00) >> 8) == 37)
							ShowControl(portamentoRate2);
						else if (((Event[x].message & 0xFF00) >> 8) == 65)
							ShowControl(portamento);
						else {
							for (y = 0; unknownControl[y] != 0; y++)
								LogBuf[lb++] = unknownControl[y];
							b = (BYTE)((Event[x].message & 0xFF00) >> 8);
							LogBuf[lb++] = (BYTE)(b / 100) + '0';
							LogBuf[lb++] = (BYTE)((b % 100) / 10) + '0';
							LogBuf[lb++] = (BYTE)(b % 10) + '0';
						}
					}
				}
				else if ((Event[x].message & 0xF0) == 0xA0) {// Key Aftertouch
					for (y = 0; Chan[y] != 0; y++)
						LogBuf[lb++] = Chan[y];
					ch = (BYTE)(((Event[x].message & 0xF) + 1) / 10) + '0';
					if (ch != '0')
						LogBuf[lb++] = ch;
					LogBuf[lb++] = (BYTE)(((Event[x].message & 0xF) + 1) % 10) + '0';
					LogBuf[lb++] = ' ';

					for (y = 0; KeyAftertouch[y] != 0; y++)
						LogBuf[lb++] = KeyAftertouch[y];

					b = (BYTE)(Event[x].message >> 8) & 0xFF;
					if (Letter[b-21]) {
						LogBuf[lb++] = Letter[b-21];
					}
					else if (usingsharp) {
						LogBuf[lb++] = Letter[b-20];
						LogBuf[lb++] = '#';
					}
					else {
						LogBuf[lb++] = Letter[b-22];
						LogBuf[lb++] = 'b';
					}
					LogBuf[lb++] = '=';
					b = (BYTE)((Event[x].message & 0xFF0000) >> 0x10);
					LogBuf[lb++] = (BYTE)(b / 100) + '0';
					LogBuf[lb++] = (BYTE)((b % 100) / 10) + '0';
					LogBuf[lb++] = (BYTE)(b % 10) + '0';
				}
				else if ((Event[x].message & 0xF0) == 0xD0) {// Aftertouch
					for (y = 0; Chan[y] != 0; y++)
						LogBuf[lb++] = Chan[y];
					ch = (BYTE)(((Event[x].message & 0xF) + 1) / 10) + '0';
					if (ch != '0')
						LogBuf[lb++] = ch;
					LogBuf[lb++] = (BYTE)(((Event[x].message & 0xF) + 1) % 10) + '0';
					LogBuf[lb++] = ' ';

					for (y = 0; AfterTouch[y] != 0; y++)
						LogBuf[lb++] = AfterTouch[y];
					b = (BYTE)((Event[x].message & 0xFF00) >> 8);
					LogBuf[lb++] = (BYTE)(b / 100) + '0';
					LogBuf[lb++] = (BYTE)((b % 100) / 10) + '0';
					LogBuf[lb++] = (BYTE)(b % 10) + '0';
				}
				else if ((Event[x].message & 0xF0) == 0xE0) {// Pitch Bend
					if ((channel == 9) && (!EWQL)) { // percussion
						ptrPercussion = Percussion[Event[x].note - 27];
						while (*ptrPercussion)
							LogBuf[lb++] = *ptrPercussion++;
					}
					else {
						lb--;// because of space at beginning of instrument name
						for (y = 0; (ChannelInstruments[channel][y] != 0xFF) && (InstrumentOffset[channel][y] <= Event[x].pixel) && (y < 16); y++)
							;
						for (z = 4; myInstruments[ChannelInstruments[channel][y-1]][z]; z++)
							LogBuf[lb++] = myInstruments[ChannelInstruments[channel][y-1]][z];
					}
					LogBuf[lb++] = '[';
					if ((channel + 1) / 10)
						LogBuf[lb++] = (BYTE)((channel + 1)/ 10) + '0';
					LogBuf[lb++] = (char)((channel + 1) % 10) + '0';
					LogBuf[lb++] = ']';
					LogBuf[lb++] = ' ';

					for (y = 0; pitchBend[y] != 0; y++)
						LogBuf[lb++] = pitchBend[y];
					v = (Event[x].message & 0xFF00) >> 8;
					w = (Event[x].message & 0xFF0000) >> 9;// >> 16, and then << 7
					y = v | w;
					if (y / TENTHOUSAND)
						LogBuf[lb++] = (BYTE)(y / TENTHOUSAND) + '0';
					if (y / THOUSAND)
						LogBuf[lb++] = (BYTE)((y % TENTHOUSAND) / THOUSAND) + '0';
					if (y / HUNDRED)
						LogBuf[lb++] = (BYTE)((y % THOUSAND) / HUNDRED) + '0';
					if (y / TEN)
						LogBuf[lb++] = (BYTE)((y % HUNDRED) / TEN) + '0';
					LogBuf[lb++] = (BYTE)(y % TEN) + '0';
				}
				else {
					for (y = 0; unknownMessage[y] != 0; y++)
						LogBuf[lb++] = unknownMessage[y];
					b = (BYTE)(Event[x].message >> 2);
					LogBuf[lb++] = '0';
					LogBuf[lb++] = 'x';
					b = (BYTE)(Event[x].message >> 4) & 0xF;
					b |= '0';
					if (b > '9') b += 7;
					LogBuf[lb++] = b;
					b = (BYTE)Event[x].message & 0xF;
					b |= '0';
					if (b > '9') b += 7;
					LogBuf[lb++] = b;
				}
			}// end of if (Event[x].message)

			else if (Event[x].dMilliSecondsPerTick) {
				for (y = 0; beatsPerMinute[y] != 0; y++)
					LogBuf[lb++] = beatsPerMinute[y];
				d = 60000.0 / (Event[x].dMilliSecondsPerTick * (double)TicksPerQuarterNote);
				d3 = modf(d, &d2);
				if (d3 > 0.5)
					d2 += 1.0;
				y = (int)d2;
				if (y / TENTHOUSAND)
					LogBuf[lb++] = (BYTE)(y / TENTHOUSAND) + '0';
				if ((y % TENTHOUSAND) / THOUSAND)
					LogBuf[lb++] = (BYTE)((y % TENTHOUSAND) / THOUSAND) + '0';
				if ((y % THOUSAND) / HUNDRED)
					LogBuf[lb++] = (BYTE)((y % THOUSAND) / HUNDRED) + '0';
				LogBuf[lb++] = (BYTE)((y % HUNDRED) / TEN) + '0';
				LogBuf[lb++] = (BYTE)(y % TEN) + '0';
			}

			else if (Event[x].KeySignature) {
				for (y = 0; keysig[y] != 0; y++)
					LogBuf[lb++] = keysig[y];
				for (y = 0; y < 13; y++) {
					if (Keys[y] == Event[x].KeySignature) {
						LogBuf[lb++] = keys[y*2];
						LogBuf[lb++] = keys[(y*2)+1];
						break;
					}
				}
			}

			else if (Event[x].BeatNoteType) {
				for (y = 0; TimeSignature[y] != 0; y++)
					LogBuf[lb++] = TimeSignature[y];
				if (Event[x].BeatsPerMeasure / 10)
					LogBuf[lb++] = (Event[x].BeatsPerMeasure / 10) + '0';
				LogBuf[lb++] = (Event[x].BeatsPerMeasure % 10) + '0';
				LogBuf[lb++] = '/';
				if (Event[x].BeatNoteType / 10)
					LogBuf[lb++] = (Event[x].BeatNoteType / 10) + '0';
				LogBuf[lb++] = (Event[x].BeatNoteType % 10) + '0';
			}

			else
				for (y = 0; unknownEntry[y] != 0; y++)
					LogBuf[lb++] = unknownEntry[y];
			LogBuf[lb++] = 0;
			*(DWORD*)&LogBuf[lb] = x;// for EventIndex[] (index = x)
			lb += 4;
		}// end of if (ActiveChannel[x]
	}// end of for (x = 0;

	if (lb >= 3999900)
		MessageBox(NULL, "Not all data was translated.", ERROR, MB_OK);
	hFile = CreateFile("Log.txt", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		BYTE *TempBuf;

		TempBuf = malloc(lb);
		for (x = 0; x < lb; x++) {
			if (LogBuf[x] != 0)
				TempBuf[x] = LogBuf[x];
			else {
				TempBuf[x++] = ' ';
				TempBuf[x++] = ' ';
				TempBuf[x++] = ' ';
				TempBuf[x++] = '\r';
				TempBuf[x] = '\n';
			}
		}
		WriteFile(hFile, TempBuf, x, &dwBytesWritten, NULL);
		CloseHandle(hFile);
		free(TempBuf);
	}
}

DWORD Atoi(char *ptr)
{
	DWORD x;

	for (x = 0;(*ptr >= '0') && (*ptr <= '9'); ptr++)
	{
		x *= 10;
		x += *ptr - '0';
	}
	return x;
}
/*
void Itoa(int number, char *ch)
{
	if (number > 100000)
		*ch++ = ((number % 1000000) / 100000) + '0';
	else
		*ch++ = ' ';
	if (number > 10000)
		*ch++ = ((number % 100000) / 10000) + '0';
	else
		*ch++ = ' ';
	if (number > 1000)
		*ch++ = ((number % 10000) / 1000) + '0';
	else
		*ch++ = ' ';
	if (number > 100)
		*ch++ = ((number % 1000) / 100) + '0';
	else
		*ch++ = ' ';
	if (number > 10)
		*ch++ = ((number % 100) / 10) + '0';
	else
		*ch++ = ' ';
	if (number > 1)
		*ch++ = (number % 10) + '0';
	else
		*ch++ = ' ';
}
*/
void DrawNote(int x, int y)
{
	if (sharporflat == 0) {
		if (Event[ptr].overlapped)
			SelectObject(hdcMem, hBlackPen);
		else if ((Channel == 0) || (Channel == 8))
			SelectObject(hdcMem, hRedPen);
		else if ((Channel == 1) || (Channel == 10))
			SelectObject(hdcMem, hOrangePen);
		else if ((Channel == 2) || (Channel == 11))
			SelectObject(hdcMem, hYellowPen);
		else if ((Channel == 3) || (Channel == 12))
			SelectObject(hdcMem, hGreenPen);
		else if ((Channel == 4) || (Channel == 13))
			SelectObject(hdcMem, hBlueGreenPen);
		else if ((Channel == 5) || (Channel == 14))
			SelectObject(hdcMem, hBluePen);
		else if ((Channel == 6) || (Channel == 15))
			SelectObject(hdcMem, hVioletPen);
		else if (Channel == 7)
			SelectObject(hdcMem, hBrownPen);
		else if (Channel == 9)
			SelectObject(hdcMem, hGreyNaturalPen);
	}
	else {
		if (usingsharp) {
			if (Event[ptr].overlapped)
				SelectObject(hdcMem, hBlackPen);
			else if ((Channel == 0) || (Channel == 8))
				SelectObject(hdcMem, hRedSharpPen);
			else if (Channel == 1)
				SelectObject(hdcMem, hOrangeSharpPen);
			else if ((Channel == 2) || (Channel == 11))
				SelectObject(hdcMem, hYellowSharpPen);
			else if ((Channel == 3) || (Channel == 12))
				SelectObject(hdcMem, hGreenSharpPen);
			else if ((Channel == 4) || (Channel == 13))
				SelectObject(hdcMem, hBlueGreenSharpPen);
			else if ((Channel == 5) || (Channel == 14))
				SelectObject(hdcMem, hBlueSharpPen);
			else if ((Channel == 6) || (Channel == 15))
				SelectObject(hdcMem, hVioletSharpPen);
			else if ((Channel == 7) || (Channel == 10))
				SelectObject(hdcMem, hBrownSharpPen);
			else if (Channel == 9)
				SelectObject(hdcMem, hGreySharpPen);
		}
		else {
			if (Event[ptr].overlapped)
				SelectObject(hdcMem, hBlackPen);
			else if ((Channel == 0) || (Channel == 8))
				SelectObject(hdcMem, hRedFlatPen);
			else if (Channel == 1)
				SelectObject(hdcMem, hOrangeFlatPen);
			else if ((Channel == 2) || (Channel == 11))
				SelectObject(hdcMem, hYellowFlatPen);
			else if ((Channel == 3) || (Channel == 12))
				SelectObject(hdcMem, hGreenFlatPen);
			else if ((Channel == 4) || (Channel == 13))
				SelectObject(hdcMem, hBlueGreenFlatPen);
			else if ((Channel == 5) || (Channel == 14))
				SelectObject(hdcMem, hBlueFlatPen);
			else if ((Channel == 6) || (Channel == 15))
				SelectObject(hdcMem, hVioletFlatPen);
			else if ((Channel == 7) || (Channel == 10))
				SelectObject(hdcMem, hBrownFlatPen);
			else if (Channel == 9)
				SelectObject(hdcMem, hGreyFlatPen);
			y -= NoteMiddle;
			notebottom -= NoteMiddle;
		}
	}
	if (onlynotenames == FALSE) {
		if (showaspace)
			z = notend-1;
		else
			z = notend;
		if (((Channel <= 7) || (Channel == 9)) && (Event[ptr].overlapped == FALSE)) {
			for ( ; y < (int)notebottom; y++) {// DRAW PIANO ROLL NOTE
				MoveToEx(hdcMem, x, y, NULL);
				LineTo(hdcMem, z, y);
			}
		}
		else {
			for ( ; y < (int)notebottom; y += 2) {// DRAW PIANO ROLL NOTE
				MoveToEx(hdcMem, x, y, NULL);
				LineTo(hdcMem, z, y);
			}
		}
	}
}

void ChangeVolume(void)
{
	for (x = 0; x < 16; x++) {
		vol = origVol[x] + volchange;
		if (vol > 127)
			vol = 127;
		else if (vol < 0)
			vol = 0;
		if (!EWQL)
			midiOutShortMsg(hMidiOut, (0x07B0|x) | (vol << 16));
		else {
			for (y = 0; y < NumberOfPorts; y++)
				midiOutShortMsg(hMidisOut[y], (0x07B0|x) | (vol << 16));
		}
	}
	CheckMenuItem(hMenu, PLAYBACKVOLUME100, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME80, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME60, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME40, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME20, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME0, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME_20, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME_40, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME_60, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME_80, MF_UNCHECKED);
	CheckMenuItem(hMenu, PLAYBACKVOLUME_100, MF_UNCHECKED);
}

void UpdateVolChange(void)
{
		switch (volchange)
		{
		case 100:
			CheckMenuItem(hMenu, PLAYBACKVOLUME100, MF_CHECKED);
			break;
		case 80:
			CheckMenuItem(hMenu, PLAYBACKVOLUME80, MF_CHECKED);
			break;
		case 60:
			CheckMenuItem(hMenu, PLAYBACKVOLUME60, MF_CHECKED);
			break;
		case 40:
			CheckMenuItem(hMenu, PLAYBACKVOLUME40, MF_CHECKED);
			break;
		case 20:
			CheckMenuItem(hMenu, PLAYBACKVOLUME20, MF_CHECKED);
			break;
		case 0:
			CheckMenuItem(hMenu, PLAYBACKVOLUME0, MF_CHECKED);
			break;
		case -20:
			CheckMenuItem(hMenu, PLAYBACKVOLUME_20, MF_CHECKED);
			break;
		case -40:
			CheckMenuItem(hMenu, PLAYBACKVOLUME_40, MF_CHECKED);
			break;
		case -60:
			CheckMenuItem(hMenu, PLAYBACKVOLUME_60, MF_CHECKED);
			break;
		case -80:
			CheckMenuItem(hMenu, PLAYBACKVOLUME_80, MF_CHECKED);
			break;
		case -100:
			CheckMenuItem(hMenu, PLAYBACKVOLUME_100, MF_CHECKED);
			break;
		}
}

void TitleBar(void)
{
	strcpy(TitleName, szAppName);
	x = strlen(Filename);
	if (x < (256 - 27)) {
		strcat(TitleName, "  \"");
		for (x = 0; (Filename[x] != '.') && (Filename[x] != 0); x++)
			;
		ch = Filename[x];
		Filename[x] = 0;
		strcat(TitleName, Filename);
		Filename[x] = ch;
		strcat(TitleName, "\"");
	}
	SetWindowText(hwnd, TitleName);
}

void MixerTitleBar(void)
{
	strcpy(TitleName, AudioMixer);
	x = strlen(Filename);
	if (x < (256 - 27)) {
		strcat(TitleName, "  \"");
		for (x = 0; (Filename[x] != '.') && (Filename[x] != 0); x++)
			;
		ch = Filename[x];
		Filename[x] = 0;
		strcat(TitleName, Filename);
		Filename[x] = ch;
		strcat(TitleName, "\"");
	}
	SetWindowText(hwndMixer, TitleName);
}

void GetEventTickptr(void)
{
	Event[e].time = timeGetTime();
	if (Startime == 0) {// if starting before a full measure of ticks
		Startime = Event[e].time;
		lastE = e;
		counter = 1;// 0xFF;
//		firstnote = FALSE;
	}
	if (!othernotesplaying) {
		x = MulDiv((int)((pixel-Pixel) * TicksPerQuarterNote), (int)(Event[e].time - Startime), (int)(Beats * milliSecondsPerBeat * 40));
		if (x != -1)
			Event[e].tickptr = x;
		else// if Beats was 0
			Event[e].tickptr = Tickptr;
	}
	else {// other notes playing
		EnterCriticalSection(&csTickptr); Tock = Tickptr; Tickptr = 0xFFFFFFFF; LeaveCriticalSection(&csTickptr);
		if (Tock != 0xFFFFFFFF) {
			Event[e].tickptr = Tock;
		}
		else {// if starting a note before a written note starts
			for (x = e-1; x >= 0; x--) {
				if (Event[x].time) {
					Event[e].tickptr = Event[x].tickptr + (int)((double)(Event[e].time - Event[x].time) / dMilliSecondsPerTick);
					break;
				}
			}
			if (x < 0)
				Event[e].tickptr = 0;
		}
	}
	Event[e].pixel = Event[e].tickptr * 40 / TicksPerQuarterNote;
	Event[e].channel = ActiveChannel;
	Event[e].port = Port;
}

void StopTimer(void)
{
	timeBeginPeriod(TIMER_RESOLUTION); 
	if (uTimer3ID == 0)
		uTimer3ID = timeSetEvent(time, TIMER_RESOLUTION, TimerFunc3, TempEvent.message, TIME_ONESHOT);// end the note
	else if (uTimer7ID == 0)
		uTimer7ID = timeSetEvent(time, TIMER_RESOLUTION, TimerFunc7, TempEvent.message, TIME_ONESHOT);// end the note
	else if (uTimer8ID == 0)
		uTimer8ID = timeSetEvent(time, TIMER_RESOLUTION, TimerFunc8, TempEvent.message, TIME_ONESHOT);// end the note
}

//BOOL CALLBACK EnumChildProc(HWND hwndChild, LPARAM lParam)
//{
//	RECT rectChild;
//
//	GetWindowRect(hwndChild, &rectChild);
//	if (rectChild.top == 109)
//		hwndOpenList = hwndChild;
//	return TRUE;
//}

UINT CALLBACK OFNHookProc(HWND hwndHook, UINT message, WPARAM wParam, LPARAM lParam)
{// make it full-screen
	int width, height;
	RECT windowRect, rectChild;
	static HWND hwnd1, hwnd2, hwnd3, hwnd4, hwnd5, hwnd6;
	NMHDR *nmhdr;// a Common Control structure

	if (message == WM_NOTIFY) {
		nmhdr = (LPNMHDR)lParam;
		if (nmhdr->code == CDN_INITDONE) {// notification code
			hwndParent = GetParent(hwndHook); // also used in PlayListProc
//			EnumChildWindows(hwndParent, EnumChildProc, 0);
			if ((!fromplaylist) && (bigopen)) {
				GetWindowRect(hwndParent, &windowRect);
				width = (windowRect.right - windowRect.left) + 200;
				height = (windowRect.bottom - windowRect.top) + 200;
				MoveWindow(hwndParent, 0, 0, WorkArea.right, WorkArea.bottom, FALSE);// SystemParametersInfo(SPI_GETWORKAREA, 0, &WorkArea, 0)

				hwndOpenList = GetDlgItem(hwndParent, lst1);// lst1 from FILEOPEN.DLG
				GetWindowRect(hwndOpenList, &rectChild);
				MoveWindow(hwndOpenList, rectChild.left, rectChild.top-20, WorkArea.right-rectChild.left, WorkArea.bottom-80-rectChild.top, FALSE);

				hwnd1 = GetDlgItem(hwndParent, stc3);// "File &Name"
				GetWindowRect(hwnd1, &rectChild);
				MoveWindow(hwnd1, rectChild.left, WorkArea.bottom - 90, rectChild.right-rectChild.left, rectChild.bottom-rectChild.top, FALSE);

				hwnd2 = GetDlgItem(hwndParent, edt1);
				GetWindowRect(hwnd2, &rectChild);
				MoveWindow(hwnd2, rectChild.left, WorkArea.bottom - 90, rectChild.right-rectChild.left, rectChild.bottom-rectChild.top, FALSE);

				hwnd3 = GetDlgItem(hwndParent, stc2);// "File of &Type"
				GetWindowRect(hwnd3, &rectChild);
				MoveWindow(hwnd3, rectChild.left, WorkArea.bottom - 60, rectChild.right-rectChild.left, rectChild.bottom-rectChild.top, FALSE);

				hwnd4 = GetDlgItem(hwndParent, cmb1);
				GetWindowRect(hwnd4, &rectChild);
				MoveWindow(hwnd4, rectChild.left, WorkArea.bottom - 60, rectChild.right-rectChild.left, rectChild.bottom-rectChild.top, FALSE);

				hwnd5 = GetDlgItem(hwndParent, IDOK);
				GetWindowRect(hwnd5, &rectChild);
				MoveWindow(hwnd5, rectChild.left, WorkArea.bottom - 90, rectChild.right-rectChild.left, rectChild.bottom-rectChild.top, FALSE);

				hwnd6 = GetDlgItem(hwndParent, IDCANCEL);
				GetWindowRect(hwnd6, &rectChild);
				MoveWindow(hwnd6, rectChild.left, WorkArea.bottom - 60, rectChild.right-rectChild.left, rectChild.bottom-rectChild.top, FALSE);

//				PostMessage(hwndHook, WM_USER6, 0, 0);// to change to detail list view when Windows is ready
			}
		}
	}
//	if (message == WM_USER6)
//		SendMessage(hwndParent, WM_COMMAND, (WPARAM)(0x702C << 0x10) | GetDlgCtrlID(hwndOpenList), (LPARAM)(HWND)hwndOpenList);
	return 0;
}

void PlayPlayListProc(void)
{
	hFile = CreateFile(PlayList[playlist], GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		if (fileSize = GetFileSize(hFile, NULL)) {
			if (fileSize > MAX_MIDI) {
				MessageBox(hwnd, "That file is too big.", "", MB_OK);
				CloseHandle(hFile);
			}
			else {
				SendMessage(hwnd, WM_COMMAND, WM_USER4, 0);//most of ID_NEW
				ReadFile(hFile, Midi, fileSize, &dwBytesRead, NULL);
				CloseHandle(hFile);
				if (*(DWORD*)&Midi[0] == 0x46464952) {//"RIFF"
					if (*(DWORD*)&Midi[8] == 0x45564157) {// "WAVE"
						MessageBox(hwnd, "That's a WAVE file", "Oops", MB_OK);
						return;
					}
					for (x = 0, y = 20; y < (int)fileSize; x++, y++)
						Midi[x] = Midi[y];
					fileSize -= 20;
				}
				if (*(DWORD*)&Midi[0] == 0x6468544D) {//"MThd" - MIDI files are in Big Endian format (not Intel format)
					BOOL noinstruments = TRUE;

					for (x = 0; PlayList[playlist][x] != 0; x++)
						;
					for ( ; PlayList[playlist][x] != '\\'; x--)
						;
					for (x++, y = 0; PlayList[playlist][x] != 0; x++, y++)
						Filename[y] = PlayList[playlist][x];
					Filename[y] = 0; // for TitleBar

					TitleBar();
					for (y = 0; y < 16; y++) {
						for (x = 0; x < 16; x++)
							ChannelInstruments[y][x] = 0xFF;
					}
					if (!EWQL) {
						for (x = 0; x < 16; x++) {
							midiOutShortMsg(hMidiOut, (0xB0|x) | (121 << 8));// all controllers off
							midiOutShortMsg(hMidiOut, (0xB0|x) | (7 << 8) | (127 << 16));// full Volume (not default of 100)
							origVol[x] = 127;
						}
					}
					else {
						for (y = 0; y < NumberOfPorts; y++) {
							for (x = 0; x < 16; x++) {
								midiOutShortMsg(hMidisOut[y], (0xB0|x) | (121 << 8));// all controllers off
								midiOutShortMsg(hMidisOut[y], (0xB0|x) | (7 << 8) | (127 << 16));// full Volume (not default of 100)
								origVol[x] = 127;
							}
						}
					}
					TempoChange = 0.0;
					BigText[bt] = 0;
					//////////
					ReadMidi();
					//////////
					originalE = e;
					for (x = 0; x < (int)e; x++)
						EventPixels[x] = Event[x].pixel;// for SAVE at CLOSE

					for (x = 0; x < 16; x++) {
						if ((ChannelInstruments[x][0] != 0xFF) || (EWQL)) {
							ActiveChannels[x] = TRUE;
							noinstruments = FALSE;
						}
						else
							ActiveChannels[x] = FALSE;
					}
					if (noinstruments) {
						ChannelInstruments[0][0] = 0;// need at least a piano
						ActiveChannels[0] = TRUE;
					}
					ActiveChannel = 0;
					ActiveInstrument = 0;
					SaveEvents();// ID_FILES_OPEN
					FillRect(hdcMem, &rect, hBrush);
					InvalidateRect(hwnd, &rect, FALSE);
				}
				else {
					MessageBox(hwnd, "That's not a MIDI file.", ERROR, MB_OK);
				}
				Sleep(500);
				SendMessage(hwnd, WM_COMMAND, PLAY, 0);
			}
		}// end of if (fileSize = GetFileSize(hFile, NULL))
		else
			CloseHandle(hFile);
	}
}

int CALLBACK PlayListProc(HWND hwndPlayList, UINT message, WPARAM wParam, LPARAM lParam)
{
	int dlgWidth, dlgHeight;
	static HWND hwndList, hwndPlay;
	static RECT dlgRect;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndList = GetDlgItem(hwndPlayList, IDC_LIST1);
		hwndPlay = GetDlgItem(hwndPlayList, IDOK);
		SetWindowText(hwndPlay, "PLAY");

		GetWindowRect(hwndPlayList, &dlgRect);
		dlgWidth = dlgRect.right-dlgRect.left;
		dlgHeight = dlgRect.bottom-dlgRect.top;
		MoveWindow(hwndPlayList, rect.right-dlgWidth, TitleAndMenu, dlgWidth, dlgHeight, TRUE);
		break;

	case 0xABCD:
		SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)PlayList[playlist]);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			playlist = 0;
			PlayPlayListProc();
		case IDCANCEL:
			SendMessage(hwndPlayList, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_CLOSE:
		SendMessage(hwndParent, WM_CLOSE, 0, 0);
		fromplaylist = FALSE;
		ofn.Flags = OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_ENABLESIZING|OFN_EXPLORER; // no OFN_ENABLEHOOK
		DestroyWindow(hwndPlayList);
		hwndPlayList = NULL;
		break;
	}
	return 0;
}

int CALLBACK UserProc(HWND hwndUser, UINT message, WPARAM wParam, LPARAM lParam)
{ // show User.txt
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndUser, IDC_EDIT1);
		SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hHelpFont, (LPARAM)MAKELPARAM(TRUE, 0));
		SetWindowText(hwndEdit, UserBuf);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			SendMessage(hwndUser, WM_CLOSE, 0, 0);
			break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndUser);
		hwndUser = NULL;
		break;
	}
	return 0;
}

void RecordWave(void)
{
	done = FALSE; // for WM_USER3
	recordedtowave = FALSE;
	if ((keyboardactive) || (sustainflag))
		return;// unusual, but PLAY looks for these
	for (x = 0; x < (int)e; x++) // e is 4 with no music
		if (Event[x].note)
			break;
	if (x < (int)e) {
		musicflag = TRUE;
		SetWindowText(hwndMixer, AudioMixer);
		dMilliSeconds = prevMilliSecondsPer = 0.0;
		prevTickptr = 0;
		for (x = 0; x <= (int)e; x++) {
			if (Event[x].dMilliSecondsPerTick) {
				dMilliSeconds += (prevMilliSecondsPer * (double)(Event[x].tickptr - prevTickptr));
				prevMilliSecondsPer = Event[x].dMilliSecondsPerTick;
				prevTickptr = Event[x].tickptr;
			}
		}
		dMilliSeconds += (prevMilliSecondsPer * (double)(Event[e-1].tickptr - prevTickptr));
		if (wBitsPerSample != 24)
			d = dMilliSeconds * (double)(nSamplesPerSec * nRecordChannels * 2) / 1000.0;
		else
			d = dMilliSeconds * (double)(nSamplesPerSec * nRecordChannels * 3) / 1000.0;
		d2 = modf(d, &d3);
		BufferSize = (int)d3 + (BLOCK_SIZE * 12); // for unknown reason
		if (d2 > 0.1)
			BufferSize += 2;
	}
	else {
		BufferSize = 52920000; // 5 minutes at 176400 bytes/second (44100 kHz * 2 bytes/point * 2 channels)
		XXX = 0; // flag
	}
	if (wBitsPerSample != 24) {
		WaveFormat.Format.wFormatTag = WAVE_FORMAT_PCM;// simple, uncompressed format
		WaveFormat.Format.nBlockAlign = nRecordChannels * 2; // 2 is bytes/sample
		WaveFormat.Format.wBitsPerSample = 16;
		WaveFormat.Format.cbSize = 0;
	}
	else {
		WaveFormat.Format.wFormatTag = WAVE_FORMAT_EXTENSIBLE;
		WaveFormat.Format.nBlockAlign = nRecordChannels * 3; // 3 is bytes/sample
		WaveFormat.Format.wBitsPerSample = 24;
		WaveFormat.Format.cbSize = 22;
		WaveFormat.Samples.wValidBitsPerSample = 24;
		WaveFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
		// the following should be WaveFormat.Subformat = KSDATAFORMAT_SUBTYPE_PCM defined as 00000001-0000-0010-8000-00aa00389b71 (but that doesn't work here)
		WaveFormat.SubFormat.Data1 = 1;
		WaveFormat.SubFormat.Data2 = 0;
		WaveFormat.SubFormat.Data3 = 0x10;
		WaveFormat.SubFormat.Data4[0] = 0x80;
		WaveFormat.SubFormat.Data4[1] = 0;
		WaveFormat.SubFormat.Data4[2] = 0;
		WaveFormat.SubFormat.Data4[3] = 0xAA;
		WaveFormat.SubFormat.Data4[4] = 0;
		WaveFormat.SubFormat.Data4[5] = 0x38;
		WaveFormat.SubFormat.Data4[6] = 0x9B;
		WaveFormat.SubFormat.Data4[7] = 0x71;
	}
	WaveFormat.Format.nSamplesPerSec = nSamplesPerSec;
	WaveFormat.Format.nChannels = nRecordChannels;
	WaveFormat.Format.nAvgBytesPerSec = (DWORD)(nSamplesPerSec * WaveFormat.Format.nBlockAlign);
	BytesPerSample = WaveFormat.Format.wBitsPerSample / 8;
	if (WaveBuf)
		VirtualFree(WaveBuf, 0, MEM_RELEASE);
	WaveBuf = VirtualAlloc(NULL, BufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	WaveInHdr.lpData = (LPSTR)WaveBuf;
	WaveInHdr.dwBufferLength = BufferSize;
	WaveInHdr.dwBytesRecorded = 0;
	WaveInHdr.dwUser = 0;
	WaveInHdr.dwFlags = 0;
	WaveInHdr.dwLoops = 0;

	waveInOpen(&hWaveIn, indexRecord, &WaveFormat.Format, (DWORD)&waveInProc, 0, CALLBACK_FUNCTION);
	waveInPrepareHeader(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));
	waveInAddBuffer(hWaveIn, &WaveInHdr, sizeof(WAVEHDR));

	recordingtowave = TRUE;
	waveInStart(hWaveIn);

	dMultiplier = (double)meterRect.bottom / 32768.0;
	wave0 = 0;
	wavePtr = wave0;
	nSamplesPer40MilliSeconds = nSamplesPerSec / 25; // 40*25 = 1000
	BytesPer40MilliSeconds = nSamplesPer40MilliSeconds*WaveFormat.Format.nBlockAlign;
	timeBeginPeriod(TIMER_RESOLUTION);
	if (wBitsPerSample != 24)
		uTimerElevenID = timeSetEvent(40, TIMER_RESOLUTION, TimerFunc11, 0, TIME_PERIODIC);// 1000/25 = 40 to show Hz meter
	else
		uTimerElevenID = timeSetEvent(40, TIMER_RESOLUTION, TimerFunc12, 0, TIME_PERIODIC);// 1000/25 = 40 to show Hz meter
	ShowQuality(); // needs: nSamplesPerSec wBitsPerSample nRecordChannels
	if (musicflag)
		///////////////////////////////////////
		SendMessage(hwnd, WM_COMMAND, PLAY, 0);
		///////////////////////////////////////
	else
		InvalidateRect(hwnd, &rect, FALSE); // to show RecordingToWave
}

void PlayWave(void)
{
	hFile = CreateFile(FullFilename2, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		if (fileSize = GetFileSize(hFile, NULL)) {
			BufferSize = fileSize;
			if (WaveBuf)
				VirtualFree(WaveBuf, 0, MEM_RELEASE);
			WaveBuf = VirtualAlloc(NULL, BufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			ReadFile(hFile, WaveBuf, fileSize, &dwBytesRead, NULL);
			CloseHandle(hFile);
			if (*(DWORD*)&WaveBuf[8] == 0x45564157) {// "WAVE"
				if ((*(WORD*)&WaveBuf[20] == 1) || (*(WORD*)&WaveBuf[20] == 0xFFFE)) {// PCM or WAVEFORMATEXTENSIBLE
					subchunksize = *(DWORD*)&WaveBuf[16];
					nextchunk = subchunksize + 20;
					if (*(DWORD*)&WaveBuf[nextchunk] == 0x74636166)// "fact"
						nextchunk += 12; // ignore fact chunk
					if (*(DWORD*)&WaveBuf[nextchunk] == 0x61746164) {// "data"
						if (wavemixer) {
							if (playing2wavefiles)
								SetWindowText(hwndMixer, AudioMixer);
							else
								MixerTitleBar();
						}
						playingwave = TRUE;

						WaveFormat.Format.wFormatTag = *(WORD*)&WaveBuf[20]; // wave type
						nRecordChannels = *(WORD*)&WaveBuf[22]; // for ShowQuality
						WaveFormat.Format.nChannels = nRecordChannels;
						nSamplesPerSec = *(DWORD*)&WaveBuf[24];
						WaveFormat.Format.nSamplesPerSec = nSamplesPerSec;
						WaveFormat.Format.nAvgBytesPerSec = *(DWORD*)&WaveBuf[28]; // byte rate
						WaveFormat.Format.nBlockAlign = *(WORD*)&WaveBuf[32];
						wBitsPerSample = *(WORD*)&WaveBuf[34];
						WaveFormat.Format.wBitsPerSample = wBitsPerSample;
						if (WaveFormat.Format.wFormatTag == WAVE_FORMAT_EXTENSIBLE) {
							WaveFormat.Format.cbSize = 22; // sizeof(WAVEFORMATEXTENSIBLE);
							WaveFormat.Samples.wValidBitsPerSample = WaveFormat.Format.wBitsPerSample;
							WaveFormat.dwChannelMask = SPEAKER_FRONT_LEFT | SPEAKER_FRONT_RIGHT;
							// the following should be WaveFormat.Subformat = KSDATAFORMAT_SUBTYPE_PCM defined as 00000001-0000-0010-8000-00aa00389b71 (but that doesn't work here)
							WaveFormat.SubFormat.Data1 = 1;
							WaveFormat.SubFormat.Data2 = 0;
							WaveFormat.SubFormat.Data3 = 0x10;
							WaveFormat.SubFormat.Data4[0] = 0x80;
							WaveFormat.SubFormat.Data4[1] = 0;
							WaveFormat.SubFormat.Data4[2] = 0;
							WaveFormat.SubFormat.Data4[3] = 0xAA;
							WaveFormat.SubFormat.Data4[4] = 0;
							WaveFormat.SubFormat.Data4[5] = 0x38;
							WaveFormat.SubFormat.Data4[6] = 0x9B;
							WaveFormat.SubFormat.Data4[7] = 0x71;
						}
						else
							WaveFormat.Format.cbSize = 0;
						waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat.Format, (DWORD)&waveOutProc, 0, CALLBACK_FUNCTION);
						WaveOutHdr.lpData = (LPSTR)&WaveBuf[nextchunk+8];
						WaveOutHdr.dwBufferLength = *(DWORD*)&WaveBuf[nextchunk+4];
						WaveOutHdr.dwBytesRecorded = 0;
						WaveOutHdr.dwUser = 0;
						WaveOutHdr.dwFlags = 0;
						WaveOutHdr.dwLoops = 0;
						waveOutPrepareHeader(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
						waveOutWrite(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));

						dMultiplier = (double)meterRect.bottom / 32768.0;
						wave0 = nextchunk+8;
						wavePtr = wave0;
						nSamplesPer40MilliSeconds = nSamplesPerSec / 25;
						BytesPer40MilliSeconds = nSamplesPer40MilliSeconds*WaveFormat.Format.nBlockAlign;
						timeBeginPeriod(TIMER_RESOLUTION);
						if (wBitsPerSample != 24)
							uTimerElevenID = timeSetEvent(40, TIMER_RESOLUTION, TimerFunc11, 0, TIME_PERIODIC);// 1000/25 = 40 to show Hz meter
						else // if (wBitsPerSample == 24)
							uTimerElevenID = timeSetEvent(40, TIMER_RESOLUTION, TimerFunc12, 0, TIME_PERIODIC);// 1000/25 = 40 to show Hz meter
						ModifyMenu(hMenu, PLAY, MF_BYCOMMAND|MF_STRING, PLAY, Stop2);
						DrawMenuBar(hwnd);
						InvalidateRect(hwnd, &rect, FALSE);

						ShowQuality();
					}// end of if (*(DWORD*)&WaveBuf[nextchunk] == 0x61746164) {// "data"
					else {
						recordingtowave = FALSE;
						playing2wavefiles = FALSE;
						MessageBox(hwnd, "Can't play Wave file\n", "", MB_OK);
					}
				}// end of if (*(WORD*)&WaveBuf[20] == 1) {// PCM type
				else {
					recordingtowave = FALSE;
					playing2wavefiles = FALSE;
					MessageBox(hwnd, "This program can't play\nthat kind of Wave file.", "", MB_OK);
				}
			}//if (*(DWORD*)&WaveBuf[8] == 0x45564157) {// "WAVE"
			else {
				recordingtowave = FALSE;
				playing2wavefiles = FALSE;
				MessageBox(hwnd, "That's not a Wave file.", "Oops", MB_OK);
			}
		}// end of if (fileSize = GetFileSize(hFile, NULL))
		else {
			recordingtowave = FALSE;
			playing2wavefiles = FALSE;
			CloseHandle(hFile);
			MessageBox(hwnd, "That file is empty", ERROR, MB_OK);
		}
	}// end of if (hFile != INVALID_HANDLE_VALUE)
	else {
		recordingtowave = FALSE;
		playing2wavefiles = FALSE;
		MessageBox(hwnd, FullFilename2, "Unable to open", MB_OK);
	}
}

//MIXERLINE_COMPONENTTYPE_DST_UNDEFINED = 0 Audio line is a destination that cannot be defined by one of the standard component types. A mixer device is required to use this component type for line component types that have not been defined by Microsoft Corporation.
//MIXERLINE_COMPONENTTYPE_DST_DIGITAL = 1 Audio line is a digital destination (for example, digital input to a DAT or CD audio device).
//MIXERLINE_COMPONENTTYPE_DST_LINE = 2 Audio line is a line level destination (for example, line level input from a CD audio device) that will be the final recording source for the analog-to-digital converter (ADC). Because most audio cards for personal computers provide some sort of gain for the recording audio source line, the mixer device will use the MIXERLINE_COMPONENTTYPE_DST_WAVEIN type.
//MIXERLINE_COMPONENTTYPE_DST_MONITOR = 3 Audio line is a destination used for a monitor.
//MIXERLINE_COMPONENTTYPE_DST_SPEAKERS = 4 Audio line is an adjustable (gain and/or attenuation) destination intended to drive speakers. This is the typical component type for the audio output of audio cards for personal computers.
//MIXERLINE_COMPONENTTYPE_DST_HEADPHONES = 5 Audio line is an adjustable (gain and/or attenuation) destination intended to drive headphones. Most audio cards use the same audio destination line for speakers and headphones, in which case the mixer device simply uses the MIXERLINE_COMPONENTTYPE_DST_SPEAKERS type.
//MIXERLINE_COMPONENTTYPE_DST_TELEPHONE = 6 Audio line is a destination that will be routed to a telephone line.
//MIXERLINE_COMPONENTTYPE_DST_WAVEIN = 7 Audio line is a destination that will be the final recording source for the waveform-audio input (ADC). This line typically provides some sort of gain or attenuation. This is the typical component type for the recording line of most audio cards for personal computers.
//MIXERLINE_COMPONENTTYPE_DST_VOICEIN = 8 Audio line is a destination that will be the final recording source for voice input. This component type is exactly like MIXERLINE_COMPONENTTYPE_DST_WAVEIN but is intended specifically for settings used during voice recording/recognition. Support for this line is optional for a mixer device. Many mixer devices provide only MIXERLINE_COMPONENTTYPE_DST_WAVEIN.

//MIXERLINE_COMPONENTTYPE_SRC_UNDEFINED = 4096 Audio line is a source that cannot be defined by one of the standard component types. A mixer device is required to use this component type for line component types that have not been defined by Microsoft Corporation. 
//MIXERLINE_COMPONENTTYPE_SRC_DIGITAL = 4097 Audio line is a digital source (for example, digital output from a DAT or audio CD). 
//MIXERLINE_COMPONENTTYPE_SRC_LINE = 4098 Audio line is a line-level source (for example, line-level input from an external stereo) that can be used as an optional recording source. Because most audio cards for personal computers provide some sort of gain for the recording source line, the mixer device will use the MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY type. 
//MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE = 4099 Audio line is a microphone recording source. Most audio cards for personal computers provide at least two types of recording sources: an auxiliary audio line and microphone input. A microphone audio line typically provides some sort of gain. Audio cards that use a single input for use with a microphone or auxiliary audio line should use the MIXERLINE_COMPONENTTYPE_SRC_MICROPHONE component type. 
//MIXERLINE_COMPONENTTYPE_SRC_SYNTHESIZER = 4100 Audio line is a source originating from the output of an internal synthesizer. Most audio cards for personal computers provide some sort of MIDI synthesizer (for example, an Adlib®-compatible or OPL/3 FM synthesizer). 
//MIXERLINE_COMPONENTTYPE_SRC_COMPACTDISC = 4101 Audio line is a source originating from the output of an internal audio CD. This component type is provided for audio cards that provide an audio source line intended to be connected to an audio CD (or CD-ROM playing an audio CD). 
//MIXERLINE_COMPONENTTYPE_SRC_TELEPHONE = 4102 Audio line is a source originating from an incoming telephone line. 
//MIXERLINE_COMPONENTTYPE_SRC_PCSPEAKER = 4103 Audio line is a source originating from personal computer speaker. Several audio cards for personal computers provide the ability to mix what would typically be played on the internal speaker with the output of an audio card. Some audio cards support the ability to use this output as a recording source. 
//MIXERLINE_COMPONENTTYPE_SRC_WAVEOUT = 4104 Audio line is a source originating from the waveform-audio output digital-to-analog converter (DAC). Most audio cards for personal computers provide this component type as a source to the MIXERLINE_COMPONENTTYPE_DST_SPEAKERS destination. Some cards also allow this source to be routed to the MIXERLINE_COMPONENTTYPE_DST_WAVEIN destination.
//MIXERLINE_COMPONENTTYPE_SRC_AUXILIARY = 4105 Audio line is a source originating from the auxiliary audio line. This line type is intended as a source with gain or attenuation that can be routed to the MIXERLINE_COMPONENTTYPE_DST_SPEAKERS destination and/or recorded from the MIXERLINE_COMPONENTTYPE_DST_WAVEIN destination. 
//MIXERLINE_COMPONENTTYPE_SRC_ANALOG = 4106 Audio line is an analog source (for example, analog output from a video-cassette tape). 

//MIXERR_INVALCONTROL 401 The control reference is invalid. 
//MIXERR_INVALLINE 400 The audio line reference is invalid. 
//MSYSERR_BADDEVICEID 2 The hmxobj parameter specifies an invalid device identifier. 
//MMSYSERR_INVALFLAG 10 One or more flags are invalid. 
//MMSYSERR_INVALHANDLE 5 The hmxobj parameter specifies an invalid handle. 
//MMSYSERR_INVALPARAM 11 One or more parameters are invalid. 
//MMSYSERR_NODRIVER 6 No mixer device is available for the object specified by hmxobj. 

//0x00000000 = MIXERCONTROL_CT_CLASS_CUSTOM
//0x00000000 = MIXERCONTROL_CONTROLTYPE_CUSTOM
//0x50000000 = MIXERCONTROL_CT_CLASS_FADER
//0x50030002 = MIXERCONTROL_CONTROLTYPE_BASS
//0x50030004 = MIXERCONTROL_CONTROLTYPE_EQUALIZER
//0x50030000 = MIXERCONTROL_CONTROLTYPE_FADER 
//0x50030003 = MIXERCONTROL_CONTROLTYPE_TREBLE
//0x50030001 = MIXERCONTROL_CONTROLTYPE_VOLUME
//0x70000000 = MIXERCONTROL_CT_CLASS_LIST 
//0x71010001 = MIXERCONTROL_CONTROLTYPE_MIXER
//0x71010000 = MIXERCONTROL_CONTROLTYPE_MULTIPLESELECT
//0x70010001 = MIXERCONTROL_CONTROLTYPE_MUX
//0x70010000 = MIXERCONTROL_CONTROLTYPE_SINGLESELECT
//0x10000000 = MIXERCONTROL_CT_CLASS_METER ;
//0x10010000 = MIXERCONTROL_CONTROLTYPE_BOOLEANMETER;
//0x10020001 = MIXERCONTROL_CONTROLTYPE_PEAKMETER;
//0x10020000 = MIXERCONTROL_CONTROLTYPE_SIGNEDMETER;
//0x10030000 = MIXERCONTROL_CONTROLTYPE_UNSIGNEDMETER ;
//0x30000000 = MIXERCONTROL_CT_CLASS_NUMBER ;
//0x30040000 = MIXERCONTROL_CONTROLTYPE_DECIBELS;
//0x30050000 = MIXERCONTROL_CONTROLTYPE_PERCENT;
//0x30020000 = MIXERCONTROL_CONTROLTYPE_SIGNED;
//0x30030000 = MIXERCONTROL_CONTROLTYPE_UNSIGNED ;
//0x40000000 = MIXERCONTROL_CT_CLASS_SLIDER ;
//0x40020001 = MIXERCONTROL_CONTROLTYPE_PAN;
//0x40020002 = MIXERCONTROL_CONTROLTYPE_QSOUNDPAN;
//0x40020000 = MIXERCONTROL_CONTROLTYPE_SLIDER ;
//0x20000000 = MIXERCONTROL_CT_CLASS_SWITCH ;
//0x20010000 = MIXERCONTROL_CONTROLTYPE_BOOLEAN;
//0x21010000 = MIXERCONTROL_CONTROLTYPE_BUTTON;
//0x20010004 = MIXERCONTROL_CONTROLTYPE_LOUDNESS;
//0x20010003 = MIXERCONTROL_CONTROLTYPE_MONO;
//0x20010002 = MIXERCONTROL_CONTROLTYPE_MUTE;
//0x20010001 = MIXERCONTROL_CONTROLTYPE_ONOFF;
//0x20010005 = MIXERCONTROL_CONTROLTYPE_STEREOENH ;
//0x60000000 = MIXERCONTROL_CT_CLASS_TIME ;
//0x60030000 = MIXERCONTROL_CONTROLTYPE_MICROTIME;
//0x61030000 = MIXERCONTROL_CONTROLTYPE_MILLITIME ;

LRESULT CALLBACK SoundProc(HWND hwndSound, UINT message, WPARAM wParam, LPARAM lParam)
{
	int x, y, z;
	double d, d2, d3;

	if (message == MM_MIXM_CONTROL_CHANGE) {
		if (fromkeyboard == FALSE) {
			mixerLine.cbStruct = sizeof(MIXERLINE);
			mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
			MixerError = mixerGetLineInfo((HMIXEROBJ)hMixer, &mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE);
			if (!MixerError) {
				mixerLineControls.cbStruct = sizeof(MIXERLINECONTROLS);
				mixerLineControls.dwLineID = mixerLine.dwLineID;
				mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
				mixerLineControls.cControls = 1;
				mixerLineControls.cbmxctrl = sizeof(MIXERCONTROL);
				mixerLineControls.pamxctrl = &mixerControl;
				MixerError = mixerGetLineControls((HMIXEROBJ)hMixer, &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
				if (!MixerError) {
					mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
					mixerControlDetails.dwControlID = (DWORD)mixerControl.dwControlID;
					mixerControlDetails.cChannels = 1;// mixerLine.cChannels;
					mixerControlDetails.cMultipleItems = 0;
					mute = FALSE;
					mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
					mixerControlDetails.paDetails = &mixerControlDetailsMasterMute;// for WM_PAINT to show "Muted" or not
					MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE);
					if (!MixerError) {
						mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
						MixerError = mixerGetLineControls((HMIXEROBJ)hMixer, &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE);
						if (!MixerError) {
							mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
							mixerControlDetails.dwControlID = (DWORD)mixerControl.dwControlID;
							mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
							mixerControlDetails.paDetails = &mixerControlDetailsVolume;
							MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer, &mixerControlDetails, MIXER_GETCONTROLDETAILSF_VALUE);
							if (!MixerError) {
								d = mixerControlDetailsVolume.dwValue / 655.35;// percentage of max value 0xFFFF
								d2 = modf(d, &d3);
								x = (int)d3;
								if (d2 > 0.50)
									x++;
								if (x < 100) {
									MasterVolume[4] = ' ';
									MasterVolume[5] = (x / 10) + '0';
									MasterVolume[6] = (x % 10) + '0';
								}
								else {
									MasterVolume[4] = '1';
									MasterVolume[5] = '0';
									MasterVolume[6] = '0';
								}
							}
						}
					}
				}
			}
		}// end of if (fromkeyboard == FALSE)
		else {
			fromkeyboard = FALSE;
			MasterVolume[6] = '0';
		}
		InvalidateRect(hwnd, &rect, FALSE);

		if ((wavemixer) && (!fromwavemixer)) {
			///////
			FillMixerBuf();
			///////
			for (x = 0, y = 0, z = 8; x < (int)Entries; x++) {// ShowSliders
				if ((MixerBuf[x].mixer[0]) && (MixerBuf[x].type[0])) {
					if ((MixerBuf[x].RecordPlay == 'P') && (0 == strcmp(MixerBuf[x].mixercaps, Playback[indexPlay]))) {
						SendMessage(hSlider[y], TBM_SETPOS, 1, 100-MixerBuf[x].Volume);
						Slider[y].hSlider = hSlider[y];
						Slider[y].mixer = x;
						SetWindowText(hEdit[y], MixerBuf[x].type);
						SetWindowText(hStatic[y], MixerBuf[x].volume);
						if (MixerBuf[x].muteselected)
							CheckDlgButton(hwndMixer, checks[y], BST_CHECKED);
						else
							CheckDlgButton(hwndMixer, checks[y], BST_UNCHECKED);
						y++;
						if (MixerBuf[x].basstreble) {
							SendMessage(hBassSlider, TBM_SETPOS, 1, 100-MixerBuf[x].bass);
							SendMessage(hTrebleSlider, TBM_SETPOS, 1, 100-MixerBuf[x].treble);
						}
					}
					if ((MixerBuf[x].RecordPlay == 'R') && (0 == strcmp(MixerBuf[x].mixercaps, Record[indexRecord]))) {
						SendMessage(hSlider[z], TBM_SETPOS, 1, 100-MixerBuf[x].Volume);
						Slider[z].hSlider = hSlider[z];
						Slider[z].mixer = x;
						SetWindowText(hEdit[z], MixerBuf[x].type);
						SetWindowText(hStatic[z], MixerBuf[x].volume);
						if (0 == strcmp(Selected, MixerBuf[x].type))
							CheckDlgButton(hwndMixer, checks[z], BST_CHECKED);
						else
							CheckDlgButton(hwndMixer, checks[z], BST_UNCHECKED);
						z++;
					}
				}
			}
		}
	}
	return DefWindowProc(hwndSound, message, wParam, lParam);
}

void SetMasterVolume(void)
{
	mixerLine.cbStruct = sizeof(MIXERLINE);
	mixerLine.dwComponentType = MIXERLINE_COMPONENTTYPE_DST_SPEAKERS;
	mixerGetLineInfo((HMIXEROBJ)hMixer, &mixerLine, MIXER_GETLINEINFOF_COMPONENTTYPE);

	mixerLineControls.cbStruct = sizeof(MIXERLINECONTROLS);
	mixerLineControls.dwLineID = mixerLine.dwLineID;
	if (mute)
		mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_MUTE;
	else
		mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
	mixerLineControls.cControls = 1;
	mixerLineControls.cbmxctrl = sizeof(MIXERCONTROL);
	mixerLineControls.pamxctrl = &mixerControl;
	mixerGetLineControls((HMIXEROBJ)hMixer, &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE);

	mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
	mixerControlDetails.dwControlID = (DWORD)mixerControl.dwControlID;
	mixerControlDetails.cChannels = 1;// mixerLine.cChannels;
	mixerControlDetails.cMultipleItems = 0;
	if (mute) {
		mute = FALSE;
		mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
		mixerControlDetails.paDetails = &mixerControlDetailsMasterMute;
	}
	else {
		mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
		mixerControlDetails.paDetails = &mixerControlDetailsVolume;
	}
	/////
	mixerSetControlDetails((HMIXEROBJ)hMixer, &mixerControlDetails, MIXER_SETCONTROLDETAILSF_VALUE);
	/////
}

void FillMixerBuf(void)
{
	Entries = play = record = m = 0;
	NumOfMixers = mixerGetNumDevs();
	for (mixerNumber = 0; mixerNumber < NumOfMixers; mixerNumber++) {
		mixerOpen(&hMixer2, mixerNumber, 0, 0, 0);
		mixerGetDevCaps((UINT)hMixer2, &mixerCaps, sizeof(MIXERCAPS));
		for (Destination = 0; Destination < mixerCaps.cDestinations; Destination++) {
			mixerLine2.cbStruct = sizeof(MIXERLINE);
			mixerLine2.dwDestination = Destination;
			MixerError = mixerGetLineInfo((HMIXEROBJ)hMixer2, &mixerLine2, MIXER_GETLINEINFOF_DESTINATION);
			if ((!MixerError) && (mixerLine2.cControls)) {
				if (mixerLine2.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_SPEAKERS) {
					RecordPlay = 'P';
					strcpy(Playback[play++],mixerCaps.szPname);
				}
				else if (mixerLine2.dwComponentType == MIXERLINE_COMPONENTTYPE_DST_WAVEIN) {
					RecordPlay = 'R';
					strcpy(Record[record++],mixerCaps.szPname);
				}
				else
					RecordPlay = ' ';
				mixerLineControls2.cbStruct = sizeof(MIXERLINECONTROLS);
				mixerLineControls2.dwLineID = mixerLine2.dwLineID;
				mixerLineControls2.cControls = mixerLine2.cControls;
				mixerLineControls2.cbmxctrl = sizeof(mixerControl2[0]);
				mixerLineControls2.pamxctrl = mixerControl2;
				MixerError = mixerGetLineControls((HMIXEROBJ)hMixer2, &mixerLineControls2, MIXER_GETLINECONTROLSF_ALL);
				if (!MixerError) {
					for (control = 0; control < mixerLineControls2.cControls; control++) {
						mixerControlDetails2.cbStruct = sizeof(MIXERCONTROLDETAILS);
						mixerControlDetails2.cMultipleItems = mixerControl2[control].cMultipleItems;
						mixerControlDetails2.dwControlID = (DWORD)mixerControl2[control].dwControlID;
						mixerControlDetails2.cChannels = 1;// mixerLine2.cChannels;
						if (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME) {
							mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
							mixerControlDetails2.paDetails = &mixerControlDetailsVolume2;
							MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_VALUE);
							if (!MixerError) {
								d = mixerControlDetailsVolume2.dwValue / 655.35;// percentage of max value 0xFFFF
								d2 = modf(d, &d3);
								WaveVolume = (int)d3;
								if (d2 > 0.50)
									WaveVolume++;
								_itoa(WaveVolume, temp, 10);
								strcpy(MixerBuf[Entries].volume, temp);
								strcpy(MixerBuf[Entries].type, mixerLine2.szName);
								strcpy(MixerBuf[Entries].mixer, mixerLine2.Target.szPname);
								strcpy(MixerBuf[Entries].mixercaps, mixerCaps.szPname);
								MixerBuf[Entries].dwControlID = mixerControlDetails2.dwControlID;
								MixerBuf[Entries].mixerNumber = mixerNumber;
								MixerBuf[Entries].RecordPlay = RecordPlay;
								MixerBuf[Entries].Volume = WaveVolume;
							}
						}
//////// select
						else if ((mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MUX) || (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MIXER)) {
							pList = (MIXERCONTROLDETAILS_LISTTEXT*)malloc(mixerControlDetails2.cMultipleItems * sizeof(MIXERCONTROLDETAILS_LISTTEXT));
							mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_LISTTEXT);
							mixerControlDetails2.paDetails = pList;
							MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_LISTTEXT);
							if (!MixerError) {
								pSelected = (MIXERCONTROLDETAILS_UNSIGNED*)malloc(mixerControlDetails2.cMultipleItems * sizeof(MIXERCONTROLDETAILS_UNSIGNED));
								mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
								mixerControlDetails2.paDetails = pSelected;// look at pSelected[0], pSelected[1], etc
								MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_VALUE);
								for (i = 0; i < mixerControlDetails2.cMultipleItems; i++) {
									if (pSelected[i].dwValue) {
										strcpy(Selected, pList[i].szName);
										MixerBuf[Entries].muteselected = 1;
									}
								}
								free(pSelected);
							}
							free(pList);
						}
//////// mute
						else if (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE) {
							mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
							mixerControlDetails2.paDetails = &mixerControlDetailsMute;
							MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_VALUE);
							if (!MixerError)
								MixerBuf[Entries].muteselected = (BYTE)mixerControlDetailsMute.fValue;
						}
//////// bass & treble
						else if (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_BASS) {
							mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
							mixerControlDetails2.paDetails = &mixerControlDetailsOther;
							MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_VALUE);
							if (!MixerError) {
								MixerBuf[Entries].basstreble = TRUE;
								d = mixerControlDetailsOther.dwValue / 655.35;// percentage of max value 0xFFFF
								d2 = modf(d, &d3);
								BassOrTreble = (BYTE)d3;
								if (d2 > 0.50)
									BassOrTreble++;
								MixerBuf[Entries].bass = BassOrTreble;
							}
						}
						else if (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_TREBLE) {
							mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
							mixerControlDetails2.paDetails = &mixerControlDetailsOther;
							MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_VALUE);
							if (!MixerError) {
								MixerBuf[Entries].basstreble = TRUE;
								d = mixerControlDetailsOther.dwValue / 655.35;// percentage of max value 0xFFFF
								d2 = modf(d, &d3);
								BassOrTreble = (BYTE)d3;
								if (d2 > 0.50)
									BassOrTreble++;
								MixerBuf[Entries].treble = BassOrTreble;
							}
						}
					}// end of for (control = 0; control < mixerLineControls2.cControls; control++)
					Entries++;
				}// end of if (!MixerError)
				for (i = 0; i < mixerLine2.cConnections; i++) {
					mixerLine3.cbStruct = sizeof(MIXERLINE);
					mixerLine3.dwDestination = Destination;
					mixerLine3.dwSource = i;
					MixerError = mixerGetLineInfo((HMIXEROBJ)hMixer2, &mixerLine3, MIXER_GETLINEINFOF_SOURCE);
					if ((!MixerError) && (mixerLine3.cControls)) {
						mixerLineControls2.cbStruct = sizeof(MIXERLINECONTROLS);
						mixerLineControls2.dwLineID = mixerLine3.dwLineID;
						mixerLineControls2.cControls = mixerLine3.cControls;
						mixerLineControls2.cbmxctrl = sizeof(mixerControl2[0]);
						mixerLineControls2.pamxctrl = mixerControl2;
						MixerError = mixerGetLineControls((HMIXEROBJ)hMixer2, &mixerLineControls2, MIXER_GETLINECONTROLSF_ALL);
						if (!MixerError) {
							for (control = 0; control < mixerLineControls2.cControls; control++) {
								mixerControlDetails2.cbStruct = sizeof(MIXERCONTROLDETAILS);
								mixerControlDetails2.cMultipleItems = mixerControl2[control].cMultipleItems;
								mixerControlDetails2.dwControlID = (DWORD)mixerControl2[control].dwControlID;
								mixerControlDetails2.cChannels = 1;// mixerLine3.cChannels;
								if (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_VOLUME) {
									mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
									mixerControlDetails2.paDetails = &mixerControlDetailsVolume2;
									MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_VALUE);
									if (!MixerError) {
										d = mixerControlDetailsVolume2.dwValue / 655.35;// percentage of max value 0xFFFF
										d2 = modf(d, &d3);
										WaveVolume = (int)d3;
										if (d2 > 0.50)
											WaveVolume++;
										_itoa(WaveVolume, temp, 10);
										strcpy(MixerBuf[Entries].volume, temp);
										strcpy(MixerBuf[Entries].type, mixerLine3.szName);
										strcpy(MixerBuf[Entries].mixer, mixerLine3.Target.szPname);
										strcpy(MixerBuf[Entries].mixercaps, mixerCaps.szPname);
										MixerBuf[Entries].dwControlID = mixerControlDetails2.dwControlID;
										MixerBuf[Entries].mixerNumber = mixerNumber;
										MixerBuf[Entries].RecordPlay = RecordPlay;
										MixerBuf[Entries].Volume = WaveVolume;
									}
								}
//////// mute
								else if (mixerControl2[control].dwControlType == MIXERCONTROL_CONTROLTYPE_MUTE) {
									mixerControlDetails2.cbDetails = sizeof(MIXERCONTROLDETAILS_BOOLEAN);
									mixerControlDetails2.paDetails = &mixerControlDetailsMute;
									MixerError = mixerGetControlDetails((HMIXEROBJ)hMixer2, &mixerControlDetails2, MIXER_GETCONTROLDETAILSF_VALUE);
									if (!MixerError) {
										MixerBuf[Entries].muteselected = (BYTE)mixerControlDetailsMute.fValue;
										MixerBuf[Entries].muteDestination = Destination;
									}
								}
							}
							Entries++;
						}
					}
				}// end of for (i = 0; i < mixerLine2.cConnections; i++)
			}
		}
		mixerClose(hMixer2);
	}
}

void ClearMeter(void)
{
	HDC hdc;

	hdc = GetDC(hwndMeter);
	hOldPen = SelectObject(hdc, hDialogPen);
	MoveToEx(hdc, 2, meterRect.bottom, NULL);
	LineTo(hdc, 2, meterRect.top);
	MoveToEx(hdc, 3, meterRect.bottom, NULL);
	LineTo(hdc, 3, meterRect.top);
	MoveToEx(hdc, 4, meterRect.bottom, NULL);
	LineTo(hdc, 4, meterRect.top);
	MoveToEx(hdc, 5, meterRect.bottom, NULL);
	LineTo(hdc, 5, meterRect.top);
	MoveToEx(hdc, 6, meterRect.bottom, NULL);
	LineTo(hdc, 6, meterRect.top);
	SelectObject(hdc, hOldPen);
	ReleaseDC(hwndMeter, hdc);
}

void GetWaveQuality(void)
{
	switch (WaveOptionsIndex)
	{
	case 0:
		nSamplesPerSec = 8000;
		nRecordChannels = 1;
		wBitsPerSample = 16;
		break;
	case 1:
		nSamplesPerSec = 11025;
		nRecordChannels = 1;
		wBitsPerSample = 16;
		break;
	case 2:
		nSamplesPerSec = 11025;
		nRecordChannels = 2;
		wBitsPerSample = 16;
		break;
	case 3:
		nSamplesPerSec = 22050;
		nRecordChannels = 1;
		wBitsPerSample = 16;
		break;
	case 4:
		nSamplesPerSec = 22050;
		nRecordChannels = 2;
		wBitsPerSample = 16;
		break;
	case 5:
		nSamplesPerSec = 44100;
		nRecordChannels = 1;
		wBitsPerSample = 16;
		break;
	case 6:
		nSamplesPerSec = 44100;
		nRecordChannels = 2;
		wBitsPerSample = 16;
		break;
	case 7:
		nSamplesPerSec = 44100;
		nRecordChannels = 1;
		wBitsPerSample = 24;
		break;
	case 8:
		nSamplesPerSec = 44100;
		nRecordChannels = 2;
		wBitsPerSample = 24;
		break;
	case 9:
		nSamplesPerSec = 48000;
		nRecordChannels = 1;
		wBitsPerSample = 24;
		break;
	case 10:
		nSamplesPerSec = 48000;
		nRecordChannels = 2;
		wBitsPerSample = 24;
		break;
	}
}

void ShowQuality(void)
{ // needs: nSamplesPerSec wBitsPerSample nRecordChannels
	_itoa(nSamplesPerSec, temp, 10);
	for (x = 0; temp[x]; x++)
		;
	strcat(&temp[x], " Hz");
	SetDlgItemText(hwndMixer, IDC_STATIC18, temp);
	if (wBitsPerSample == 24)
		SetDlgItemText(hwndMixer, IDC_STATIC19, "24 bits");
	else if (wBitsPerSample == 16)
		SetDlgItemText(hwndMixer, IDC_STATIC19, "16 bits");
	else if (wBitsPerSample == 8)
		SetDlgItemText(hwndMixer, IDC_STATIC19, "8 bits");
	if (nRecordChannels == 1)
		SetDlgItemText(hwndMixer, IDC_STATIC21, "Mono");
	else
		SetDlgItemText(hwndMixer, IDC_STATIC21, "Stereo");
}

void ShowUnplayedNote(int y)
{
	PreviousWrongEvent = y;
	Event[y].time = 1;// flag to show played
	if (Event[y].pixel >= (Lines[L].pixel + Lines[L].PixelsPerLine)) {
		L++;
		Row++;
	}
	XLoc = (WORD)(Event[y].pixel - Lines[L].pixel);
	YLoc = NoteLoc[Event[y].note-21] + (Row*BottomNoteTop);
	if ((!Letter[Event[y].note-21]) && (usingsharp == FALSE))
		YLoc -= 6;
	hdc = GetDC(hwnd);
	if (!onlynotenames)
		hOldBrush = SelectObject(hdc, hDarkGreyBrush);
	else
		hOldBrush = SelectObject(hdc, GetStockObject(NULL_BRUSH));
	Rectangle(hdc, XLoc, YLoc, XLoc+(Event[y].ticksinnote*40/TicksPerQuarterNote), YLoc+7);
	SelectObject(hdc, hOldBrush);
	ReleaseDC(hwnd, hdc);
}

void CheckForNewInstrument(void)
{
	for (x = 0, y = 0, z = 0; x < 16; x++) {
		tempBuf[x] = 0xFF;
		if ((ChannelInstruments[x][0] != 0xFF) && ((x != 9) || (EWQL))) // don't include percussion
			tempBuf[z++] = (BYTE)x;
		if ((ActiveChannels[x]) && ((x != 9) || (EWQL))) { // don't include percussion
			y++;
			OnlyActiveChannel = x;
		}
	}
	if ((y == 1) && (z > 1)) {// only one instrument show & play and others exist
		if (IDYES == MessageBox(hwnd, "Change the Instrument\nto another Instrument\nwhen it's copied?", "", MB_YESNO)) {
			for (x = 0, y = 0; x < 16; x++) {
				if (ChannelInstruments[x][0] != 0xFF) {
					DialogBox(hInst, "NEWCHANNEL", hwnd, NewChannelProc);
					break;
				}
			}
		}
	}
}

void FillLines()
{
	for (x = 0; x < (int)e; x++) {
		if (Event[x].BeatsPerMeasure) {
			BeatsPerMeasure = Event[x].BeatsPerMeasure;
			BeatNoteType = Event[x].BeatNoteType;
			switch (Event[x].BeatNoteType)
			{
			case 32:
				PixelsPerBeat = 5;
				break;
			case 16:
				PixelsPerBeat = 10;
				break;
			case 8:
				PixelsPerBeat = 20;
				break;
			case 4:
				PixelsPerBeat = 40;
				break;
			case 2:
				PixelsPerBeat = 80;
				break;
			case 1:
				PixelsPerBeat = 160;
				break;
			default:
				PixelsPerBeat = 40;
			}			
			PixelsPerMeasure = BeatsPerMeasure * PixelsPerBeat;

			for (l = 0; Lines[l].pixel < Event[x].pixel; l++)
				;
			if (Lines[l].pixel != Event[x].pixel)
				l--;
			for (y = 0, LinePixels = 0; (Lines[l].pixel + LinePixels) < Event[x].pixel; LinePixels += Lines[l].PixelsPerMeasure[y], y++)
				;// put Lines[l].pixel + LinePixels at Event[x].pixel
			for ( ; LinePixels <= (rect.right-PixelsPerMeasure); y++) {
				Lines[l].PixelsPerBeat[y] = PixelsPerBeat;
				Lines[l].BeatsPerMeasure[y] = BeatsPerMeasure;
				Lines[l].PixelsPerMeasure[y] = PixelsPerMeasure;
				LinePixels += PixelsPerMeasure;
			}
			Lines[l].PixelsPerLine = LinePixels;
			Lines[l].rowonpage = (l % Rows) * PixelsInGrandStaffAndMyExtraSpace;
			for (ThisLinePixels = 0; ThisLinePixels <= (rect.right-PixelsPerMeasure); ThisLinePixels += PixelsPerMeasure)
				;
			for (l++; l < 300; l++) {//put latest beat & beatnote in rest of Lines
				Lines[l].pixel = Lines[l-1].pixel + Lines[l-1].PixelsPerLine;
				Lines[l].PixelsPerLine = ThisLinePixels;
				Lines[l].rowonpage = (l % Rows) * PixelsInGrandStaffAndMyExtraSpace;
				for (y = 0; y < 64; y++) {
					Lines[l].PixelsPerBeat[y] =	PixelsPerBeat;
					Lines[l].BeatsPerMeasure[y] = BeatsPerMeasure;
					Lines[l].PixelsPerMeasure[y] = PixelsPerMeasure;
				}
			}
			for (l = 0; l < 300; ) {
				PixelsPerPage = 0;
				if (Lines[l].rowonpage == 0) {
					PixelsPerPage += Lines[l].PixelsPerLine;
					for (z = l+1; Lines[z].rowonpage != 0; z++)
						PixelsPerPage += Lines[z].PixelsPerLine;
					Lines[l].PixelsPerPage = PixelsPerPage;
					for (l++; Lines[l].rowonpage != 0; l++)
						Lines[l].PixelsPerPage = PixelsPerPage;
				}
			}
		}// end of if (Event[x].BeatsPerMeasure)

		else if (Event[x].KeySignature) {
			KeySignature = Event[x].KeySignature;
			if (firstkeysignature) {
				firstkeysignature = FALSE;
				if ((KeySignature >= 250) && (KeySignature <= 255))
					usingsharp = FALSE;
				else if ((KeySignature >= 1) && (KeySignature <= 6))
					usingsharp = TRUE;
			}
			for (l = 0; Lines[l].pixel < Event[x].pixel; l++)
				;
			if (Lines[l].pixel != Event[x].pixel)
				l--;
			for (y = 0, LinePixels = 0; (Lines[l].pixel + LinePixels) < Event[x].pixel; LinePixels += Lines[l].PixelsPerMeasure[y], y++)
				;// put Lines[l].pixel + LinePixels at Event[x].pixel
			for ( ; l < 300; l++, y = 0) {
				for ( ; y < 64; y++) {
					Lines[l].KeySignature[y] = KeySignature;
				}
			}
		}
		else if (Event[x].message == (DWORD)(0x7F40B0 | Event[x].channel)) {// sustain pedal on
			channel = Event[x].channel;
			for (y = x+1; y < (int)e; y++) {
				if (Event[y].message == (DWORD)(0x0040B0 | channel)) // sustain pedal off
					break;
			}
			if (y == (int)e)// sustain on but no sustain off
				continuousustain[channel] = TRUE;
		}
	}// end of for (x = 0; x < (int)e; x++)
	MeasureNumber = 0;
	for (l = 0; l < 255; l++) {
		for (x = 0, y = 0; x < Lines[l].PixelsPerLine; x += Lines[l].PixelsPerMeasure[y], y++)
			;
		MeasureNumber += y;
		Lines[l+1].FirstMeasureNumber = MeasureNumber;
	}
}

void GetPixelsBetweenLines(void)
{
	PixelsInGrandStaff = PixelsBetweenLines * TOTALINES;
	if (rect.bottom >= ((PixelsInGrandStaff * 3) + (24 * 4))) {
		MyExtraSpace = (rect.bottom - PixelsInGrandStaff * 3) / 4;
		Rows = 3;
		Row2Top = PixelsInGrandStaff + (MyExtraSpace*2);
		Row3Top = (PixelsInGrandStaff*2) + (MyExtraSpace*3);
	}
	else if (rect.bottom >= ((PixelsInGrandStaff * 2) + (24 * 3))) {
		MyExtraSpace = (rect.bottom - PixelsInGrandStaff * 2) / 3;
		Rows = 2;
		Row2Top = PixelsInGrandStaff + (MyExtraSpace*2);
	}
	else if (rect.bottom >= (PixelsInGrandStaff + (24 * 2))) {
		MyExtraSpace = (rect.bottom - PixelsInGrandStaff) / 2;
		Rows = 1;
	}
	else if (rect.bottom != 0) { // if not minimized
		MyExtraSpace = 24;
		PixelsBetweenLines = (rect.bottom - (MyExtraSpace * 2)) / TOTALINES;
		Rows = 1;
	}
	Row1Top = MyExtraSpace;
	PixelsInGrandStaffAndMyExtraSpace = PixelsInGrandStaff + MyExtraSpace;
	NoteMiddle = PixelsBetweenLines / 2;
	NoteTop = NoteMiddle + (PixelsBetweenLines/4);
	BottomNoteTop = PixelsInGrandStaffAndMyExtraSpace - NoteTop;
	for (x = 0; x < 88; x++)
		NoteLoc[x] = BottomNoteTop - (UpFrom21[x] * PixelsBetweenLines / 2); // NoteLoc changes with Spaces Between Staff Lines

	SharpSigs[0] = PixelsBetweenLines * 9;
	SharpSigs[1] = SharpSigs[0] + (PixelsBetweenLines * 3 / 2);
	SharpSigs[2] = SharpSigs[0] - (PixelsBetweenLines / 2);
	SharpSigs[3] = SharpSigs[0] + PixelsBetweenLines;
	SharpSigs[4] = SharpSigs[0] + (PixelsBetweenLines * 5 / 2);
	SharpSigs[5] = SharpSigs[0] + (PixelsBetweenLines / 2);
	FlatSigs[0] = PixelsBetweenLines * 11;
	FlatSigs[1] = FlatSigs[0] - (PixelsBetweenLines * 3 / 2);
	FlatSigs[2] = FlatSigs[0] + (PixelsBetweenLines / 2);
	FlatSigs[3] = FlatSigs[0] - PixelsBetweenLines;
	FlatSigs[4] = FlatSigs[0] + PixelsBetweenLines;
	FlatSigs[5] = FlatSigs[0] - (PixelsBetweenLines / 2);
	///////////
	FillLines();
	///////////
	lf7.lfHeight = 6 + (PixelsBetweenLines / 2);
	hNoteFont = CreateFontIndirect(&lf7);
	hOldFont = SelectObject(hdcMem, hNoteFont);
	SelectObject(hdcMem, hOldFont);
	PedalDownX = -1;
}

void Metronome(void)
{
	if (pianoMetronome == FALSE) {
		pianoMetronome = TRUE;
		timeBeginPeriod(TIMER_RESOLUTION);
		uTimer13ID = timeSetEvent(60000/InitialBeatsPerMinute, TIMER_RESOLUTION, TimerFunc13, 0, TIME_PERIODIC);
	}
	else {
		pianoMetronome = FALSE;
		timeKillEvent(uTimer13ID);
		timeEndPeriod(TIMER_RESOLUTION);
		uTimer13ID = 0;
	}
}
/*
BOOL CheckForPortFile(void)
{ // "Port       "
	for (x = 0; ofn3.lpstrFileTitle[x] == Ports[x]; x++)
		;
	if ((x == 4) && (ofn3.lpstrFileTitle[x] >= '0') && (ofn3.lpstrFileTitle[x] <= '9')) {
		return TRUE;
	}
	else if ((ofn3.lpstrFile[ofn3.nFileOffset] == 'P') && (ofn3.lpstrFile[ofn3.nFileOffset+1] == 'o') && (ofn3.lpstrFile[ofn3.nFileOffset+2] == 'r') && (ofn3.lpstrFile[ofn3.nFileOffset+3] == 't')) {
		x=x;
	}
	else
		return FALSE;	
}
*/
