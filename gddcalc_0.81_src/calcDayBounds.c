/* CalcDayBounds
 * Calculates the times of sunrise and sunset on a given day,
 * month, and year.
 */

#include "gddcalc.h"
#include "solpos.h"

void CalcDayBounds(int day, int month, int year,
   float *sunrise, float *sunset)
{
   long retval;
   char bfr1[64];
   
/* The posdata structure is defined in solpos.h and pd is defined there
 * as a posdata structure. pdat is a pointer to pd.
 */
   
   pdat = &pd;
   S_init(pdat);                     // default initialization of pdat
   pdat->latitude = active_latitude;
   pdat->longitude = active_longitude;
   pdat->aspect = active_aspect;
   pdat->tilt = active_tilt;
   pdat->year = year;
   pdat->month = month;
   pdat->day = day;
   pdat->hour = active_hour;
   pdat->minute = active_minute;
   pdat->second = active_second;
   pdat->temp = active_temp;
   pdat->press = active_press;
   pdat->timezone = active_timezone;

   if (IsLeapYear(active_year))
      pdat->daynum = day + month_days[1][pdat->month];
   else
      pdat->daynum = day + month_days[0][pdat->month];

   retval = S_solpos(pdat);
      
   *sunrise = pdat->sretr;
   *sunset = pdat->ssetr;
   return;
}

