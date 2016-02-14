/* GetMonthsGDD
 * Called by Works2 and ShowDaysGDD. Calculates the increment of GDD
 * (growing degree days) that corresponds to the amount of radiation
 * falling on a (possibly tilted) square meter during one month.
 * month lies in [1, 12].
 */
 
#include "gddcalc.h"

float NorthGDD(float, int);
float SouthGDD(float, int);

float GetMonthsGDD(float irr, int month)
{
   float gdd;
   
   if (northsouth == 1)
      gdd = NorthGDD(irr, month);
   else if (northsouth == 2)   
      gdd = SouthGDD(irr, month);

   return gdd;   
}
