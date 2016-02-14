/* StartDlg2Log
 * Called by Works2 when it starts Dialog 2. Various parameters
 * that remain constant during the dialog - such as latitude and
 * longitude - are recorded in the log.
 */
 
#include <windows.h> 
#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include "gddcalc.h"
 
 void StartDlg2Log()
 {
   extern HANDLE hLogfile;
   
   char bfr[256];
   char *monthname[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun",
      "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
   DWORD dwCountWritten;

   sprintf(bfr, "%s%s%s%7.4f%s%9.4f%s%6.1f%s",
      "\n\nSolar radiation and GDD during a range of days",
      "\n----------------------------------------------",
      "\n\n   Latitude  ", active_latitude,
      "\n   Longitude ", active_longitude,
      "\n   Elevation ", active_elev, " meters");
   WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
      
   sprintf(bfr, "%s%4.1f%s%4.1f",
      "\n   Tilt ", active_tilt,
      "\n   Aspect ", active_aspect);
   WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
      
   sprintf(bfr, "%s%2d %s %4d",
      "\n   Startdate ", active_startday,
      monthname[active_startmonth-1], active_year);
   WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
     
   sprintf(bfr, "%s%2d %s %4d",
      "\n   Stopdate ", active_stopday,
      monthname[active_stopmonth-1], active_year);
   WriteFile(hLogfile, bfr, strlen(bfr), &dwCountWritten, NULL);
            
   return;      
}
