/* GetRandomName
 * Called from Logging and from WrtPrefFile. Generates a filename
 * of the form x.y where x is an input string and y is an 8-digit
 * random number.
 */

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <winbase.h>
 
char *GetRandomName(char *leftStr)
{
	char tmpBfr[12], fileName[24];
	int i, r1, r2, r3;
    
/* Construct a 10-digit random integer and copy the lowest order
 * 8 digits into tmpBfr.
 */
   srand(GetTickCount());
   r1 = rand();
   r2 = rand();
   r3 = r1*r2;
   sprintf(tmpBfr, "%10d", r3);

   for (i = 0; i < 10; i++)
      if (tmpBfr[i] == ' ') tmpBfr[i] = '0';

   for (i = 0; i < 9; i++)
      tmpBfr[i] = tmpBfr[i + 2];

/* Form a filename of the form leftStr.xxxxxxxx, where xxxxxxxx is
 * an 8-digit random integer.
 */
   strcpy(fileName, leftStr);
   strcat(fileName, ".");
   strcat(fileName, tmpBfr);

   return fileName;
}

