/* ShowDayBounds
 * Called from Works1 (for Dialog 1) and from Dlg4Proc (for Dialog 4).
 * Calculates the times of sunrise and sunset, and places them in the
 * appropriate externals and in the dialog windows.
 */

#include <windows.h>
#include "resource.h"
#include "gddcalc.h"
#include "solpos.h"

void CalcDayBounds(int, int, int, float *, float *);

void ShowDayBounds(HWND hDlg)
{
   extern int logActive;
	extern HANDLE hLogfile;

   HWND hVar;
   char bfr[128], tstring[6];
   char sriseBfr[16], ssetBfr[16];
   char *zone[] = {"(EST)", "(CST)", "(MST)", "(PST)"};
   char *monthname[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
   int day, month, year;
   int i, hr, minutes, sec;
   long retval;
   float sunrise, sunset;
   DWORD dwCountWritten;

   day = active_day;
   month = active_month;
   year = active_year;

/* Calculate the times of sunrise and sunset in minutes past midnight */

   CalcDayBounds(day, month, year, &sunrise, &sunset);
  
/* Construct the string that indicates time zone */

   i = -active_timezone - 5;
   strcpy(tstring, zone[i]);

/* Put the sunrise and sunset times into the dialog window, and also
 * into externals for use in other routines.
 */

   active_sunrise = sunrise;
   hr = (int)(active_sunrise/60);
   minutes = (int)(active_sunrise - (float)(hr*60));
   sec = (int)((active_sunrise - (float)(hr*60) - (float)minutes)*60);
   sprintf(sriseBfr, "%2.2d:%2.2d:%2.2d %s", hr, minutes, sec, tstring);
//   sprintf(sriseBfr, "%f", sunrise);
   hVar = GetDlgItem(hDlg, IDC_SUNRISE1);
   SetWindowText(hVar, sriseBfr);
   UpdateWindow(hVar);
   
   active_sunset = sunset;
   hr = (int)(active_sunset/60);
   minutes = (int)(active_sunset - (float)(hr*60));
   sec = (int)((active_sunset - (float)(hr*60) - (float)minutes)*60);
   sprintf(ssetBfr, "%2.2d:%2.2d:%2.2d %s", hr, minutes, sec, tstring);
//   sprintf(ssetBfr, "%f", sunset);
   hVar = GetDlgItem(hDlg, IDC_SUNSET1);
   SetWindowText(hVar, ssetBfr);
   UpdateWindow(hVar);
   
/* If we are logging, send it to the logfile */

   if (logActive == 1)
   {
      sprintf(bfr, "%s%s%s%7.4f%s%9.4f",
         "\n\nSolar radiation during one day",
         "\n------------------------------",
         "\n\n   Latitude:  ", active_latitude,
         "\n   Longitude: ", active_longitude);
      WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
      
      sprintf(bfr, "%s%6.1f%s%s%4.1f%s%s%4.1f%s",
         "\n   Elevation: ", active_elev, " meters",
         "\n   Tilt: ", active_tilt, " degrees",
         "\n   Aspect: ", active_aspect, " degrees");
      WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
      
      sprintf(bfr, "%s%2d %s %4d",
         "\n   Date: ", active_day, monthname[active_month-1], active_year);
      WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
      
      sprintf(bfr, "%s%s%s%s",
         "\n   Sunrise at ", sriseBfr,
         "\n   Sunset at ", ssetBfr);
      WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
   }
   return;
}

