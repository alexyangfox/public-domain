/* ChkTimes
 * Called by Works2. Looks at the start time and the stop time present in
 * Dialog 2. Returns 1 if the stop time is later than the start time, or 0
 * otherwise (an error condition).
 */
 
#include <windows.h>
#include "resource.h"

int ChkTimes(HWND hDlg2)
{
   HWND hVar;
   char strVar[24];
   int startMonth, stopMonth;
   int startDay, stopDay;
       
   hVar = GetDlgItem(hDlg2, IDC_STARTMONTH2);               // get start month   
   startMonth = 1 + SendMessage(hVar, CB_GETCURSEL, 0, 0);

   hVar = GetDlgItem(hDlg2, IDC_STOPMONTH2);                // get stop month   
   stopMonth = 1 + SendMessage(hVar, CB_GETCURSEL, 0, 0);
   
   if (stopMonth < startMonth)
      return 0;
   else if (stopMonth == startMonth)
   {
      hVar = GetDlgItem(hDlg2, IDC_STARTDAY2);              // get start day
      GetWindowText(hVar, strVar, 24);
      startDay = atoi(strVar);

      hVar = GetDlgItem(hDlg2, IDC_STOPDAY2);               // get stop day
      GetWindowText(hVar, strVar, 24);
      stopDay = atoi(strVar);
        
      if (stopDay < startDay) return 0; 
   }
   
   return 1;    
}
