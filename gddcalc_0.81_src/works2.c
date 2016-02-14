/* Works2
 * Called from gddcalc when the user presses the Start button of Dialog 2.
 * Calculates monthly and accumulated irradiance and GDD month by month
 * during an inteval specified by the user.
 */
 
#include <windows.h> 
#include "resource.h"
#include "gddcalc.h"

float GetMonthsIrr(int);
float GetMonthsGDD(float, int);
float GetDaysIrr(int, int, int);
int CALLBACK Dlg1aProc(HWND, UINT, WPARAM, LPARAM);
int CALLBACK Dlg2aProc(HWND, UINT, WPARAM, LPARAM);

extern int logActive;
extern HANDLE hLogfile;

int ChkInputs2(HWND);
int ChkTimes(HWND);
void Reinitialize2(HWND);
float GetMonthsIrr(int);
float GetMonthsGDD(float, int);

void Works2(HWND hDlg2, HINSTANCE hInstance, int flag)
{
   HWND hVar;
   DWORD dwCountWritten;
   char strVar[24], bfr[128];
   char *monthname[] = {"January", "February", "March", "April", "May", "June",
      "July", "August", "September", "October", "November", "December"};
   int day, firstday, lastday;
   static int month, year;
   float daysIrr, monthsIrr, monthsGDD;
   float fullMonthsIrr, fullMonthsGDD;
   static float totalIrr, totalGDD;

/* The first time we enter Works2 (when the user presses the START button)
 * flag = 0. This tells us to set up the analysis and proceed through the
 * first month. On subsequent entries (when the user presses the CONTINUE
 * button we jump down to NEXTMONTH to calculate accumulated irradiation and
 * GDD for the next month.
 */
   if (flag) goto NEXTMONTH;
      
/* When we enter Works2 the text boxes in Dialog 2 all should contain
 * values that describe the analysis the user wants. Check them for
 * legitimacy and place them in the externals defined in gddcalc.h.
 * If the inputs are not OK, put up a dialog box (Dialog 1a) that
 * informs the user.
 */
   if (!ChkInputs2(hDlg2))
   {
      DialogBox(hInstance, TEXT ("DIALOG1A"), hDlg2, Dlg1aProc);
      return;
   }

/* Now make sure the stop time is later than the start time */

   if (!ChkTimes(hDlg2))
   {
      DialogBox(hInstance, TEXT ("DIALOG2A"), hDlg2, Dlg2aProc);
      return;
   }

/* The inputs (i.e., the values in Dialog 2) are OK. Put them into the
 * active set of variables (from which all calculations are made).
 */
   Reinitialize2(hDlg2);

/* If we are logging, start the log for this invocation of Dialog 2 */

   if (logActive) StartDlg2Log();

/* The primary loop begins here. Each cycle computes the accumulated irradiance
 * and GDD for one month.
 */
   totalIrr = 0;
   totalGDD = 0;
   month = active_startmonth - 1;
   year = active_year;
   
NEXTMONTH:

   month++;

/* What are the first and last days of the current month for which we
 * will calculate GDD? Put this information into the dialog window.
 */
   if (month == active_startmonth)
   {
      firstday = active_startday;
      if (active_startmonth == active_stopmonth)
         lastday = active_stopday;
      else
         lastday = dayspermonth[month - 1];                
   }
   else if (month == active_stopmonth)
   {
      lastday = active_stopday;
      if (active_startmonth == active_stopmonth)
         firstday = active_startday;
      else
         firstday = 1;
   }
   else
   {
      firstday = 1;
      lastday = dayspermonth[month - 1];
   }
   
   hVar = GetDlgItem(hDlg2, IDC_OUTDAY2);
   sprintf(strVar, "%d - %d", firstday, lastday);
   SetWindowText(hVar, strVar);
   UpdateWindow(hVar);

   hVar = GetDlgItem(hDlg2, IDC_OUTMONTH2);
   sprintf(strVar, "%s", monthname[month - 1]);
   SetWindowText(hVar, strVar);
   UpdateWindow(hVar);

/* Calculate this month's total corrected irradiation and the correlated
 * GDD, then put them into the dialog window along with the accumulated
 * amounts.
 */
   if (firstday == 1 && lastday == dayspermonth[month-1])
   {
      monthsIrr = GetMonthsIrr(month);
      monthsGDD = GetMonthsGDD(monthsIrr, month);
   }
   else
   {
      monthsIrr = 0;
      for (day = firstday; day <= lastday; day++)
      {
         daysIrr = GetDaysIrr(day, month, year);
         monthsIrr = monthsIrr + daysIrr;
      }
      fullMonthsIrr = GetMonthsIrr(month);
      fullMonthsGDD = GetMonthsGDD(fullMonthsIrr, month);
      monthsGDD = (monthsIrr/fullMonthsIrr)*fullMonthsGDD;      
   }

   sprintf(strVar, "%.2f mj/sqm", monthsIrr);
   hVar = GetDlgItem(hDlg2, IDC_CORRIRR2);
   SetWindowText(hVar, strVar);
   UpdateWindow(hVar);

   sprintf(strVar, "%.2f", monthsGDD);
   hVar = GetDlgItem(hDlg2, IDC_DEGDAYS2);
   SetWindowText(hVar, strVar);
   UpdateWindow(hVar);
   
   totalIrr = totalIrr + monthsIrr;
   sprintf(strVar, "%.2f mj/sqm", totalIrr);
   hVar = GetDlgItem(hDlg2, IDC_TOTLCORRIRR2);
   SetWindowText(hVar, strVar);
   UpdateWindow(hVar);
   
   totalGDD = totalGDD + monthsGDD;
   sprintf(strVar, "%.2f", totalGDD);
   hVar = GetDlgItem(hDlg2, IDC_TOTLGDD2);
   SetWindowText(hVar, strVar);
   UpdateWindow(hVar);

/* Enable anad disable the start and continue buttons, as appropriate */

   if (month == active_startmonth && active_startmonth != active_stopmonth)
   {
      hVar = GetDlgItem(hDlg2, IDC_START2);
      EnableWindow(hVar, FALSE);
      hVar = GetDlgItem(hDlg2, IDC_CONTINUE2);
      EnableWindow(hVar, TRUE);
   }
   else if (month == active_stopmonth)
   {
      hVar = GetDlgItem(hDlg2, IDC_CONTINUE2);
      EnableWindow(hVar, FALSE);
      hVar = GetDlgItem(hDlg2, IDC_START2);
      EnableWindow(hVar, TRUE);
   }

/* If we are logging, send it to the logfile */

   if (logActive == 1)
   {
      sprintf(bfr, "%s%s%s%d - %d",
         "\n\n   Month: ", monthname[month - 1],
         "\n   Days: ", firstday, lastday);
      WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
      
      sprintf(bfr, "%s%s%7.2f%s%7.2f",
         "\n   Irradiance:",
         "\n      this month: ", monthsIrr,
         "\n      accumulated: ", totalIrr);
      WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);

      sprintf(bfr, "%s%s%7.2f%s%7.2f",
         "\n   Growing Degree Days:",
         "\n      this month: ", monthsGDD,
         "\n      accumulated: ", totalGDD);
      WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
    }
   return;       
}

/* Dialog 2a displays a warning that one or more inputs is out of bounds */

int CALLBACK Dlg2aProc(HWND hDlg2a, UINT message, WPARAM wParam, LPARAM lParam)
{
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
   HDC hDC;
   PAINTSTRUCT ps;
   COLORREF clrYellow = RGB(250, 250, 140);
   HBRUSH hBrush;
   
   switch (message)
   {
      case WM_INITDIALOG:
         break;
          
      case WM_COMMAND:
         switch (LOWORD (wParam))
         {
            case IDOK:
               EndDialog (hDlg2a, 0);
               return 0;
         }
         break;
         
      case WM_PAINT:
         hDC = BeginPaint(hDlg2a, &ps);
         SetBkMode(hDC, TRANSPARENT);
         hBrush = CreateSolidBrush(clrYellow);
         SelectObject(hDC, hBrush);
         Polygon(hDC, outline, 5);
         DeleteObject(hBrush);
         EndPaint(hDlg2a, &ps);
         break;

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrYellow);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrYellow);
   }
   return 0;     
}
