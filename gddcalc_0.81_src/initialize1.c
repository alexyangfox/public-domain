/* initialize1.c
 * Called by gddcalc. If a preferences file exists the persistant
 * geographic values in that file are assigned to the active variable
 * set, and are copied into Dialog 1.
 */

#include <windows.h>
#include <stdio.h>
#include "resource.h"
#include "gddcalc.h"

float GetDescriptor(char *);

void Initialize1(HWND hDlg1)
{
	extern BOOL havePrefs;
   char bfr[128], null = '\0';
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

   active_timezone = -8;
   active_hour = 12;
   active_minute = 0;
   active_second = 0;
   active_dt = 1;
   active_temp = 15;
   active_press = 1013;

/* Fill in the text boxes of Dialog 1 (just the input boxes) */
   
   PopulateDialog1(hDlg1);
      
/* Make sure the output text boxes are empty */
   
   sprintf(bfr, "%c", null);
   hVar = GetDlgItem(hDlg1, IDC_SUNRISE1);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%c", null);
   hVar = GetDlgItem(hDlg1, IDC_SUNSET1);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%c", null);
   hVar = GetDlgItem(hDlg1, IDC_FULLIRR1);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);
   
   return;
}

