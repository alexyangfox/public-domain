#include <windows.h>
#include <winuser.h>
#include <wingdi.h>
#include <stdio.h>
#include <ctype.h>

LPARAM CALLBACK TutorialMsgProc(HWND, UINT, WPARAM, LPARAM);

int ShowTutorialMsg(HINSTANCE hInstance, HWND hWnd)
{
   extern BOOL tutorialRegistered;
   static TCHAR szAppName[] = TEXT("TutorialMsg");
   WNDCLASS wndclass;
   HWND hWndAbout;
   
   wndclass.style         = CS_HREDRAW | CS_VREDRAW;
   wndclass.lpfnWndProc   = TutorialMsgProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = 0;
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = LoadIcon (NULL, IDI_APPLICATION);
   wndclass.hCursor       = LoadCursor (NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH) GetStockObject (WHITE_BRUSH);
   wndclass.lpszMenuName  = NULL;
   wndclass.lpszClassName = szAppName;

   if (!tutorialRegistered)
   {
      if (!RegisterClass(&wndclass))
      {
         MessageBox(NULL,
                 TEXT("This program requires Windows NT!"),
                 szAppName, MB_ICONERROR);
         return 0;
      }
      else
         tutorialRegistered = TRUE;
   }
     
   hWndAbout = CreateWindow(szAppName, TEXT("GDDCALC tutorial"),
              WS_OVERLAPPEDWINDOW, 
              100, 100,
              550, 350,
              hWnd, NULL, hInstance, NULL);
   ShowWindow(hWndAbout, SW_SHOW);
   UpdateWindow(hWndAbout);
   return 0;
}

LPARAM CALLBACK TutorialMsgProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   static HBITMAP hBitmap;
   static int     cxClient, cyClient, cxSource, cySource;
   BITMAP         bitmap;
   HDC            hdc, hdcMem;
   HINSTANCE      hInstance;
   int            x, y;
   PAINTSTRUCT    ps;
     
   switch (message)
   {
      case WM_CREATE:
         hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
         hBitmap = LoadBitmap (hInstance, TEXT("tutorial"));
         GetObject(hBitmap, sizeof(BITMAP), &bitmap) ;
         cxSource = bitmap.bmWidth;
         cySource = bitmap.bmHeight;
         return 0 ;

      case WM_SIZE:
         cxClient = LOWORD (lParam);
         cyClient = HIWORD (lParam);
         return 0;

      case WM_PAINT:
         hdc = BeginPaint(hWnd, &ps);
         hdcMem = CreateCompatibleDC (hdc);
         SelectObject(hdcMem, hBitmap);
         x = 10;
         y = 10;
         BitBlt(hdc, x, y, cxSource, cySource, hdcMem, 0, 0, SRCCOPY);
         DeleteDC(hdcMem);
         EndPaint(hWnd, &ps);
         return 0;

      case WM_DESTROY:
         DeleteObject(hBitmap);
         CloseWindow(hWnd);
         return 0;
   }
   return DefWindowProc(hWnd, message, wParam, lParam);
}
