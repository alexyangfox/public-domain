#include <windows.h>//put msimg32.lib in Project/Settings/Link for TransparentBlt
#include <mmsystem.h>// add winmm.lib to Project -Settings -Link
#include <math.h>//for modf
#define BACKGROUND 0xD0D0D0
#define BLACK 0
#define WHITE 0xFFFFFF
#define PADDLE 0x00FFFF
#define PADDLEWIDTH 180
#define BALL_DIAMETER 35
#define DIRECTION 1
#define SPACE 1
#define d 0.70710678118655// square root of 0.5 (45 degree angle)
#define STICKS 31
#define CHINESESYMBOL 52
#define RIDECYMBAL 59
#define CLAVES 75
#define HIGHWOODBLOCK 76

char Version[] = "Brokeout\nVersion 1.3\nJul 16, 2010\nDoug Cox\njdmcox.com/";
int Timers, cxScreen, cyScreen, PaddleLeft, PaddleTop, PaddleBottom, OldPaddle, NewPaddle, PaddleMoves = 2;
int v, w, x, y, z, xBall, yBall, xBallCenter, yBallCenter, xDirection, yDirection, BlockWidth, BlockHeight, unit, brush;
double d2, d3;
struct
{
	int left;
	int right;
	int top;
	int bottom;
} Block[75];//5*15
BYTE red, green, blue, PercussionNote;
BOOL notfirst = FALSE, first = TRUE, goingleft, goingright, inred = TRUE, ingreen = FALSE, inblue = FALSE;
HWND hwnd;
HPEN hPen, hBackgroundPen, hColor;
HBRUSH hPaddleBrush, hBackgroundBrush, hWhiteBrush, hBlockBrush[5];
RECT rect;
HBITMAP hBitmap, hBitmap2;
HDC hdc, hdcMem, hdcMem2;
PAINTSTRUCT ps;
SYSTEMTIME st;
FILETIME ft;
ULARGE_INTEGER ul;
HMIDIOUT hMidiOut;

LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		ShowCursor(FALSE);
		hBackgroundPen = CreatePen(PS_SOLID, 0, BACKGROUND);
		hPen = CreatePen(PS_SOLID, 0, BLACK);
		hPaddleBrush = CreateSolidBrush(PADDLE);
		hWhiteBrush = CreateSolidBrush(WHITE);
		hBackgroundBrush = CreateSolidBrush(BACKGROUND);
		hBlockBrush[0] = CreateSolidBrush(0x000F0);
		hBlockBrush[1] = CreateSolidBrush(0x00F0F0);
		hBlockBrush[2] = CreateSolidBrush(0x00F000);
		hBlockBrush[3] = CreateSolidBrush(0xF0F000);
		hBlockBrush[4] = CreateSolidBrush(0xF00000);

		midiOutSetVolume(0, 0xFFFFFFFF);// sets SW Synth Volume in both stereo channels
		midiOutOpen(&hMidiOut, MIDIMAPPER, 0, 0, 0);
		midiOutShortMsg(hMidiOut, 0xC9);
		midiOutShortMsg(hMidiOut, 0xB9 | (7 << 8) | (127 << 16));// MIDI Volume Control
		return 0;

	case WM_SIZE:
		cxScreen = LOWORD(lParam);
		cyScreen = HIWORD(lParam);
		rect.left = rect.top = 0;
		rect.right = cxScreen;
		rect.bottom = cyScreen;
		MoveWindow(hwnd, 0, 0, cxScreen, cyScreen, TRUE);
		return 0;
		
	case WM_TIMER:
		InvalidateRect(hwnd, &rect, FALSE);
		UpdateWindow(hwnd);
		return 0;

	case WM_KEYDOWN:
		if (wParam == VK_LEFT)
			goingleft = TRUE;

		else if (wParam == VK_RIGHT)
			goingright = TRUE;

		else if (wParam == VK_UP)
		{
			if (0x40000000 & lParam)//bit 30
				return 0;//ignore typematics
			SetTimer(hwnd, Timers, 1, NULL);
			Timers++;
		}

		else if ((wParam == VK_DOWN) && (Timers > 0))
		{
			if (0x40000000 & lParam)//bit 30
				return 0;//ignore typematics
			KillTimer(hwnd, Timers);
			Timers--;
		}

		else if (wParam == VK_ESCAPE)
		{
			for (x = 0; x < Timers; x++)
				KillTimer(hwnd, x);
			if (IDNO == MessageBox(hwnd, "", "QUIT?", MB_YESNO))
				for (x = 0; x < Timers; x++)
					SetTimer(hwnd, x, 1, NULL);//x is an ID and 1 means send a WM_TIMER message every millisecond
			else
				DestroyWindow(hwnd);
		}
		return 0;

	case WM_KEYUP:
		goingleft = goingright = FALSE;
		return 0;

	case WM_PAINT:
		if (first) {
			GetClientRect(hwnd, &rect);
		}
		hdc = BeginPaint(hwnd, &ps);
		if (notfirst)
		{
			if ((goingleft) && (OldPaddle > PaddleMoves+2))
			{
				NewPaddle = OldPaddle-PaddleMoves;
				BitBlt(hdc, NewPaddle-PaddleMoves+2, PaddleTop, PADDLEWIDTH+2, 20, hdc, OldPaddle-PaddleMoves+2, PaddleTop, SRCCOPY);
				OldPaddle = NewPaddle;
			}
			else if ((goingright) && (OldPaddle < (cxScreen-(PADDLEWIDTH+3))))
			{
				NewPaddle = OldPaddle+PaddleMoves;
				BitBlt(hdc, NewPaddle-PaddleMoves, PaddleTop, PADDLEWIDTH+2, 20, hdc, OldPaddle-PaddleMoves, PaddleTop, SRCCOPY);
				OldPaddle = NewPaddle;
			}
			d2 += d;
			modf(d2, &d3);
			if (unit <= (int)d3) 
			{
				if ((xBall+BALL_DIAMETER >= cxScreen) || (xBall == 0))
				{//RIGHT/LEFT SIDE
					midiOutShortMsg(hMidiOut, 0x99 | (127 << 16) | (STICKS << 8));
					xDirection = -xDirection;
				}
				if (yBall == 0)
				{//TOP
					midiOutShortMsg(hMidiOut, 0x99 | (127 << 16) | (STICKS << 8));
					yDirection = -yDirection;
				}
				else if (yBall+BALL_DIAMETER >= PaddleBottom)
				{
//					midiOutShortMsg(hMidiOut, 0x99 | (127 << 16) | (CHINESESYMBOL << 8));
					for (x = 0; x < Timers; x++)
						KillTimer(hwnd, x);
					if (IDYES == MessageBox(hwnd, "", "Play Again?", MB_YESNO))
						goto jump;
					else
					{
						DestroyWindow(hwnd);
						EndPaint(hwnd, &ps);
						return 0;
					}
				}
				else if ((yDirection == 1) && (yBall+BALL_DIAMETER >= PaddleTop) && (yBallCenter <= PaddleTop) && (xBall+BALL_DIAMETER >= NewPaddle) && (xBall <= NewPaddle+PADDLEWIDTH))
				{//PADDLE
					midiOutShortMsg(hMidiOut, 0x99 | (127 << 16) | (STICKS << 8));
					if ((xDirection == 1) && (xBallCenter <= NewPaddle+(PADDLEWIDTH/2)))
						xDirection = -xDirection;
					else if ((xDirection == -1) && (xBallCenter > NewPaddle+(PADDLEWIDTH/2)))
						xDirection = -xDirection;
					yDirection = -yDirection;
				}
				else for (z = 0; z < 75; z++)
				{//BLOCKS
					if (((xBall+BALL_DIAMETER == Block[z].left) || (xBall == Block[z].right))
					  && ((yBallCenter >= Block[z].top) && (yBallCenter <= Block[z].bottom)))
					{//LEFT/RIGHT SIDE
						xDirection = -xDirection;
						midiOutShortMsg(hMidiOut, 0x99 | (127 << 16) | (STICKS << 8));
						BitBlt(hdc, Block[z].left, Block[z].top, BlockWidth, BlockHeight, hdc, 0, 0, SRCCOPY);
						Block[z].left = Block[z].top = Block[z].right = Block[z].bottom = -1;
					}
					else if ((yDirection == -1) && (xDirection == -1) && ((yBall < Block[z].bottom) && (yBall > Block[z].top) && (xBall > Block[z].left) && (xBall < Block[z].right))
					 || ((yDirection == -1) && (xDirection == 1)  && (yBall < Block[z].bottom) && (yBall > Block[z].top) && (xBall+BALL_DIAMETER > Block[z].left) && (xBall+BALL_DIAMETER < Block[z].right))
					 || ((yDirection == 1) && (xDirection == 1)  && (yBall+BALL_DIAMETER > Block[z].top) && (yBall+BALL_DIAMETER < Block[z].bottom) && (xBall+BALL_DIAMETER > Block[z].left) && (xBall+BALL_DIAMETER < Block[z].right))
					 || ((yDirection == 1) && (xDirection == -1)  && (yBall+BALL_DIAMETER > Block[z].top) && (yBall+BALL_DIAMETER < Block[z].bottom) && (xBall > Block[z].left) &&  (xBall < Block[z].right)))
					{//CORNER
						yDirection = -yDirection;
						xDirection = -xDirection;
						midiOutShortMsg(hMidiOut, 0x99 | (127 << 16) | (STICKS << 8));
						BitBlt(hdc, Block[z].left, Block[z].top, BlockWidth, BlockHeight, hdc, 0, 0, SRCCOPY);
						Block[z].left = Block[z].top = Block[z].right = Block[z].bottom = -1;
					}
					else if (((yBall+BALL_DIAMETER == Block[z].top) || (yBall == Block[z].bottom))
						 && ((xBallCenter >= Block[z].left) && (xBallCenter <= Block[z].right)))
					{//TOP/BOTTOM
						yDirection = -yDirection;
						midiOutShortMsg(hMidiOut, 0x99 | (127 << 16) | (STICKS << 8));
						BitBlt(hdc, Block[z].left, Block[z].top, BlockWidth, BlockHeight, hdc, 0, 0, SRCCOPY);
						Block[z].left = Block[z].top = Block[z].right = Block[z].bottom = -1;
					}
				}
				TransparentBlt(hdc, xBall, yBall, BALL_DIAMETER, BALL_DIAMETER, hdcMem2, 0, 0, BALL_DIAMETER, BALL_DIAMETER, WHITE);//erase
				TransparentBlt(hdc, xBall+xDirection, yBall+yDirection, BALL_DIAMETER, BALL_DIAMETER, hdcMem, 0, 0, BALL_DIAMETER, BALL_DIAMETER, WHITE);
				xBall += xDirection;
				yBall += yDirection;
				xBallCenter += xDirection;
				yBallCenter += yDirection;
				unit++;
			}
		}
		else//if (notfirst == FALSE)
		{
			notfirst = TRUE;
			if (first)
			{
				first = FALSE;
				hdcMem = CreateCompatibleDC(hdc);
				hBitmap = CreateCompatibleBitmap(hdc, cxScreen, cyScreen);
				SelectObject(hdcMem, hBitmap);
				hdcMem2 = CreateCompatibleDC(hdc);
				hBitmap2 = CreateCompatibleBitmap(hdc, cxScreen, cyScreen);
				SelectObject(hdcMem2, hBitmap2);
				FillRect(hdcMem2, &rect, hWhiteBrush);
				SelectObject(hdcMem2, hBackgroundPen);
				SelectObject(hdcMem2, hBackgroundBrush);
				Ellipse(hdcMem2, 0, 0, BALL_DIAMETER, BALL_DIAMETER);//to erase
				FillRect(hdcMem, &rect, hWhiteBrush);
				SelectObject(hdcMem, hPen);
				SelectObject(hdcMem, hPaddleBrush);
				Ellipse(hdcMem, 0, 0, BALL_DIAMETER, BALL_DIAMETER);
			}
			GetSystemTime(&st);
			SystemTimeToFileTime(&st, &ft);
			ul.LowPart = ft.dwLowDateTime;
			ul.HighPart = ft.dwHighDateTime;
			ul.QuadPart /= 10000;//because low 4 digits are 0's
			srand(ul.LowPart);
			srand(rand());

jump:		BlockWidth = (cxScreen-(16*SPACE)) / 15;
			BlockHeight = 32;
			NewPaddle = OldPaddle = (rand() % (cxScreen-(PADDLEWIDTH+2))+PaddleMoves);
			PaddleTop = 700;//cyScreen-100;
			PaddleBottom = 720;//cyScreen-80;

			FillRect(hdc, &rect, hBackgroundBrush);
			SelectObject(hdc, hPen);
			SelectObject(hdc, hPaddleBrush);
			RoundRect(hdc, OldPaddle, PaddleTop, OldPaddle+PADDLEWIDTH, PaddleBottom, 10, 10);
			MoveToEx(hdc, OldPaddle+(PADDLEWIDTH/2), PaddleTop, NULL);
			LineTo(hdc, OldPaddle+(PADDLEWIDTH/2), PaddleTop+20);

			Timers = 5;
			goingleft = goingright = FALSE;
			d2 = 0.0;
			z = brush = unit = 0;
			for (v = 0, y = 120; v < 5; y += BlockHeight+SPACE, v++)
			{
				switch (v)
				{
					case 0:
						red = 0xFF;
						green = 0;
						blue = 0;
						break;
					case 1:
						red = 0xFF;
						green = 0xFF;
						blue = 0;
						break;
					case 2:
						red = 0;
						green = 0xFF;
						blue = 0;
						break;
					case 3:
						red = 0;
						green = 0xFF;
						blue = 0xFF;
						break;
					case 4:
						red = 0;
						green = 0;
						blue = 0xFF;
						break;								
				}
				SelectObject(hdc, hBlockBrush[brush++]);
				for (x = SPACE; x < (15*(BlockWidth+SPACE)); x += (BlockWidth+SPACE))
				{
					Block[z].left = x;
					Block[z].top = y;
					Block[z].right = x+BlockWidth;
					Block[z].bottom = y+BlockHeight;
					for (w = 0; w < BlockHeight; w++)
					{
						hColor = CreatePen(PS_SOLID, 0, RGB(red, green, blue));
						SelectObject(hdc, hColor);
						MoveToEx(hdc, x, y+w, NULL);
						LineTo(hdc, x+BlockWidth, y+w);
						switch (v)
						{
							case 0:
								green += 8;
								break;
							case 1:
								red -= 8;
								break;
							case 2:
								blue += 8;
								break;
							case 3:
								green -= 8;
								break;
							case 4:
								red += 8;
								break;								
						}
					}
					z++;
				}
			}
			xBall = (OldPaddle+(PADDLEWIDTH/2) - (BALL_DIAMETER/2));
			yBall = PaddleTop-50;
			xBallCenter = xBall+(BALL_DIAMETER/2);
			yBallCenter = yBall+(BALL_DIAMETER/2);
			if (xBall < (cxScreen/2))
				xDirection = DIRECTION;
			else
				xDirection = -DIRECTION;
			yDirection = -DIRECTION;
			for (x = 0; x < Timers; x++)
				SetTimer(hwnd, x, 1, NULL);//x is an ID and 1 means send a WM_TIMER message every millisecond
			SetBkMode(hdc, TRANSPARENT);
			TextOut(hdc, (cxScreen/2)-140, cyScreen-60, "Use All Arrow Keys - Press ESC to Pause/Exit", 44);
			SetBkMode(hdc, OPAQUE);
		}
		EndPaint(hwnd, &ps);
		return 0;

	case WM_DESTROY:
		midiOutReset(hMidiOut);
		midiOutClose(hMidiOut);
		ReleaseDC(hwnd, hdcMem);
		ShowCursor(TRUE);
		for (x = 0; x < Timers; x++)
			KillTimer(hwnd, x);
		PostQuitMessage(0);
		return 0;
	}
	return DefWindowProc(hwnd, message, wParam, lParam);
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	char szAppName[] = "STATIC";
	MSG          msg;
	WNDCLASS     wndclass;

	wndclass.style         = CS_HREDRAW|CS_VREDRAW;
	wndclass.lpfnWndProc   = WndProc;
	wndclass.cbClsExtra    = 0;
	wndclass.cbWndExtra    = 0;
	wndclass.hInstance     = hInstance;
	wndclass.hIcon         = LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)CreateSolidBrush(BACKGROUND);
	wndclass.lpszMenuName  = NULL;
	wndclass.lpszClassName = szAppName;

	if (!RegisterClass(&wndclass))
		return 0;

	hwnd = CreateWindow(szAppName, NULL,
		WS_POPUP,
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
