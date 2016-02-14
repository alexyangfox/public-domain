/* Initialize5
 * Called by Dlg5Proc in gddcalc. Assigns values to the variables
 * that define place and time. The values included here are from
 * the southern Willamette Valley near Eugene.
 */

#include "gddcalc.h"

void SetMonthLengths(int);        // prototype

void Initialize5()
{
   active_year = 2006;
   active_startmonth = 4;         // runs from 1 to 12
   active_startday = 1;           // runs from 1 to dayspermonth[month-1]
   active_stopmonth = 10;
   active_stopday = 31;
   active_latitude = 44.133;         // Eugene
   active_longitude = -123.214;      // Eugene
   active_elev = 114;                // Eugene
   active_aspect = 180;
   active_tilt = 0;
   active_timezone = -8;
   active_hour = 12;
   active_minute = 0;
   active_second = 0;
   active_dt = 1;
   active_temp = 15;
   active_press = 1013;
   northsouth = 1;
   
   SetMonthLengths(active_year);
   
   
      
   return;
}
