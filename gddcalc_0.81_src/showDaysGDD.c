/* ShowDaysGDD
 * Called by Works1 to calculate the GDD accumulated during the
 * present day and put it into Dialog 1.
 */
 
#include <windows.h>
#include "gddcalc.h"
#include "solpos.h"
#include "resource.h"

float GetDaysIrr(int, int, int);
float GetMonthsIrr(int);
float GetMonthsGDD(float, int);

void ShowDaysGDD(HWND hDlg1)
{
   extern int logActive;
	extern HANDLE hLogfile;

   char bfr1[24], bfr2[128];
   int day, month, year;
   float daysIrr, monthsIrr, monthsGDD, daysGDD;
   DWORD dwCountWritten;
   HWND hVar;
   
   day = active_day;
   month = active_month;
   year = active_year;   
   daysIrr = GetDaysIrr(day, month, year);
   
   if (daysIrr <= 0.01)
      daysGDD = 0;
   else
   {
      monthsIrr = GetMonthsIrr(month);
      monthsGDD = GetMonthsGDD(monthsIrr, month);
      daysGDD = (daysIrr/monthsIrr)*monthsGDD;
   }
   
   sprintf(bfr1, "%5.2f deg days", daysGDD);
   hVar = GetDlgItem(hDlg1, IDC_GDD1);
   SetWindowText(hVar, bfr1);
   UpdateWindow(hVar);

/* If we are logging, send it to the logfile */

   if (logActive == 1)
   {           
      sprintf(bfr2, "%s%s%c", "\n   Accumulated GDD ", bfr1, '\n'); 
      WriteFile(hLogfile, bfr2, strlen(bfr2), &dwCountWritten, NULL);      
   }
   
   return;
}
