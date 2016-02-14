/* Initialize2
 * Called by gddcalc. If a preferences file exists the persistant
 * geographic values in that file are assigned to the active variable
 * set, and are copied into Dialog 2.
 */

#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include "gddcalc.h"

float GetDescriptor(char *);

void Initialize2(HWND hDlg2)
{
	extern BOOL havePrefs;
   char bfr[24], null = '\0';
   HWND hVar;
   FILE *fp;

   fp = fopen("Locale.txt", "r");

   if (fp == NULL)
      havePrefs = FALSE;
   else
   {
      havePrefs = TRUE;
      while (fgets(bfr, sizeof(bfr), fp))
      {
         if (bfr[0] == 'l' && bfr[1] == 'a' && bfr[2] == 't')
            active_latitude = GetDescriptor(bfr);
         else if (bfr[0] == 'l' && bfr[1] == 'o' && bfr[2] == 'n')
            active_longitude = GetDescriptor(bfr);
         else if (bfr[0] == 'e' && bfr[1] == 'l' && bfr[2] == 'e')
            active_elev = GetDescriptor(bfr);
         else if (bfr[0] == 'a' && bfr[1] == 's' && bfr[2] == 'p')
            active_aspect = GetDescriptor(bfr);
         else if (bfr[0] == 't' && bfr[1] == 'i' && bfr[2] == 'l')
            active_tilt = GetDescriptor(bfr);
      }
      fclose(fp);   
   }

   active_year = 2006;
   active_startmonth = 4;         // runs from 1 to 12
   active_startday = 1;           // runs from 1 to dayspermonth[month-1]
   active_stopmonth = 9;
   active_stopday = 30;
   active_timezone = -8;
   active_hour = 0;
   active_minute = 0;
   active_second = 0;
   active_dt = 1;
   active_temp = 15;
   active_press = 1013;

/* Fill in the text boxes of Dialog 2 (just the input boxes) */
   
   PopulateDialog2(hDlg2);
      
/* Make sure the output text boxes are empty */
   
   sprintf(bfr, "%c", null);
   hVar = GetDlgItem(hDlg2, IDC_OUTDAY2);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%c", null);
   hVar = GetDlgItem(hDlg2, IDC_OUTMONTH2);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%c", null);
   hVar = GetDlgItem(hDlg2, IDC_CORRIRR2);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%c", null);
   hVar = GetDlgItem(hDlg2, IDC_DEGDAYS2);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);
   
   return;
}

