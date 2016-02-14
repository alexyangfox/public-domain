//http://jdmcox.com/PyramidSolitaire.rc
//http://jdmcox.com/PyramidSolitaire.h (rename to resource.h)
#include <windows.h>
#include "resource.h"

char About[] = "jdmcox.com\n28 Apr 2008";

int x, y, z, picked, Width, Height, Top, Yincrement, NumWidth, NumHeight, xLoc, yLoc, showcard = 0;
int deck, deck1, deck2[24], ptr, deck1left, deck1right, deck2left, deck2right;
int CardLocX[52];
int CardLocY[52];
struct Cards{
	char num;
	BYTE sortednum;
	BYTE suit;
	RECT rect;
} Card[52];
double CardWidth, CardHeight, Space, Center;
char szAppName[] = "PyramidSolitaire";
char TimesNewRoman[] = "Times New Roman";
char Symbol[] = "Symbol";
HWND hwnd, hwndNext;
RECT rect, textRect;
HDC hdc, hdcMem = 0;
PAINTSTRUCT ps;
HBITMAP hBitmap;
HBRUSH hBrush, hGreenBrush, hGrayBrush;
HPEN hPen;
HINSTANCE hInst;
HMENU hMenu;
LOGFONT lf, upsidedownlf, lfSuit, smallSuitlf, smallUpsidedownSuitlf;
HFONT hFont, hUpsidedownFont, hSuitFont, hSmallSuitFont, hSmallUpsidedownSuitFont;
SIZE Size;
SYSTEMTIME st;
FILETIME ft;
ULARGE_INTEGER ul;
//CHOOSEFONT cf;
char Help[] = "\
The object of the game is to remove all the cards by removing\n\
them in pairs of cards that have a combined value of 13.\n\
\n\
Cards have their numeric value, except:\n\
  Ace	=   1\n\
  Jack	=  11	(pairs with a 2)\n\
  Queen	=  12	(pairs with an Ace)\n\
  King	=  13\n\
A King counts as a pair and is removed by itself.\n\
\n\
Click on the deck to show a card.\n\
Two cards from the deck can show at once.\n\
\n\
Click on a card to be paired and removed\n\
that isn't under another card or that's from the deck.\n\
\n\
If you hold down the 2,3,4,5,6,7,8,9,T,J,Q,K,or A key,\n\
those cards in the pyramid will change to a grey color.\n\
";

void Shuffle(void);
void DrawCard(int x);
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int compare(const void *x, const void *y)
{
	if (((struct Cards*)x)->sortednum > ((struct Cards*)y)->sortednum)
		return 1;
	else if (((struct Cards*)x)->sortednum < ((struct Cards*)y)->sortednum)
		return -1;
	else
		return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	MSG          msg;
	WNDCLASS     wndclass;

	hInst = hInstance;

	wndclass.style         = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)CreateSolidBrush(0x008000);
	wndclass.lpszMenuName  = "PYRAMID";
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass (&wndclass))
		return 0;

	hwnd = CreateWindow (szAppName, szAppName,
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL);

	hwndNext = GetWindow(hwnd, GW_HWNDNEXT);

	ShowWindow (hwnd, SW_SHOWMAXIMIZED);
	UpdateWindow (hwnd);
								  
	while (GetMessage (&msg, NULL, 0, 0))
	{
		TranslateMessage (&msg);
		DispatchMessage (&msg);
	}
	return msg.wParam;
}

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
	case WM_SIZE:
		GetClientRect(hwnd, &rect);
		Width = LOWORD(lParam);
		Height = HIWORD(lParam);
		Space = Width / 42;
		CardWidth = ((double)Width - (8 * Space)) / 7;
		CardHeight = CardWidth * 1.32;
//72=-96 36=-48 28=-37 26=-35 24=-32 22=-29 20=-27 18=-24 16=-21 14=-19 12=-16
		if (CardWidth < 20)
		{
			lf.lfHeight = upsidedownlf.lfHeight = smallSuitlf.lfHeight = smallUpsidedownSuitlf.lfHeight = -19;
			lfSuit.lfHeight = -21;
		}
		else if ((CardWidth >= 20) && (CardWidth < 40))
		{
			lf.lfHeight = upsidedownlf.lfHeight = smallSuitlf.lfHeight = smallUpsidedownSuitlf.lfHeight = -21;
			lfSuit.lfHeight = -24;
		}
		else if ((CardWidth >= 40) && (CardWidth < 60))
		{
			lf.lfHeight = upsidedownlf.lfHeight = smallSuitlf.lfHeight = smallUpsidedownSuitlf.lfHeight = -24;
			lfSuit.lfHeight = -27;
		}
		else if ((CardWidth >= 60) && (CardWidth < 80))
		{
			lf.lfHeight = upsidedownlf.lfHeight = smallSuitlf.lfHeight = smallUpsidedownSuitlf.lfHeight = -27;
			lfSuit.lfHeight = -29;
		}
		else if ((CardWidth >= 80) && (CardWidth < 100))
		{
			lf.lfHeight = upsidedownlf.lfHeight = smallSuitlf.lfHeight = smallUpsidedownSuitlf.lfHeight = -29;
			lfSuit.lfHeight = -32;
		}
		else if ((CardWidth >= 100) && (CardWidth < 120))
		{
			lf.lfHeight = upsidedownlf.lfHeight = smallSuitlf.lfHeight = smallUpsidedownSuitlf.lfHeight = -32;
			lfSuit.lfHeight = -35;
		}
		else if ((CardWidth >= 120) && (CardWidth < 140))
		{
			lf.lfHeight = upsidedownlf.lfHeight = smallSuitlf.lfHeight = smallUpsidedownSuitlf.lfHeight = -35;
			lfSuit.lfHeight = -37;
		}
		else if (CardWidth >= 140)
		{
			lf.lfHeight = upsidedownlf.lfHeight = smallSuitlf.lfHeight = smallUpsidedownSuitlf.lfHeight = -37;
			lfSuit.lfHeight = -48;
		}
		DeleteObject(hFont);
		DeleteObject(hUpsidedownFont);
		DeleteObject(hSuitFont);
		DeleteObject(hSmallSuitFont);
		DeleteObject(hSmallUpsidedownSuitFont);
		hFont = CreateFontIndirect(&lf);
		hUpsidedownFont = CreateFontIndirect(&upsidedownlf);
		hSuitFont = CreateFontIndirect(&lfSuit);
		hSmallSuitFont = CreateFontIndirect(&smallSuitlf);
		hSmallUpsidedownSuitFont = CreateFontIndirect(&smallUpsidedownSuitlf);

		Center = (double)Width/2;
		Top = (int)(Height/4);
		Yincrement = (Height - (int)CardHeight - Top) / 6;
		CardLocX[0] = (int)(Center - (CardWidth/2));
		CardLocY[0] = Top;
		CardLocX[1] = (int)(Center - (Space/2) - CardWidth);
		CardLocY[1] = Top + Yincrement;
		CardLocX[2] = (int)(Center + (Space/2));
		CardLocY[2] = Top + Yincrement;
		CardLocX[3] = (int)(Center - (CardWidth/2) - Space - CardWidth);
		CardLocY[3] = Top + (Yincrement*2);
		CardLocX[4] = (int)(Center - (CardWidth/2));
		CardLocY[4] = Top + (Yincrement*2);
		CardLocX[5] = (int)(Center + (CardWidth/2) + Space);
		CardLocY[5] = Top + (Yincrement*2);
		CardLocX[6] = (int)(Center - (Space/2) - CardWidth - Space - CardWidth);
		CardLocY[6] = Top + (Yincrement*3);
		CardLocX[7] = (int)(Center - (Space/2) - CardWidth);
		CardLocY[7] = Top + (Yincrement*3);
		CardLocX[8] = (int)(Center + (Space/2));
		CardLocY[8] = Top + (Yincrement*3);
		CardLocX[9] = (int)(Center + (Space/2) + CardWidth + Space);
		CardLocY[9] = Top + (Yincrement*3);
		CardLocX[10] = (int)(Center - (CardWidth/2) - Space - CardWidth - Space - CardWidth);
		CardLocY[10] = Top + (Yincrement*4);
		CardLocX[11] = (int)(Center - (CardWidth/2) - Space - CardWidth);
		CardLocY[11] = Top + (Yincrement*4);
		CardLocX[12] = (int)(Center - (CardWidth/2));
		CardLocY[12] = Top + (Yincrement*4);
		CardLocX[13] = (int)(Center + (CardWidth/2) + Space);
		CardLocY[13] = Top + (Yincrement*4);
		CardLocX[14] = (int)(Center + (CardWidth/2) + Space + CardWidth + Space);
		CardLocY[14] = Top + (Yincrement*4);
		CardLocX[15] = (int)(Center - (Space/2) - CardWidth - Space - CardWidth - Space - CardWidth);
		CardLocY[15] = Top + (Yincrement*5);
		CardLocX[16] = (int)(Center - (Space/2) - CardWidth - Space - CardWidth);
		CardLocY[16] = Top + (Yincrement*5);
		CardLocX[17] = (int)(Center - (Space/2) - CardWidth);
		CardLocY[17] = Top + (Yincrement*5);
		CardLocX[18] = (int)(Center + (Space/2));
		CardLocY[18] = Top + (Yincrement*5);
		CardLocX[19] = (int)(Center + (Space/2) + CardWidth + Space);
		CardLocY[19] = Top + (Yincrement*5);
		CardLocX[20] = (int)(Center + (Space/2) + CardWidth + Space + CardWidth + Space);
		CardLocY[20] = Top + (Yincrement*5);
		CardLocX[21] = (int)(Center - (CardWidth/2) - Space - CardWidth - Space - CardWidth - Space - CardWidth);
		CardLocY[21] = Top + (Yincrement*6);
		CardLocX[22] = (int)(Center - (CardWidth/2) - Space - CardWidth - Space - CardWidth);
		CardLocY[22] = Top + (Yincrement*6);
		CardLocX[23] = (int)(Center - (CardWidth/2) - Space - CardWidth);
		CardLocY[23] = Top + (Yincrement*6);
		CardLocX[24] = (int)(Center - (CardWidth/2));
		CardLocY[24] = Top + (Yincrement*6);
		CardLocX[25] = (int)(Center + (CardWidth/2) + Space);
		CardLocY[25] = Top + (Yincrement*6);
		CardLocX[26] = (int)(Center + (CardWidth/2) + Space + CardWidth + Space);
		CardLocY[26] = Top + (Yincrement*6);
		CardLocX[27] = (int)(Center + (CardWidth/2) + Space + CardWidth + Space + CardWidth + Space);
		CardLocY[27] = Top + (Yincrement*6);
		break;

	case WM_CREATE:
		lf.lfEscapement = lf.lfOrientation = 0;
		lf.lfWeight = 700;
		lf.lfItalic = 0;
		lf.lfUnderline = 0;
		lf.lfStrikeOut = 0;
		lf.lfCharSet = 0;
		lf.lfOutPrecision = 3;//1;
		lf.lfClipPrecision = 2;
		lf.lfQuality = 1;
		lf.lfPitchAndFamily = 34;//0x22;
		for (x = 0; TimesNewRoman[x] != 0; x++)
			lf.lfFaceName[x] = TimesNewRoman[x];
		lf.lfFaceName[x] = 0;

		upsidedownlf.lfEscapement = upsidedownlf.lfOrientation = 1800;
		upsidedownlf.lfWeight = 700;
		upsidedownlf.lfItalic = 0;
		upsidedownlf.lfUnderline = 0;
		upsidedownlf.lfStrikeOut = 0;
		upsidedownlf.lfCharSet = 0;
		upsidedownlf.lfOutPrecision = 3;//1;
		upsidedownlf.lfClipPrecision = 2;
		upsidedownlf.lfQuality = 1;
		upsidedownlf.lfPitchAndFamily = 34;//0x22;
		for (x = 0; TimesNewRoman[x] != 0; x++)
			upsidedownlf.lfFaceName[x] = TimesNewRoman[x];
		upsidedownlf.lfFaceName[x] = 0;

		lfSuit.lfEscapement = lfSuit.lfOrientation = 0;
		lfSuit.lfWeight = 400;
		lfSuit.lfItalic = 0;
		lfSuit.lfUnderline = 0;
		lfSuit.lfStrikeOut = 0;
		lfSuit.lfCharSet = 0;
		lfSuit.lfOutPrecision = 1;
		lfSuit.lfClipPrecision = 2;
		lfSuit.lfQuality = 1;
		lfSuit.lfPitchAndFamily = 0x22;
		for (x = 0; Symbol[x] != 0; x++)
			lfSuit.lfFaceName[x] = Symbol[x];
		lfSuit.lfFaceName[x] = 0;

		smallSuitlf.lfEscapement = smallSuitlf.lfOrientation = 0;
		smallSuitlf.lfWeight = 400;
		smallSuitlf.lfItalic = 0;
		smallSuitlf.lfUnderline = 0;
		smallSuitlf.lfStrikeOut = 0;
		smallSuitlf.lfCharSet = 0;
		smallSuitlf.lfOutPrecision = 1;
		smallSuitlf.lfClipPrecision = 2;
		smallSuitlf.lfQuality = 1;
		smallSuitlf.lfPitchAndFamily = 0x22;
		for (x = 0; Symbol[x] != 0; x++)
			smallSuitlf.lfFaceName[x] = Symbol[x];
		smallSuitlf.lfFaceName[x] = 0;

		smallUpsidedownSuitlf.lfEscapement = smallUpsidedownSuitlf.lfOrientation = 1800;
		smallUpsidedownSuitlf.lfWeight = 400;
		smallUpsidedownSuitlf.lfItalic = 0;
		smallUpsidedownSuitlf.lfUnderline = 0;
		smallUpsidedownSuitlf.lfStrikeOut = 0;
		smallUpsidedownSuitlf.lfCharSet = 0;
		smallUpsidedownSuitlf.lfOutPrecision = 1;
		smallUpsidedownSuitlf.lfClipPrecision = 2;
		smallUpsidedownSuitlf.lfQuality = 1;
		smallUpsidedownSuitlf.lfPitchAndFamily = 0x22;
		for (x = 0; Symbol[x] != 0; x++)
			smallUpsidedownSuitlf.lfFaceName[x] = Symbol[x];
		smallUpsidedownSuitlf.lfFaceName[x] = 0;

		hFont = CreateFontIndirect(&lf);
		hUpsidedownFont = CreateFontIndirect(&upsidedownlf);
		hSuitFont = CreateFontIndirect(&lfSuit);
		hSmallSuitFont = CreateFontIndirect(&smallSuitlf);
		hSmallUpsidedownSuitFont = CreateFontIndirect(&smallUpsidedownSuitlf);
/*
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
		ChooseFont(&cf);
*/
		hBrush = CreateSolidBrush(0xFFFFFF);
		hGreenBrush = CreateSolidBrush(0x008000);//green
		hGrayBrush = CreateSolidBrush(0xE0E0E0);
		hPen = CreatePen(PS_SOLID, 1, 0);

		Shuffle();
		return 0;

	case WM_COMMAND:
		switch (LOWORD(wParam))
		{
		case ID_EXIT:
			SendMessage(hwnd, WM_DESTROY, 0, 0);
			break;

		case ID_PLAYANOTHER:
			Shuffle();
			InvalidateRect(hwnd, &rect, FALSE);
			break;

		case IDHELP:
			MessageBox(hwnd, Help, szAppName, MB_OK);
			break;

		case ID_ABOUT:
			MessageBox(hwnd, About, "PyramidSolitaire", MB_OK);
			break;
		}
		break;

//            0
//          1  2
//        3  4  5
//      6  7  8  9
//    10 11 12 13 14
//  15 16 17 18 19 20
//21 22 23 24 25 26 27
	case WM_LBUTTONDOWN:
		xLoc = LOWORD(lParam);
		yLoc = HIWORD(lParam);
		deck1left = (int)((Space*2)+CardWidth);
		deck1right = (int)((Space*2)+(CardWidth*2));
		deck2left =  (int)((Space*3)+(CardWidth*2));
		deck2right = (int)((Space*3)+(CardWidth*3));
		if ((yLoc <= (int)(Space+CardHeight)) && (yLoc >= (int)Space) && (xLoc >= (int)Space) && (xLoc <= (int)(Space+CardWidth)))
		{//deck
			if (deck1 != -1)
			{
				ptr++;
				deck2[ptr] = deck1;
			}
			while ((deck < 52) && (Card[deck].num == 0))
				deck++;
			deck1 = deck;
			if (deck != 52)
				deck++;
			else
			{//put cards back in deck
				deck = 28;
				deck1 = ptr = -1;
			}
			InvalidateRect(hwnd, &rect, FALSE);
		}
		else if ((yLoc <= (int)(Space+CardHeight)) && (yLoc >= (int)Space) && (xLoc >= deck1left) && (xLoc <= deck1right))
		{
			if (deck1 != -1)
			{
				if (picked == -1)
				{
					if (Card[deck1].sortednum == 13)//King
					{
						Card[deck1].num = 0;
						deck1 = -1;
					}
					else
						picked = deck1;
				}
				else
				{
					if ((Card[picked].sortednum + Card[deck1].sortednum) == 13)
					{
						Card[picked].num = 0;
						Card[deck1].num = 0;
						deck1 = -1;
					}
					if (picked == deck2[ptr])
					{
						deck2[ptr] = -1;
						ptr--;
					}
					picked = -1;
				}

				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		else if ((yLoc <= (int)(Space+CardHeight)) && (yLoc >= (int)Space) && (xLoc >= deck2left) && (xLoc <= deck2right))
		{
			if (deck2[ptr] != -1)
			{

				if (picked == -1)
				{
					if (Card[deck2[ptr]].sortednum == 13)//King
					{
						Card[deck2[ptr]].num = 0;
						deck2[ptr] = -1;
						ptr--;
					}
					else
						picked = deck2[ptr];
				}
				else
				{
					if ((Card[picked].sortednum + Card[deck2[ptr]].sortednum) == 13)
					{
						Card[picked].num = 0;
						Card[deck2[ptr]].num = 0;
						deck2[ptr] = -1;
						ptr--;
					}
					if (picked == deck1)
						deck1 = -1;
					picked = -1;
				}

				InvalidateRect(hwnd, &rect, FALSE);
			}
		}
		else
		{
			for (x = 27; x >= 0; x--)
			{
				if ((Card[x].num != 0) && (Card[x].rect.top <= yLoc) && (Card[x].rect.bottom >= yLoc) && (Card[x].rect.left <= xLoc) && (Card[x].rect.right >= xLoc))
				{
					BOOL good = TRUE;

					switch (x)
					{
						case 0:
							if ((Card[1].num != 0) || (Card[2].num != 0))
								good = FALSE;
							break;
						case 1:
							if ((Card[3].num != 0) || (Card[4].num != 0))
								good = FALSE;
							break;
						case 2:
							if ((Card[4].num != 0) || (Card[5].num != 0))
								good = FALSE;
							break;
						case 3:
							if ((Card[6].num != 0) || (Card[7].num != 0))
								good = FALSE;
							break;
						case 4:
							if ((Card[7].num != 0) || (Card[8].num != 0))
								good = FALSE;
							break;
						case 5:
							if ((Card[8].num != 0) || (Card[9].num != 0))
								good = FALSE;
							break;
						case 6:
							if ((Card[10].num != 0) || (Card[11].num != 0))
								good = FALSE;
							break;
						case 7:
							if ((Card[11].num != 0) || (Card[12].num != 0))
								good = FALSE;
							break;
						case 8:
							if ((Card[12].num != 0) || (Card[13].num != 0))
								good = FALSE;
							break;
						case 9:
							if ((Card[13].num != 0) || (Card[14].num != 0))
								good = FALSE;
							break;
						case 10:
							if ((Card[15].num != 0) || (Card[16].num != 0))
								good = FALSE;
							break;
						case 11:
							if ((Card[16].num != 0) || (Card[17].num != 0))
								good = FALSE;
							break;
						case 12:
							if ((Card[17].num != 0) || (Card[18].num != 0))
								good = FALSE;
							break;
						case 13:
							if ((Card[18].num != 0) || (Card[19].num != 0))
								good = FALSE;
							break;
						case 14:
							if ((Card[19].num != 0) || (Card[20].num != 0))
								good = FALSE;
							break;
						case 15:
							if ((Card[21].num != 0) || (Card[22].num != 0))
								good = FALSE;
							break;
						case 16:
							if ((Card[22].num != 0) || (Card[23].num != 0))
								good = FALSE;
							break;
						case 17:
							if ((Card[23].num != 0) || (Card[24].num != 0))
								good = FALSE;
							break;
						case 18:
							if ((Card[24].num != 0) || (Card[25].num != 0))
								good = FALSE;
							break;
						case 19:
							if ((Card[25].num != 0) || (Card[26].num != 0))
								good = FALSE;
							break;
						case 20:
							if ((Card[26].num != 0) || (Card[27].num != 0))
								good = FALSE;
							break;
						case 21:
						case 22:
						case 23:
						case 24:
						case 25:
						case 26:
						case 27:
						break;
					}
					if (good)
					{
						if (picked == -1)
						{
							if (Card[x].sortednum == 13)//King
							{
								Card[x].num = 0;
								if (x == deck2[ptr])
								{
									deck2[ptr] = -1;
									ptr--;
								}
							}
							else
								picked = x;
						}
						else
						{
							if ((Card[picked].sortednum + Card[x].sortednum) == 13)
							{
								Card[picked].num = 0;
								Card[x].num = 0;
								if (picked == deck1)
									deck1 = -1;
								else if (picked == deck2[ptr])
								{
									deck2[ptr] = -1;
									ptr--;
								}
								}
							picked = -1;
						}
						InvalidateRect(hwnd, &rect, FALSE);
					}
					break;//out of for loop
				}
			} 
		}
		break;

	case WM_KEYUP:
		showcard = 0;
		InvalidateRect(hwnd, &rect, FALSE);
		break;

	case WM_KEYDOWN:
		switch (wParam)
		{
		case '2':
			showcard = 2;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '3':
			showcard = 3;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '4':
			showcard = 4;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '5':
			showcard = 5;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '6':
			showcard = 6;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '7':
			showcard = 7;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '8':
			showcard = 8;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '9':
			showcard = 9;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case '1':
			showcard = 10;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'T':
			showcard = 10;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'J':
			showcard = 11;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'Q':
			showcard = 12;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'K':
			showcard = 13;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		case 'A':
			showcard = 1;
			InvalidateRect(hwnd, &rect, FALSE);
			break;
		}
		break;

	case WM_PAINT:
		hdc = BeginPaint (hwnd, &ps);
		if (hdcMem)
		{
			DeleteDC (hdcMem);
			DeleteObject (hBitmap);
		}
		hdcMem = CreateCompatibleDC (hdc);
		hBitmap = CreateCompatibleBitmap (hdc, Width, Height);
		SelectObject (hdcMem, hBitmap);
		SelectObject(hdcMem, hGreenBrush);
		FillRect(hdcMem, &rect, hGreenBrush);
		SelectObject(hdcMem, hPen);
		SetBkMode(hdcMem, TRANSPARENT);
//SetBkColor(hdcMem, 0x808080);
		SelectObject(hdcMem, hGrayBrush);
		RoundRect(hdcMem, (int)Space, (int)Space, (int)(CardWidth+Space), (int)(CardHeight+Space), 20, 20);
		SelectObject(hdcMem, hFont);
		SetTextColor(hdcMem, 0);
		GetTextExtentPoint32(hdcMem, "W", 1, &Size);
		NumWidth = Size.cx;
		textRect.left = (int) Space;
		textRect.top = (int)((CardHeight+Space)/2)-Size.cy;
		textRect.right = (int)(CardWidth+Space);
		textRect.bottom = (int)(CardHeight+Space);
		DrawText(hdcMem, "The", 3, &textRect, DT_CENTER);
		textRect.top = (int)((CardHeight+Space)/2);
		DrawText(hdcMem, "Deck", 4, &textRect, DT_CENTER);
		SelectObject(hdcMem, hBrush);
		if ((Card[deck1].num) && (deck1 != -1))
		{
			if (deck1 == picked)
			{
				SelectObject(hdcMem, hGrayBrush);
				RoundRect(hdcMem, (int)CardWidth+(int)Space*2, (int)Space, (int)(CardWidth*2)+(int)Space*2, (int)(CardHeight+Space), 20, 20);
				SelectObject(hdcMem, hBrush);
			}
			else
				RoundRect(hdcMem, (int)CardWidth+(int)Space*2, (int)Space, (int)(CardWidth*2)+(int)Space*2, (int)(CardHeight+Space), 20, 20);
			CardLocX[deck1] = (int)((Space*2)+CardWidth);
			CardLocY[deck1] = (int)Space;
			////////////////
			DrawCard(deck1);
			////////////////
		}
		if ((ptr != -1) && (Card[deck2[ptr]].num) && (deck2[ptr] != -1))
		{
			if (deck2[ptr] == picked)
			{
				SelectObject(hdcMem, hGrayBrush);
				RoundRect(hdcMem, (int)CardWidth*2+(int)Space*3, (int)Space, (int)CardWidth*3+(int)Space*3, (int)(CardHeight+Space), 20, 20);
				SelectObject(hdcMem, hBrush);
			}
			else
				RoundRect(hdcMem, (int)CardWidth*2+(int)Space*3, (int)Space, (int)CardWidth*3+(int)Space*3, (int)(CardHeight+Space), 20, 20);
			CardLocX[deck2[ptr]] = (int)((Space*3)+(CardWidth*2));
			CardLocY[deck2[ptr]] = (int)Space;
			////////////////
			DrawCard(deck2[ptr]);
			////////////////
		}

		for (x = 0; x < 28; x++)
		{
			if (Card[x].num)
			{
				Card[x].rect.left = CardLocX[x];
				Card[x].rect.top = CardLocY[x];
				Card[x].rect.right = CardLocX[x]+(int)CardWidth;
				Card[x].rect.bottom = CardLocY[x]+(int)CardHeight;
				if ((picked == x) || (showcard == Card[x].sortednum))
				{
					SelectObject(hdcMem, hGrayBrush);
					RoundRect(hdcMem, CardLocX[x], CardLocY[x], CardLocX[x]+(int)CardWidth, CardLocY[x]+(int)CardHeight, 20, 20);
					SelectObject(hdcMem, hBrush);
				}
				else
					RoundRect(hdcMem, CardLocX[x], CardLocY[x], CardLocX[x]+(int)CardWidth, CardLocY[x]+(int)CardHeight, 20, 20);
				////////////
				DrawCard(x);
				////////////
			}
		}

		BitBlt(hdc, 0, 0, Width, Height, hdcMem, 0, 0, SRCCOPY);
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		DeleteDC (hdcMem);
		DeleteObject (hBitmap);
		PostQuitMessage(0);
		SetForegroundWindow(hwndNext);
		return 0;
	}
	return DefWindowProc (hwnd, message, wParam, lParam);
}

void Shuffle(void)
{
	GetSystemTime(&st);
	SystemTimeToFileTime(&st, &ft);
	ul.LowPart = ft.dwLowDateTime;
	ul.HighPart = ft.dwHighDateTime;
	ul.QuadPart /= 10000;//because low 4 digits are 0's
	srand(ul.LowPart);
	srand(rand());
	for (x = 0; x < 52; x++)
	{
		Card[x].num = x;
		Card[x].sortednum = rand();
	}
	qsort(Card, 52, sizeof(struct Cards), compare);
	for (x = 0; x < 52; x++)
	{
		if (Card[x].num < 13)
			Card[x].suit = 0xA7;
		else if ((Card[x].num >=13) && (Card[x].num < 26))
			Card[x].suit = 0xA8;
		else if ((Card[x].num >=26) && (Card[x].num < 39))
			Card[x].suit = 0xA9;
		else if ((Card[x].num >=39) && (Card[x].num < 52))
			Card[x].suit = 0xAA;
		Card[x].num %= 13;
		Card[x].num += '2';
		if (Card[x].num == 0x3A)//one more than '9'
		{
			Card[x].num = '1';
			Card[x].sortednum = 10;
		}
		else if (Card[x].num == 0x3B)
		{
			Card[x].num = 'J';
			Card[x].sortednum = 11;
		}
		else if (Card[x].num == 0x3C)
		{
			Card[x].num = 'Q';
			Card[x].sortednum = 12;
		}
		else if (Card[x].num == 0x3D)
		{
			Card[x].num = 'K';
			Card[x].sortednum = 13;
		}
		else if (Card[x].num == 0x3E)
		{
			Card[x].num = 'A';
			Card[x].sortednum = 1;
		}
		else
			Card[x].sortednum = Card[x].num - '0';
	}
	deck = 28;
	deck1 = -1;
	ptr = -1;//will increment to 0
	picked = -1;
}

void DrawCard(int x)
{
	SelectObject(hdcMem, hFont);
	if ((Card[x].suit == 0xA8) || (Card[x].suit == 0xA9))
		SetTextColor(hdcMem, 0x0000F0);
	else
		SetTextColor(hdcMem, 0);
	if (Card[x].num == '1')
	{
		GetTextExtentPoint32(hdcMem, "10", 2, &Size);
		TextOut(hdcMem, CardLocX[x]+((int)(CardWidth/10))+5-(Size.cx/2), CardLocY[x]+((int)(CardHeight/10))+5-(Size.cy/2), "10", 2);
	}
	else
	{
		GetTextExtentPoint32(hdcMem, &Card[x].num, 1, &Size);
		TextOut(hdcMem, CardLocX[x]+((int)(CardWidth/10))+5-(Size.cx/2), CardLocY[x]+((int)(CardHeight/10))+5-(Size.cy/2), &Card[x].num, 1);
	}
	NumHeight = Size.cy;
	SelectObject(hdcMem, hUpsidedownFont);
	if (Card[x].num == '1')
		TextOut(hdcMem, CardLocX[x]+(int)CardWidth-((int)(CardWidth/10))-5+(Size.cx/2), CardLocY[x]+(int)CardHeight-2, "10", 2);
	else
		TextOut(hdcMem, CardLocX[x]+(int)CardWidth-((int)(CardWidth/10))-5+(Size.cx/2), CardLocY[x]+(int)CardHeight-2, &Card[x].num, 1);
	if ((Card[x].suit == 0xA8) || (Card[x].suit == 0xA9))
		SetTextColor(hdcMem, 0x0000F0);
	SelectObject(hdcMem, hSmallSuitFont);
	GetTextExtentPoint32(hdcMem, &Card[x].suit, 1, &Size);
	TextOut(hdcMem, CardLocX[x]+((int)(CardWidth/10))+5-(Size.cx/2), CardLocY[x]+6+(NumHeight/2), &Card[x].suit, 1);
	SelectObject(hdcMem, hSmallUpsidedownSuitFont);
	TextOut(hdcMem, CardLocX[x]+(int)CardWidth-((int)(CardWidth/10))-5+(Size.cx/2), CardLocY[x]+(int)CardHeight-(NumHeight/2), &Card[x].suit, 1);

	SelectObject(hdcMem, hSuitFont);
	GetTextExtentPoint32(hdcMem, &Card[x].suit, 1, &Size);

	if (CardWidth < 110)
		return;
	y = (int)(CardHeight-(double)Size.cy);
//z = 6;
//Top = y/z;
//for (y = 0; y <= z; y++)
//	TextOut(hdcMem, CardLocX[x], CardLocY[x]+(y*Top), &Card[x].suit, 1);
	switch (Card[x].num)
	{
	case '2':
		Top = y/4;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+(4*Top), &Card[x].suit, 1);
		break;
	case '3':
		Top = y/4;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+(4*Top), &Card[x].suit, 1);
		break;
	case '4':
		Top = y/4;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(4*Top), &Card[x].suit, 1);
		break;
	case '5':
		Top = y/4;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(4*Top), &Card[x].suit, 1);
		break;
	case '6':
		Top = y/4;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(4*Top), &Card[x].suit, 1);
		break;
	case '7':
		Top = y/4;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+Top, &Card[x].suit, 1);
		break;
	case '8':
		Top = y/3;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+Top, &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+Top, &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(3*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(3*Top), &Card[x].suit, 1);
		break;
	case '9':
		Top = y/4;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+Top, &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+Top, &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(3*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(3*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+(2*Top), &Card[x].suit, 1);
		break;
	case '1':
		Top = y/6;
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x], &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(2*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(4*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/4), CardLocY[x]+(6*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth*3/4)-Size.cx, CardLocY[x]+(6*Top), &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+Top, &Card[x].suit, 1);
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+(5*Top), &Card[x].suit, 1);
		break;
	case 'J':
		MoveToEx(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(Size.cy/2), NULL);
		LineTo(hdcMem, CardLocX[x]+(int)CardWidth-NumWidth, CardLocY[x]+(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+(int)CardWidth-NumWidth, CardLocY[x]+(int)CardHeight-(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(int)CardHeight-(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(Size.cy/2));
		break;
	case 'Q':
		MoveToEx(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(Size.cy/2), NULL);
		LineTo(hdcMem, CardLocX[x]+(int)CardWidth-NumWidth, CardLocY[x]+(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+(int)CardWidth-NumWidth, CardLocY[x]+(int)CardHeight-(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(int)CardHeight-(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(Size.cy/2));
		break;
	case 'K':
		MoveToEx(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(Size.cy/2), NULL);
		LineTo(hdcMem, CardLocX[x]+(int)CardWidth-NumWidth, CardLocY[x]+(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+(int)CardWidth-NumWidth, CardLocY[x]+(int)CardHeight-(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(int)CardHeight-(Size.cy/2));
		LineTo(hdcMem, CardLocX[x]+NumWidth, CardLocY[x]+(Size.cy/2));
		break;
	case 'A':
		TextOut(hdcMem, CardLocX[x]+(int)(CardWidth/2)-(Size.cx/2), CardLocY[x]+(int)(CardHeight/2)-(Size.cy/2), &Card[x].suit, 1);
		break;
	}
}
