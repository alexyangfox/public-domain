/* Logging
 * Springs into action when the user presses "Start logging" or
 * "Save the log" in the application's File menu.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <winbase.h>
#include "gddcalc.h"
#include "resource.h"

int CALLBACK Dlg6Proc(HWND, UINT, WPARAM, LPARAM);
char *GetRandomName(char *);

void Logging(HINSTANCE hInstance, HWND hWnd, HANDLE hMenu, int logFlag)
{
	extern int logActive, logSaved;
	extern char logfileName[48];
	extern HANDLE hLogfile;

	char bfr[24], outBfr[128], dateBfr[64];
	char prefix[] = {"logfile."}; 
	int i, r1, r2, r3;
	DWORD dwCountWritten;
	SYSTEMTIME st;
		
	switch(logFlag)
	{
      case LOGSTART:

/* We open a file with a temporary (random) name. Later, inputs and
 * outputs from Dialog 1 and Dialog 2 will be written into that file.
 * Eventually it will be given a permanent name supplied by the user.
 */
         strcpy(logfileName, GetRandomName("logfile"));
         hLogfile = CreateFile(logfileName, GENERIC_READ | GENERIC_WRITE,
                      0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL,
                      NULL);

/* Write a header for the log */
	      
         GetLocalTime(&st);
         GetDateFormat(LOCALE_USER_DEFAULT, DATE_LONGDATE,
                       NULL, NULL, dateBfr, 63);
         sprintf(outBfr, "%s%d:%d, %s%s",
            "This log was started at ", 
            st.wHour, st.wMinute, dateBfr,
            "\n=====================================================");
         WriteFile(hLogfile, outBfr, strlen(outBfr), &dwCountWritten, NULL);
         EnableMenuItem(hMenu, IDM_START_LOG, MF_GRAYED);
         EnableMenuItem(hMenu, IDM_SAVE_LOG, MF_ENABLED);
         logActive = 1;
         break;

/* We come here when the user clicks on the "Save the Log" menu item */

      case LOGSAVE:
         DialogBox(hInstance, TEXT ("DIALOG6"), hWnd, Dlg6Proc);
         if (logSaved)
         {
            EnableMenuItem(hMenu, IDM_START_LOG, MF_ENABLED);
            EnableMenuItem(hMenu, IDM_SAVE_LOG, MF_GRAYED);
            logActive = 0;
            logSaved = 0;
         }
         break;
   }   
   return;
}

int CALLBACK Dlg6Proc(HWND hDlg6, UINT message, WPARAM wParam, LPARAM lParam)
{
	extern HANDLE hLogfile;
	extern char logfileName[48];
   extern int logSaved;
   static POINT outline[5] = {0,0, 1000,0, 1000,1000, 0,1000, 0,0};
	TCHAR newfileName[64];
   HWND hVar;
   HDC hDC;
   PAINTSTRUCT ps;
   COLORREF clrYellow = RGB(250, 250, 140);
   HBRUSH hBrush;
   
   switch (message)
   {
      case WM_INITDIALOG:
         hVar = GetDlgItem(hDlg6, IDC_SAVENAME6);
         SetFocus(hVar);
         break;

      case WM_PAINT:
         hDC = BeginPaint(hDlg6, &ps);
         SetBkMode(hDC, TRANSPARENT);
         hBrush = CreateSolidBrush(clrYellow);
         SelectObject(hDC, hBrush);
         Polygon(hDC, outline, 5);
         DeleteObject(hBrush);
         EndPaint(hDlg6, &ps);

      case WM_CTLCOLORSTATIC:
         SetBkColor((HDC)wParam, clrYellow);
         SetTextColor((HDC)wParam, RGB(0, 0, 0));
         return (LRESULT)CreateSolidBrush(clrYellow);
          
      case WM_COMMAND:
         switch (LOWORD (wParam))
         {
            case IDC_CONTINUE6:
               hVar = GetDlgItem(hDlg6, IDC_SAVENAME6);
               GetWindowText(hVar, newfileName, 64);
               CloseHandle(hLogfile);
               if (rename(logfileName, newfileName))
               {
                  MessageBox(hDlg6,
                     "That file name appears already to be in use.\n" \
                     "Try another.",
                     "Error",
                     MB_OK);
               }
               else
               {                                                                         
                  EndDialog (hDlg6, 0);
                  logSaved = 1;
                  return 0;
               }
         }
   }
   return 0;
}
