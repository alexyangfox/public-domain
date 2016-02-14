/* Reinitialize2
 * Called from Works2, after the inputs in Dialog 2 have been checked.
 * Puts the contents of the dialog's text boxes into the active set of
 * variables.
 */
 
#include <windows.h>
#include "gddcalc.h"
#include "resource.h"
 
void Reinitialize2(HWND hDlg2)
{
   HWND hVar;
   char strVar[24];

   hVar = GetDlgItem(hDlg2, IDC_LATITUDE2);    // get latitude   
   GetWindowText(hVar, strVar, 24);
   active_latitude = atof(strVar);
     
   hVar = GetDlgItem(hDlg2, IDC_LONGITUDE2);   // get longitude   
   GetWindowText(hVar, strVar, 24);
   active_longitude = atof(strVar);

   hVar = GetDlgItem(hDlg2, IDC_ELEV2);        // get elevation   
   GetWindowText(hVar, strVar, 24);
   active_elev = atof(strVar);
     
   hVar = GetDlgItem(hDlg2, IDC_ASPECT2);      // get aspect   
   GetWindowText(hVar, strVar, 24);
   active_aspect = atof(strVar);
     
   hVar = GetDlgItem(hDlg2, IDC_TILT2);        // get tilt   
   GetWindowText(hVar, strVar, 24);
   active_tilt = atof(strVar);

   hVar = GetDlgItem(hDlg2, IDC_STARTDAY2);    // get start day   
   GetWindowText(hVar, strVar, 24);
   active_startday = atof(strVar);

   hVar = GetDlgItem(hDlg2, IDC_STOPDAY2);     // get stop day   
   GetWindowText(hVar, strVar, 24);
   active_stopday = atof(strVar);

   hVar = GetDlgItem(hDlg2, IDC_STARTMONTH2);  // get start month   
   active_startmonth = 1 + SendMessage(hVar, CB_GETCURSEL, 0, 0);

   hVar = GetDlgItem(hDlg2, IDC_STOPMONTH2);   // get stop month   
   active_stopmonth = 1 + SendMessage(hVar, CB_GETCURSEL, 0, 0);

   hVar = GetDlgItem(hDlg2, IDC_YEAR2);        // get year   
   GetWindowText(hVar, strVar, 24);
   active_year = atof(strVar);
   
/* Time zone depends on longitude. We assume we are in the lower 48 states.
 * This is all very approximate.
 */
    if (active_longitude > -85)
       active_timezone = -5;
    else if (active_longitude > -103)
       active_timezone = -6;
    else if (active_longitude > -115)
       active_timezone = -7;
    else
       active_timezone = -8;

   return;
} 
 
