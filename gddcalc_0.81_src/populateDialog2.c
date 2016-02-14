/* PopulateDialog2
 * Called by Initialize2 when Dialog 2 initializes. Places values in the
 * text and combo boxes of Dialog 2.
 */

#include <windows.h>
#include <stdio.h>
#include <ctype.h>
#include "gddcalc.h"
#include "resource.h"

void PopulateDialog2(HWND hDlg2)
{
   extern BOOL havePrefs;
   HWND hVar;
   char strVar[12];

   strcpy(strVar, "");   
   
   hVar = GetDlgItem(hDlg2, IDC_LATITUDE2);
   if (havePrefs) sprintf(strVar, "%8.4f", active_latitude);
   SetWindowText(hVar, strVar);
   
   hVar = GetDlgItem(hDlg2, IDC_LONGITUDE2);
   if (havePrefs) sprintf(strVar, "%8.4f", active_longitude);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg2, IDC_ELEV2);
   if (havePrefs) sprintf(strVar, "%.1f meters", active_elev);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg2, IDC_ASPECT2);
   if (havePrefs) sprintf(strVar, "%6.1f deg true", active_aspect);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg2, IDC_TILT2);
   if (havePrefs) sprintf(strVar, "%6.2f deg", active_tilt);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg2, IDC_STARTDAY2);
   sprintf(strVar, "%d", active_startday);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg2, IDC_STOPDAY2);
   sprintf(strVar, "%d", active_stopday);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg2, IDC_YEAR2);
   sprintf(strVar, "%d", active_year);
   SetWindowText(hVar, strVar);

   hVar = GetDlgItem(hDlg2, IDC_STARTMONTH2);
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
   SendMessage(hVar, CB_SETCURSEL, active_startmonth - 1, 0);

   hVar = GetDlgItem(hDlg2, IDC_STOPMONTH2);
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
   SendMessage(hVar, CB_SETCURSEL, active_stopmonth - 1, 0);

   return;
}
