/* SouthGDD
 * Called by GetMonthsGDD. Calculates the increment of GDD
 * (growing degree days) that corresponds to the amount of radiation
 * falling on a (possibly tilted) square meter during one month.
 * month lies in [1, 12].
 */
 
#include "gddcalc.h"

float SouthGDD(float irr, int month)
{
   float gdd;
   float a[] = {-88.658, 0.80834, -0.001587, 1.3977e-06};
   float b[] = {-1568.5, 2.4629};
   float c[] = {141.06, -3.2648, 0.023786, -6.109e-05, 7.084e-08,
      -3.1199e-11};

   if (month < 7)
   {
      if (irr < 712.99)
         gdd = a[0] + a[1]*irr + a[2]*irr*irr + a[3]*pow(irr,3);
      else
         gdd = b[0] + b[1]*irr;
   }
   else
       gdd = c[0] + c[1]*irr + c[2]*irr*irr + c[3]*pow(irr,3)
          + c[4]*pow(irr,4) + c[5]*pow(irr,5);

/* In this case we do not correct GDD for field elevation. The correlation
 * between GDD and elevation for these 11 stations is either insignificant
 * or positive when examined month-by-month. The positive correlations
 * (which are never very great) may be due to the fact that stations with
 * the highest GDD tend to be more southerly, where it is hotter but where
 * the elevations are greater. In other words, the positive correlation of
 * GDD with elevation may be a latitude effect. 
 */
   if (gdd < 0) gdd = 0;
   return gdd;
}
