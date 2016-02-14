/* ShowDlg3Results
 * Called by Dlg3Proc to put the results calculated by CalcDlg3Results
 * onscreen.
 */

#include <windows.h>
#include <math.h>
#include <string.h>
#include <stdio.h>
#include "resource.h"
#include "gddcalc.h"
#include "solpos.h"

void ShowDlg3Results(HWND hDlg3)
{
   char bfr[24];
   pdat = &pd;
   HWND hVar;

   sprintf(bfr, "%.4f  (97.0329)", pdat->azim);
   hVar = GetDlgItem(hDlg3, IDC_AZIM);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);
     
   sprintf(bfr, "%.5f  (0.91257)", pdat->cosinc);
   hVar = GetDlgItem(hDlg3, IDC_COSINC);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);
   
   sprintf(bfr, "%.4f  (48.4099)", pdat->elevref);
   hVar = GetDlgItem(hDlg3, IDC_ELEVREF);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);
   
   sprintf(bfr, "%.3f  (989.669)", pdat->etr);
   hVar = GetDlgItem(hDlg3, IDC_ETR);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);
   
   sprintf(bfr, "%.2f  (1323.24)", pdat->etrn);
   hVar = GetDlgItem(hDlg3, IDC_ETRN);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%.2f  (1207.55)", pdat->etrtilt);
   hVar = GetDlgItem(hDlg3, IDC_ETRTILT);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%.5f  (1.03704)", pdat->prime);
   hVar = GetDlgItem(hDlg3, IDC_PRIME);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%.5f  (1.20191)", pdat->sbcf);
   hVar = GetDlgItem(hDlg3, IDC_SBCF);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%.3f  (347.173)", pdat->sretr);
   hVar = GetDlgItem(hDlg3, IDC_SUNRISE);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%.2f  (1181.11)", pdat->ssetr);
   hVar = GetDlgItem(hDlg3, IDC_SUNSET);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%.5f  (0.96428)", pdat->unprime);
   hVar = GetDlgItem(hDlg3, IDC_UNPRIME);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   sprintf(bfr, "%.4f  (41.5901)", pdat->zenref);
   hVar = GetDlgItem(hDlg3, IDC_ZENREF);
   SetWindowText(hVar, bfr);
   UpdateWindow(hVar);

   return;    
}
