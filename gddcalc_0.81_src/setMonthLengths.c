/* SetMonthLengths
 * Assigns the proper non-leap-year number of days to each
 * element of the dayspermonth array.
 */

#include "gddcalc.h"

int IsLeapYear(int);              // prototype

void SetMonthLengths(int year)
{   
   dayspermonth[0] = 31;
   
   if (IsLeapYear(year))          // make sure February has
      dayspermonth[1] = 29;       // 29 days during leap years
   else
      dayspermonth[1] = 28;

   dayspermonth[2] = 31;
   dayspermonth[3] = 30;
   dayspermonth[4] = 31;
   dayspermonth[5] = 30;
   dayspermonth[6] = 31;
   dayspermonth[7] = 31;
   dayspermonth[8] = 30;
   dayspermonth[9] = 31;
   dayspermonth[10] = 30;
   dayspermonth[11] = 31;

   return;   
} 
