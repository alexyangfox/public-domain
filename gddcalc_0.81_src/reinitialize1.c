/* Reinitialize1
 * Called by Works1 to copy parameter values from the dialog window
 * to the active set of variables, which the calculations will use.
 * We copy values from the screen window and redefine the active set
 * rather than use those already in the active set because the user
 * may have typed new values into the dialog.
 */

#include <windows.h>
#include "gddcalc.h"
#include "resource.h"
 
void Reinitialize1(HWND hDlg1)
{
   HWND hVar;
   char strVar[24];

   hVar = GetDlgItem(hDlg1, IDC_LATITUDE1);    // get latitude   
   GetWindowText(hVar, strVar, 24);
   active_latitude = atof(strVar);
     
   hVar = GetDlgItem(hDlg1, IDC_LONGITUDE1);   // get longitude   
   GetWindowText(hVar, strVar, 24);
   active_longitude = atof(strVar);

   hVar = GetDlgItem(hDlg1, IDC_ELEV1);        // get elevation   
   GetWindowText(hVar, strVar, 24);
   active_elev = atof(strVar);
     
   hVar = GetDlgItem(hDlg1, IDC_ASPECT1);      // get aspect   
   GetWindowText(hVar, strVar, 24);
   active_aspect = atof(strVar);
     
   hVar = GetDlgItem(hDlg1, IDC_TILT1);        // get tilt   
   GetWindowText(hVar, strVar, 24);
   active_tilt = atof(strVar);

   hVar = GetDlgItem(hDlg1, IDC_DAY1);         // get day   
   GetWindowText(hVar, strVar, 24);
   active_day = atof(strVar);

   hVar = GetDlgItem(hDlg1, IDC_MONTH1);       // get month   
   active_month = 1 + SendMessage(hVar, CB_GETCURSEL, 0, 0);

   hVar = GetDlgItem(hDlg1, IDC_YEAR1);        // get year   
   GetWindowText(hVar, strVar, 24);
   active_year = atof(strVar);
   
/* Time zone depends on longitude. The following code works
 * (approximately) for the lower 48 and it remains here for the
 * sake of some possible future use. However, elsewhere we have
 * restricted the play to the Pacific Northwest.
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
