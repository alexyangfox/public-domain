// uses ChessAlpha2.ttf fonts and Check.wav and CheckMate.wav
#include <windows.h>
#include <mmsystem.h> // add winmm.lib to Project -Settings -Link (to play "Check.wav")
#include <stdio.h>//for sprintf
#include "resource.h"

#define WhiteBishop 192
#define WhiteKnight 194
#define WhiteRook 196
#define WhiteQueen 198
#define WhiteKing 200
#define WhitePawn 202
#define BlackBishop 224
#define BlackKnight 226
#define BlackRook 228
#define BlackQueen 230
#define BlackKing 232
#define BlackPawn 234

#define DiscoveredCheck 1
#define Check 2
#define CheckMate 3
#define PIECEMOVES 512
#define PGNSIZE 4096
#define PGN2SIZE 100
#define PGNENTRIES 22000
#define PGNBUFSIZE 10000
#define WHITE 1
#define BLACK 0

int x, y, n, i, j, p, SquareSize, SquareMiddle, BoardSize, LeftSide, Border, Bottom, xPos, yPos, X, Y, X2, Y2, ptr, ptr2;
int MenuHeight, BlackKingX, BlackKingY, WhiteKingX, WhiteKingY, SaveX, SaveY, SaveX2, SaveY2, x2, y2;
int checktype, PossibleRookLoc, OriginX, pm, pmMax, pmHome, Border, SavedPieceLoc, Lines, pmLoadedGame, BufSize, Dot;
int slowX, slowY, slowX2, slowY2, slowPiece, destX = -1, destY = -1, fromX, fromY, diffX, diffY, OldPiece;
int Entry[PGNENTRIES], e = 1, nPGN = 1, Games, bigNum, littleNum, fook;

DWORD dwBytesRead, dwBytesWritten, fileSize, RealfileSize, subchunksize, nextchunk, nSamplesPerSec;
WORD nChannels, wBitsPerSample;
BYTE PieceLoc[8][8];
BYTE TempPieceLoc[8][8];
BYTE Placed[8][8];
BYTE BlackNums[8][8];
BYTE WhiteNums[8][8];
BYTE blackpawns, blackrooks, blackknights, blackbishops, blackqueen, whitepawns, whiterooks, whiteknights, whitebishops, whitequeen;
BYTE MovedPiece, ReplacedPiece, SaveMovedPiece, SaveReplacedPiece, Piece;
BYTE number, piece, color;
BYTE *WaveBuf, *Buf;
BYTE *PGNfile;

struct {
	BYTE Piece;
	BYTE PieceTaken;
	BYTE fromX;
	BYTE fromY;
	BYTE toX;
	BYTE toY;
	BYTE Note1;
} PieceMoves[PIECEMOVES];

struct {
	char Number[4];
	char WhitePiece[8];
	char BlackPiece[8];
} PGN2[PGN2SIZE];

char PGN[PGNSIZE]; // to read file
char Pgn[PGNSIZE]; // to write file
char pgn[18];
BYTE Chessmen[256]; // sparse array to hold piece font numbers
char szAppName[] = "   SimpleChess";
char WhiteMoves[] = "   WHITE'S MOVE";
char BlackMoves[] = "   BLACK'S MOVE";
char letter[1];
char num[1];
char PromotedType;
char ChessAlpha2[] = "Chess Alpha 2";
char Filename[MAX_PATH];
char FullFilename[MAX_PATH];
char PGNbuf[PGNBUFSIZE];
char PGNtitle[256];
char N[4], TotalGames[6];
char ch[1];
char pawns[256];

char Help[] = "\
Move a piece by pointing to it\n\
and holding the left mouse button down\n\
while moving the pointer.\n\
\n\
Use the left and right arrow keys to go backward and forward through the game.\n\
Hold the Ctrl key down while pressing the right arrow key to go forward at full speed.\n\
If you play a piece after going backward in a game, that becomes the last piece played.\n\
\n\
Press the Home or End key to go to the beginning or end of a game.\n\
You can also select Options -Make Current Position Home at any point in the game.\n\
\n\
Press the right mouse button to toggle numbers of first-level attackers/defenders.\n\
\n\
Press F1 to see relative piece count.\n\
\n\
See Rules of Chess on Wikipedia.";

char About[] = "by Doug Cox\n16 Mar 2011\njdmcox@jdmcox.com\nUpdates at jdmcox.com\n\nUses Check.wav,\nCheckMate.wav,\nand ChessAlpha2.ttf\n";

BOOL (*pCheckMoves)(void); // pointer to functions
BOOL WhitesMove, movingPiece = FALSE, choosePiece = FALSE, fromcheckcheck, showPGN, replacedPiece, fromblackandwhite = FALSE;
BOOL castled = FALSE, PieceColor, promoted = FALSE, returned, rotated, placepieces, nosave, gotbracket, exitflag = FALSE;
BOOL fromcastling = FALSE, fromright = FALSE, fromhome = FALSE, fromend = FALSE, showing = FALSE, showingnums = FALSE;

HWND hwnd, hwndPlacePieces = NULL, hwndPGN, hwndEdit2;
HINSTANCE hInst;
HANDLE hFile;
HMENU hMenu;
HFONT hFont;
LOGFONT lf;
OPENFILENAME ofn;
HBRUSH hBeigeBrush, hBrownBrush, hBorderBrush, hGreyBrush;
HPEN hBeigePen, hBrownPen, hBorderPen, hGreyPen;
HBRUSH hBlack, hDarkGrey, hLightGrey, hWhite;
HGDIOBJ hBrushObject, hPenObject, hOldFont;
RECT rect;
HBITMAP hMemBitmap, hOldBitmap;
HDC hdc, hdcMem = 0;
PAINTSTRUCT ps;
HWAVEOUT hWaveOut;
WAVEHDR WaveOutHdr;
WAVEFORMATEX WaveFormat;

void Setup(void);
int CheckKing(int, int, int);
BOOL CheckRookMoves(void);
BOOL CheckKnightMoves(void);
BOOL CheckMove(void);
BOOL CheckForCheck(void);
BOOL CheckForMate(int, int, int);
void AddFromLoc(void);
void PGN2toPgn(void);
void PlayCheck(void);
void GetPieceMoves(BOOL);
void GetBlackAndWhiteNums(void);
void CALLBACK waveOutProc(HWAVEOUT, UINT, DWORD, DWORD, DWORD);
int CALLBACK PromotionProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK PGNfilesProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK PlacePiecesProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
WNDPROC pEdit2Proc;

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
	wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE ("PAWN"));
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(WHITE_BRUSH);
	wndclass.lpszMenuName  = "MENU";
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	hwnd = CreateWindow(szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

//	ShowWindow(hwnd, SW_SHOWNORMAL);
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
LRESULT CALLBACK Edit2Proc(HWND hwnd2, UINT message, WPARAM wParam, LPARAM lParam)
{
	if ((message == WM_KEYDOWN) && (wParam == VK_RETURN)) // IDC_EDIT2 has to be multiline and "Want return"
		exitflag = TRUE;
	return CallWindowProc(pEdit2Proc, hwnd2, message, wParam, lParam);
}

void CALLBACK waveOutProc(HWAVEOUT hWaveOut, UINT message, DWORD dwInstance, DWORD wParam, DWORD lParam)
{ // to close playing of Check.wav
	if (message == WOM_DONE)
		PostMessage(hwnd, WM_USER, 0, 0);
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_USER: // from waveOutProc
		waveOutUnprepareHeader(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
		waveOutReset(hWaveOut);
		waveOutClose(hWaveOut);
		if (WaveBuf)
			VirtualFree(WaveBuf, 0, MEM_RELEASE);
		WaveBuf = 0;
		return 0;

	case WM_CREATE:
		WaveBuf = 0;
		AddFontResource("ChessAlpha2.ttf");
		Setup();
		Filename[0] = 0;
		showPGN = FALSE;
		hMenu = GetMenu(hwnd);
		if (showPGN) // I change its initialization a lot
			CheckMenuItem(hMenu, ID_SHOWPGN, MF_CHECKED);
		else
			CheckMenuItem(hMenu, ID_SHOWPGN, MF_UNCHECKED);

		MenuHeight = GetSystemMetrics(SM_CYMENU);
		for (x = 0; x < 256; x++)
			Chessmen[x] = x; // a sparse array for piece font numbers
//		hGreyBrush = CreateSolidBrush(0xA0B0B0);
		hBeigeBrush = CreateSolidBrush(0x65838A);
		hBeigePen = CreatePen(PS_SOLID, 1, 0x65838A);
		hBrownBrush = CreateSolidBrush(0x305472);
		hBrownPen = CreatePen(PS_SOLID, 1, 0x305472);
		hBorderBrush = CreateSolidBrush(0x2D4868);
		hBorderPen = CreatePen(PS_SOLID, 1, 0x2D4868);

		hGreyBrush = CreateSolidBrush(0x888888);
		hGreyPen = CreatePen(PS_SOLID, 1, 0x888888);
		hBlack = CreateSolidBrush(0x303030);
		hDarkGrey = CreateSolidBrush(0x585858);
		hLightGrey = CreateSolidBrush(0xB8B8B8);
		hWhite = CreateSolidBrush(0xD0D0D0);

		ofn.lStructSize       = sizeof(OPENFILENAME);
		ofn.lpstrFilter       = TEXT(" *.pgn\0*.pgn\0\0");
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
		ofn.lpstrInitialDir   = NULL;
		ofn.nFileOffset       = 0;
		ofn.nFileExtension    = 0;
		ofn.lCustData         = 0;
		ofn.lpfnHook          = NULL;
		ofn.lpTemplateName    = NULL;
		return 0;

	case WM_SIZE:
		GetClientRect(hwnd, &rect);
		Border = 16;
		Bottom = rect.bottom - MenuHeight;
		Lines = rect.bottom / 20;
		if (HIWORD(lParam) < LOWORD(lParam)) {
			SquareSize = (HIWORD(lParam) - (Border*2)) / 8; // height
			BoardSize = SquareSize * 8;
			LeftSide = (LOWORD(lParam)/2) - ((SquareSize*8)/2);
		}
		else {
			SquareSize = (LOWORD(lParam) - (Border*2)) / 8; // width
			BoardSize = SquareSize * 8;
			LeftSide = (LOWORD(lParam)/2) - ((SquareSize*8)/2);
		}
		lf.lfHeight = -((SquareSize*3/4) * 96 / 72); // presumes 96 = dots/inch of monitor
		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 3;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 0x22;
		for (x = 0; ChessAlpha2[x] != 0; x++)
			lf.lfFaceName[x] = ChessAlpha2[x];
		lf.lfFaceName[x] = 0;
		hFont = CreateFontIndirect(&lf); // chess pieces
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_FILE_NEWGAME:
			Setup();
			Filename[0] = 0;
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case FILE_SAVE:
			if (Filename[0]) {
				PGN2toPgn();
				if (INVALID_HANDLE_VALUE != (hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL))) {
					WriteFile(hFile, Pgn, p, &dwBytesWritten, NULL);
					CloseHandle(hFile);
				}
				else
					MessageBox(hwnd, "Can't save it.", ERROR, MB_OK);
				break;
			} // fall thru...

		case ID_FILE_SAVEGAME:
			if (!nosave) {
				PGN2toPgn();
				if (GetSaveFileName(&ofn)) {
					if (INVALID_HANDLE_VALUE != (hFile = CreateFile(FullFilename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL))) {
						WriteFile(hFile, Pgn, p, &dwBytesWritten, NULL);
						CloseHandle(hFile);
					}
					else
						MessageBox(hwnd, "Can't save it.", ERROR, MB_OK);
				}
			}
			else
				MessageBox(hwnd, "You can't save a set-up board.", "Sorry", MB_OK);
			break;

		case ID_FILE_LOADGAME:
			if (GetOpenFileName(&ofn)) {
				hFile = CreateFile(FullFilename, GENERIC_READ, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
				if (hFile != INVALID_HANDLE_VALUE) {
					if (fileSize = GetFileSize(hFile, NULL)) {
						RealfileSize = fileSize; // because fileSize is changed below

						Setup(); // initializes PGN and PieceLoc and PieceMoves
						PGNfile = VirtualAlloc(NULL, fileSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
						ReadFile(hFile, PGNfile, fileSize, &dwBytesRead, NULL);
						CloseHandle(hFile);

						gotbracket = FALSE;
						for (ptr = 0; ptr < (int)fileSize; ptr++) { // 32 is first unused space in PieceMoves
							if (PGNfile[ptr] == '[') {
								gotbracket = TRUE;
								break;
							}
						}
						if (gotbracket) {
							for (x = 0; Filename[x] != 0; x++) {
								if (Filename[x] != PGNtitle[x]) { // if a different file loaded
									e = 1;
									nPGN = 1;
									break;
								}
							}
							for (x = 0, Games = 0; x < (int)fileSize; x++) {
								if (*(DWORD*)&PGNfile[x] == 0x6576455B) { // "[Eve"
									Games++;
									Entry[Games] = x;
								}
							}
							for (x = Entry[e], y = 0; (x < (int)fileSize) && (y < (PGNBUFSIZE-256)); x++) {
								PGNbuf[y++] = PGNfile[x];
								if (PGNfile[x] == '[') {
									for (x++; PGNfile[x] != ']'; x++, y++)
										PGNbuf[y] = PGNfile[x];
									PGNbuf[y++] = PGNfile[x];
								}
								if ((PGNfile[x] == '.') && (PGNfile[x+1] != '.')) {
									for (x++; x < (int)fileSize; x++, y++ ) {
										PGNbuf[y] = PGNfile[x];
										if (PGNfile[x] == '[')
											break;
									}
									PGNbuf[y] = 0;

									SaveX = x; // for PGNfilesProc
									if (DialogBox(hInst, "PGNFILES", hwnd, PGNfilesProc)) {
										for (x = 0, y = 0; PGNbuf[y] != 0; x++, y++)
											PGN[x] = PGNbuf[y];
										PGN[x] = 0;
										ptr = 0;
										fileSize = (DWORD)x; // for after "break"
										break;
									}
									else {
										InvalidateRect(hwnd, &rect, FALSE);
										VirtualFree(PGNfile, 0, MEM_RELEASE);
										return 0;
									}
								}
							}
						} // end of if (gotbracket)

						else {
							for (x = 0; x < (int)fileSize; x++)
								PGN[x] = PGNfile[x];
							ptr = 0;
						}

						for ( ; ptr < (int)fileSize; ptr++) {
							if (PGN[ptr] == '[') {
								for ( ; PGN[ptr] != ']'; ptr++)
									;
								ptr++;
							}
							if ((PGN[ptr] == '.') && (PGN[ptr+1] != '.'))
								break; // found first number followed by a dot
						}
						VirtualFree(PGNfile, 0, MEM_RELEASE);

						pm = 32; // pm points to first empty PieceMoves
						number = 0;
						// loop thru each number
						for ( ; ptr < (int)fileSize; ptr++) {
							if (PGN[ptr] == '{') {
								for ( ; PGN[ptr] != '}'; ptr++)
									;
								ptr++;
							}
							else if ((PGN[ptr] == '.') && (PGN[ptr+1] == '.') && (PGN[ptr+2] == '.'))
								ptr += 2;
							else if (PGN[ptr] == '.') { // there must be a '.' after the number
								number++;
								if (number == 99) {
									MessageBox(hwnd, "The max moves is 99!", "", MB_OK);
									DestroyWindow(hwnd);
									return 0;
								}
								for (x = ptr-1; (x != -1) && (PGN[x] != ' ') && (PGN[x] != '\r') && (PGN[x] != '\n'); x--)
									;
								x++;
								for (ptr2 = 0; x <= ptr; x++, ptr2++)
									PGN2[number].Number[ptr2] = PGN[x];
							}
							else if (((PGN[ptr] & 0xDF) >= 'A') && ((PGN[ptr] & 0xDF) <= 'Z')) { // move to piece name or pawn toX
								if (ptr < (int)fileSize) {
									GetPieceMoves(WHITE);
									if (PGN[ptr] == 'O') {
										for ( ; ptr < (int)fileSize; ptr++) {
											if ((PGN[ptr] != 'O') && (PGN[ptr] != '-'))
												break;
										}
									}
									else if (PGN[ptr] == '0')
										MessageBox(hwnd, "Castling is apparently indicated using zeros instead of O's", ERROR, MB_OK);
								}
								for ( ; ptr < (int)fileSize; ptr++) {
									if (PGN[ptr] == '{') {
										for ( ; PGN[ptr] != '}'; ptr++)
											;
									}
									else if (((PGN[ptr] & 0xDF) >= 'A') && ((PGN[ptr] & 0xDF) <= 'Z')) // move to piece name or pawn toX
										break;
									else if ((PGN[ptr] == '.') && (PGN[ptr] == '.') && (PGN[ptr] == '.'))
										ptr += 2;
								}
								if (ptr < (int)fileSize) {
									GetPieceMoves(BLACK);
									for ( ; ptr < (int)fileSize; ptr++) {
										if ((PGN[ptr] != 'O') && (PGN[ptr] != '-'))
											break;
									}
								}
								else {
									WhitesMove = FALSE;
									SetWindowText(hwnd, BlackMoves);
								}
							}
						}
						ptr--;
						pmLoadedGame = pm;
						InvalidateRect(hwnd, &rect, FALSE);
						UpdateWindow(hwnd); // necessary!
					}
					else // if (0 == fileSize)
						CloseHandle(hFile);
				}
			}
			break;

		case ID_FILE_EXIT:
			DestroyWindow(hwnd);
			break;

		case ID_SHOWPGN:
			if (showPGN) {
				showPGN = FALSE;
				CheckMenuItem(hMenu, ID_SHOWPGN, MF_UNCHECKED);
			}
			else {
				showPGN = TRUE;
				CheckMenuItem(hMenu, ID_SHOWPGN, MF_CHECKED);
			}
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd); // necessary here!
			break;

		case ID_SETUPPIECES:
			SetWindowText(hwnd, szAppName);
			nosave = FALSE;
			pmLoadedGame = 0;
			rotated = FALSE;
			placepieces = TRUE;
			Filename[0] = 0;
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++)
					Placed[y][x] = 0;
			}
			Placed[0][4] = 232; // black king
			Placed[7][4] = 200; // white king
			InvalidateRect(hwnd, &rect, FALSE);
			hwndPlacePieces = CreateDialog(hInst, "PLACEPIECES", hwnd, PlacePiecesProc);
			break;

		case ID_LOADPLACEDPIECES:
			hFile = CreateFile("Placed.dta", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
			if (hFile != INVALID_HANDLE_VALUE) {
				if (fileSize = GetFileSize(hFile, NULL)) {
					ReadFile(hFile, Placed, 64, &dwBytesRead, NULL);
					CloseHandle(hFile);

					SetWindowText(hwnd, szAppName);
					nosave = FALSE;
					pmLoadedGame = 0;
					rotated = FALSE;
					placepieces = TRUE;
					Filename[0] = 0;
					InvalidateRect(hwnd, &rect, FALSE);
					hwndPlacePieces = CreateDialog(hInst, "PLACEPIECES", hwnd, PlacePiecesProc);
				}
			}
			break;

		case ID_SHOWATTACKERSDEFENDERS:
			if (showing == FALSE) {
				showing = TRUE;
				CheckMenuItem(hMenu, ID_SHOWATTACKERSDEFENDERS, MF_CHECKED);
			}
			else {
				showing = FALSE;
				CheckMenuItem(hMenu, ID_SHOWATTACKERSDEFENDERS, MF_UNCHECKED);
			}
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case ID_MAKECURRENTPOSITIONHOME:
			pmHome = pm;
			break;

/*
		case ID_ROTATEBOARD:
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					TempPieceLoc[y][x] = PieceLoc[7-y][7-x];
				}
			}
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					PieceLoc[y][x] = TempPieceLoc[y][x];
				}
			}
			for (x = 0; x < pm; x++) {
				PieceMoves[x].fromX = 7 - PieceMoves[x].fromX;
				PieceMoves[x].toX = 7 - PieceMoves[x].toX;
				PieceMoves[x].fromY = 7 - PieceMoves[x].fromY;
				PieceMoves[x].toY = 7 - PieceMoves[x].toY;
			}

			if (rotated)
				rotated = FALSE;
			else
				rotated = TRUE;
			InvalidateRect(hwnd, &rect, FALSE);
			UpdateWindow(hwnd); // necessary here!
			break;
*/
		case HELP:
			MessageBox(hwnd, Help, szAppName, MB_OK);
			break;

		case ID_HELP_ABOUT:
			MessageBox(hwnd, About, szAppName, MB_OK);
			break;
		}
		break;

	case WM_RBUTTONDOWN:
			if (showingnums == FALSE)
				showingnums = TRUE;
			else
				showingnums = FALSE;
			InvalidateRect(hwnd, &rect, FALSE);
		break;

	case WM_LBUTTONDOWN:
		if (!placepieces) {
			if ((LOWORD(lParam) >= LeftSide) && (LOWORD(lParam) <= LeftSide+(SquareSize*8))) {
				xPos = LOWORD(lParam);
				yPos = HIWORD(lParam);
				X = X2 = (xPos-LeftSide) / SquareSize; // for PieceLoc[Y][X]
				Y = Y2 = yPos / SquareSize; // for PieceLoc[Y][X]
				if ((WhitesMove) && (PieceLoc[Y][X] >= 224) && (PieceLoc[Y][X] <= 234))
					return 0;
				else if ((!WhitesMove) && (PieceLoc[Y][X] >= 192) && (PieceLoc[Y][X] <= 202))
					return 0;
				MovedPiece = PieceLoc[Y][X];
	SaveMovedPiece = MovedPiece;
	ReplacedPiece = PieceLoc[Y][X];
	SaveReplacedPiece = ReplacedPiece;
				PossibleRookLoc = X;
				PieceLoc[Y][X] = 0;
				movingPiece = TRUE;
				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		return 0;

	case WM_MOUSEMOVE:
		xPos = LOWORD(lParam);
		yPos = HIWORD(lParam);

		if (movingPiece) {
			if (xPos < LeftSide)
				xPos = LeftSide + (SquareSize/2);
			else if (xPos > (LeftSide + (SquareSize*8)))
				xPos = LeftSide + (SquareSize*7) + (SquareSize/2);
			else if (yPos < SquareSize)
				yPos = Border + (SquareSize/2);
			else if (yPos > Border + (SquareSize*8))
				yPos = Border + (SquareSize*8);
			InvalidateRect(hwnd, &rect, FALSE);
		}
		return 0;

	case WM_LBUTTONUP:
		if (movingPiece) {
			X2 = (xPos-LeftSide) / SquareSize; // for PieceLoc[Y][X]
			Y2 = yPos / SquareSize; // for PieceLoc[Y][X]
			ReplacedPiece = PieceLoc[Y2][X2];
			SaveReplacedPiece = ReplacedPiece;
			SaveMovedPiece = MovedPiece;
			if (ReplacedPiece)
				replacedPiece = TRUE; // for 'x' below
			else
				replacedPiece = FALSE;
			PieceLoc[Y2][X2] = MovedPiece;
			PieceLoc[Y][X] = 0;
			fromcheckcheck = FALSE;
			if ((FALSE == CheckMove())) {
				PieceLoc[Y][X] = MovedPiece;
				if ((X2 != X) || (Y2 != Y))
					PieceLoc[Y2][X2] = SaveReplacedPiece;
			}
			else { // possible good move
				checktype = CheckForCheck();
				if (DiscoveredCheck == checktype) {
					if ((X2 != X) || (Y2 != Y)) {
						PieceLoc[Y][X] = SaveMovedPiece;
						PieceLoc[Y2][X2] = SaveReplacedPiece;
						goto skip;
					}
				}
				if ((PGN2[number+1].WhitePiece[0]) || (PGN2[number+1].BlackPiece[0])) {
					for (x = number+1; PGN2[x].Number[0] != 0; x++) {
						for (y = 0; y < 4; y++)
							PGN2[x].Number[y] = 0;
						for (y = 0; y < 8; y++)
							PGN2[x].WhitePiece[y] = 0;
						for (y = 0; y < 8; y++)
							PGN2[x].BlackPiece[y] = 0;
					}
				}
				if ((WhitesMove) || (number == 0)) {
					PGN[ptr++] = '\r';
					PGN[ptr++] = '\n';
					number++;
					if (number == 99)
						MessageBox(hwnd, "The max moves is 99!\n\nDon't make any more moves!", "", MB_OK);
					else if (number > 99) {
						DestroyWindow(hwnd);
						return 0;
					}
					ptr2 = 0;
					if (number < 10) {
						PGN[ptr++] = number + '0';
						PGN2[number].Number[ptr2++] = number + '0';
					}
					else if (number < 100) {
						PGN[ptr++] = (number / 10) + '0';
						PGN[ptr++] = (number % 10) + '0';
						PGN2[number].Number[ptr2++] = (number / 10) + '0';
						PGN2[number].Number[ptr2++] = (number % 10) + '0';
					}
					else if (number < 1000) {
						PGN[ptr++] = (number / 100) + '0';
						PGN[ptr++] = ((number % 100) / 10) + '0';
						PGN[ptr++] = (number % 10) + '0';
						PGN2[number].Number[ptr2++] = (number / 100) + '0';
						PGN2[number].Number[ptr2++] = ((number % 100) / 10) + '0';
						PGN2[number].Number[ptr2++] = (number % 10) + '0';
					}
					PGN[ptr++] = '.';
					PGN2[number].Number[ptr2++] = '.';
				}
				if ((number == 1) && (!WhitesMove) && (PGN2[1].WhitePiece[0] == 0)) { // OptionS -Place Pieces, Black moves first
					PGN[ptr++] = '.';
					PGN[ptr++] = '.';
					PGN2[number].Number[ptr2++] = '.';
					PGN2[number].Number[ptr2++] = '.';
				}
				PGN[ptr++] = ' ';

				ptr2 = 0;
				OriginX = 0xFF; // flag
				switch (PieceLoc[Y2][X2])
				{
				case BlackPawn:
				case WhitePawn:
					OriginX = X;
					break;
				case BlackRook:
					PGN[ptr++] = 'R';
					PGN2[number].BlackPiece[ptr2++] = 'R';
					for (y = 0; y < 8; y++) {
						for (x = 0; x < 8; x++) {
							if ((y == Y) && (x == X))
								continue; // don't count moved piece
							if ((PieceLoc[y][x] == BlackRook) && ((y != Y2) || (x != X2))) {
								pCheckMoves = CheckRookMoves;
								pCheckMoves();
								AddFromLoc();
								break;
							}
						}
					}
					break;
				case WhiteRook:
					PGN[ptr++] = 'R';
					PGN2[number].WhitePiece[ptr2++] = 'R';
					for (y = 0; y < 8; y++) {
						for (x = 0; x < 8; x++) {
							if ((y == Y) && (x == X))
								continue; // don't count moved piece
							if ((PieceLoc[y][x] == WhiteRook) && ((y != Y2) || (x != X2))) {
								pCheckMoves = CheckRookMoves;
								pCheckMoves();
								AddFromLoc();
								break;
							}
						}
					}
					break;
				case BlackKnight:
					PGN[ptr++] = 'N';
					PGN2[number].BlackPiece[ptr2++] = 'N';
					for (y = 0; y < 8; y++) {
						for (x = 0; x < 8; x++) {
							if ((y == Y) && (x == X))
								continue; // don't count moved piece
							if ((PieceLoc[y][x] == BlackKnight) && (y != Y2) && (x != X2)) {
								pCheckMoves = CheckKnightMoves;
								AddFromLoc();
								break;
							}
						}
					}
					break;
				case WhiteKnight:
					PGN[ptr++] = 'N';
					PGN2[number].WhitePiece[ptr2++] = 'N';
					for (y = 0; y < 8; y++) {
						for (x = 0; x < 8; x++) {
							if ((y == Y) && (x == X))
								continue; // don't count moved piece
							if ((PieceLoc[y][x] == WhiteKnight) && ((y != Y2) || (x != X2))) {
								pCheckMoves = CheckKnightMoves;
								AddFromLoc();
								break;
							}
						}
					}
					break;
				case BlackBishop:
					PGN[ptr++] = 'B';
					PGN2[number].BlackPiece[ptr2++] = 'B';
					break;
				case WhiteBishop:
					PGN[ptr++] = 'B';
					PGN2[number].WhitePiece[ptr2++] = 'B';
					break;
				case BlackQueen:
					PGN[ptr++] = 'Q';
					PGN2[number].BlackPiece[ptr2++] = 'Q';
					break;
				case WhiteQueen:
					PGN[ptr++] = 'Q';
					PGN2[number].WhitePiece[ptr2++] = 'Q';
					break;
				case BlackKing:
					if (castled) {
						if (PieceLoc[0][6] == BlackKing) {
							PGN[ptr++] = 'O';
							PGN[ptr++] = '-';
							PGN[ptr++] = 'O';
							PGN2[number].BlackPiece[ptr2++] = 'O';
							PGN2[number].BlackPiece[ptr2++] = '-';
							PGN2[number].BlackPiece[ptr2++] = 'O';
						}
						else if (PieceLoc[0][2] == BlackKing) {
							PGN[ptr++] = 'O';
							PGN[ptr++] = '-';
							PGN[ptr++] = 'O';
							PGN[ptr++] = '-';
							PGN[ptr++] = 'O';
							PGN2[number].BlackPiece[ptr2++] = 'O';
							PGN2[number].BlackPiece[ptr2++] = '-';
							PGN2[number].BlackPiece[ptr2++] = 'O';
							PGN2[number].BlackPiece[ptr2++] = '-';
							PGN2[number].BlackPiece[ptr2++] = 'O';
						}
					}
					else {
						PGN[ptr++] = 'K';
						PGN2[number].BlackPiece[ptr2++] = 'K';
					}
					break;
				case WhiteKing:
					if (castled) {
						if (PieceLoc[7][6] == WhiteKing) {
							PGN[ptr++] = 'O';
							PGN[ptr++] = '-';
							PGN[ptr++] = 'O';
							PGN2[number].WhitePiece[ptr2++] = 'O';
							PGN2[number].WhitePiece[ptr2++] = '-';
							PGN2[number].WhitePiece[ptr2++] = 'O';
						}
						else if (PieceLoc[7][2] == WhiteKing) {
							PGN[ptr++] = 'O';
							PGN[ptr++] = '-';
							PGN[ptr++] = 'O';
							PGN[ptr++] = '-';
							PGN[ptr++] = 'O';
							PGN2[number].WhitePiece[ptr2++] = 'O';
							PGN2[number].WhitePiece[ptr2++] = '-';
							PGN2[number].WhitePiece[ptr2++] = 'O';
							PGN2[number].WhitePiece[ptr2++] = '-';
							PGN2[number].WhitePiece[ptr2++] = 'O';
						}
					}
					else {
						PGN[ptr++] = 'K';
						PGN2[number].WhitePiece[ptr2++] = 'K';
					}
					break;
				} // end of switch

				if (choosePiece) {
					choosePiece = FALSE;
					promoted = TRUE;
					DialogBox(hInst, TEXT("PROMOTION"), hwnd, PromotionProc); // returns //PieceLoc[Y2][X2] = WhiteQueen or whatever;
				}
				if (replacedPiece) {
					if (OriginX != 0xFF) {
						PGN[ptr++] = OriginX + 'a'; // put pawn's X before 'x'
						if (WhitesMove)
							PGN2[number].WhitePiece[ptr2++] = OriginX + 'a';
						else
							PGN2[number].BlackPiece[ptr2++] = OriginX + 'a';
					}
					if (WhitesMove)
						PGN2[number].WhitePiece[ptr2++] = 'x';
					else
						PGN2[number].BlackPiece[ptr2++] = 'x';
					PGN[ptr++] = 'x';
				}
				if (!castled) {
				//////////////////////
					PGN[ptr++] = X2 + 'a';
					PGN[ptr++] = '8' - Y2;
					if (WhitesMove) {
						PGN2[number].WhitePiece[ptr2++] = X2 + 'a';
						PGN2[number].WhitePiece[ptr2++] = '8' - Y2;
					}
					else {
						PGN2[number].BlackPiece[ptr2++] = X2 + 'a';
						PGN2[number].BlackPiece[ptr2++] = '8' - Y2;
					}
				//////////////////////
				}
				else { // if (castled)
					castled = FALSE;
				}
				PieceMoves[pm].Piece = PieceLoc[Y2][X2];
				PieceMoves[pm].PieceTaken = SaveReplacedPiece;
				PieceMoves[pm].fromX = X;
				PieceMoves[pm].fromY = Y;
				PieceMoves[pm].toX = X2;
				PieceMoves[pm].toY = Y2;
				if (promoted) {
					promoted = FALSE;
					PieceMoves[pm].Note1 = PromotedType;
					PGN[ptr++] = '=';
					PGN[ptr++] = PromotedType;
					if (WhitesMove) {
						PGN2[number].WhitePiece[ptr2++] = '=';
						PGN2[number].WhitePiece[ptr2++] = PromotedType;
					}
					else {
						PGN2[number].BlackPiece[ptr2++] = '=';
						PGN2[number].BlackPiece[ptr2++] = PromotedType;
					}
					checktype = CheckForCheck();
				}
				if (checktype == Check) {
					PGN[ptr++] = '+';
					if (WhitesMove)
						PGN2[number].WhitePiece[ptr2++] = '+';
					else
						PGN2[number].BlackPiece[ptr2++] = '+';

					checktype = 0;
					hFile = CreateFile("Check.wav", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
					PlayCheck();
				}
				else if (checktype == CheckMate) {
					PGN[ptr++] = '*';
					if (WhitesMove)
						PGN2[number].WhitePiece[ptr2++] = '*';
					else
						PGN2[number].BlackPiece[ptr2++] = '*';
					checktype = 0;
					hFile = CreateFile("CheckMate.wav", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
					PlayCheck();
				}
				if (PieceMoves[pm].Piece == WhiteKing) {
					if ((PieceMoves[pm].fromX == 4) && (PieceMoves[pm].toX == 6)) { // castling
						pm++;
						PieceMoves[pm].Piece = WhiteRook;
						PieceMoves[pm].PieceTaken = 0;
						PieceMoves[pm].fromX = 7;
						PieceMoves[pm].toX = 5;
						PieceMoves[pm].toY = PieceMoves[pm].fromY = 7;
					}
					else if ((PieceMoves[pm].fromX == 4) && (PieceMoves[pm].toX == 2)) { // castling
						pm++;
						PieceMoves[pm].Piece = WhiteRook;
						PieceMoves[pm].PieceTaken = 0;
						PieceMoves[pm].fromX = 0;
						PieceMoves[pm].toX = 3;
						PieceMoves[pm].toY = PieceMoves[pm].fromY = 7;
					}
				}
				else if (PieceMoves[pm].Piece == BlackKing) {
					if ((PieceMoves[pm].fromX == 4) && (PieceMoves[pm].toX == 6)) { // castling
						pm++;
						PieceMoves[pm].Piece = BlackRook;
						PieceMoves[pm].PieceTaken = 0;
						PieceMoves[pm].fromX = 7;
						PieceMoves[pm].toX = 5;
						PieceMoves[pm].toY = PieceMoves[pm].fromY = 0;
					}
					else if ((PieceMoves[pm].fromX == 4) && (PieceMoves[pm].toX == 2)) { // castling
						pm++;
						PieceMoves[pm].Piece = BlackRook;
						PieceMoves[pm].PieceTaken = 0;
						PieceMoves[pm].fromX = 0;
						PieceMoves[pm].toX = 3;
						PieceMoves[pm].toY = PieceMoves[pm].fromY = 0;
					}
				}
				if (pm < pmLoadedGame) {
					for (x = pm+1; x <= pmMax; x++) {
						PieceMoves[x].Piece = 0;
						PieceMoves[x].PieceTaken = 0;
						PieceMoves[x].fromX = 0;
						PieceMoves[x].fromY = 0;
						PieceMoves[x].toX = 0;
						PieceMoves[x].toY = 0;
						PieceMoves[x].Note1 = 0xFF;
					}
				}
				pmMax = pm;
				pm++;

				WhitesMove ^= 1;
				if (WhitesMove)
					SetWindowText(hwnd, WhiteMoves);
				else
					SetWindowText(hwnd, BlackMoves);
			}
skip:		movingPiece = FALSE;
			InvalidateRect(hwnd, &rect, FALSE);
		}
		return 0;

	case WM_KEYDOWN:
		switch(wParam)
		{
		case VK_F1: // for "White down 1 pawn", etc
			blackpawns = 0;
			blackrooks = 0;
			blackknights = 0;
			blackbishops = 0;
			blackqueen = 0;
			whitepawns = 0;
			whiterooks = 0;
			whiteknights = 0;
			whitebishops = 0;
			whitequeen = 0;
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					switch (PieceLoc[y][x])
					{
					case BlackPawn:
						blackpawns++;
						break;
					case BlackRook:
						blackrooks++;
						break;
					case BlackKnight:
						blackknights++;
						break;
					case BlackBishop:
						blackbishops++;
						break;
					case BlackQueen:
						blackqueen++;
						break;
					case WhitePawn:
						whitepawns++;
						break;
					case WhiteRook:
						whiterooks++;
						break;
					case WhiteKnight:
						whiteknights++;
						break;
					case WhiteBishop:
						whitebishops++;
						break;
					case WhiteQueen:
						whitequeen++;
						break;
					}
				}
			}
			x = 0;
			if ((blackqueen-whitequeen) == 1)
				x += sprintf(&pawns[x], "White is down 1 queen\r\n");
			else if ((blackqueen-whitequeen) > 1)
				x += sprintf(&pawns[x], "White is down %i queens\r\n", blackqueen-whitequeen);
			if ((blackrooks-whiterooks) == 1)
					x += sprintf(&pawns[x], "White is down 1 rook\r\n");
			else if ((blackrooks-whiterooks) > 1)
				x += sprintf(&pawns[x], "White is down %i rooks\r\n", blackrooks-whiterooks);
			if ((blackbishops-whitebishops) == 1)
				x += sprintf(&pawns[x], "White is down 1 bishop\r\n");
			else if ((blackbishops-whitebishops) > 1)
				x += sprintf(&pawns[x], "White is down %i bishops\r\n", blackbishops-whitebishops);
			if ((blackknights-whiteknights) == 1)
				x += sprintf(&pawns[x], "White is down 1 knight\r\n");
			else if ((blackknights-whiteknights) > 1)
				x += sprintf(&pawns[x], "White is down %i knights\r\n", blackknights-whiteknights);
			if ((blackpawns-whitepawns) == 1)
				x += sprintf(&pawns[x], "White is down 1 pawn\r\n");
			else if ((blackpawns-whitepawns) > 1)
				x += sprintf(&pawns[x], "White is down %i pawns\r\n", blackpawns-whitepawns);
			if (x)
				x += sprintf(&pawns[x], "\r\n");
			if ((whitequeen-blackqueen) == 1)
				x += sprintf(&pawns[x], "Black is down 1 queen\r\n");
			else if ((whitequeen-blackqueen) > 1)
				x += sprintf(&pawns[x], "Black is down %i queens\r\n", whitequeen-blackqueen);
			if ((whiterooks-blackrooks) == 1)
				x += sprintf(&pawns[x], "Black is down 1 rook\r\n");
			else if ((whiterooks-blackrooks) > 1)
				x += sprintf(&pawns[x], "Black is down %i rooks\r\n", whiterooks-blackrooks);
			if ((whitebishops-blackbishops) == 1)
				x += sprintf(&pawns[x], "Black is down 1 bishop\r\n");
			else if ((whitebishops-blackbishops) > 1)
				x += sprintf(&pawns[x], "Black is down %i bishops\r\n", whitebishops-blackbishops);
			if ((whiteknights-blackknights) == 1)
				x += sprintf(&pawns[x], "Black is down 1 knight\r\n");
			else if ((whiteknights-blackknights) > 1)
				x += sprintf(&pawns[x], "Black is down %i knights\r\n", whiteknights-blackknights);
			if ((whitepawns-blackpawns) == 1)
				x += sprintf(&pawns[x], "Black is down 1 pawn\r\n");
			else if ((whitepawns-blackpawns) > 1)
				x += sprintf(&pawns[x], "Black is down %i pawns\r\n", whitepawns-blackpawns);
			if (x)
				MessageBox(hwnd, pawns, "", MB_OK);
			else
				MessageBox(hwnd, "Sides are even", "", MB_OK);
			break;

		case VK_HOME:
			fromhome = TRUE;
			if (pm > pmHome) {
				while (pm > pmHome) {
					SendMessage(hwnd, WM_KEYDOWN, VK_LEFT, 0);
				}
			}
			else {
				while (pm < pmHome) {
					SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
				}
			}
			fromhome = FALSE;
			if (WhitesMove)
				SetWindowText(hwnd, WhiteMoves);
			else
				SetWindowText(hwnd, BlackMoves);
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case VK_END:
			fromend = TRUE;
			while (pm <= pmMax) {
				SendMessage(hwnd, WM_KEYDOWN, VK_RIGHT, 0);
			}
			fromend = FALSE;
			if (WhitesMove)
				SetWindowText(hwnd, WhiteMoves);
			else
				SetWindowText(hwnd, BlackMoves);
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case VK_RETURN:
			if (!placepieces) {
				pmHome = pm;
				break;
			}
		case VK_SPACE:
			if (placepieces)
				SendMessage(hwndPlacePieces, WM_COMMAND, IDOK, 0);
			break;
			
		case VK_DELETE:
			if (placepieces) {
				Placed[(yPos-Border)/SquareSize][(xPos-LeftSide)/SquareSize] = 0;
				InvalidateRect(hwnd, &rect, FALSE);
			}
			break;
		case 'W':
			if (placepieces)
				SendMessage(hwndPlacePieces, WM_COMMAND, IDC_RADIO1, 0);
			break;
		case 'K':
			if (placepieces) {
				if (color == 'W')
					piece = WhiteKing;
				else // if (color == 'B')
					piece = BlackKing;
			}
			if ((xPos >= LeftSide) && (xPos <= LeftSide+(SquareSize*8)) && (yPos >= Border) && (yPos <= (SquareSize*8)+Border))
				Placed[(yPos-Border)/SquareSize][(xPos-LeftSide)/SquareSize] = piece;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'Q':
			if (placepieces) {
				if (color == 'W')
					piece = WhiteQueen;
				else // if (color == 'B')
					piece = BlackQueen;
			}
			if ((xPos >= LeftSide) && (xPos <= LeftSide+(SquareSize*8)) && (yPos >= Border) && (yPos <= (SquareSize*8)+Border))
				Placed[(yPos-Border)/SquareSize][(xPos-LeftSide)/SquareSize] = piece;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'B':
			if (placepieces) {
				if (GetKeyState(VK_SHIFT) & 0x8000) { // Shift is pressed
					SendMessage(hwndPlacePieces, WM_COMMAND, IDC_RADIO2, 0);
					break;
				}
				else if (color == 'W')
					piece = WhiteBishop;
				else // if (color == 'B')
					piece = BlackBishop;
			}
			if ((xPos >= LeftSide) && (xPos <= LeftSide+(SquareSize*8)) && (yPos >= Border) && (yPos <= (SquareSize*8)+Border))
				Placed[(yPos-Border)/SquareSize][(xPos-LeftSide)/SquareSize] = piece;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'N':
			if (placepieces) {
				if (color == 'W')
					piece = WhiteKnight;
				else // if (color == 'B')
					piece = BlackKnight;
			}
			if ((xPos >= LeftSide) && (xPos <= LeftSide+(SquareSize*8)) && (yPos >= Border) && (yPos <= (SquareSize*8)+Border))
				Placed[(yPos-Border)/SquareSize][(xPos-LeftSide)/SquareSize] = piece;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'R':
			if (placepieces) {
				if (color == 'W')
					piece = WhiteRook;
				else // if (color == 'B')
					piece = BlackRook;
			}
			if ((xPos >= LeftSide) && (xPos <= LeftSide+(SquareSize*8)) && (yPos >= Border) && (yPos <= (SquareSize*8)+Border))
				Placed[(yPos-Border)/SquareSize][(xPos-LeftSide)/SquareSize] = piece;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'P':
			if (placepieces) {
				if (color == 'W')
					piece = WhitePawn;
				else // if (color == 'B')
					piece = BlackPawn;
			}
			if ((xPos >= LeftSide) && (xPos <= LeftSide+(SquareSize*8)) && (yPos >= Border) && (yPos <= (SquareSize*8)+Border))
				Placed[(yPos-Border)/SquareSize][(xPos-LeftSide)/SquareSize] = piece;
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case VK_LEFT:
x1:		if (pm > 32) {
				pm--;
				if (PieceMoves[pm].Note1 == 0xFF) // not a promoted pawn
					PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = PieceMoves[pm].Piece;
				else {
					if (!WhitesMove)
						PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = WhitePawn;
					else
						PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = BlackPawn;
				}
				if ((PieceMoves[pm-1].Piece == WhitePawn) && (PieceMoves[pm-1].fromY == 6) && (PieceMoves[pm-1].toY == 4)) // en passant
					PieceLoc[4][PieceMoves[pm-1].toX] = WhitePawn;
				else if ((PieceMoves[pm-1].Piece == BlackPawn) && (PieceMoves[pm-1].fromY == 1) && (PieceMoves[pm-1].toY == 3)) // en passant
					PieceLoc[3][PieceMoves[pm-1].toX] = BlackPawn;
				PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX] = PieceMoves[pm].PieceTaken;
				if (((PieceMoves[pm-1].Piece != WhiteKing) && (PieceMoves[pm-1].Piece != BlackKing))
				 || ((abs(PieceMoves[pm-1].toX - PieceMoves[pm-1].fromX) != 2))) { // not castling
					WhitesMove ^= 1;
					if (WhitesMove)
						number--;
				}
				else { // if castling
					goto x1; // make castling one move
				}
				if (!fromhome) {
					if (WhitesMove)
						SetWindowText(hwnd, WhiteMoves);
					else
						SetWindowText(hwnd, BlackMoves);
					InvalidateRect(hwnd, &rect, FALSE);
				}
			}
			break;
		case VK_RIGHT:
			if (pm <= pmMax) { // pm 53
x2:			OldPiece = PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX];
				PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX] = PieceMoves[pm].Piece;
				PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = 0;
				if (((PieceMoves[pm].Piece != WhiteKing) && (PieceMoves[pm].Piece != BlackKing))
				 || ((abs(PieceMoves[pm].toX - PieceMoves[pm].fromX) != 2)))
				{ // not castling
					WhitesMove ^= 1;
					if (!WhitesMove)
						number++;

					if ((PieceMoves[pm].Piece == BlackPawn)
					 && (PieceMoves[pm].toY == 5)
					 && (PieceMoves[pm-1].Piece == WhitePawn)
					 && ((PieceMoves[pm-1].toX == PieceMoves[pm].fromX-1) || (PieceMoves[pm-1].toX == PieceMoves[pm].fromX+1))
					 && (PieceMoves[pm-1].toY == 4)) {
						PieceLoc[PieceMoves[pm-1].toY][PieceMoves[pm-1].toX] = 0; // en passant
					}
					else if ((PieceMoves[pm].Piece == WhitePawn)
					 && (PieceMoves[pm].toY == 2)
					 && (PieceMoves[pm-1].Piece == BlackPawn)
					 && ((PieceMoves[pm-1].toX == PieceMoves[pm].fromX-1) || (PieceMoves[pm-1].toX == PieceMoves[pm].fromX+1))
					 && (PieceMoves[pm-1].toY == 3)) {
						PieceLoc[PieceMoves[pm-1].toY][PieceMoves[pm-1].toX] = 0; // en passant
					}

					pm++;
					if (!fromcastling) {
						if ((!fromend) && (0 == (GetKeyState(VK_CONTROL) & 0x80000000))) { // Ctrl not down
							fromright = TRUE; // to move piece slowly
							slowY = PieceMoves[pm].fromY;
						}
					}
					else // if (fromcastling)
						fromcastling = FALSE;
				}
				else { // if castling
					pm++;
					fromcastling = TRUE;
					goto x2; // make castling one move
				}
				if ((!fromend) && (!fromhome)) {
					if (WhitesMove)
						SetWindowText(hwnd, WhiteMoves);
					else
						SetWindowText(hwnd, BlackMoves);
					if (!fromright)
						InvalidateRect(hwnd, &rect, FALSE);
					else { // if (fromright)
						slowPiece = PieceMoves[pm-1].Piece;
						fromX = PieceMoves[pm-1].fromX;
						fromY = PieceMoves[pm-1].fromY;
						destX = PieceMoves[pm-1].toX;
						destY = PieceMoves[pm-1].toY;
						slowX = LeftSide+(SquareSize*PieceMoves[pm-1].fromX);
						slowY = Border+(SquareSize*PieceMoves[pm-1].fromY);
						slowX2 = LeftSide+(SquareSize*destX);
						slowY2 = Border+(SquareSize*destY);
						diffX = diffY = 0;
						bigNum = (20 * SquareSize / 86) & 0xFFFFFFFE; // make it even
						littleNum = bigNum / 2;
						if (destX > fromX) {
							if (abs(destX-fromX) == abs(destY-fromY)*2) {
								diffX = bigNum;
								slowX2 = slowX + ((slowX2-slowX)/bigNum)*bigNum;
							}
							else {
								diffX = littleNum;
								slowX2 = slowX + ((slowX2-slowX)/littleNum)*littleNum;
							}
						}
						else if (destX < fromX) {
							if (abs(destX-fromX) == abs(destY-fromY)*2) {
								diffX = -bigNum;
								slowX2 = slowX + ((slowX2-slowX)/bigNum)*bigNum;
							}
							else {
								diffX = -littleNum;
								slowX2 = slowX + ((slowX2-slowX)/littleNum)*littleNum;
							}
						}
						if (destY > fromY) {
							if (abs(destY-fromY) == abs(destX-fromX)*2) {
								diffY = bigNum;
								slowY2 = slowY + ((slowY2-slowY)/bigNum)*bigNum;
							}
							else {
								diffY = littleNum;
								slowY2 = slowY + ((slowY2-slowY)/littleNum)*littleNum;
							}
						}
						else if (destY < fromY) {
							if (abs(destY-fromY) == abs(destX-fromX)*2) {
								diffY = -bigNum;
								slowY2 = slowY + ((slowY2-slowY)/bigNum)*bigNum;
							}
							else {
								diffY = -littleNum;
								slowY2 = slowY + ((slowY2-slowY)/littleNum)*littleNum;
							}
						}
						for ( ; (slowX != slowX2) || (slowY != slowY2); slowX += diffX, slowY += diffY) {
							InvalidateRect(hwnd, &rect, FALSE);
							UpdateWindow(hwnd); // necessary
						}
						fromright = FALSE;
						destX = destY = -1; // back to normal
						if (!fromend) {
							InvalidateRect(hwnd, &rect, FALSE);
						}
					}
				}
			}
			break;
		}
		return 0;


	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		if (hdcMem) {
			SelectObject(hdcMem, hOldBitmap);
			DeleteDC(hdcMem);
			DeleteObject(hMemBitmap);
		}
		hdcMem = CreateCompatibleDC(hdc);
		hMemBitmap = CreateCompatibleBitmap(hdc, rect.right, rect.bottom);
		hOldBitmap = SelectObject(hdcMem, hMemBitmap);
		FillRect(hdcMem, &rect, hGreyBrush);
		hPenObject = SelectObject(hdcMem, hBorderPen);
		hBrushObject = SelectObject(hdcMem, hBorderBrush);
		if (!showing) {
			Rectangle(hdcMem, LeftSide, 0, LeftSide+(SquareSize*8), Border);
			Rectangle(hdcMem, LeftSide, Border+(SquareSize*8), LeftSide+(SquareSize*8), Border + (SquareSize*8) + Border);
			Rectangle(hdcMem, LeftSide-Border, 0, LeftSide+Border, Border + (SquareSize*8) + Border);
			Rectangle(hdcMem, LeftSide+(SquareSize*8), 0, LeftSide+(SquareSize*8)+Border, Border + (SquareSize*8) + Border);
		}
		SelectObject(hdcMem, hBrushObject);
		SelectObject(hdcMem, hPenObject);

		if (showPGN) {
			SetBkMode(hdcMem, TRANSPARENT);
			if (gotbracket)
				TextOut(hdcMem, 0, 0, PGNtitle, strlen(PGNtitle));
			else
				TextOut(hdcMem, 0, 0, Filename, strlen(Filename));
			for (x = 0, y = 1, n = 1; n <= number; y++, n++) {
				if (n == Lines) {
					x = rect.right - 100;
					y = 0;
				}
				for (i = 0, j = 0; PGN2[n].Number[j] != 0; i++, j++) {
					pgn[i] = PGN2[n].Number[j];
				}
				pgn[i++] = ' ';
				for (j = 0; PGN2[n].WhitePiece[j] != 0; i++, j++) {
					pgn[i] = PGN2[n].WhitePiece[j];
				}
				if ((n < number) || (WhitesMove)) { // for left/right arrows
					pgn[i++] = ' ';
					for (j = 0; PGN2[n].BlackPiece[j] != 0; i++, j++) {
						pgn[i] = PGN2[n].BlackPiece[j];
					}
				}
				/////////////////////////////////
				TextOut(hdcMem, x, y*20, pgn, i);
				/////////////////////////////////
			}
			letter[0] = 'a';
			for (x = 0; x < (SquareSize*8); x += SquareSize, letter[0]++) {
				TextOut(hdcMem, LeftSide+x+(SquareSize/2), Border+(SquareSize*8), &letter[0], 1);
//				TextOut(hdcMem, LeftSide+x+(SquareSize/2), 0, &letter[0], 1);
			}
			num[0] = '8';
			for (y = 0; y < (SquareSize*8); y += SquareSize, num[0]--) {
				TextOut(hdcMem, LeftSide-(Border*3/4), (Border/2)+y+(SquareSize/2), &num[0], 1);
//				TextOut(hdcMem, LeftSide+(Border/4)+(SquareSize*8), (Border/2)+y+(SquareSize/2), &num[0], 1);
			}
		}

		if ((!showing) || (placepieces)) {
			hPenObject = SelectObject(hdcMem, hBeigePen);
			hBrushObject = SelectObject(hdcMem, hBeigeBrush);
			for (y = Border; y < (Border + (SquareSize*8)); y += SquareSize*2) {
				for (x = LeftSide; x < (LeftSide+(SquareSize*8)); x += SquareSize*2)
					Rectangle(hdcMem, x, y, x+SquareSize, y+SquareSize);
			}
			for (y = Border+SquareSize; y < (Border+(SquareSize*8)); y += SquareSize*2) {
				for (x = LeftSide+SquareSize; x < (LeftSide+SquareSize+(SquareSize*8)); x += SquareSize*2)
					Rectangle(hdcMem, x, y, x+SquareSize, y+SquareSize);
			}
			SelectObject(hdcMem, hBrownBrush);
			SelectObject(hdcMem, hBrownPen);
			for (y = Border+SquareSize; y < (Border + (SquareSize*8)); y += SquareSize*2) {
				for (x = LeftSide; x < (LeftSide+(SquareSize*8)); x += SquareSize*2)
					Rectangle(hdcMem, x, y, x+SquareSize, y+SquareSize);
			}
			for (y = Border; y < (Border + (SquareSize*8)); y += SquareSize*2) {
				for (x = LeftSide+SquareSize; x < (LeftSide+SquareSize+(SquareSize*8)); x += SquareSize*2) {
					Rectangle(hdcMem, x, y, x+SquareSize, y+SquareSize);
				}
			}
			SelectObject(hdcMem, hBrushObject);
			SelectObject(hdcMem, hPenObject);
		}

		else if ((showing) && (!placepieces)) {
			GetBlackAndWhiteNums();
			hPenObject = SelectObject(hdcMem, hGreyPen);
			for (y = Border, i = 0; y < (Border + (SquareSize*8)); y += SquareSize, i++) {
				for (x = LeftSide, j = 0; x < (LeftSide+(SquareSize*8)); x += SquareSize, j++) {
					fook = BlackNums[i][j] - WhiteNums[i][j];
					if (fook >= 2)
						hBrushObject = SelectObject(hdcMem, hBlack);
					else if (fook == 1)
						hBrushObject = SelectObject(hdcMem, hDarkGrey);
					else if (fook == 0)
						hBrushObject = SelectObject(hdcMem, hGreyBrush);
					else if (fook == -1)
						hBrushObject = SelectObject(hdcMem, hLightGrey);
					else if (fook <= -2)
						hBrushObject = SelectObject(hdcMem, hWhite);
					Rectangle(hdcMem, x, y, x+SquareSize, y+SquareSize);
					SelectObject(hdcMem, hBrushObject);
				}
			}
			SelectObject(hdcMem, hPenObject);
		}
		hOldFont = SelectObject(hdcMem, hFont);
		SetBkMode(hdcMem, TRANSPARENT);
		if (!placepieces) {
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					if ((y != destY) || (x != destX))
						TextOut(hdcMem, LeftSide+(SquareSize*x), Border+(SquareSize*y), &Chessmen[PieceLoc[y][x]], 1);
					else { // move piece slowly
						TextOut(hdcMem, LeftSide+(SquareSize*x), Border+(SquareSize*y), &Chessmen[OldPiece], 1);
						TextOut(hdcMem, slowX, slowY, &Chessmen[slowPiece], 1);
					}
				}
			}
		}
		else {
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++)
					TextOut(hdcMem, LeftSide+(SquareSize*x), Border+(SquareSize*y), &Chessmen[Placed[y][x]], 1);
			}
		}
		SelectObject(hdcMem, hOldFont);

		///////////
		BitBlt(hdc, 0, 0, rect.right, rect.bottom, hdcMem, 0, 0, SRCCOPY);
		///////////
		if (movingPiece) {
			hOldFont = SelectObject(hdc, hFont);
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, xPos-(SquareSize/2), yPos-(SquareSize/2), &Chessmen[MovedPiece], 1);
			SetBkMode(hdc, OPAQUE);
			SelectObject(hdc, hOldFont);
		}
		if ((showingnums) && (!placepieces)) {
			GetBlackAndWhiteNums();
			SquareMiddle = SquareSize/2;
			SetBkMode(hdc, TRANSPARENT);
			SetTextColor(hdc, 0x707070);
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					ch[0] = '0' + BlackNums[y][x];
					TextOut(hdc, LeftSide+(SquareSize*x), Border+(SquareSize*y)+SquareMiddle-20, ch, 1);
				}
			}
			SetTextColor(hdc, 0x707070);
			for (y = 0; y < 8; y++) {
				for (x = 0; x < 8; x++) {
					ch[0] = '0' + WhiteNums[y][x];
					TextOut(hdc, LeftSide+(SquareSize*x), Border+(SquareSize*y)+SquareMiddle, ch, 1);
				}
			}
			SetBkMode(hdc, OPAQUE);
		}
		EndPaint(hwnd, &ps);
		return 0;


	case WM_DESTROY:
		RemoveFontResource("ChessAlpha2.ttf");
		DeleteDC(hdcMem);
		DeleteObject(hMemBitmap);
		if (WaveBuf)
			VirtualFree(WaveBuf, 0, MEM_RELEASE);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

//SUB-ROUTINES*******************************************************

void Setup(void)
{
	pmHome = 32;
	if (hwndPlacePieces)
		SendMessage(hwndPlacePieces, WM_CLOSE, 0, 0);
	nosave = FALSE;
	SetWindowText(hwnd, WhiteMoves);
	placepieces = FALSE;
	pmLoadedGame = 0;
	rotated = FALSE;
	WhitesMove = TRUE;
	pm = 32; // pm points to first empty PieceMoves
	pmMax = 31;
	number = 0;
	for (x = 0; x < PGNSIZE; x++)
		PGN[x] = 0;
	for (x = 0; x < PGN2SIZE; x++) {
		for (y = 0; y < 4; y++)
			PGN2[x].Number[y] = 0;
		for (y = 0; y < 8; y++) {
			PGN2[x].WhitePiece[y] = 0;
			PGN2[x].BlackPiece[y] = 0;
		}
	}
	ptr = ptr2 = 0;
	PieceLoc[0][0] = BlackRook;
	PieceLoc[0][1] = BlackKnight;
	PieceLoc[0][2] = BlackBishop;
	PieceLoc[0][3] = BlackQueen;
	PieceLoc[0][4] = BlackKing;
	PieceLoc[0][5] = BlackBishop;
	PieceLoc[0][6] = BlackKnight;
	PieceLoc[0][7] = BlackRook;
	PieceLoc[7][0] = WhiteRook;
	PieceLoc[7][1] = WhiteKnight;
	PieceLoc[7][2] = WhiteBishop;
	PieceLoc[7][3] = WhiteQueen;
	PieceLoc[7][4] = WhiteKing;
	PieceLoc[7][5] = WhiteBishop;
	PieceLoc[7][6] = WhiteKnight;
	PieceLoc[7][7] = WhiteRook;
	for (x = 0; x < 8; x++)
		PieceLoc[1][x] = BlackPawn;
	for (x = 0; x < 8; x++)
		PieceLoc[6][x] = WhitePawn;
	for (y = 2; y < 6; y++) {
		for (x = 0; x < 8; x++)
			PieceLoc[y][x] = 0;
	}
	// load 32 pieces to their default positions
	for (x = 0; x < PIECEMOVES; x++) {
		PieceMoves[x].PieceTaken = 0;
		PieceMoves[x].Note1 = 0xFF;
	}
	PieceMoves[0].Piece = WhiteRook;
	PieceMoves[1].Piece = WhiteKnight;
	PieceMoves[2].Piece = WhiteBishop;
	PieceMoves[3].Piece = WhiteQueen;
	PieceMoves[4].Piece = WhiteKing;
	PieceMoves[5].Piece = WhiteBishop;
	PieceMoves[6].Piece = WhiteKnight;
	PieceMoves[7].Piece = WhiteRook;

	PieceMoves[8].Piece = BlackRook;
	PieceMoves[9].Piece = BlackKnight;
	PieceMoves[10].Piece = BlackBishop;
	PieceMoves[11].Piece = BlackQueen;
	PieceMoves[12].Piece = BlackKing;
	PieceMoves[13].Piece = BlackBishop;
	PieceMoves[14].Piece = BlackKnight;
	PieceMoves[15].Piece = BlackRook;
	for (x = 0, y = 0; x < 8; x++, y++) {
		PieceMoves[x].fromX = PieceMoves[x].toX = y;
		PieceMoves[x].fromY = PieceMoves[x].toY = 7;
	}
	for (x = 8, y = 0; x < 16; x++, y++) {
		PieceMoves[x].fromX = PieceMoves[x].toX = y;
		PieceMoves[x].fromY = PieceMoves[x].toY = 0;
	}
	for (x = 16, y = 0; x < 24; x++, y++) {
		PieceMoves[x].Piece = WhitePawn;
		PieceMoves[x].fromX = PieceMoves[x].toX = y;
		PieceMoves[x].fromY = PieceMoves[x].toY = 6;
	}
	for (x = 24, y = 0; x < 32; x++, y++) {
		PieceMoves[x].Piece = BlackPawn;
		PieceMoves[x].fromX = PieceMoves[x].toX = y;
		PieceMoves[x].fromY = PieceMoves[x].toY = 1;
	}
	for (x = 32; x < PIECEMOVES; x++) {
		PieceMoves[x].Piece = 0;
		PieceMoves[x].PieceTaken = 0;
		PieceMoves[x].fromX = 0;
		PieceMoves[x].fromY = 0;
		PieceMoves[x].toX = 0;
		PieceMoves[x].toY = 0;
		PieceMoves[x].Note1 = 0xFF;
	}
}

BOOL CheckPassedThruSquare(int PassedThruSquareY, int PassedThruSquareX, int King)
{
	int x, y;

	SaveX2 = X2;
	SaveY2 = Y2;	
	X2 = PassedThruSquareX;
	Y2 = PassedThruSquareY;
	for (y = 0; y < 8; y++) { // check for check or discovered check
		for (x = 0; x < 8; x++) {
			if (PieceLoc[y][x]) {
				X = x;
				Y = y;
				MovedPiece = PieceLoc[y][x];
				ReplacedPiece = King;
				if (CheckMove()) { // move to PassedThruSquare can be made (so King would be passing thru check)
					MovedPiece = SaveMovedPiece;
					ReplacedPiece = SaveReplacedPiece;
					X = SaveX;
					Y = SaveY;
					X2 = SaveX2;
					Y2 = SaveY2;
					return FALSE;
				}
			}
		}
	}
	MovedPiece = SaveMovedPiece;
	ReplacedPiece = SaveReplacedPiece;
	X = SaveX;
	Y = SaveY;
	X2 = SaveX2;
	Y2 = SaveY2;
	return TRUE;
}

BOOL CheckRookMoves(void) // is this a legal move?
{
	int x, y;

	if (X2 == X) {
		if (Y2 > Y) {
			for (y = Y+1; y < Y2; y++)
				if (PieceLoc[y][X] != 0)
					return FALSE;
		}
		else { // if (Y2 < Y) {
			for (y = Y-1; y > Y2; y--) {
				if (PieceLoc[y][X] != 0)
					return FALSE;
			}
		}
		return TRUE;
	}
	else if (Y2 == Y) {
		if (X2 > X) {
			for (x = X+1; x < X2; x++) {
				if (PieceLoc[Y][x] != 0)
					return FALSE;
			}
		}
		else { // if (X2 < X) {
			for (x = X-1; x > X2; x--) {
				if (PieceLoc[Y][x] != 0)
					return FALSE;
			}
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CheckKnightMoves(void) // is this a legal move?
{
	if ((X2 == X-1) && (Y2 == Y+2))
		return TRUE;
	else if ((X2 == X+1) && (Y2 == Y+2))
		return TRUE;
	else if ((X2 == X-2) && (Y2 == Y+1))
		return TRUE;
	else if ((X2 == X+2) && (Y2 == Y+1))
		return TRUE;
	else if ((X2 == X-1) && (Y2 == Y-2))
		return TRUE;
	else if ((X2 == X+1) && (Y2 == Y-2))
		return TRUE;
	else if ((X2 == X-2) && (Y2 == Y-1))
		return TRUE;
	else if ((X2 == X+2) && (Y2 == Y-1))
		return TRUE;
	else
		return FALSE;
}

BOOL CheckBishopMoves(void) // is this a legal move?
{
	if (abs(X2-X) != abs(Y2-Y))
		return FALSE;
	if ((X2 > X) && (Y2 > Y)) {
		for (x = X+1, y = Y+1; x < X2; x++, y++) {
			if (PieceLoc[y][x] != 0)
				return FALSE;
		}
		return TRUE;
	}
	else if ((X2 < X) && (Y2 > Y)) {
		for (x = X-1, y = Y+1; x > X2; x--, y++) {
			if (PieceLoc[y][x] != 0)
				return FALSE;
		}
		return TRUE;
	}
	else if ((X2 > X) && (Y2 < Y)) {
		for (x = X+1, y = Y-1; x < X2; x++, y--) {
			if (PieceLoc[y][x] != 0)
				return FALSE;
		}
		return TRUE;
	}
	else if ((X2 < X) && (Y2 < Y)) {
		for (x = X-1, y = Y-1; x > X2; x--, y--) {
			if (PieceLoc[y][x] != 0)
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CheckQueenMoves(void) // is this a legal move?
{
	if (X2 == X) {
		if (Y2 > Y) {
			for (y = Y+1; y < Y2; y++)
				if (PieceLoc[y][X] != 0)
					return FALSE;
		}
		else { // if (Y2 < Y) {
			for (y = Y-1; y > Y2; y--) {
				if (PieceLoc[y][X] != 0)
					return FALSE;
			}
		}
		return TRUE;
	}
	else if (Y2 == Y) {
		if (X2 > X) {
			for (x = X+1; x < X2; x++) {
				if (PieceLoc[Y][x] != 0)
					return FALSE;
			}
		}
		else { // if (X2 < X) {
			for (x = X-1; x > X2; x--) {
				if (PieceLoc[Y][x] != 0)
					return FALSE;
			}
		}
		return TRUE;
	}

	else if (abs(X2-X) != abs(Y2-Y))
		return FALSE;
	else if ((X2 > X) && (Y2 > Y)) {
		for (x = X+1, y = Y+1; x < X2; x++, y++) {
			if (PieceLoc[y][x] != 0)
				return FALSE;
		}
		return TRUE;
	}
	else if ((X2 < X) && (Y2 > Y)) {
		for (x = X-1, y = Y+1; x > X2; x--, y++) {
			if (PieceLoc[y][x] != 0)
				return FALSE;
		}
		return TRUE;
	}
	else if ((X2 > X) && (Y2 < Y)) {
		for (x = X+1, y = Y-1; x < X2; x++, y--) {
			if (PieceLoc[y][x] != 0)
				return FALSE;
		}
		return TRUE;
	}
	else if ((X2 < X) && (Y2 < Y)) {
		for (x = X-1, y = Y-1; x > X2; x--, y--) {
			if (PieceLoc[y][x] != 0)
				return FALSE;
		}
		return TRUE;
	}
	return FALSE;
}

BOOL CheckKingMoves(void) // is this a legal move?
{
	if (((X2 == X+1) || (X2 == X-1) || (X2 == X)) && ((Y2 == Y+1) || (Y2 == Y-1) || (Y2 == Y)))
		return TRUE;
	else
		return FALSE;
}

BOOL CheckMove(void)
{
	if ((X2 == X) && (Y2 == Y))
		return FALSE;
	switch (MovedPiece)
	{
	case BlackPawn:
		if ((ReplacedPiece >= 224) && (ReplacedPiece <= 334))
			return FALSE; // if ReplacedPiece is black (leave this check here because of CheckForCheck (both white & black are checked))
		if (fromblackandwhite) {
			if ((Y2 == Y+1) && ((X2 == X+1) || (X2 == X-1)))
				return TRUE;
			else
				return FALSE;
		}
		if (!fromcheckcheck) {
			if (Y2 == 7)
				choosePiece = TRUE; // promote the pawn
			else
				choosePiece = FALSE;
			if ((PieceMoves[pm-1].Piece == WhitePawn) && (PieceMoves[pm-1].fromY == 6) && (PieceMoves[pm-1].toY == 4)) { // en passant
				if ((PieceLoc[Y][X-1] == WhitePawn) && (X2 == X-1)) {
					PieceLoc[Y][X-1] = 0;
					replacedPiece = TRUE; // for 'x' in PGN
					return TRUE;
				}
				else if ((PieceLoc[Y][X+1] == WhitePawn) && (X2 == X+1)) {
					PieceLoc[Y][X+1] = 0;
					replacedPiece = TRUE; // for 'x' in PGN
					return TRUE;
				}
			}
		}
		if (!rotated) {
			if (((Y2 == Y+1) || ((Y2 == 3) && (Y == 1))) && (X2 == X) && (ReplacedPiece == 0))
				return TRUE;
			else if ((Y2 == Y+1) && ((X2 == X-1) || (X2 == X+1)) && (ReplacedPiece != 0))
				return TRUE;
			else
				return FALSE;
		}
		else {
			if (((Y2 == Y-1) || (Y2 == 4)) && (X2 == X) && (ReplacedPiece == 0))
				return TRUE;
			else if ((Y2 == Y-1) && ((X2 == X-1) || (X2 == X+1)) && (ReplacedPiece != 0))
				return TRUE;
			else
				return FALSE;
		}
	case WhitePawn:
		if ((ReplacedPiece >= 192) && (ReplacedPiece <= 202))
			return FALSE; // if ReplacedPiece is white
		if (fromblackandwhite) {
			if ((Y2 == Y-1) && ((X2 == X+1) || (X2 == X-1)))
				return TRUE;
			else
				return FALSE;
		}
		if (!fromcheckcheck) {
			if (Y2 == 0)
				choosePiece = TRUE;
			else
				choosePiece = FALSE;
			if ((PieceMoves[pm-1].Piece == BlackPawn) && (PieceMoves[pm-1].fromY == 1) && (PieceMoves[pm-1].toY == 3)) { // en passant
				if ((PieceLoc[Y][X-1] == BlackPawn) && (X2 == X-1)) {
					PieceLoc[Y][X-1] = 0;
					replacedPiece = TRUE; // for 'x' in PGN
					return TRUE;
				}
				else if ((PieceLoc[Y][X+1] == BlackPawn) && (X2 == X+1)) {
					PieceLoc[Y][X+1] = 0;
					replacedPiece = TRUE; // for 'x' in PGN
					return TRUE;
				}
			}
		}
		if (!rotated) {
			if (((Y2 == Y-1) || ((Y2 == 4) && (Y == 6))) && (X2 == X) && (ReplacedPiece == 0))
				return TRUE;
			else if ((Y2 == Y-1) && ((X2 == X-1) || (X2 == X+1)) && (ReplacedPiece != 0))
				return TRUE;
			else
				return FALSE;
		}
		else {
			if (((Y2 == Y+1) || (Y2 == 3)) && (X2 == X) && (ReplacedPiece == 0))
				return TRUE;
			else if ((Y2 == Y+1) && ((X2 == X-1) || (X2 == X+1)) && (ReplacedPiece != 0))
				return TRUE;
			else
				return FALSE;
		}
	case BlackKnight:
		if ((ReplacedPiece >= 224) && (ReplacedPiece <= 334))
			return FALSE; // if ReplacedPiece is black
		if (CheckKnightMoves())
			return TRUE;
		else
			return FALSE;
	case WhiteKnight:
		if ((ReplacedPiece >= 192) && (ReplacedPiece <= 202))
			return FALSE; // if ReplacedPiece is white
		if (CheckKnightMoves())
			return TRUE;
		else
			return FALSE;
	case BlackBishop:
		if ((ReplacedPiece >= 224) && (ReplacedPiece <= 334))
			return FALSE; // if ReplacedPiece is black
		if (CheckBishopMoves())
			return TRUE;
		else
			return FALSE;
	case WhiteBishop:
		if ((ReplacedPiece >= 192) && (ReplacedPiece <= 202))
			return FALSE; // if ReplacedPiece is white
		if (CheckBishopMoves())
			return TRUE;
		else
			return FALSE;
	case BlackRook:
		if ((ReplacedPiece >= 224) && (ReplacedPiece <= 334))
			return FALSE; // if ReplacedPiece is black
		if (CheckRookMoves())
			return TRUE;
		else
			return FALSE;
	case WhiteRook:
		if ((ReplacedPiece >= 192) && (ReplacedPiece <= 202))
			return FALSE; // if ReplacedPiece is white
		if (CheckRookMoves())
			return TRUE;
		else
			return FALSE;
	case BlackQueen:
		if ((ReplacedPiece >= 224) && (ReplacedPiece <= 334))
			return FALSE; // if ReplacedPiece is black
		if (CheckQueenMoves())
			return TRUE;
		else
			return FALSE;
	case WhiteQueen:
		if ((ReplacedPiece >= 192) && (ReplacedPiece <= 202))
			return FALSE; // if ReplacedPiece is white
		if (CheckQueenMoves())
			return TRUE;
		else
			return FALSE;
	case BlackKing:
		if ((ReplacedPiece >= 224) && (ReplacedPiece <= 334))
			return FALSE; // if ReplacedPiece is black
		if (!fromblackandwhite) {
			if ((X == 4) && (X2 == 6) && (Y == 0)) {
				for (x = 32; x < pm; x++) {
					if (PieceMoves[x].Piece == BlackKing)
						return FALSE; // if king moved previously
					if ((PieceMoves[x].Piece == BlackRook) && (PieceMoves[x].fromX == 7))
						return FALSE;
				}
				if ((PieceLoc[0][5] == 0) && (ReplacedPiece == 0) && PieceLoc[0][7] == BlackRook) {
					if (CheckForCheck())
						return FALSE;
					if (FALSE == CheckPassedThruSquare(0, 5, BlackKing))
						return FALSE;
					PieceLoc[0][5] = BlackRook;
					PieceLoc[0][7] = 0;
					castled = TRUE;
					return TRUE;
				}
			}
			else if ((X == 4) && (X2 == 2) && (Y == 0)) {
				for (x = 32; x < pm; x++) {
					if (PieceMoves[x].Piece == BlackKing)
						return FALSE; // if king moved previously
					if ((PieceMoves[x].Piece == BlackRook) && (PieceMoves[x].fromX == 0))
						return FALSE;
				}
				if (checktype == Check)
					return FALSE;
				if ((PieceLoc[0][1] == 0) && (PieceLoc[0][3] == 0) && (ReplacedPiece == 0) && PieceLoc[0][0] == 228) {
					if (CheckForCheck())
						return FALSE;
					if (FALSE == CheckPassedThruSquare(0, 3, BlackKing))
						return FALSE;
					PieceLoc[0][3] = BlackRook;
					PieceLoc[0][0] = 0;
					castled = TRUE;
					return TRUE;
				}
			}
		}
		if (CheckKingMoves())
			return TRUE;
		else
			return FALSE;
	case WhiteKing:
		if ((ReplacedPiece >= 192) && (ReplacedPiece <= 202))
			return FALSE; // if ReplacedPiece is white
		if (!fromblackandwhite) {
			if ((X == 4) && (X2 == 6) && (Y == 7)) {
				for (x = 32; x < pm; x++) {
					if (PieceMoves[x].Piece == WhiteKing)
						return FALSE; // if king moved previously
					if ((PieceMoves[x].Piece == WhiteRook) && (PieceMoves[x].fromX == 7))
						return FALSE;
				}
				if (checktype == Check)
					return FALSE;
				if ((PieceLoc[7][5] == 0) && (ReplacedPiece == 0) && (PieceLoc[7][7] == WhiteRook)) {
					if (CheckForCheck())
						return FALSE;
					if (FALSE == CheckPassedThruSquare(7, 5, WhiteKing))
						return FALSE;
					PieceLoc[7][5] = WhiteRook;
					PieceLoc[7][7] = 0;
					castled = TRUE;
					return TRUE;
				}
			}
			else if ((X == 4) && (X2 == 2) && (Y == 7)) {
				for (x = 32; x < pm; x++) {
					if (PieceMoves[x].Piece == WhiteKing)
						return FALSE; // if king moved previously
					if ((PieceMoves[x].Piece == WhiteRook) && (PieceMoves[x].fromX == 7))
						return FALSE;
				}
				if (checktype == Check)
					return FALSE;
				if ((PieceLoc[7][1] == 0) && (ReplacedPiece == 0) && (PieceLoc[7][3] == 0) && (PieceLoc[7][0] == 196)) {
					if (CheckForCheck())
						return FALSE;
					if (FALSE == CheckPassedThruSquare(7, 3, WhiteKing))
						return FALSE;
					PieceLoc[7][3] = WhiteRook;
					PieceLoc[7][0] = 0;
					castled = TRUE;
					return TRUE;
				}
			}
		}
		if (CheckKingMoves())
			return TRUE;
		else
			return FALSE;
	}
	return FALSE;
}

BOOL CheckForDiscovered(int KingX, int KingY, int King, int CheckerX, int CheckerY)
{
	int tempX, tempY, tempPiece;

	X2 = KingX;
	Y2 = KingY;
	ReplacedPiece = King;
	tempPiece = PieceLoc[Y][X];
	tempX = X;
	tempY = Y;
	PieceLoc[Y][X] = 0; // piece that moves to block check
	for (Y = 0; Y < 8; Y++) {
		for (X = 0; X < 8; X++) {
			if (((Y == KingY) && (X == KingX)) || ((Y == CheckerY) && (X == CheckerX)))
				continue;
			MovedPiece = PieceLoc[Y][X];
			if (CheckMove()) { // looking for possible discovered check
				PieceLoc[tempY][tempX] = tempPiece;
				return TRUE;
			}
		}
	}
	PieceLoc[tempY][tempX] = tempPiece;
	return FALSE;
}

BOOL CheckForMate(int CheckerX, int CheckerY, int King)
{
	int x, y, KingX, KingY, CheckingPiece, incX, incY;

	KingX = X2;
	KingY = Y2;
// 1.check to see if king can move
	ReplacedPiece = King;
	PieceLoc[KingY][KingX] = 0; // temp
	for (y = Y2-1; y <= (Y2+1); y++) {
		for (x = X2-1; x <= (X2+1); x++) {
			if ((y >= 0) && (y <= 7) && (x >= 0) && (x <= 7) && ((y != Y2) || (x != X2))) {
				if ((PieceLoc[y][x] == 0) || (PieceLoc[y][x] >= (King+24)) || (PieceLoc[y][x] <= (King-30))) { // if king can move there
					Y2 = y; // King's possible move
					X2 = x; // King's possible move
					for (Y = 0; Y < 8; Y++) { // check for check
						for (X = 0; X < 8; X++) {
							MovedPiece = PieceLoc[Y][X];
							if (MovedPiece) {
								if (CheckMove()) { // if a piece would check the king in that position
									goto keeplooking;
								}
							}
						}
					}
					PieceLoc[KingY][KingX] = King;
					return FALSE; // king wouldn't be checked in that position

keeplooking:	X2 = KingX;
					Y2 = KingY;
				}
			}
		}
	}
	PieceLoc[KingY][KingX] = King;

// 2.check to see if the checking piece can be taken
	X2 = CheckerX;
	Y2 = CheckerY;
	ReplacedPiece = PieceLoc[Y2][X2];
	for (Y = 0; Y < 8; Y++) {
		for (X = 0; X < 8; X++) {
			if (PieceLoc[Y][X]) {
				MovedPiece = PieceLoc[Y][X];
				if (CheckMove()) {
					if ((MovedPiece == WhiteKing) || (MovedPiece == BlackKing)) { // king possibly takes checking piece
						x = X;
						y = Y;
						for (Y = 0; Y < 8; Y++) {
							for (X = 0; X < 8; X++) {
								if (PieceLoc[Y][X]) {
									MovedPiece = PieceLoc[Y][X];
									if ((MovedPiece != WhiteKing) && (MovedPiece != BlackKing)) {
										if (CheckMove()) { // if king would be in check again after taking checking piece
											goto badkingmove;
										}
									}
								}
							}
						}
					}
					else
						return FALSE;
badkingmove:	X = x;
					Y = y;
				}
			}
		}
	}

// 3.check to see if a piece can block check
	X2 = KingX;
	Y2 = KingY;
	CheckingPiece = PieceLoc[CheckerY][CheckerX];
	ReplacedPiece = 0;
	if ((CheckingPiece != WhiteKnight) && (CheckingPiece != BlackKnight) && (CheckingPiece != WhitePawn) && (CheckingPiece != BlackPawn)) {
		diffY = CheckerY - KingY;
		diffX = CheckerX - KingX;
		if (diffY >= 0) incY = 1;
		else incY = -1;
		if (diffX >= 0) incX = 1;
		else incX = -1;
		if ((CheckingPiece == WhiteBishop) || (CheckingPiece == BlackBishop)) {
			for (Y2 = KingY+incY, X2 = KingX+incX; Y2 != CheckerY; Y2 += incY, X2 += incX) {
				for (Y = 0; Y < 8; Y++) {
					for (X = 0; X < 8; X++) {
						if (PieceLoc[Y][X]) {
							if (((Y == KingY) && (X == KingX)) || ((Y == CheckerY) && (X == CheckerX)))
								continue;
							MovedPiece = PieceLoc[Y][X];
							if (((CheckingPiece == WhiteBishop) && (MovedPiece >= 224)) || ((CheckingPiece == BlackBishop) && (MovedPiece < 224))) {
								if (CheckMove()) {
									if (CheckForDiscovered(KingX, KingY, King, CheckerX, CheckerY))
										return TRUE;
									else
										return FALSE;
								}
							}
						}
					}
				}
			}
		}
		else if ((CheckingPiece == WhiteRook) || (CheckingPiece == BlackRook)) {
			if (CheckerY == KingY) {
				for (X2 = KingX+incX; X2 != CheckerX; X2 += incX) {
					for (Y = 0; Y < 8; Y++) {
						for (X = 0; X < 8; X++) {
							if ((PieceLoc[Y][X] != King) && (PieceLoc[Y][X] != CheckingPiece)) {
								if (PieceLoc[Y][X]) {
									MovedPiece = PieceLoc[Y][X];
									if (((CheckingPiece == WhiteRook) && (MovedPiece >= 224)) || ((CheckingPiece == BlackRook) && (MovedPiece < 224))) {
										if (CheckMove()) {
											if (CheckForDiscovered(KingX, KingY, King, CheckerX, CheckerY))
												return TRUE;
											else
												return FALSE;
										}
									}
								}
							}
						}
					}
				}
			}
			else { // if (CheckerX == KingX)
				for (Y2 = KingY+incY; Y2 != CheckerY; Y2 += incY) {
					for (Y = 0; Y < 8; Y++) {
						for (X = 0; X < 8; X++) {
							if ((PieceLoc[Y][X] != King) && (PieceLoc[Y][X] != CheckingPiece)) {
								if (PieceLoc[Y][X]) {
									MovedPiece = PieceLoc[Y][X];
									if (((CheckingPiece == WhiteRook) && (MovedPiece >= 224)) || ((CheckingPiece == BlackRook) && (MovedPiece < 224))) {
										if (CheckMove()) {
											if (CheckForDiscovered(KingX, KingY, King, CheckerX, CheckerY))
												return TRUE;
											else
												return FALSE;
										}
									}
								}
							}
						}
					}
				}
			}
		}
		else if ((CheckingPiece == WhiteQueen) || (CheckingPiece == BlackQueen)) {
			if (CheckerY == KingY) {
				for (X2 = KingX+incX; X2 != CheckerX; X2 += incX) {
					for (Y = 0; Y < 8; Y++) {
						for (X = 0; X < 8; X++) {
							if ((PieceLoc[Y][X] != King) && (PieceLoc[Y][X] != CheckingPiece)) {
								if (PieceLoc[Y][X]) {
									MovedPiece = PieceLoc[Y][X];
									if (((CheckingPiece == WhiteQueen) && (MovedPiece >= 224)) || ((CheckingPiece == BlackQueen) && (MovedPiece < 224))) {
										if (CheckMove()) {
											if (CheckForDiscovered(KingX, KingY, King, CheckerX, CheckerY))
												return TRUE;
											else
												return FALSE;
										}
									}
								}
							}
						}
					}
				}
			}
			else if (CheckerX == KingX) {
				for (Y2 = KingY+incY; Y2 != CheckerY; Y2 += incY) {
					for (Y = 0; Y < 8; Y++) {
						for (X = 0; X < 8; X++) {
							if ((PieceLoc[Y][X] != King) && (PieceLoc[Y][X] != CheckingPiece)) {
								if (PieceLoc[Y][X]) {
									MovedPiece = PieceLoc[Y][X];
									if (((CheckingPiece == WhiteQueen) && (MovedPiece >= 224)) || ((CheckingPiece == BlackQueen) && (MovedPiece < 224))) {
										if (CheckMove()) {
											if (CheckForDiscovered(KingX, KingY, King, CheckerX, CheckerY))
												return TRUE;
											else
												return FALSE;
										}
									}
								}
							}
						}
					}
				}
			}
			else {
				for (Y2 = KingY+incY, X2 = KingX+incX; Y2 != CheckerY; Y2 += incY, X2 += incX) {
					for (Y = 0; Y < 8; Y++) {
						for (X = 0; X < 8; X++) {
							if ((PieceLoc[Y][X] != King) && (PieceLoc[Y][X] != CheckingPiece)) {
								if (PieceLoc[Y][X]) {
									MovedPiece = PieceLoc[Y][X];
									if (((CheckingPiece == WhiteQueen) && (MovedPiece >= 224)) || ((CheckingPiece == BlackQueen) && (MovedPiece < 224))) {
										if (CheckMove()) {
											if (CheckForDiscovered(KingX, KingY, King, CheckerX, CheckerY))
												return TRUE;
											else
												return FALSE;
										}
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return TRUE;
}

int CheckKing(int KingX, int KingY, int King)
{
	int CheckType = 0;

	X2 = KingX;
	Y2 = KingY;
	ReplacedPiece = King;
	for (Y = 0; Y < 8; Y++) { // check for check or discovered check
		for (X = 0; X < 8; X++) {
			if (PieceLoc[Y][X]) {
				MovedPiece = PieceLoc[Y][X];
				if (CheckMove()) { // if that move would cause the king to be in check
					if ((King == WhiteKing) && (WhitesMove)) {
						CheckType = DiscoveredCheck;
						goto checked;
					}
					else if ((King == BlackKing) && (!WhitesMove)) {
						CheckType = DiscoveredCheck;
						goto checked;
					}
					else if ((King == WhiteKing) && (!WhitesMove)) {
						if (CheckForMate(X, Y, King)) {
							CheckType = CheckMate;
							goto checked;
						}
						else {
							CheckType = Check;
							goto checked;
						}
					}
					else if ((King == BlackKing) && (WhitesMove)) {
						if (CheckForMate(X, Y, King)) {
							CheckType = CheckMate;
							goto checked;
						}
						else {
							CheckType = Check;
							goto checked;
						}
					}
				}
			}
		}
	}
checked:;
	ReplacedPiece = King;
	X = SaveX;
	Y = SaveY;
	X2 = SaveX2;
	Y2 = SaveY2;	
	return CheckType;
}

BOOL CheckForCheck(void)
{
	fromcheckcheck = TRUE;
//	SaveMovedPiece = MovedPiece;
//	SaveReplacedPiece = ReplacedPiece;
	SaveX = X;
	SaveY = Y;
	SaveX2 = X2;
	SaveY2 = Y2;	
	for (y = 0; y < 8; y++) {
		for (x = 0; x < 8; x++) {
			if (PieceLoc[y][x] == BlackKing) {
				BlackKingY = y;
				BlackKingX = x;
			}
			if (PieceLoc[y][x] == WhiteKing) {
				WhiteKingY = y;
				WhiteKingX = x;
			}
		}
	}
	if (WhitesMove) {
		returned = CheckKing(WhiteKingX, WhiteKingY, WhiteKing);
		if (returned) {
			return returned;
		}
		returned = CheckKing(BlackKingX, BlackKingY, BlackKing);
		if (returned) {
			return returned;
		}
	}
	else {
		returned = CheckKing(BlackKingX, BlackKingY, BlackKing);
		if (returned) {
			return returned;
		}
		returned = CheckKing(WhiteKingX, WhiteKingY, WhiteKing);
		if (returned) {
			return returned;
		}
	}
	MovedPiece = SaveMovedPiece;
	ReplacedPiece = SaveReplacedPiece;
	X = SaveX;
	Y = SaveY;
	X2 = SaveX2;
	Y2 = SaveY2;
	return 0;
}

void AddFromLoc(void)
{
	SaveX = X;
	SaveY = Y;
	X = x;
	Y = y;
	if (pCheckMoves()) { // if other Knight/Rook could also move there
		X = SaveX;
		Y = SaveY;
		if (X != x) {
			PGN[ptr++] = X + 'a';
			if (WhitesMove)
				PGN2[number].WhitePiece[ptr2++] = X + 'a';
			else
				PGN2[number].BlackPiece[ptr2++] = X + 'a';
		}
		else {
			PGN[ptr++] = '8' - Y;
			if (WhitesMove)
				PGN2[number].WhitePiece[ptr2++] = '8' - Y;
			else
				PGN2[number].BlackPiece[ptr2++] = '8' - Y;
		}
	}
	X = SaveX;
	Y = SaveY;
}


void FillPieceMoves(PieceColor)
{
	PieceMoves[pm].fromX = 0xFF; // flag

	switch(PieceMoves[pm].Piece)
	{
	case BlackRook:
	case WhiteRook:
		pCheckMoves = CheckRookMoves; // pCheckMoves points to CheckRookMoves
		break;
	case BlackKnight:
	case WhiteKnight:
		pCheckMoves = CheckKnightMoves;
		break;
	case BlackBishop:
	case WhiteBishop:
		pCheckMoves = CheckBishopMoves;
		break;
	case BlackQueen:
	case WhiteQueen:
		pCheckMoves = CheckQueenMoves;
		break;
	case BlackKing:
	case WhiteKing:
		pCheckMoves = CheckKingMoves;
		break;
	}
	if (PieceColor == WHITE)
		PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
	else
		PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
	ptr++;
	if (PGN[ptr] == 'x') { // capturing
		if (PieceColor == WHITE)
			PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
		else
			PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
		ptr++;
	}
	else if (PGN[ptr+1] == 'x') { // capturing, with fromX at PGN[ptr]
		if (PieceColor == WHITE) {
			PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
			PGN2[number].WhitePiece[ptr2++] = PGN[ptr+1];
		}
		else {
			PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
			PGN2[number].BlackPiece[ptr2++] = PGN[ptr+1];
		}
		if ((PGN[ptr] >= 'a') && (PGN[ptr] <= 'h')) {
			PieceMoves[pm].fromX = PGN[ptr] - 'a';
			x = PieceMoves[pm].fromX;
			for (y = 0; y < 8; y++) {
				if (PieceLoc[y][x] == PieceMoves[pm].Piece) {
					PieceMoves[pm].fromY = y;
					break;
				}
			}
		}
		else if ((PGN[ptr] >= '1') && (PGN[ptr] <= '8')) {
			PieceMoves[pm].fromY = 8 - (PGN[ptr] - '0');
			y = PieceMoves[pm].fromY;
			for (x = 0; x < 8; x++) {
				if (PieceLoc[y][x] == PieceMoves[pm].Piece) {
					PieceMoves[pm].fromX = x;
					break;
				}
			}
		}
		ptr += 2;
	}
	else if ((PGN[ptr+2] >= '1') && (PGN[ptr+2] <= '8')) { // fromX or from Y is at PGN[ptr] e.g. Nbc3 or N1c3
		if ((PGN[ptr] >= 'a') && (PGN[ptr] <= 'h')) {
			PieceMoves[pm].fromX = PGN[ptr] - 'a';
			x = PieceMoves[pm].fromX;
			for (y = 0; y < 8; y++) {
				if (PieceLoc[y][x] == PieceMoves[pm].Piece) {
					PieceMoves[pm].fromY = y;
					break;
				}
			}
		}
		else if ((PGN[ptr] >= '1') && (PGN[ptr] <= '8')) {
			PieceMoves[pm].fromY = 8 - (PGN[ptr] - '0');
			y = PieceMoves[pm].fromY;
			for (x = 0; x < 8; x++) {
				if (PieceLoc[y][x] == PieceMoves[pm].Piece) {
					PieceMoves[pm].fromX = x;
					break;
				}
			}
		}
		if (PieceColor == WHITE)
			PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
		else
			PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
		ptr++;
	}
	if (PieceColor == WHITE)
		PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
	else
		PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
	PieceMoves[pm].toX = PGN[ptr++] - 'a';
	if (PieceColor == WHITE)
		PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
	else
		PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
	PieceMoves[pm].toY = 8 - (PGN[ptr++] - '0');
	PieceMoves[pm].PieceTaken = PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX];
	if (PieceMoves[pm].fromX == 0xFF) {
		static int x, y;

		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				if (PieceMoves[pm].Piece == PieceLoc[y][x]) {
					X = x;
					Y = y;
					X2 = PieceMoves[pm].toX;
					Y2 = PieceMoves[pm].toY;
					if (pCheckMoves()) {
						PieceMoves[pm].fromX = x;
						PieceMoves[pm].fromY = y;
						break;
					}
				}
			}
		}		
	}
	PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX] = PieceMoves[pm].Piece;
	PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = 0;
}


void GetPieceMoves(BOOL PieceColor)
{
	static int x, y;

	ptr2 = 0;
	if ((PGN[ptr] >= 'a') && (PGN[ptr] <= 'h')) { // it's a pawn
		fromX = 0xFF;
		if (PieceColor == WHITE)
			PieceMoves[pm].Piece = WhitePawn;
		else
			PieceMoves[pm].Piece = BlackPawn;
		if (PGN[ptr+1] == 'x') { // capturing
			PieceMoves[pm].fromX = PGN[ptr] - 'a'; // special case
			if (PieceColor == WHITE) {
				PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
				PGN2[number].WhitePiece[ptr2++] = PGN[ptr+1];
			}
			else {
				PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
				PGN2[number].BlackPiece[ptr2++] = PGN[ptr+1];
			}
			fromX = PieceMoves[pm].fromX; // special case
			ptr += 2;
		}
		if (PieceColor == WHITE)
			PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
		else
			PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
		PieceMoves[pm].toX = PGN[ptr++] - 'a';
		if (PieceColor == WHITE)
			PGN2[number].WhitePiece[ptr2++] = PGN[ptr];
		else
			PGN2[number].BlackPiece[ptr2++] = PGN[ptr];
		PieceMoves[pm].toY = 8 - (PGN[ptr++] - '0');
		PieceMoves[pm].PieceTaken = PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX];
		if (fromX == 0xFF) {
			PieceMoves[pm].fromX = PieceMoves[pm].toX;
			x = PieceMoves[pm].fromX;
			if (PieceColor == WHITE) {
				for (y = (PieceMoves[pm].toY+1); y < 8; y++) {
					if (PieceMoves[pm].Piece == PieceLoc[y][x]) {
						PieceMoves[pm].fromY = y;
						break;
					}
				}
			}
			else { // if (PieceColor == BLACK) {
				for (y = (PieceMoves[pm].toY-1); y > 0; y--) {
					if (PieceMoves[pm].Piece == PieceLoc[y][x]) {
						PieceMoves[pm].fromY = y;
						break;
					}
				}
			}
		}
		else {
			x = fromX;
			PieceMoves[pm].fromX = x;
			for (y = 0; y < 8; y++) {
				if ((PieceMoves[pm].Piece == PieceLoc[y][x]) && (abs(PieceMoves[pm].toY - y) == 1)) {
					PieceMoves[pm].fromY = y;
					break;
				}
			}

			if ((PieceMoves[pm].Piece == BlackPawn)
			 && (PieceMoves[pm].toY == 5)
			 && (PieceMoves[pm-1].Piece == WhitePawn)
			 && ((PieceMoves[pm-1].toX == PieceMoves[pm].fromX-1) || (PieceMoves[pm-1].toX == PieceMoves[pm].fromX+1))
			 && (PieceMoves[pm-1].toY == 4)) {
				PieceLoc[PieceMoves[pm-1].toY][PieceMoves[pm-1].toX] = 0;
			}
			if ((PieceMoves[pm].Piece == WhitePawn)
			 && (PieceMoves[pm].toY == 2)
			 && (PieceMoves[pm-1].Piece == BlackPawn)
			 && ((PieceMoves[pm-1].toX == PieceMoves[pm].fromX-1) || (PieceMoves[pm-1].toX == PieceMoves[pm].fromX+1))
			 && (PieceMoves[pm-1].toY == 3)) {
				PieceLoc[PieceMoves[pm-1].toY][PieceMoves[pm-1].toX] = 0;
			}
		}

		if ((PGN[ptr] != ' ') && (PGN[ptr] != '\r') && (PGN[ptr] != '\n')) { // '+' or '=' or '#'
			if (PGN[ptr] == '=') {
				if (PieceColor == WHITE)
					PGN2[number].WhitePiece[ptr2++] = PGN[ptr++];
				else
					PGN2[number].BlackPiece[ptr2++] = PGN[ptr++];
			}
			if ((PGN[ptr] != ' ') && (PGN[ptr+1] != '\r') && (PGN[ptr+1] != '\n')) { // =Q or =R or =B or =N
				if (PGN[ptr] == 'Q') {
					if (PieceMoves[pm].toY == 7)
						PieceMoves[pm].Piece = BlackQueen;
					else if (PieceMoves[pm].toY == 0)
						PieceMoves[pm].Piece = WhiteQueen;
				}
				else if (PGN[ptr] == 'R') {
					if (PieceMoves[pm].toY == 7)
						PieceMoves[pm].Piece = BlackRook;
					else if (PieceMoves[pm].toY == 0)
						PieceMoves[pm].Piece = WhiteRook;
				}
				else if (PGN[ptr] == 'N') {
					if (PieceMoves[pm].toY == 7)
						PieceMoves[pm].Piece = BlackKnight;
					else if (PieceMoves[pm].toY == 0)
						PieceMoves[pm].Piece = WhiteKnight;
				}
				else if (PGN[ptr] == 'B') {
					if (PieceMoves[pm].toY == 7)
						PieceMoves[pm].Piece = BlackBishop;
					else if (PieceMoves[pm].toY == 0)
						PieceMoves[pm].Piece = WhiteBishop;
				}
			if (PieceColor == WHITE)
				PGN2[number].WhitePiece[ptr2++] = PGN[ptr++];
			else
				PGN2[number].BlackPiece[ptr2++] = PGN[ptr++];
			}
		}
		PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX] = PieceMoves[pm].Piece;
		PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = 0;
	}

	else { // not a pawn
		switch(PGN[ptr])
		{
		case 'R':
			if (PieceColor == WHITE)
				PieceMoves[pm].Piece = WhiteRook;
			else
				PieceMoves[pm].Piece = BlackRook;
			FillPieceMoves(PieceColor);
			break;
		case 'N':
			if (PieceColor == WHITE)
				PieceMoves[pm].Piece = WhiteKnight;
			else
				PieceMoves[pm].Piece = BlackKnight;
			FillPieceMoves(PieceColor);
			break;
		case 'B':
			if (PieceColor == WHITE)
				PieceMoves[pm].Piece = WhiteBishop;
			else
				PieceMoves[pm].Piece = BlackBishop;
			FillPieceMoves(PieceColor);
			break;
		case 'Q':
			if (PieceColor == WHITE)
				PieceMoves[pm].Piece = WhiteQueen;
			else
				PieceMoves[pm].Piece = BlackQueen;
			FillPieceMoves(PieceColor);
			break;
		case 'K':
			if (PieceColor == WHITE)
				PieceMoves[pm].Piece = WhiteKing;
			else
				PieceMoves[pm].Piece = BlackKing;
			FillPieceMoves(PieceColor);
			break;

		case 'O': // castle
			if (PieceColor == WHITE)
				for (y = ptr; (PGN[y] != ' ') && (PGN[y] != '\r') && (PGN[y] != '\n') && y < (int)fileSize; ptr2++, y++)
					PGN2[number].WhitePiece[ptr2] = PGN[y];
			else
				for (y = ptr; (PGN[y] != ' ') && (PGN[y] != '\r') && (PGN[y] != '\n') && y < (int)fileSize; ptr2++, y++)
					PGN2[number].BlackPiece[ptr2] = PGN[y];
			if (PGN[ptr+3] != '-') { // O-O
				ptr += 3;
				PieceMoves[pm].toX = 6;
			}
			else {
				PieceMoves[pm].toX = 2; // O-O-O
				ptr += 4;
			}

			if (PieceColor == WHITE) {
				PieceMoves[pm].Piece = WhiteKing;
				PieceMoves[pm].fromX = 4;
				PieceMoves[pm].fromY = PieceMoves[pm].toY = 7;
				PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX] = PieceMoves[pm].Piece;
				PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = 0;
				pm++;

				PieceMoves[pm].Piece = WhiteRook;
				PieceMoves[pm].fromY = PieceMoves[pm].toY = 7;
				if (PieceMoves[pm-1].toX == 6) {
					PieceMoves[pm].fromX = 7;
					PieceMoves[pm].toX = 5;
				}
				else {
					PieceMoves[pm].fromX = 0;
					PieceMoves[pm].toX = 3;
				}
				PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX] = PieceMoves[pm].Piece;
				PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = 0;
			}
			else {
				PieceMoves[pm].Piece = BlackKing;
				PieceMoves[pm].fromX = 4;
				PieceMoves[pm].fromY = PieceMoves[pm].toY = 0;
				PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX] = PieceMoves[pm].Piece;
				PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = 0;
				pm++;

				PieceMoves[pm].Piece = BlackRook;
				PieceMoves[pm].fromY = PieceMoves[pm].toY = 0;
				if (PieceMoves[pm-1].toX == 6) {
					PieceMoves[pm].fromX = 7;
					PieceMoves[pm].toX = 5;
				}
				else {
					PieceMoves[pm].fromX = 0;
					PieceMoves[pm].toX = 3;
				}
				PieceLoc[PieceMoves[pm].toY][PieceMoves[pm].toX] = PieceMoves[pm].Piece;
				PieceLoc[PieceMoves[pm].fromY][PieceMoves[pm].fromX] = 0;
			}
			break;
		default:
			MessageBox(hwnd, "bug", "", MB_OK);
		}
		if ((PGN[ptr] != ' ') && (PGN[ptr] != 'O') &&(PGN[ptr] != '\r') && (PGN[ptr] != '\n')) { // '+' or '=' or '#'
			if (PieceColor == WHITE)
				PGN2[number].WhitePiece[ptr2++] = PGN[ptr++];
			else
				PGN2[number].BlackPiece[ptr2++] = PGN[ptr++];
			if ((PGN[ptr] != ' ') && (PGN[ptr+1] != '\r') && (PGN[ptr+1] != '\n')) { // =Q or =R or =B or =N
				if (PieceColor == WHITE)
					PGN2[number].WhitePiece[ptr2++] = PGN[ptr++];
				else
					PGN2[number].BlackPiece[ptr2++] = PGN[ptr++];
			}
		}
	}
	pmMax = pm;
	pm++;
}

void PGN2toPgn(void)
{ // for writing to file
	if (rotated) {
		SendMessage(hwnd, WM_COMMAND, ID_ROTATEBOARD, 123);
	}
	p = 0;
	Pgn[p++] = '\r';
	Pgn[p++] = '\n';
	for (n = 1; n <= number; n++) {
		for (y = 0; PGN2[n].Number[y] != 0; y++, p++)
			Pgn[p] = PGN2[n].Number[y];
		Pgn[p++] = ' ';
		for (y = 0; PGN2[n].WhitePiece[y] != 0; y++, p++)
			Pgn[p] = PGN2[n].WhitePiece[y];
		if ((n < number) || (((pm-1) & 1) == 1)) { // if not last line or if last line includes black piece
			Pgn[p++] = ' ';
			for (y = 0; PGN2[n].BlackPiece[y] != 0; y++, p++)
				Pgn[p] = PGN2[n].BlackPiece[y];
		}
		Pgn[p++] = '\r';
		Pgn[p++] = '\n';
	}
}

void GetBlackAndWhiteNums(void)
{
	fromblackandwhite = TRUE;
//	if (!movingPiece) {
		SaveX = X;
		SaveY = Y;
		SaveX2 = X2;
		SaveY2 = Y2;
		SaveMovedPiece = MovedPiece;
		SaveReplacedPiece = ReplacedPiece;
		ReplacedPiece = 0;
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				BlackNums[y][x] = 0;
				WhiteNums[y][x] = 0;
				TempPieceLoc[y][x] = PieceLoc[y][x]; // CheckMove might change it
			}
		}
		for (Y = 0; Y < 8; Y++) {
			for (X = 0; X < 8; X++) {
				for (Y2 = 0; Y2 < 8; Y2++) {
					for (X2 = 0; X2 < 8; X2++) {
						if ((Y == Y2) && (X == X2))
							continue;
						MovedPiece = PieceLoc[Y][X];
						if (CheckMove()) {
							if ((MovedPiece >= 224) && (MovedPiece <= 334)) // black piece
								BlackNums[Y2][X2]++;
							else if ((MovedPiece >= 192) && (MovedPiece <= 202)) // white piece
								WhiteNums[Y2][X2]++;
						}
					}
				}
			}
		}
		MovedPiece = SaveMovedPiece;
		ReplacedPiece = SaveReplacedPiece;
		X = SaveX;
		Y = SaveY;
		X2 = SaveX2;
		Y2 = SaveY2;
		for (y = 0; y < 8; y++) {
			for (x = 0; x < 8; x++) {
				PieceLoc[y][x] = TempPieceLoc[y][x];
			}
		}
//	}
	fromblackandwhite = FALSE;
}

void PlayCheck(void)
{
	if (hFile != INVALID_HANDLE_VALUE) {
		if (fileSize = GetFileSize(hFile, NULL)) {
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
						WaveFormat.nSamplesPerSec = *(DWORD*)&WaveBuf[24];
						WaveFormat.nBlockAlign = *(WORD*)&WaveBuf[32];
						WaveFormat.nAvgBytesPerSec = *(DWORD*)&WaveBuf[28];
						WaveFormat.wBitsPerSample = *(WORD*)&WaveBuf[34];
						WaveFormat.cbSize = 0;
						nChannels = WaveFormat.nChannels;
						nSamplesPerSec = WaveFormat.nSamplesPerSec;
						wBitsPerSample = WaveFormat.wBitsPerSample;

						waveOutOpen(&hWaveOut, WAVE_MAPPER, &WaveFormat, (DWORD)&waveOutProc, 0, CALLBACK_FUNCTION);
						WaveOutHdr.lpData = (LPSTR)&WaveBuf[nextchunk+8];
						WaveOutHdr.dwBufferLength = *(DWORD*)&WaveBuf[nextchunk+4];
						WaveOutHdr.dwBytesRecorded = 0;
						WaveOutHdr.dwUser = 0;
						WaveOutHdr.dwFlags = 0;
						WaveOutHdr.dwLoops = 0;
						waveOutPrepareHeader(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
						waveOutWrite(hWaveOut, &WaveOutHdr, sizeof(WAVEHDR));
					}
				}
			}
		}
	}
}

int CALLBACK PromotionProc(HWND hwndDlg, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_INITDIALOG:
		CheckRadioButton(hwndDlg, IDC_RADIO1, IDC_RADIO4, IDC_RADIO1);
		SetFocus(hwndDlg);
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDOK:
		case IDCANCEL:
			if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO1)) {
				if ((Y2 == 0) && (!rotated))
					PieceLoc[Y2][X2] = WhiteQueen;
				else if ((Y2 == 7) && (!rotated))
					PieceLoc[Y2][X2] = BlackQueen;
				PromotedType = 'Q';
			}
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO2)) {
				if ((Y2 == 0) && (!rotated))
					PieceLoc[Y2][X2] = WhiteRook;
				else if ((Y2 == 7) && (!rotated))
					PieceLoc[Y2][X2] = BlackRook;
				PromotedType = 'R';
			}
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO3)) {
				if ((Y2 == 0) && (!rotated))
					PieceLoc[Y2][X2] = WhiteBishop;
				else if ((Y2 == 7) && (!rotated))
					PieceLoc[Y2][X2] = BlackBishop;
				PromotedType = 'B';
			}
			else if (BST_CHECKED == IsDlgButtonChecked(hwndDlg, IDC_RADIO4)) {
				if ((Y2 == 0) && (!rotated))
					PieceLoc[Y2][X2] = WhiteKnight;
				else if ((Y2 == 7) && (!rotated))
					PieceLoc[Y2][X2] = BlackKnight;
				PromotedType = 'N';
			}
			EndDialog (hwndDlg, TRUE);
			return TRUE;
		}
		break;
	}
	return FALSE;
}

void SetTitle(HWND hwndDlg)
{
	int y, z;

	for (z = 0, y = 0; Filename[y] != 0; z++, y++)
		PGNtitle[z] = Filename[y];
	PGNtitle[z++] = ' ';
	PGNtitle[z++] = ' ';
	_itoa(nPGN, N, 10);
	for (y = 0; N[y] != 0; z++, y++)
		PGNtitle[z] = N[y];
	PGNtitle[z++] = ' ';
	PGNtitle[z++] = '/';
	PGNtitle[z++] = ' ';
	_itoa(Games, TotalGames, 10);
	for (y = 0; TotalGames[y] != 0; z++, y++)
		PGNtitle[z] = TotalGames[y];
	PGNtitle[z] = 0;
	SetWindowText(hwndDlg, PGNtitle);
}
//hwndDescEdit
int CALLBACK PGNfilesProc(HWND hwndPGN, UINT message, WPARAM wParam, LPARAM lParam)
{
	static int x, y, z;
	static char num[8];
	static BOOL frombutton3;
	static HWND hwndEdit, tempHwnd;

	switch (message)
	{
	case WM_INITDIALOG:
		x = SaveX; // because DialogBox messes up the variable x
		frombutton3 = FALSE;
		hwndEdit = GetDlgItem(hwndPGN, IDC_EDIT1);
		hwndEdit2 = GetDlgItem(hwndPGN, IDC_EDIT2);
		pEdit2Proc = (WNDPROC)SetWindowLong(hwndEdit2, GWL_WNDPROC, (LONG)Edit2Proc);
		SendMessage(hwndEdit, WM_SETFONT, (WPARAM)NULL, TRUE); // use default system font
		SetWindowText(hwndEdit, PGNbuf);
		SetTitle(hwndPGN);
		SetFocus(hwndPGN);
		break;
	
	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case IDC_EDIT2: // show game #
			if (exitflag) {
				exitflag = FALSE;
				SetFocus(hwndPGN);
				GetWindowText(hwndEdit2, num, 7);
				frombutton3 = TRUE;
				nPGN = atoi(num);
				if (nPGN > Games) {
					nPGN = Games;
					_itoa(Games, num, 10);
					SetWindowText(hwndEdit2, num);
				}
				else if (nPGN == 0) {
					nPGN = 1;
					SetWindowText(hwndEdit2, "1");
				}
				e = nPGN;
				x = Entry[e];
				goto x4;
			}
			break;

		case IDOK:
			y = strlen(PGNtitle);
			PGNtitle[y++] = ' ';
			PGNtitle[y++] = ' ';
			for (z = x-1; (z > 0) && (*(DWORD*)&PGNfile[z] != 0x6576455B); z--) { // look backwards for "[Result" until "[Event"
				if (PGNfile[z] == '[') {
					if (*(DWORD*)&PGNfile[z+1] == 0x75736552) { // "Resu"
						for (z += 8; PGNfile[z] != ']'; y++, z++)
							PGNtitle[y] = PGNfile[z]; // e.g. "1-0"
						PGNtitle[y] = 0;
						break;
					}
				}
			}
			EndDialog (hwndPGN, TRUE);
			return TRUE;

		case IDCANCEL:
			Filename[0] = 0;
			EndDialog (hwndPGN, FALSE);
			return FALSE;

		case IDC_BUTTON1: // Next
			if (x >= (int)RealfileSize)
				return FALSE;
			nPGN++;
			e++;
x4:		for (y = 0; (x < (int)RealfileSize) && (y < (PGNBUFSIZE-256)) && (e < PGNENTRIES); x++) {
				PGNbuf[y++] = PGNfile[x];
				if (PGNfile[x] == '[') {
					for (x++; (x < (int)RealfileSize) && (PGNfile[x] != ']'); x++, y++)
						PGNbuf[y] = PGNfile[x];
					PGNbuf[y++] = PGNfile[x];
					if (x == (int)RealfileSize) {
						goto x3;
					}
				}
				if ((PGNfile[x] == '.') && (PGNfile[x+1] != '.')) {
					for (x++; x < (int)RealfileSize; x++, y++) {
						PGNbuf[y] = PGNfile[x];
						if ((PGNfile[x] == '[') || (x == (int)RealfileSize)) {
							PGNbuf[y] = 0;
							goto x3;
						}
					}
					if (x >= (int)RealfileSize) {
						PGNbuf[y] = 0;
						goto x3;
					}
				}
			}
			if (x >= (int)RealfileSize) {
				PGNbuf[y] = 0;
				goto x3;
			}
			return FALSE;

		case IDC_BUTTON2: // Previous
			if (nPGN > 1) {
				nPGN--;
				e--;
				x = Entry[e];
				goto x4;
			}
			return FALSE;

		case IDC_BUTTON3: // Go To This Num:
			GetWindowText(hwndEdit2, num, 7);
			frombutton3 = TRUE;
			nPGN = atoi(num);
			if (nPGN > Games) {
				nPGN = Games;
				_itoa(Games, num, 10);
				SetWindowText(hwndEdit2, num);
			}
			else if (nPGN == 0) {
				nPGN = 1;
				SetWindowText(hwndEdit2, "1");
			}
			e = nPGN;
			x = Entry[e];
			goto x4;
		}

x3:	SendMessage(hwndEdit, EM_SETSEL, 0, -1);
		SendMessage(hwndEdit, WM_CLEAR, 0, 0);
		SetWindowText(hwndEdit, PGNbuf);
		SetTitle(hwndPGN);
		return FALSE;
	}
	return FALSE;
}

int CALLBACK PlacePiecesProc(HWND hwndPlacePieces, UINT message, WPARAM wParam, LPARAM lParam)
{
	static BOOL init;

	switch (message)
	{
	case WM_INITDIALOG:
		color = 'B'; // Black Pieces
		CheckRadioButton(hwndPlacePieces, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2); // Black Pieces
		init = TRUE;
		showing = showingnums = FALSE; // so it will have to be re-evaluated
		CheckMenuItem(hMenu, ID_SHOWATTACKERSDEFENDERS, MF_UNCHECKED);
		break;

	case WM_SETFOCUS:
		if (init) {
			init = FALSE;
			SetFocus(hwnd);
		}
		break;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
			case IDC_RADIO1:
				CheckRadioButton(hwndPlacePieces, IDC_RADIO1, IDC_RADIO2, IDC_RADIO1);
				color = 'W';
				SetFocus(hwnd);
				break;
			case IDC_RADIO2:
				CheckRadioButton(hwndPlacePieces, IDC_RADIO1, IDC_RADIO2, IDC_RADIO2);
				color = 'B';
				SetFocus(hwnd);
				break;

			case IDOK: // spacebar
				nosave = TRUE;
				if (IDYES == MessageBox(hwndPlacePieces, "", "White's Move?", MB_YESNO)) {
					WhitesMove = TRUE;
					SetWindowText(hwnd, WhiteMoves);
				}
				else {
					WhitesMove = FALSE;
					SetWindowText(hwnd, BlackMoves);
				}
				for (x = 0; x <= PIECEMOVES; x++) {
					PieceMoves[x].Piece = 0;
					PieceMoves[x].PieceTaken = 0;
					PieceMoves[x].fromX = 0;
					PieceMoves[x].fromY = 0;
					PieceMoves[x].toX = 0;
					PieceMoves[x].toY = 0;
					PieceMoves[x].Note1 = 0xFF;
				}
				for (x = 0; x < PGNSIZE; x++)
					PGN[x] = 0;
				for (x = 0; x < PGN2SIZE; x++) {
					for (y = 0; y < 4; y++)
						PGN2[x].Number[y] = 0;
					for (y = 0; y < 8; y++) {
						PGN2[x].WhitePiece[y] = 0;
						PGN2[x].BlackPiece[y] = 0;
					}
				}
				for (y = 0; y < 8; y++) {
					for (x = 0; x < 8; x++) {
						PieceLoc[y][x] = 0;
					}
				}
				i = j = 0;
				pm = 32;
				for (y = 0; y < 8; y++) {
					for (x = 0; x < 8; x++) {
						PieceLoc[y][x] = Placed[y][x];

						if (PieceLoc[y][x]) {
							pm--;
							PieceMoves[pm].Piece = PieceLoc[y][x];
							PieceMoves[pm].PieceTaken = 0;
							PieceMoves[pm].fromX = i++;
							PieceMoves[pm].toX = x;
							PieceMoves[pm].fromY = j++;
							PieceMoves[pm].toY = y;
							PieceMoves[pm].Note1 = 0xFF;
						}
					}
				}
				pm = 32;
				pmMax = pm-1;
				ptr = ptr2 = 0;
				number = 0;
				hFile = CreateFile("Placed.dta", GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
				WriteFile(hFile, Placed, 64, &dwBytesWritten, NULL);
				CloseHandle(hFile);
				SendMessage(hwndPlacePieces, WM_CLOSE, 0, 0);
				break;

			case IDCANCEL:
				nosave = FALSE;
				SendMessage(hwndPlacePieces, WM_CLOSE, 0, 0);
				break;
		}
		break;

	case WM_CLOSE:
		DestroyWindow(hwndPlacePieces);
		hwndPlacePieces = NULL;
		placepieces = FALSE;
//		showing = showingnums = FALSE; // so it will have to be re-evaluated
//		CheckMenuItem(hMenu, ID_SHOWATTACKERSDEFENDERS, MF_UNCHECKED);
		InvalidateRect(hwnd, &rect, FALSE);
		break;
	}
	return 0;
}
