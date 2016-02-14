/* InitializeAtlanta
 * Called by Dlg3Proc and Dlg4Proc in gddcalc.c. Assigns values to some
 * of the variables declared in Dialog3. The values included here are the
 * test parameters provided by the author of solpos. They are valid for
 * Atlanta on 22 July 1999 at 09:45:37.
 */

#include "gddcalc.h"

void InitializeAtlanta()
{
      active_year = 1999;
      active_month = 7;
      active_day = 22;
      active_latitude = 33.65;
      active_longitude = -84.43;
      active_aspect = 135;
      active_tilt = 33.65;
      active_timezone = -5;
      active_hour = 9;
      active_minute = 45;
      active_second = 37;
      active_dt = 1;
      active_temp = 27;
      active_press = 1006;
      active_daynum = 203;
}

/* Note that when Dialog 4 uses this subroutine it will find slightly
 * different values for sunrise and sunset if we use different values
 * (0, say) for active_hour, active_minute, and active_second. One would
 * expect sunrise and sunset to be insensitive to those three parameters,
 * but for some reason they are not.
 */
