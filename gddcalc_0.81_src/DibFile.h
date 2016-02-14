/* DibFile.h
 * This is a header file for DibFile.c.
 * Adapted from Charles Petzold's book.
 */
 
void DibFileInitialize(HWND hwnd);
BITMAPFILEHEADER *DibLoadImage(PTSTR pstrFileName);
