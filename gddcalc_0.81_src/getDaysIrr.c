/* GetDaysIrr
 * Called from ShowDaysIrr, ShowDaysGDD, GetMonthsIrr, and Works2.
 * Now we are ready to accumulate irradiance from sunrise to sunset.
 * The air mass correction for irradiation used here assumes that when
 * the sun is directly overhead the zenith air mass (S_ampress = 1)
 * reduces the incident energy to 70% of its extraterrestrial value.
 * See below for a justification of the factor 70%. Thus the amount
 * of energy that reaches the ground through a given air mass is equal
 * to [top of atmosphere energy] x 0.7^[air mass]. In the loop below,
 * solpos calculates etrtilt (extraterrestrial radiance, i.e., the no
 * atmosphere radiance, falling on a tilted surface). This quantity has
 * units of watts per sqm, or joules per sqm per sec. It is multiplied by
 * 60 to get joules per sqm per minute, and then by the number of minutes
 * in the time step (active_dt) to get the number of joules per sqm
 * during the step. Finally, it is divided by 1,000,000 to produce
 * megajoules per sqm during the time step.
 *
 *
 * Vignola found in 19?? that at Eugene the ratio of total radiation
 * falling on the earth's surface (direct + diffuse) to extraterrestrial
 * radiation is about 0.77. This should be reduced in line with a recent
 * finding that global dimming due to atmospheric pollution (about 10% in
 * 20??) is increasing. The 0.70 figure has been selected because it gives
 * results comparable to those obtained by Amin Pirzadeh in the Okanagan
 * Valley.
 */

#include <math.h>
#include "gddcalc.h"
#include "solpos.h"

void CalcDayBounds(int, int, int, float *, float *);
 
float GetDaysIrr(int day, int month, int year)
{
   long retval;
   float daysIrr;
   float minutes, startMinute, stopMinute, dt;

/* Calculate the times of sunrise (startMinute) and sunset (stopMinute) */
   
   CalcDayBounds(day, month, year, &startMinute, &stopMinute);
   stopMinute = stopMinute + 1;
   dt = (float)active_dt;

/* Accumulate the day's irradiance minute-by-minute. Note that most
 * of pdat need not be reinitialized for the calls to S_solpos below
 * because it is initialized in CalcDayBounds.
 */
   daysIrr = 0;
   minutes = startMinute;
   while (minutes < stopMinute)
   {
      pdat->hour = (int)(minutes/60);
      pdat->minute = (int)(minutes - (float)pdat->hour*60);
      retval = S_solpos(pdat);      
      daysIrr = daysIrr + pdat->etrtilt*pow(0.7, pdat->ampress)*60*dt;
      minutes = minutes + dt;
   }

/* Convert joules/sqm to megajoules/sqm */

   daysIrr = 0.000001*daysIrr;
   
   return daysIrr;
}
