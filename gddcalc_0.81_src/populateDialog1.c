/* PopulateDialog1
 * Called by Initialize1 when Dialog 1 initializes. Places values in the
 * text boxes and the combo box of Dialog 1.
 */

#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include "gddcalc.h"
#include "resource.h"

void PopulateDialog1(HWND hDlg1)
{
   extern BOOL havePrefs;
   HWND hVar;
   char strVar[12];

   strcpy(strVar, "");   

   hVar = GetDlgItem(hDlg1, IDC_LATITUDE1);
   if (havePrefs) sprintf(strVar, "%8.4f", active_latitude); 
   SetWindowText(hVar, strVar);
   
   hVar = GetDlgItem(hDlg1, IDC_LONGITUDE1);
   if (havePrefs) sprintf(strVar, "%8.4f", active_longitude);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg1, IDC_ELEV1);
   if (havePrefs) sprintf(strVar, "%.1f meters", active_elev);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg1, IDC_ASPECT1);
   if (havePrefs) sprintf(strVar, "%6.1f deg true", active_aspect);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg1, IDC_TILT1);
   if (havePrefs) sprintf(strVar, "%6.1f deg", active_tilt);
   SetWindowText(hVar, strVar);

/* Whether we have preferences or not, the time boxes (day, month,
 * year) are left blank.
 */
   hVar = GetDlgItem(hDlg1, IDC_DAY1);
   SetWindowText(hVar, "");

   hVar = GetDlgItem(hDlg1, IDC_YEAR1);
   SetWindowText(hVar, "");

   hVar = GetDlgItem(hDlg1, IDC_MONTH1);
   SendMessage(hVar, CB_ADDSTRING, 0, (LPARAM)"January");
   SendMessage(hVar, CB_ADDSTRING, 1, (LPARAM)"February");
   SendMessage(hVar, CB_ADDSTRING, 2, (LPARAM)"March");
   SendMessage(hVar, CB_ADDSTRING, 3, (LPARAM)"April");
   SendMessage(hVar, CB_ADDSTRING, 4, (LPARAM)"May");
   SendMessage(hVar, CB_ADDSTRING, 5, (LPARAM)"June");
   SendMessage(hVar, CB_ADDSTRING, 6, (LPARAM)"July");
   SendMessage(hVar, CB_ADDSTRING, 7, (LPARAM)"August");
   SendMessage(hVar, CB_ADDSTRING, 8, (LPARAM)"September");
   SendMessage(hVar, CB_ADDSTRING, 9, (LPARAM)"October");
   SendMessage(hVar, CB_ADDSTRING, 10, (LPARAM)"November");
   SendMessage(hVar, CB_ADDSTRING, 11, (LPARAM)"December");

   return;
}
