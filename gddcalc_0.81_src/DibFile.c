/* DibFile.c
 * Adapted from Charles Petzold's book.
 */

#include <windows.h>
#include "dibfile.h"

static OPENFILENAME ofn;

void DibFileInitialize (HWND hwnd)
{
     static TCHAR szFilter[] = TEXT ("Bitmap Files (*.BMP)\0*.bmp\0")  \
                               TEXT ("All Files (*.*)\0*.*\0\0");
     
     ofn.lStructSize       = sizeof (OPENFILENAME);
     ofn.hwndOwner         = hwnd;
     ofn.hInstance         = NULL;
     ofn.lpstrFilter       = szFilter;
     ofn.lpstrCustomFilter = NULL;
     ofn.nMaxCustFilter    = 0;
     ofn.nFilterIndex      = 0;
     ofn.lpstrFile         = NULL;      // Set in Open and Close functions
     ofn.nMaxFile          = MAX_PATH;
     ofn.lpstrFileTitle    = NULL;      // Set in Open and Close functions
     ofn.nMaxFileTitle     = MAX_PATH;
     ofn.lpstrInitialDir   = NULL;
     ofn.lpstrTitle        = NULL;
     ofn.Flags             = 0;         // Set in Open and Close functions
     ofn.nFileOffset       = 0;
     ofn.nFileExtension    = 0;
     ofn.lpstrDefExt       = TEXT ("bmp");
     ofn.lCustData         = 0;
     ofn.lpfnHook          = NULL;
     ofn.lpTemplateName    = NULL;
}

BITMAPFILEHEADER *DibLoadImage(PTSTR pstrFileName)
{
     BOOL               bSuccess;
     DWORD              dwFileSize, dwHighSize, dwBytesRead;
     HANDLE             hFile;
     BITMAPFILEHEADER * pbmfh;

     hFile = CreateFile (pstrFileName, GENERIC_READ, FILE_SHARE_READ, NULL,
                         OPEN_EXISTING, FILE_FLAG_SEQUENTIAL_SCAN, NULL);

     if (hFile == INVALID_HANDLE_VALUE)
          return NULL;

     dwFileSize = GetFileSize (hFile, &dwHighSize);

     if (dwHighSize)
     {
          CloseHandle (hFile);
          return NULL;
     }

     pbmfh = malloc (dwFileSize);

     if (!pbmfh)
     {
          CloseHandle (hFile);
          return NULL;
     }

     bSuccess = ReadFile (hFile, pbmfh, dwFileSize, &dwBytesRead, NULL);
     CloseHandle (hFile);

     if (!bSuccess || (dwBytesRead != dwFileSize)         
                   || (pbmfh->bfType != * (WORD *) "BM") 
                   || (pbmfh->bfSize != dwFileSize))
     {
          free (pbmfh);
          return NULL;
     }
     return pbmfh;
}
