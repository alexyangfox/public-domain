/* SetPreferences
 * Called from gddcalc. Sets a persistant geographic descriptors
 * by soliciting them with Dialog 7, then writes them into a file
 * named Locale.txt.
 */

#include <windows.h>
#include <stdlib.h>
#include "resource.h"

int WrtPrefFile(HWND);
int CALLBACK Dlg1aProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Dlg7Proc(HWND, UINT, WPARAM, LPARAM);

void SetPreferences(HINSTANCE hInstance, HWND hWnd)
{
   DialogBox(hInstance, TEXT ("DIALOG7"), hWnd, Dlg7Proc);
   return;
}

int CALLBACK Dlg7Proc(HWND hDlg7, UINT message, WPARAM wParam, LPARAM lParam)
{
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
   static HINSTANCE hInstance;
   HWND hVar;
   HDC hDC;
   PAINTSTRUCT ps;
   COLORREF clrGreen = RGB(169, 228, 170);
   COLORREF clrFawn = RGB(213, 203, 185);
   HBRUSH hBrush;
   
   switch (message)
   {
      case WM_CREATE:
         hInstance = ((LPCREATESTRUCT) lParam)->hInstance;
         break;

      case WM_INITDIALOG:
         hVar = GetDlgItem(hDlg7, IDC_LATITUDE7);
         SetFocus(hVar);
         break;

      case WM_PAINT:
         hDC = BeginPaint(hDlg7, &ps);
         SetBkMode(hDC, TRANSPARENT);
         hBrush = CreateSolidBrush(clrGreen);
         SelectObject(hDC, hBrush);
         Polygon(hDC, outline, 5);
         DeleteObject(hBrush);
         EndPaint(hDlg7, &ps);

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrGreen);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrGreen);

      case WM_CTLCOLOREDIT:
         SetBkColor((HDC)wParam, clrFawn);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrFawn);
          
      case WM_COMMAND:
         switch (LOWORD (wParam))
         {
            case IDC_CLOSE7:
               if (WrtPrefFile(hDlg7) == 1)
               {
                  EndDialog(hDlg7, 0);
                  return 0;
               }
               else
               {
                  DialogBox(hInstance, TEXT ("DIALOG1A"), hDlg7, Dlg1aProc);
                  return 0;
               }
            case IDC_CANCEL7:
               EndDialog(hDlg7, 0);
               return 0;
         }
   }
   return 0;
}

