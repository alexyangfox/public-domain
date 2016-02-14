/* GDDCalc
   Calculates the amount of solar energy that falls on a square meter
   during a specified interval. Uses historical temperature data to
   calculate the associated growing degree days (GDD).
       
   THIS SOFTWARE (THE PRESENT PROGRAM AND ALL ITS SUBROUTINES AND
   INCLUDE FILES) IS OFFERED "AS IS", AND NWV GRANTS NO WARRANTIES
   OF ANY KIND, EXPRESS OR IMPLIED. NWV SPECIFICALLY DISCLAIMS ANY
   IMPLIED WARRANTIES OF MERCHANTABILITY, FITNESS FOR A SPECIFIC
   PURPOSE OR NONINFRINGEMENT CONCERNING THIS SOFTWARE.   
*/

#include <windows.h>
#include <winuser.h>            // for SendMessage
#include <wingdi.h>
#include <stdio.h>
#include <ctype.h>
#include "resource.h"
#include "gddcalc.h"

int logActive = 0;              // External variables
int logSaved = 0;
char logfileName[48];
HANDLE hLogfile;
BOOL havePrefs;
BOOL aboutRegistered;
BOOL tutorialRegistered;

void Initialize1(HWND);
void Initialize2(HWND);
void Initialize5();
void InitializeAtlanta();
void Works1(HWND, HINSTANCE);
void ShowDayBounds(HWND);
void ShowDaysIrr(HWND);
void Works2(HWND, HINSTANCE, int);
void Works5(HWND, HINSTANCE);
void CalcDlg3Results();
void ShowDlg3Results(HWND);
void Logging(HINSTANCE, HWND, HANDLE, int);
LPARAM CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Dlg1Proc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Dlg2Proc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Dlg3Proc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Dlg4Proc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Dlg5Proc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain (HINSTANCE hInstance, HINSTANCE hPrevInstance,
   PSTR szCmdLine, int iCmdShow)
{
   static TCHAR szAppName[] = TEXT("gddcalc");
   MSG          msg;
   HWND         hWnd;
   WNDCLASS     wndclass;
     
   wndclass.style         = 0;
   wndclass.lpfnWndProc   = WndProc;
   wndclass.cbClsExtra    = 0;
   wndclass.cbWndExtra    = 0;
   wndclass.hInstance     = hInstance;
   wndclass.hIcon         = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_VINE));
   wndclass.hCursor       = LoadCursor(NULL, IDC_ARROW);
   wndclass.hbrBackground = (HBRUSH)CreateSolidBrush(RGB(213, 203, 185));
   wndclass.lpszMenuName  = szAppName;
   wndclass.lpszClassName = szAppName;
     
   if(!RegisterClass(&wndclass))
   {
      MessageBox(NULL, TEXT("This program requires Windows NT!"),
         szAppName, MB_ICONERROR);
      return 0;
   }
     
   hWnd = CreateWindow(szAppName,
      TEXT("  GDDCALC - Growing Degree Day Calculator"),
      WS_OVERLAPPEDWINDOW,
      20, 20,                             // Upper left corner of window
      648, 480,                           // Width & height of window
      NULL, NULL, hInstance, NULL);
     
   ShowWindow(hWnd, iCmdShow);
   UpdateWindow(hWnd);
        
   while(GetMessage(&msg, NULL, 0, 0))
   {
      TranslateMessage(&msg);
      DispatchMessage(&msg);
   }
   return msg.wParam;
}

int ShowDib (HDC hdc, BITMAPINFO * pbmi, BYTE * pBits, int cxDib, int cyDib, 
             int cxClient, int cyClient)
{
   SetStretchBltMode (hdc, COLORONCOLOR) ;
   return StretchDIBits (hdc, 0, 0, cxClient, cyClient, 0, 0, cxDib, cyDib,
                         pBits, pbmi, DIB_RGB_COLORS, SRCCOPY) ;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
   extern BOOL aboutRegistered;
   extern BOOL tutorialRegistered;
   extern char logFileName[48];
   static HINSTANCE hInstance;
   static BITMAPFILEHEADER *pbmfh;
   static BITMAPINFO       *pbmi;
   static BYTE             *pBits;
   static int              cxClient, cyClient, cxDib, cyDib;
   static HANDLE           hFile, hMenu;
   int                     ierr;
   HDC                     hdc;
   PAINTSTRUCT             ps;

   switch(message)
   {
      case WM_CREATE:
         hInstance =((LPCREATESTRUCT) lParam)->hInstance;
         DibFileInitialize(hWnd);
         aboutRegistered = FALSE;
         tutorialRegistered = FALSE;

/* Copy an image to the main window */

         SetCursor(LoadCursor(NULL, IDC_WAIT)) ;
         ShowCursor(TRUE) ;
         pbmfh = (BITMAPFILEHEADER *)DibLoadImage(TEXT("gleaves.bmp"));
         ShowCursor(FALSE) ;
         SetCursor(LoadCursor(NULL, IDC_ARROW)) ;
         InvalidateRect(hWnd, NULL, TRUE);
        
         if (pbmfh == NULL)
         {
            MessageBox(hWnd, TEXT("Cannot load DIB file"),
                       TEXT("GDDCALC"), 0);
            return 0;
         }
                 
         pbmi = (BITMAPINFO *)(pbmfh + 1);
         pBits = (BYTE *)pbmfh + pbmfh->bfOffBits;
        
         if (pbmi->bmiHeader.biSize == sizeof(BITMAPCOREHEADER))
         {
            cxDib = ((BITMAPCOREHEADER *)pbmi)->bcWidth;
            cyDib = ((BITMAPCOREHEADER *)pbmi)->bcHeight;
         }
         else
         {
            cxDib = pbmi->bmiHeader.biWidth;
            cyDib = abs(pbmi->bmiHeader.biHeight);
         }

/* Disable the "Save the Log" menu item */

         hMenu = GetMenu(hWnd);
         EnableMenuItem(hMenu, IDM_SAVE_LOG, MF_GRAYED);
         
         return 0;
      
      case WM_SIZE:
         cxClient = LOWORD(lParam);
         cyClient = HIWORD(lParam);
         InvalidateRect (hWnd, NULL, TRUE) ;
          
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {                 
            case IDM_DIALOG1:
               DialogBox(hInstance, TEXT("DIALOG1"), hWnd, Dlg1Proc);
               break;

            case IDM_DIALOG2:
               DialogBox(hInstance, TEXT("DIALOG2"), hWnd, Dlg2Proc);
               break;

            case IDM_DIALOG3:
               DialogBox(hInstance, TEXT("DIALOG3"), hWnd, Dlg3Proc);
               break;

            case IDM_DIALOG4:
               DialogBox(hInstance, TEXT("DIALOG4"), hWnd, Dlg4Proc);
               break;

            case IDM_DIALOG5:
               DialogBox(hInstance, TEXT("DIALOG5"), hWnd, Dlg5Proc);
               break;

            case IDM_START_LOG:
               Logging(hInstance, hWnd, hMenu, LOGSTART);
               break;

            case IDM_SAVE_LOG:
               Logging(hInstance, hWnd, hMenu, LOGSAVE);
               break;

            case IDM_LOCALE:
               SetPreferences(hInstance, hWnd);
               break;               

            case IDM_HELP:
               ShowTutorialMsg(hInstance, hWnd);
               break;

            case IDM_ABOUT:
               ShowAboutBox(hInstance, hWnd);               
               break;

            case IDM_APP_EXIT:
               SendMessage(hWnd, WM_CLOSE, 0, 0);
               break;
         }
         return 0;
         
     case WM_PAINT:
        hdc = BeginPaint(hWnd, &ps);
        if (pbmfh) ShowDib(hdc, pbmi, pBits, cxDib, cyDib,
                           cxClient, cyClient);
        EndPaint(hWnd, &ps);           
        return 0;
                                                
     case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
   }
   return DefWindowProc(hWnd, message, wParam, lParam);
}

/* Calculate incident radiation and GDD during one day */

int CALLBACK Dlg1Proc(HWND hDlg1, UINT message, WPARAM wParam, LPARAM lParam)
{
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
   static HINSTANCE hInstance ;
   HBRUSH       hBrush;
   HDC          hDC;
   PAINTSTRUCT  ps;
   COLORREF     clrGreen = RGB(169, 228, 170);
   COLORREF     clrFawn = RGB(213, 203, 185);

   switch(message)
   {
      case WM_CREATE:
         hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
         return 0;
         
      case WM_PAINT:
             hDC = BeginPaint(hDlg1, &ps);
             SetBkMode(hDC, TRANSPARENT);
             hBrush = CreateSolidBrush(clrGreen);
             SelectObject(hDC, hBrush);
             Polygon(hDC, outline, 5);
             DeleteObject(hBrush);
             EndPaint(hDlg1, &ps);
         return 0;

      case WM_INITDIALOG: 
         Initialize1(hDlg1);
         return 0;
          
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {
            case IDC_START1:
               Works1(hDlg1, hInstance);
               return 0;
               
            case IDC_CLOSE1:
               EndDialog(hDlg1, 0);
               return 0;
         }

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrGreen);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrGreen);

      case WM_CTLCOLOREDIT:
         SetBkColor((HDC)wParam, clrFawn);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrFawn);
   }
   return 0;
}

/* Calculate indicent radiation and GDD during an extended period */

int CALLBACK Dlg2Proc(HWND hDlg2, UINT message, WPARAM wParam, LPARAM lParam)
{
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
   static HINSTANCE hInstance ;
   HBRUSH       hBrush;
   HDC          hDC;
   PAINTSTRUCT  ps;
   COLORREF     clrGreen = RGB(169, 228, 170);
   COLORREF     clrFawn = RGB(213, 203, 185);
   LOGFONT      lf;
   HFONT        hFont;
   HWND         hVar;

   switch(message)
   {
      case WM_PAINT:
         hDC = BeginPaint(hDlg2, &ps);
         SetBkMode(hDC, TRANSPARENT);

/* Paint the dialog box green */

         hBrush = CreateSolidBrush(clrGreen);
         SelectObject(hDC, hBrush);
         Polygon(hDC, outline, 5);
         DeleteObject(hBrush);
         EndPaint(hDlg2, &ps);
         return 0;

      case WM_CREATE :
         hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
         return 0 ;

      case WM_INITDIALOG: 
         Initialize2(hDlg2);
         hVar = GetDlgItem(hDlg2, IDC_CONTINUE2);
         EnableWindow(hVar, FALSE);
         return 0;
          
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {               
            case IDC_START2:
               Works2(hDlg2, hInstance, 0);
               return 0;
               
            case IDC_CONTINUE2:
               Works2(hDlg2, hInstance, 1);
               return 0;

            case IDC_CLOSE2:
               EndDialog(hDlg2, 0);
               return 0;
         }

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrGreen);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrGreen);

      case WM_CTLCOLOREDIT:
         SetBkColor((HDC)wParam, clrFawn);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrFawn);
   }
   return 0;
}

/* Test case: calculate various solpos parameters at a particular instant */

int CALLBACK Dlg3Proc(HWND hDlg3, UINT message, WPARAM wParam, LPARAM lParam)
{
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
   HDC hDC;
   PAINTSTRUCT ps;
   COLORREF clrFawn = RGB(213, 203, 185);
   COLORREF clrBlue = RGB(166, 202, 240);
   HBRUSH hBrush;
   HINSTANCE hInstance;
   HWND hVar;
   
   switch (message)
   {
      case WM_INITDIALOG:
         hVar = GetDlgItem(hDlg3, IDC_CLOSE3);
         EnableWindow(hVar, FALSE);
         return 0;
         
      case WM_COMMAND:
         switch (LOWORD (wParam))
         {
            case IDC_START3:
               InitializeAtlanta();
               CalcDlg3Results();
               ShowDlg3Results(hDlg3);
               hVar = GetDlgItem(hDlg3, IDC_START3);
               EnableWindow(hVar, FALSE);
               hVar = GetDlgItem(hDlg3, IDC_CLOSE3);
               EnableWindow(hVar, TRUE);
               return 0;
               
            case IDC_CLOSE3:
               EndDialog(hDlg3, 0);
               return 0;
         }
      case WM_PAINT:
         hDC = BeginPaint(hDlg3, &ps);
         SetBkMode(hDC, TRANSPARENT);
         hBrush = CreateSolidBrush(clrBlue);
         SelectObject(hDC, hBrush);
         Polygon(hDC, outline, 5);
         DeleteObject(hBrush);
         EndPaint(hDlg3, &ps);
         return 0;

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrBlue);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrBlue);

      case WM_CTLCOLOREDIT:
         SetBkColor((HDC)wParam, clrFawn);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrFawn);
   }
   return 0;     
}

/* Test case: Calculate incident radiation and GDD during a particular day */

int CALLBACK Dlg4Proc(HWND hDlg4, UINT message, WPARAM wParam, LPARAM lParam)
{
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
   HDC hDC;
   PAINTSTRUCT ps;
   COLORREF clrFawn = RGB(213, 203, 185);
   COLORREF clrBlue = RGB(166, 202, 240);
   HBRUSH hBrush;
   HINSTANCE hInstance;
   LOGFONT      lf;
   HFONT        hFont;
   HWND         hVar;
   
   switch (message)
   {
      case WM_INITDIALOG:
         hVar = GetDlgItem(hDlg4, IDC_CLOSE4);
         EnableWindow(hVar, FALSE);
         return 0;
         
      case WM_COMMAND:
         switch (LOWORD (wParam))
         {
            case IDC_START4:
               InitializeAtlanta();
               ShowDayBounds(hDlg4);
               ShowDaysIrr(hDlg4);
               hVar = GetDlgItem(hDlg4, IDC_START4);
               EnableWindow(hVar, FALSE);
               hVar = GetDlgItem(hDlg4, IDC_CLOSE4);
               EnableWindow(hVar, TRUE);
               return 0;
               
            case IDC_CLOSE4:
               EndDialog(hDlg4, 0);
               return 0;
         }
      case WM_PAINT:
         hDC = BeginPaint(hDlg4, &ps);
         SetBkMode(hDC, TRANSPARENT);
         hBrush = CreateSolidBrush(clrBlue);
         SelectObject(hDC, hBrush);
         Polygon(hDC, outline, 5);
         DeleteObject(hBrush);
         EndPaint(hDlg4, &ps);
         return 0;

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrBlue);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrBlue);

      case WM_CTLCOLOREDIT:
         SetBkColor((HDC)wParam, clrFawn);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrFawn);
   }
   return 0;     
}

/* Test case: calculate incident radiation and GDD during the growing season */

int CALLBACK Dlg5Proc(HWND hDlg5, UINT message, WPARAM wParam, LPARAM lParam)
{
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
   HDC hDC;
   PAINTSTRUCT ps;
   COLORREF clrFawn = RGB(213, 203, 185);
   COLORREF clrBlue = RGB(166, 202, 240);
   HBRUSH hBrush;
   HINSTANCE hInstance;
   LOGFONT      lf;
   HFONT        hFont;
   HWND         hVar;
   
   switch (message)
   {
      case WM_INITDIALOG:
         Initialize5();
         hVar = GetDlgItem(hDlg5, IDC_END5);
         EnableWindow(hVar, FALSE);
         return 0;
         
      case WM_COMMAND:
         switch(LOWORD(wParam))
         {               
            case IDC_START5:
               hVar = GetDlgItem(hDlg5, IDC_START5);
               EnableWindow(hVar, FALSE);
               Works5(hDlg5, hInstance);
               hVar = GetDlgItem(hDlg5, IDC_END5);
               EnableWindow(hVar, TRUE);
               SetFocus(hVar);
               return 0;
               
            case IDC_END5:
               EndDialog(hDlg5, 0);
               return 0;
         }

     case WM_PAINT:
         hDC = BeginPaint(hDlg5, &ps);
         SetBkMode(hDC, TRANSPARENT);
         hBrush = CreateSolidBrush(clrBlue);
         SelectObject(hDC, hBrush);
         Polygon(hDC, outline, 5);
         DeleteObject(hBrush);
         EndPaint(hDlg5, &ps);
         return 0;

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrBlue);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrBlue);

      case WM_CTLCOLOREDIT:
         SetBkColor((HDC)wParam, clrFawn);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrFawn);
   }
   return 0;     
}
