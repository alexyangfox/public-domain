/* ChkInputs2
 * Called by Works2. Looks at some of the inputs provided by the user
 * in Dialog 1 and checks them for legitimacy. If they are OK they are
 * placed in the active variable set that holds their values. If they
 * are inappropriate or blank "error" is written in the variable's
 * text box and the user is notified.
 */

#include <windows.h>
#include "resource.h"
#include "gddcalc.h"

void SetMonthLengths(int);                     // prototype

int ChkInputs2(HWND hDlg2)
{
   HWND hVar;
   char strVar[24], tmpBfr[24];
   int latOK = 1, lonOK = 1, startdayOK = 1, stopdayOK = 1;
   int yearOK = 1, elevOK = 1, aspectOK = 1, tiltOK = 1;
   int day, year, startMonth, stopMonth;
   float lat, lon, elev, aspect, tilt;
   
   hVar = GetDlgItem(hDlg2, IDC_LATITUDE2);    // check latitude   
   GetWindowText(hVar, strVar, 24);
   lat = atof(strVar);
   if (lat < 42 || lat > 47 || strcmp(strVar, "") == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      latOK = 0;
   }
   else
      active_latitude = lat;
   
   hVar = GetDlgItem(hDlg2, IDC_LONGITUDE2);   // check longitude
   GetWindowText(hVar, strVar, 24);
   lon = atof(strVar);
   if (lon > 0)                         // If the user enters a
   {                                    // positive (east) longitude
      lon = -lon;                       // make it a negative (west)
      strcpy(tmpBfr, "-");              // latitude. We know he
      strcat(tmpBfr, strVar);           // means west.
      SetWindowText(hVar, tmpBfr);
      UpdateWindow(hVar);
   }   
   if (lon < -124 || lon > -122 || strcmp(strVar, "") == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      lonOK = 0;
   }
   else
      active_longitude = lon;
    
   hVar = GetDlgItem(hDlg2, IDC_ELEV2);        // check elevation
   GetWindowText(hVar, strVar, 24);
   elev = atof(strVar);
   if (elev < 0 || elev > 500 || strcmp(strVar, "") == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      elevOK = 0;
   }
   else
      active_elev = elev;

   hVar = GetDlgItem(hDlg2, IDC_ASPECT2);     // check aspect
   GetWindowText(hVar, strVar, 24);
   aspect = atof(strVar);
   if (aspect < 0 || aspect > 360 || strcmp(strVar, "") == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      aspectOK = 0;
   }
   else
      active_aspect = aspect;

   hVar = GetDlgItem(hDlg2, IDC_TILT2);       // check tilt
   GetWindowText(hVar, strVar, 24);
   tilt = atof(strVar);
   if (tilt < 0 || tilt > 30 || strcmp(strVar, "") == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      tiltOK = 0;
   }
   else
      active_tilt = tilt;
    
   hVar = GetDlgItem(hDlg2, IDC_YEAR2);        // check year
   GetWindowText(hVar, strVar, 24);
   year = atoi(strVar);
   if (year < 1950 || year > 2050 || strcmp(strVar, "") == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      yearOK = 0;
   }
   else
   {
      active_year = year;
      SetMonthLengths(year);
   }
    
   hVar = GetDlgItem(hDlg2, IDC_STARTMONTH2);  // check start day
   startMonth = 1 + SendMessage(hVar, CB_GETCURSEL, 0, 0);   
   hVar = GetDlgItem(hDlg2, IDC_STARTDAY2);
   GetWindowText(hVar, strVar, 24);
   day = atoi(strVar);
   if (day < 1 || day > dayspermonth[startMonth - 1] ||
      strcmp(strVar, "") == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      startdayOK = 0;
   }
   else
      active_startday = day;
    
   hVar = GetDlgItem(hDlg2, IDC_STOPMONTH2);   // check stop day
   stopMonth = 1 + SendMessage(hVar, CB_GETCURSEL, 0, 0);   
   hVar = GetDlgItem(hDlg2, IDC_STOPDAY2);
   GetWindowText(hVar, strVar, 24);
   day = atoi(strVar);
   if (day < 1 || day > dayspermonth[stopMonth - 1] ||
      strcmp(strVar, "") == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      stopdayOK = 0;
   }
   else
      active_stopday = day;

   if (latOK && lonOK && startdayOK && stopdayOK && yearOK
         && elevOK && aspectOK && tiltOK)
   {
      if (lat > 43.7)
         northsouth = 1;
      else
         northsouth = 2;
      return 1;
   }
   else
      return 0;
}
