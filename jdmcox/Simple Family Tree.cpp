/*
C source code compiled with Microsoft's Visual C/C++ 6.0 (with the Feb 2003 MSDN library of API's)
http://jdmcox.com/Simple Family Tree.rc
http://jdmcox.com/Simple Family Tree.h (rename it to resource.h)
http://jdmcox.com/ijl15.dll
http://jdmcox.com/ijl15.zip
add _UNICODE AND UNICODE to Project Settings C/C++ tab Category: General Preprocessor Definitions
when debugging 16-bit char data, instead of looking at &Buf[x] in the Watch window, look at &Buf[x],ma
*/
#include <windows.h>
#include <stdio.h>//for _snwprintf
#include <tchar.h>//for unicode
#include "ijl.h"//put ijl15.lib in Project/Settings/Link
#include "resource.h"
#include <shlobj.h>
#define WIN32_LEAN_AND_MEAN
#define LIGHTGRAY 0xF0F7F8
#define ROWS 100
#define COLS 1500
#define BIGCOLS 16384
#define ROWSUP 21
#define MIDROW 20
#define CHILD 1
#define MAN 2
#define WOMAN 3
#define BAD 4
#define NONE 0
#define BIRTHDAY 1
#define AGE 2
#define PHOTO 3
#define FULL 1
#define HALF 2
#define QUARTER 3
#define EIGHTH 4
#define MAXSPOUSES 6
#define EXTRAINDIVIDUALS 1500
//U-00000000 – U-0000007F: 	0xxxxxxx
//U-00000080 – U-000007FF: 	110xxxxx 10xxxxxx
//U-00000800 – U-0000FFFF: 	1110xxxx 10xxxxxx 10xxxxxx
#define MASKBITS   0x3F//00111111
#define MASKBYTE   0x80//10000000
#define MASK2BYTES 0xC0//11000000
#define MASK3BYTES 0xE0//11100000
#define MASK4BYTES 0xF0//11110000
#define MASK5BYTES 0xF8//11111000
#define MASK6BYTES 0xFC//11111100

BOOL errorinbuf = FALSE, kings, badformat = FALSE, badend, multiplemonitors;
//WINDOWPLACEMENT windowPlacement;
TCHAR MulMonHelp[] = TEXT("Multiple Monitor Help");
TCHAR FixBuf[10000];

struct {
	int fams;
	int year;
} Marriages[7], TempMarriages;

struct {
	int fams;
	TCHAR tempFAMS[64];
} IndivFams[7];

DWORD PhotoID;
char *tempBuf, *bJpeg;
TCHAR *NamePtr;
TCHAR ThisPhotoBuf[512];
char bThisPhotoBuf[512];

TCHAR Head[] = TEXT("\
0 HEAD\r\n\
1 SOUR SIMPLE_FAMILY_TREE\r\n\
2 VERS 1.31\r\n\
1 GEDC\r\n\
2 VERS 5.5\r\n\
2 FORM LINEAGE-LINKED\r\n\
1 CHAR UTF-8\r\n");

TCHAR Trlr[] = TEXT("0 TRLR\r\n");

TCHAR SimpleFamilyTree[] = TEXT("\\Simple Family Tree\\");
TCHAR szAppName[] = TEXT("Simple Family Tree 1.32");
char AppName[] = "Simple Family Tree 1.32";

char Help[] = ("\
Shows the ancestors and descendants\n\
of a highlighted individual.\n\
\n\
To highlight any individual, left-click on him/her.\n\
\n\
To display any individual's info, right-click on him/her.\n\
\n\
Backspace and Space go backward and forward\n\
thru individuals you've highlighted.\n\
\n\
Shortcut keys for Menu items are U, N, I, L, and R.\n\
\n\
To undo the last change you made, select Undo.\n\
When you make any change in Simple Family Tree,\n\
the current Gedcom data file is first backed-up,\n\
and then the change is written to it.\n\
Undo swaps the Gedcom data file and the\n\
back-up file, thus undoing the last change.\n\
\n\
To show or stop showing an individual's Age or\n\
Birthday when you move the mouse over him/her,\n\
press the A or B key.\n\
To show or stop showing which individuals have a\n\
photo linked to them, press the P key.\n\
\n\
Duplicated descendants show a small yellow square.\n\
\n\
If scrolling is possible, a horizontal and/or\n\
vertical scroll line will appear to indicate\n\
location within the family tree.\n\
\n\
To scroll, hold the Left Mouse button down\n\
while moving the Mouse,\n\
or use the mousewheel\n\
(hold the Ctrl key down for left-right),\n\
or press an Arrow Key\n\
(hold it down to scroll faster and faster).\n");

char Help2[] = ("\
Automatically saves changes in a Gedcom file\n\
(a Notepad file with a .ged extension).\n\
Other family tree programs can read that file,\n\
and Simple Family Tree can open and display the\n\
data in a Gedcom file created by other programs.\n\
\n\
Gets its display inspiration from Family Historian\n\
(my favorite full-featured family tree program).\n\
\n\
Saves the highlighted individual's Gedcom number\n\
in Simple Family Tree.ini so it can re-open\n\
at that individual.\n\
\n\
Shows ancestors and descendants of the highlighted\n\
individual (as well as up to 6 of his/her spouses).\n\
The display limits are quite large: 13 ancestor rows,\n\
80 descendant rows, and 1500 individuals in a row.\n\
\n\
The Individual's Info window can show the following:\n\
Alias, Buried, Christened, Education, Event,\n\
Graduation, Married/Divorced, [alternate] Name,\n\
Note, Occupation, and Residence.\n\
\n\
You can enter an alternate Name, info on 4 Marriages,\n\
6 Events, and enter about 6,000 words in a Note.\n\
\n\
Same-sex spouses are allowed, but only one of them\n\
can link to a child (normally an adopted child).\n\
\n\
Works with any language by using Unicode and saving\n\
Gedcom data in UTF-8 format.\n\
\n\
Selecting Edit -Decrease Size will select Small Fonts.\n\
\n\
29 Jan 2011       go to jdmcox.com for updates\n\
Doug Cox   jdmcox@jdmcox.com");

TCHAR NamesDates[] = TEXT("\
A woman's last name should normally be her father's last name.\n\
\n\
Slashes around the last name are necessary so that suffix names\n\
can be entered; e.g. John /Doe/ Jr  or  Maria /Sanchez/ Gomez.\n\
If you don't add the slashes, they're added automatically to the last name.\n\
\n\
To enter é hold the Alt key down while entering 130 on the keyboard\n\
number pad, or 160 for á, 161 for í, 162 for ó, 163 for ú, or 164 for ñ.\n\
\n\
Dates in family trees are usually entered in this format: 1 Jan 1999.\n\
\n\
To differentiate between a legal marriage and a partnership,\n\
you could indicate it in Marriages (by an entry in Date or Place).");

TCHAR PhotoHelp[] = TEXT("\n\
The photo shown is reduced by 1/2, 1/4, or 1/8 to fit on the screen.\n\
\n\
When selecting a photo, remember that the file directory dialog box\n\
can easily be changed to show thumbnails, and can be enlarged.\n\
\n\
The link to an individual's photo is saved in a file with the current\n\
Gedcom filename with \"Photos.txt\" appended, and which will contain\n\
the individual's Gedcom number and the full filename of the photo.\n\n");

TCHAR AlternateParents[] = TEXT("\
To add a second set of parents to an individual, highlight a prospective\n\
parent (single or married), and select as a child that individual who\n\
already has one parent or one set of parents.\n\
\n\
Spouses can have the same sex, but in that case, they can't have a child.\n\
However, one of them can have a child or adopt a child.\n\
\n\
If an individual has two sets of parents (adoptive or whatever),\n\
a button labeled \"Swap Primary & Secondary Parents\" will show.");

TCHAR MultipleMonitorHelp[] = TEXT("\
To show Simple Family Tree on multiple monitors, the primary monitor must be on the left.\n\
\n\
Make Simple Family Tree not-full-screen\n\
Move the left and top and bottom borders to the left and top and bottom of the screen\n\
Move the right border past the screen's right border to the next monitor's right border\n\
\n\
To print the wider virtual screen, hold CTRL down and press the PrintScreen key.");
//TCHAR SameSex[] = TEXT("Same-sex couples can't share children\nin this program because they might appear\nto be the biological parents.");

BOOL CheckBuf(void)
;
void Fill_Indiv(void)
;
void FillArray(void)
;
void FillhdcMem(void)
;
void InitializeAgain(void)
;
BOOL GetSpouse(void)
;
BOOL GetChild(void)
;
DWORD GetName(void)
;
void GetData(void)
;
int CALLBACK ListProc(HWND hwndListDlg, UINT message, WPARAM wParam, LPARAM lParam)
;
int CALLBACK IndivProc(HWND hwndIndiv, UINT message, WPARAM wParam, LPARAM lParam)
;
int CALLBACK NewIndivProc(HWND hwndNewIndiv, UINT message, WPARAM wParam, LPARAM lParam)
;
int CALLBACK NewProc(HWND hwndNew, UINT message, WPARAM wParam, LPARAM lParam)
;
int CALLBACK GEDProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
;
//int CALLBACK FixGedProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
//;
int CALLBACK Name2Proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
;
int CALLBACK EventProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
;
int CALLBACK NoteProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
;
int CALLBACK MarriageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
;
int CALLBACK FixBugProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
;
void PutFAMS(DWORD Offset)
;
void PutFAMC(DWORD Offset)
;
void PutNewFAM(int Person, DWORD Offset)
;
void PutFAM(int Person, DWORD Offset)
;
BOOL CheckifChild(DWORD Offset)
;
void GetNameinBuf(DWORD Offset, TCHAR* Name)
;
void CheckFAM(void)
;
void GetSpouseOffset(DWORD Num)
;
void SaveEdited(void)
;
void ReStart(void)
;
DWORD GetBufEnd(DWORD x)
;
int CheckRelative(DWORD saveHighlighted, DWORD savei)
;
void ParseBirthday(void)
;
void ParseBirthday2(void)
;
void ShowJpeg(void)
;
//void GetPhotoFileName(void)
//;
void UndoIt(void)
;
void CheckForPhotos(void)
;
void SortSiblings(void)
;
void CursorMoved(int xPos, int yPos)
;
DWORD Atoi(TCHAR*ptr)
;
void RemovePhoto(DWORD)
;
//void FixGed(void)
//;
LRESULT CALLBACK PhotoProc(HWND hwndPhoto, UINT message, WPARAM wParam, LPARAM lParam)
;
void CheckFAMS(void)
;
//void MergeFile(void)
//;

TCHAR PrinterName[64];
DWORD PrinterNameLen = 64;
DWORD dwSizeNeeded, dwNumItems, dwItem, dwNeeded;
HDC hDC;
HANDLE hPrinter;
LPPRINTER_INFO_5 lpInfo;
LPDEVMODE pDevMode;

int PossibleMissingName;
int u, d, bd, date[1024], smallest, dateptr, Year, Year2;
int lines, famchild, twice, Font = 6;
int Resolution, PadBytes;
int TypeOffset[6];
int DateOffset[6];
int PlacOffset[6];
int FamOffset[4];
int MarriedToOffset[7];
int MarrDateOffset[7];
int MarrPlacOffset[7];
int index, Response, oldX, newX, so, maxChars;
int BoxLeft, BoxTop, BoxRight, BoxBottom, BoxWidth, BoxHeight;
//int PhotoBoxLeft, PhotoBoxRight, PhotoBoxTop, PhotoBoxBottom;
//int FirstxPos, FirstyPos, PhotoX, PhotoY;
int Width, Height, JpegWidth, JpegHeight, IndivTop;
int Y, Z, YSpacing, cxScreen, cyScreen, cxSingleScreen, xLoc, yLoc;
int xPos, yPos, xPrevious, yPrevious, Top, Left;
int Number = 10, endlessloop;
int hPos, hWidth, vPos, vHeight;
int Frame, TitleAndMenu, Namend;
int Birthdate, BirthDateLen, BirthLocLen, Death2Len, DeathLocLen;
DWORD highlighted = 0xFFFFFFFF, oldHighlighted, IndivNum = 0xFFFFFFFF, IndivNumber, highlightedNum;
DWORD ParentSpouse, spouseX, spouseof, Num, parentX, OldSize, NewSize;
DWORD v, w, x, y, z, I, X, firstX, tempY, prevy, SpaceToLeft, LeftmostSibling, FirstParent, Child, TotalParents, fileSize, fileSize2, PhotoFileSize, JpegFileSize, dwBytesRead, dwBytesWritten, wParameter, MouseLoc;
DWORD BadLine, LineBegin, LineEnd;
DWORD xBig, yBig, badX, badY, BufLines;
DWORD Leftchild, Rightchild, prevRightchild, ChildLineLength, LinetoChildrenX, LinefromChildrenX, LinefromLeftchild, LinefromRightchild, Parent, Parents;
DWORD AncestorMinX, DependantMinX, LeftmostChild, LeftmostParent, LeftmostSpouse, LinefromChildX, LinetoChildX, SpouseLineLen;
DWORD ptr, marrPtr, ArrayX, ArrayY, HighlightedAncestorX, HighlightedDependantX, BufEnd;
DWORD *ArrayUp;
DWORD Array[ROWS][COLS];
DWORD indiv[ROWS];
DWORD Col[ROWS];
DWORD Spouses[MAXSPOUSES];
DWORD BirthdayNames[1024];
DWORD BirthdayDates[1024];
DWORD i, s, ws, s2, ws2, LastIndiv, RealLastIndiv, row, col, RowsUp, LastRow, iFromList;
DWORD NameX, Gap, SmallestGap, NoteBegin, NoteEnd;
DWORD Fams, FamNum, FamsEnd, FamPtr, LastIndivNum, Number2, IndivOffset, DeleteOffset, NewIndivOffset, HusbNum, WifeNum;
DWORD FamS[MAXSPOUSES];
DWORD SingleParentFamNum, NoteLen;
DWORD SpouseOffset[21];
DWORD ChildOffset[21];
DWORD HusbOffset, WifeOffset, Children, SingleChildOffset, child, FamC;
DWORD g = 0, charIndex, lineIndex, firstCharIndex, lineLength, typeNameLen;
DWORD GotSpouse[MAXSPOUSES];
DWORD Backspace[512];
DWORD bs;
DWORD test, test2;
DWORD PhotoNum;
WORD Age, day, months[] = {0, 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
BYTE *utf;

TCHAR CmdLine;
TCHAR age[4];
TCHAR CurrentDir[MAX_PATH];
TCHAR FamilyTreeIni[] = TEXT("Simple Family Tree.ini");
TCHAR IniBuf[512];
TCHAR KeyFile[] = TEXT("SimpleFamilyTree.dta");
TCHAR SimpleFamilyTreePhotos[MAX_PATH];// = "SimpleFamilyTreePhotos.dta");
TCHAR SimpleFamilyTreePhotosTxt[MAX_PATH];// = "SimpleFamilyTreePhotos.txt");
TCHAR SimpleFamilyTreePhotosBak[MAX_PATH];// = "SimpleFamilyTreePhotos.bak");
TCHAR PhotoDta[] = TEXT("Photos.dta");
TCHAR PhotoTxt[] = TEXT("Photos.txt");
TCHAR PhotoBak[] = TEXT("Photos.bak");
TCHAR Filename[MAX_PATH];
TCHAR FullFilename[MAX_PATH];
TCHAR FullSaveAsFilename[MAX_PATH];
TCHAR FinalFilename[MAX_PATH];
TCHAR IndivFilename[MAX_PATH];
TCHAR IndivFullFilename[MAX_PATH];
TCHAR PhotoFilename[MAX_PATH];
TCHAR FullPhotoFilename[MAX_PATH];
TCHAR BackupFilename[MAX_PATH];
TCHAR TempFilename[] = TEXT("zzzz.tmp");
TCHAR *Buf, *Buf2, *Birth, *Death, *BirthLoc, *DeathLoc, *Jpeg;
TCHAR temp[512], temp2[] = TEXT("8888-8888");
TCHAR HighlightedName[256];
TCHAR IndivName[256];
TCHAR TitleBar[300];
TCHAR Error[512];
TCHAR Photos[] = TEXT("Photo");
TCHAR Arial[] = TEXT("Arial");
TCHAR CourierNew[] = TEXT("Courier New");
TCHAR SmallFonts[] = TEXT("Small Fonts");
TCHAR ch;
TCHAR Blank[] = TEXT("\0");//"    ");
DWORD BigNoteLen;
TCHAR BigNote[30000];
DWORD IndivBufLen;
TCHAR IndivBuf[32000];
TCHAR MarrData[5000];
TCHAR IniShow[] = TEXT("ShowIndivInfo=");
TCHAR Loops[] = TEXT("00,000 loops");
TCHAR Additions[] = TEXT("\r\n***Additional entries***\r\nNAME: \r\n\r\nEVENT:\r\nTYPE: \r\nDATE: \r\nPLACE: \r\n\r\nEVENT:\r\nTYPE: \r\nDATE: \r\nPLACE: \r\n\r\nEVENT:\r\nTYPE: \r\nDATE: \r\nPLACE: \r\n\r\nNOTE: \r\n\r\n\r\n\r\n\r\n");
TCHAR *Ones[12] = {TEXT("ALI"),TEXT("BUR"),TEXT("CHR"),TEXT("EDU"),TEXT("EVE"),TEXT("GRA"),TEXT("NAM"),TEXT("NOT"),TEXT("OCC"),TEXT("RES"),TEXT("MAR"),TEXT("DIV")};
int TypeNameLen[12] = {10, 11, 15, 14, 10, 14, 9, 9, 14, 14, 10, 11};
TCHAR Alias[] = TEXT("\r\nALIAS:\r\n");//10
TCHAR Burial[] = TEXT("\r\nBURIED:\r\n");//11
TCHAR Christened[] = TEXT("\r\nCHRISTENED:\r\n");//15
TCHAR Education[] = TEXT("\r\nEDUCATION:\r\n");//14
TCHAR Event[] = TEXT("\r\nEVENT:\r\n");//10
TCHAR Graduated[] = TEXT("\r\nGRADUATED:\r\n");//14
TCHAR name[] = TEXT("\r\nNAME2: ");//9
//TCHAR Key[] = TEXT("\0\0\0\0\0\0\0");
TCHAR Note[] = TEXT("\r\nNOTE ");//7
TCHAR Occupation[] = TEXT("\r\nOCCUPATION: ");//14
TCHAR Residence[] = TEXT("\r\nRESIDENCE:\r\n");//14
TCHAR Married[] = TEXT("\r\nMARRIED ");//10
TCHAR Divorced[] = TEXT("\r\nDIVORCED ");//11
TCHAR Num1[24] = TEXT("0 @I");
TCHAR Num2[9] = TEXT("@ INDI\r\n");
TCHAR Name[256];
TCHAR Name1[256];
TCHAR Name2[256] = TEXT("1 NAME ");
TCHAR Name3[256] = TEXT("1 NAME ");
TCHAR PrintName[256] = TEXT("M  ");
TCHAR PrintBirthDate[32] = TEXT("BORN: ");
TCHAR PrintBirthLoc[256] = TEXT("BORN IN: ");
TCHAR PrintDeathDate[32] = TEXT("DIED: ");
TCHAR PrintDeathLoc[256] = TEXT("DIED IN: ");
DWORD PrintNameLen;
DWORD PrintBirthDateLen;
DWORD PrintBirthLocLen;
DWORD PrintDeathDateLen;
DWORD PrintDeathLocLen;
TCHAR UnKnown[] = TEXT("1 NAME /UNKNOWN/\r\n");// 18
TCHAR Note2[30000] = TEXT("1 NOTE ");
TCHAR Sex;//M or F
TCHAR Sex2[10] = TEXT("1 SEX M\r\n");
TCHAR Birth1[9] = TEXT("1 BIRT\r\n");
TCHAR BirthDate[32] = TEXT("2 DATE ");
TCHAR BirthLoc2[256] = TEXT("2 PLAC ");
TCHAR Death1[9] = TEXT("1 DEAT\r\n");
TCHAR Death2[32] = TEXT("2 DATE ");
TCHAR DeathLoc2[256] = TEXT("2 PLAC ");
TCHAR Marr[] = TEXT("1 MARR\r\n");
TCHAR Type[256] = TEXT("2 TYPE ");
TCHAR Date[256] = TEXT("2 DATE ");
TCHAR Place[256] = TEXT("2 PLAC ");
TCHAR Even[] = TEXT("1 EVEN\r\n");
TCHAR WholeEvent[512];
TCHAR Conc[] = TEXT("\r\n2 CONC ");
TCHAR Cont[] = TEXT("2 CONT ");
TCHAR FamEntry[21] = TEXT("0 @F");
TCHAR Famend[8] = TEXT("@ FAM\r\n");
TCHAR FamHusb[10] = TEXT("1 HUSB ");
TCHAR FamWife[10] = TEXT("1 WIFE ");
TCHAR FamChil[10] = TEXT("1 CHIL ");
TCHAR NewIndiv[1140];
TCHAR Famspouse[24] = TEXT("1 FAMx @F");//x becomes C or S
TCHAR ChildsName[50];
TCHAR ChildsExtra[] = TEXT("'s Parents");
TCHAR ParentsName[50];
TCHAR And[] = TEXT(" and ");
TCHAR Nonelse[] = TEXT("no one else?");
TCHAR BirthDeath[256];
TCHAR one, two, three;
//TCHAR Line[128];
//TCHAR unknown[] = TEXT("UNKNOWN\r");
//TCHAR TheKey[] = TEXT("\0\0\0\0\0\0\0");
TCHAR PhotoDirectory[MAX_PATH];
TCHAR *PhotoBuf;
BYTE *JpegBuf;
BYTE *photo_pixel_buf;
BOOL first, firstime, duplicate, inbox, gotleftchild, notempty, parent, gotchild, nogood = FALSE, arrayfilled = FALSE, showindividual = FALSE, gotmarrdata;
BOOL preditit = FALSE, editit = FALSE, fromnewlink = FALSE, newfile, newfather = FALSE, newmother = FALSE, newspouse = FALSE, newchild = FALSE, famcfirst = TRUE;
BOOL foundspouse, notherspouse, deleetit = FALSE, unlinked = FALSE, fromrestart = FALSE, indivbox = FALSE, list = FALSE, firstphoto = TRUE, showmiddlename = TRUE;
BOOL nonote, noname, gotit, mouseover = NONE, gotmouseover, childhasparents, photo = FALSE, fromcommandline = FALSE, specialcase = FALSE, photowritten;
BOOL gotdate = FALSE, gotmonth, updateindiv = FALSE, dontupdateindiv, anyeditit = FALSE, undone = FALSE, firstmousemove, gotbirthdate, fromspace = FALSE;

struct INDIV {
	DWORD Name;
	DWORD Birth;
	DWORD BirthLoc;
	DWORD Death;
	DWORD DeathLoc;
	DWORD Num;
	DWORD Parent;
	DWORD Note;
	DWORD Child;
	DWORD X;
	DWORD Childof;
	DWORD Spouseof[MAXSPOUSES];
	DWORD Spouse;
	DWORD LeftChild;
	DWORD RightChild;
	DWORD Flags;
	WORD ArrayX;
	WORD ArrayY;
	WORD Sex;
	WORD cx;
} *Indiv;

struct INDIV tempIndiv;
struct INDIV OrigIndiv[EXTRAINDIVIDUALS];
DWORD OrigIndivPtrs[EXTRAINDIVIDUALS];
DWORD oi;

HWND hwnd, hwndList, hwndInstr, hwndIndiv = NULL, hwndListDlg = NULL, hwndBirthday = NULL, hwndPhoto = NULL, hwndDescEdit;
HWND hwndName, hwndBirth, hwndAge = NULL, hwndSex, hwndDeath, hwndBirthLoc, hwndDeathLoc, hwndNote;
HWND hwndMarriedTo[6], hwndDate[6], hwndPlace[6];
HANDLE hFile, hFile2, hBirthFile, hKeyFile, hPhotoFile = NULL, hPhotoFile2, hJpegFile, hFindFile, hTemp;
HDC hdc, hdcMem, hdcPrn;
HBITMAP hBitmap, hPrnBitmap;
PAINTSTRUCT ps;
RECT rect, rectMem, HighlightedRect, ListRect, dlgRect, boxRect;
SIZE Size, Size2, PrintSize;
HCURSOR hCursor, hDrawingCursor1, hDrawingCursor2, hWaitingCursor, hArrowCursor;
HINSTANCE hInst, hInst2;
LOGFONT lf, lf2, lf3, lf4;
HFONT hFont, hFont2, hFont3;
OPENFILENAME ofn, ofn2;
HBRUSH hBrush, hHighlightedBrush, hYellowBrush, hPurpleBrush;
WIN32_FIND_DATA fd;
WIN32_FIND_DATAA afd;
struct _SYSTEMTIME st, *lpSystemTime = &st;
struct _FILETIME ft;
ULARGE_INTEGER li1, li2;
PRINTDLG pd;
DOCINFO di = { sizeof(DOCINFO), TEXT("x"), NULL, NULL, 0 };
HGDIOBJ hObject;
HMODULE hModule;
BITMAPINFOHEADER bmih, *pbmih = &bmih, bmih2, *pbmih2 = &bmih2, bmih3, *pbmih3 = &bmih3, bmih4, *pbmih4 = &bmih4;
BITMAPINFO bmi, *pbmi = &bmi, bmi2, *pbmi2 = &bmi2, bmi3, *pbmi3 = &bmi3, bmi4, *pbmi4 = &bmi4;
JPEG_CORE_PROPERTIES jcprops;
IJLERR jerr;
HMENU hMenu, hMenu2;
COLORREF RED = 0xFF, BLACK = 0;//xFFFFFF;
CHOOSEFONT cf;
SCROLLINFO si;

LRESULT CALLBACK WndProc (HWND, UINT, WPARAM, LPARAM);
WNDPROC pListProc;

LRESULT CALLBACK EditListProc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if ((message == WM_SYSKEYDOWN) && (wParam == 'I') && (!showindividual))
		SendMessage(hwndListDlg, WM_COMMAND, IDC_BUTTON1, 0);
	else if ((message == WM_SYSKEYDOWN) && (wParam == 'F') && (showindividual))
		SendMessage(hwndListDlg, WM_COMMAND, IDC_BUTTON1, 0);
	else if ((message == WM_KEYDOWN) && (wParam == VK_RETURN))
		SendMessage(hwndListDlg, WM_COMMAND, (WPARAM)IDOK, 0);
	else if ((message == WM_KEYDOWN) && (wParam == VK_ESCAPE))
		SendMessage(hwndListDlg, WM_COMMAND, (WPARAM)IDCANCEL, 0);
	return CallWindowProc(pListProc, hwnd2, message, wParam, lParam);
}

LRESULT CALLBACK PhotoProc(HWND hwndPhoto, UINT message, WPARAM wParam, LPARAM lParam)
{
//	int xPos, yPos;
	HDC hdcPhoto;
	PAINTSTRUCT psPhoto;

	switch(message)
	{
	case WM_CREATE:
		PrinterName[0] = 0;
		bmih4.biSize = sizeof(BITMAPINFOHEADER);
		bmih4.biWidth = JpegWidth;
		bmih4.biHeight = -(JpegHeight);// - for right-side-up picture
		bmih4.biPlanes = 1;
		bmih4.biBitCount = 24;
		bmih4.biCompression = BI_RGB;
		bmih4.biSizeImage = 0;
		bmih4.biXPelsPerMeter = 0;
		bmih4.biYPelsPerMeter = 0;
		bmih4.biClrUsed = 0;
		bmih4.biClrImportant = 0;
		bmi4.bmiHeader = *pbmih4;
//		BoxWidth = BoxRight - BoxLeft;
//		BoxHeight = BoxBottom - BoxTop;
//		firstmousemove = TRUE;
		return 0;
/*
	case WM_MOUSEMOVE:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);
		if (firstmousemove)
		{
			firstmousemove = FALSE;
			FirstxPos = xPos;
			FirstyPos = yPos;
			PhotoBoxLeft = FirstxPos - PhotoX;
			PhotoBoxRight = PhotoBoxLeft + BoxWidth;
			PhotoBoxTop = FirstyPos - PhotoY;
			PhotoBoxBottom = PhotoBoxTop + BoxHeight;
		}
		else if ((xPos < PhotoBoxLeft) || (xPos > (PhotoBoxRight)) || (yPos < (PhotoBoxTop)) || (yPos > (PhotoBoxBottom)))
			DestroyWindow(hwndPhoto);
		return 0;
*/
	case WM_PAINT:
		hdcPhoto = BeginPaint(hwndPhoto, &psPhoto);
		SetDIBitsToDevice(hdcPhoto, 0, 0, JpegWidth, JpegHeight, 0, 0, 0, JpegHeight, photo_pixel_buf, pbmi4, DIB_RGB_COLORS);
		EndPaint(hwndPhoto, &psPhoto);
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_ESCAPE)
			DestroyWindow(hwndPhoto);
//		else if (wParam == 'P')
//		{
//			gotmouseover = FALSE;
//			mouseover = NONE;
//			InvalidateRect(hwnd, &rect, FALSE);
//			DestroyWindow(hwndPhoto);
//		}
		break;

	case WM_DESTROY:
		hwndPhoto = NULL;
		VirtualFree(photo_pixel_buf, 0, MEM_RELEASE);
		photo_pixel_buf = NULL;
		return 0;		
	}
	return DefWindowProc(hwndPhoto, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	hInst = hInstance;
	wndclass.style         = CS_HREDRAW|CS_VREDRAW|CS_DBLCLKS;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (hInstance, MAKEINTRESOURCE (IDI_ICON1));
	wndclass.hCursor       = NULL;//LoadCursor(hInst, MAKEINTRESOURCE(IDC_HAND));//LoadCursor(NULL, IDC_ARROW);//IDC_CURSOR2
	wndclass.hbrBackground = NULL;//(HBRUSH)CreateSolidBrush(LIGHTGRAY);
	wndclass.lpszMenuName  = TEXT("SIMPLEFAMILYTREE");
	wndclass.lpszClassName = szAppName;

	if (!RegisterClassW(&wndclass))
		return 0;

	wndclass.lpfnWndProc = PhotoProc;
	wndclass.hIcon = NULL;
	wndclass.lpszClassName = Photos;
	RegisterClassW(&wndclass);

	if (szCmdLine[0]!= 0)
	{
		y = 0;
		CmdLine = '\x0';
		if (szCmdLine[0] == '"')
		{
			CmdLine = '"';
			y = 1;
		}
		for (x = 0; szCmdLine[y] != CmdLine ; x++, y++)
			FullFilename[x] = szCmdLine[y];
		FullFilename[x] = 0;
		for ( ; (x > 0) && (FullFilename[x] != '\\'); x--)
			;
		for (x++, y = 0; FullFilename[x] != 0; x++, y++)
			Filename[y] = FullFilename[x];
		Filename[y] = 0;
		fromcommandline = TRUE;
	}

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	ShowWindow(hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow(hwnd);

	while (GetMessage(&msg, NULL, 0, 0))
	{
		if ((hwndIndiv == NULL) || (!IsDialogMessage (hwndIndiv, &msg)) && ((hwndListDlg == NULL) || (!IsDialogMessage (hwndListDlg, &msg))))
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
	}
	return (int)(WPARAM)msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_SIZE:
		rect.right = LOWORD(lParam);
		rect.bottom = HIWORD(lParam);
		if (Height > rect.bottom)
			Top = Height - rect.bottom;
		else
			Top = 0;
		if (Width > rect.right)
			Left = Width - rect.right;
		else
			Left = 0;
		break;

	case WM_CREATE:
		if (NOERROR == SHGetSpecialFolderPath(hwnd, FinalFilename, CSIDL_APPDATA, 0)) {
			for (x = 0; (x < MAX_PATH) && (FinalFilename[x] != 0); x++)
				;
			if (x < MAX_PATH) {
				for (y = 0; SimpleFamilyTree[y] != 0; x++, y++)
					FinalFilename[x] = SimpleFamilyTree[y];
				FinalFilename[x] = 0;
				CreateDirectory(FinalFilename, NULL);
			}
		}
		else
			FinalFilename[0] = 0;

		hMenu = GetMenu(hwnd);
		CheckMenuItem(hMenu, ID_EDIT_SHOWMIDDLENAME, MF_CHECKED);
		PhotoDirectory[0] = 0;
		for (x = 0; x < 512; x++)
			Backspace[x] = -1;
		bs = 0;
		RowsUp = MIDROW;
		LastIndiv = 0;//flag
		hBrush = CreateSolidBrush(LIGHTGRAY);
		hHighlightedBrush = CreateSolidBrush(0xC0C0C0);
		hYellowBrush = CreateSolidBrush(0xFFFF);
		hPurpleBrush = CreateSolidBrush(0xFF00FF);
		first = TRUE;
		Indiv = NULL;
		Buf = NULL;
		GetCurrentDirectory(MAX_PATH, CurrentDir);
		if (FALSE == fromcommandline)
		{
			Filename[0] = 0;
			FullFilename[0] = 0;
		}
		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.lpstrFilter       = TEXT(" *.ged\0*.ged\0\0");
		ofn.lpstrCustomFilter = NULL;
		ofn.lpstrFile         = FullFilename;
		ofn.lpstrFileTitle    = Filename;
		ofn.Flags             = OFN_HIDEREADONLY|OFN_NOCHANGEDIR|OFN_OVERWRITEPROMPT;
		ofn.lpstrDefExt       = TEXT("ged");
		ofn.hwndOwner         = hwnd;
		ofn.hInstance         = NULL;
		ofn.nMaxFile          = MAX_PATH;
		ofn.nMaxCustFilter    = 0;
		ofn.nFilterIndex      = 0;
		ofn.nMaxFileTitle     = MAX_PATH;
		ofn.lpstrInitialDir   = CurrentDir;
		ofn.nFileOffset       = 0;
		ofn.nFileExtension    = 0;
		ofn.lCustData         = 0;
		ofn.lpfnHook          = NULL;
		ofn.lpTemplateName    = NULL;

		ofn2.lStructSize       = sizeof(OPENFILENAME);
		ofn2.lpstrFilter       = TEXT(" *.ged\0*.ged\0\0");
		ofn2.lpstrCustomFilter = NULL;
		ofn2.lpstrFile         = FullSaveAsFilename;
		ofn2.lpstrFileTitle    = Filename;
		ofn2.Flags             = OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;
		ofn2.lpstrDefExt       = TEXT("ged");
		ofn2.hwndOwner         = hwnd;
		ofn2.hInstance         = NULL;
		ofn2.nMaxFile          = MAX_PATH;
		ofn2.nMaxCustFilter    = 0;
		ofn2.nFilterIndex      = 0;
		ofn2.nMaxFileTitle     = MAX_PATH;
		ofn2.lpstrInitialDir   = TEXT("C:");
		ofn2.nFileOffset       = 0;
		ofn2.nFileExtension    = 0;
		ofn2.lCustData         = 0;
		ofn2.lpfnHook          = NULL;
		ofn2.lpTemplateName    = NULL;

		lf.lfHeight = -11;
		lf.lfWeight = 400;
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

		lf2.lfHeight = -15;
		lf2.lfWeight = 400;
		lf2.lfItalic = 0;
		lf2.lfUnderline = 0;
		lf2.lfStrikeOut = 0;
		lf2.lfCharSet = 0;
		lf2.lfOutPrecision = 1;
		lf2.lfClipPrecision = 2;
		lf2.lfQuality = 1;
		lf2.lfPitchAndFamily = 0x31;
		for (x = 0; CourierNew[x] != 0; x++)
			lf2.lfFaceName[x] = CourierNew[x];
		lf2.lfFaceName[x] = 0;
		hFont2 = CreateFontIndirect(&lf2);

//		lf3.lfHeight = -96;
		lf3.lfWeight = 400;
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

//		lf4.lfHeight = -11
		lf4.lfWeight = 400;
		lf4.lfItalic = 0;
		lf4.lfUnderline = 0;
		lf4.lfStrikeOut = 0;
		lf4.lfCharSet = 0;
		lf4.lfOutPrecision = 1;
		lf4.lfClipPrecision = 2;
		lf4.lfQuality = 1;
		lf4.lfPitchAndFamily = 0x22;
		for (x = 0; SmallFonts[x] != 0; x++)
			lf4.lfFaceName[x] = SmallFonts[x];
		lf4.lfFaceName[x] = 0;

		cf.lStructSize    = sizeof (CHOOSEFONT) ;
		cf.hwndOwner      = hwnd ;
		cf.hDC            = NULL ;
		cf.lpLogFont      = &lf ;
		cf.iPointSize     = 0 ;
		cf.Flags          = CF_INITTOLOGFONTSTRUCT | CF_SCREENFONTS | CF_EFFECTS ;
		cf.rgbColors      = 0 ;
		cf.lCustData      = 0 ;
		cf.lpfnHook       = NULL ;
		cf.lpTemplateName = NULL ;
		cf.hInstance      = NULL ;
		cf.lpszStyle      = NULL ;
		cf.nFontType      = 0 ;      
		cf.nSizeMin       = 0 ;
		cf.nSizeMax       = 0 ;

		cxSingleScreen = GetSystemMetrics(SM_CXSCREEN);
		cxScreen = GetSystemMetrics(SM_CXVIRTUALSCREEN);
		cyScreen = GetSystemMetrics(SM_CYVIRTUALSCREEN);
		Width = cxScreen;
		Height = cyScreen;
		if (cxSingleScreen != cxScreen) {
			AppendMenu(hMenu, MF_STRING, 0x1234, TEXT("Multiple Monitor Help"));
		}
		IndivTop = GetSystemMetrics(SM_CYFRAME);
		IndivTop += GetSystemMetrics(SM_CYCAPTION);
		Frame = GetSystemMetrics(SM_CXSIZEFRAME);
		TitleAndMenu = GetSystemMetrics(SM_CYCAPTION) + GetSystemMetrics(SM_CYMENU);
		hCursor = LoadCursor(NULL, IDC_HAND);
		hDrawingCursor1 = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR1));
		hDrawingCursor2 = LoadCursor(hInst, MAKEINTRESOURCE(IDC_CURSOR2));
		hWaitingCursor = LoadCursor(NULL, IDC_WAIT);
		hArrowCursor = LoadCursor(NULL, IDC_ARROW);
		SetCursor(hArrowCursor);

		hdc = GetDC(hwnd);
		hdcMem = CreateCompatibleDC(hdc);
		hBitmap = CreateCompatibleBitmap(hdc, cxScreen, cyScreen);
		SelectObject(hdcMem, hBitmap);
		SelectObject(hdcMem, hFont);
		SelectObject(hdc, hFont);
		ReleaseDC(hwnd, hdc);
		rectMem.bottom = cyScreen; rectMem.left = 0; rectMem.right = cxScreen; rectMem.top = 0;
		FillRect(hdcMem, &rectMem, hBrush);
		GetTextExtentPoint32(hdcMem, TEXT("j"), 1, &Size);//get Size.cy
		Z = Size.cy*2;
		YSpacing = Z + 10 + (12*2);//12 comes from 17-5, below
		SetBkMode(hdcMem, TRANSPARENT);

		if (fromcommandline)
		{
			for (x = 0; x < 512; x++)
				Backspace[x] = -1;
			bs = 0;
			xLoc = yLoc = 0;
			arrayfilled = FALSE;
			fromnewlink = FALSE;
			highlighted = 0xFFFFFFFF;
			IndivNum = 0xFFFFFFFF;
			IndivNumber = 0xFFFFFFFF;
			LastIndiv = 0;
			if (Buf != NULL)
			{
				free(Buf);
				Buf = NULL;
			}
			Fill_Indiv();
			_snwprintf(TitleBar, 300, TEXT(" %s  [%s]"), szAppName, FullFilename);
			SetWindowText(hwnd, TitleBar);
			SendMessage(hwnd, WM_COMMAND, ID_SHOWLISTOFINDIVIDUALS, 0);//show LIST OF INDIVIDUALS
			return 0;
		}

		hFile = CreateFile(FamilyTreeIni, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			if (fileSize = GetFileSize(hFile, NULL))
			{
				if (fileSize < sizeof(IniBuf))
				{
					utf = (BYTE*)calloc(1, fileSize);
					ReadFile(hFile, utf, fileSize, &dwBytesRead, NULL);
					for (x = 0, y = 0; x < fileSize; )
					{
						if(((utf[x] & MASK3BYTES) == MASK3BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE) && ((utf[x+2] & MASKBYTE) == MASKBYTE))
						{// 1110xxxx 10xxxxxx 10xxxxxx
							IniBuf[y++] = ((utf[x] & 0x0F) << 12) | ((utf[x+1] & MASKBITS) << 6) | (utf[x+2] & MASKBITS);
							x += 3;
						}
						else if(((utf[x] & MASK2BYTES) == MASK2BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE))
						{// 110xxxxx 10xxxxxx
							IniBuf[y++] = ((utf[x] & 0x1F) << 6) | (utf[x+1] & MASKBITS);
							x += 2;
						}
						else if(utf[x] < MASKBYTE)// 0xxxxxxx
							IniBuf[y++] = (TCHAR)utf[x++];
						else
							IniBuf[y++] = (TCHAR)utf[x++];//not utf-8 character
					}
					free(utf);
					for (x = 0; (IniBuf[x] != '\r') && (x < fileSize); x++)
						FullFilename[x] = IniBuf[x];
					FullFilename[x] = 0;
					if (0 != GetFileTitle(FullFilename, Filename, MAX_PATH))
						MessageBox(hwnd, TEXT("Filename error"), Error, MB_OK);
					else
					{
						if ((0 == wcscmp(Filename, TEXT("The Kings of Europe.ged")))
							|| (0 == wcscmp(Filename, TEXT("RomanEmperors.ged"))))
							kings = TRUE;
						else
							kings = FALSE;
					}
					x += 2;//to beginning of second line
					if (x < y)
					{
						if (IniBuf[x] == '@')
						{
							x++;
							if ((IniBuf[x] < '0') || (IniBuf[x] > '9'))//not a number
								x++;
							for (IndivNumber = 0; ((IniBuf[x] >= '0') && (IniBuf[x] <= '9') && (x < y)); x++)//@I71@
								IndivNumber = (IndivNumber * 10) + (IniBuf[x] - '0');
							for ( ; (x < y) && (IniBuf[x] != '\n'); x++)
								;
							if (IniBuf[x] == '\n')
							{
								for ( ; (x < y) && (IniBuf[x] != '='); x++)
									;
								if (IniBuf[x] == '=')
								{
									if ((IniBuf[x+1] == '1') || (IniBuf[x+1] == 'T') || (IniBuf[x+1] == 'Y'))
										showindividual = TRUE;
									else
										showindividual = FALSE;
//									for ( ; (x < y-5) && (IniBuf[x] != '\n'); x++)
//										;
//									if (IniBuf[x] == '\n')
//									{
//										x=x;
//									}
								}
							}
							first = FALSE;
						}
					}
/*
					hKeyFile = CreateFile(KeyFile, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
					if (hKeyFile != INVALID_HANDLE_VALUE)
					{
						ReadFile(hKeyFile, Key, 6, &dwBytesRead, NULL);
						CloseHandle (hKeyFile);
					}
					TheKey[0] = '4';
					TheKey[1] = '3';
					TheKey[2] = 'y';
					TheKey[3] = 'u';
					TheKey[4] = '2';
					TheKey[5] = '1';
*/
				}
			}
			CloseHandle(hFile);
			DeleteFile(FamilyTreeIni);

			if (first == FALSE)
			{
				Fill_Indiv();
				_snwprintf(TitleBar, 300, TEXT(" %s  [%s]"), szAppName, FullFilename);
				SetWindowText(hwnd, TitleBar);
			}
//			first = TRUE;
		}
		return 0;

	case WM_COMMAND:
		wParameter = LOWORD(wParam);
		switch (wParameter)
		{
		case OPEN:
			PhotoDirectory[0] = 0;
			ch = FullFilename[0];
			ofn.lpstrFilter = TEXT(" *.ged\0*.ged\0\0");
			ofn.lpstrFile = FullFilename;
			ofn.lpstrFileTitle = Filename;
			ofn.lpstrTitle = TEXT("Open/Create a Family Tree File");
			ofn.lpstrDefExt = TEXT("ged");
			if (GetOpenFileName(&ofn))
			{
				if (0 == wcscmp(Filename, TEXT("The Kings of Europe.ged")))
					kings = TRUE;
				else
					kings = FALSE;
				for (x = 0; x < 512; x++)
					Backspace[x] = -1;
				bs = 0;
				xLoc = yLoc = 0;
				arrayfilled = FALSE;
				fromnewlink = FALSE;
				showindividual = FALSE;
				highlighted = 0xFFFFFFFF;
				IndivNum = 0xFFFFFFFF;
				IndivNumber = 0xFFFFFFFF;
				LastIndiv = 0;
				if (Buf != NULL)
				{
					free(Buf);
					Buf = NULL;
				}
				firstphoto = TRUE;
				Fill_Indiv();
				_snwprintf(TitleBar, 300, TEXT(" %s  [%s]"), szAppName, FullFilename);
				SetWindowText(hwnd, TitleBar);
				SendMessage(hwnd, WM_COMMAND, ID_SHOWLISTOFINDIVIDUALS, 0);//show LIST OF INDIVIDUALS
			}
			else
				FullFilename[0] = ch;
			break;

		case ID_FILE_SAVEAS:
			MessageBox(hwnd, TEXT("Family tree data is automatically saved after each change to it.\n\nThis is only for writing a copy of that file to another folder."), TEXT("NOTE"), MB_OK);
			if (GetSaveFileName(&ofn2)) {
				utf = (BYTE*)VirtualAlloc(NULL, 2*fileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
				for (x = 0, y = 0; x < fileSize; x++)
				{
					if (Buf[x] < 0x80)
						utf[y++] = *(BYTE*)&Buf[x];
					else if (Buf[x] < 0x800)
					{
						utf[y++] = ((BYTE)(MASK2BYTES | Buf[x] >> 6));
						utf[y++] = ((BYTE)(MASKBYTE | Buf[x] & MASKBITS));
					}
					else if(Buf[x] < 0x10000)
					{
						utf[y++] = ((BYTE)(MASK3BYTES | Buf[x] >> 12));
						utf[y++] = ((BYTE)(MASKBYTE | Buf[x] >> 6 & MASKBITS));
						utf[y++] = ((BYTE)(MASKBYTE | Buf[x] & MASKBITS));
					}
					else
						MessageBox(hwnd, TEXT("HUH?"), NULL, MB_OK);
				}
				hFile = CreateFile(FullSaveAsFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				if (0 == WriteFile(hFile, utf, y, &dwBytesWritten, NULL))
					MessageBox(hwnd, FullFilename, TEXT("This file was NOT written:"), MB_OK);
				else
					FlushFileBuffers(hFile);
				CloseHandle(hFile);
				VirtualFree(utf, 0, MEM_RELEASE);
			}
			break;

		case PRINT:
			GetClientRect(hwnd, &rect);
			hdc = GetDC(hwnd);
			BitBlt(hdcMem, 0, 0, rect.right, rect.bottom, hdc, 0, 0, SRCCOPY);
			ReleaseDC(hwnd, hdc);
			EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &dwSizeNeeded, &dwNumItems);//get printer name for OpenPrinter
			lpInfo = (LPPRINTER_INFO_5)malloc(dwSizeNeeded);
			EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, (LPBYTE)lpInfo, dwSizeNeeded, &dwSizeNeeded, &dwNumItems);
			PrinterName[0] = 0;
			if (GetDefaultPrinter(PrinterName, &PrinterNameLen))
			{//show possible default printer first
				if (PrinterName[0] != 0)
				{
					if (IDYES == MessageBox(hwnd, PrinterName, TEXT("Use this printer?"), MB_YESNO))
						goto skip;	
				}
			}
			for ( dwItem = 0; dwItem < dwNumItems; dwItem++ )
			{
				if (wcscmp(lpInfo[dwItem].pPrinterName, PrinterName))
				{
					if (IDYES == MessageBox(hwnd, lpInfo[dwItem].pPrinterName, TEXT("Use this printer?"), MB_YESNO))
					{
						wcscpy(PrinterName, lpInfo[dwItem].pPrinterName);
						break;	
					}
				}
			}
skip:		free(lpInfo);
			if (dwItem == dwNumItems)
				break;//nothing chosen;
			OpenPrinter(PrinterName, &hPrinter, NULL);//get hPrinter for DocumentProperties
			dwNeeded = DocumentProperties(hwnd, hPrinter, PrinterName, NULL, NULL, 0);//get DEVMODE size
			pDevMode = (LPDEVMODE)malloc(dwNeeded);
			DocumentProperties(hwnd, hPrinter, PrinterName, pDevMode, NULL, DM_OUT_BUFFER);//get DEVMODE info
			pDevMode->dmOrientation = DMORIENT_LANDSCAPE;
			DocumentProperties(hwnd, hPrinter, PrinterName, pDevMode, pDevMode, DM_IN_BUFFER|DM_OUT_BUFFER);//integrate landscape selection
			ClosePrinter(hPrinter);
			hDC = CreateDC(TEXT("WINSPOOL"), PrinterName, NULL, pDevMode);

			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd);
			hdcPrn = CreateCompatibleDC(hDC);
			hPrnBitmap = CreateCompatibleBitmap(hDC, rect.right, rect.bottom);
			hObject = SelectObject(hdcPrn, hPrnBitmap);
			BitBlt(hdcPrn, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
			w = GetDeviceCaps(hDC, PHYSICALWIDTH);//PHYSICALWIDTH = dpi * 11.0 in Landscape Mode
			z = GetDeviceCaps(hDC, PHYSICALHEIGHT);//PHYSICALHEIGHT = dpi * 8.5 in Landscape Mode
//			if (w > z)
			{//10500 = 10.5" * 1000 & 85 = 8.5 * 10 & 110 = 11.0 * 10 & "/25)*25" rounds down to nearest 25
				x = (((((z * 10500)/85) / rect.right) / 25) * 25);
				y = (((((w * 10500)/110) / rect.right) / 25) * 25);
				if (StartDoc(hDC, &di) > 0)
				{
					if (StartPage(hDC) > 0)
					{//the /100 is there because of the previous w/(8.5*10) and the 10.5*1000
						StretchBlt(hDC, 0, 0, (rect.right * x)/100, (rect.bottom * y)/100, hdcPrn, 0, 0, rect.right, rect.bottom, SRCCOPY);
						if (EndPage(hDC) > 0)
							EndDoc(hDC);
					}
				}
			}
			SelectObject(hdcPrn, hObject);
			DeleteDC(hdcPrn);
			DeleteDC(hDC);
			DeleteObject(hPrnBitmap);
			free(pDevMode);
			break;

		case ID_FILE_EXIT:
			DestroyWindow(hwnd);
			break;

		case ID_EDIT_FONT:
			if (ChooseFont(&cf))
			{
				hFont = CreateFontIndirect(&lf);
				GetDC(hwnd);
				SelectObject(hdc, hFont);
				SelectObject(hdcMem, hFont);
				ReleaseDC(hwnd, hdc);
				GetTextExtentPoint32(hdcMem, TEXT("j"), 1, &Size);//get Size.cy
				Z = Size.cy*2;
				YSpacing = Z + 10 + (12*2);//12 comes from 17-5, below
				Fill_Indiv();
			}
			break;

		case ID_EDIT_INCREASESIZE:
			SendMessage(hwnd, WM_KEYDOWN, 187, 0);
			break;
		case ID_EDIT_DECREASESIZE:
			SendMessage(hwnd, WM_KEYDOWN, 189, 0);
			break;

		case ID_UNDO:
			UndoIt();
			break;

		case ID_EDIT_SHOWMIDDLENAME:
			if (showmiddlename)
			{
				showmiddlename = FALSE;
				CheckMenuItem(hMenu, ID_EDIT_SHOWMIDDLENAME, MF_UNCHECKED);
			}
			else
			{
				showmiddlename = TRUE;
				CheckMenuItem(hMenu, ID_EDIT_SHOWMIDDLENAME, MF_CHECKED);
			}
			Fill_Indiv();
			break;

		case ID_SHOWLISTOFINDIVIDUALS:
			if (LastIndiv)
			{
				if (unlinked)
				{
					unlinked = FALSE;
					if (DestroyWindow(hwndIndiv))
						hwndIndiv = NULL;
				}
				if (hwndListDlg)
				{
					if (DestroyWindow(hwndListDlg))
						hwndListDlg = NULL;
				}
				hwndListDlg = CreateDialog(hInst, TEXT("LIST"), hwnd, ListProc);
			}
			break;

		case ID_NEWINDIVIDUAL:
			DialogBox(hInst, TEXT("NEWINDIVIDUAL"), hwnd, NewIndivProc);
			break;

		case ID_INDIVIDUALSINFO:
			if (highlighted != 0xFFFFFFFF)
			{
				IndivNum = highlighted;
//				preditit = TRUE;
				if (hwndIndiv)
				{
					if (DestroyWindow(hwndIndiv))
						hwndIndiv = NULL;
				}
				hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);
				indivbox = FALSE;
			}
			break;

		case READTHIS:
			MessageBoxA(hwnd, Help, AppName, MB_OK);
			MessageBoxA(hwnd, Help2, AppName, MB_OK);
			break;

		case 0x1234:
			MessageBox(hwnd, MultipleMonitorHelp, TEXT("Multiple Monitor Help"), MB_OK);
			break;
		}
		return 0;

	case WM_MOUSEMOVE:
		if (wParam != MK_LBUTTON)
		{
			xPos = LOWORD(lParam);
			yPos = HIWORD(lParam);
			CursorMoved(xPos, yPos);
		}
		else//if left button down
		{
			SetCursor(hDrawingCursor1);
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
			xLoc += (xPrevious-xPos);
			yLoc += (yPrevious-yPos);
			if (xLoc < 0)
				xLoc = 0;
			if (xLoc > Left)
				xLoc = Left;
			if (yLoc < 0)
				yLoc = 0;
			if (yLoc > Top)
				yLoc = Top;
			FillhdcMem();
		}
		break;

	case WM_LBUTTONDOWN:
		if (hwndPhoto)
		{
			DestroyWindow(hwndPhoto);
			hwndPhoto = NULL;
		}
//		if (mouseover == PHOTO)
//		{
//			gotmouseover = FALSE;
//			mouseover = NONE;
//			InvalidateRect(hwnd, &rect, FALSE);
//		}

		if (inbox == FALSE)
			SetCursor(hDrawingCursor1);
		else//if (MouseLoc == highlighted)
		{
			if (list)
			{
				list = FALSE;
				if (DestroyWindow(hwndListDlg))
					hwndListDlg = NULL;
			}
			oldHighlighted = highlighted;
			if (MouseLoc < RealLastIndiv)
				highlighted = MouseLoc;
			else
			{
				y = Indiv[MouseLoc].Num;
				for (x = 0; (x < RealLastIndiv) && (Indiv[x].Num != y); x++)
					;
				highlighted = x;
				MouseLoc = x;
			}

			if (bs == 511)
				bs = 0;
			bs++;
			if (Backspace[bs] != -1)
				for (x = 511; x >= bs; x--)
					Backspace[x] = Backspace[x-1];
			Backspace[bs] = highlighted;
			InitializeAgain();
			SendMessage(hwnd, WM_MOUSEMOVE, 0, lParam);
		}
		if (gotmouseover)
		{
			gotmouseover = FALSE;
			if (hwndBirthday)
			{
				DestroyWindow(hwndBirthday);
				hwndBirthday = NULL;
			}
			if (hwndAge)
			{
				DestroyWindow(hwndAge);
				hwndAge = NULL;
			}
		}
		return 0;

	case WM_LBUTTONUP:
		if (inbox == FALSE)
			SetCursor(hDrawingCursor2);
		else
			SetCursor(hCursor);
		return 0;

	case WM_RBUTTONDOWN:
		if (inbox)
		{
//			preditit = TRUE;
			IndivNum = MouseLoc;
			if (hwndIndiv)
			{
				if (DestroyWindow(hwndIndiv))
					hwndIndiv = NULL;
			}
			hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);
			indivbox = FALSE;
		}
		if (gotmouseover)
		{
			gotmouseover = FALSE;
			if (hwndBirthday)
			{
				DestroyWindow(hwndBirthday);
				hwndBirthday = NULL;
			}
			if (hwndAge)
			{
				DestroyWindow(hwndAge);
				hwndAge = NULL;
			}
			if (hwndPhoto)
			{
				DestroyWindow(hwndPhoto);
				hwndPhoto = NULL;
			}
		}
		return 0;

	case 0x020A://WM_MOUSEWHEEL
		if (gotmouseover)
		{
			gotmouseover = FALSE;
			if (hwndBirthday)
			{
				DestroyWindow(hwndBirthday);
				hwndBirthday = NULL;
			}
			if (hwndAge)
			{
				DestroyWindow(hwndAge);
				hwndAge = NULL;
			}
//			if (hwndPhoto)
//			{
//				DestroyWindow(hwndPhoto);
//				hwndPhoto = NULL;
//			}
		}
		if (LOWORD(wParam) == MK_CONTROL)
		{
			if (wParam & 0x80000000)
			{
				if (xLoc < (Left-50))
					xLoc += 50;
				else
					xLoc = Left;
			}
			else if (xLoc > 50)
			{
				xLoc -= 50;
			}
			else
				xLoc = 0;
		}
		else
		{
			if (wParam & 0x80000000)
			{
				if (yLoc < (Top-50))
					yLoc += 50;
				else
					yLoc = Top;
			}
			else if (yLoc > 50)
			{
				yLoc -= 50;
			}
			else
				yLoc = 0;
		}
		CursorMoved(xPos, yPos);
		FillhdcMem();
		return 0;

	case WM_KEYUP:
		Number = 10;
		break;

	case WM_KEYDOWN:
		if (gotmouseover)
		{
			gotmouseover = FALSE;
			if (hwndBirthday)
			{
				DestroyWindow(hwndBirthday);
				hwndBirthday = NULL;
			}
			if (hwndAge)
			{
				DestroyWindow(hwndAge);
				hwndAge = NULL;
			}
//			if (hwndPhoto)
//			{
//				DestroyWindow(hwndPhoto);
//				hwndPhoto = NULL;
//			}
		}
		switch (wParam)
		{
		case 187:// '+'
			if (Font < 6)
			{
				Font++;
				if (Font == 6)
					hFont = CreateFontIndirect(&lf);
				else
				{
					if (Font == 5)
						lf4.lfHeight = -9;
					else if (Font == 4)
						lf4.lfHeight = -8;
					else if (Font == 3)
						lf4.lfHeight = -7;
					else if (Font == 2)
						lf4.lfHeight = -5;
					else if (Font == 1)
						lf4.lfHeight = -4;
					hFont = CreateFontIndirect(&lf4);
				}
				GetDC(hwnd);
				SelectObject(hdc, hFont);
				SelectObject(hdcMem, hFont);
				ReleaseDC(hwnd, hdc);
				GetTextExtentPoint32(hdcMem, TEXT("j"), 1, &Size);//get Size.cy
				Z = Size.cy*2;
				YSpacing = Z + 10 + (12*2);//12 comes from 17-5, below
				Fill_Indiv();
			}
			break;
		case 189:// '-'
			if (Font > 0)
			{
				Font--;
				if (Font == 5)
					lf4.lfHeight = -9;
				else if (Font == 4)
					lf4.lfHeight = -8;
				else if (Font == 3)
					lf4.lfHeight = -7;
				else if (Font == 2)
					lf4.lfHeight = -5;
				else if (Font == 1)
					lf4.lfHeight = -4;
				else if (Font == 0)
					lf4.lfHeight = -3;
				hFont = CreateFontIndirect(&lf4);
				GetDC(hwnd);
				SelectObject(hdc, hFont);
				SelectObject(hdcMem, hFont);
				ReleaseDC(hwnd, hdc);
				GetTextExtentPoint32(hdcMem, TEXT("j"), 1, &Size);//get Size.cy
				Z = Size.cy*2;
				YSpacing = Z + 10 + (12*2);//12 comes from 17-5, below
				Fill_Indiv();
			}
			break;
		case 'Z':
			if (GetKeyState(VK_CONTROL) < 0)
				UndoIt();
			break;
		case 'A':
			gotmouseover = FALSE;
			if (hwndAge)
			{
				DestroyWindow(hwndAge);
				hwndAge = NULL;
			}
			if (mouseover == AGE)
			{
				mouseover = NONE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else
			{
				mouseover = AGE;
				hdc = GetDC(hwnd);
				SetTextColor(hdc, RED);
				TextOut(hdc, 0, 0, TEXT(" A "), 3);
//				SetTextColor(hdc, BLACK);
				ReleaseDC(hwnd, hdc);
				lParam = xPos;
				lParam |= (yPos << 16);
				SendMessage(hwnd, WM_MOUSEMOVE, 0, lParam);
			}
			break;
		case 'B':
			gotmouseover = FALSE;
			if (hwndBirthday)
			{
				DestroyWindow(hwndBirthday);
				hwndBirthday = NULL;
			}
			if (mouseover == BIRTHDAY)
			{
				mouseover = NONE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else
			{
				mouseover = BIRTHDAY;
				hdc = GetDC(hwnd);
				SetTextColor(hdc, RED);
				TextOut(hdc, 0, 0, TEXT(" B "), 3);
//				SetTextColor(hdc, BLACK);
				ReleaseDC(hwnd, hdc);
				lParam = xPos;
				lParam |= (yPos << 16);
				SendMessage(hwnd, WM_MOUSEMOVE, 0, lParam);
			}
			break;
/*
		case 'P':
			gotmouseover = FALSE;
			if (hwndPhoto)
			{
				DestroyWindow(hwndPhoto);
				hwndPhoto = NULL;
			}
			if (mouseover == PHOTO)
			{
				mouseover = NONE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			else
			{
				mouseover = PHOTO;
				hdc = GetDC(hwnd);
				SetTextColor(hdc, RED);
				TextOut(hdc, 0, 0, TEXT(" P "), 3);
//				SetTextColor(hdc, BLACK);
				ReleaseDC(hwnd, hdc);
				lParam = xPos;
				lParam |= (yPos << 16);
				SendMessage(hwnd, WM_MOUSEMOVE, 0, lParam);
			}
			break;
*/
		case 'P':
			if (photo)
			{
				photo = FALSE;
				FillhdcMem();
			}
			else
			{
				photo = TRUE;
				FillhdcMem();
			}
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case 'U':
			UndoIt();
			break;
		case 'I':
			SendMessage(hwnd, WM_COMMAND, ID_INDIVIDUALSINFO, 0);
			break;
		case 'L':
//		case VK_RETURN:
			SendMessage(hwnd, WM_COMMAND, ID_SHOWLISTOFINDIVIDUALS, 0);
			break;
		case 'N':
			SendMessage(hwnd, WM_COMMAND, ID_NEWINDIVIDUAL, 0);
			break;
		case 'R':
		case VK_F1:
			SendMessage(hwnd, WM_COMMAND, READTHIS, 0);
			break;
		case VK_BACK:
			if (bs > 1)
			{
				bs--;
				if (Backspace[511] != -1)
					bs = 511;
				highlighted = Backspace[bs];
				fromspace = TRUE;
				InitializeAgain();
			}
			break;
		case VK_SPACE:
			if ((Backspace[bs+1] != -1) && (bs != 511))
				bs++;
//			else if (bs == 511)
//				bs = 1;
			highlighted = Backspace[bs];
			fromspace = TRUE;
			InitializeAgain();
			break;
		case VK_UP:
			if (yLoc >= Number)
				yLoc -= Number;
			else if (yLoc < Number)
				yLoc = 0;
			CursorMoved(xPos, yPos-Number);
			FillhdcMem();
			Number++;
			break;
		case VK_DOWN:
			if (yLoc < Top)
			{
				if ((yLoc + Number) > Top)
					yLoc = Top;
				else
					yLoc += Number;
				CursorMoved(xPos, yPos+Number);
				FillhdcMem();
				Number++;
			}
			break;
		case VK_RIGHT:
			if ((xLoc + Number) < Left)
				xLoc += Number;
			else
				xLoc = Left;
			CursorMoved(xPos+Number, yPos);
			FillhdcMem();
			Number++;
			break;
		case VK_LEFT:
			if (xLoc > Number)
				xLoc -= Number;
			else
				xLoc = 0;
			CursorMoved(xPos-Number, yPos);
			Number++;
			FillhdcMem();
			break;
		}
		break;
/*
	case WM_VSCROLL:
		switch (LOWORD (wParam))
		{
		case SB_TOP:
//			vScrollPos = 0;
			yLoc = 0;
			break;
		case SB_BOTTOM:
//			vScrollPos = totalLines - linesPerScreen;
			break;
		case SB_LINEUP:
//			vScrollPos -= 1;
			if (yLoc >= Number)
				yLoc -= Number;
			else if (yLoc < Number)
				yLoc = 0;
			CursorMoved(xPos, yPos-Number);
			FillhdcMem();
			Number++;
			break;
		case SB_LINEDOWN:
//			vScrollPos += 1;
			break;
		case SB_PAGEUP:
//			vScrollPos -= linesPerScreen;
			break;
		case SB_PAGEDOWN:
//			vScrollPos += linesPerScreen;
			break;
		case SB_THUMBPOSITION:
//			vScrollPos = HIWORD (wParam);
			break;
		}
//		vScrollPos = max (0, min (vScrollPos, totalLines - (linesPerScreen)));
//		if (vScrollPos != GetScrollPos(hwnd, SB_VERT))
//			SetScrollPos(hwnd, SB_VERT, vScrollPos, TRUE);
		return 0;
*/
	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
//		if (hdcMem)
//		{
			BitBlt(hdc, 0, 0, cxScreen, cyScreen, hdcMem, 0, 0, SRCCOPY);
			if (mouseover == BIRTHDAY)
			{
				SetTextColor(hdc, RED);
				TextOut(hdc, 0, 0, TEXT(" B "), 3);
//				SetTextColor(hdc, BLACK);
			}
			else if (mouseover == AGE)
			{
				SetTextColor(hdc, RED);
				TextOut(hdc, 0, 0, TEXT(" A "), 3);
//				SetTextColor(hdc, BLACK);
			}
//			else if (mouseover == PHOTO)
//			{
//				SetTextColor(hdc, RED);
//				TextOut(hdc, 0, 0, " P ", 3);
//				SetTextColor(hdc, BLACK);
//			}
			if (LastIndiv)
			{
				if (Width > rect.right)
				{
					hWidth = rect.right * rect.right / Width;
					hPos = ((xLoc * rect.right) / Width);
					MoveToEx(hdc, hPos, rect.bottom-5, NULL);
					LineTo(hdc, hPos + hWidth, rect.bottom-5);
					MoveToEx(hdc, hPos, rect.bottom-6, NULL);
					LineTo(hdc, hPos + hWidth, rect.bottom-6);
					MoveToEx(hdc, hPos, rect.bottom-7, NULL);
					LineTo(hdc, hPos + hWidth, rect.bottom-7);
				}
				if (Height > rect.bottom)
				{
					vHeight = rect.bottom * rect.bottom / Height;
					vPos = ((yLoc * rect.bottom) / Height);
					MoveToEx(hdc, rect.right-5, vPos, NULL);
					LineTo(hdc, rect.right-5, vPos + vHeight);
					MoveToEx(hdc, rect.right-6, vPos, NULL);
					LineTo(hdc, rect.right-6, vPos + vHeight);
					MoveToEx(hdc, rect.right-7, vPos, NULL);
					LineTo(hdc, rect.right-7, vPos + vHeight);
				}
			}
//		}
		EndPaint(hwnd, &ps);
		break;

	case WM_DESTROY:
		DeleteDC(hdcMem);
		DeleteObject(hBitmap);
		DeleteObject(hBrush);
		DeleteObject(hHighlightedBrush);
		DeleteObject(hYellowBrush);
		if (Buf != NULL)
		{
			for (x = 0; FullFilename[x] != 0; x++)
				IniBuf[x] = FullFilename[x];
			IniBuf[x++] = '\r';
			IniBuf[x++] = '\n';
			IniBuf[x++] = '@';
			IniBuf[x++] = 'I';
			if ((highlighted != 0xFFFFFFFF) && (Indiv[highlighted].Num))
			{
				ch = (char)((Indiv[highlighted].Num % 1000000000) / 100000000);
				if (ch)
					IniBuf[x++] = ch + '0';
				ch = (char)((Indiv[highlighted].Num % 100000000) / 10000000);
				if (ch)
					IniBuf[x++] = ch + '0';
				ch = (char)((Indiv[highlighted].Num % 10000000) / 1000000);
				if ((ch) || (IniBuf[x-1] != 'I'))
					IniBuf[x++] = ch + '0';
				ch = (char)((Indiv[highlighted].Num % 1000000) / 100000);
				if ((ch) || (IniBuf[x-1] != 'I'))
					IniBuf[x++] = ch + '0';
				ch = (char)((Indiv[highlighted].Num % 100000) / 10000);
				if ((ch) || (IniBuf[x-1] != 'I'))
					IniBuf[x++] = ch + '0';
				ch = (char)((Indiv[highlighted].Num % 10000) / 1000);
				if ((ch) || (IniBuf[x-1] != 'I'))
					IniBuf[x++] = ch + '0';
				ch = (char)((Indiv[highlighted].Num % 1000) / 100);
				if ((ch) || (IniBuf[x-1] != 'I'))
					IniBuf[x++] = ch + '0';
				ch = (char)((Indiv[highlighted].Num % 100) / 10);
				if ((ch) || (IniBuf[x-1] != 'I'))
					IniBuf[x++] = ch + '0';
				ch = (char)(Indiv[highlighted].Num % 10);
				if ((ch) || (IniBuf[x-1] != 'I'))
					IniBuf[x++] = ch + '0';
				IniBuf[x++] = '@';
				IniBuf[x++] = '\r';
				IniBuf[x++] = '\n';
				for (y = 0; y < 14; x++, y++)
					IniBuf[x] = IniShow[y];
				if (showindividual)
					IniBuf[x++] = 'T';
				else
					IniBuf[x++] = 'F';
				IniBuf[x++] = '\r';
				IniBuf[x++] = '\n';
				utf = (BYTE*)malloc(2*x);
				for (z = 0, y = 0; z < x; z++)
				{
					if (IniBuf[z] < 0x80)
						utf[y++] = (BYTE)IniBuf[z];
					else if (IniBuf[z] < 0x800)
					{
						utf[y++] = ((BYTE)(MASK2BYTES | IniBuf[z] >> 6));
						utf[y++] = ((BYTE)(MASKBYTE | IniBuf[z] & MASKBITS));
					}
					else if(IniBuf[z] < 0x10000)
					{
						utf[y++] = ((BYTE)(MASK3BYTES | IniBuf[z] >> 12));
						utf[y++] = ((BYTE)(MASKBYTE | IniBuf[z] >> 6 & MASKBITS));
						utf[y++] = ((BYTE)(MASKBYTE | IniBuf[z] & MASKBITS));
					}
				}
				hFile = CreateFile(FamilyTreeIni, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				WriteFile(hFile, utf, y, &dwBytesWritten, NULL);
				FlushFileBuffers(hFile);
				CloseHandle(hFile);
				free(utf);
			}
			free(Buf);
		}
		if (Indiv != NULL)
			VirtualFree(Indiv, 0, MEM_RELEASE);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}


//SUBROUTINES////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Fill_Indiv(void)
{
	int gotspouse, Line, ErrorLen;
	int husb, wife, chil[21], num, childnum;
	DWORD birthoffset, birthlocoffset, deathoffset, deathlocoffset, fams;
//	DWORD indivNum;
	BOOL gotname, gotsex, gotnum;
	FillRect(hdcMem, &rectMem, hBrush);
	hdc = GetDC(hwnd);
	FillRect(hdc, &rect, hBrush);
	ReleaseDC(hwnd, hdc);
	if ((fromrestart == FALSE) || (Buf == NULL))
	{
		hFile = CreateFile(FullFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
//			x = GetFinalPathNameByHandle(hFile, FinalPath, 1024, 0);
			if (fileSize = GetFileSize(hFile, NULL))
			{
				utf = (BYTE*)calloc(1, fileSize);
				ReadFile(hFile, utf, fileSize, &dwBytesRead, NULL);
				CloseHandle(hFile);
				Buf = (TCHAR*)calloc(1, (2*fileSize)+10000);//10000 for new individuals
				if ((utf[0] == 0xEF) && (utf[1] == 0xBB) && (utf[2] == 0xBF))
					x = 3; // disregard UTF-8 BOM
				else
					x = 0;
				for (y = 0; x < fileSize; )
				{
					if(((utf[x] & MASK3BYTES) == MASK3BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE) && ((utf[x+2] & MASKBYTE) == MASKBYTE))
					{// 1110xxxx 10xxxxxx 10xxxxxx
						Buf[y++] = ((utf[x] & 0x0F) << 12) | ((utf[x+1] & MASKBITS) << 6) | (utf[x+2] & MASKBITS);
						x += 3;
					}
					else if(((utf[x] & MASK2BYTES) == MASK2BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE))
					{// 110xxxxx 10xxxxxx
						Buf[y++] = ((utf[x] & 0x1F) << 6) | (utf[x+1] & MASKBITS);
						x += 2;
					}
//					else if(utf[x] < MASKBYTE)// 0xxxxxxx
//						Buf[y++] = (TCHAR)utf[x++];
					else
						Buf[y++] = (TCHAR)utf[x++];//not utf-8 character
				}
				free(utf);
				////////
				fileSize = y;// /= 2;
				////////
				if (FALSE == CheckBuf())
				{
					if (badformat)
					{// following is copied from ReStart
						MessageBox(hwnd, TEXT("The Gedcom data file had errors that have been fixed.\n\nIt has been saved with a .bak filename extension.\n\nIt should look okay when you re-open it."), ERROR, MB_OK);
						for (x = 0; (FullFilename[x] != '.') && (FullFilename[x] != 0); x++)
							BackupFilename[x] = FullFilename[x];
						if (BackupFilename[x-4] == '.') {
							BackupFilename[x-3] = 'b';
							BackupFilename[x-2] = 'a';
							BackupFilename[x-1] = 'k';
						}
						else {
							BackupFilename[x++] = '.';
							BackupFilename[x++] = 'b';
							BackupFilename[x++] = 'a';
							BackupFilename[x++] = 'k';
						}
						BackupFilename[x] = 0;
						CopyFile(FullFilename, BackupFilename, FALSE);//overwrites existing backup file
						utf = (BYTE*)VirtualAlloc(NULL, 2*fileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
						for (x = 0, y = 0; x < fileSize; x++)
						{
							if (Buf[x] < 0x80)
								utf[y++] = *(BYTE*)&Buf[x];
							else if (Buf[x] < 0x800)
							{
								utf[y++] = ((BYTE)(MASK2BYTES | Buf[x] >> 6));
								utf[y++] = ((BYTE)(MASKBYTE | Buf[x] & MASKBITS));
							}
							else if(Buf[x] < 0x10000)
							{
								utf[y++] = ((BYTE)(MASK3BYTES | Buf[x] >> 12));
								utf[y++] = ((BYTE)(MASKBYTE | Buf[x] >> 6 & MASKBITS));
								utf[y++] = ((BYTE)(MASKBYTE | Buf[x] & MASKBITS));
							}
							else
								MessageBox(hwnd, TEXT("HUH?"), NULL, MB_OK);
						}
						hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
						if (0 == WriteFile(hFile, utf, y, &dwBytesWritten, NULL))
							MessageBox(hwnd, FullFilename, TEXT("This file was NOT written:"), MB_OK);
						else
							FlushFileBuffers(hFile);
						CloseHandle(hFile);
						VirtualFree(utf, 0, MEM_RELEASE);

						badend = FALSE;
						for (y = fileSize; (Buf[y] != 'R'); y--)
							if ((Buf[y] != '\n') && (Buf[y] != 0) && (Buf[y] != '\r'))
								badend = TRUE;
						if (badend)
							MessageBox(hwnd, TEXT("EXTRA CHARACTERS AT END!"), NULL, MB_OK);
						y--;
					}
					DestroyWindow(hwnd);
					return;
				}
			}
			else//if (fileSize == 0)
			{
				CloseHandle(hFile);
//				MessageBox(hwnd, "...is empty.  Deleting it...", FullFilename, MB_OK);
//				DeleteFile(FullFilename);
				return;
			}
		}
		else//new file
			return;
	}
	for (x = 0, LastIndiv = 0; x < fileSize; x++)
	{
		if ((Buf[x] == '0') && (Buf[x+1] == ' ') && (Buf[x+2] == '@') && ((Buf[x-1] == '\n') || (Buf[x-1] == '\r')))
		{
			for (y = x; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
			{
				if ((Buf[y] == 'I') && (Buf[y+1] == 'N') && (Buf[y+2] == 'D') && (Buf[y+3] == 'I')) 
				{
					LastIndiv++;
					break;
				}
			}
		}
	}
	LastIndiv++;
	if (Indiv != NULL)
	{
		VirtualFree(Indiv, 0, MEM_RELEASE);
		Indiv = NULL;
	}
	Indiv = (struct INDIV*)VirtualAlloc(NULL, (LastIndiv + EXTRAINDIVIDUALS) * sizeof(struct INDIV), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	for (x = 0 ; x < (DWORD)(LastIndiv+EXTRAINDIVIDUALS); x++)//+EXTRAINDIVIDUALS for duplicates
	{
		Indiv[x].Num = 0;
		Indiv[x].Childof = 0;
		for (y = 0; y < MAXSPOUSES; y++)
			Indiv[x].Spouseof[y] = 0;
		Indiv[x].Name = 0;
		Indiv[x].Note = 0;
		Indiv[x].Sex = 0;
		Indiv[x].Birth = 0;
		Indiv[x].BirthLoc = 0;
		Indiv[x].Death = 0;
		Indiv[x].DeathLoc = 0;
		Indiv[x].Parent = 0;
		Indiv[x].Spouse = 0;
		Indiv[x].Child = 0;
		Indiv[x].cx = 0;
		Indiv[x].X = 0;
		Indiv[x].LeftChild = 0;
		Indiv[x].RightChild = 0;
		Indiv[x].ArrayX = 0;
		Indiv[x].ArrayY = 0;
		Indiv[x].Flags = 0;
	}
	oi = 0;

	gotnum = FALSE;
	gotname = TRUE;
	gotsex = TRUE;
	i = 0;
	Line = 1;
	for (x = 0; x < fileSize; x++)
	{
x22:	if ((Buf[x] == '\r') || (Buf[x] == '\n'))
		{
			Line++;
			if (Buf[x] == '\r')
				x++;
			if (x < (fileSize-1))
			{
				if (((Buf[x+1] < '0') || (Buf[x+1] > '9'))
				 || (Buf[x+2] != ' ')
				 || ((Buf[x+3] >= '0') && (Buf[x+3] <= '9')))
				{//bad format in gedcom file
//					DWORD X, Y;

					if ((Buf[x+1] == '\r') && (Buf[x+2] == '\n'))
					{//a blank line
						x += 2;
						goto x22;
					}
					for (badX = badY = x+1; ((badX-badY) < 300) && (Buf[badX] != '\r') && (Buf[badX] != '\n'); badX++)
						;
//					ch = Buf[badX];
//					Buf[badX] = 0;
//					_snwprintf(Error, 512, TEXT("The line  \"%s\"  in\n%s\ndoesn't start with a number and then a space.\nFix it in Notepad by making it conform to\nthe format of similar but good lines,\nand tell me (jdmcox@jdmcox.com) about it.\n\nBUT BEFORE YOU DO THAT, SEE THE NEXT MESSAGE..."), &Buf[badY], FullFilename);
//					Buf[badX] = ch;
//FixGed();
//					MessageBox(hwnd, Error, Filename, MB_OK);
//					UndoIt();
					BadLine = x+1;
					MessageBox(hwnd, TEXT("is missing a number at the beginning of a line"), Filename, MB_OK);
					if (DialogBox(hInst, TEXT("FIXBUG"), hwnd, FixBugProc)) {
						utf = (BYTE*)VirtualAlloc(NULL, 2*fileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
						for (x = 0, y = 0; x < fileSize; x++)
						{
							if (Buf[x] < 0x80)
								utf[y++] = *(BYTE*)&Buf[x];
							else if (Buf[x] < 0x800)
							{
								utf[y++] = ((BYTE)(MASK2BYTES | Buf[x] >> 6));
								utf[y++] = ((BYTE)(MASKBYTE | Buf[x] & MASKBITS));
							}
							else if(Buf[x] < 0x10000)
							{
								utf[y++] = ((BYTE)(MASK3BYTES | Buf[x] >> 12));
								utf[y++] = ((BYTE)(MASKBYTE | Buf[x] >> 6 & MASKBITS));
								utf[y++] = ((BYTE)(MASKBYTE | Buf[x] & MASKBITS));
							}
							else
								MessageBox(hwnd, TEXT("HUH?"), NULL, MB_OK);
						}
						hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
						if (0 == WriteFile(hFile, utf, y, &dwBytesWritten, NULL))
							MessageBox(hwnd, FullFilename, TEXT("This file was NOT written:"), MB_OK);
						else
							FlushFileBuffers(hFile);
						CloseHandle(hFile);
						VirtualFree(utf, 0, MEM_RELEASE);
						DestroyWindow(hwnd);
					}
					else
						UndoIt();
					return;
				}
			}
			if ((Buf[x+1] == '0') && (Buf[x+2] == ' ') && (Buf[x+3] == '@'))
			{
				x += 4;//to possible individual's number
				for (y = x; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
					if ((Buf[y] == 'I') && (Buf[y+1] == 'N') && (Buf[y+2] == 'D') && (Buf[y+3] == 'I'))
						break;
				if (Buf[y] == 'I')
				{//INDI
					if (gotname == FALSE)
					{// no name for previous indiv
//						_snwprintf(Error, 512, TEXT("Missing a line like \"1 FirstName /LASTNAME/\"\n after the line \"0 @I%u@ INDI\" line\nin %s\n(which can be edited in Notepad)"), Indiv[i].Num, FullFilename);
//						MessageBox(hwnd, Error, Filename, MB_OK);
//						DestroyWindow(hwnd);
//						return;
						y = PossibleMissingName; // new 24 Aug 2010
						for (z = fileSize; z >= y; z--)
							Buf[z+18] = Buf[z];
						for (w = 0; w < 18; w++, y++)
							Buf[y] = UnKnown[w];
						fileSize += 18;
						ReStart();
						return;
					}
					else // new 24 Aug 2010
					{
						for (y += 4; (y < fileSize) && (Buf[y] != '\n'); y++)
							;
						if (y < fileSize)
							PossibleMissingName = y + 1;
					}
//					if (gotsex == FALSE)
//					{
//						_snwprintf(Error, 512, TEXT("Missing SEX after 0 @I%d@ INDI\nin %s\n(which can be edited in Notepad)"), Indiv[i].Num, FullFilename);
//						MessageBox(hwnd, Error, Filename, MB_OK);
//						DestroyWindow(hwnd);
//						return;
//					}
					gotspouse = 0xFFFF;
					Fams = x;
					gotname = FALSE;
					gotsex = FALSE;
					famchild = 0;
					birthoffset = deathoffset = birthlocoffset = deathlocoffset = 0xFFFFFFFF;
					i++;
					if (Buf[x] >= 'A')
						x++;//ignore leading letter
					for (Indiv[i].Num = 0; Buf[x] != '@'; x++)
						Indiv[i].Num = (Indiv[i].Num * 10) + (Buf[x] - '0');
					if ((Indiv[i].Num == IndivNumber) && (fromnewlink == FALSE))
					{
						if ((IndivNum == 0xFFFFFFFF) && (highlighted == 0xFFFFFFFF))
						{
							IndivNum = i;
							highlighted = i;
						}
					}
					gotnum = TRUE;
				}
				else
					gotnum = FALSE;
			}
			else if (gotnum == FALSE)
				continue;
			else if ((Buf[x+1] == '1') && (Buf[x+2] == ' ') && (Buf[x+3] == 'N') && (Buf[x+4] == 'A') && (Buf[x+5] == 'M') && (Buf[x+6] == 'E'))//1 NAME 
			{//NAME
				if (gotname)
					continue;
				x += 8;
				for (y = x; (Buf[y] != '\n') && (y < fileSize); y++) {
					if (Buf[y] == '/') {
						for ( ; (Buf[y] != '\n') && (y < fileSize); y++) {
							if (Buf[y] == '/') {
								gotname = TRUE;
								goto x0;
							}
						}
					}
				}
x0:			Indiv[i].Name = x;
			}
			else if ((Buf[x+1] == '1') && (Buf[x+2] == ' ') && (Buf[x+3] == 'S') && (Buf[x+4] == 'E') && (Buf[x+5] == 'X'))
			{//SEX
				if (Buf[x+7] == 'M')
				{
					Indiv[i].Sex = 1;
					gotsex = TRUE;
				}
				else if (Buf[x+7] == 'F')
				{
					Indiv[i].Sex = 0xFFFF;
					gotsex = TRUE;
				}
			}
			else if ((Buf[x+1] == '1') && (Buf[x+2] == ' ') && (Buf[x+3] == 'B') && (Buf[x+4] == 'I') && (Buf[x+5] == 'R') && (Buf[x+6] == 'T'))
			{//BIRT
				while (TRUE) {
 					for (x++; (x < fileSize) && (Buf[x] != '\n'); x++)
						;
					if (Buf[x+1] != '2') {
						x--;
						break;
					}
					if ((Buf[x+1] == '2') && (Buf[x+2] == ' ') && (Buf[x+3] == 'D') && (Buf[x+4] == 'A') && (Buf[x+8] != '\r') && (Buf[x+8] != '\n'))//DATE after \r\n
					{//2 BIRT 13 APR 1938
						x += 8;
						if (birthoffset == 0xFFFFFFFF)
						{
							birthoffset = x;
							Indiv[i].Birth = x;
							for ( ; Buf[x] != '\n'; x++)
								;
							if ((Buf[x+1] == '2') && (Buf[x+2] == ' ') && (Buf[x+3] == 'P') && (Buf[x+4] == 'L') && (Buf[x+5] == 'A') && (Buf[x+6] == 'C'))
							{
								x += 8;
								if (birthlocoffset == 0xFFFFFFFF)
									birthlocoffset = x;
								else
									Indiv[i].Flags |= 0x10;
								Indiv[i].BirthLoc = x;
							}
							else
								x--;
						}
						else
						{
							Indiv[i].Flags |= 1;
//							for (y = Indiv[i].Name; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
//								;
//							ch = Buf[y];
//							Buf[y] = 0;
//							ErrorLen = _snwprintf(Error, "%s has more than one birth date.\r\n", &Buf[Indiv[i].Name]);
//							Buf[y] = ch;
//							WriteFile(hErrorFile, Error, ErrorLen, &dwBytesWritten, NULL);
						}	
					}
					else if ((Buf[x+1] == '2') && (Buf[x+2] == ' ') && (Buf[x+3] == 'P') && (Buf[x+4] == 'L') && (Buf[x+5] == 'A') && (Buf[x+6] == 'C'))//PLAC after \r/n
					{
						x += 8;
						if (birthlocoffset == 0xFFFFFFFF)
						{
							birthlocoffset = x;
							Indiv[i].BirthLoc = x;
						}
						else
						{
							Indiv[i].Flags |= 0x10;
//							for (y = Indiv[i].Name; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
//								;
//							ch = Buf[y];
//							Buf[y] = 0;
//							ErrorLen = _snwprintf(Error, "%s has more than one birth place.\r\n", &Buf[Indiv[i].Name]);
//							Buf[y] = ch;
//							WriteFile(hErrorFile, Error, ErrorLen, &dwBytesWritten, NULL);
						}	
					}
//					else
//						x--;
				}
			}
			else if ((Buf[x+1] == '1') && (Buf[x+2] == ' ') && (Buf[x+3] == 'D') && (Buf[x+4] == 'E') && (Buf[x+5] == 'A') && (Buf[x+6] == 'T'))
			{//DEAT
				while (TRUE) {
					for (x++; (x < fileSize) && (Buf[x] != '\n'); x++)
						;
					if (Buf[x+1] != '2') {
						x--;
						break;
					}
					if ((Buf[x+1] == '2') && (Buf[x+2] == ' ') && (Buf[x+3] == 'D') && (Buf[x+4] == 'A') && (Buf[x+8] != '\r') && (Buf[x+8] != '\n'))//DATE after \r\n
					{//2 BIRT 13 APR 1938
						x += 8;
						if (deathoffset == 0xFFFFFFFF)
						{
							deathoffset = x;
							Indiv[i].Death = x;
							for ( ; Buf[x] != '\n'; x++)
								;
							if ((Buf[x+1] == '2') && (Buf[x+2] == ' ') && (Buf[x+3] == 'P') && (Buf[x+4] == 'L') && (Buf[x+5] == 'A') && (Buf[x+6] == 'C'))
							{
								x += 8;
								if (deathlocoffset == 0xFFFFFFFF)
									deathlocoffset = x;
								else
									Indiv[i].Flags |= 0x1000;
								Indiv[i].DeathLoc = x;
							}
							else
								x--;
						}
						else
						{
							Indiv[i].Flags |= 0x100;
//							for (y = Indiv[i].Name; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
//								;
//							ch = Buf[y];
//							Buf[y] = 0;
//							ErrorLen = _snwprintf(Error, "%s has more than one death date.\r\n", &Buf[Indiv[i].Name]);
//							Buf[y] = ch;
//							WriteFile(hErrorFile, Error, ErrorLen, &dwBytesWritten, NULL);
						}	
					}
					else if ((Buf[x+1] == '2') && (Buf[x+2] == ' ') && (Buf[x+3] == 'P') && (Buf[x+4] == 'L') && (Buf[x+5] == 'A') && (Buf[x+6] == 'C'))//PLAC after \r\n
					{
						x += 8;
						if (deathlocoffset == 0xFFFFFFFF)
						{
							deathlocoffset = x;
							Indiv[i].DeathLoc = x;
						}
						else
						{
							Indiv[i].Flags |= 0x1000;
//						for (y = Indiv[i].Name; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
//							;
//						ch = Buf[y];
//						Buf[y] = 0;
//						ErrorLen = _snwprintf(Error, "%s has more than one death place.\r\n", &Buf[Indiv[i].Name]);
//						Buf[y] = ch;
//						WriteFile(hErrorFile, Error, ErrorLen, &dwBytesWritten, NULL);
						}	
					}
//					else
//						x--;
				}
			}
			else if ((Buf[x+1] == '1') && (Buf[x+2] == ' ') && (Buf[x+3] == 'F') && (Buf[x+4] == 'A') && (Buf[x+5] == 'M'))
			{
				famcfirst = TRUE;
				if (Buf[x+6] == 'C')//child of
				{//FAMC
					if (famchild < 2)
					{
						if (famchild == 1)
							Indiv[i].Flags |= 0x10000;
						famchild++;
						if (gotspouse == 1)
							famcfirst = FALSE;
						x += 9;//to family number
						if (Buf[x] >= 'A')
							x++;//ignore leading letter
						for (Indiv[i].Childof = 0 ; Buf[x] != '@'; x++)
							Indiv[i].Childof = (Indiv[i].Childof * 10) + (Buf[x] - '0');
					}
					else
					{
						for (y = Indiv[i].Name; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
							;
						ch = Buf[y];
						Buf[y] = 0;
						MessageBox(hwnd, TEXT("In the Gedcom data file, %s is a child of more than two couples!\nChange it in Notepad"), &Buf[Indiv[i].Name], MB_OK);
						Buf[y] = ch;
					}
				}
				else if (Buf[x+6] == 'S')//spouse of
				{//FAMS
					gotspouse = 1;
					x += 9;//to family number
					if (Buf[x] >= 'A')
						x++;//ignore leading letter
					for (y = 0; y < MAXSPOUSES; y++)
					{
						if (Indiv[i].Spouseof[y] == 0)
						{
							for ( ; Buf[x] != '@'; x++)
								Indiv[i].Spouseof[y] = (Indiv[i].Spouseof[y] * 10) + (Buf[x] - '0');
						}
					}
				}
			}
			else if ((Buf[x+1] == '1') && (Buf[x+2] == ' ') && (Buf[x+3] == 'N') && (Buf[x+4] == 'O') && (Buf[x+5] == 'T') && (Buf[x+6] == 'E'))
			{//NOTE
				Indiv[i].Note = x+8;
			}
		}//end of if ((Buf[x] == '\r') || (Buf[x] == '\n'))
	}//end of for (x = 0

	LastIndivNum = Fams;
	for (y = 0; Fams < fileSize; Fams++)
	{//point to 0 @F1@ FAM
		if ((Buf[Fams-1] == '\n') && (Buf[Fams] == '0'))
		{
			if ((Buf[Fams+2] == 'T') && (Buf[Fams+3] == 'R') && (Buf[Fams+4] == 'L') && (Buf[Fams+5] == 'R'))
				break;
			for (x = Fams; (x < fileSize) && (Buf[x] != '\n'); x++)
			{
				if ((Buf[x] == 'F') && (Buf[x+1] == 'A') && (Buf[x+2] == 'M'))
					goto gotfams;
			}
		}
	} 
gotfams:;
	if (gotname == FALSE)
	{
		_snwprintf(Error, 512, TEXT("IMPOSSIBLE:\nMissing a line like \"1 FirstName /LASTNAME/\" after the \"0 @I%u@ INDI\" line\nin %s\n(which can be edited in Notepad)"), Indiv[i].Num, FullFilename);
		MessageBox(hwnd, Error, Filename, MB_OK);
		DestroyWindow(hwnd);
		return;
	}
//	if (gotsex == FALSE)
//	{
//		_snwprintf(Error, "In the Gedcom file, SEX is missing after 0 @%d@ INDI line\nin %s\n(which can be edited in Notepad)", Indiv[i].Num, FullFilename);
//		MessageBox(hwnd, Error, Filename, MB_OK);
//		DestroyWindow(hwnd);
//		return;
//	}

	for (x = Fams; Buf[x] != '\n'; x++)
	{
		if (Buf[x] == '@')
		{
			x++;
			if ((Buf[x] < '0') || (Buf[x] > '9'))
				x++;
			if ((Buf[x] == '0') && (Buf[x+1] == '@'))
			{
				for ( ; (Buf[x] != '\r') && (Buf[x] != '\n'); x++)
					;
				ch = Buf[x];
				Buf[x] = 0;
				_snwprintf(Error, 512, TEXT("The line\n%s in the Gedcom file\nis going to cause problems\nif you edit anything."), &Buf[Fams]);
				MessageBox(hwnd, Error, Filename, MB_OK);
				Buf[x] = ch;
			} 
		}
	}

	for (FamsEnd = Fams; FamsEnd < fileSize; FamsEnd++)
	{//get FamsEnd
		if ((Buf[FamsEnd-1] == '\n') && (Buf[FamsEnd] == '0') && (Buf[FamsEnd+2] != '@'))
			break;
	}
	i++;
	LastIndiv = i;
	RealLastIndiv = i;
	fromnewlink = FALSE;

	//check for copies of spouse in Spouseof
	for (x = 1; x < LastIndiv; x++)
	{
		for (s = 0; (s < MAXSPOUSES) && (Indiv[x].Spouseof[s]); s++)
			if (Indiv[x].Spouseof[s] == Indiv[x].Spouseof[s+1])
			{
				for (y = Indiv[x].Name; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
					;
				ch = Buf[y];
				Buf[y] = 0;
				ErrorLen = _snwprintf(Error, 512, TEXT("has multiple occurrences of the same spouse in the Gedcom file.\r\nOpen %s\r\nin Notepad and delete the extra ones.\r\n"), Filename);
				MessageBox(hwnd, Error, &Buf[Indiv[x].Name], MB_OK);
				Buf[y] = ch;
				for (s = MAXSPOUSES; s; s--)
				{//fix Indiv[x].Spouseof
					if (Indiv[x].Spouseof[s] == Indiv[x].Spouseof[s-1])
						Indiv[x].Spouseof[s] = 0;
				}
				break;
			}
	}

	//check for multiple HUSB & WIFE & CHIL(with same @Inumber@) in FAM records
	for (x = Fams; x < FamsEnd; x++)
	{//0 @F123@ FAM
		if (Buf[x-1] == '\n')
		{
			if ((Buf[x] == '0') && (Buf[x+2] == '@'))// && (Buf[x+3] == 'F')
			{
//
				gotit = FALSE;
				test = x + 3;
				if ((Buf[test] < '0') || (Buf[test] > '9'))
					test++;
				for (test2 = test; Buf[test2] != '\n'; test2++)
				{
					if ((Buf[test2] == 'F') && (Buf[test2+1] == 'A')  && (Buf[test2+2] == 'M')) 
					{
						gotit = TRUE;
						break;
					}
				}
				if (gotit == FALSE)
					continue;
//
				husb = wife = num = 0;
				for (y = 0; y < 21; y++)
					chil[y] = 0;
//				y = x+3;
//				if ((Buf[y] < '0') || (Buf[y] > '9'))
//					y++;
//				fams = Atoi(&Buf[y]);
				fams = Atoi(&Buf[test]);
				for (y = x, z = 0; (z < 512) && (Buf[y] != '\r') && (Buf[y] != '\n'); y++, z++)
					temp[z] = Buf[y];
				temp[z] = 0;
			}
			if ((Buf[x] == '1') && (Buf[x+2] == 'H') && (Buf[x+3] == 'U') && (Buf[x+4] == 'S'))
			{
				y = x+8;
				if ((Buf[y] < '0') || (Buf[y] > '9'))
					y++;
				if (husb)
				{
//					indivNum = Atoi(&Buf[y]);
//					for (i = 0; i < LastIndiv; i++)
//					{
//						if (Indiv[i].Num == indivNum)
//						{
//							for (s = 0; (s < MAXSPOUSES) && Indiv[i].Spouseof[s]; s++)
//							{
//								if ((Indiv[i].Spouseof[s] == fams) && (Indiv[i].Sex == 1))
//									x=x;
//							}
//						}
//					}
					ErrorLen = _snwprintf(Error, 512, TEXT("Family record %s in the Gedcom file\nhas more than one HUSB (husband). Fix it in Notepad."), temp);
					MessageBox(hwnd, Error, Filename, MB_OK);
				}
				else
//					husb = Atoi(&Buf[y]);
					husb = fams;
			}
			if ((Buf[x] == '1') && (Buf[x+2] == 'W') && (Buf[x+3] == 'I') && (Buf[x+4] == 'F'))
			{
				if (wife)
				{
					ErrorLen = _snwprintf(Error, 512, TEXT("Family record %s in the Gedcom file\nhas more than one WIFE. Fix it in Notepad."), temp);
					MessageBox(hwnd, Error, Filename, MB_OK);
//badY = x;
//FixGed();
				}
				else
					wife++;
			}
			if ((Buf[x] == '1') && (Buf[x+2] == 'C') && (Buf[x+3] == 'H') && (Buf[x+4] == 'I'))
			{
				y = x+8;
				if ((Buf[y] < '0') || (Buf[y] > '9'))
					y++;
				childnum = Atoi(&Buf[y]);
				for (y = 0; chil[y] != 0; y++)
				{
					if (childnum == chil[y])
					{
						ErrorLen = _snwprintf(Error, 512, TEXT("Family record %s in the Gedcom file has same CHIL (child) repeated.\r\nFix it in Notepad"), temp);
						MessageBox(hwnd, Error, Filename, MB_OK);
					}
				}
				chil[num++] = childnum; 
			}
		}
	}
	for (i = 0; i < LastIndiv; i++)// to the same folder as the file just opened.
	{//move FAMS for single parent to Spouseof[0] (make it the first spouseof)
		for (s = 1; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s]); s++)
		{
			for (w = 0; w < LastIndiv; w++)
			{
				if (w == i)
					continue;
				for (ws = 0; (ws < MAXSPOUSES) && (Indiv[w].Spouseof[ws]); ws++)
				{
					if ((Indiv[w].Spouseof[ws] & 0x7FFFFFFF) == (Indiv[i].Spouseof[s] & 0x7FFFFFFF))
					{
						goto x4;//single parent not found yet
					}
				}
			}
			v = Indiv[i].Spouseof[s];
			for (w = 1; w <= s; w++)
				Indiv[i].Spouseof[w] = Indiv[i].Spouseof[w-1];
			Indiv[i].Spouseof[0] = v;//done
			goto x4;
		}
x4:;
	}
	if (LastIndiv == 2)
	{//one time only special deal
		highlighted = 1;
		Backspace[1] = 1;
		IndivNum = 1;
	}
	SortSiblings();
	CheckFAMS();

	if ((highlighted != 0xFFFFFFFF) && (Buf))
		///////////
		FillArray();
		///////////
}


void FillArray(void)
{
	SetCursor(hWaitingCursor);
	CheckForPhotos();
	for (y = 0; y < ROWS; y++)
	{
		for (x = 0; x < COLS; x++)
		{
			Array[y][x] = 0xFFFF;
		}
		indiv[y] = 0;
		Col[y] = 0;
	}
	z = highlighted;
	ArrayUp = (DWORD*)calloc(1, ROWSUP*BIGCOLS*sizeof(DWORD));
	ArrayUp[MIDROW*BIGCOLS] = highlighted;
	Array[MIDROW][0] = highlighted;
	TotalParents = 2;
	for (row = MIDROW - 1, notempty = TRUE; (notempty) && (row > 5); row--, TotalParents *= 2)//row > 3
	{//put ancestors in ArrayUp
		notempty = FALSE;
		for (col = 0, Child = 0, Parents = 0, ptr = 0; Child < (TotalParents/2); Child++, col += 2)
		{//get all parents
			if (ptr > COLS)
			{
				MessageBox(hwnd, TEXT("Too many columns"), ERROR, MB_OK);
				highlighted = oldHighlighted;
				bs--;
				return;
			}	
			z = ArrayUp[((row+1)*BIGCOLS)+Child];
			if (z == 0)
				continue;
			v = Indiv[z].Childof;
			for (i = 1; i < RealLastIndiv; )
			{//get two parents
				for (parent = 0; i < RealLastIndiv; i++)
				{
					for (s = 0; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s] != 0); s++)
					{
						if (v == Indiv[i].Spouseof[s])
						{
							for (y = MIDROW-1; y >= row; y--)
							{
								for (x = 0; Array[y][x] != 0xFFFF; x++)
								{
									if (Array[y][x] == i)
									{//if duplicate individual in Indiv (e.g.married cousins)
										Indiv[LastIndiv] = Indiv[i];
										Indiv[LastIndiv].Child = TRUE;
										Indiv[z].Parent = LastIndiv;
										notempty = TRUE;
										ArrayUp[(row*BIGCOLS)+(col+parent)] = LastIndiv;
										Array[row][ptr++] = LastIndiv;
										if (parent == 1)//second parent
										{
											Indiv[LastIndiv].Spouse = FirstParent;
											Indiv[FirstParent].Spouse = LastIndiv++;
											goto x2;
										}
										FirstParent = LastIndiv++;
										parent++;
										goto x3;//found match
									}
								}
							}
							Indiv[i].Child = TRUE;
							Indiv[z].Parent = i;
							notempty = TRUE;
							ArrayUp[(row*BIGCOLS)+(col+parent)] = i;
							Array[row][ptr++] = i;
							if (parent == 1)//second parent
							{
								Indiv[i].Spouse = FirstParent;
								Indiv[FirstParent].Spouse = i;
								goto x2;
							}
							FirstParent = i;
							parent++;
							goto x3;//found match
						}
					}
x3:;
				}
			}//end of for (i = 1; i < LastIndiv; )
x2:			Parents += 2;
		}
	}
	RowsUp = row + 2;
	free(ArrayUp);

//get ancestors Indiv[I].cx
	for (y = MIDROW; y >= RowsUp; y--)
	{
		X = cxScreen*25;
		for (x = 0; Array[y][x] != 0xFFFF; x++)
		{
			I = Array[y][x];
			w = GetName();
			GetTextExtentPoint32(hdcMem, temp, w, &Size);
			Indiv[I].ArrayX = (WORD)x;
			Indiv[I].ArrayY = (WORD)y;
			Indiv[I].cx = (WORD)Size.cx;
			Indiv[I].X = X;
			X += Size.cx + 24;
		}
	}

//get ancestors Indiv[I].X
	endlessloop = 0;
begin:;
	if (endlessloop >= 280000)
	{
//	for (x = 0, biggest = 0; x < RealLastIndiv; x++)
//	{
//		if (Indiv[x].X > biggest)
//		{
//			biggest = Indiv[x].X;
//			z = x;
//		}
//	}
//	if (Indiv[z].X > 280000)
//	{	
		for (x = Indiv[highlighted].Name; (Buf[x] != '\r') && (Buf[x] != '\n'); x++)
			;
		ch = Buf[x];
		Buf[x] = 0;
		_snwprintf(Error, 512, TEXT("A loop didn't finish executing\nin the ancestor routine when\n%s\nwas selected."), &Buf[Indiv[highlighted].Name]);
		Buf[x] = ch;
		MessageBox(hwnd, Error, ERROR, MB_OK);
		highlighted = oldHighlighted;
		bs--;
		DestroyWindow(hwnd);
		return;
	}
	for (y = MIDROW; y >= RowsUp; y--)
	{
		for (x = 0; Array[y][x] != 0xFFFF; x++)
		{
			Child = Array[y][x];
			Parent = Indiv[Child].Parent;
			if (Parent)
			{
				LinefromChildX = Indiv[Child].X + (Indiv[Child].cx/2);
				ParentSpouse = Indiv[Parent].Spouse;
				if (ParentSpouse)
				{
					if (Indiv[ParentSpouse].Parent == FALSE)
						Indiv[ParentSpouse].X = Indiv[Parent].X - 24 - Indiv[ParentSpouse].cx;//special case
					SpouseLineLen = Indiv[Parent].X - (Indiv[ParentSpouse].X + Indiv[ParentSpouse].cx);
					LinetoChildX = Indiv[ParentSpouse].X + Indiv[ParentSpouse].cx + (SpouseLineLen/2);
					if (LinetoChildX != LinefromChildX)
					{
						oldX = Indiv[ParentSpouse].X;
						if (Indiv[ParentSpouse].ArrayX == 0)
						{//move to left
							Indiv[ParentSpouse].X = LinefromChildX - (SpouseLineLen/2) - Indiv[ParentSpouse].cx;
							newX = Indiv[ParentSpouse].X;
							Indiv[Parent].X += newX - oldX;
						}
						else
						{//move to right 
							if (Indiv[Parent].X < (LinefromChildX + (SpouseLineLen/2)))
							{
								Indiv[ParentSpouse].X = LinefromChildX - (SpouseLineLen/2) - Indiv[ParentSpouse].cx;
								newX = Indiv[ParentSpouse].X;
								Indiv[Parent].X += newX - oldX;
							}
							else
							{//move child to right
								oldX = Indiv[Child].X;
								Indiv[Child].X = LinetoChildX - (Indiv[Child].cx/2);
								newX = Indiv[Child].X;
								ArrayX = Indiv[Child].ArrayX;
								ArrayY = Indiv[Child].ArrayY;
								for (z = ArrayX+1; Array[y][z] != 0xFFFF; z++)
									Indiv[Array[y][z]].X += newX - oldX;
								endlessloop++;
								goto begin;
							}
						}
						ArrayX = Indiv[Parent].ArrayX;
						ArrayY = Indiv[Parent].ArrayY;
						for (z = ArrayX+1; Array[y-1][z] != 0xFFFF; z++)
							Indiv[Array[y-1][z]].X += newX - oldX;
					}
				}
				else//no ParentSpouse
				{
					LinetoChildX = Indiv[Parent].X + (Indiv[Parent].cx/2);
					if (LinetoChildX != LinefromChildX)
					{
						oldX = Indiv[Parent].X;
						if (Indiv[Parent].ArrayX == 0)
						{//move to left
							Indiv[Parent].X = LinefromChildX - (Indiv[Parent].cx/2);
							newX = Indiv[Parent].X;
						}
						else
						{//move to right 
							if (Indiv[Parent].X < (LinefromChildX - (Indiv[Parent].cx/2)))
							{
								Indiv[Parent].X = LinefromChildX - (Indiv[Parent].cx/2);
								newX = Indiv[Parent].X;
							}
							else
							{//move child to right
								oldX = Indiv[Child].X;
								Indiv[Child].X = LinetoChildX - (Indiv[Child].cx/2);
								newX = Indiv[Child].X;
								ArrayX = Indiv[Child].ArrayX;
								ArrayY = Indiv[Child].ArrayY;
								for (z = ArrayX+1; Array[y][z] != 0xFFFF; z++)
									Indiv[Array[y][z]].X += newX - oldX;
								endlessloop++;
								goto begin;
							}
						}
						ArrayX = Indiv[Parent].ArrayX;
						ArrayY = Indiv[Parent].ArrayY;
						for (z = ArrayX+1; Array[y-1][z] != 0xFFFF; z++)
							Indiv[Array[y-1][z]].X += newX - oldX;
					}
				}
			}
		}
	}

//MessageBox(hwnd, _itoa(endlessloop, asdf, 10), "", MB_OK);

	AncestorMinX = 0xFFFFFFFF;
	for (y = RowsUp; (y < ROWS) && (Array[y][0] != 0xFFFF) && (y < (MIDROW+1)); y++)
		if (Indiv[Array[y][0]].X < AncestorMinX)
			AncestorMinX = Indiv[Array[y][0]].X;
	AncestorMinX -= 50;
	for (y = RowsUp; y < (MIDROW+1); y++)
		for (x = 0; Array[y][x] != 0xFFFF; x++)
		{
			Indiv[Array[y][x]].X -=	AncestorMinX;
			Indiv[Array[y][x]].Flags |= 0x1000000;
		}
	HighlightedAncestorX = Indiv[highlighted].X + (Indiv[highlighted].cx/2);


//get descendants Array[y][x]
	firstime = TRUE;
	inbox = FALSE;
	row = MIDROW;
	w = highlighted;
	Array[row][Col[row]++] = highlighted;
	if (Indiv[highlighted].Spouseof[0])
	{//fill Array
		while (TRUE)
		{
			indiv[row] = highlighted;
			if ((GetSpouse() == FALSE) && (firstime == FALSE))//tricky
				break;//exit routine
			firstime = FALSE;
			do
			{
				if (GetChild())
				{//look for spouse of child
					if (FALSE == GetSpouse())//if no spouse of child
						row--;//back to where it was
				}
				else if (w == LastIndiv)//no child
					row--;//back to where it was
				if (nogood)
				{
					highlighted = oldHighlighted;
					bs--;
					DestroyWindow(hwnd);
					return;
				}
			} while (row >= MIDROW);
			row = MIDROW;
		}
	}

//get descendants Indiv[I].cx
	for (y = MIDROW; (y < ROWS) && (Array[y][0] != 0xFFFF); y++)
	{
		X = cxScreen*25;
		for (x = 0; Array[y][x] != 0xFFFF; x++)
		{
	 		I = Array[y][x];
			w = GetName();
			GetTextExtentPoint32(hdcMem, temp, w, &Size);
			Indiv[I].cx = (WORD)Size.cx;
			Indiv[I].X = X;
			X += Size.cx + 24;
		}
	}
for (twice = 2; twice; twice--)
{
//get descendants Indiv[i].X
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
	Loops[0] = '0';
	endlessloop = 0;
begin2:
	for (y = MIDROW+1; (y < ROWS) && (Array[y][0] != 0xFFFF); y++)
	{
		prevRightchild = 0xFFFF;
		for (parentX = 0; Array[y-1][parentX] != 0xFFFF; parentX++)
		{
			if (Indiv[Array[y-1][parentX]].Child)
			{
				gotleftchild = FALSE;
				for (x = 0; Array[y][x] != 0xFFFF; x++)
				{
					if (Indiv[Array[y][x]].Parent == Array[y-1][parentX])
					{
						if (gotleftchild == FALSE)
						{
							gotleftchild = TRUE;
							Leftchild = x;
							Indiv[Array[y][x]].LeftChild = TRUE;
						}
						if (gotleftchild)
							Rightchild = x;
					}
				}
				if (gotleftchild)
				{
					if (Indiv[Array[y][Rightchild]].X != Indiv[Array[y][Leftchild]].X)
						Indiv[Array[y][Rightchild]].RightChild = TRUE;
					else
						Indiv[Array[y][Leftchild]].LeftChild = FALSE;
 					if (Rightchild > Leftchild)
						ChildLineLength = (Indiv[Array[y][Rightchild]].X + (Indiv[Array[y][Rightchild]].cx/2))
						 - (Indiv[Array[y][Leftchild]].X + (Indiv[Array[y][Leftchild]].cx/2));
					else
						ChildLineLength = 0;
					if (Indiv[Array[y-1][parentX]].Spouse)
						LinetoChildrenX = Indiv[Array[y-1][parentX]].X - 12;
					else
						LinetoChildrenX = Indiv[Array[y-1][parentX]].X + (Indiv[Array[y-1][parentX]].cx/2);
					LinefromChildrenX = Indiv[Array[y][Leftchild]].X + (Indiv[Array[y][Leftchild]].cx/2) + (ChildLineLength/2);

					if (LinetoChildrenX > LinefromChildrenX)
					{
						for (x = Leftchild; Array[y][x] != 0xFFFF; x++)//x <= Rightchild
							Indiv[Array[y][x]].X += LinetoChildrenX - LinefromChildrenX;//move children to the right
					}

					else if (LinetoChildrenX != LinefromChildrenX)
					{//move parents to the right & start over
						oldX = Indiv[Array[y-1][parentX]].X;
						////////////////////////////
						Indiv[Array[y-1][parentX]].X = LinefromChildrenX + 12;//move parent to right
						////////////////////////////
						newX = Indiv[Array[y-1][parentX]].X - oldX;// how far parent moved to right
						for (s = 0; (s < MAXSPOUSES) && (Indiv[Array[y-1][parentX]].Spouseof[s]); s++)
						{
							spouseof = Indiv[Array[y-1][parentX]].Spouseof[s] & 0x7FFFFFFF;
							for (z = parentX-1; z != 0xFFFFFFFF; z--)
							{
								for (ws = 0; (ws < MAXSPOUSES) && (Indiv[Array[y-1][z]].Spouseof[ws]); ws++)
								{
									if ((Indiv[Array[y-1][z]].Spouseof[ws] & 0x7FFFFFFF) == spouseof)
									{
										for (v = 1; parentX-v != 0xFFFFFFFF; v++)
										{
											if (Indiv[Array[y-1][parentX-v]].Child)
											{//not parent's spouse, but with a child
												for (x = 1; x != v; x++)
													Indiv[Array[y-1][parentX-v+x]].X += newX;
												break;
											}
											for (ws = 0; (ws < MAXSPOUSES) && (Indiv[Array[y-1][parentX-v]].Spouseof[ws]); ws++)
											{
												if ((Indiv[Array[y-1][parentX-v]].Spouseof[ws] & 0x7FFFFFFF) == spouseof)
												{
													for (z = parentX-v; (z != 0xFFFFFFFF) && (Indiv[Array[y-1][z]].Child == 0); z--)
														;
													z++;
													for ( ; z != parentX; z++)
														Indiv[Array[y-1][z]].X += newX;
													goto x1;
												}
											}
										}
										goto x1;
									}
								}
							}
						}
						for (z = parentX-1; (z != 0xFFFFFFFF) && (Indiv[Array[y-1][z]].Child == 0); z--)
						{
							; //if single parent
						}
						z++;
						for ( ; z < parentX; z++)
							Indiv[Array[y-1][z]].X += newX;
x1:					for (z = parentX+1; Array[y-1][z] != 0xFFFF; z++)
							Indiv[Array[y-1][z]].X += newX;//move individuals-to-right-of-parent to right
						endlessloop++;
						if ((endlessloop % 10000 == 0) && (xPos) && (yPos))
						{
							Loops[0]++;
							hdc = GetDC(hwnd);
							TextOut(hdc, xPos+20, yPos, Loops, 12);
							ReleaseDC(NULL, hdc);
						}
						if (endlessloop < 280000)
							goto begin2;
						else
						{
							MessageBox(hwnd, TEXT("A loop didn't finish executing.\nThere must be too much data."), ERROR, MB_OK);
							highlighted = oldHighlighted;
							bs--;
							DestroyWindow(hwnd);
							return;
						}
					}//end of else if (LinetoChildrenX != LinefromChildrenX)
				}//end of if (gotleftchild)
			}//end of if (Indiv[Array[y-1][parentX]].Child)
		}
	}
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

	SetCursor(hCursor);

	for (LastRow = MIDROW; (LastRow < ROWS) && (Array[LastRow][0] != 0xFFFF); LastRow++)
		;
	DependantMinX = 0xFFFFFFFF;
	for (y = MIDROW; y < LastRow; y++)
	{
		if (Indiv[Array[y][0]].X < DependantMinX)
		{
			DependantMinX = Indiv[Array[y][0]].X;
			row = (WORD)y;
		}
	}
	DependantMinX -= 50;

	for (y = MIDROW; y < LastRow; y++)
	{
		for (x = 0; Array[y][x] != 0xFFFF; x++)
		{
			Indiv[Array[y][x]].X -=	DependantMinX;
			Indiv[Array[y][x]].Flags |= 0x1000000;
		}
	}

//fill-in empty cols
	SmallestGap = 0xFFFFFFFF;
	y = row;//furthest-left row
	for (x = 0; Array[y][x] != 0xFFFF; x++)
	{
gaploop:
		z = Indiv[Array[y][x]].X + Indiv[Array[y][x]].cx + 24;// end of Indiv[Array[y][x]] + 24
		if ((Array[y][x+1] == 0xFFFF) || (Indiv[Array[y][x+1]].X > z))
		{//if at a gap
			w = Indiv[Array[y][x]].X;
			for (row = MIDROW; row < LastRow; row++)
			{
				for (col = 0; (Array[row][col] != 0xFFFF) && (Indiv[Array[row][col]].X <= z); col++)
				{
					if ((Indiv[Array[row][col]].X + Indiv[Array[row][col]].cx + 24) > z)
					{// if end of Indiv[Array[row][col]] > end of Indiv[Array[y][x]]
						x = col;
						y = row;
						goto gaploop;
					}
					else if (((Indiv[Array[row][col]].X + Indiv[Array[row][col]].cx + 24) > Indiv[Array[y][x]].X)
						  && ((Indiv[Array[row][col]].X + Indiv[Array[row][col]].cx + 24) <= z)
						  && (Array[row][col+1] != 0xFFFF)
						  && ((Indiv[Array[row][col]].X + Indiv[Array[row][col]].cx + 24) == Indiv[Array[row][col+1]].X)
						  && ((Indiv[Array[row][col+1]].X + Indiv[Array[row][col+1]].cx + 24) > z))
					{
						x = col+1;
						y = row;
						goto gaploop;
					}
				}
			}
			//find the gap size to fill
			for (row = MIDROW; row < LastRow; row++)
			{
				Col[row] = 0xFFFFFFFF;
				for (col = 0; Array[row][col] != 0xFFFF; col++)
				{
					if (Indiv[Array[row][col]].X > z)
					{
						Col[row] = col;
						Gap = Indiv[Array[row][col]].X - z;
						if (SmallestGap > Gap)
						{
							SmallestGap = Gap;
							x = col;
							y = row;
						}
						break;
					}
				}
			}
			if (SmallestGap != 0xFFFFFFFF)
			{
				for (row = MIDROW; row < LastRow; row++)
				{
					for (col = Col[row]; Array[row][col] != 0xFFFF; col++)
					{
						if ((Col[row] != 0xFFFFFFFF) && (Array[row][col] != 0xFFFF))
							Indiv[Array[row][col]].X -= SmallestGap;//fill the gap
					}
				}
			}
			else
				break;
			SmallestGap = 0xFFFFFFFF;
			goto gaploop;
		}// end of if ((Array[y][x+1]...
	}// end of for (x = 0; Array[y][x] != 0xFFFF; x++)

}// end of for (twice = 2; twice; twice--)

	HighlightedDependantX = Indiv[highlighted].X + (Indiv[highlighted].cx/2);
	if (HighlightedDependantX > HighlightedAncestorX)
	{
		z = HighlightedDependantX - HighlightedAncestorX;
		for (y = RowsUp; y < MIDROW; y++)
		{
			for (x = 0; Array[y][x] != 0xFFFF; x++)
			{
				Indiv[Array[y][x]].X +=	z;
			}
		}
	}
	else if (HighlightedAncestorX > HighlightedDependantX)
	{
		z = HighlightedAncestorX - HighlightedDependantX;
		for (y = MIDROW; y < LastRow; y++)
		{
			for (x = 0; Array[y][x] != 0xFFFF; x++)
			{
				Indiv[Array[y][x]].X +=	z;
			}
		}
	}

	Width = 0;
	for (y = RowsUp; y < LastRow; y++)
	{
		for (x = 0; Array[y][x] != 0xFFFF; x++)
		{
			if (Width < (int)Indiv[Array[y][x]].X)
				Width = Indiv[Array[y][x]].X;
		}
	}
	Width += 150;
	Height = (LastRow - RowsUp + 1) * YSpacing;

	if (rect.bottom)
		y = rect.bottom;
	else
		y = cyScreen;
	if (rect.right)
		x = rect.right;
	else
		x = cxScreen;
	if (Height > (int)y)
		Top = Height - y;
	else
		Top = 0;
	if (Width > (int)x)
		Left = Width - x;
	else
		Left = 0;
	if ((Indiv[highlighted].X + Indiv[highlighted].cx) > x)
		xLoc = Indiv[highlighted].X + Indiv[highlighted].cx - (x/2);
	else
		xLoc = 0;
	z = (MIDROW - RowsUp) * YSpacing;
	if (z > (DWORD)Top)
		yLoc = Top;
	else
		yLoc = z;
	xPrevious = -1;

	FillhdcMem();
	arrayfilled = TRUE;
}


void FillhdcMem(void)
{
	DWORD w;

	if (LastIndiv == 0)
		return;
	FillRect(hdcMem, &rectMem, hBrush);
	HighlightedRect.left = Indiv[Array[MIDROW][0]].X - xLoc - 5;
	HighlightedRect.right = Indiv[Array[MIDROW][0]].X - xLoc + Indiv[Array[MIDROW][0]].cx + 5;
	HighlightedRect.top = ((MIDROW-RowsUp+1)*YSpacing) - yLoc - 5;
	HighlightedRect.bottom = ((MIDROW-RowsUp+1)*YSpacing) - yLoc + Z + 5;
	FillRect(hdcMem, &HighlightedRect, hHighlightedBrush);
	for (Y = -yLoc, y = RowsUp; y < LastRow; y++)
	{
		Y += YSpacing;
		gotleftchild = FALSE;
		for (x = 0; Array[y][x] != 0xFFFF; x++)
		{
			I = Array[y][x];
			if (((Indiv[I].X + Indiv[I].cx + 5) >= (DWORD)xLoc) && ((Indiv[I].X - 5) <= (DWORD)(xLoc + cxScreen)))
			{//if box is on-screen
				X = Indiv[I].X - xLoc;
				Size.cx = Indiv[I].cx;
				w = GetName();
				TextOut(hdcMem, X, Y, temp, w);
				bd = 0;
				if (Indiv[I].Birth)
				{
					Birth = &Buf[Indiv[I].Birth];
					for (w = 0; (Birth[w] != '\r') && (Birth[w] != '\n'); w++)
					{
						if ((Birth[w] >= '0') && (Birth[w] <= '9') && (Birth[w+1] >= '0') && (Birth[w+1] <= '9') && (Birth[w+2] >= '0') && (Birth[w+2] <= '9') && (Birth[w+3] >= '0') && (Birth[w+3] <= '9'))
						{
							temp[bd++] = Birth[w];
							temp[bd++] = Birth[w+1];
							temp[bd++] = Birth[w+2];
							temp[bd++] = Birth[w+3];
							break;
						}
					}
				}
				if (Indiv[I].Death)
				{
					Death = &Buf[Indiv[I].Death];
					for (w = 0; (Death[w] != '\r') && (Death[w] != '\n'); w++)
					{
						if ((Death[w] >= '0') && (Death[w] <= '9') && (Death[w+1] >= '0') && (Death[w+1] <= '9') && (Death[w+2] >= '0') && (Death[w+2] <= '9') && (Death[w+3] >= '0') && (Death[w+3] <= '9'))
						{
							temp[bd++] = '-';
							temp[bd++] = Death[w];
							temp[bd++] = Death[w+1];
							temp[bd++] = Death[w+2];
							temp[bd++] = Death[w+3];
							break;
						}
					}
				}
				if (bd)
				{
x24:				GetTextExtentPoint32(hdcMem, temp, bd, &Size2);
					if (Size2.cx < (Size.cx+8))
						TextOut(hdcMem, X, Y+Size.cy, temp, bd);
					else if (bd > 4)
					{
						bd--;
						goto x24;//make it fit in box
					}
				}
				if ((photo) && (Indiv[I].Flags & 0x100000))//there's a photo of him/her
				{
					SetTextColor(hdcMem, RED);
					TextOut(hdcMem, X+Size.cx-2, Y+Size.cy+5, TEXT("P"), 1);
					SetTextColor(hdcMem, BLACK);
				}
				//draw box:
				MoveToEx(hdcMem, X-5, Y-5, NULL);
				LineTo(hdcMem, X-5, Y+5+Z);
				LineTo(hdcMem, X+5+Size.cx, Y+5+Z);
				LineTo(hdcMem, X+5+Size.cx, Y-5);
				LineTo(hdcMem, X-5, Y-5);
				if (Indiv[I].Flags & 0x10000000)//duplicate individual (an individual married a cousin)
				{
					boxRect.left = X-4;
					boxRect.top = Y+Z-4;
					boxRect.right = X+5;
					boxRect.bottom = Y+4+Z;
					FillRect(hdcMem, &boxRect, hYellowBrush);
				}
if ((kings) && (Indiv[I].Birth))
{// The Kings of Europe
	boxRect.left = X-4;
	boxRect.top = Y-4;
	boxRect.right = X+5;
	boxRect.bottom = Y+4;
	FillRect(hdcMem, &boxRect, hPurpleBrush);
}
				if (Indiv[I].Childof != 0)
				{//draw line up from all children
					MoveToEx(hdcMem, X + (Indiv[I].cx/2), Y-5, NULL);
					LineTo(hdcMem, X + (Indiv[I].cx/2), Y-12);
				}
				if (y < MIDROW)
				{
					if ((Array[y][x+1] != 0xFFFF) && (Indiv[I].Spouse == Array[y][x+1]))
					{//draw line down to child
						z = X + Size.cx + 5;
						SpouseLineLen = (Indiv[Array[y][x+1]].X - xLoc - 5) - z;
						MoveToEx(hdcMem, z + (SpouseLineLen/2), Y+Size.cy+2, NULL);
						LineTo(hdcMem, z + (SpouseLineLen/2), Y+YSpacing-5);//Y+Size.cy+30
					}
					else if ((y < MIDROW) && (Indiv[I].Child) && (Indiv[I].Spouse == 0))
					{//draw line down from single parent
						MoveToEx(hdcMem, X + (Size.cx/2), Y+Z+5, NULL);
						LineTo(hdcMem, X + (Size.cx/2), Y+YSpacing-5);//Y+Size.cy+30
					}
				}
				if (y >= MIDROW)
				{
					if ((Indiv[I].Parent) && (y != MIDROW))
					{//draw line up from descendants
						MoveToEx(hdcMem, X+(Size.cx/2), Y-5, NULL);
						LineTo(hdcMem, X+(Size.cx/2), Y-18);
					}
					if ((Indiv[I].Flags & 8) && (y != MIDROW))
					{//draw short line up from duplicate descendant
						MoveToEx(hdcMem, X+(Size.cx/2), Y-5, NULL);
						LineTo(hdcMem, X+(Size.cx/2), Y-12);
					}
					if (Indiv[I].Child)
					{//draw line down
						if (Indiv[I].Spouse)
						{
							MoveToEx(hdcMem, X-12, Y+Size.cy+2, NULL);
							LineTo(hdcMem, X-12, Y+Z+17);
						}
						else
						{
							MoveToEx(hdcMem, X + (Size.cx/2), Y+Z+5, NULL);
							LineTo(hdcMem, X + (Size.cx/2), Y+Z+17);
						}
					}
				}
			}//end of if box is on-screen
			else if (y < MIDROW)//and Indiv[I].X not on-screen
			{
				if ((Indiv[I].Spouse == Array[y][x+1]) && (Array[y][x+1] != 0xFFFF))
				{
					if ((Array[y][x+1] != 0xFFFF) && (((Indiv[Array[y][x+1]].X - 5) > (DWORD)xLoc) && (Indiv[Array[y][x+1]].X - 5) < (DWORD)(xLoc + cxScreen)))
					{//draw spouse lines if spouse is on-screen
						MoveToEx(hdcMem, 0, Y+Size.cy+2, NULL);
						LineTo(hdcMem, Indiv[Array[y][x+1]].X - xLoc - 5, Y+Size.cy+2);
						MoveToEx(hdcMem, 0, Y+Size.cy-2, NULL);
						LineTo(hdcMem, Indiv[Array[y][x+1]].X - xLoc - 5, Y+Size.cy-2);
					}
					SpouseLineLen = (Indiv[Array[y][x+1]].X - 5) - (Indiv[I].X + Indiv[I].cx);
					if (((Indiv[Array[y][x+1]].X - 5 - (SpouseLineLen/2)) > (DWORD)xLoc) && ((Indiv[Array[y][x+1]].X - 5 - (SpouseLineLen/2)) < (DWORD)(xLoc + cxScreen)))
					{//line down to child
						int addative = 2;
						if (SpouseLineLen & 1)
							addative++;
//						MoveToEx(hdcMem, addative + Indiv[Array[y][x+1]].X - xLoc - 5 - (SpouseLineLen/2), Y+Size.cy+2, NULL);
//						LineTo(hdcMem, addative + Indiv[Array[y][x+1]].X - xLoc - 5 - (SpouseLineLen/2), Y+YSpacing-5);//Y+Size.cy+30
						MoveToEx(hdcMem, addative + Indiv[I].X + Indiv[I].cx + (SpouseLineLen/2) - xLoc, Y+Size.cy+2, NULL);
						LineTo(hdcMem, addative + Indiv[I].X + Indiv[I].cx + (SpouseLineLen/2) - xLoc, Y+YSpacing-5);//Y+Size.cy+30
					}
				}
			}
			if (x && ((y >= MIDROW) && (Indiv[I].Spouse))
			 || ((y < MIDROW) && (Indiv[I].Spouse == Array[y][x-1])))
			{//draw spouse lines to left
				if ((Indiv[I].X - 5) > (DWORD)(xLoc + cxScreen))
				{
					if ((x) && ((Indiv[Array[y][x-1]].X + Indiv[Array[y][x-1]].cx + 5) > (WORD)xLoc))
					{//from right to on-screen
						MoveToEx(hdcMem, cxScreen, Y+Size.cy-2, NULL);
						LineTo(hdcMem, Indiv[Array[y][x-1]].X - xLoc + Indiv[Array[y][x-1]].cx + 5, Y+Size.cy-2);
						MoveToEx(hdcMem, cxScreen, Y+Size.cy+2, NULL);
						LineTo(hdcMem, Indiv[Array[y][x-1]].X - xLoc + Indiv[Array[y][x-1]].cx + 5, Y+Size.cy+2);
					}
					else
					{
						MoveToEx(hdcMem, cxScreen, Y+Size.cy-2, NULL);
						LineTo(hdcMem, 0, Y+Size.cy-2);
						MoveToEx(hdcMem, cxScreen, Y+Size.cy+2, NULL);
						LineTo(hdcMem, 0, Y+Size.cy+2);
					}
				}
				else if (((Indiv[I].X - 5) > (DWORD)xLoc)
					  && ((Indiv[I].X - 5) < (DWORD)(xLoc + cxScreen)))
				{//from on-screen
					if ((Indiv[Array[y][x-1]].X + Indiv[Array[y][x-1]].cx + 5) > (DWORD)xLoc)
					{//on-screen to on-screen
						MoveToEx(hdcMem, X-5, Y+Size.cy-2, NULL);
						LineTo(hdcMem, Indiv[Array[y][x-1]].X - xLoc + Indiv[Array[y][x-1]].cx + 5, Y+Size.cy-2);
						MoveToEx(hdcMem, X-5, Y+Size.cy+2, NULL);
						LineTo(hdcMem, Indiv[Array[y][x-1]].X - xLoc + Indiv[Array[y][x-1]].cx + 5, Y+Size.cy+2);
					}
					else//if ((Indiv[Array[y][x-1]].X + Indiv[Array[y][x-1]].cx + 5) < xLoc))
					{//from on-screen to left off-screen
						MoveToEx(hdcMem, X-5, Y+Size.cy-2, NULL);
						LineTo(hdcMem, 0, Y+Size.cy-2);
						MoveToEx(hdcMem, X-5, Y+Size.cy+2, NULL);
						LineTo(hdcMem, 0, Y+Size.cy+2);
					}
				}
			}

			if (y >= MIDROW)
			{
				if ((Indiv[I].LeftChild) && (gotleftchild == FALSE))
				{//draw sibling lines
					gotleftchild = TRUE;
					if ((Indiv[I].X + (Indiv[I].cx/2)) <= (DWORD)xLoc)
						Leftchild = 0;
					else if (((Indiv[I].X + (Indiv[I].cx/2)) > (DWORD)xLoc) && ((Indiv[I].X + (Indiv[I].cx/2)) < (DWORD)(xLoc + cxScreen)))
						Leftchild = Indiv[I].X - xLoc + (Indiv[I].cx/2);
					else
						Leftchild = 0xFFFFFFFF;//flag that it's to right of screen
				}
				if ((Indiv[I].RightChild) && (gotleftchild))
				{//draw sibling lines
					gotleftchild = FALSE;
					if ((Leftchild != 0xFFFFFFFF) && ((Indiv[I].X + (Indiv[I].cx/2)) >= (DWORD)(xLoc + cxScreen)) && (Indiv[I].LeftChild == FALSE))
					{
						MoveToEx(hdcMem, Leftchild, Y - 17, NULL);//connect siblings
						LineTo(hdcMem, cxScreen, Y - 17);
					}
					else if (((Indiv[I].X + (Indiv[I].cx/2)) > (DWORD)xLoc) && ((Indiv[I].X + (Indiv[I].cx/2)) < (DWORD)(xLoc + cxScreen)))
					{
						MoveToEx(hdcMem, Leftchild, Y - 17, NULL);//connect siblings
						LineTo(hdcMem, Indiv[I].X - xLoc + (Indiv[I].cx/2) + 1, Y - 17);
					}
				}
			}
		}
	}

	InvalidateRect(hwnd, &rect, FALSE);
	UpdateWindow(hwnd);
	if (updateindiv)
	{
		updateindiv = FALSE;
//		preditit = TRUE;
		hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);
		indivbox = FALSE;
	}
}

DWORD GetName(void)
{//abnornal name: Valentine/HOLLINGSWORTH/Sr, (came to America 1682)..
	int x;

	NameX = Indiv[I].Name;
	if (showmiddlename)
	{
		for (x = 0, w = NameX; (x < 255) && (Buf[w] != '\n') && (Buf[w] != '/'); w++, x++)// make the middle name(s) show
			temp[x] = Buf[w];
	}
	else
	{
		for (x = 0, w = NameX; (x < 255) && (Buf[w] != '\n') && (Buf[w] != ' ') && (Buf[w] != '/'); w++, x++)
			temp[x] = Buf[w];
	}

	if (Buf[w] == '/')
		temp[x] = ' ';
	else if ((x == 255) || (Buf[w] == '\n'))
		x = 0;
	else
		temp[x++] = ' ';
	for (z = w; (Buf[z] != '/') && (z < (NameX+255)); z++)
		;
	z++;//past '/'
	for ( ; Buf[z] != '/'; x++, z++)
	{
		temp[x] = Buf[z];
/*
		if ((temp[x] >= 'a') && (temp[x] <= 'z') && ((temp[x] != 'c') || (temp[x-1] != 'M')))
			temp[x] &= 0xDF;
		else if (temp[x] == -15)//ñ 
			temp[x] = 'N';
		else if (temp[x] == -31)//á
			temp[x] = 'A';
		else if (temp[x] == -23)//é
			temp[x] = 'E';
		else if (temp[x] == -19)//í
			temp[x] = 'I';
		else if (temp[x] == -13)//ó
			temp[x] = 'O';
		else if (temp[x] == -6)//ú
			temp[x] = 'U';
*/
	}
	temp[x] = 0;
	return x;
}


int CALLBACK FixBugProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD x, y, z, Buf2Len, FixBufLen;
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		for (x = BadLine-2, z = 0; (x != 0) && (z < 16); x--)
			if (Buf[x] == '\n')
				z++;
		x += 2; // past '\n'
		LineBegin = x;
		for (y = BadLine, z = 0; (y < fileSize) && (z <= 16); y++)
			if (Buf[y] == '\n')
				z++;
		LineEnd = y; // '\n' or Buf[fileSize]
		ch = Buf[LineEnd];
		Buf[LineEnd] = 0;
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hFont2, TRUE);
		SetWindowText(hwndEdit, &Buf[x]);
		Buf[LineEnd] = ch;
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			Buf2 = (TCHAR*)malloc((fileSize-LineEnd)*2);
			for (x = LineEnd, y = 0; x < fileSize; x++, y++)
				Buf2[y] = Buf[x];
			Buf2Len = y;
			FixBufLen = GetWindowText(hwndEdit, FixBuf, 10000);
			for (x = LineBegin, y = 0; y < FixBufLen; x++, y++)
				Buf[x] = FixBuf[y];
			for (z = 0; z < Buf2Len; x++, z++)
				Buf[x] = Buf2[z];
			fileSize = x;
			free(Buf2);
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK ListProc(HWND hwndListDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int dlgWidth;
	static TCHAR IndivInfo[] = TEXT("Show Individual's &Info");
	static TCHAR FamTree[] = TEXT("Show Individual's &Family Tree");
	static HWND hwndButton;

	switch (message)
	{
	case WM_INITDIALOG:
		list = TRUE;
		_snwprintf(temp, 256, TEXT("List of %i Individuals"), RealLastIndiv-1);
		SetWindowText(hwndListDlg, temp);
		index = -1;
		HighlightedName[0] = 0;//flag
		GetWindowRect(hwndListDlg, &dlgRect);
		dlgWidth = dlgRect.right-dlgRect.left;
		MoveWindow(hwndListDlg, cxSingleScreen-dlgWidth-10, IndivTop, dlgWidth, cyScreen-30, TRUE);
		hwndList = GetDlgItem(hwndListDlg, IDC_LIST1);
		hwndButton = GetDlgItem(hwndListDlg, IDC_BUTTON1);
		if (showindividual)
			SendMessage(hwndButton, WM_SETTEXT, 0, (LPARAM)FamTree);
		else
			SendMessage(hwndButton, WM_SETTEXT, 0, (LPARAM)IndivInfo);
		GetWindowRect(hwndList, &dlgRect);
		MoveWindow(hwndList, 0, 70, dlgWidth-6, cyScreen-130, TRUE);
		SendMessage(hwndList, WM_SETFONT, (WPARAM)hFont, TRUE);

		SendMessage(hwndList, LB_INITSTORAGE, RealLastIndiv, RealLastIndiv*40);
		for (I = 1; I < RealLastIndiv; I++)
		{
			if (Indiv[I].Name == 0)
				continue;
			NameX = Indiv[I].Name;
			for (z = NameX; (Buf[z] != '/') && (z < (NameX+255)); z++)
			{
				if ((Buf[z] == '\n') || (Buf[z] == '\r'))
				{
					_snwprintf(Error, 512, TEXT("NO!\nMissing a line like \"1 FirstName /LASTNAME/\"\n after the line \"0 @I%u@ INDI\" line\nin %s\n(which can be edited in Notepad)"), Indiv[I].Num, FullFilename);
					MessageBox(hwnd, Error, ERROR, MB_OK);
					DestroyWindow(hwnd);
					return 0;
				}
			}
			z++;//past '/'
			for (w = 0; (Buf[z] != '/') && (Buf[z] != '\r') && (Buf[z] != '\n'); w++, z++) // added '\r' and '\n' check 24 Dec 2010
			{
				if (w >= 512) {
					_snwprintf(Error, 512, TEXT("A name is too long"));
					MessageBox(hwnd, Error, ERROR, MB_OK);
					return 0;
				}
				temp[w] = Buf[z];
/*
				if ((temp[w] >= 'a') && (temp[w] <= 'z') && ((temp[w] != 'c') || (temp[w-1] != 'M')))
					temp[w] &= 0xDF;
				else if (temp[w] == -15)//ñ 
					temp[w] = 'N';
				else if (temp[w] == -31)//á
					temp[w] = 'A';
				else if (temp[w] == -23)//é
					temp[w] = 'E';
				else if (temp[w] == -19)//í
					temp[w] = 'I';
				else if (temp[w] == -13)//ó
					temp[w] = 'O';
				else if (temp[w] == -6)//ú
					temp[w] = 'U';
*/
			}
			temp[w++] = ',';
			temp[w++] = ' ';
			for (z = NameX; (Buf[z] != '/') && (z < (NameX+255)); w++, z++) {
				temp[w] = Buf[z];
			}
			if (Indiv[I].Birth)
			{
				temp[w++] = ' ';
				temp[w++] = 'b';
				temp[w++] = '.';
				Birth = &Buf[Indiv[I].Birth];
				for (z = 0; (Birth[z] != '\r') && (Birth[z] != '\n'); z++)
				{
					if ((Birth[z] >= '0') && (Birth[z] <= '9') && (Birth[z+1] >= '0') && (Birth[z+1] <= '9') && (Birth[z+2] >= '0') && (Birth[z+2] <= '9') && (Birth[z+3] >= '0') && (Birth[z+3] <= '9'))
					{
						temp[w++] = Birth[z++];
						temp[w++] = Birth[z++];
						temp[w++] = Birth[z++];
						temp[w++] = Birth[z++];
						break;
					}
				}
			}
			y = _snwprintf(Error, 512, TEXT("  [%u]"), Indiv[I].Num);
			for (x = 0; x < y; x++, w++)
				temp[w] = Error[x];
			temp[w] = 0;
			SendMessage(hwndList, LB_ADDSTRING, 0, (LPARAM)(LPCTSTR)temp);
			if ((highlighted != 0xFFFF) && (I == highlighted))
			{
				for (x = 0; (x < 256) && (temp[x] != 0); x++)
					HighlightedName[x] = temp[x];
				HighlightedName[x] = 0;
			}
		}
		if (HighlightedName[0] != 0)
		{
			index = SendMessage(hwndList, LB_FINDSTRINGEXACT, -1, (LPARAM)(LPCSTR)HighlightedName);
			SendMessage(hwndList, LB_SETCURSEL, index, 0);
			if (fromnewlink == FALSE)
				IndivNum = highlighted;//for IndivProc
			if (showindividual)
			{
				if (hwndIndiv)
				{
					if (DestroyWindow(hwndIndiv))
						hwndIndiv = NULL;
				}
//				preditit = TRUE;
//_snwprintf(temp, 512, TEXT("%i"), newspouse);
//MessageBox(hwnd, temp, TEXT("one"), MB_OK);
//				hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);//caused a bug by re-initializing newspouse, etc
//				indivbox = TRUE;
			}
		}
		if (undone)
		{
			undone = FALSE;
			FindFirstFile(FullFilename, &fd);
			li1 = *(ULARGE_INTEGER*)&ft;
			li1.LowPart = fd.ftLastWriteTime.dwLowDateTime;
			li1.HighPart = fd.ftLastWriteTime.dwHighDateTime;
			FindFirstFile(BackupFilename, &fd);
			li2 = *(ULARGE_INTEGER*)&ft;
			li2.LowPart = fd.ftLastWriteTime.dwLowDateTime;
			li2.HighPart = fd.ftLastWriteTime.dwHighDateTime;
			if (li1.QuadPart > li2.QuadPart)
				MessageBox(hwndListDlg, TEXT("Latest data"), TEXT(""), MB_OK);
			else
				MessageBox(hwndListDlg, TEXT("Older data"), TEXT(""), MB_OK);
		}
		if (showindividual) {
			hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);
			indivbox = TRUE;
		}
		SetFocus(hwndList);
		pListProc = (WNDPROC)SetWindowLong(hwndList, GWL_WNDPROC, (LONG)EditListProc);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:
			if (index == -1)
				IndivNum = 0;//flag
			if (highlighted != 0xFFFFFFFF)
//			if (index != -1)
			{

				if (showindividual)
				{
					showindividual = FALSE;
					InitializeAgain();			
					for (i = 1; i < LastIndiv; i++)
					{
						if (Indiv[i].Num == Num)
						{
							oldHighlighted = highlighted;
							highlighted = i;
							IndivNum = highlighted;
							InitializeAgain();			
							break;
						}
					}
					SetWindowText(hwndButton, IndivInfo);
					if (hwndIndiv)
					{
						if (DestroyWindow(hwndIndiv))
							hwndIndiv = NULL;
					}
				}
				else // if (!showindividual)
				{
					IndivNum = highlighted;
					showindividual = TRUE;
					SetWindowText(hwndButton, FamTree);
					if (hwndIndiv)
					{
						if (DestroyWindow(hwndIndiv))
							hwndIndiv = NULL;
					}
					hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);
					indivbox = TRUE;
				}
				SetFocus(hwndList);
			}
			else
				MessageBox(hwndList, TEXT("First highlight an individual"), TEXT(""), MB_OK);
			break;

		case IDC_LIST1:
			if (HIWORD (wParam) == LBN_DBLCLK)
				SendMessage(hwndListDlg, WM_COMMAND, (WPARAM)IDOK, 0);

			else if ((HIWORD (wParam) == LBN_SELCHANGE) && (fromnewlink == FALSE))
			{
				if (index == -1)
				{
					IndivNum = 0;//flag
					if (hwndIndiv)
					{
						if (DestroyWindow(hwndIndiv))
							hwndIndiv = NULL;
					}
					hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);
					indivbox = TRUE;
					SetFocus(hwndList);
				}
				index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
				SendMessage(hwndList, LB_GETTEXT, index, (LPARAM)IndivName);
				for (x = 0; IndivName[x] != '['; x++)
					;
				x++;
				for (y = x, Num = 0; IndivName[y] != ']'; y++)
					Num = (Num * 10) + (IndivName[y] - '0'); // [178] is @I178@
				if (showindividual == FALSE)
				{
					for (i = 1; i < LastIndiv; i++)
					{
						if (Indiv[i].Num == Num)
						{
							oldHighlighted = highlighted;
							highlighted = i;
fromrestart = TRUE; // to not open the file again
							InitializeAgain();
fromrestart = FALSE;
							break;
						}
					}
				}
				else // if (showindividual)
				{
					if (indivbox == FALSE)
					{
						if (DestroyWindow(hwndIndiv))
							hwndIndiv = NULL;
					}
					if (hwndIndiv == NULL)
					{
						hwndIndiv = CreateDialog(hInst, TEXT("INDIVIDUAL"), hwnd, IndivProc);
						indivbox = TRUE;
						SetFocus(hwndList);
					}
					for (i = 1; i < LastIndiv; i++)
					{
						if (Indiv[i].Num == Num)
						{
							if (Indiv[i].Name)
							{
								NameX = Indiv[i].Name;
								for (x = NameX, y = 0; (Buf[x] != '\r') && (Buf[x] != '\n'); x++)
//									if (Buf[x] != '/')
										Name[y++] = Buf[x];
								Name[y] = 0;
								Namend = x;
//								ch = Buf[x];
//								Buf[x] = 0;
//								SendDlgItemMessage(hwndIndiv, IDC_EDIT1, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)&Buf[Indiv[i].Name]);
								SendDlgItemMessage(hwndIndiv, IDC_EDIT1, WM_SETTEXT, 0, (LPARAM)(LPCTSTR) Name);
//								Buf[x] = ch;
							}
							else
								MessageBox(hwnd, TEXT("Tell jdmcox@jdmcox about this."), TEXT("Oops"), MB_OK);
							if (Indiv[i].Sex == 1)//male
								x = SendDlgItemMessage(hwndIndiv, IDC_EDIT3, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)TEXT("Mr"));
							else if (Indiv[i].Sex == 0xFFFF)//female
								SendDlgItemMessage(hwndIndiv, IDC_EDIT3, WM_SETTEXT, 0, (LPARAM)TEXT("Ms"));
							else
								SendDlgItemMessage(hwndIndiv, IDC_EDIT2, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)Blank);
							if (Indiv[i].Birth)
							{
								Birth = &Buf[Indiv[i].Birth];
								for (x = 0; (Birth[x] != '\r') && (Birth[x] != '\n'); x++)
									;
								ch = Birth[x];
								Birth[x] = 0;
								SendDlgItemMessage(hwndIndiv, IDC_EDIT2, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)&Buf[Indiv[i].Birth]);
								Birth[x] = ch;
							}
							else
								SendDlgItemMessage(hwndIndiv, IDC_EDIT2, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)Blank);
							if (Indiv[i].BirthLoc)
							{
								BirthLoc = &Buf[Indiv[i].BirthLoc];
								for (x = 0; (BirthLoc[x] != '\r') && (BirthLoc[x]!= '\n'); x++)
									;
								ch = BirthLoc[x];
								BirthLoc[x] = 0;
								SendDlgItemMessage(hwndIndiv, IDC_EDIT5, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)&Buf[Indiv[i].BirthLoc]);
								BirthLoc[x] = ch;
							}
							else
								SendDlgItemMessage(hwndIndiv, IDC_EDIT5, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)Blank);
							if (Indiv[i].Death)
							{
								Death = &Buf[Indiv[i].Death];
								for (x = 0; (Death[x] != '\r') && (Death[x]!= '\n'); x++)
									;
								ch = Death[x];
								Death[x] = 0;
								SendDlgItemMessage(hwndIndiv, IDC_EDIT4, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)&Buf[Indiv[i].Death]);
								Death[x] = ch;
							}
							else
								SendDlgItemMessage(hwndIndiv, IDC_EDIT4, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)Blank);
							if (Indiv[i].DeathLoc)
							{
								DeathLoc = &Buf[Indiv[i].DeathLoc];
								for (x = 0; (DeathLoc[x] != '\r') && (DeathLoc[x]!= '\n'); x++)
									;
								ch = DeathLoc[x];
								DeathLoc[x] = 0;
								SendDlgItemMessage(hwndIndiv, IDC_EDIT6, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)&Buf[Indiv[i].DeathLoc]);
								DeathLoc[x] = ch;
							}
							else
								SendDlgItemMessage(hwndIndiv, IDC_EDIT6, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)Blank);
							//////////
							IndivNum = i;
							GetData();
							//////////
							SendDlgItemMessage(hwndIndiv, IDC_EDIT7, WM_SETTEXT, 0, (LPARAM)(LPCTSTR)BigNote);

//							_snwprintf(temp, "  (number %i in Gedcom file)", Indiv[IndivNum].Num);
//							SetWindowText(hwndIndiv, temp);
							SetWindowText(hwndIndiv, Name);
						}
					}
				}
			}
			break;

		case IDOK:
			list = FALSE;
			index = SendMessage(hwndList, LB_GETCURSEL, 0, 0);
			SendMessage(hwndList, LB_GETTEXT, index, (LPARAM)IndivName);
			for (x = 0; (IndivName[x] != 0) && (IndivName[x] != '['); x++)
				;
			if (IndivName[x] == 0)
				break;
			x++;
			for (y = x, Num = 0; IndivName[y] != ']'; y++)
				Num = (Num * 10) + (IndivName[y] - '0');
			for (i = 1; i < LastIndiv; i++)
			{
				if (Indiv[i].Num == Num)
				{
					iFromList = i;
					if (fromnewlink)
					{
						fromnewlink = FALSE;
						if (hwndIndiv)
						{
							if (DestroyWindow(hwndIndiv))
								hwndIndiv = NULL;
						}
						EndDialog (hwndListDlg, TRUE);//trick
						hwndListDlg = NULL;
						return TRUE;
					}
					else
					{
						oldHighlighted = highlighted;
						highlighted = i;
						if (bs < 511)
							bs++;
						if (Backspace[bs] != -1)
							for ( ; (bs < 511) && (Backspace[bs] != -1); bs++)
								;
						Backspace[bs] = i;
					}
					if (hwndIndiv)
					{
						if (DestroyWindow(hwndIndiv))
							hwndIndiv = NULL;
					}
					if (hwndListDlg)
					{
						if (DestroyWindow(hwndListDlg))
							hwndListDlg = NULL;
					}
					if (showindividual)
						InitializeAgain();
					return TRUE;
				}
			}
			break;

		case IDCANCEL:
			list = FALSE;
			if (fromnewlink)
			{
				fromnewlink = FALSE;
				EndDialog (hwndListDlg, FALSE);//trick
				hwndListDlg = NULL;
				return FALSE;
			}
			if (hwndIndiv)
			{
				if (DestroyWindow(hwndIndiv))
					hwndIndiv = NULL;
			}
			if (DestroyWindow(hwndListDlg))
				hwndList = NULL;
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK IndivProc(HWND hwndIndiv, UINT message, WPARAM wParam, LPARAM lParam)
{
	static TCHAR tempFamc[256];
	static HWND hwndAsterisk, hwndShowPhoto, hwndRemovePhoto, hwndAdoptive;

	switch (message)
	{
	case WM_INITDIALOG:
//		if (hwndPhoto)
//		{
//			DestroyWindow(hwndPhoto);
//			hwndPhoto = NULL;
//		}
//		if (mouseover == PHOTO)
//		{
//			gotmouseover = FALSE;
//			mouseover = NONE;
//			InvalidateRect(hwnd, &rect, FALSE);
//		}
		if (IndivNum == 0xFFFFFFFF)
			IndivNum = 0; // just in case
		IndivNumber = Indiv[IndivNum].Num;
		preditit = TRUE;
		dontupdateindiv = FALSE;
		newspouse = newchild = newfather = newmother = FALSE;
		hwndName = GetDlgItem(hwndIndiv, IDC_EDIT1);
		hwndBirth = GetDlgItem(hwndIndiv, IDC_EDIT2);
		hwndSex = GetDlgItem(hwndIndiv, IDC_EDIT3);
		hwndDeath = GetDlgItem(hwndIndiv, IDC_EDIT4);
		hwndBirthLoc = GetDlgItem(hwndIndiv, IDC_EDIT5);
		hwndDeathLoc = GetDlgItem(hwndIndiv, IDC_EDIT6);
		hwndNote = GetDlgItem(hwndIndiv, IDC_EDIT7);
		hwndAsterisk = GetDlgItem(hwndIndiv, IDC_EDIT8);
		hwndRemovePhoto = GetDlgItem(hwndIndiv, IDC_BUTTON13);
		hwndShowPhoto = GetDlgItem(hwndIndiv, IDC_BUTTON14);
		hwndAdoptive = GetDlgItem(hwndIndiv, IDC_BUTTON12);
		if (IndivNum == 0)
			break;
		if (Indiv[IndivNum].Sex == 1)//male
		{
			SetWindowText(hwndSex, TEXT("Mr"));
			PrintName[1] = 'r';
		}
		else if (Indiv[IndivNum].Sex == 0xFFFF)
		{
			SetWindowText(hwndSex, TEXT("Ms"));
			PrintName[1] = 's';
		}
		else {
			SetWindowText(hwndSex, Blank);
			PrintName[1] = '?';
		}

		if (Indiv[IndivNum].Flags & 0x100000)
		{
			SetWindowText(hwndShowPhoto, TEXT("Show &Photo"));
			ShowWindow(hwndRemovePhoto, SW_SHOW);
		}
		else
			SetWindowText(hwndShowPhoto, TEXT("Link &Photo"));

		if (Indiv[IndivNum].Flags & 0x10000)
			ShowWindow(hwndAdoptive, SW_SHOW);
		NameX = Indiv[IndivNum].Name;
		for (x = NameX, y = 0; (x != fileSize) && (Buf[x] != '\r') && (Buf[x]!= '\n'); x++)
//			if (Buf[x] != '/')
			Name[y++] = Buf[x];
		Name[y] = 0;
		Namend = x;
		ch = Buf[x];
		SetWindowTextW(hwndName, Name);
		for (x = 0, y = 0, z = 3; Name[x] != 0; x++) {
			if (Name[x] != '/') {
				Name1[y++] = Name[x];
				PrintName[z++] = Name[x];
			}
		}
		PrintName[z++] = '\r';
		PrintName[z++] = '\n';
		PrintNameLen = z;
		Name1[y] = 0;
		SetWindowText(hwndIndiv, Name1);
		if (Indiv[IndivNum].Birth)
		{
			Birth = &Buf[Indiv[IndivNum].Birth];
			for (x = 0, y = 0, z = 6; (y < 256) && (Birth[x] != '\r') && (Birth[x] != '\n'); x++, y++, z++)
			{
				BirthDeath[y] = Birth[x];
				PrintBirthDate[z] = Birth[x];
			}
			if (Indiv[IndivNum].Flags & 1)
				BirthDeath[y++] = '*';
			PrintBirthDate[z++] = '\r';
			PrintBirthDate[z++] = '\n';
			PrintBirthDateLen = z;
			BirthDeath[y] = 0;
			SetWindowText(hwndBirth, BirthDeath);
		}
		else
			SetWindowText(hwndBirth, Blank);
		if (Indiv[IndivNum].BirthLoc)
		{
			BirthLoc = &Buf[Indiv[IndivNum].BirthLoc];
			for (x = 0, y = 0, z = 9; (y < 256) && (BirthLoc[x] != '\r') && (BirthLoc[x]!= '\n'); x++, y++, z++)
			{
				BirthDeath[y] = BirthLoc[x];
				PrintBirthLoc[z] = BirthLoc[x];
			}
			if (Indiv[IndivNum].Flags & 0x10)
				BirthDeath[y++] = '*';
			PrintBirthLoc[z++] = '\r';
			PrintBirthLoc[z++] = '\n';
			PrintBirthLocLen = z;
			BirthDeath[y] = 0;
			SetWindowText(hwndBirthLoc, BirthDeath);
		}
		else
			SetWindowText(hwndBirthLoc, Blank);
		if (Indiv[IndivNum].Death)
		{
			Death = &Buf[Indiv[IndivNum].Death];
			for (x = 0, y = 0, z = 6; (y < 256) && (Death[x] != '\r') && (Death[x]!= '\n'); x++, y++, z++)
			{
				BirthDeath[y] = Death[x];
				PrintDeathDate[z] = Death[x];
			}
			if (Indiv[IndivNum].Flags & 0x100)
				BirthDeath[y++] = '*';
			PrintDeathDate[z++] = '\r';
			PrintDeathDate[z++] = '\n';
			PrintDeathDateLen = z;
			BirthDeath[y] = 0;
			SetWindowText(hwndDeath, BirthDeath);
		}
		else
			SetWindowText(hwndDeath, Blank);

		if (Indiv[IndivNum].DeathLoc)
		{
			DeathLoc = &Buf[Indiv[IndivNum].DeathLoc];
			for (x = 0, y = 0, z = 9; (y < 256) && (DeathLoc[x] != '\r') && (DeathLoc[x]!= '\n'); x++, y++, z++)
			{
				BirthDeath[y] = DeathLoc[x];
				PrintDeathLoc[z] = DeathLoc[x];
			}
			if (Indiv[IndivNum].Flags & 0x1000)
				BirthDeath[y++] = '*';
			PrintDeathLoc[z++] = '\r';
			PrintDeathLoc[z++] = '\n';
			PrintDeathLocLen = z;
			BirthDeath[y] = 0;
			SetWindowText(hwndDeathLoc, BirthDeath);
		}
		else
			SetWindowText(hwndDeathLoc, Blank);
		if ((Indiv[IndivNum].Flags) &&  (Indiv[IndivNum].Flags < 0x10000))
			SetWindowText(hwndAsterisk, TEXT("* More than one Birth/Death Date/Place in Gedcom file"));
		//////////
		GetData();
		//////////
		SetWindowText(hwndNote, BigNote);
		break;


	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON15: // Print
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
/*
			EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, NULL, 0, &dwSizeNeeded, &dwNumItems);//get printer name for OpenPrinter
			lpInfo = (LPPRINTER_INFO_5)malloc(dwSizeNeeded);
			EnumPrinters(PRINTER_ENUM_LOCAL, NULL, 5, (LPBYTE)lpInfo, dwSizeNeeded, &dwSizeNeeded, &dwNumItems);
			if (PrinterName[0] == 0) { // only ask for printer name once
				if (GetDefaultPrinter(PrinterName, &PrinterNameLen))
				{//show possible default printer first
					if (PrinterName[0] != 0)
					{
						if (IDYES == MessageBox(hwnd, PrinterName, TEXT("Use this printer?"), MB_YESNO))
							goto skip2;
					}
				}
				for ( dwItem = 0; dwItem < dwNumItems; dwItem++ )
				{
					if (wcscmp(lpInfo[dwItem].pPrinterName, PrinterName))
					{
						if (IDYES == MessageBox(hwnd, lpInfo[dwItem].pPrinterName, TEXT("Use this printer?"), MB_YESNO))
						{
							wcscpy(PrinterName, lpInfo[dwItem].pPrinterName);
							break;	
						}
					}
				}
			}
skip2:	free(lpInfo);
			if (dwItem == dwNumItems)
				break;//nothing chosen;
			OpenPrinter(PrinterName, &hPrinter, NULL);//get hPrinter for DocumentProperties
			dwNeeded = DocumentProperties(hwnd, hPrinter, PrinterName, NULL, NULL, 0);//get DEVMODE size
			pDevMode = (LPDEVMODE)malloc(dwNeeded);
			DocumentProperties(hwnd, hPrinter, PrinterName, pDevMode, NULL, DM_OUT_BUFFER);//get DEVMODE info
			pDevMode->dmOrientation = DMORIENT_PORTRAIT;
			DocumentProperties(hwnd, hPrinter, PrinterName, pDevMode, pDevMode, DM_IN_BUFFER|DM_OUT_BUFFER);//integrate landscape selection
			ClosePrinter(hPrinter);
			hDC = CreateDC(TEXT("WINSPOOL"), PrinterName, NULL, pDevMode);

			lf3.lfHeight = (Font*15) + 6; // so 0 Font doesn't equal 0 lf3.lfHeight
			hFont3 = CreateFontIndirect(&lf3);
			SelectObject(hDC, hFont3);
			GetTextExtentPoint32(hDC, TEXT("j"), 1, &PrintSize); // get PrintSize.cy
			if (StartDoc(hDC, &di) > 0)
			{
				if (StartPage(hDC) > 0)
				{//the /100 is there because of the previous w/(8.5*10) and the 10.5*1000
					TextOut(hDC, 100, 0, PrintName, PrintNameLen);
					if (Indiv[IndivNum].Birth)
						TextOut(hDC, 100, PrintSize.cy, PrintBirthDate, PrintBirthDateLen);
					if (Indiv[IndivNum].BirthLoc)
						TextOut(hDC, 100, PrintSize.cy*2, PrintBirthLoc, PrintBirthLocLen);
					if (Indiv[IndivNum].Death)
						TextOut(hDC, 100, PrintSize.cy*3, PrintDeathDate, PrintDeathDateLen);
					if (Indiv[IndivNum].DeathLoc)
						TextOut(hDC, 100, PrintSize.cy*4, PrintDeathLoc, PrintDeathLocLen);
					if (BigNoteLen)
					{
						for (x = 0, y = 0, z = 5; x < BigNoteLen; x++)
						{
							if (BigNote[x] == '\r') {
								TextOut(hDC, 100, PrintSize.cy*z, &BigNote[y], x-y);
								x++; // past CarriageReturn
								y = x+1; // past LineFeed
								z++; // line
							}
						}
					}
					if (EndPage(hDC) > 0)
						EndDoc(hDC);
				}
			}
			DeleteDC(hDC);
			free(pDevMode);
*/
			ofn.lpstrFilter = TEXT(" *.txt\0*.txt\0\0");
			ofn.lpstrFile = IndivFullFilename;
			ofn.lpstrFileTitle = IndivFilename;
			ofn.lpstrTitle = TEXT("Save As");
			ofn.lpstrDefExt = TEXT("txt");
			if (GetSaveFileName(&ofn)) {
				for (x = 0; x < PrintNameLen; x++)
					IndivBuf[x] = PrintName[x];
				for (y = 0; y < PrintBirthDateLen; x++, y++)
					IndivBuf[x] = PrintBirthDate[y];
				for (y = 0; y < PrintBirthLocLen; x++, y++)
					IndivBuf[x] = PrintBirthLoc[y];
				for (y = 0; y < PrintDeathDateLen; x++, y++)
					IndivBuf[x] = PrintDeathDate[y];
				for (y = 0; y < PrintDeathLocLen; x++, y++)
					IndivBuf[x] = PrintDeathLoc[y];
				for (y = 0; y < BigNoteLen; x++, y++)
					IndivBuf[x] = BigNote[y];
				IndivBufLen = x;
				utf = (BYTE*)VirtualAlloc(NULL, 2*IndivBufLen, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
				for (x = 0, y = 0; x < IndivBufLen; x++)
				{
					if (IndivBuf[x] < 0x80)
						utf[y++] = *(BYTE*)&IndivBuf[x];
					else if (IndivBuf[x] < 0x800)
					{
						utf[y++] = ((BYTE)(MASK2BYTES | IndivBuf[x] >> 6));
						utf[y++] = ((BYTE)(MASKBYTE | IndivBuf[x] & MASKBITS));
					}
					else if(IndivBuf[x] < 0x10000)
					{
						utf[y++] = ((BYTE)(MASK3BYTES | IndivBuf[x] >> 12));
						utf[y++] = ((BYTE)(MASKBYTE | IndivBuf[x] >> 6 & MASKBITS));
						utf[y++] = ((BYTE)(MASKBYTE | IndivBuf[x] & MASKBITS));
					}
					else
						MessageBox(hwnd, TEXT("HUH?"), NULL, MB_OK);
				}
				hFile = CreateFile(IndivFullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				WriteFile(hFile, utf, y, &dwBytesWritten, NULL);
				CloseHandle(hFile);
				VirtualFree(utf, 0, MEM_RELEASE);
			}
			ofn.lpstrFilter       = TEXT(" *.ged\0*.ged\0\0");
			ofn.lpstrFile         = FullFilename;
			ofn.lpstrFileTitle    = NULL;
			ofn.lpstrFileTitle    = Filename;
			ofn.lpstrDefExt       = TEXT("ged");
			SetFocus(hwndIndiv);
			break;

		case READ:
			MessageBox(hwndIndiv, NamesDates, TEXT("Names and Dates"), MB_OK);
			MessageBox(hwndIndiv, PhotoHelp, TEXT("Photos"), MB_OK);
			MessageBox(hwndIndiv, AlternateParents, TEXT("Alternate Parents"), MB_OK);
			break;

		case IDC_BUTTON13://Remove photo link
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			if (Indiv[IndivNum].Flags & 0x100000)//really not necessary
			{//if photo exists
//				GetPhotoFileName();
				hPhotoFile = CreateFile(SimpleFamilyTreePhotosTxt, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
				if (hPhotoFile != INVALID_HANDLE_VALUE)
				{
					DWORD x2 = 0;

					if (PhotoFileSize = GetFileSize(hPhotoFile, NULL))
					{
						utf = (BYTE*)malloc(PhotoFileSize);
						ReadFile(hPhotoFile, utf, PhotoFileSize, &dwBytesRead, NULL);
						PhotoBuf = (TCHAR*)malloc(sizeof(TCHAR)*PhotoFileSize);
						if ((utf[0] == 0xEF) && (utf[1] == 0xBB) && (utf[2] == 0xBF))
							x = 3;
						else
							x = 0;
						for (y = 0; x < PhotoFileSize; )
						{
							if(((utf[x] & MASK3BYTES) == MASK3BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE) && ((utf[x+2] & MASKBYTE) == MASKBYTE))
							{// 1110xxxx 10xxxxxx 10xxxxxx
								PhotoBuf[y++] = ((utf[x] & 0x0F) << 12) | ((utf[x+1] & MASKBITS) << 6) | (utf[x+2] & MASKBITS);
								x += 3;
							}
							else if(((utf[x] & MASK2BYTES) == MASK2BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE))
							{// 110xxxxx 10xxxxxx
								PhotoBuf[y++] = ((utf[x] & 0x1F) << 6) | (utf[x+1] & MASKBITS);
								x += 2;
							}
							else if(utf[x] < MASKBYTE)// 0xxxxxxx
								PhotoBuf[y++] = utf[x++];
							else
								PhotoBuf[y++] = utf[x++];//not utf-8 character

						}
						free(utf);

						PhotoFileSize = y;
						NamePtr = &Buf[Indiv[IndivNum].Name];
						for (x2 = 0; x2 < PhotoFileSize; x2++)
						{
							if ((PhotoBuf[x2] == '\n') && ((x2+1) < PhotoFileSize))
							{
								x = x2+1;//to beginning of next line
								for (y = 0; (PhotoBuf[x] != '<') && (PhotoBuf[x] != '@') && (x < PhotoFileSize); x++, y++)
									Name[y] = PhotoBuf[x];
								Name[y-1] = 0;//ignore ' ' before '>' or '@'
								if (PhotoBuf[x] == '@')
								{
									for ( ; (PhotoBuf[x] < '0') || (PhotoBuf[x] > '9'); x++)
										;
									PhotoID = Atoi(&PhotoBuf[x]);
									if (PhotoID == Indiv[IndivNum].Num)
									{
										RemovePhoto(x2+1);
										break;
									}
								}
								else if (PhotoBuf[x] == '<')
								{
									x += 3;//past "<> "
									for (z = 0; z < 30; z++)
										if (NamePtr[z] != Name[z])
											break;
									if (NamePtr[z] == '\r')
									{
										RemovePhoto(x2+1);
										break;
									}
								}
							}
						}
						free(PhotoBuf);
					}
					CloseHandle(hPhotoFile);
					SendMessage(hwndIndiv, WM_COMMAND, IDOK, 0);
				}
			}
			break;

		case IDC_BUTTON14://Show Photo or Link Photo
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
//			GetPhotoFileName();
			hPhotoFile = CreateFile(SimpleFamilyTreePhotosTxt, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, 0, NULL);
			if (hPhotoFile != INVALID_HANDLE_VALUE)
			{//OPEN_ALWAYS means hPhotoFile _can't_ == INVALID_HANDLE_VALUE
				DWORD x2 = 0;

				if (PhotoFileSize = GetFileSize(hPhotoFile, NULL))
				{
					utf = (BYTE*)malloc(PhotoFileSize);
					ReadFile(hPhotoFile, utf, PhotoFileSize, &dwBytesRead, NULL);
					PhotoBuf = (TCHAR*)malloc(sizeof(TCHAR)*PhotoFileSize);
					for (x = 0, y = 0; x < PhotoFileSize; )
					{
						if(((utf[x] & MASK3BYTES) == MASK3BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE) && ((utf[x+2] & MASKBYTE) == MASKBYTE))
						{// 1110xxxx 10xxxxxx 10xxxxxx
							PhotoBuf[y++] = ((utf[x] & 0x0F) << 12) | ((utf[x+1] & MASKBITS) << 6) | (utf[x+2] & MASKBITS);
							x += 3;
						}
						else if(((utf[x] & MASK2BYTES) == MASK2BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE))
						{// 110xxxxx 10xxxxxx
							PhotoBuf[y++] = ((utf[x] & 0x1F) << 6) | (utf[x+1] & MASKBITS);
							x += 2;
						}
						else if(utf[x] < MASKBYTE)// 0xxxxxxx
							PhotoBuf[y++] = (TCHAR)utf[x++];
						else
							PhotoBuf[y++] = (TCHAR)utf[x++];//not utf-8 character
					}
					free(utf);
					if (Indiv[IndivNum].Flags & 0x100000)
					{
						NamePtr = &Buf[Indiv[IndivNum].Name];
 						for ( ; x2 < PhotoFileSize; x2++)
						{
							if (PhotoBuf[x2] == '\n')
							{
								x = x2+1;//to beginning of next line
								for (y = 0; ((PhotoBuf[x] != '@') && (PhotoBuf[x] != '<')) && (x < PhotoFileSize); x++, y++)
									Name[y] = PhotoBuf[x];
								Name[y-1] = 0;//ignore ' ' before '>'
								if (PhotoBuf[x] == '@')
								{//search on IndivNum
									for ( ; (PhotoBuf[x] < '0') || (PhotoBuf[x] > '9'); x++)
										;
									PhotoID = Atoi(&PhotoBuf[x]);
									if (PhotoID == Indiv[IndivNum].Num)
									{
										for (y = x; (PhotoBuf[y] != '@') && (y < PhotoFileSize); y++)
											;
										if (y != PhotoFileSize)
										{
											y += 2;//to beginning of photot filename
											x = y;
											for ( ; (y < PhotoFileSize) && (PhotoBuf[y] != '\r'); y++)
												;
											PhotoBuf[y] = 0;
											Jpeg = &PhotoBuf[x];
											hJpegFile = CreateFile(Jpeg, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, NULL);
											if (hJpegFile != INVALID_HANDLE_VALUE)
											{
												if (JpegFileSize = GetFileSize(hJpegFile, NULL))
												{
													JpegBuf = (BYTE*)malloc(JpegFileSize);
													ReadFile(hJpegFile, JpegBuf, JpegFileSize, &dwBytesRead, NULL);
													CloseHandle(hJpegFile);
													ShowJpeg();
													free(JpegBuf);
												}
												else
												{
													CloseHandle(hJpegFile);
													MessageBox(hwnd, TEXT("Jpeg file is empty."), ERROR, MB_OK);
												}
											}
											else//didn't find that jpeg file
//												MessageBox(hwnd, TEXT("This individual has an invalid photo link.\nSelect Remove Photo Link."), TEXT(""), MB_OK);
												MessageBox(hwnd, TEXT("is an invalid photo link."), Jpeg, MB_OK);
											goto endofor;
										}
									}
								}
								else if (PhotoBuf[x] == '<')
								{
									x += 3;//past "<> "
									for (z = 0; z < 30; z++)
										if (NamePtr[z] != Name[z])
											break;
									if (NamePtr[z] == '\r')
									{
										for (y = x; (y < PhotoFileSize) && (PhotoBuf[y] != '\r'); y++)
											;
										PhotoBuf[y] = 0;
										Jpeg = &PhotoBuf[x];
										hJpegFile = CreateFile(Jpeg, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
										if (hJpegFile != INVALID_HANDLE_VALUE)
										{
											if (JpegFileSize = GetFileSize(hJpegFile, NULL))
											{
												JpegBuf = (BYTE*)malloc(JpegFileSize);
												ReadFile(hJpegFile, JpegBuf, JpegFileSize, &dwBytesRead, NULL);
												CloseHandle(hJpegFile);
												ShowJpeg();
												free(JpegBuf);
											}
											else
											{
												CloseHandle(hJpegFile);
												MessageBox(hwnd, TEXT("Jpeg file is empty."), ERROR, MB_OK);
											}
										}
										else//didn't find that jpeg file
											MessageBox(hwnd, TEXT("This individual has an invalid photo link.\nSelect Remove Photo Link."), TEXT(""), MB_OK);
										break;
									}
								}
							}
						}
					}
					else
						x2 = PhotoFileSize;//flag
endofor:			free(PhotoBuf);
				}//end of if (PhotoFileSize = GetFileSize(hPhotoFile, NULL))
				if (x2 == PhotoFileSize)
				{//not found, so add photo link
					ofn.lpstrFilter = TEXT(" *.jpg\0*.jpg\0\0");
					ofn.lpstrFile = FullPhotoFilename;
					ofn.lpstrFileTitle = PhotoFilename;
					ofn.lpstrTitle = TEXT("Select a photo to link to");
					ofn.lpstrDefExt = TEXT("jpg");
					if (PhotoDirectory[0])
						ofn.lpstrInitialDir = PhotoDirectory;
					if (GetOpenFileName(&ofn))
					{
						x = 0;
						if (PhotoFileSize == 0)
						{
							ThisPhotoBuf[x++] = '\r';
							ThisPhotoBuf[x++] = '\n';
						}
						for (y = Indiv[IndivNum].Name, z = 0; (y < fileSize) && (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
							Name[z++] = Buf[y];
						Name[z] = 0;
						for (y = 0; Name[y] != 0; x++, y++)
							ThisPhotoBuf[x] = Name[y];
						ThisPhotoBuf[x++] = ' ';
						for (y = Indiv[IndivNum].Name; (y != 0) && (Buf[y] != '@'); y--)
							;
						if (Buf[y] == '@')
						{
							z = y;
							for (y--; (y != 0) && (Buf[y] != '@') && (Buf[y] != '\r'); y--)
								;
							if (Buf[y] == '@')
								for ( ; y <= z; x++, y++)
									ThisPhotoBuf[x] = Buf[y];
						}
						else
						{
							ThisPhotoBuf[x++] = '<';
							ThisPhotoBuf[x++] = '>';
						}
						ThisPhotoBuf[x++] = ' ';
						for (y = 0; FullPhotoFilename[y] != 0; x++, y++)
							ThisPhotoBuf[x] = FullPhotoFilename[y];
						ThisPhotoBuf[x++] = '\r';
						ThisPhotoBuf[x++] = '\n';
						for (z = 0, y = 0; (z < x) && (y < 512); z++)
						{
							if (ThisPhotoBuf[z] < 0x80)
								bThisPhotoBuf[y++] = (BYTE)ThisPhotoBuf[z];
							else if (ThisPhotoBuf[z] < 0x800)
							{
								bThisPhotoBuf[y++] = ((BYTE)(MASK2BYTES | ThisPhotoBuf[z] >> 6));
								bThisPhotoBuf[y++] = ((BYTE)(MASKBYTE | ThisPhotoBuf[z] & MASKBITS));
							}
							else if(ThisPhotoBuf[z] < 0x10000)
							{
								bThisPhotoBuf[y++] = ((BYTE)(MASK3BYTES | ThisPhotoBuf[z] >> 12));
								bThisPhotoBuf[y++] = ((BYTE)(MASKBYTE | ThisPhotoBuf[z] >> 6 & MASKBITS));
								bThisPhotoBuf[y++] = ((BYTE)(MASKBYTE | ThisPhotoBuf[z] & MASKBITS));
							}
						}
						SetFilePointer(hPhotoFile, 0, NULL, FILE_END);
						WriteFile(hPhotoFile, bThisPhotoBuf, y, &dwBytesWritten, NULL);
						Indiv[IndivNum].Flags |= 0x100000;
 						SendMessage(hwndIndiv, WM_COMMAND, IDOK, 0);
					}
					else
						SetFocus(hwndIndiv);
					ofn.lpstrInitialDir = CurrentDir;
				}
				CloseHandle(hPhotoFile);
			}//end of if (hPhotoFile != INVALID_HANDLE_VALUE)
			break;

		case IDC_BUTTON1:
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			DialogBox(hInst, TEXT("GED"), hwndIndiv, GEDProc);
			break;

		case IDC_BUTTON2://father
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			x = Indiv[IndivNum].Childof & 0x7FFFFFFF;
			if (x != 0)
			{
				for (i = 0; i < LastIndiv; i++)
				{
					for (s = 0; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s]); s++)
					{
						if ((x == (Indiv[i].Spouseof[s] & 0x7FFFFFFF)) && (Indiv[i].Sex == 1))
						{
							MessageBox(hwndIndiv, TEXT("already has a father.\nTo add another man as an adoptive parent,\nhighlight that man,\nthen link this individual to him as a child."), Name, MB_OK);
							goto x19;
						}
					}
				}
			}
			newfather = TRUE;
			goto x15;
		case IDC_BUTTON3://spouse
			if ((anyeditit) || (Indiv[IndivNum].Spouseof[MAXSPOUSES-1]))
			{
				MessageBeep(MB_OK);
				break;
			}
			i = 0;
			newspouse = TRUE;
			goto x15;
		case IDC_BUTTON4://child
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			i = 0;
			newchild = TRUE;
			goto x15;
		case IDC_BUTTON6://mother
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			x = Indiv[IndivNum].Childof & 0x7FFFFFFF;
			if (x != 0)
			{
				for (i = 0; i < LastIndiv; i++)
				{
					for (s = 0; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s]); s++)
					{
						if ((x == (Indiv[i].Spouseof[s] & 0x7FFFFFFF)) && (Indiv[i].Sex == 0xFFFF))
						{
							MessageBox(hwndIndiv, TEXT("already has a mother.\nTo add another woman as an adoptive parent,\nhighlight that woman,\nthen link this individual to her as a child."), Name, MB_OK);
							goto x19;
						}
					}
				}
			}
			newmother = TRUE;

x15:;
			anyeditit = TRUE;
			Response = DialogBox(hInst, TEXT("NEW"), hwnd, NewProc);
			if (Response == 123)
			{//new individual
				fromnewlink = TRUE;
				Response = DialogBox(hInst, TEXT("NEWINDIVIDUAL"), hwnd, NewIndivProc);
				Sex = Sex2[6];
			}
			else if (Response == 321)
			{//link to an existing individual
				fromnewlink = TRUE;

				if (DialogBox(hInst, TEXT("LIST"), hwnd, ListProc))//trick
				{ // i comes from DialogBox
					i = iFromList;
					if ((newfather) && (Indiv[i].Sex != 1))
					{
						newfather = FALSE;
						for (y = Indiv[i].Name, z = 0; (y < fileSize) && (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
							if (Buf[y] != '/')
								Name[z++] = Buf[y];
						Name[z] = 0;
						MessageBox(hwnd, TEXT("can't be a father (she's female)."), Name, MB_OK);
						goto x19;
					}
					else if ((newmother) && (Indiv[i].Sex != 0xFFFF))
					{
						newmother = FALSE;
						for (y = Indiv[i].Name, z = 0; (y < fileSize) && (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
							if (Buf[y] != '/')
								Name[z++] = Buf[y];
						Name[z] = 0;
						ch = Buf[y];
						Buf[y] = 0;
						MessageBox(hwnd, TEXT("can't be a mother (he's male)."), Name, MB_OK);
						Buf[y] = ch;
						goto x19;
					}
					for (NewIndivOffset = Indiv[i].Name; (NewIndivOffset != 0) && ((Buf[NewIndivOffset] != '\n') || (Buf[NewIndivOffset+1] != '0')); NewIndivOffset--)
						;
					NewIndivOffset++;//NewIndivOffset is beginning of new individual's record in Buf
					Number2 = Indiv[i].Num;//the number of the individual added (IndivNum is the Indiv offset of the individual adding someone)
					if (Indiv[i].Sex == 1)
						Sex = 'M';
					else if (Indiv[i].Sex == 0xFFFF)
						Sex = 'F';
					else
						Sex = 0;
				}
				else
					goto x19;
			}

			if (Response)
			{
				for (IndivOffset = Indiv[IndivNum].Name; (IndivOffset != 0) && ((Buf[IndivOffset] != '\n') || (Buf[IndivOffset+1] != '0')); IndivOffset--)
					;
				IndivOffset++;//IndivOffset is beginning of individual's record in Buf

				if (newspouse)
				{//IndivNum is the person getting the spouse and i is the perspective spouse
					dontupdateindiv = TRUE;
					for (s = 0; (s < MAXSPOUSES) && (Indiv[IndivNum].Spouseof[s]); s++)
					{
						for (ws = 0; (ws < MAXSPOUSES) && (Indiv[i].Spouseof[ws]); ws++)
						{
							if ((Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF) == (Indiv[i].Spouseof[ws] & 0x7FFFFFFF))
							{
								MessageBox(hwnd, TEXT("Can't marry the same person more than once\n(as far as a family tree is concerned)!"), Name, MB_OK);
								goto x19;
							}
						}
					}

					if (FALSE == CheckRelative(highlighted, i))
					{
						MessageBox(hwnd, TEXT("A marriage to an ancestor/descendant\n is not allowed in this program."), ERROR, MB_OK);
						goto x19;
					}
					if (Indiv[IndivNum].Spouseof[0])
					{//if he has a spouse or child
						g = 0;
						for (s = 0; (s < MAXSPOUSES) && (Indiv[IndivNum].Spouseof[s]); s++)
						{
							for (I = 0; I < LastIndiv; I++)
							{
								if (I == IndivNum)
									continue;
								for (w = 0; (w < MAXSPOUSES) && (Indiv[I].Spouseof[w]); w++)
								{
									if ((Indiv[I].Spouseof[w] & 0x7FFFFFFF) == (Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF))
									{
										GotSpouse[g++] = Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF;
									}
								}
							}
						}
						for (s = 0; (s < MAXSPOUSES) && (Indiv[IndivNum].Spouseof[s]); s++)
						{
							for (I = 0; I < LastIndiv; I++)
							{
								if ((Indiv[I].Childof & 0x7FFFFFFF) == (Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF))
								{
									for (x = 0; x < g; x++)
										if (GotSpouse[x] == (Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF))
											break;
									if (x == g)
									{//individual has a child without having a spouse
										for (y = Indiv[IndivNum].Name, z = 0; (y < fileSize) && (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
											if (Buf[y] != '/')
												Name[z++] = Buf[y];
										Name[z] = 0;
										for (y = Indiv[i].Name, z = 0; (y < fileSize) && (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
											if (Buf[y] != '/')
												Name1[z++] = Buf[y];
										Name1[z] = 0;
										if (Indiv[IndivNum].Sex != Indiv[i].Sex)
										{
											_snwprintf(Error, 512, TEXT("Are the offspring of %s\nalso the offspring of %s?"), Name, Name1);
											if (IDYES == MessageBox(hwnd, Error, TEXT(""), MB_YESNO))
											{
												FamNum = Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF;
												if (Sex == 0)
													goto x17;
												if (Sex == 'M')
													PutFAM(MAN, NewIndivOffset);
												else if (Sex == 'F')
													PutFAM(WOMAN, NewIndivOffset);
												PutFAMS(NewIndivOffset);
												goto x17;
											}
										}
//										else
//											MessageBox(hwnd, SameSex, "", MB_OK);
									}
								}
							}
						}
					}
					if (Indiv[i].Spouseof[0])
					{//if prospective spouse has a spouse/child
						g = 0;
						for (s = 0; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s]); s++)
						{
							for (I = 0; I < LastIndiv; I++)
							{
								if (I == i)
									continue;
								for (w = 0; (w < MAXSPOUSES) && (Indiv[I].Spouseof[w]); w++)
								{
									if ((Indiv[I].Spouseof[w] & 0x7FFFFFFF) == (Indiv[i].Spouseof[s] & 0x7FFFFFFF))
									{
										GotSpouse[g++] = Indiv[i].Spouseof[s] & 0x7FFFFFFF;
									}
								}
							}
						}
						for (s = 0; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s]); s++)
						{
							for (I = 0; I < LastIndiv; I++)
							{
								if ((Indiv[I].Childof & 0x7FFFFFFF) == (Indiv[i].Spouseof[s] & 0x7FFFFFFF))
								{
									for (x = 0; x < g; x++)
										if (GotSpouse[x] == (Indiv[i].Spouseof[s] & 0x7FFFFFFF))
											break;
									if (x == g)
									{//individual has a child without having a spouse
										TCHAR tempName[256];
										for (w = 0, z = Indiv[i].Name; (z < (Indiv[i].Name+256)) && (Buf[z] != '\r') && (Buf[z] != '\n'); z++)
											if (Buf[z] != '/')
												tempName[w++] = Buf[z];
										tempName[w] = 0;
										for (w = 0, z = Indiv[IndivNum].Name; (z < (Indiv[IndivNum].Name+256)) && (Buf[z] != '\r') && (Buf[z] != '\n'); z++)
											if (Buf[z] != '/')
												Name[w++] = Buf[z];
										Name[w] = 0;
										if (Indiv[IndivNum].Sex != Indiv[i].Sex)
										{
											_snwprintf(temp, 256, TEXT("Are %s's offspring also\n %s's offspring?"), tempName, Name);
											if (IDYES == MessageBox(hwnd, temp, TEXT(""), MB_YESNO))
											{
												FamNum = Indiv[i].Spouseof[s] & 0x7FFFFFFF;
												if (Indiv[IndivNum].Sex == 0)
													goto x17;
												else if (Indiv[IndivNum].Sex == 1)
													PutFAM(MAN, IndivOffset);
												else if (Indiv[IndivNum].Sex == 0xFFFF)
													PutFAM(WOMAN, IndivOffset);
												PutFAMS(IndivOffset);
												goto x17;
											}
										}
//										else
//											MessageBox(hwnd, SameSex, "", MB_OK);
									}
								}
							}
						}
					}
					if (Sex == 0)
						goto x17;
					else if (Sex == 'M')
					{
						PutNewFAM(MAN, NewIndivOffset);
						PutFAM(WOMAN, IndivOffset);
					}
					else if (Sex == 'F')
					{
						PutNewFAM(MAN, IndivOffset);
						PutFAM(WOMAN, NewIndivOffset);
					}
					if (NewIndivOffset > IndivOffset)
					{
						PutFAMS(NewIndivOffset);
						PutFAMS(IndivOffset);
					}
					else
					{
						PutFAMS(IndivOffset);
						PutFAMS(NewIndivOffset);
					}
				}

				else if (newchild)
				{
					if (FALSE == CheckRelative(highlighted, i))//check for an attempt to make an ancestor a child
					{
						MessageBox(hwnd, TEXT("Can't make an ancestor/descendant a child!"), ERROR, MB_OK);
						goto x19;
					}
					FamNum = 0xFFFFFFFF;//flag
					for (x = IndivOffset+1; (x < fileSize) && ((Buf[x] != '\n') || (Buf[x+1] != '0')); x++)
					{
						if ((Buf[x] == '\n') && (Buf[x+1] == '1') && (Buf[x+3] == 'F') && (Buf[x+4] == 'A') && (Buf[x+5] == 'M') && (Buf[x+6] == 'S'))
						{
							FamNum = Atoi(&Buf[x+10]);
							for (x++, y = 0; Buf[x] != '\n'; x++, y++)
								Famspouse[y] = Buf[x];
							Famspouse[y++] = '\n';
							Famspouse[y] = 0;
							break;
						}
					}
					if (FamNum == 0xFFFFFFFF)//no FAMS
					{//prospective parent doesn't have a spouse/child
						CheckifChild(NewIndivOffset);//the child
						if (FamNum == 0xFFFFFFFE)
						{
							MessageBox(hwnd, TEXT("This child already has two sets of parents"),ERROR, MB_OK);
							goto x19;
						}
						if (Indiv[IndivNum].Sex == 0)
							goto x17;
						if (Indiv[IndivNum].Sex == 1)
							PutNewFAM(MAN, IndivOffset);
						else if (Indiv[IndivNum].Sex == 0xFFFF)
							PutNewFAM(WOMAN, IndivOffset);
						PutFAM(CHILD, NewIndivOffset);
						if (NewIndivOffset > IndivOffset)
						{
							PutFAMC(NewIndivOffset);
							PutFAMS(IndivOffset);
						}
						else
						{
							PutFAMS(IndivOffset);
							PutFAMC(NewIndivOffset);
						}
					}
					else
					{//ask which couple to add the child to, and add the appropriate FAMC to the child's record
						childhasparents = FALSE;
						if (CheckifChild(NewIndivOffset))
						{
							childhasparents = TRUE;
							if (FamNum == 0xFFFFFFFE)
							{
								MessageBox(hwnd, TEXT("This child already has two sets of parents."), ERROR, MB_OK);
								goto x19;
							}
						}
						GetNameinBuf(NewIndivOffset, ChildsName);//prevy comes from here
						foundspouse = FALSE;
						FamNum = 0;//flag
						SingleParentFamNum = 0;
						for (s = 0; (s < MAXSPOUSES) && (Indiv[IndivNum].Spouseof[s]); s++)
						{
							notherspouse = TRUE;
							for (I = 0; I < LastIndiv; I++)
							{
								if (I == IndivNum)
									continue;
								for (w = 0; (w < MAXSPOUSES) && (Indiv[I].Spouseof[w]); w++)
								{
									if ((Indiv[I].Spouseof[w] & 0x7FFFFFFF) == (Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF))
									{
										notherspouse = FALSE;
										foundspouse = TRUE;
										GetNameinBuf(IndivOffset, ParentsName);//prevy comes from here
										if ((childhasparents) && (prevy == 0))
											continue;//single parent can't adopt
										for (x = Indiv[I].Name; (Buf[x] != '\r') && (Buf[x] != '\n'); x++)
										{
											if (Buf[x] != '/')
												ParentsName[y++] = Buf[x];
										}
										ParentsName[y++] = '?';
										ParentsName[y] = 0;
										y = 0;
										if (Indiv[IndivNum].Sex != Indiv[I].Sex)
										{
											if (IDYES == MessageBox(hwnd, ParentsName, ChildsName, MB_YESNO))
											{
												if (Indiv[i].Childof)//or childhasparents
												{
													for (s = 0; (s < MAXSPOUSES) && (Indiv[I].Spouseof[s]); s++)
													{
														if ((Indiv[I].Spouseof[s] & 0x7FFFFFFF) == (Indiv[i].Childof & 0x7FFFFFFF))
														{//got child's parent(s)
															for (s = 0; (s < MAXSPOUSES) && (Indiv[I].Spouseof[s]); s++)
															{
																for (w = 0; (w < MAXSPOUSES) && (Indiv[IndivNum].Spouseof[w]); w++)
																{
																	if ((Indiv[I].Spouseof[s] & 0x7FFFFFFF) == (Indiv[IndivNum].Spouseof[w] & 0x7FFFFFFF))
																	{//if current parent and prospective parent are already spouses
																		MessageBox(hwnd, TEXT("Un-link the child or spouse first."), TEXT(""), MB_OK);
																		goto x19;
																	}
																}
															}
														}
													}
												}
												FamNum = Indiv[I].Spouseof[w] & 0x7FFFFFFF;
												PutFAM(CHILD, NewIndivOffset);
												PutFAMC(NewIndivOffset);
												goto x17;
											}
										}
//										else
//											MessageBox(hwnd, SameSex, "", MB_OK);
									}
								}
							}
							if (notherspouse == TRUE)
								SingleParentFamNum = Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF;
						}
						if ((foundspouse) && (childhasparents == FALSE))
						{
							for (x = 0, y = prevy; Nonelse[x] != 0; x++, y++)
								ParentsName[y] = Nonelse[x];
							ParentsName[y] = 0;
							if (IDYES == MessageBox(hwnd, ParentsName, ChildsName, MB_YESNO))
							{
								if (FamNum == 0)
								{
									if (SingleParentFamNum)
										FamNum = SingleParentFamNum;

									else
									{
										if (Indiv[IndivNum].Sex == 0)
											goto x17;
										if (Indiv[IndivNum].Sex == 1)
											PutNewFAM(MAN, IndivOffset);
										else if (Indiv[IndivNum].Sex == 0xFFFF)
											PutNewFAM(WOMAN, IndivOffset);
									}
								}
								specialcase = TRUE;
								PutFAM(CHILD, NewIndivOffset);
								if (NewIndivOffset > IndivOffset)
								{
									PutFAMC(NewIndivOffset);
									for (x = 0; Indiv[IndivNum].Spouseof[x] != 0; x++)
										if (Indiv[IndivNum].Spouseof[x] == SingleParentFamNum)
											break;
									if (Indiv[IndivNum].Spouseof[x] == 0) // not found
										PutFAMS(IndivOffset);
								}
								else
								{
									for (x = 0; Indiv[IndivNum].Spouseof[x] != 0; x++)
										if (Indiv[IndivNum].Spouseof[x] == SingleParentFamNum)
											break;
									if (Indiv[IndivNum].Spouseof[x] == 0) // not found
										PutFAMS(IndivOffset);
									PutFAMC(NewIndivOffset);
								}
								goto x17;
							}
						}
						else//no spouse, but has FAMS in record
						{//put 1 HUSB or 1 WIFE and 1 CHIL in FAMS record & add 1 FAMC to child's record & add 1 FAMS to new parent's record
							if (FamNum == 0)
							{
								if (SingleParentFamNum)
									FamNum = SingleParentFamNum;

								else
								{
									if (Indiv[IndivNum].Sex == 0)
										goto x17;
									if (Indiv[IndivNum].Sex == 1)
										PutNewFAM(MAN, IndivOffset);
									else if (Indiv[IndivNum].Sex == 0xFFFF)
										PutNewFAM(WOMAN, IndivOffset);
								}
							}
							PutFAM(CHILD, NewIndivOffset);
							PutFAMC(NewIndivOffset);
						}
x17:;
					}
				}

				else if ((newfather) || (newmother))
				{
					if (FALSE == CheckRelative(highlighted, i))
					{//check to see if new parent is a descendant of this individual
						MessageBox(hwnd, TEXT("Can't make an ancestor/descendant a parent!"), ERROR, MB_OK);
						goto x19;
					}
					Famspouse[5] = 'C';
					if (FALSE == CheckifChild(IndivOffset))
					{//not a child of anyone
						GetNameinBuf(IndivOffset, ChildsName);
						GetNameinBuf(NewIndivOffset, ParentsName);
						if ((i != 0xFFFFFFFF) && (Indiv[i].Spouseof[0]))
						{//if new parent has FAMS in his record
							foundspouse = FALSE;
							FamNum = 0;//flag
							SingleParentFamNum = 0;
							for (s = 0; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s]); s++)
							{
								notherspouse = TRUE;
								for (I = 0; I < LastIndiv; I++)
								{
									if (I == i)
										continue;
									for (w = 0; (w < MAXSPOUSES) && (Indiv[I].Spouseof[w]); w++)
									{
										if ((Indiv[I].Spouseof[w] & 0x7FFFFFFF) == (Indiv[i].Spouseof[s] & 0x7FFFFFFF))
										{
											notherspouse = FALSE;
											foundspouse = TRUE;
											for (x = Indiv[I].Name, y = prevy; (Buf[x] != '\r') && (Buf[x] != '\n'); x++)
											{
												if (Buf[x] != '/')
													ParentsName[y++] = Buf[x];
											}
											ParentsName[y++] = '?';
											ParentsName[y] = 0;
											if (Indiv[I].Sex != Indiv[i].Sex)
											{
												if (IDYES == MessageBox(hwnd, ParentsName, ChildsName, MB_YESNO))
												{//put 1 CHIL in FAM record & add 1 FAMC to child's record
													FamNum = Indiv[I].Spouseof[w] & 0x7FFFFFFF;
													PutFAM(CHILD, IndivOffset);
													PutFAMC(IndivOffset);
													goto x18;
												}
											}
//											else
//												MessageBox(hwnd, SameSex, "", MB_OK);
										}
									}
								}
								if (notherspouse == TRUE)
									SingleParentFamNum = Indiv[i].Spouseof[s] & 0x7FFFFFFF;//i
							}
							if (foundspouse)
							{//put 1 CHIL in FAM record & add 1 FAMC to child's record
								for (x = 0, y = prevy; Nonelse[x] != 0; x++, y++)
									ParentsName[y] = Nonelse[x];
								ParentsName[y] = 0;
								if (IDYES == MessageBox(hwnd, ParentsName, ChildsName, MB_YESNO))
								{
									if (FamNum == 0)
									{
										if (SingleParentFamNum)
											FamNum = SingleParentFamNum;
										else
										{
											if (Indiv[i].Sex == 1)
												PutNewFAM(MAN, NewIndivOffset);
											else if (Indiv[i].Sex == 0xFFFF)
												PutNewFAM(WOMAN, NewIndivOffset);
										}
									}
									PutFAM(CHILD, IndivOffset);
									if (NewIndivOffset > IndivOffset)
									{
										PutFAMS(NewIndivOffset);
										PutFAMC(IndivOffset);
									}
									else
									{
										PutFAMC(IndivOffset);
										PutFAMS(NewIndivOffset);
									}
								}
							}
							else//no other spouse
							{//put 1 HUSB or 1 WIFE and 1 CHIL in FAMS record & add 1 FAMC to child's record & add 1 FAMS to new parent's record
								if (FamNum == 0)
								{
									if (SingleParentFamNum)
										FamNum = SingleParentFamNum;
									else
									{
										if (Indiv[i].Sex == 1)
											PutNewFAM(MAN, NewIndivOffset);
										else if (Indiv[i].Sex == 0xFFFF)
											PutNewFAM(WOMAN, NewIndivOffset);
									}
								}
								PutFAM(CHILD, IndivOffset);
								PutFAMC(IndivOffset);
							}
x18:;
						}
						else//new parent doesn't have FAMS in his record
						{
							if (Sex == 'M')
								PutNewFAM(MAN, NewIndivOffset);
							else if (Sex == 'F')
								PutNewFAM(WOMAN, NewIndivOffset);
							PutFAM(CHILD, IndivOffset);
							if (NewIndivOffset > IndivOffset)
							{
								PutFAMS(NewIndivOffset);
								PutFAMC(IndivOffset);
							}
							else
							{
								PutFAMC(IndivOffset);
								PutFAMS(NewIndivOffset);
							}
						}
					}
					else//already a child of a parent
					{
						if (FamNum == 0xFFFFFFFE)//from CheckifChild
						{
							MessageBox(hwnd, TEXT("Don't try to get a child with two sets of parents,\nget a spouse for one of them instead."), TEXT(""), MB_OK);
							goto x19;
						}
						else
						{
							int oldi = i;//oldi is prospective parent
							I = Indiv[IndivNum].Childof & 0x7FFFFFFF;
							for (i = 1; i < LastIndiv; i++)
							{ 
								for (s = 0; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s]); s++)
								{
									if ((Indiv[i].Spouseof[s] & 0x7FFFFFFF) == I)//(Indiv[IndivNum].Childof & 0x7FFFFFFF))
									{//got child's current parent(s)
										for (s = 0; (s < MAXSPOUSES) && (Indiv[i].Spouseof[s]); s++)
										{
											for (w = 0; (w < MAXSPOUSES) && (Indiv[oldi].Spouseof[w]); w++)
											{
												if ((Indiv[i].Spouseof[s] & 0x7FFFFFFF) == (Indiv[oldi].Spouseof[w] & 0x7FFFFFFF))
												{//if current parent and prospective parent are already spouses
													MessageBox(hwnd, TEXT("Un-link the child or spouse first."), TEXT(""), MB_OK);
													goto x19;
												}
											}
										}
									}
								}
							}
							if (Sex == 'M')
								PutFAM(MAN, NewIndivOffset);
							else if (Sex == 'F')
								PutFAM(WOMAN, NewIndivOffset);
							PutFAMS(NewIndivOffset);
						}
					}
				}//end of if ((newfather) || (newmother))
				newspouse = newchild = newfather = newmother = FALSE;//Individual is IndivNum & father or mother is i
				if (dontupdateindiv == FALSE)
					updateindiv = TRUE;
				ReStart();
				if (errorinbuf)
				{
					errorinbuf = FALSE;
					anyeditit = FALSE;
					preditit = FALSE;
					editit = FALSE;
					if (DestroyWindow(hwndIndiv))
						hwndIndiv = NULL;
					indivbox = FALSE;
					return FALSE;
				}
				if (DestroyWindow(hwndIndiv))
					hwndIndiv = NULL;
				return TRUE;
			}//end of if (Response)
x19:		newfather = newmother = newspouse = newchild = FALSE;
			anyeditit = FALSE;
			SetFocus(hwndIndiv);
			break;

		case IDC_BUTTON12://Swap Primary & Secondary Parents
			if (Indiv[IndivNum].Flags & 0x10000)
			{//if it's showing - see above
				BufEnd = GetBufEnd(Indiv[IndivNum].Name);
				for (x = Indiv[IndivNum].Name; x < BufEnd; x++)
				{
					if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] == 'F') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M') && (Buf[x+5] == 'C'))
					{
						z = x;
						y = 0;
						tempFamc[y++] = Buf[x++];
						for ( ; (y < 256) && (x <= BufEnd); x++, y++)
						{
							if ((Buf[x-1] == '\n') && (Buf[x] != '2'))
								break;
							tempFamc[y] = Buf[x];
						}
						tempFamc[y] = 0;
						w = z+1;
						for ( ; z <= fileSize; x++, z++)
							Buf[z] = Buf[x];//first FAMC is moved to tempFamc
						for ( ; w <= BufEnd; w++)
							if ((Buf[w-1] == '\n') && (Buf[w] != '2'))
								break;
						for (z = fileSize, y += fileSize; z >= w; z--, y--)
							Buf[y] = Buf[z];
						for (y = 0; tempFamc[y] != 0; y++, w++)
							Buf[w] = tempFamc[y];
						preditit = FALSE;
						if (editit)
						{
							editit = FALSE;
							SaveEdited();
						}
						if (DestroyWindow(hwndIndiv))
							hwndIndiv = NULL;
						indivbox = FALSE;
						ReStart();
						return TRUE;
					}
				}
			}
			break;

		case EDIT://Edit Individual
			if ((preditit == TRUE) && (editit == FALSE) && (anyeditit == FALSE))
			{
				anyeditit = TRUE;
				editit = TRUE;
				BufEnd = GetBufEnd(Indiv[IndivNum].Name);
				SendMessage(hwndName, EM_SETREADONLY, 0, 0);
				SendMessage(hwndBirth, EM_SETREADONLY, 0, 0);
				SendMessage(hwndDeath, EM_SETREADONLY, 0, 0);
				SendMessage(hwndBirthLoc, EM_SETREADONLY, 0, 0);
				SendMessage(hwndDeathLoc, EM_SETREADONLY, 0, 0);
				GetData();
				SetWindowText(hwndNote, BigNote);
				SetFocus(hwndName);
			}
/*
			else
			{
				anyeditit = FALSE;
				editit = FALSE;
				SendMessage(hwndName, EM_SETREADONLY, 1, 0);
				SendMessage(hwndBirth, EM_SETREADONLY, 1, 0);
				SendMessage(hwndDeath, EM_SETREADONLY, 1, 0);
				SendMessage(hwndBirthLoc, EM_SETREADONLY, 1, 0);
				SendMessage(hwndDeathLoc, EM_SETREADONLY, 1, 0);
			}
*/
			break;

		case IDC_BUTTON11://Add Name2
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			if (DialogBox(hInst, TEXT("NAME2"), hwndIndiv, Name2Proc))
			{
				if (hwndIndiv)
				{
					if (DestroyWindow(hwndIndiv))
						hwndIndiv = NULL;
				}
				updateindiv = TRUE;
				ReStart();
				errorinbuf = FALSE;
			}
			else
				SetFocus(hwndIndiv);
			break;

		case IDC_BUTTON7://marriage
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			if (Indiv[IndivNum].Spouseof[0])
			{
				if (DialogBox(hInst, TEXT("MARRIAGES"), hwndIndiv, MarriageProc))
				{
					if (hwndIndiv)
					{
						if (DestroyWindow(hwndIndiv))
							hwndIndiv = NULL;
					}
					updateindiv = TRUE;
					ReStart();
					errorinbuf = FALSE;
//					hwndIndiv = CreateDialog(hInst, "INDIVIDUAL", hwnd, IndivProc);
				}
			}
			else
			{
				MessageBox(hwndIndiv, TEXT("This individual isn't married."), TEXT(""), MB_OK);
				SetFocus(hwndIndiv);
			}
			break;
		
		case IDC_BUTTON10://Add Note
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			if (DialogBox(hInst, TEXT("NOTE"), hwndIndiv, NoteProc))
			{
				if (hwndIndiv)
				{
					if (DestroyWindow(hwndIndiv))
						hwndIndiv = NULL;
				}
				updateindiv = TRUE;
				ReStart();
				errorinbuf = FALSE;
//				hwndIndiv = CreateDialog(hInst, "INDIVIDUAL", hwnd, IndivProc);
			}
			else
				SetFocus(hwndIndiv);
			break;

		case IDC_BUTTON9://Add Event
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			if (DialogBox(hInst, TEXT("EVENT"), hwndIndiv, EventProc))
			{
				if (hwndIndiv)
				{
					if (DestroyWindow(hwndIndiv))
						hwndIndiv = NULL;
				}
				updateindiv = TRUE;
				ReStart();
				errorinbuf = FALSE;
//				hwndIndiv = CreateDialog(hInst, "INDIVIDUAL", hwnd, IndivProc);
			}
			else
				SetFocus(hwndIndiv);
			break;

		case IDC_BUTTON8://Delete
			if (anyeditit)
			{
				break;
			}
			anyeditit = TRUE;
			if (IDYES == MessageBox(hwnd, Name, TEXT(" DELETE"), MB_YESNO|MB_DEFBUTTON2))
			{
				deleetit = TRUE;
				goto x21;
			}
			else
			{
				anyeditit = FALSE;
				SetFocus(hwndIndiv);
				break;
			}
		case IDC_BUTTON5://Un-Link
			if (anyeditit)
			{
				MessageBeep(MB_OK);
				break;
			}
			anyeditit = TRUE;
			if (IDNO == MessageBox(hwnd, Name, TEXT(" UN-LINK"), MB_YESNO|MB_DEFBUTTON2))
			{
				anyeditit = FALSE;
				SetFocus(hwndIndiv);
				break;
			}
x21:;
			for (x = 0; x < 21; x++)
				SpouseOffset[x] = 0;
			so = 0;
			for (IndivOffset = Indiv[IndivNum].Name; (IndivOffset != 0) && ((Buf[IndivOffset] != '\n') || (Buf[IndivOffset+1] != '0')); IndivOffset--)
				;
			IndivOffset++;//Individual's record in Buf
			FamC = Indiv[IndivNum].Childof & 0x7FFFFFFF;
			for (x = 0; x < MAXSPOUSES; x++)
				FamS[x] = Indiv[IndivNum].Spouseof[x] & 0x7FFFFFFF;
			//go thru all the FAM records & delete IndivNum from those in FamC and FamS, and so on
			for (FamPtr = FamsEnd; FamPtr >= Fams; FamPtr--)
			{
				if ((Buf[FamPtr-1] == '\n') && (Buf[FamPtr] == '0') && (Buf[FamPtr+2] == '@'))// && (Buf[FamPtr+3] == 'F')
				{
					gotit = FALSE;
					test = FamPtr + 3;
					if ((Buf[test] < '0') || (Buf[test] > '9'))
						test++;
					for (test2 = test; Buf[test2] != '\n'; test2++)
					{
						if ((Buf[test2] == 'F') && (Buf[test2+1] == 'A')  && (Buf[test2+2] == 'M')) 
						{
							gotit = TRUE;
							break;
						}
					}
					if (gotit == FALSE)
						continue;
					v = z = test;//pointing to fams number
					for ( ; Buf[z] != '@'; z++)//???????
						;
					for (FamNum = 0, w = 1, z--; z >= v; z--)
					{
						FamNum += (Buf[z] - '0') * w;
						w *= 10;
					}
					if (FamC == FamNum)
					{
						CheckFAM();
						if ((Children == 1) && ((HusbOffset == 0) || (WifeOffset == 0)))
						{//if there's only one CHIL and only one HUSB or WIFE, save the HUSB or WIFE @I123@ to delete the FAMS in that record, and delete FAM
							if (HusbOffset)
								y = HusbOffset;
							else if (WifeOffset)
								y = WifeOffset;
							else
							{
								MessageBox(hwnd, TEXT("No HUSB or WIFE in FAM record!"), ERROR, MB_OK);//really should exit or something after this
								anyeditit = FALSE;
								preditit = FALSE;
								editit = FALSE;
								if (DestroyWindow(hwndIndiv))
									hwndIndiv = NULL;
								indivbox = FALSE;
								return FALSE;
							}
							y += 8;
							if ((Buf[y] < '0') || (Buf[y] > '9'))
								y++;
							GetSpouseOffset(Atoi(&Buf[y]));
							y = SingleChildOffset + 8;//SingleChildOffset from CheckFAM
							if ((Buf[y] < '0') || (Buf[y] > '9'))
								y++;
							GetSpouseOffset(Atoi(&Buf[y]));
							for (x = y = v-4; (Buf[y] != '\n') || (Buf[y+1] != '0'); y++)
								;
							y++;
							z = y - x;
							for ( ; y < fileSize; x++, y++)
								Buf[x] = Buf[y];//delete FAM record
							fileSize -= z;
							FamsEnd -= z;
						}
						else//((Children) && ((HusbOffset) && (WifeOffset))
						{//just delete the child
							y = SingleChildOffset + 8;
							if ((Buf[y] < '0') || (Buf[y] > '9'))
								y++;
							GetSpouseOffset(Atoi(&Buf[y]));
							for (x = y = SingleChildOffset; Buf[y] != '\n'; y++)
								;
							y++;
							z = y - x;
							for ( ; y < fileSize; x++, y++)
								Buf[x] = Buf[y];
							fileSize -= z;
							FamsEnd -= z;
						}
					}
					else//check FamS
					{
						for (s = 0; (s < MAXSPOUSES) && (FamS[s]); s++)
						{
							if (FamS[s] == FamNum)
							{
								CheckFAM();
								y = 0;
								if ((HusbNum == Indiv[IndivNum].Num) && (HusbOffset))//((Indiv[IndivNum].Sex == 1) && 
									y = HusbOffset;
								else if ((WifeNum == Indiv[IndivNum].Num) && (WifeOffset))//((Indiv[IndivNum].Sex == 0xFFFF) && 
									y = WifeOffset;
								else
								{
									MessageBox(hwnd, TEXT("No HUSB or WIFE in FAM record"), ERROR, MB_OK);//really should exit or something after this
									anyeditit = FALSE;
									preditit = FALSE;
									editit = FALSE;
									if (DestroyWindow(hwndIndiv))
										hwndIndiv = NULL;
									indivbox = FALSE;
									return FALSE;
								}
								if (y)
								{
									z = y + 8;
									if ((Buf[z] < '0') || (Buf[z] > '9'))
										z++;
									GetSpouseOffset(Atoi(&Buf[z]));//Indiv[IndivNum]
								}
								if (Children == 0)//from CheckFAM
								{//delete FAM
									if ((y == WifeOffset) && (HusbOffset))
										y = HusbOffset;
									else if ((y == HusbOffset) && (WifeOffset))
										y = WifeOffset;
									if (y)
									{
										z = y + 8;
										if ((Buf[z] < '0') || (Buf[z] > '9'))
											z++;
										GetSpouseOffset(Atoi(&Buf[z]));//the other spouse
									}
									for (x = y = v-4; (Buf[y] != '\n') || (Buf[y+1] != '0'); y++)
										;
									y++;
									z = y - x;
									for ( ; y < fileSize; x++, y++)
										Buf[x] = Buf[y];
									fileSize -= z;
									FamsEnd -= z;
								}
								else//if a child/children
								{//delete FAM record or delete HUSB or WIFE in FAM record
									if ((HusbOffset == 0) || (WifeOffset == 0))
									{//delete FAM
										for (x = 0; x < child; x++)
										{
											z = ChildOffset[x] + 8;//ChildOffset from CheckFAM
											if ((Buf[z] < '0') || (Buf[z] > '9'))
												z++;
											GetSpouseOffset(Atoi(&Buf[z]));//put child's offset in SpouseOffset
										}
										for (x = y = v-4; (Buf[y] != '\n') || (Buf[y+1] != '0'); y++)
											;
									}
									else//if husband & wife & child delete HUSB or WIFE line
										for (x = y; Buf[y] != '\n'; y++)//y aready points to HusbOffset or WifeOffset
											;
									y++;
									z = y - x;
									for ( ; y < fileSize; x++, y++)
										Buf[x] = Buf[y];
									fileSize -= z;
									FamsEnd -= z;
								}
							}
						}
					}
				}
			}//end of for (FamPtr = FamsEnd;
			while (TRUE)
			{
				z = 0;
				for (s = 0; (s < 21) && (SpouseOffset[s]); s++)
				{
					if (SpouseOffset[s] & 0x80000000)
						continue;//if top bit was set
					if (SpouseOffset[s] > z)
						z = SpouseOffset[s]; 
				}
				if (z)
				{
					for (s = 0; s < 21; s++)
						if (z == SpouseOffset[s])
							SpouseOffset[s] |= 0x80000000;
					for (w = z, y = z; Buf[y] != '\n'; y++)
						;
					y++;
					z = y - w;
					for ( ; y < fileSize; w++, y++)
						Buf[w] = Buf[y];
					fileSize -= z;
					Fams -= z;
					FamsEnd -= z;
				}
				else
					break;
			}
			if (deleetit)
			{
				deleetit = FALSE;
//				for (DeleteOffset = Indiv[IndivNum].Name; (DeleteOffset != 0) && ((Buf[DeleteOffset] != '\n') || (Buf[DeleteOffset+1] != '0')); DeleteOffset--)
//					;
//				DeleteOffset++;//DeleteOffset is beginning of individual's record in Buf
				for (x = 0; x < fileSize; x++)
				{
					if ((Buf[x] == '\n') && (Buf[x+1] == '0') && (Buf[x+2] == ' ')  && (Buf[x+3] == '@'))
					{
						for (y = x+4; (y < fileSize) && (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
							;
						if ((Buf[y-4] == 'I') && (Buf[y-3] == 'N') && (Buf[y-2] == 'D') && (Buf[y-1] == 'I'))
						{
							y = x + 4;
							if ((Buf[y] < '0') || Buf[y] > '9')
								y++;
							if (Atoi(&Buf[y]) == (int)Indiv[IndivNum].Num)
							{
								DeleteOffset = x+1;
								BufEnd = GetBufEnd(DeleteOffset);
								for (x = DeleteOffset, y = BufEnd; y < fileSize; x++, y++)
									Buf[x] = Buf[y];
								fileSize = x;
								break;
							}
						}
					}
				} 
			}
			highlighted = 0xFFFFFFFF;
			IndivNum = 0xFFFFFFFF;
			ReStart();
			errorinbuf = FALSE;
			unlinked = TRUE;
			SendMessage(hwnd, WM_COMMAND, ID_SHOWLISTOFINDIVIDUALS, 0);
			SetFocus(hwndList);
			//fall thru...
		case IDOK:
			anyeditit = FALSE;
			preditit = FALSE;
			if (editit)
			{
				editit = FALSE;
				updateindiv = TRUE;
				SaveEdited();
			}
			if (DestroyWindow(hwndIndiv))
				hwndIndiv = NULL;
			indivbox = FALSE;
			return TRUE;

		case IDCANCEL:
			anyeditit = FALSE;
			preditit = FALSE;
			editit = FALSE;
			if (DestroyWindow(hwndIndiv))
				hwndIndiv = NULL;
			indivbox = FALSE;
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK NewIndivProc(HWND hwndNewIndiv, UINT message, WPARAM wParam, LPARAM lParam)
{
	DWORD x;
	static HWND hwndName, hwndBirth, hwndSex, hwndDeath, hwndBirthLoc, hwndDeathLoc;

	switch (message)
	{
	case WM_INITDIALOG:
		if (hwndList)
		{
			if (DestroyWindow(hwndListDlg))
				hwndList = NULL;
		}
		hwndName = GetDlgItem(hwndNewIndiv, IDC_EDIT1);
		hwndBirth = GetDlgItem(hwndNewIndiv, IDC_EDIT2);
		hwndDeath = GetDlgItem(hwndNewIndiv, IDC_EDIT4);
		hwndBirthLoc = GetDlgItem(hwndNewIndiv, IDC_EDIT5);
		hwndDeathLoc = GetDlgItem(hwndNewIndiv, IDC_EDIT6);
		if ((newfather) || ((newspouse) && (Indiv[IndivNum].Sex == 0xFFFF)))
			CheckRadioButton(hwndNewIndiv, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
		else if ((newmother) || ((newspouse) && (Indiv[IndivNum].Sex == 1)))
			CheckRadioButton(hwndNewIndiv, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
		SetFocus(hwndNewIndiv);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON1:
			MessageBox(hwndNewIndiv, NamesDates, TEXT(""), MB_OK);
			break;

		case IDOK:
			Famspouse[5] = 'C';
			newfile = FALSE;
			if (Buf == NULL)
			{
				newfile = TRUE;
				Buf = (TCHAR*)malloc(10000);
				for (x = 0; Head[x] != 0; x++)
					Buf[x] = Head[x];
				fileSize = x;
				Fams = x;
				Num1[4] = '1';
				for (x = 5; x < 24; x++)
					Num1[x] = 0;
				Number2 = IndivNum = IndivNumber = 1;
			}
			else
			{
				y = LastIndivNum;
				if ((Buf[y] > '9') || (Buf[y] < '0'))
					y++;//go to the first digit
				for (Number2 = 0; (y < fileSize) && (Buf[y] != '@'); y++)
					if ((Buf[y] <= '9') && (Buf[y] >= '0'))
						Number2 = (Number2 * 10) + (Buf[y] - '0');
x23:			Number2++;//new last number
				for (x = 0; x < LastIndiv; x++)
					if (Indiv[x].Num == Number2)
						goto x23;
				_itow(Number2, &Num1[4], 10);
			}
			for (x = 4; Num1[x] != 0; x++)
				;
			for (y = 0; Num2[y] != 0; x++, y++)
				Num1[x] = Num2[y];
			Num1[x] = 0;
			if (BST_CHECKED == IsDlgButtonChecked(hwndNewIndiv, IDC_RADIO1))
				Sex2[6] = 'M';
			else if (BST_CHECKED == IsDlgButtonChecked(hwndNewIndiv, IDC_RADIO2))
				Sex2[6] = 'F';
			else
			{
				if (IDYES == MessageBox(hwnd, TEXT("Is this individual a Male?"), TEXT(""), MB_YESNO))
				{
					Sex2[6] = 'M';
					CheckRadioButton(hwndNewIndiv, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
				}
				else
				{
					Sex2[6] = 'F';
					CheckRadioButton(hwndNewIndiv, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
				}
			}
			if (0 == GetWindowText(hwndName, Name1, 256))
			{
				SetFocus(hwndName);
				break;
			}
			for (x = 0, y = 7, z = 0; Name1[x] != 0; x++, y++)
			{
				Name2[y] = Name1[x];
				if (Name1[x] == '/')
					z++;
			}
			if (z != 2)
			{
				for (x = 0, y = 7; Name1[x] != 0; x++)
				{
					if (Name1[x] != '/')
						Name2[y++] = Name1[x];//get rid of all '/'s
				}
				if (Name2[y-1] == ' ')
					y--;
				Name2[y++] = '/';//put the last one in
				Name2[y++] = 0;
				Name2[y] = 0;//necessary
				z = y;
				for ( ; (y > 0) && (Name2[y] != ' '); y--)
					Name2[y] = Name2[y-1];
				Name2[y+1] = '/';
				y = z;
			}
			Name2[y++] = '\r';
			Name2[y++] = '\n';
			Name2[y++] = 0;
			BirthDateLen = GetWindowText(hwndBirth, &BirthDate[7], 32);
			BirthDate[BirthDateLen+7] = '\r';
			BirthDate[BirthDateLen+8] = '\n';
			BirthDate[BirthDateLen+9] = 0;
			BirthLocLen = GetWindowText(hwndBirthLoc, &BirthLoc2[7], 256);
			BirthLoc2[BirthLocLen+7] = '\r';
			BirthLoc2[BirthLocLen+8] = '\n';
			BirthLoc2[BirthLocLen+9] = 0;
			Death2Len = GetWindowText(hwndDeath, &Death2[7], 32);
			Death2[Death2Len+7] = '\r';
			Death2[Death2Len+8] = '\n';
			Death2[Death2Len+9] = 0;
			DeathLocLen = GetWindowText(hwndDeathLoc, &DeathLoc2[7], 256);
			DeathLoc2[DeathLocLen+7] = '\r';
			DeathLoc2[DeathLocLen+8] = '\n';
			DeathLoc2[DeathLocLen+9] = 0;
			if ((newfather) && (Sex2[6] != 'M'))
			{
				MessageBox(hwnd, TEXT("Wrong sex for a father"), ERROR, MB_OK);
				SetFocus(hwndSex);
				return FALSE;
			}
			else if ((newmother) && (Sex2[6] != 'F'))
			{
				MessageBox(hwnd, TEXT("Wrong sex for a mother"), ERROR, MB_OK);
				SetFocus(hwndSex);
				return FALSE;
			}
			y = 0;
			for (x = 0; Num1[x] != 0; x++, y++)
				NewIndiv[y] = Num1[x];
			for (x = 0; Name2[x] != 0; x++, y++)
				NewIndiv[y] = Name2[x];
			for (x = 0; Sex2[x] != 0; x++, y++)
				NewIndiv[y] = Sex2[x];
			if ((BirthDateLen) || (BirthLocLen))
				for (x = 0; Birth1[x] != 0; x++, y++)
					NewIndiv[y] = Birth1[x];
			if (BirthDateLen)
				for (x = 0; BirthDate[x] != 0; x++, y++)
					NewIndiv[y] = BirthDate[x];
			if (BirthLocLen)
				for (x = 0; BirthLoc2[x] != 0; x++, y++)
					NewIndiv[y] = BirthLoc2[x];
			if ((Death2Len) || (DeathLocLen))
				for (x = 0; Death1[x] != 0; x++, y++)
					NewIndiv[y] = Death1[x];
			if (Death2Len)
				for (x = 0; Death2[x] != 0; x++, y++)
					NewIndiv[y] = Death2[x];
			if (DeathLocLen)
				for (x = 0; DeathLoc2[x] != 0; x++, y++)
					NewIndiv[y] = DeathLoc2[x];
			NewIndiv[y] = 0;
			NewIndivOffset = Fams;//NewIndivOffset is beginning of new individual's record in Buf
			if (fileSize)
				for (w = fileSize, z = fileSize+y; w >= Fams; w--, z--)
					Buf[z] = Buf[w];
			else
				w = 0xFFFFFFFF;//for w++
			for (w++, z = 0; z < y; w++, z++)
				Buf[w] = NewIndiv[z];
			fileSize += y;
			Fams += y;
			FamsEnd += y;
			if (fromnewlink == FALSE)
			{
				if (Indiv)
				{
					IndivNum = LastIndiv;
					if (IndivNum == 0)
						IndivNum++;
					Indiv[IndivNum].Num = Number2;
				}
				else
					IndivNum = 0xFFFFFFFF;
			}
			ReStart();
			if (errorinbuf)
			{
				errorinbuf = FALSE;
				fromnewlink = TRUE;
				EndDialog (hwndNewIndiv, FALSE);
				return FALSE;
			}
			if (Indiv)
				for (i = 1; Indiv[i].Num != Number2; i++)
					; 
			EndDialog (hwndNewIndiv, TRUE);
			return TRUE;

		case IDCANCEL:
			fromnewlink = TRUE;
			EndDialog (hwndNewIndiv, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK NewProc(HWND hwndNew, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_BUTTON2:
			EndDialog (hwndNew, 321);//Link to an Existing Individual
			return 321;
		case IDC_BUTTON1:
			EndDialog (hwndNew, 123);//New Individual
			return 123;
		case IDOK:
		case IDCANCEL:
			EndDialog (hwndNew, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK GEDProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		y = GetBufEnd(NameX);
		ch = Buf[y];
		Buf[y] = 0;
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetWindowText(hwndDlg, Filename);
		SetWindowText(hwndEdit, &Buf[IndivOffset]);
		Buf[y] = ch;
		SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hFont2, TRUE);
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
/*
int CALLBACK FixGedProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	static HWND hwndEdit;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndEdit = GetDlgItem(hwndDlg, IDC_EDIT1);
		SetWindowText(hwndDlg, Filename);
		y = SendMessage(hwndEdit, EM_GETLIMITTEXT, 0, 0);
		y = SendMessage(hwndEdit, EM_LIMITTEXT, fileSize + 100, 0);
		SendMessage(hwndEdit, WM_SETTEXT, 0, (LPARAM)Buf);
		SendMessage(hwndEdit, WM_SETFONT, (WPARAM)hFont2, TRUE);
		SendMessage(hwndEdit, EM_LINESCROLL, 0, BufLines-1);
		SetFocus(hwndEdit);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			fileSize = GetWindowText(hwndEdit, Buf, fileSize);
			EndDialog (hwndDlg, TRUE);
			return TRUE;
		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}
*/
int CALLBACK Name2Proc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Name2Len;
	static HWND hwndName2;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndName2 = GetDlgItem(hwndDlg, IDC_EDIT1);
		if (noname == FALSE)
		{
			y = 0;
			for (x = Namend; x < BufEnd; x++)//FamsEnd
			{
				if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] == 'N') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M') && (Buf[x+5] == 'E'))
				{
					NoteBegin = x;
					for ( ; (y < 256) && ((Buf[x] != '\r') && (Buf[x] != '\n')); x++, y++)
						Name3[y] = Buf[x];
					for ( ; Buf[x-1] != '\n'; x++)
						;
					NoteEnd = x;
					break;
				}
			}
			Name3[y] = 0;
			SetWindowText(hwndName2, &Name3[7]);
		}
		SetFocus(hwndName2);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			Name2Len = GetWindowText(hwndName2, &Name3[7], 256);
			Name3[7+Name2Len] = '\r';
			Name3[8+Name2Len] = '\n';
			Name3[9+Name2Len] = 0;
			for (y = IndivOffset ; y < BufEnd; y++)
			{
				if ((Buf[y] == '1') && (Buf[y-1] == '\n') && (Buf[y+2] == 'N') && (Buf[y+3] == 'A') && (Buf[y+4] == 'M'))
				{
					for (y++; y < BufEnd; y++)
						if ((Buf[y] == '1') && (Buf[y-1] == '\n'))
							break;
					break;
				}
			}
			if ((Name2Len == 0) || (noname == FALSE))
			{
				y = NoteBegin;
				for ( ; NoteEnd < fileSize; NoteBegin++, NoteEnd++)
					Buf[NoteBegin] = Buf[NoteEnd];
				fileSize = NoteBegin;
			}
			if (Name2Len)
			{
				for (z = fileSize + (Name2Len+9), w = fileSize; w >= y; w--, z--)
					Buf[z] = Buf[w];
				for (x = y, z = 0; z < (DWORD)(Name2Len+9); x++, z++)
					Buf[x] = Name3[z];
				fileSize += (Name2Len+9);
			}
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK MarriageProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int Marriage, m, M, biggest, DateLen, PlacLen, fams = 0;
	DWORD SpouseNum;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndMarriedTo[0] = GetDlgItem(hwndDlg, IDC_EDIT2);
		hwndDate[0] = GetDlgItem(hwndDlg, IDC_EDIT3);
		hwndPlace[0] = GetDlgItem(hwndDlg, IDC_EDIT4);
		hwndMarriedTo[1] = GetDlgItem(hwndDlg, IDC_EDIT5);
		hwndDate[1] = GetDlgItem(hwndDlg, IDC_EDIT6);
		hwndPlace[1] = GetDlgItem(hwndDlg, IDC_EDIT7);
		hwndMarriedTo[2] = GetDlgItem(hwndDlg, IDC_EDIT8);
		hwndDate[2] = GetDlgItem(hwndDlg, IDC_EDIT9);
		hwndPlace[2] = GetDlgItem(hwndDlg, IDC_EDIT10);
		hwndMarriedTo[3] = GetDlgItem(hwndDlg, IDC_EDIT11);
		hwndDate[3] = GetDlgItem(hwndDlg, IDC_EDIT12);
		hwndPlace[3] = GetDlgItem(hwndDlg, IDC_EDIT13);

		for (x = 0; x < 7; x++)
		{
			MarriedToOffset[x] = 0;
			MarrDateOffset[x] = 0;
			MarrPlacOffset[x] = 0;
			Marriages[x].fams = 9999;
			Marriages[x].year = 9999;
		}
		for (Marriage = 0; Marriage < 4; Marriage++)
		{
			SpouseNum = Indiv[IndivNum].Spouseof[Marriage] & 0x7FFFFFFF;
			for (i = 0; i < LastIndiv; i++)
			{
				if (i == IndivNum)
					continue;
				for (ws = 0; (ws < 4) && (Indiv[i].Spouseof[ws]); ws++)
				{
					if (SpouseNum == (Indiv[i].Spouseof[ws] & 0x7FFFFFFF))
					{
						MarriedToOffset[Marriage] = Indiv[i].Name;
						for (z = MarriedToOffset[Marriage]; Buf[z] != '\r' && Buf[z] != '\n'; z++)
							;
						ch = Buf[z];
						Buf[z] = 0;
						SetWindowText(hwndMarriedTo[Marriage], &Buf[MarriedToOffset[Marriage]]);
						Buf[z] = ch;
						goto gotaname;
					}
				}
			}
gotaname:for (x = Fams ; x < FamsEnd; x++)
			{//look for 0 @F6@ FAM, where 6 is Indiv[IndivNum].Spouseof[s]
				if ((Buf[x-1] == '\n') && (Buf[x] == '0') && (Buf[x+2] == '@'))
				{
					x += 3;//to 'F' or number
					if ((Buf[x] > '9') || (Buf[x] < '0'))
						x++;
					for (FamNum = 0; (Buf[x] <= '9') && (Buf[x] >= '0'); x++)
						FamNum = (FamNum * 10) + (Buf[x] - '0');
					if (SpouseNum == FamNum)
					{
						for ( ; ((Buf[x] != '0') || (Buf[x-1] != '\n')); x++)
						{
							if ((Buf[x-1] == '\n') && (Buf[x] == '1'))
							{
								if (FamOffset[Marriage] == 0)
									FamOffset[Marriage] = x;
								if ((Buf[x+2] == 'M') && (Buf[x+3] == 'A') && (Buf[x+4] == 'R') && (Buf[x+5] == 'R'))
								{
									for (x++; (Buf[x-1] != '\n') || ((Buf[x] != '1') && (Buf[x] != '0')); x++)
									{
										if ((Buf[x+2] == 'D') && (Buf[x+3] == 'A') && (Buf[x+4] == 'T'))
										{
											MarrDateOffset[Marriage] = x+7;
											for (z = MarrDateOffset[Marriage]; Buf[z] != '\r' && Buf[z] != '\n'; z++)
												;
											Marriages[Marriage].fams = SpouseNum;
											Marriages[Marriage].year = Atoi(&Buf[z-4]);
											ch = Buf[z];
											Buf[z] = 0;
											SetWindowText(hwndDate[Marriage], &Buf[MarrDateOffset[Marriage]]);
											Buf[z] = ch;
											x = z;
										}
										else if ((Buf[x+2] == 'P') && (Buf[x+3] == 'L') && (Buf[x+4] == 'A')) 
										{
											MarrPlacOffset[Marriage] = x+7;
											for (z = MarrPlacOffset[Marriage]; Buf[z] != '\r' && Buf[z] != '\n'; z++)
												;
											ch = Buf[z];
											Buf[z] = 0;
											SetWindowText(hwndPlace[Marriage], &Buf[MarrPlacOffset[Marriage]]);
											Buf[z] = ch;
											x = z;
										}
									}
									break;
								}
							}
						}
					}
				}
			}
		} 
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			for (Marriage = 3; Marriage >= 0; Marriage--)
			{
				biggest = 0;
				M = -1;
				for (m = 3; m >= 0; m--)
				{
					if (FamOffset[m] > biggest)
					{
						biggest = FamOffset[m];
						M = m;
					}
				}
				if (M == -1)
					break;

				if (MarrDateOffset[M])
				{
					for (z = MarrDateOffset[M]; (Buf[z-1] != '\n') || ((Buf[z] != '1') && (Buf[z] != '0')); z++)
						;
					y = MarrDateOffset[M]-15;//7;
					for (w = y; z < fileSize; w++, z++)
						Buf[w] = Buf[z];
					fileSize = w;//got rid of 1 MARR\r\n2 DATE\r\n1 Jan 2000\r\n2 PLAC\r\nsomewhere
					Buf[fileSize] = 0;//for debugging
				}
				else if (MarrPlacOffset[M])
				{
					for (z = MarrPlacOffset[M]; (Buf[z-1] != '\n') || ((Buf[z] != '1') && (Buf[z] != '0')); z++)
						;
					y = MarrPlacOffset[M]-15;//7;
					for (w = y; z < fileSize; w++, z++)
						Buf[w] = Buf[z];
					fileSize = w;
					Buf[fileSize] = 0;
				}
				else
					y = FamOffset[M];

				DateLen = GetWindowText(hwndDate[M], &Date[7], 256);
				PlacLen = GetWindowText(hwndPlace[M], &Place[7], 256);
				if (DateLen)
				{//add 1 MARR\r\n back in
					x = DateLen;
					for (w = fileSize, z = w+8; w >= y; w--, z--)
						Buf[z] = Buf[w];
					for (z = 0; z < 8; y++, z++)
						Buf[y] = Marr[z];
					fileSize += 8;
					x += 7;
					for (w = fileSize, z = w+x+2; w >= y; w--, z--)
						Buf[z] = Buf[w];
					for (z = 0; z < x; y++, z++)
						Buf[y] = Date[z];
					Buf[y++] = '\r';
					Buf[y++] = '\n';
					fileSize += x+2;
				}
				if (PlacLen)
				{
					x = PlacLen;
					if ((DateLen == 0) && ((Buf[y-3] != 'R') || (Buf[y-4] != 'R')))
					{//add 1 MARR\r\n back in
						for (w = fileSize, z = w+8; w >= y; w--, z--)
							Buf[z] = Buf[w];
						for (z = 0; z < 8; y++, z++)
							Buf[y] = Marr[z];
						fileSize += 8;
					}
					x += 7;
					for (w = fileSize, z = w+x+2; w >= y; w--, z--)
						Buf[z] = Buf[w];
					for (z = 0; z < x; y++, z++)
						Buf[y] = Place[z];
					Buf[y++] = '\r';
					Buf[y++] = '\n';
					fileSize += x+2;
				}
				FamOffset[M] = 0;//don't use it anymore
			}//end of for (Marriage = 3;

			// sort IndivNum's FAMS's by marriage date
			if (Marriages[1].year != 9999) {// if more than 1 marriage
				for (y = 0; Marriages[y].year < 9999; y++) {
					for (x = 0; Marriages[x].year < 9999; x++) {
						if (Marriages[x].year > Marriages[x+1].year) {
							TempMarriages = Marriages[x];
							Marriages[x] = Marriages[x+1];
							Marriages[x+1] = TempMarriages;
						}
					}
				}
				firstX = 0;
				for (x = Indiv[IndivNum].Name; (x < fileSize) && ((Buf[x] != '0') || (Buf[x-1] != '\n')); x++) {
					if ((Buf[x] == '1') && (Buf[x+2] == 'F') && (Buf[x+5] == 'S') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M')) {
						if (!firstX)
							firstX = x;
						for (y = x, z = 0; Buf[y] != '\n'; y++, z++)
							IndivFams[fams].tempFAMS[z] = Buf[y];
						IndivFams[fams].tempFAMS[z++] = Buf[y];
						IndivFams[fams].tempFAMS[z] = 0;

						for (x += 8; Buf[x] != '@'; x++) {
							if ((Buf[x] >= '0') && (Buf[x] <= '9')) {
								IndivFams[fams].fams = Atoi(&Buf[x]);
								break;
							}
						}
						fams++;
					}
				}
				for (y = 0; Marriages[y].year != 9999; y++) {
					for (x = 0; x < 7; x++) {
						if (IndivFams[x].fams == Marriages[y].fams)
							for (z = 0; IndivFams[x].tempFAMS[z] != 0; z++, firstX++)
								Buf[firstX] = IndivFams[x].tempFAMS[z];
					}
				}
			}
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

int CALLBACK EventProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int event, TypeLen, DateLen, PlaceLen;
	static HWND hwndType[6], hwndDate[6], hwndPlace[6];

	switch (message)
	{
	case WM_INITDIALOG:
		hwndType[0] = GetDlgItem(hwndDlg, IDC_EDIT1);
		hwndDate[0] = GetDlgItem(hwndDlg, IDC_EDIT2);
		hwndPlace[0] = GetDlgItem(hwndDlg, IDC_EDIT3);
		hwndType[1] = GetDlgItem(hwndDlg, IDC_EDIT4);
		hwndDate[1] = GetDlgItem(hwndDlg, IDC_EDIT5);
		hwndPlace[1] = GetDlgItem(hwndDlg, IDC_EDIT6);
		hwndType[2] = GetDlgItem(hwndDlg, IDC_EDIT7);
		hwndDate[2] = GetDlgItem(hwndDlg, IDC_EDIT8);
		hwndPlace[2] = GetDlgItem(hwndDlg, IDC_EDIT9);
		hwndType[3] = GetDlgItem(hwndDlg, IDC_EDIT10);
		hwndDate[3] = GetDlgItem(hwndDlg, IDC_EDIT11);
		hwndPlace[3] = GetDlgItem(hwndDlg, IDC_EDIT12);
		hwndType[4] = GetDlgItem(hwndDlg, IDC_EDIT13);
		hwndDate[4] = GetDlgItem(hwndDlg, IDC_EDIT14);
		hwndPlace[4] = GetDlgItem(hwndDlg, IDC_EDIT15);
		hwndType[5] = GetDlgItem(hwndDlg, IDC_EDIT16);
		hwndDate[5] = GetDlgItem(hwndDlg, IDC_EDIT17);
		hwndPlace[5] = GetDlgItem(hwndDlg, IDC_EDIT18);
		for (x = 0; x < 6; x++)
		{
			TypeOffset[x] = 0;
			DateOffset[x] = 0;
			PlacOffset[x] = 0;
		}
		y = 0;
		for (x = Namend; x < BufEnd; x++)
		{
			if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] == 'E') && (Buf[x+3] == 'V') && (Buf[x+4] == 'E') && (Buf[x+5] == 'N'))
			{
				for (x++; x < BufEnd; x++)
				{
					if (Buf[x-1] == '\n')
					{
						if (Buf[x] == '2')
						{
							if ((Buf[x+2] == 'T') && (Buf[x+3] == 'Y') && (Buf[x+4] == 'P') && (Buf[x+5] == 'E'))
							{
								TypeOffset[y] = x;
								for (z = x; Buf[z] != '\r' && Buf[z] != '\n'; z++)
									;
								ch = Buf[z];
								Buf[z] = 0;
								SetWindowText(hwndType[y], &Buf[x+7]);
								Buf[z] = ch;
							}
							else if ((Buf[x+2] == 'D') && (Buf[x+3] == 'A') && (Buf[x+4] == 'T') && (Buf[x+5] == 'E'))
							{
								DateOffset[y] = x;
								for (z = x; Buf[z] != '\r' && Buf[z] != '\n'; z++)
									;
								ch = Buf[z];
								Buf[z] = 0;
								SetWindowText(hwndDate[y], &Buf[x+7]);
								Buf[z] = ch;
							}
							else if ((Buf[x+2] == 'P') && (Buf[x+3] == 'L') && (Buf[x+4] == 'A') && (Buf[x+5] == 'C'))
							{
								PlacOffset[y] = x;
								for (z = x; Buf[z] != '\r' && Buf[z] != '\n'; z++)
									;
								ch = Buf[z];
								Buf[z] = 0;
								SetWindowText(hwndPlace[y], &Buf[x+7]);
								Buf[z] = ch;
							}
						}
						else
						{
							x--;//for the x++ in the loop
							break;
						}
					}
				}
				y++;
			}
		}
		SetFocus(hwndType[0]);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			for (x = Namend; x < BufEnd; x++)
			{//delete all 1 EVEN data for individual
				if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] == 'E') && (Buf[x+3] == 'V') && (Buf[x+4] == 'E') && (Buf[x+5] == 'N'))
				{
					y = x;
					for ( ; x < BufEnd; x++)
					{
						if ((Buf[x-1] == '\n') && ((Buf[x] == '1') || (Buf[x] == '0')) && ((Buf[x+2] != 'E') || (Buf[x+3] != 'V') || (Buf[x+4] != 'E') || (Buf[x+5] != 'N')))
							break;
					}
					for ( ; x < fileSize; x++, y++)
						Buf[y] = Buf[x];
					Buf[y] = 0;
					fileSize = y;
				}
			}
			for (event = 0; event < 6; event++)
			{
				x = 0;
				if (TypeLen = GetWindowText(hwndType[event], &Type[7], 256))//"2 TYPE "
				{
					for (y = 0; (x < 500) && (Type[y] != 0); x++, y++)
						WholeEvent[x] = Type[y];
					WholeEvent[x++] = '\r';
					WholeEvent[x++] = '\n';
				}
				if (DateLen = GetWindowText(hwndDate[event], &Date[7], 256))//"2 DATE "
				{
					for (y = 0; (x < 500) && (Date[y] != 0); x++, y++)
						WholeEvent[x] = Date[y];
					WholeEvent[x++] = '\r';
					WholeEvent[x++] = '\n';
				}
				if (PlaceLen = GetWindowText(hwndPlace[event], &Place[7], 256))//"2 PLAC "
				{
					for (y = 0; (x < 500) && (Place[y] != 0); x++, y++)
						WholeEvent[x] = Place[y];
					WholeEvent[x++] = '\r';
					WholeEvent[x++] = '\n';
				}
				if (x)
				{
					for (y = Namend; y < BufEnd; y++)
					{
						if ((Buf[y] == '1') && (Buf[y-1] == '\n'))
						{//put it in front of one of the following
							if ((Buf[y+2] == 'F') && (Buf[y+3] == 'A') && (Buf[y+4] == 'M'))
								break;
							else if ((Buf[y+2] == 'N') && (Buf[y+3] == 'O') && (Buf[y+4] == 'T'))
								break;
							else if ((Buf[y+2] == 'C') && (Buf[y+3] == 'H') && (Buf[y+4] == 'A'))
								break;
						}
					}
					for (w = fileSize, z = w+8; w >= y; w--, z--)
						Buf[z] = Buf[w];
					for (z = 0; z < 8; y++, z++)
						Buf[y] = Even[z];//1 EVEN\r\n
					fileSize += 8;
					for (w = fileSize, z = w+x; w >= y; w--, z--)
						Buf[z] = Buf[w];
					for (z = 0; z < x; y++, z++)
						Buf[y] = WholeEvent[z];
					fileSize += x;
				}
			}//end of for (event = 5
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

/*
1 NOTE @NI0125@
0 @NI0125@ NOTE
1 CONC [Brrderbund Family Archive #110, Vol. 1, Ed. 4, Social Security Death 
1 CONC Index: U.S., Social Security Death Index, Surnames from A through L, Date 
1 CONC of Import: Feb 26, 1999, Internal Ref. #1.111.4.38527.107]
1 CONT
1 CONT Individual: Carle, Bertha
1 CONT Birth date: Aug 3, 1908
1 CONT Death date: Jan 21, 1988
1 CONT Social Security #: 440-32-1489
1 CONT Last residence: 67042
1 CONT State of issue: OK
*/
int CALLBACK NoteProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	int EnteredNoteLen;
	BOOL odd = FALSE;
	static HWND hwndNote;

	switch (message)
	{
	case WM_INITDIALOG:
		hwndNote = GetDlgItem(hwndDlg, IDC_EDIT1);
		if (nonote == FALSE)
		{
			y = 7;
			for (x = Namend; x < FamsEnd; x++)
			{
				if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] == 'N') && (Buf[x+3] == 'O') && (Buf[x+4] == 'T') && (Buf[x+5] == 'E'))
				{
					NoteBegin = x;
					if (Buf[x+7] == '@')
					{//1 NOTE @NI0125@
						for (x += 8; (x < FamsEnd) && (Buf[x] != '\n'); x++)
							;
						x++;
						if (Buf[x] == '0')
						{
							for ( ; x < FamsEnd; x++)
							{
								if ((Buf[x] == 'N') && (Buf[x+1] == 'O') && (Buf[x+2] == 'T') && (Buf[x+3] == 'E'))
								{
									x += 4;
									odd = TRUE;
									break;
								}
							}
						}
					}
					for ( ; (y < 29999) && (x < FamsEnd); x++, y++)
					{
						if (Buf[x-1] == '\n')
						{
							if (odd)
							{
								if (Buf[x] == '0')
									break;
							}
							else if ((y > 7) && ((Buf[x] == '1') || (Buf[x] == '0')))
								break;
							if ((Buf[x+2] == 'C') && (Buf[x+3] == 'O') && (Buf[x+4] == 'N'))
							{
								if (Buf[x+5] == 'C')//"2 CONC "
									y -= 2;
								if (Buf[x+6] != ' ')
									x--;
							}
							x += 7;//past "2 CONC " or "2 CONT "
						}
						Note2[y] = Buf[x];
					}
					NoteEnd = x;
					break;
				}
			}
			Note2[y] = 0;
			SetWindowText(hwndNote, &Note2[7]);
		}
		SetFocus(hwndNote);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
			maxChars = -7;//for "1 NOTE "
			EnteredNoteLen = GetWindowText(hwndNote, &Note2[7], 30000);
			NoteLen = EnteredNoteLen + 7;
			for (y = 0; y < (NoteLen-1); y++, maxChars++)
			{
				if (Note2[y] == '\n')
				{//add "2 CONT "
					y++;
					for (v = NoteLen+7, x = NoteLen; x >= y; x--, v--)
						Note2[v] = Note2[x];
					NoteLen += 7;
					for (v = 0; v < 7; y++, v++)
						Note2[y] = Cont[v];
					maxChars = -1;
				}
				else if (maxChars == 256)
				{//add "\r\n2 CONC "
					for (v = NoteLen+9, x = NoteLen; x >= y; x--, v--)
						Note2[v] = Note2[x];
					NoteLen += 9;
					for (v = 0; v < 9; y++, v++)
						Note2[y] = Conc[v];
					maxChars = -1;
				}
			}
			y++;
			if (Note2[y-1] != '\n')
			{
				Note2[y++] = '\r';
				Note2[y++] = '\n';
			}
			Note2[y] = 0;
			NoteLen = y;
			if ((EnteredNoteLen == 0) || (nonote == FALSE))
			{
				y = NoteBegin;
				for ( ; NoteEnd < fileSize; NoteBegin++, NoteEnd++)
					Buf[NoteBegin] = Buf[NoteEnd];//remove previous note
				fileSize = NoteBegin;
			}
			if (EnteredNoteLen)
			{
				if (nonote)
				{
					for (y = Namend; y < BufEnd; y++)//put Note before CHAN or at end
						if ((Buf[y] == '1') && (Buf[y-1] == '\n') && (Buf[y+2] == 'C') && (Buf[y+3] == 'H') && (Buf[y+4] == 'A'))
							break;
				}
				for (z = fileSize + NoteLen, w = fileSize; w >= y; w--, z--)
					Buf[z] = Buf[w];
				for (x = y, z = 0; z < (DWORD)NoteLen; x++, z++)
					Buf[x] = Note2[z];
				fileSize += NoteLen;
			}
			EndDialog (hwndDlg, TRUE);
			return TRUE;

		case IDCANCEL:
			EndDialog (hwndDlg, FALSE);
			return FALSE;
		}
	}
	return FALSE;
}

void InitializeAgain(void)
{
	for (x = 0; x < oi; x++)
	{
		w = OrigIndivPtrs[x];
		Indiv[w] = OrigIndiv[x];
		OrigIndivPtrs[x] = 0;
	}
	oi = 0;
	if (arrayfilled)
	{
		for (y = RowsUp; (y < ROWS) && (Array[y][0] != 0xFFFF); y++)
		{
			for (x = 0; Array[y][x] != 0xFFFF; x++)
			{
				for (s = 0; s < MAXSPOUSES; s++)
					Indiv[Array[y][x]].Spouseof[s] &= 0x7FFFFFFF;
				Indiv[Array[y][x]].Childof &= 0x7FFFFFFF;
				Indiv[Array[y][x]].Parent = 0;
				Indiv[Array[y][x]].Spouse = 0;
				Indiv[Array[y][x]].Child = 0;
				Indiv[Array[y][x]].X = 0;
				Indiv[Array[y][x]].LeftChild = 0;
				Indiv[Array[y][x]].RightChild = 0;
				Indiv[Array[y][x]].ArrayX = 0;
				Indiv[Array[y][x]].ArrayY = 0;
				Indiv[Array[y][x]].Flags = 0;
			}
		}
		arrayfilled = FALSE;
		for (x = 1; x < RealLastIndiv; x++)
		{
			Indiv[x].Childof &= 0x7FFFFFFF;
			Indiv[x].Parent = 0;
			Indiv[x].Spouse = 0;
			Indiv[x].Child = 0;
			Indiv[x].X = 0;
			Indiv[x].LeftChild = 0;
			Indiv[x].RightChild = 0;
			Indiv[x].ArrayX = 0;
			Indiv[x].ArrayY = 0;
			Indiv[x].Flags = 0;
		}
	}

	for (x = RealLastIndiv ; x < LastIndiv; x++)
	{
		Indiv[x].Num = 0;
		Indiv[x].Childof = 0;
		for (s = 0; s < MAXSPOUSES; s++)
			Indiv[x].Spouseof[s] = 0;
		Indiv[x].Name = 0;
		Indiv[x].Note = 0;
		Indiv[x].Sex = 0;
		Indiv[x].Birth = 0;
		Indiv[x].BirthLoc = 0;
		Indiv[x].Death = 0;
		Indiv[x].DeathLoc = 0;
		Indiv[x].Parent = 0;
		Indiv[x].Spouse = 0;
		Indiv[x].Child = 0;
		Indiv[x].X = 0;
		Indiv[x].LeftChild = 0;
		Indiv[x].RightChild = 0;
		Indiv[x].ArrayX = 0;
		Indiv[x].ArrayY = 0;
		Indiv[x].Flags = 0;
	}
	LastIndiv = RealLastIndiv;
	if (hwndIndiv)
	{
		if (DestroyWindow(hwndIndiv))
			hwndIndiv = NULL;
	}
	xLoc = yLoc = 0;
	if ((highlighted != 0xFFFFFFFF) && (Buf))
		///////////
		if (fromspace)
		{
			fromspace = FALSE;
			FillArray();
		}
		else
			Fill_Indiv();
		///////////
}

BOOL GetSpouse(void)
{
	BOOL gotspouse = FALSE;

	for (s = 0; s < MAXSPOUSES; s++)
	{
		if ((Indiv[indiv[row]].Spouseof[0]) && (row != MIDROW))
			gotspouse = TRUE;//tricky
		if (Indiv[indiv[row]].Spouseof[s] == 0)
			return gotspouse;
		for (w = 1; w < LastIndiv; w++)
		{
			if (w != indiv[row])
			{
				for (ws = 0; ws < MAXSPOUSES; ws++)
				{
					if (x = Indiv[w].Spouseof[ws]) // yes, =
					{
						if (x == Indiv[indiv[row]].Spouseof[s])
						{// found another indiv with same spouse number
							for (y = MIDROW; (y < ROWS) && (Array[y][0] != 0xFFFF); y++)//y <= row
							{
								for (x = 0; Array[y][x] != 0xFFFF; x++)
								{
									if ((Array[y][x] == w) && (w < RealLastIndiv))
									{//duplicate individual
										OrigIndiv[oi] = Indiv[w];
										OrigIndivPtrs[oi] = w;
										oi++;
										Indiv[w].Flags |= 0x10000000;//to show yellow box
										Indiv[LastIndiv] = Indiv[w];
										Indiv[LastIndiv].Spouse = 1;
										Indiv[LastIndiv].Childof = 0;
										Indiv[LastIndiv].Spouseof[0] = Indiv[w].Spouseof[ws] | 0x80000000;
										for (z = 1; z < MAXSPOUSES; z++)
											Indiv[LastIndiv].Spouseof[z] = 0;
										Indiv[w].Spouseof[ws] = 0;
										for (z = ws; z < MAXSPOUSES; z++)
											Indiv[w].Spouseof[z] = Indiv[w].Spouseof[z+1]; // new Oct 19 2010
										Array[row][Col[row]++] = LastIndiv;
										if (Col[row] == COLS)
										{
											MessageBox(hwnd, TEXT("Too many columns\n(in GetSpouse)"), ERROR, MB_OK);
											highlighted = oldHighlighted;
											bs--;
											nogood = TRUE;
											return FALSE;
										}
										LastIndiv++;
										return TRUE;
									}
								}
							}
							Indiv[w].Spouse = 1;
							Indiv[w].Spouseof[ws] |= 0x80000000;//flag
							Array[row][Col[row]++] = w;
							if (Col[row] == COLS)
							{
								MessageBox(hwnd, TEXT("Too many columns\n(in GetSpouse)"), ERROR, MB_OK);
								highlighted = oldHighlighted;
								bs--;
								nogood = TRUE;
								return FALSE;
							}
							gotspouse = TRUE;
						}
					}// end of if (x = Indiv[w].Spouseof[ws])
					else
						break;//out of for (ws = 0; ws < MAXSPOUSES; ws++)
				}
			}
		}
	}
	return gotspouse;
}

BOOL GetChild(void)
{
	for (s = 0; s < MAXSPOUSES; s++)
	{//look for a child
		spouseof = Indiv[indiv[row]].Spouseof[s] & 0x7FFFFFFF;
		for (w = 1; w < LastIndiv; w++)
		{
			if ((Indiv[w].Childof != 0) && (Indiv[w].Childof == spouseof))
			{
				for (y = MIDROW; (y < ROWS) && (Array[y][0] != 0xFFFF); y++)
				{
					for (z = 0; Array[y][z] != 0xFFFF; z++)
					{
						if ((Array[y][z] == w) && (w < RealLastIndiv))
						{//duplicate individual
							OrigIndiv[oi] = Indiv[w];
							OrigIndivPtrs[oi] = w;
							oi++;
							Indiv[w].Flags |= 0x10000000;//to show yellow box
							Indiv[LastIndiv] = Indiv[w];
							Indiv[LastIndiv].Childof = 0;
							Indiv[w].Childof |= 0x80000000;//flag
							for (x = 0; x < MAXSPOUSES; x++)
								Indiv[LastIndiv].Spouseof[x] = 0;
							Indiv[LastIndiv].Spouse = 0;
							Indiv[LastIndiv].Child = 0;
							gotit = FALSE;
							for (x = 1; x < LastIndiv; x++)
							{//get parent's spouse
								for (ws = 0; (ws < MAXSPOUSES) && (Indiv[x].Spouseof[ws] != 0); ws++)
								{
									if ((x != indiv[row]) && ((Indiv[x].Spouseof[ws] & 0x7FFFFFFF) == spouseof))
									{
										Indiv[x].Child = TRUE;
										Indiv[LastIndiv].Parent = x;
										Indiv[w].Flags |= 8;//for a line up
										gotit = TRUE;
									}
								}
							}
							if (gotit == FALSE)
							{
								Indiv[indiv[row]].Child = TRUE;
								Indiv[LastIndiv].Parent = indiv[row];//to put a line down from the single parent
								Indiv[w].Flags |= 8;//for a line up
							}
							row++;
							if (row >= ROWS)
							{
								MessageBox(hwnd, TEXT("Too many dependant rows"), ERROR, MB_OK);
								highlighted = oldHighlighted;
								bs--;
								nogood = TRUE;
								return 0;
							}
							indiv[row] = LastIndiv;
							Array[row][Col[row]++] = LastIndiv;
							if (Array[row][Col[row]-1] == Array[row][Col[row]-2])
							{
								MessageBox(hwnd, TEXT("Something went wrong\nin the GetChild routine."), ERROR, MB_OK);
								highlighted = oldHighlighted;
								bs--;
								nogood = TRUE;
								return 0;
							}
							LastIndiv++;
							return TRUE;
						}
					}
				}//end of looking for a duplicate
				gotit = FALSE;
				for (x = 1; x < LastIndiv; x++)
				{//get parent's spouse
					for (ws = 0; (ws < MAXSPOUSES) && (Indiv[x].Spouseof[ws] != 0); ws++)
					{
						if ((x != indiv[row]) && ((Indiv[x].Spouseof[ws] & 0x7FFFFFFF) == spouseof))
						{
							Indiv[x].Child = TRUE;
							Indiv[w].Parent = x;
							gotit = TRUE;
						}
					}
				}
				if (gotit == FALSE)
				{
					Indiv[indiv[row]].Child = TRUE;
					Indiv[w].Parent = indiv[row];//to put a line down from the single parent
/*
					z = Indiv[w].Childof;// NEW Aug 29, 2009 (doesn't work like I thought it would)
					for (y = 0; y < 6; y++)
					{// put spouse number in Indiv and FAMS in Buf with single parent in front of others
						if (Indiv[indiv[row]].Spouseof[y] == z)
						{
							if (y)
							{
								DWORD fb = 0;
								DWORD FamsBegin[6], FamsNum[6];
								TCHAR SingleFam[24];

								for ( ; y > 0; y--)
									Indiv[indiv[row]].Spouseof[y] = Indiv[indiv[row]].Spouseof[y-1];
								Indiv[indiv[row]].Spouseof[0] = z;

								for (x = Indiv[indiv[row]].Name; x < fileSize; x++)
								{
									if ((Buf[x] == '1') && (Buf[x+2] == 'F') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M') && (Buf[x+5] == 'S'))
									{
										FamsBegin[fb] = x;
										x += 8;
										if ((Buf[x] > '9') || (Buf[x] < '0'))
											x++;// to FAMS number
										FamsNum[fb] = Atoi(&Buf[x]);
										fb++;
									}
									if ((Buf[x] == '0') && (Buf[x-1] == '\n'))
									{
										for (y = 0; y < fb; y++)
										{
											if (FamsNum[y] == z)
											{
												for (x = FamsBegin[y], z = 0; (z < 24) && (Buf[x] != '\n'); x++, z++)
													SingleFam[z] = Buf[x];
												SingleFam[z] = Buf[x];
												for (y = FamsBegin[y]-1; x > (FamsBegin[0]+z); x--, y--)
													Buf[x] = Buf[y];
												for (x = FamsBegin[0], z = 0; SingleFam[z-1] != '\n'; x++, z++)
													Buf[x] = SingleFam[z];
												break;
											}
										}
										break;
									}
								}
								break;
							}
						}
					}
*/
				}
				Indiv[w].Childof |= 0x80000000;//flag
				row++;
				if (row >= ROWS)
				{
					MessageBox(hwnd, TEXT("Too many dependant rows"), ERROR, MB_OK);
					highlighted = oldHighlighted;
					bs--;
					nogood = TRUE;
					return 0;
				}
				indiv[row] = w;
				Array[row][Col[row]++] = w;
				if (Array[row][Col[row]-1] == Array[row][Col[row]-3])
				{
					MessageBox(hwnd, TEXT("Something went wrong in\nthe GetChild routine"), ERROR, MB_OK);
					highlighted = oldHighlighted;
					bs--;
					nogood = TRUE;
					return 0;
				}
				return TRUE;
			}//end of if ((Indiv[w].Childof) && (Indiv[w].Childof == spouseof))
		}//end of for (w = 1; w < LastIndiv; w++)
		if (spouseof == 0)
			break;//needs to be here for some reason
	}//end of for (s = 0; s < MAXSPOUSES; s++)
	return FALSE;
}//end of GetChild


BOOL PutMarrDivData(DWORD i)
{
	DWORD MarrDate = 0, MarrPlace = 0, DivDate = 0, DivPlace = 0;
	BOOL divorced = FALSE, marrdate = TRUE;

	for ( ; x < FamsEnd; x++)
	{//goto end of 1 MARR
		if (Buf[x] == '\n')
		{
			if ((Buf[x+1] == '1') && (Buf[x+3] == 'D') && (Buf[x+4] == 'I') && (Buf[x+5] == 'V'))
				divorced = TRUE;
			else if (Buf[x+1] == '1')//1 CHAN
			{
				divorced = FALSE;
				marrdate = FALSE;
			}
			else if (Buf[x+1] == '2')
			{
				if ((marrdate) && (Buf[x+3] == 'D') && (Buf[x+4] == 'A') && (Buf[x+5] == 'T') && (Buf[x+6] == 'E') && (MarrDate == FALSE || divorced == TRUE))
				{
					x += 3;//8
					if (divorced == FALSE)
						MarrDate = x;
					else
						DivDate = x;
				}
				else if ((Buf[x+3] == 'P') && (Buf[x+4] == 'L') && (Buf[x+5] == 'A') && (Buf[x+6] == 'C') && (MarrPlace == FALSE || divorced == TRUE))
				{
					x += 3;//8
					if (divorced == FALSE)
						MarrPlace = x;
					else
						DivPlace = x;
				}
			}
			else if (Buf[x+1] == '0')
			{
				if (MarrDate || MarrPlace)
				{
					for (y = 0; y < 10; y++, marrPtr++)
						MarrData[marrPtr] = Married[y];
					for (y = Indiv[i].Name; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
						;
					for (z = Indiv[i].Name; z < y; z++, marrPtr++)
						MarrData[marrPtr] = Buf[z];
					MarrData[marrPtr++] = '\r';
					MarrData[marrPtr++] = '\n';
					MarrData[marrPtr] = 0;
					if (MarrDate)
					{
						for (y = MarrDate; (Buf[y] != '\r') && (Buf[y] != '\n'); y++, marrPtr++)
							MarrData[marrPtr] = Buf[y];
						MarrData[marrPtr++] = '\r';
						MarrData[marrPtr++] = '\n';
						MarrData[marrPtr] = 0;
					}
					if (MarrPlace)
					{
						for (y = MarrPlace; (Buf[y] != '\r') && (Buf[y] != '\n'); y++, marrPtr++)
							MarrData[marrPtr] = Buf[y];
						MarrData[marrPtr++] = '\r';
						MarrData[marrPtr++] = '\n';
						MarrData[marrPtr] = 0;
					}
				}
				if (DivDate || DivPlace)
				{
					for (y = 0; y < 11; y++, marrPtr++)
						MarrData[marrPtr] = Divorced[y];
					for (y = Indiv[i].Name; (Buf[y] != '\r') && (Buf[y] != '\n'); y++)
						;
					for (z = Indiv[i].Name; z < y; z++, marrPtr++)
						MarrData[marrPtr] = Buf[z];
					MarrData[marrPtr++] = '\r';
					MarrData[marrPtr++] = '\n';
					MarrData[marrPtr] = 0;
					if (DivDate)
					{
						for (y = DivDate; (Buf[y] != '\r') && (Buf[y] != '\n'); y++, marrPtr++)
							MarrData[marrPtr] = Buf[y];
						MarrData[marrPtr++] = '\r';
						MarrData[marrPtr++] = '\n';
						MarrData[marrPtr] = 0;
					}
					if (DivPlace)
					{
						for (y = DivPlace; (Buf[y] != '\r') && (Buf[y] != '\n'); y++, marrPtr++)
							MarrData[marrPtr] = Buf[y];
						MarrData[marrPtr++] = '\r';
						MarrData[marrPtr++] = '\n';
						MarrData[marrPtr] = 0;
					}
				}
				return FALSE;
			}
		}
	}
	return TRUE;
}

void GetMarrDiv(void)
{
	DWORD i;

	x += 3;//to 'F' or number
	if ((Buf[x] > '9') || (Buf[x] < '0'))
		x++;
	for (FamNum = 0; (Buf[x] <= '9') && (Buf[x] >= '0'); x++)
		FamNum = (FamNum * 10) + (Buf[x] - '0');
//	if (IndivNum >= RealLastIndiv) // new Oct 19 2010
//	{
//		y = Indiv[IndivNum].Num;
//		for (z = 0; z < RealLastIndiv; z++)
//			if (Indiv[z].Num == y)
//				IndivNum = z;
//	}
	for (s = 0; (s < MAXSPOUSES) && Indiv[IndivNum].Spouseof[s]; s++)
	{
		if ((Indiv[IndivNum].Spouseof[s] & 0x7FFFFFFF) == FamNum)
		{
			for (i = 0; i < LastIndiv; i++)
			{
				if (i == IndivNum)
					continue;
				for (ws = 0; (ws < MAXSPOUSES) && (Indiv[i].Spouseof[ws]); ws++)
				{
					if ((Indiv[i].Spouseof[ws] & 0x7FFFFFFF) == FamNum)
					{
						for ( ; (x < FamsEnd) && ((Buf[x] != '\n') || (Buf[x+1] != '0')); x++)
						{
							if ((Buf[x] == 'M') && (Buf[x+1] == 'A') && (Buf[x+2] == 'R') && (Buf[x+3] == 'R')) 
							{
								if (FALSE == PutMarrDivData(i))
									return;
							}
						}
						return;
					}
				}
			}
		}
	}
}

void InsertMarrData(void)
{
	int y;

	gotmarrdata = TRUE;
	for (y = 0; y < (int)marrPtr; y++, ptr++)
		BigNote[ptr] = MarrData[y];
}

void GetData(void)
{
	BOOL itsanote = FALSE;

	for (x = 0; x < ptr; x++)
		BigNote[x] = 0;
	BigNote[29999] = 0;
	ptr = 0;
	marrPtr = 0;
	gotmarrdata = FALSE;

	BufEnd = GetBufEnd(Namend);
	noname = nonote = TRUE;
	for (x = Namend; x < BufEnd; x++)
	{
		if ((Buf[x] == '\n') && (Buf[x+1] == '1') && (Buf[x+3] == 'N'))
		{
			if ((Buf[x+4] == 'A') && (Buf[x+5] == 'M'))
				noname = FALSE;
			if ((Buf[x+4] == 'O') && (Buf[x+5] == 'T'))
				nonote = FALSE;
		}
	}

	x = Fams;
	GetMarrDiv();
	for (x = Fams ; x < FamsEnd; x++)
	{//look for 0 @F6@ FAM, where 6 is Indiv[IndivNum].Spouseof[s]
		if ((Buf[x] == '\n') && (Buf[x+1] == '0'))
		{
			x++;
			for (y = x; (y < FamsEnd) && (Buf[y] != '\n'); y++)
			{
				if ((Buf[y] == 'F') && (Buf[y+1] == 'A') && (Buf[y+2] == 'M'))
				{
					GetMarrDiv();
					x--;
				}
			}
		}
	} 
	gotit = FALSE;
	for (x = Namend; x < FamsEnd; x++)
	{
		if (Buf[x] == '\n')
		{
			if  ((Buf[x+1] == '0') && (Buf[x+2] == ' ') && (Buf[x+3] == '@'))
			{
				for (y = x+1; Buf[y] != '\n'; y++)
				{
					if ((Buf[y] == 'I') && (Buf[y+1] == 'N') && (Buf[y+2] == 'D') && (Buf[y+3] == 'I')) 
					{
						gotit = TRUE;
						break;
					}
					else if ((Buf[y] == 'F') && (Buf[y+1] == 'A') && (Buf[y+2] == 'M')) 
					{
						gotit = TRUE;
						break;
					}
				}
				if (gotit)
					break;
			}
			x++;
			if ((x < FamsEnd) && (Buf[x] == '1'))
			{
				x += 2;
				for (y = 0; y < 10; y++)//10 doesn't include MARR & DIV
				{
					if ((Buf[x] == Ones[y][0]) && (Buf[x+1] == Ones[y][1]) && (Buf[x+2] == Ones[y][2]))
					{
						switch (y)
						{
							case 0://ALIA
								if (gotmarrdata == FALSE)
									InsertMarrData();
								for (y = 0; (ptr < 29999) && (y < 10); y++, ptr++)
									BigNote[ptr] = Alias[y];
								if (ptr == 29999)
									return;
								x -= 2;
								break;
							case 1://BURI
								if (gotmarrdata == FALSE)
									InsertMarrData();
								for (y = 0; (ptr < 29999) && (y < 11); y++, ptr++)
									BigNote[ptr] = Burial[y];
								if (ptr == 29999)
									return;
								x += 6;
								break;
							case 2://CHR
								if (gotmarrdata == FALSE)
									InsertMarrData();
								for (y = 0; (ptr < 29999) && (y < 15); y++, ptr++)
									BigNote[ptr] = Christened[y];
								if (ptr == 29999)
									return;
								x += 5;
								break;
							case 3://EDUC
								if (gotmarrdata == FALSE)
									InsertMarrData();
								for (y = 0; (ptr < 29999) && (y < 14); y++, ptr++)
									BigNote[ptr] = Education[y];
								if (ptr == 29999)
									return;
								x += 6;// was 7
								break;
							case 4://EVEN
								if (gotmarrdata == FALSE)
									InsertMarrData();
								for (y = 0; (ptr < 29999) && (y < 10); y++, ptr++)
									BigNote[ptr] = Event[y];
								if (ptr == 29999)
									return;
								x += 6;
								break;
							case 5://GRAD
								if (gotmarrdata == FALSE)
									InsertMarrData();
								for (y = 0; (ptr < 29999) && (y < 14); y++, ptr++)
									BigNote[ptr] = Graduated[y];
								if (ptr == 29999)
									return;
								x += 6;
								break;
							case 6://NAME
								for (y = 0; (ptr < 29999) && (y < 9); y++, ptr++)
									BigNote[ptr] = name[y];
								if (ptr == 29999)
									return;
								x -= 2;
								break;
							case 7://NOTE
								if (gotmarrdata == FALSE)
									InsertMarrData();
								for (y = 0; (ptr < 29999) && (y < 7); y++, ptr++)
									BigNote[ptr] = Note[y];
								if (ptr == 29999)
									return;
itsanote = TRUE;
								if ((Buf[x+4] == '\r') || (Buf[x+4] == '\n'))
								{
									x += 5;
									if (Buf[x] == '\n')
										x++;
									break;
								}
								x += 5;
								if (Buf[x] == '@')
								{//e.g. @NI3445@
									x++;
									for (y = x; Buf[y] != '\n'; y++)
										;//start comparing at next line
									for ( ; y < fileSize; y++)
									{
										if ((Buf[y] == '\n') && (Buf[y+1] == '0') && (Buf[y+2] == ' ') && (Buf[y+3] == '@') && (Buf[y+4] == 'N'))
										{
											for (z = 0, y += 4; Buf[y+z] != '@'; z++)
												if (Buf[y+z] != Buf[x+z])
													break;
											if ((Buf[y+z] == '@') && (Buf[x+z] == '@'))
											{//found it
												for (y += z; (y < fileSize) && (Buf[y] != '\n'); y++)
													;//start at next line
												for ( ; (y < fileSize); y++)
												{
													if (Buf[y] == '\n')
													{
x8:														if (Buf[y+1] == '0')
															goto x12;
														if ((Buf[y+1] == '2') && (Buf[y+2] == ' ') && (Buf[y+3] == 'C') && (Buf[y+4] == 'O') && (Buf[y+5] == 'N'))
														{ // CONC or CONT
															y += 8;
															if (Buf[y-2] == 'C')
																ptr -= 2;
															else if (Buf[y] == '\n') // Buf[y-2] == 'T'
																goto x8;
															for ( ; (ptr < 29999) && (y < fileSize) && (Buf[y] != '\n'); y++, ptr++)
																BigNote[ptr] = Buf[y];
															if (ptr < 29999)
																BigNote[ptr++] = Buf[y];
															else//ptr == 29999
																return;
															y--;
														}
													}
												}
												break;
											}
										}
									}
								}
								else
									x -= 7;
								break;
							case 8://OCCU
								if (gotmarrdata == FALSE)
									InsertMarrData();
								for (y = 0; (ptr < 29999) && (y < 14); y++, ptr++)
									BigNote[ptr] = Occupation[y];
								if (ptr == 29999)
									return;
								if ((Buf[x+4] != '\r') && (Buf[x+4] != '\n'))
								{
									for (x += 5; Buf[x] != '\n'; x++, ptr++)
										BigNote[ptr] = Buf[x];
									BigNote[ptr++] = Buf[x++];
									if (Buf[x] == '1')
									{
										x -= 2;
										goto x6;
									}
								}
								else
									x += 6;

								break;
							case 9://RESI
								if (gotmarrdata == FALSE)
									InsertMarrData();
								if (Buf[x+3] != 'I')
									return;
								for (y = 0; (ptr < 29999) && (y < 14); y++, ptr++)
									BigNote[ptr] = Residence[y];
								if (ptr == 29999)
									return;
								x += 6;
								break;
						}
x5:;//Buf[x] is at beginning of line
						if (((Buf[x+2] == 'S') && (Buf[x+3] == 'O') && (Buf[x+4] == 'U') && (Buf[x+5] == 'R'))
						 || ((Buf[x+2] == 'P') && (Buf[x+3] == 'A') && (Buf[x+4] == 'G') && (Buf[x+5] == 'E'))
						 || ((Buf[x+2] == 'S') && (Buf[x+3] == 'P') && (Buf[x+4] == 'O') && (Buf[x+5] == 'U'))
						 || ((Buf[x+2] == 'R') && (Buf[x+3] == 'E') && (Buf[x+4] == 'M') && (Buf[x+5] == 'A'))
						 || ((Buf[x+2] == 'T') && (Buf[x+3] == 'E') && (Buf[x+4] == 'X') && (Buf[x+5] == 'T'))
						 || ((Buf[x+2] == 'Q') && (Buf[x+3] == 'U') && (Buf[x+4] == 'A') && (Buf[x+5] == 'Y'))
						 || ((Buf[x+2] == 'O') && (Buf[x+3] == 'B') && (Buf[x+4] == 'J') && (Buf[x+5] == 'E'))
						 || ((Buf[x+2] == 'T') && (Buf[x+3] == 'I') && (Buf[x+4] == 'T') && (Buf[x+5] == 'L'))
						 || ((Buf[x+2] == 'F') && (Buf[x+3] == 'O') && (Buf[x+4] == 'R') && (Buf[x+5] == 'M'))
						 || ((Buf[x+2] == 'F') && (Buf[x+3] == 'I') && (Buf[x+4] == 'L') && (Buf[x+5] == 'E'))
						 || ((Buf[x+2] == 'D') && (Buf[x+3] == 'A') && (Buf[x+4] == 'T') && (Buf[x+5] == 'A')))
						{
							for ( ; (x < FamsEnd) && (Buf[x] != '\n'); x++)
								;
							x++;
						}
						else
						{
							if ((Buf[x+2] == 'C') && (Buf[x+3] == 'O') && (Buf[x+4] == 'N') && (Buf[x+5] == 'C'))
								ptr -= 2;
//							else if (((Buf[x+2] == 'D') && (Buf[x+3] == 'A') && (Buf[x+4] == 'T') && (Buf[x+5] == 'E'))
//								|| ((Buf[x+2] == 'P') && (Buf[x+3] == 'L') && (Buf[x+4] == 'A') && (Buf[x+5] == 'C'))
//								|| ((Buf[x+2] == 'T') && (Buf[x+3] == 'Y') && (Buf[x+4] == 'P') && (Buf[x+5] == 'E')))
//							{
//								x -= 5;//slightly trick
//							}
							x += 6;
							if (Buf[x] == ' ')
								x++;
							for ( ; (ptr < 29999) && (x < FamsEnd) && (Buf[x] != '\n'); x++, ptr++)//x += 6 (or 7) is for: "2 PLAC "
								BigNote[ptr] = Buf[x];
							if (ptr == 29999)
								return;
							if (Buf[x] == '\n')
							{
								if (Buf[x-1] != '\r')
									BigNote[ptr++] = '\r';
								BigNote[ptr++] = Buf[x++];
							} 
							if (ptr >= 29999)
								return;
						}
						if (Buf[x] != '2')
						{
							x -= 2;//back to just before the '\n'
							break;
						}
						else
							goto x5;
					}
				}
			}
		}
x6:;
	}
x12:if (gotmarrdata == FALSE)
		InsertMarrData();
	BigNote[ptr] = 0;
	BigNoteLen = ptr;
}


void PutFAMS(DWORD Offset)
{
	Famspouse[5] = 'S';
	_itow(FamNum, &Famspouse[9], 10);
	for (y = 9; Famspouse[y] != 0; y++)
		;
	Famspouse[y++] = '@';
	Famspouse[y++] = '\r';
	Famspouse[y++] = '\n';
	Famspouse[y] = 0;
	//if famcfirst, look for 1 FAMC or end of IndivNum's record
	//else put it after 1 FAMS or before end of IndivNum's record or 1 NOTE
	for (x = Offset+1; (x < fileSize) && ((Buf[x-1] != '\n') || (Buf[x] != '0')); x++)
	{
		if ((Buf[x-1] == '\n') && (Buf[x] == '1'))
		{
			if ((Buf[x+2] == 'F') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M') && (Buf[x+5] == 'C'))
			{
				if (famcfirst)
				{//put 1 FAMS after 1 FAMC (and after all 1 FAMSs) in Individual's record
					for (x++; (Buf[x-1] != '\n'); x++)
						;
					if (!specialcase)
					{
						while ((Buf[x+2] == 'F') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M') && (Buf[x+5] == 'S'))
						{
							for (x++; Buf[x-1] != '\n'; x++)
								;
						}
					}
					else
						specialcase = FALSE;
				}
				break;
			}
			else if ((specialcase) && (Buf[x+2] == 'F') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M') && (Buf[x+5] == 'S'))
			{
				specialcase = FALSE;
				break;
			}
			else if ((Buf[w+2] == 'N') && (Buf[w+3] == 'O') && (Buf[w+4] == 'T') && (Buf[w+5] == 'E'))
			{//put 1 FAMS before 1 NOTE in Individual's record
				break;
			}
		}
	}
	for (w = fileSize, z = fileSize+y; w >= x; w--, z--)
		Buf[z] = Buf[w];
	for (w++, z = 0; z < y; w++, z++)
		Buf[w] = Famspouse[z];
	fileSize += y;
	Fams += y;
	FamsEnd += y;
}

void PutFAMC(DWORD Offset)
{
	Famspouse[5] = 'C';
	_itow(FamNum, &Famspouse[9], 10);
	for (y = 9; Famspouse[y] != 0; y++)
		;
	Famspouse[y++] = '@';
	Famspouse[y++] = '\r';
	Famspouse[y++] = '\n';
	Famspouse[y] = 0;
	//if famcfirst, look for 1 FAMS or end of IndivNum's record or 1 NOTE and put 1 FAMC in front of it
	//else put it after 1 FAMS or before end of IndivNum's record or 1 NOTE
	for (x = Offset+1; (x < fileSize) && ((Buf[x-1] != '\n') || (Buf[x] != '0')); x++)
	{
		if ((Buf[x-1] == '\n') && (Buf[x] == '1'))
		{
			if ((Buf[x+2] == 'F') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M') && (Buf[x+5] == 'S'))
			{
				if (famcfirst == FALSE)
				{//put 1 FAMC before 1 FAMS in Individual's record
					for (x++; (Buf[x-1] != '\n'); x++)
						;
				}
				break;
			}
			else if ((Buf[w+2] == 'N') && (Buf[w+3] == 'O') && (Buf[w+4] == 'T') && (Buf[w+5] == 'E'))
			{//put 1 FAMC before 1 NOTE in Individual's record
				break;
			}
		}
	}
	for (w = fileSize, z = fileSize+y; w >= x; w--, z--)
		Buf[z] = Buf[w];
	for (w++, z = 0; z < y; w++, z++)
		Buf[w] = Famspouse[z];
	fileSize += y;
	Fams += y;
	FamsEnd += y;
}

void PutNewFAM(int Person, DWORD Offset)
{
	FamNum = 1;//for first one
	gotit = FALSE;
	for (y = (int)FamsEnd; y >= Fams; y--)
	{//get a new FamNum
		if ((Buf[y] == '0') && (Buf[y+1] == ' ') && (Buf[y+2] == '@'))// && (Buf[y+3] == 'F')
		{
			test = y+3;
			if ((Buf[test] < '0') || (Buf[test] > '9'))
				test++;
			for (test2 = test; Buf[test2] != '\n'; test2++)
			{
				if ((Buf[test2] == 'F') && (Buf[test2+1] == 'A')  && (Buf[test2+2] == 'M')) 
				{
					FamNum = Atoi(&Buf[test]);
					FamNum++;
					gotit = TRUE;
					break;
				}
			}
			if (gotit)
				break;
		}
	}
	y = 0;
	for (x = 0; FamEntry[x] != 0; x++, y++)
		NewIndiv[y] = FamEntry[x];
	_itow(FamNum, &NewIndiv[y], 10);
	for ( ; NewIndiv[y] != 0; y++)
		;
	for (x = 0; Famend[x] != 0; x++, y++)
		NewIndiv[y] = Famend[x];
	if (Person == MAN)
	{
		for (x = 0; FamHusb[x] != 0; x++, y++)
			NewIndiv[y] = FamHusb[x];
		for (z = Offset; Buf[z] != '@'; z++)
			;
		NewIndiv[y++] = Buf[z++];
		for ( ; Buf[z] != '@'; z++, y++)
			NewIndiv[y] = Buf[z];
	}
	else if (Person == WOMAN)
	{
		for (x = 0; FamWife[x] != 0; x++, y++)
			NewIndiv[y] = FamWife[x];
		for (z = Offset; Buf[z] != '@'; z++)
			;
		NewIndiv[y++] = Buf[z++];
		for ( ; Buf[z] != '@'; z++, y++)
			NewIndiv[y] = Buf[z];
	}
	NewIndiv[y++] = '@';
	NewIndiv[y++] = '\r';
	NewIndiv[y++] = '\n';
	NewIndiv[y] = 0;
	for (x = fileSize, z = fileSize+y; x >= FamsEnd; x--, z--)
		Buf[z] = Buf[x];
	for (x++, z = 0; z < y; x++, z++)
		Buf[x] = NewIndiv[z];
	fileSize += y;
	FamsEnd += y;
}

void PutFAM(int Person, DWORD Offset)
{//update Fam record
	for (x = Fams; x < FamsEnd; x++)
	{
		if ((Buf[x-1] == '\n') && (Buf[x] == '0') && (Buf[x+2] == '@'))// && (Buf[x+3] == 'F')
		{
			gotit = FALSE;
			test = x + 3;
			if ((Buf[test] < '0') || (Buf[test] > '9'))
				test++;
			for (test2 = test; Buf[test2] != '\n'; test2++)
			{
				if ((Buf[test2] == 'F') && (Buf[test2+1] == 'A')  && (Buf[test2+2] == 'M')) 
				{
					gotit = TRUE;
					break;
				}
			}
			if (gotit == FALSE)
				continue;
			v = z = test;//pointing to fams number
//
			for ( ; Buf[z] != '@'; z++)
				;
			for (y = 0, w = 1, z--; z >= v; z--)
			{
				y += (Buf[z] - '0') * w;
				w *= 10;
			}
			if (FamNum == y)
			{
				x = v+1;
				if (Person == CHILD)
				{
					BOOL gotwife = FALSE;
					for ( ; (Buf[x] != '\n') || (Buf[x+1] != '0'); x++)
					{
						if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] == 'W') && (Buf[x+3] == 'I') && (Buf[x+4] == 'F') && (Buf[x+5] == 'E'))
						{
							gotwife = TRUE;
							break;
						}
					}
					if (gotwife == FALSE)
					{
						for (x = v+1; (Buf[x] != '\n') || (Buf[x+1] != '0'); x++)
						{
							if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] == 'H') && (Buf[x+3] == 'U') && (Buf[x+4] == 'S') && (Buf[x+5] == 'B'))
								break;
						}
						if ((Buf[x] == '\n') && (Buf[x+1] == '0'))
							MessageBox(hwnd, TEXT("oh-oh"), ERROR, MB_OK);
					}
					for ( ; Buf[x] != '\n'; x++)
						;
					x++;
					for (y = 0; FamChil[y] != 0; y++)
						NewIndiv[y] = FamChil[y];
				}
				else//HUSB or WIFE
				{//put HUSB over first 1 (not 1 MARR) or 0, & if no HUSB same for WIFE
					for ( ; (Buf[x-1] != '\n') || (Buf[x] != '0'); x++)
					{
						if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] != 'M') && (Buf[x+3] != 'A') && (Buf[x+4] != 'R') && (Buf[x+5] != 'R'))
						{
							if (Person == MAN)
							{
								for (y = 0; FamHusb[y] != 0; y++)
									NewIndiv[y] = FamHusb[y];
							}
							else//if (Person == WOMAN)
							{
								if ((Buf[x-1] == '\n') && (Buf[x] == '1') && (Buf[x+2] == 'H') && (Buf[x+3] == 'U') && (Buf[x+4] == 'S') && (Buf[x+5] == 'B'))
									for (x++; Buf[x-1] != '\n'; x++)
										;
								for (y = 0; FamWife[y] != 0; y++)
									NewIndiv[y] = FamWife[y];
							}
							break;
						}
					}
					if ((Buf[x-1] != '\n') && (Buf[x] == '0'))
					{
						if (Person == MAN)
							for (y = 0; FamHusb[y] != 0; y++)
								NewIndiv[y] = FamHusb[y];
						else
							for (y = 0; FamWife[y] != 0; y++)
								NewIndiv[y] = FamWife[y];
					}
				}
				for (z = Offset; Buf[z] != '@'; z++)
					;
				NewIndiv[y++] = Buf[z++];
				for ( ; Buf[z] != '@'; z++, y++)
					NewIndiv[y] = Buf[z];
				NewIndiv[y++] = '@';
				NewIndiv[y++] = '\r';
				NewIndiv[y++] = '\n';
				NewIndiv[y] = 0;
				for (w = fileSize, z = fileSize+y; w >= x; w--, z--)
					Buf[z] = Buf[w];
				for (w++, z = 0; z < y; w++, z++)
					Buf[w] = NewIndiv[z];
				fileSize += y;
				FamsEnd += y;
				break;
			}
		} 
	}
}

BOOL CheckifChild(DWORD Offset)
{
	int families = 0;

	for (x = Offset+1; (x < fileSize) && ((Buf[x] != '\n') || (Buf[x+1] != '0')); x++)
	{
		if ((Buf[x] == '\n') && (Buf[x+1] == '1') && (Buf[x+3] == 'F') && (Buf[x+4] == 'A') && (Buf[x+5] == 'M') && (Buf[x+6] == 'C'))
		{//found 1 FAMC
			families++;
			FamNum = Atoi(&Buf[x+10]);
			for (x++, y = 0; Buf[x] != '\n'; x++, y++)
				Famspouse[y] = Buf[x];
			x--;//to find possible second FAMC
			Famspouse[y++] = '\n';
			Famspouse[y] = 0;
		}

	}
	if (families == 1)
		return TRUE;
	else if (families > 1)
	{
		FamNum = 0xFFFFFFFE;
		return TRUE;
	}
	return FALSE;
}

void GetNameinBuf(DWORD Offset, TCHAR* Name)
{
	for (x = Offset+1, y = 0; (x < fileSize) && ((Buf[x] != '\n') || (Buf[x+1] != '0')); x++)
	{
		if ((Buf[x] == '\n') && (Buf[x+1] == '1') && (Buf[x+3] == 'N') && (Buf[x+4] == 'A') && (Buf[x+5] == 'M') && (Buf[x+6] == 'E'))
		{
			for (x += 8, y = 0; (Buf[x] != '\r') && (Buf[x] != '\n'); x++)
				if (Buf[x] != '/')
					Name[y++] = Buf[x];
			if (Name == ParentsName)
			{
				for (x = 0; And[x] != 0; x++, y++)
					Name[y] = And[x];
				prevy = y;
			}
			else//if (Name == ChildsName)
				for (x = 0; ChildsExtra[x] != 0; x++, y++)
					Name[y] = ChildsExtra[x];
			Name[y] = 0;
			break;
		}
	}
}

void ReStart(void)
{
	DWORD x, y = fileSize-2;// don't look at last '\n'

	errorinbuf = FALSE;
	for (x = 0; x < y; x++) {
		if (Buf[x] == '\n') {
			if ((Buf[x+1] == '\n') || (Buf[x+1] == '\r'))
				continue;
			if ((Buf[x+1] >= '0') && (Buf[x+1] <= '9')) {
				if (Buf[x+2] != ' ') {
					errorinbuf = TRUE;
					break;
				}
			}
			else {
				errorinbuf = TRUE;
				break;
			}
		}
	}
	if (errorinbuf)
	{
		Fill_Indiv();
		for (x++ ; (Buf[x] != '\r') && (Buf[x] != '\n'); x++)
			;
//		ch = Buf[x];
//		Buf[x] = 0;
//		_snwprintf(Error, 512, TEXT("This line:\n%s\nneeds to start with a number and then a space.\nFix it in Notepad (the file is named at the top).\n\nAnd PLEASE send me an email that this happened!\njdmcox@jdmcox.com"), &Buf[x]);
//		MessageBox(hwnd, Error, Filename, MB_OK);
//		Buf[x] = ch;
		DialogBox(hInst, TEXT("FIXBUG"), hwnd, FixBugProc);
		return;
	}

	if ((newfile == FALSE) && (FALSE == CheckBuf()))
	{
		DestroyWindow(hwnd);
		return;
	}
	if (0 == FullFilename[0])
	{
		ofn.lpstrFilter = TEXT(" *.ged\0*.ged\0\0");
		ofn.lpstrFile = FullFilename;
		ofn.lpstrFileTitle = Filename;
		ofn.lpstrTitle = TEXT("Save As");
		ofn.lpstrDefExt = TEXT("ged");
		if (0 == GetSaveFileName(&ofn))
		{
			if (Buf)
			{
				free(Buf);
				Buf = NULL;
			}
			return;
		}
	}
	if (newfile)
	{
		newfile = FALSE;
		for (x = fileSize, y = 0; Trlr[y] != 0; x++, y++)
			Buf[x] = Trlr[y];
		fileSize += y;
	}
	for (x = 0; (FullFilename[x] != '.') && (FullFilename[x] != 0); x++)
		BackupFilename[x] = FullFilename[x];
	if (BackupFilename[x-4] == '.') {
		BackupFilename[x-3] = 'b';
		BackupFilename[x-2] = 'a';
		BackupFilename[x-1] = 'k';
	}
	else {
		BackupFilename[x++] = '.';
		BackupFilename[x++] = 'b';
		BackupFilename[x++] = 'a';
		BackupFilename[x++] = 'k';
	}
	BackupFilename[x] = 0;
	CopyFile(FullFilename, BackupFilename, FALSE);//overwrites existing backup file
//	if (0 == CopyFile(FullFilename, BackupFilename, FALSE))//overwrites existing backup file
//	{
//		LPVOID lpMsgBuf;
//
//		FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER|FORMAT_MESSAGE_FROM_SYSTEM|FORMAT_MESSAGE_IGNORE_INSERTS, NULL, GetLastError(), MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT), (LPTSTR)&lpMsgBuf, 0, NULL);
//		MessageBox(NULL, (LPCTSTR)lpMsgBuf, TEXT("Error"), MB_OK|MB_ICONINFORMATION);
//		LocalFree(lpMsgBuf);
//	}
	utf = (BYTE*)VirtualAlloc(NULL, 2*fileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	for (x = 0, y = 0; x < fileSize; x++)
	{
		if (Buf[x] < 0x80)
			utf[y++] = *(BYTE*)&Buf[x];
		else if (Buf[x] < 0x800)
		{
			utf[y++] = ((BYTE)(MASK2BYTES | Buf[x] >> 6));
			utf[y++] = ((BYTE)(MASKBYTE | Buf[x] & MASKBITS));
		}
		else if(Buf[x] < 0x10000)
		{
			utf[y++] = ((BYTE)(MASK3BYTES | Buf[x] >> 12));
			utf[y++] = ((BYTE)(MASKBYTE | Buf[x] >> 6 & MASKBITS));
			utf[y++] = ((BYTE)(MASKBYTE | Buf[x] & MASKBITS));
		}
		else
			MessageBox(hwnd, TEXT("HUH?"), NULL, MB_OK);
	}
	hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	if (0 == WriteFile(hFile, utf, y, &dwBytesWritten, NULL))
		MessageBox(hwnd, FullFilename, TEXT("This file was NOT written:"), MB_OK);
	else
		FlushFileBuffers(hFile);
	CloseHandle(hFile);
	VirtualFree(utf, 0, MEM_RELEASE);
	xLoc = yLoc = 0;
	anyeditit = FALSE;
	editit = FALSE;
	arrayfilled = FALSE;
	if (IndivNum == 0xFFFFFFFF)
		IndivNumber = 0xFFFFFFFF;
//	else if (Indiv)
//		IndivNumber = Indiv[IndivNum].Num;
	fromrestart = TRUE;
	/////////////
	Fill_Indiv();
	/////////////
	fromrestart = FALSE;
}

void CheckFAM(void)
{
	DWORD x, y;

	HusbOffset = WifeOffset = Children = child = SingleChildOffset = 0;
	for (x = 0; x < 21; x++)
		ChildOffset[x] = 0;
	for (x = v+1; x < fileSize; x++)
	{
		if (Buf[x] == '\n')
		{
			x++;
			if (Buf[x] == '0')
				break;
			if (Buf[x] == '1')
			{
				if ((Buf[x+2] == 'C') && (Buf[x+3] == 'H') && (Buf[x+4] == 'I'))
				{
					Children++;
					y = x+8;
					if ((Buf[y] < '0') || (Buf[y] > '9'))
						y++;
					if (FamC == FamNum)
					{//if Indiv[IndivNum].Childof
						if (Atoi(&Buf[y]) == (int)Indiv[IndivNum].Num)
							SingleChildOffset = x;
					}
					else//if Indiv[IndivNum].Spouseof
						ChildOffset[child++] = x;
				}
				else if ((Buf[x+2] == 'H') && (Buf[x+3] == 'U'))
				{
					HusbOffset = x;
					y = x+8;
					if ((Buf[y] < '0') || (Buf[y] > '9'))
						y++;
					HusbNum = Atoi(&Buf[y]);
				}
				else if ((Buf[x+2] == 'W') && (Buf[x+3] == 'I'))
				{
					WifeOffset = x;
					y = x+8;
					if ((Buf[y] < '0') || (Buf[y] > '9'))
						y++;
					WifeNum = Atoi(&Buf[y]);
				}
			}
		}
	}
}

void GetSpouseOffset(DWORD Num)
{
	DWORD i, p, x, FamPointedTo;

	for (i = 0; Indiv[i].Num != Num; i++)
		;
	for (x = Indiv[i].Name; ; x++)
	{
		if (Buf[x] == '\n')
		{
			x++;
			if (Buf[x] == '0')
				break;
			else if ((Buf[x] == '1') && (Buf[x+2] == 'F') && ((Buf[x+5] == 'C')) || ((Buf[x+5] == 'S')))
			{
				FamPointedTo = (DWORD)Atoi(&Buf[x+9]);
				if (FamNum == FamPointedTo)
				{
					for (p = 0; p < 21; p++)
						if (SpouseOffset[p] == x)
							break;
					if (p == 21)
						SpouseOffset[so++] = x;
				}
			}
		}
	}
}

DWORD GetBufEnd(DWORD x)
{//start with Buf[x] && end with Buf[y]
x9:		for ( ; (x != 0) && ((Buf[x] != '0') || (Buf[x+1] != ' ') || (Buf[x+2] != '@')); x--)
			;
		//look for INDI or FAM at end of line beginning with 0 @
		for (y = x+3; (y < fileSize) && (Buf[y] != '@'); y++)
			;
		for ( ; (y < fileSize) && (Buf[y] != '\n'); y++)
		{
			if (((Buf[y] == 'I') && (Buf[y+1] == 'N') && (Buf[y+2] == 'D') && (Buf[y+3] == 'I'))
			 || ((Buf[y] == 'F') && (Buf[y+1] == 'A') && (Buf[y+2] == 'M')))
			{
				break;
			}
		}
		if ((Buf[y] != 'I') && (Buf[y] != 'F'))
			goto x9;
		for (y = NameX; (y < fileSize) && ((Buf[y] != '0') || (Buf[y+1] != ' ') || ((Buf[y+2] != '@') && (Buf[y+2] != 'T'))); y++)
			;
		if (Buf[y+2] == '@')
			z = y+3;
x10:	for ( ; (z < fileSize) && ((Buf[z] != '\n') || (Buf[z+1] != ' ') || ((Buf[z+2] != '@') && (Buf[y+2] != 'T'))); z++)
		{
			if (((Buf[z] == 'I') && (Buf[z+1] == 'N') && (Buf[z+2] == 'D') && (Buf[z+3] == 'I'))
			 || ((Buf[z] == 'F') && (Buf[z+1] == 'A') && (Buf[z+2] == 'M'))
			 || ((Buf[z] == 'T') && (Buf[z+1] == 'R') && (Buf[z+2] == 'L') && (Buf[z+3] == 'R')))
			{
				break;
			}
		}
		if ((z < fileSize) && (Buf[z] != 'I') && (Buf[z] != 'F') && (Buf[z] != 'T'))
			goto x10;
		IndivOffset = x;
		return y;
}

void SaveEdited(void)
{
	TCHAR tempIndiv[31000];

	BirthDateLen = GetWindowText(hwndBirth, &BirthDate[7], 32);//"2 DATE "
	if (BirthDateLen == 0)
		BirthDate[7] = 0;
	BirthLocLen = GetWindowText(hwndBirthLoc, &BirthLoc2[7], 256);//"2 PLAC "
	if (BirthLocLen == 0)
		BirthLoc2[7] = 0;
	Death2Len = GetWindowText(hwndDeath, &Death2[7], 32);
	if (Death2Len == 0)
		Death2[7] = 0;
	DeathLocLen = GetWindowText(hwndDeathLoc, &DeathLoc2[7], 256);
	if (DeathLocLen == 0)
		DeathLoc2[7] = 0;
	if (0 == GetWindowText(hwndName, &Name2[7], 256))
	{
		updateindiv = FALSE;
		return;
	}
	for (x = 0, y = 0; Name2[x] != 0; x++)
		if (Name2[x] == '/')
			y++;
	if (y != 2)
	{
		for (x = 0, y = 0; Name2[x] != 0; x++)
			if (Name2[x] != '/')
				Name2[y++] = Name2[x];
		Name2[y++] = '/';//put the last one in
		Name2[y++] = 0;
		Name2[y] = 0;//necessary
		for ( ; (y > 0) && (Name2[y] != ' '); y--)
			Name2[y] = Name2[y-1];
		Name2[y+1] = '/';
	}

	for (x = IndivOffset, y = 0; x < Indiv[IndivNum].Name; x++, y++)
			tempIndiv[y] = Buf[x];
	for (z = 7, w = 0; Name2[z] != 0; y++, z++)
	{
		tempIndiv[y] = Name2[z];
		if (Name2[z] == '/')
			w++;
	}
	tempIndiv[y++] = '\r';
	tempIndiv[y++] = '\n';
	for ( ; Buf[x-1] != '\n'; x++)
		;
	if ((Buf[x] == '1') && (Buf[x+2] == 'N') && (Buf[x+3] == 'A') && (Buf[x+4] == 'M') && (Buf[x+5] == 'E'))
	{
		for (z = 0; z < 7; y++, z++)
			tempIndiv[y] = Name2[z];
		for (x += 7; Buf[x-1] != '\n'; x++, y++)
			tempIndiv[y] = Buf[x];//second name
	}
	for ( ; x < BufEnd; x++, y++)
	{
		if ((Buf[x] == '1') && (Buf[x-1] == '\n') && ((Buf[x+2] != 'S') || (Buf[x+3] != 'E') || (Buf[x+4] != 'X')))
			break;
		tempIndiv[y] = Buf[x];//fill tempIndiv past "1 SEX\\r\n"
	}
	if ((Buf[x+2] == 'B') && (Buf[x+3] == 'I') && (Buf[x+4] == 'R') && (Buf[x+5] == 'T'))
	{ 
		for (x++ ; x < BufEnd; x++)
		{//put x past birth data
			if ((Buf[x] == '1') && (Buf[x-1] == '\n'))
				break;
		}
	}
	if (BirthDateLen || BirthLocLen || Indiv[IndivNum].Birth || Indiv[IndivNum].BirthLoc)
	{//if any new or old birth data
		for (z = 0; Birth1[z] != 0; y++, z++)
			tempIndiv[y] = Birth1[z];//"1 BIRT\r\n"
		if (BirthDateLen)
		{
			for (z = 0; BirthDate[z] != 0; y++, z++)
				tempIndiv[y] = BirthDate[z];
			tempIndiv[y++] = '\r';
			tempIndiv[y++] = '\n';
		}
		if (BirthLocLen)
		{
			for (z = 0; BirthLoc2[z] != 0; y++, z++)
				tempIndiv[y] = BirthLoc2[z];
			tempIndiv[y++] = '\r';
			tempIndiv[y++] = '\n';
		}
		else if (BirthDateLen == 0)//also
			y -= 8;//back up over "1 BIRT\r\n"
	}

	if ((Buf[x+2] == 'D') && (Buf[x+3] == 'E') && (Buf[x+4] == 'A') && (Buf[x+5] == 'T'))
	{ 
		for (x++ ; x < BufEnd; x++)
		{//put x past birth data
			if ((Buf[x] == '1') && (Buf[x-1] == '\n'))
				break;
		}
	}
	if (Death2Len || DeathLocLen || Indiv[IndivNum].Death || Indiv[IndivNum].DeathLoc)
	{
		for (z = 0; Death1[z] != 0; y++, z++)
			tempIndiv[y] = Death1[z];//"1 DEAT\r\n"
		if (Death2Len)
		{
			for (z = 0; Death2[z] != 0; y++, z++)
				tempIndiv[y] = Death2[z];
			tempIndiv[y++] = '\r';
			tempIndiv[y++] = '\n';
		}
		if (DeathLocLen)
		{
			for (z = 0; DeathLoc2[z] != 0; y++, z++)
				tempIndiv[y] = DeathLoc2[z];
			tempIndiv[y++] = '\r';
			tempIndiv[y++] = '\n';
		}
		else if (Death2Len == 0)//also
			y -= 8;//back up over "1 DEAT\r\n"
	}

	for ( ; x < BufEnd; x++, y++)
		tempIndiv[y] = Buf[x];//the rest of it
	if (y > (BufEnd-IndivOffset))
	{
		for (z = fileSize + (y - (BufEnd-IndivOffset)), w = fileSize; w >= BufEnd; w--, z--)
			Buf[z] = Buf[w];
		for (x = IndivOffset, z = 0; z < y; x++, z++)
			Buf[x] = tempIndiv[z];
		fileSize += y - (BufEnd-IndivOffset);
	}
	else if (y < (BufEnd-IndivOffset))
	{
		for (x = IndivOffset, z = 0; z < y; x++, z++)
			Buf[x] = tempIndiv[z];
		for (y = BufEnd; y < fileSize; x++, y++)
			Buf[x] = Buf[y];
		fileSize = x;
	}
	else
		for (x = IndivOffset, z = 0; z < y; x++, z++)
			Buf[x] = tempIndiv[z];

	ReStart();
	errorinbuf = FALSE;
}

BOOL CheckBuf(void)
{
	DWORD y, x;

//	for (x = 0; (x < fileSize) && (Buf[x] != '\r')  && (Buf[x] != '\n'); x++)
//		;
//	x -= 6;// to "0 HEAD"
	if ((Buf[0] != '0') || (Buf[1] != ' ') || (Buf[2] != 'H') || (Buf[3] != 'E') || (Buf[4] != 'A') || (Buf[5] != 'D'))
	{
		MessageBox(hwnd, TEXT("File doesn't start with 0 HEAD"), ERROR, MB_OK);
		return FALSE;
	}
	for (x = fileSize-1; (Buf[x] == '\n') || (Buf[x] == '\r'); x--)
		;
	for (y = x; (y < fileSize) && (Buf[y-1] != '\n'); y++)
		;
	if (Buf[y] == '\r')
		fileSize = y; // get rid of multiple line feeds / carriage returns
	for ( ; (x != 0) && (Buf[x] != '\n'); x--)
		;
	x++;
	if ((Buf[x] != '0') || (Buf[x+1] != ' ') || (Buf[x+2] != 'T') || (Buf[x+3] != 'R') || (Buf[x+4] != 'L') || (Buf[x+5] != 'R'))
	{
		MessageBox(hwnd, TEXT("File doesn't end with 0 TRLR"), ERROR, MB_OK);
		return FALSE;
	}
	else
		Fams = x;
	badformat = FALSE;
	SetCursor(hWaitingCursor);
	for (x = 0; x < fileSize; x++)
	{
/*
if ((Buf[x] == '\n') && (Buf[x-1] != '\r')) {
	x--;
	fileSize++;
	for (y = fileSize-1; y >= x; y--)
		Buf[y+1] = Buf[y];
	Buf[x++] = '\r';
	badformat = TRUE;
}
if ((Buf[x] == '\r') && (Buf[x+1] != '\n')) {
	x++;
	fileSize++;
	for (y = fileSize-1; y >= x; y--)
		Buf[y+1] = Buf[y];
	Buf[x++] = '\n';
	badformat = TRUE;
}
*/
		if ((Buf[x+1] == '2') && (Buf[x+2] == ' ') && (Buf[x+3] == 'C') && (Buf[x+4] == 'O') && (Buf[x+5] == 'N') && (Buf[x+6] == 'T') && (Buf[x] != '\n'))
		{ // new Oct 19 2010
			fileSize++;
			for (y = fileSize-1; y >= x; y--)
				Buf[y+1] = Buf[y];
			Buf[x++] = '\r';
			Buf[x++] = '\n';
			badformat = TRUE;
		}
		else if ((Buf[x+1] == '0') && (Buf[x+2] == ' ') && (Buf[x+3] == '@') && (Buf[x] != '\n'))
		{ // new Oct 19 2010
			fileSize++;
			for (y = fileSize; y >= x; y--)
				Buf[y+1] = Buf[y];
			Buf[x++] = '\r';
			Buf[x++] = '\n';
			badformat = TRUE;
		}
		if ((Buf[x] == '\n') && (Buf[x+1] == '\n'))
		{
			Buf[x] = '\r';
			x++;
		}
	}
	SetCursor(hCursor);
	if (badformat)
		return FALSE;
	else
		return TRUE;
}

void GetDate(void)
{
			if ((one == 'J') && (two == 'A') && (three == 'N'))
				{gotmonth = TRUE; w += 3;}
			else if ((one == 'F') && (two == 'E') && (three == 'B'))
				{gotmonth = TRUE; w += 3; date[d] += 31;}
			else if ((one == 'M') && (two == 'A') && (three == 'R'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28;}
			else if ((one == 'A') && (two == 'P') && (three == 'R'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31;}
			else if ((one == 'M') && (two == 'A') && (three == 'Y'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31+30;}
			else if ((one == 'J') && (two == 'U') && (three == 'N'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31+30+31;}
			else if ((one == 'J') && (two == 'U') && (three == 'L'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31+30+31+30;}
			else if ((one == 'A') && (two == 'U') && (three == 'G'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31+30+31+30+31;}
			else if ((one == 'S') && (two == 'E') && (three == 'P'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31+30+31+30+31+31;}
			else if ((one == 'O') && (two == 'C') && (three == 'T'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31+30+31+30+31+31+30;}
			else if ((one == 'N') && (two == 'O') && (three == 'V'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31+30+31+30+31+31+30+31;}
			else if ((one == 'D') && (two == 'E') && (three == 'C'))
				{gotmonth = TRUE; w += 3; date[d] += 31+28+31+30+31+30+31+31+30+31+30;}
}

void ParseBirthday(void)
{
	for (w = 0; (Birth[w] != '\r') && (Birth[w] != '\n'); w++)
	{//1 Jan 1999 or Jan 1, 1999 or ...
		if ((Birth[w] >= '0') && (Birth[w] <= '9'))
		{
			u = Atoi(&Buf[w]);
			if (u <= 31)
			{
				date[d] += u;
				gotdate = TRUE;
			}
			for (v = w; ((Birth[v] >= '0') && (Birth[v] <= '9')); v++)
				;
			w = v;
		}
		if (((Birth[w] & 0xDF) >= 'A') && ((Birth[w] & 0xDF) <= 'Z'))
		{
			one = Birth[w] & 0xDF;
			two = Birth[w+1] & 0xDF;
			three = Birth[w+2] & 0xDF;
			GetDate();
		}
	}
	if (mouseover == AGE)
	{
		if (gotmonth == 0)
			date[d] = 0;
	}
	else if ((gotdate == 0) || (gotmonth == 0))
		date[d] = 0;
}

void ParseBirthday2(void)
{
	for (w = 0; (Death[w] != '\r') && (Death[w] != '\n'); w++)
	{//1 Jan 1999 or Jan 1, 1999 or ...
		if (date[1] > date[0])
			return;
		if ((Death[w] >= '0') && (Death[w] <= '9'))
		{
			u = Atoi(&Death[w]);
			if (u <= 31)
			{
				date[d] += u;
				gotdate = TRUE;
			}
			for (v = w; ((Death[v] >= '0') && (Death[v] <= '9')); v++)
				;
			w = v;
		}
		if (((Death[w] & 0xDF) >= 'A') && ((Death[w] & 0xDF) <= 'Z'))
		{
			one = Death[w] & 0xDF;
			two = Death[w+1] & 0xDF;
			three = Death[w+2] & 0xDF;
			GetDate();
		}
	}
	
	if ((gotdate == 0) || (gotmonth == 0))
		date[d] = 0;
}

void ShowJpeg(void)
{
	int x, y;

	if ((JpegBuf[0] != 0xFF) || (JpegBuf[1] != 0xD8) || (JpegBuf[2] != 0xFF))
	{//if not a jpeg file
		MessageBox(hwnd, TEXT("That's not a jpeg file."), ERROR, MB_OK);
		return;
	}
	jerr = ijlInit (&jcprops);//use Intel's ijl15.dll to convert JPEG files
	jcprops.JPGBytes = JpegBuf;//source
	jcprops.JPGSizeBytes = JpegFileSize;
	jcprops.JPGFile = NULL;
	jerr = ijlRead(&jcprops, IJL_JBUFF_READPARAMS);
	JpegWidth = jcprops.JPGWidth;
	JpegHeight = jcprops.JPGHeight;
	if (((rect.right-10) > JpegWidth) && ((rect.bottom-20) > JpegHeight))
		Resolution = FULL;
	else if (((rect.right-10) > JpegWidth/2) && ((rect.bottom-20) > JpegHeight/2))
	{
		Resolution = HALF;
		JpegWidth /= 2;
		JpegHeight /= 2;
	}
	else if (((rect.right-10) > JpegWidth/4) && ((rect.bottom-20) > JpegHeight/4))
	{
		Resolution = QUARTER;
		JpegWidth /= 4;
		JpegHeight /= 4;
	}
	else if (((rect.right-10) > JpegWidth/8) && ((rect.bottom-20) > JpegHeight/8))
	{
		Resolution = EIGHTH;
		JpegWidth /= 8;
		JpegHeight /= 8;
	}
	jcprops.DIBWidth = JpegWidth;
	jcprops.DIBHeight = JpegHeight;
	PadBytes = IJL_DIB_PAD_BYTES(JpegWidth, 3);
	jcprops.DIBPadBytes = PadBytes;
	photo_pixel_buf = (BYTE*)VirtualAlloc(NULL, ((JpegWidth + jcprops.DIBPadBytes) * JpegHeight * 3), MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
	jcprops.DIBBytes = photo_pixel_buf;//destination
	jcprops.DIBChannels = 3;
	jcprops.DIBColor = IJL_BGR;
	if (Resolution == FULL)
		jerr = ijlRead (&jcprops, IJL_JBUFF_READWHOLEIMAGE);
	else if (Resolution == HALF)
		jerr = ijlRead (&jcprops, IJL_JBUFF_READONEHALF);
	else if (Resolution == QUARTER)
		jerr = ijlRead (&jcprops, IJL_JBUFF_READONEQUARTER);
	else if (Resolution == EIGHTH)
		jerr = ijlRead (&jcprops, IJL_JBUFF_READONEEIGHTH);
	ijlFree(&jcprops);

	hMenu2 = CreateMenu();//to override parent menu bar
	x = JpegWidth+(Frame*2);
	y = JpegHeight+GetSystemMetrics(SM_CYCAPTION)+Frame;
	hwndPhoto = CreateWindow(Photos, NULL,
		WS_POPUP | WS_VISIBLE | WS_DLGFRAME | WS_CAPTION | WS_SYSMENU,
		(cxScreen/2)-(x/2), ((cyScreen+TitleAndMenu+Frame)/2)-(y/2), x, y,
		hwndDescEdit, hMenu2, hInst, NULL);
	_snwprintf(Error, 512, TEXT("Press Esc  [%s]"), Jpeg);
	SetWindowText(hwndPhoto, Error);
}
/*
void GetPhotoFileName(void)
{
	for (x = 0; Filename[x] != '.'; x++)
	{
		SimpleFamilyTreePhotos[x] = Filename[x];
		SimpleFamilyTreePhotosTxt[x] = Filename[x];
	}
	for (y = 0; PhotoDta[y] != 0; x++, y++)
	{
		SimpleFamilyTreePhotos[x] = PhotoDta[y];//"Photos.dta"
		SimpleFamilyTreePhotosTxt[x] = PhotoTxt[y];//"Photos.txt"//presuming same length
	}
	SimpleFamilyTreePhotos[x] = 0;
	SimpleFamilyTreePhotosTxt[x] = 0;
}
*/
void UndoIt(void)
{
	if (Filename[0])
	{
		if (IDOK == MessageBox(hwnd, TEXT("Undo the last change you made?"), TEXT(""), MB_DEFBUTTON2|MB_OKCANCEL))
		{
			if (FullFilename[0] == 0)
			{
				ch = FullFilename[0];
				ofn.lpstrFilter = TEXT(" *.ged\0*.ged\0\0");
				ofn.lpstrFile = FullFilename;
				ofn.lpstrFileTitle = Filename;
				ofn.lpstrTitle = TEXT("Open a File to Swap");
				ofn.lpstrDefExt = TEXT("ged");
				if (!GetOpenFileName(&ofn))
					return;
			}
			for (x = 0; FullFilename[x] != 0; x++)
				BackupFilename[x] = FullFilename[x];
			BackupFilename[x-3] = 'b';
			BackupFilename[x-2] = 'a';
			BackupFilename[x-1] = 'k';
			BackupFilename[x] = 0;
			if (MoveFile(BackupFilename, TempFilename))
			{
				MoveFile(FullFilename, BackupFilename);
				MoveFile(TempFilename, FullFilename);

				for (x = 0; x < 512; x++)
					Backspace[x] = -1;
				bs = 0;
				xLoc = yLoc = 0;
				arrayfilled = FALSE;
				highlighted = 0xFFFFFFFF;
				IndivNum = 0xFFFFFFFF;
				IndivNumber = 0xFFFFFFFF;
				LastIndiv = 0;
				if (Buf != NULL)
				{
					free(Buf);
					Buf = NULL;
				}
				Fill_Indiv();
				_snwprintf(TitleBar, 300, TEXT(" %s  [%s]"), szAppName, FullFilename);
				SetWindowText(hwnd, TitleBar);
				undone = TRUE;
				SendMessage(hwnd, WM_COMMAND, ID_SHOWLISTOFINDIVIDUALS, 0);//show LIST OF INDIVIDUALS
			}
			else
				MessageBox(hwnd, FullFilename, TEXT("There's no backup file for:"), MB_OK);
		}
	}
}

void CheckForPhotos(void)
{
	DWORD i, x2 = 0;

//	GetPhotoFileName();
	for (x = 0; (x < MAX_PATH) && (Filename[x] != 0) && (Filename[x] != '.'); x++)
	{
		SimpleFamilyTreePhotos[x] = Filename[x];
		SimpleFamilyTreePhotosTxt[x] = Filename[x];
		SimpleFamilyTreePhotosBak[x] = Filename[x];
	}
	for (y = 0; PhotoDta[y] != 0; x++, y++)
	{
		SimpleFamilyTreePhotos[x] = PhotoDta[y];//"Photos.dta"
		SimpleFamilyTreePhotosTxt[x] = PhotoTxt[y];//"Photos.txt"//presuming same length
		SimpleFamilyTreePhotosBak[x] = PhotoBak[y];//"Photos.bak"//presuming same length
	}
	SimpleFamilyTreePhotos[x] = 0;
	SimpleFamilyTreePhotosTxt[x] = 0;
	SimpleFamilyTreePhotosBak[x] = 0;

	hPhotoFile = CreateFile(SimpleFamilyTreePhotosTxt, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhotoFile != INVALID_HANDLE_VALUE)
	{
		if (PhotoFileSize = GetFileSize(hPhotoFile, NULL))
		{
			utf = (BYTE*)calloc(1, PhotoFileSize);
			ReadFile(hPhotoFile, utf, PhotoFileSize, &dwBytesRead, NULL);
			CloseHandle(hPhotoFile);
			hPhotoFile = NULL;
			PhotoBuf = (TCHAR*)malloc(sizeof(TCHAR)*(PhotoFileSize+10000));//for adding @I123@ on each line
			for (x = 0, y = 0; x < PhotoFileSize; )
			{
				if(((utf[x] & MASK3BYTES) == MASK3BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE) && ((utf[x+2] & MASKBYTE) == MASKBYTE))
				{// 1110xxxx 10xxxxxx 10xxxxxx
					PhotoBuf[y++] = ((utf[x] & 0x0F) << 12) | ((utf[x+1] & MASKBITS) << 6) | (utf[x+2] & MASKBITS);
					x += 3;
				}
				else if(((utf[x] & MASK2BYTES) == MASK2BYTES) && ((utf[x+1] & MASKBYTE) == MASKBYTE))
				{// 110xxxxx 10xxxxxx
					PhotoBuf[y++] = ((utf[x] & 0x1F) << 6) | (utf[x+1] & MASKBITS);
					x += 2;
				}
				else if(utf[x] < MASKBYTE)// 0xxxxxxx
					PhotoBuf[y++] = utf[x++];
				else
					PhotoBuf[y++] = utf[x++];//not utf-8 character
			}
			free(utf);

			for (x2 = 0; x2 < (PhotoFileSize-1); x2++)
			{
				if (PhotoBuf[x2] == '\n')
				{
					x = x2+1;//to beginning of next line
					for (y = 0; ((PhotoBuf[x] != '@') && (PhotoBuf[x] != '<')) && (x < PhotoFileSize); x++, y++)
						Name[y] = PhotoBuf[x];
					Name[y-1] = 0;//ignore ' ' before '<'
					if (PhotoBuf[x] == '@')
					{//search on IndivNum
						for ( ; (PhotoBuf[x] < '0') || (PhotoBuf[x] > '9'); x++)
							;
						PhotoID = Atoi(&PhotoBuf[x]);
						for (i = 0; i < LastIndiv; i++)
						{
							if (PhotoID == Indiv[i].Num)
							{
								for (y = x; (PhotoBuf[y] != '@') && (y < PhotoFileSize); y++)
									;
								if (y != PhotoFileSize)
								{
									y += 2;//to beginning of photot filename
									x = y;
									for ( ; (y < PhotoFileSize) && (PhotoBuf[y] != '\r'); y++)
										;
									ch = PhotoBuf[y];
									PhotoBuf[y] = 0;
									if (INVALID_HANDLE_VALUE != FindFirstFile(&PhotoBuf[x], &fd))
										Indiv[i].Flags |= 0x100000;
									PhotoBuf[y] = ch;
									break;
								}
							}
						}
					}
					else if (PhotoBuf[x] == '<')
					{
						for (i = 1; i < LastIndiv; i++)
						{
							NamePtr = &Buf[Indiv[i].Name];
							for (z = 0; z < 30; z++)
								if (NamePtr[z] != Name[z])
									break;
							if (NamePtr[z] == '\r')
							{
								for (y = x+3; (y < PhotoFileSize) && (PhotoBuf[y] != '\r'); y++)
									;
								ch = PhotoBuf[y];
								PhotoBuf[y] = 0;
								if (INVALID_HANDLE_VALUE != FindFirstFile(&PhotoBuf[x], &fd))
									Indiv[i].Flags |= 0x100000;

								PhotoBuf[y] = ch;
								for (y = Indiv[i].Name; (y != 0) && (Buf[y] != '@'); y--)
									;
								if (Buf[y] == '@')
								{
									z = y;
									for (y--; (y != 0) && (Buf[y] != '@') && (Buf[y] != '\r'); y--)
										;
									if (Buf[y] == '@')
									{
										for (w = 0; y <= z; w++, y++)
											ThisPhotoBuf[w] = Buf[y];
										ThisPhotoBuf[w] = 0;
										for (z = PhotoFileSize, v = PhotoFileSize+w-2; z >= x; v--, z--)//2 for the "<>"
											PhotoBuf[v] = PhotoBuf[z];
										for (w = 0; ThisPhotoBuf[w] != 0; w++, x++)
											PhotoBuf[x] = ThisPhotoBuf[w];
										PhotoFileSize += (w-2);
									}
								}

								break;
							}
						}
					}
				}
			}
			if (firstphoto)
			{
				firstphoto = FALSE;
				utf = (BYTE*)malloc(PhotoFileSize);
				for (x = 0, y = 0; y < PhotoFileSize; x++)//was x < PhotoFileSize
				{
					if (PhotoBuf[x] < 0x80)
						utf[y++] = (BYTE)PhotoBuf[x];
					else if (PhotoBuf[x] < 0x800)
					{
						utf[y++] = ((BYTE)(MASK2BYTES | PhotoBuf[x] >> 6));
						utf[y++] = ((BYTE)(MASKBYTE | PhotoBuf[x] & MASKBITS));
					}
					else if(PhotoBuf[x] < 0x10000)
					{
						utf[y++] = ((BYTE)(MASK3BYTES | PhotoBuf[x] >> 12));
						utf[y++] = ((BYTE)(MASKBYTE | PhotoBuf[x] >> 6 & MASKBITS));
						utf[y++] = ((BYTE)(MASKBYTE | PhotoBuf[x] & MASKBITS));
					}
				}
				DeleteFile(SimpleFamilyTreePhotosBak);
				MoveFile(SimpleFamilyTreePhotosTxt, SimpleFamilyTreePhotosBak);
				hTemp = CreateFile(SimpleFamilyTreePhotosTxt, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				WriteFile(hTemp, utf, y, &dwBytesWritten, NULL);
				CloseHandle(hTemp);
				free(utf);

				for (y = PhotoFileSize; y != 0; y--)
					if (PhotoBuf[y] == '\\')
						break;
				for (z = 0; x < y; x++, z++)
					PhotoDirectory[z] = PhotoBuf[x];
				PhotoDirectory[z] = 0;
			}

			free(PhotoBuf);
		}
		if (hPhotoFile)
			CloseHandle(hPhotoFile);
		///////
		return;//done!
		///////
	}
//if no .txt photo file
	photowritten = FALSE;
	hPhotoFile2 = CreateFile(SimpleFamilyTreePhotosTxt, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
	hPhotoFile = CreateFile(SimpleFamilyTreePhotos, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
	if (hPhotoFile != INVALID_HANDLE_VALUE)
	{
		if (PhotoFileSize = GetFileSize(hPhotoFile, NULL))
		{
			tempBuf = (char*)malloc(PhotoFileSize);
			ReadFile(hPhotoFile, tempBuf, PhotoFileSize, &dwBytesRead, NULL);
			for (x2 = 0; x2 < PhotoFileSize; x2++)
			{
				if (tempBuf[x2] == '\n')
				{
					x = x2+1;//to beginning of next line
					Num = *(DWORD*)&tempBuf[x];
					for (i = 1; i < LastIndiv; i++)
					{
						if (Indiv[i].Flags & 0x100000)
							continue;
						if (Indiv[i].Num == Num)
						{//found indiv in SimpleFamilyTreePhotos.dta
							x += 4;//past Num
							for (y = x; (y < PhotoFileSize) && (tempBuf[y] != '\r'); y++)
								;
							if (y != PhotoFileSize)
							{
								tempBuf[y] = 0;
								bJpeg = &tempBuf[x];
								x = y + 2;//next line
								if (INVALID_HANDLE_VALUE != FindFirstFileA(bJpeg, &afd))
								{//if found
									Indiv[i].Flags |= 0x100000;
									z = 0;
									bThisPhotoBuf[z++] = '\r';
									bThisPhotoBuf[z++] = '\n';
									for (y = Indiv[i].Name; (y < fileSize) && (Buf[y] != '\r') && (Buf[y] != '\n'); z++, y++)
										bThisPhotoBuf[z] = (char)Buf[y];
									bThisPhotoBuf[z++] = ' ';
									bThisPhotoBuf[z++] = '<';
									bThisPhotoBuf[z++] = '>';
									bThisPhotoBuf[z++] = ' ';
									for (y = 0; bJpeg[y] != 0; z++, y++)
										bThisPhotoBuf[z] = bJpeg[y];
									WriteFile(hPhotoFile2, bThisPhotoBuf, z, &dwBytesWritten, NULL);
									photowritten = TRUE;
									for ( ; y != 0; y--)
										if (bJpeg[y] == '\\')
											break;
									bJpeg[y] = 0;
									for (y = 0; bJpeg[y] != 0; y++)
										PhotoDirectory[y] = bJpeg[y];//needs to be here, although only the last one is useful
									PhotoDirectory[y] = 0;
								}
							}
							break;							
						}
					}
				}
			}
			WriteFile(hPhotoFile2, "\r\n", 2, &dwBytesWritten, NULL);
			free(tempBuf);
		}
	}
	CloseHandle(hPhotoFile2);
	if (!photowritten)
		DeleteFile(SimpleFamilyTreePhotosTxt);
	CloseHandle(hPhotoFile);
}

void SortSiblings(void)
{
	int xBirth, yBirth;

	for (x = 0; x < i; x++)
	{
		if ((Indiv[x].Childof) && (Indiv[x].Birth))
		{
			z = Indiv[x].Birth;
			for ( ; (Buf[z] != '\r') && (Buf[z] != '\n'); z++)
				;
			if ((Buf[z-1] >= '0') && (Buf[z-1] <= '9')
			 && (Buf[z-2] >= '0') && (Buf[z-2] <= '9')
			 && (Buf[z-3] >= '0') && (Buf[z-3] <= '9')
			 && (Buf[z-4] >= '0') && (Buf[z-4] <= '9'))
			{
				xBirth = (((Buf[z-4] - '0') * 1000) + ((Buf[z-3] - '0') * 100) + ((Buf[z-2] - '0') * 10) + (Buf[z-1] - '0'));
				for (y = x+1; y < i; y++)
				{
					if ((Indiv[y].Childof) && (Indiv[y].Childof == Indiv[x].Childof) && (Indiv[y].Birth))
					{
						z = Indiv[y].Birth;
						for ( ; (Buf[z] != '\r') && (Buf[z] != '\n'); z++)
							;
						if ((Buf[z-1] >= '0') && (Buf[z-1] <= '9')
						 && (Buf[z-2] >= '0') && (Buf[z-2] <= '9')
						 && (Buf[z-3] >= '0') && (Buf[z-3] <= '9')
						 && (Buf[z-4] >= '0') && (Buf[z-4] <= '9'))
						{
							yBirth = (((Buf[z-4] - '0') * 1000) + ((Buf[z-3] - '0') * 100) + ((Buf[z-2] - '0') * 10) + (Buf[z-1] - '0'));
							if (yBirth < xBirth)
							{//sort siblings by age
								tempIndiv = Indiv[y];
								for (z = y; z > x; z--)
									Indiv[z] = Indiv[z-1];
								Indiv[x] = tempIndiv;
								x--;//necessary
								break;//out of for (y = x+1; y < i; y++)
							}
						}
					}
				}
			}
		}
	}
	if (IndivNum == highlighted)
	{
		for (x = 0; x < i; x++)
		{
			if (Indiv[x].Num == IndivNumber)
			{
				IndivNum = x;
				highlighted = x;
				if (bs < 511)
				{
					bs++;
					if (Backspace[bs] != -1)
						for ( ; (bs < 511) && (Backspace[bs] != -1); bs++)
							;
				}
				Backspace[bs] = x;
			}
		}
	}
}

/*
void MergeFile(void)
{
	DWORD x = 1, x2 = 1, x3;
	DWORD y, y2, yCorrect = 0, y2Correct = 0, y3;

	while ((x < fileSize) && (x2 < fileSize2))
	{
		for ( ; x < fileSize; x++)
		{
			if ((Buf[x-1] == '\n') && (Buf[x] == '0') && (Buf[x+2] == '@'))
			{
				y = x;
				break;
			}
		}
		for ( ; x2 < fileSize2; x2++)
		{
			if ((Buf2[x2-1] == '\n') && (Buf2[x2] == '0') && (Buf2[x2+2] == '@'))
			{
				y2 = x2;
				break;
			}
		}
		for ( ; ((x < fileSize) || (x2 < fileSize2)) && (Buf[x] == Buf2[x2]); x++, x2++)
		{
			if ((Buf[x-1] == '\n') && (Buf[x] == '0') && (Buf[x+2] == '@'))
				y = x;
			if ((Buf2[x2-1] == '\n') && (Buf2[x2] == '0') && (Buf2[x2+2] == '@'))
				y2 = x2;
		}
		if (Buf[x] != Buf2[x2])
		{
			for (x3 = 0, y3 = y; x3 < fileSize; x3++)
				if ((Buf[x3] == Buf[y3])
				 && (Buf[x3-1] == Buf[y3-1])
				 && (Buf[x3] == Buf[y3])
				 && (Buf[x3+2] == Buf[y3+2])
				 && (Buf[x3+3] == Buf[y3+3])
				 && (Buf[x3+4] == Buf[y3+4])
				 && (Buf[x3+5] == Buf[y3+5])
				 && (Buf[x3+6] == Buf[y3+6]))
				{
					x=x;
				}				   
		}
	}
}
*/

void CheckFAMS(void)
{
	DWORD IndivFam, f = 0;
	DWORD fam, famend = 0;
	DWORD Fam[5000];//presumed largest number of families
	DWORD indiv;

	for (x = 1; x < LastIndiv; x++)
	{
		for (y = 0; (y < MAXSPOUSES) && (Indiv[x].Spouseof[y]) && (f < 5000); y++)
		{
			IndivFam = Indiv[x].Spouseof[y];
			gotit = FALSE;
			for (z = Fams-1; z < FamsEnd; z++)
			{
				Fam[f] = 0;
				if ((Buf[z] == '\n') && (Buf[z+1] == '0') && (Buf[z+3] == '@') && (Buf[z+4] == 'F'))
				{//at "0 @F"
					for (z += 5; Buf[z] != '@'; z++)
						Fam[f] = (Fam[f] * 10) + (Buf[z] - '0');
					if (IndivFam == Fam[f])
					{
						if (Indiv[x].Sex == 1) { // Male
							for ( ; z < fileSize; z++) { // "HUSB" and "0 @F"
								if ((Buf[z] == 'H') && (Buf[z+1] == 'U') && (Buf[z+2] == 'S') && (Buf[z+3] == 'B')) {
									for (z += 7, indiv = 0; Buf[z]!= '@'; z++)
										indiv = (indiv * 10) + (Buf[z] - '0');
									if (indiv != Indiv[x].Num)
										x=x;
									break;
								}
								else if ((Buf[z] == '\n') && (Buf[z+1] == '0') && (Buf[z+3] == '@') && (Buf[z+4] == 'F'))
									break; // no HUSB
							}
						}
						else if (Indiv[x].Sex == 0xFFFF) { // FEMALE
							for ( ; z < fileSize; z++) { // "HUSB" and "0 @F"
								if ((Buf[z] == 'W') && (Buf[z+1] == 'I') && (Buf[z+2] == 'F') && (Buf[z+3] == 'E')) {
									for (z += 7, indiv = 0; Buf[z]!= '@'; z++)
										indiv = (indiv * 10) + (Buf[z] - '0');
									if (indiv != Indiv[x].Num)
										x=x;
									break;
								}
								else if ((Buf[z] == '\n') && (Buf[z+1] == '0') && (Buf[z+3] == '@') && (Buf[z+4] == 'F'))
									break; // no WIFE
							}
						}
						if (famend < f)
							famend = f;
						f++;
						gotit = TRUE;
						break;
					}
				}
			}
			if (gotit == FALSE)
			{
				_snwprintf(Error, 512, TEXT("the line 1 FAMS @F%i@"), IndivFam);
				MessageBox(hwnd, TEXT("isn't in the Family part of the the Gedcom data file.\nYou should delete that line in Notepad."), Error, MB_OK);
			}
		}
	}
	gotit = FALSE;
	for (z = 0; z < FamsEnd; z++)
	{
		if ((Buf[z] == '\n') && (Buf[z+1] == '0') && (Buf[z+3] == '@') && (Buf[z+4] == 'F'))
		{//at "0 @F"
			for (fam = 0, z += 5; Buf[z] != '@'; z++)
				fam = (fam * 10) + (Buf[z] - '0');
			gotit = FALSE;
			for (f = 0; f <= famend; f++)
			{
				if (fam == Fam[f])
				{
					gotit = TRUE;
					break;
				}
			}
			if (gotit == FALSE)
			{
				_snwprintf(Error, 512, TEXT("the line 0 FAMS @F%i@ FAM"), fam);
				MessageBox(hwnd, TEXT("isn't in the Individuals part of the Gedcom data file.\nYou should delete that line in Notepad\n(along with any lines between it and the next 0 @Fxxxx@ FAM line)."), Error, MB_OK);
			}
		}
	}
}


int CheckRelative(DWORD saveHighlighted, DWORD savei)
{
	if (IndivNum != highlighted)
	{
		highlighted = IndivNum;
		InitializeAgain();
	}
	if (Indiv[savei].Flags & 0x1000000)
	{
		Indiv[IndivNum].Flags &= 0x0FFFFFF;
		highlighted = savei;
		InitializeAgain();
		if (Indiv[IndivNum].Flags & 0x1000000)
		{
			highlighted = saveHighlighted;
			InitializeAgain();
			return FALSE;
		}
	}
	highlighted = saveHighlighted;
	InitializeAgain();
	i = savei;
	return TRUE;
}

void CursorMoved(int xPos, int yPos)
{
	if (arrayfilled)
	{
		inbox = FALSE;
		for (yBig = RowsUp; (yBig < ROWS) && (Array[yBig][0] != 0xFFFF); yBig++)
		{
			for (xBig = 0; Array[yBig][xBig] != 0xFFFF; xBig++)
			{
				BoxLeft = Indiv[Array[yBig][xBig]].X - 5;//10
				BoxRight = Indiv[Array[yBig][xBig]].X + Indiv[Array[yBig][xBig]].cx + 5;//10
				BoxTop = ((yBig-RowsUp+1)*YSpacing) - 5;
				BoxBottom = ((yBig-RowsUp+1)*YSpacing) + Z + 5;
				if ((xPos > (BoxLeft-xLoc)) && (xPos < (BoxRight-xLoc)) && (yPos > (BoxTop-yLoc)) && (yPos < (BoxBottom-yLoc)))
				{//in a box
					SetCursor(hCursor);
					inbox = TRUE;
					if (gotmouseover == FALSE)
					{
						gotmouseover = TRUE;
						MouseLoc = Array[yBig][xBig];
						if ((mouseover == BIRTHDAY) && (Indiv[Array[yBig][xBig]].Birth))
						{
							Birth = &Buf[Indiv[MouseLoc].Birth];
							for (y = 0; (Birth[y] != '\r') && (Birth[y] != '\n'); y++)
							{
								if ((Birth[y] >= '0') && (Birth[y] <= '9') && (Birth[y+1] >= '0') && (Birth[y+1] <= '9') && (Birth[y+2] >= '0') && (Birth[y+2] <= '9') && (Birth[y+3] >= '0') && (Birth[y+3] <= '9'))
								{
									if (y != 0)
									{
										ch = Birth[y+4];
										Birth[y+4] = 0;
										hwndBirthday = CreateWindow(TEXT("STATIC"), NULL,
											WS_CHILD | WS_VISIBLE | WS_BORDER,
											(BoxLeft-xLoc), (BoxTop-yLoc), BoxRight-BoxLeft, BoxBottom-BoxTop,
											hwnd, (HMENU)188, hInst, NULL);
										SendMessage(hwndBirthday, WM_SETFONT, (WPARAM)hFont, TRUE);
										SetWindowText(hwndBirthday, Birth);
										Birth[y+4] = ch;
									}
									break;
								}
							}
						}
						else if ((mouseover == AGE) && (Indiv[MouseLoc].Birth))
						{
							Birth = &Buf[Indiv[MouseLoc].Birth];
							gotbirthdate = FALSE;
							for (x = y = 0; (Birth[y] != '\r') && (Birth[y] != '\n'); y++)
							{
								if ((gotbirthdate == FALSE) && (Birth[y] >= '0') && (Birth[y] <= '9') && ((Birth[y+1] < '0') || (Birth[y+1] > '9') || (Birth[y+2] < '0') || (Birth[y+3] > '9')|| (Birth[y+3] < '0')))
								{
									int x, z;

									Birthdate = 0;
									for (x = y; (Birth[x] >= '0') && (Birth[x] <= '9'); x++)
									{
										z = Birth[x] - '0';
										Birthdate *= 10;
										Birthdate += z;
									}
									gotbirthdate = TRUE;
								}
								if ((Birth[y] >= '0') && (Birth[y] <= '9') && (Birth[y+1] >= '0') && (Birth[y+1] <= '9') && (Birth[y+2] >= '0') && (Birth[y+2] <= '9') && (Birth[y+3] >= '0') && (Birth[y+3] <= '9'))
								{
									if (y != x)
									{
										Age = 120;//flag
										Year = Atoi(&Birth[y]);
										d = 0;//necessary because Export Birthdays uses parseBirthday & increments d
										date[d] = 0;
										gotdate = gotmonth = FALSE;
										ParseBirthday();
										if ((date[d]) || (gotmonth))
										{//got month, day, and year
											if (Indiv[MouseLoc].Death)
											{
												Death = &Buf[Indiv[MouseLoc].Death];
												for (y = 0; (Death[y] != '\r') && (Death[y] != '\n'); y++)
												{
													if ((Death[y] >= '0') && (Death[y] <= '9') && (Death[y+1] >= '0') && (Death[y+1] <= '9') && (Death[y+2] >= '0') && (Death[y+2] <= '9') && (Death[y+3] >= '0') && (Death[y+3] <= '9'))
													{
														if (y != 0)
														{
															Year2 = Atoi(&Death[y]);
															d = 1;//necessary because Export Birthdays uses parseBirthday & increments d
															date[d] = 0;
															gotdate = gotmonth = FALSE;
															ParseBirthday2();
															if (date[d])
															{
																Age = Year2 - Year;
																if (date[0] > date[1])
																	Age--;
															}
														}
													}
												}
											}
											else
											{
												GetSystemTime(&st);
												Age = st.wYear - Year;
												day = months[st.wMonth] + st.wDay;
												if (day < (date[0] + Birthdate))
													Age--;
											}
											if (Age < 120)
											{
												hwndAge = CreateWindow(TEXT("STATIC"), NULL,
													WS_CHILD | WS_VISIBLE | WS_BORDER,
													(BoxRight-xLoc-20), (BoxBottom-yLoc-20), 20, 20,
													hwnd, (HMENU)189, hInst, NULL);
												_itow(Age, age, 10);
												SetWindowText(hwndAge, age);
											}
										}
									}
									break;
								}
							}
						}
					}
				}
			}
		}
		if (inbox == FALSE)
		{
			SetCursor(hDrawingCursor2);
			if (gotmouseover)
			{
				gotmouseover = FALSE;
				if (hwndBirthday)
				{
					DestroyWindow(hwndBirthday);
					hwndBirthday = NULL;
				}
				if (hwndAge)
				{
					DestroyWindow(hwndAge);
					hwndAge = NULL;
				}
			}
		}
	}
}

DWORD Atoi(TCHAR*ptr)
{
	DWORD x;

	for (x = 0;(*ptr >= '0') && (*ptr <= '9'); ptr++)
	{
		x *= 10;
		x += *ptr - '0';
	}
	return x;
}

void RemovePhoto(DWORD x)
{
	DWORD y;
		
	if (IDOK == MessageBox(hwnd, TEXT("Remove this photo link?"), TEXT(""), MB_OKCANCEL|MB_DEFBUTTON2))
	{
		Indiv[IndivNum].Flags = Indiv[IndivNum].Flags & 0xFF0FFFFF;
		for (y = x; (y < PhotoFileSize) && (PhotoBuf[y] != '\r'); y++)
			;
		y++;//to '\n'
		if (PhotoBuf[y] == '\n')
			y++;
		else
			MessageBox(hwnd, TEXT("Carriage Return/Line Feed not used!"), TEXT(""), MB_OK);
		for ( ; x < PhotoFileSize; x++, y++)
			PhotoBuf[x] = PhotoBuf[y];
		if (PhotoFileSize != (y-x))
		{//remove photo link
			Indiv[IndivNum].Flags &= 0xFF0FFFFF;//clear that bit
			PhotoFileSize -= (y-x);
			utf = (BYTE*)VirtualAlloc(NULL, 2*PhotoFileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
			for (x = 0, y = 0; x < PhotoFileSize; x++)
			{
				if (PhotoBuf[x] < 0x80)
					utf[y++] = (BYTE)PhotoBuf[x];
				else if (PhotoBuf[x] < 0x800)
				{
					utf[y++] = ((BYTE)(MASK2BYTES | PhotoBuf[x] >> 6));
					utf[y++] = ((BYTE)(MASKBYTE | PhotoBuf[x] & MASKBITS));
				}
				else if(PhotoBuf[x] < 0x10000)
				{
					utf[y++] = ((BYTE)(MASK3BYTES | PhotoBuf[x] >> 12));
					utf[y++] = ((BYTE)(MASKBYTE | PhotoBuf[x] >> 6 & MASKBITS));
					utf[y++] = ((BYTE)(MASKBYTE | PhotoBuf[x] & MASKBITS));
				}
			}
			SetFilePointer(hPhotoFile, 0, NULL, FILE_BEGIN);
			WriteFile(hPhotoFile, utf, y, &dwBytesWritten, NULL);
			VirtualFree(utf, 0, MEM_RELEASE);
		}
		else // it would be empty
			DeleteFile(SimpleFamilyTreePhotosTxt);
	}
}
/*
void FixGed(void)
{
	DWORD x;

	for (BufLines = 0, x = 0; x < badY; x++)
		if (Buf[x] == '\n')
			BufLines++;
	if (DialogBox(hInst, TEXT("GEDFILE"), hwnd, FixGedProc))
	{
		ReStart();
		errorinbuf = FALSE;
	}
}
*/