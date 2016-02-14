/* Works1
 * Called from gddcalc when the user presses the Start button of Dialog 1.
 * Does all of the calculations.
 */
 
#include <windows.h> 
#include <stdio.h>
#include <ctype.h>
#include "resource.h"
#include "gddcalc.h"
#include <stdlib.h>

int CALLBACK Dlg1aProc(HWND, UINT, WPARAM, LPARAM);
int ChkInputs1(HWND);
void Reinitialize1(HWND);
void ShowDayBounds(HWND);
void ShowDaysIrr(HWND);
void ShowDaysGDD(HWND);
 
void Works1(HWND hDlg1, HINSTANCE hInstance)
{
   HWND hVar;
      
/* When we enter Works1 the text boxes in Dialog 1 all should contain
 * values that describe the analysis the user wants. Check them for
 * legitimacy and place them in the externals defined in gddcalc.h.
 * If the inputs are not OK, put up a dialog box (Dialog 1a) that
 * informs the user.
 */
   if (!ChkInputs1(hDlg1))
   {
      DialogBox(hInstance, TEXT ("DIALOG1A"), hDlg1, Dlg1aProc);
      return;
   }

/* The inputs (the values in Dialog 1) are OK. Put them somewhere safe */

   Reinitialize1(hDlg1);

 /* Calculate the times of sunrise and sunset and put them into the
  * appropriate externals and also into Dialog 1. They will be used by
  * ShowAccIrr1 below.
  */
   ShowDayBounds(hDlg1);

/* Calculate the accumulated irradiance between sunrise and sunset, and
 * put it into Dialog 1.
 */
   ShowDaysIrr(hDlg1);

/* Calculate the growing degree days for the day, and put this number
 * into Dialog 1.
 */
    ShowDaysGDD(hDlg1);
   
   return;
}

/* Dialog 1a diaplays a warning that one or more inputs is out of bounds */

int CALLBACK Dlg1aProc(HWND hDlg1a, UINT message, WPARAM wParam, LPARAM lParam)
{
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
   HDC hDC;
   PAINTSTRUCT ps;
   COLORREF clrYellow = RGB(250, 250, 140);
   HBRUSH hBrush;
   
   switch (message)
   {
      case WM_INITDIALOG:
         return 0;
          
      case WM_COMMAND:
         switch (LOWORD (wParam))
         {
            case IDOK:
            EndDialog(hDlg1a, 0);
            return 0;
         }
      case WM_PAINT:
             hDC = BeginPaint(hDlg1a, &ps);
             SetBkMode(hDC, TRANSPARENT);
             hBrush = CreateSolidBrush(clrYellow);
             SelectObject(hDC, hBrush);
             Polygon(hDC, outline, 5);
             DeleteObject(hBrush);
             EndPaint(hDlg1a, &ps);

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrYellow);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrYellow);
   }
   return 0;     
}
