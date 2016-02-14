/* CalcDlg3Results
 * Called from Dlg3Proc in gddcalc. Calculates test results.
 * ShowDlg3Results will put them onscreen.
 */

#include "gddcalc.h"
#include "solpos.h"

int IsLeapYear(int);                 // prototype

void CalcDlg3Results()
{
   long retval;
   
/* The posdata structure is defined in solpos.h and pd is defined there
 * as a posdata structure. pdat is a pointer to pd. Here we set values
 * into some quantities that are input to solpos.
 */
   
   pdat = &pd;
   S_init(pdat);                     // default initialization of pdat
   pdat->latitude = active_latitude;
   pdat->longitude = active_longitude;
   pdat->aspect = active_aspect;
   pdat->tilt = active_tilt;
   pdat->year = active_year;      // not a leap year in this test
   pdat->month = active_month;
   pdat->day = active_day;
   pdat->hour = active_hour;
   pdat->minute = active_minute;
   pdat->second = active_second;
   pdat->temp = active_temp;
   pdat->press = active_press;
   pdat->timezone = active_timezone;
   pdat->daynum = active_daynum;

/* Make the calculations */

   retval = S_solpos(pdat);
   
   return;
}

