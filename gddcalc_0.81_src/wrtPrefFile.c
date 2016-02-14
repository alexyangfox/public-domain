/* WrtPrefFile
 * Takes the geographic descriptors that the user has entered
 * in Dialog 7 and writes them to a preferences file.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <winbase.h>
#include "gddcalc.h"
#include "resource.h"

char *GetRandomName(char *);

int WrtPrefFile(HWND hDlg7)
{
   char prefFileName[48];
	char strVar[24], outBfr[128], dateBfr[64];
   int latOK = 1, lonOK = 1, elevOK = 1, aspectOK = 1, tiltOK = 1;
   float lat, lon, elev, aspect, tilt;
   HANDLE hPrefFile;
	DWORD dwCount;
	SYSTEMTIME st;
	HWND hVar;

/* Open a file with a random name to receive the persistant descriptors.
 * If the descriptors all pass the tests it later will be renamed to
 * GddPrefFile.txt; if not it will be deleted and we will try again.
 */
   strcpy(prefFileName, GetRandomName("preffile"));
   hPrefFile = CreateFile(prefFileName,
      GENERIC_READ | GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
      FILE_ATTRIBUTE_NORMAL, NULL);

/* Write a header for the file */
	      
   GetLocalTime(&st);
   GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE,
      NULL, NULL, dateBfr, 63);
   sprintf(outBfr, "%s%d:%d, %s%s",
      "This Locale File was written at ", 
      st.wHour, st.wMinute, dateBfr,
      "\n===================================================================");
   WriteFile(hPrefFile, outBfr, strlen(outBfr), &dwCount, NULL);

/* Read the descriptors from Dialog 7 and check them for appropriateness.
 * If all of them are OK, send them to the preferences file. If there
 * are problems, notify the user and recycle.
 */
 
   hVar = GetDlgItem(hDlg7, IDC_LATITUDE7);   // check latitude   
   GetWindowText(hVar, strVar, 24);
   lat = atof(strVar);
   if (lat < 40 || lat > 55 || strlen(strVar) == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      latOK = 0;
   }
   else
   {
      strcpy(outBfr, "\n\nlatitude  ");
      strcat(outBfr, strVar);
      WriteFile(hPrefFile, outBfr, strlen(outBfr), &dwCount, NULL);
   } 
   
   hVar = GetDlgItem(hDlg7, IDC_LONGITUDE7);   // check longitude
   GetWindowText(hVar, strVar, 24);
   lon = atof(strVar);
   if (lon > 0)                         // If the user enters a
   {                                    // positive (east) longitude
      lon = -lon;                       // make it a negative (west)
      strcpy(outBfr, "-");              // latitude. We know he
      strcat(outBfr, strVar);           // means west.
      strcpy(strVar, outBfr);
      SetWindowText(hVar, strVar);
      UpdateWindow(hVar);
   }   
   if (lon < -124 || lon > -122 || strlen(strVar) == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      lonOK = 0;
   }
   else
   {
      strcpy(outBfr, "\nlongitude  ");
      strcat(outBfr, strVar);
      WriteFile(hPrefFile, outBfr, strlen(outBfr), &dwCount, NULL);
   } 
    
   hVar = GetDlgItem(hDlg7, IDC_ELEV7);        // check elevation
   GetWindowText(hVar, strVar, 24);
   elev = atof(strVar);
   if (elev < 0 || elev > 500 || strlen(strVar) == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      elevOK = 0;
   }
   else
   {
      strcpy(outBfr, "\nelevation  ");
      strcat(outBfr, strVar);
      WriteFile(hPrefFile, outBfr, strlen(outBfr), &dwCount, NULL);
   } 
    
   hVar = GetDlgItem(hDlg7, IDC_ASPECT7);     // check aspect
   GetWindowText(hVar, strVar, 24);
   aspect = atof(strVar);
   if (aspect < 0 || aspect > 360 || strlen(strVar) == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      aspectOK = 0;
   }
   else
   {
      strcpy(outBfr, "\naspect  ");
      strcat(outBfr, strVar);
      WriteFile(hPrefFile, outBfr, strlen(outBfr), &dwCount, NULL);
   } 

   hVar = GetDlgItem(hDlg7, IDC_TILT7);       // check tilt
   GetWindowText(hVar, strVar, 24);
   tilt = atof(strVar);
   if (tilt < 0 || tilt > 30 || strlen(strVar) == 0)
   {
      SetWindowText(hVar, "error");
      UpdateWindow(hVar);
      tiltOK = 0;
   }
   else
   {
      strcpy(outBfr, "\ntilt  ");
      strcat(outBfr, strVar);
      WriteFile(hPrefFile, outBfr, strlen(outBfr), &dwCount, NULL);
   } 

   CloseHandle(hPrefFile);

   if (latOK && lonOK && elevOK && aspectOK && tiltOK)
   {
      remove("Locale.txt");
      rename(prefFileName, "Locale.txt");
      return 1;
   }
   else
   {
      remove(prefFileName);
      return 0;
   }
}

