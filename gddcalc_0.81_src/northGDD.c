/* NorthGDD
 * Called by GetMonthsGDD. Calculates the increment of GDD
 * (growing degree days) that corresponds to the amount of radiation
 * falling on a (possibly tilted) square meter during one month.
 * month lies in [1, 12].
 */
 
#include "gddcalc.h"

float NorthGDD(float irr, int month)
{
   float gdd;
   float avg_elev = 116;
   float a[] = {-13.975, 0.10023, 0.15769e-03};
   float b[] = {57.618, -0.98467, 0.0055579, -1.0617e-05, 7.1208e-09};
   float c[] = {-51.943, 0.40056, 0.0011463, -1.4594e-06};
   float m[] = {0.0, -0.016221, -0.092232, -0.12081, -0.13742, -0.19295,
      -0.26874, -0.24694, -0.14155, -0.062668, -0.037714, 0.0};

   if (month < 7)
   {
      if (irr < 637.2)      // The a & b curves cross at irr = 637.2
         gdd = a[0] + a[1]*irr + a[2]*irr*irr;
      else
         gdd = b[0] + b[1]*irr + b[2]*irr*irr + b[3]*pow(irr,3) + b[4]*pow(irr,4);
   }
   else
   {
       gdd = c[0] + c[1]*irr + c[2]*irr*irr + c[3]*pow(irr,3);
   }

/* Correct GDD for field elevation. 116 meters is the average elevation
 * of the temperature stations used in forming the relationship between GDD
 * and irradiance. The correction is found by plotting GDD against irradiance
 * for the 21 stations used in the GDD estimates, separately for each month,
 * and then fitting a straight line to each plot. m[] is the slope of that
 * fit. 
 */
   gdd = gdd + (active_elev - avg_elev)*m[month - 1];

   if (gdd < 0) gdd = 0;
   
   return gdd;
}        
