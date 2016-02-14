/* Works5
 * Called from gddcalc when the user presses the Start button of Dialog 5.
 * Calculates irradiance and GDD month by month, April through October.
 */
 
#include <windows.h> 
#include "resource.h"

float GetMonthsIrr(int);
float GetMonthsGDD(float, int);

void Works5(HWND hDlg5, HINSTANCE hInstance)
{
   HWND hIrr, hGdd;
   char strVar[24];
   int month;
   float monthsIrr, monthsGDD;

   for (month = 4; month <= 10; month++)
   {
      monthsIrr = GetMonthsIrr(month);
      monthsGDD = GetMonthsGDD(monthsIrr, month);
      
      if (month == 4)
      {
         hIrr = GetDlgItem(hDlg5, IDC_APRIRR5);
         hGdd = GetDlgItem(hDlg5, IDC_APRGDD5);
      }
      else if (month == 5)
      {
         hIrr = GetDlgItem(hDlg5, IDC_MAYIRR5);
         hGdd = GetDlgItem(hDlg5, IDC_MAYGDD5);
      }
      else if (month == 6)
      {
         hIrr = GetDlgItem(hDlg5, IDC_JUNIRR5);
         hGdd = GetDlgItem(hDlg5, IDC_JUNGDD5);
      }
      else if (month == 7)
      {
         hIrr = GetDlgItem(hDlg5, IDC_JULIRR5);
         hGdd = GetDlgItem(hDlg5, IDC_JULGDD5);
      }
      else if (month == 8)
      {
         hIrr = GetDlgItem(hDlg5, IDC_AUGIRR5);
         hGdd = GetDlgItem(hDlg5, IDC_AUGGDD5);
      }
      else if (month == 9)
      {
         hIrr = GetDlgItem(hDlg5, IDC_SEPIRR5);
         hGdd = GetDlgItem(hDlg5, IDC_SEPGDD5);
      }
      else if (month == 10)
      {
         hIrr = GetDlgItem(hDlg5, IDC_OCTIRR5);
         hGdd = GetDlgItem(hDlg5, IDC_OCTGDD5);
      }
          
      sprintf(strVar, "%.2f mj/sqm", monthsIrr);
      SetWindowText(hIrr, strVar);
      UpdateWindow(hIrr);

      sprintf(strVar, "%.2f", monthsGDD);
      SetWindowText(hGdd, strVar);
      UpdateWindow(hGdd);    
   }
   return;
}
