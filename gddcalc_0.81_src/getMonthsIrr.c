/* GetMonthsIrr
 * Calculates the total corrected irradiance for a given month.
 * Called by Works2, Works5, and ShowDaysGDD.
 */
 
#include "gddcalc.h"

float GetDaysIrr(int, int, int);

float GetMonthsIrr(int month)
{
   int day, year;
   float daysIrr, monthsIrr;
   
   year = active_year;
   monthsIrr = 0;
   for (day = 1; day <= dayspermonth[month-1]; day++)
   {
      daysIrr = GetDaysIrr(day, month, year);
      monthsIrr = monthsIrr + daysIrr;
   }

   return monthsIrr;   
}
