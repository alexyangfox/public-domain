/* ShowDaysIrr
 * Called from Works1 (for Dialog 1) and from Dlg4Proc (for Dialog 4).
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

#include <windows.h>
#include "gddcalc.h"
#include "solpos.h"
#include "resource.h"

float GetDaysIrr(int, int, int);
 
void ShowDaysIrr(HWND hDlg1)
{
   extern int logActive;
	extern HANDLE hLogfile;

   char bfr1[24], bfr2[128];
   int day, month, year;
   float daysIrr;
   DWORD dwCountWritten;
   HWND hVar;
   
   day = active_day;
   month = active_month;
   year = active_year;   
   daysIrr = GetDaysIrr(day, month, year);

/* Send the accumulated irradiance to the Dialog */
      
   sprintf(bfr1, "%6.2f mj/sqm", daysIrr);
   hVar = GetDlgItem(hDlg1, IDC_FULLIRR1);
   SetWindowText(hVar, bfr1);
   UpdateWindow(hVar);

/* If we are logging, send it to the logfile */

   if (logActive == 1)
   {           
      sprintf(bfr2, "%s%s", "\n   Accumulated irradiance ", bfr1); 
      WriteFile(hLogfile, bfr2, strlen(bfr2), &dwCountWritten, NULL);      
   }
   return;
}
