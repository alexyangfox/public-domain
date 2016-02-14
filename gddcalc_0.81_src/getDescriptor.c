/* GetDescriptor
 * Called by Initialize1 and Initialize2. On entry, bfr contains
 * a line of characters read from Locale.txt. The line will
 * consist of an initial tag (latitude, longitude, etc.) followed
 * by some blanks and then a number. This routine returns the
 * floating point number.
 */

#include <string.h>
#include <stdlib.h>
 
float GetDescriptor(char *bfr)
{
   int i, j, k, lth;
   
   lth = strlen(bfr);
   for (i = 3; i < lth; i++)
      if (bfr[i] == ' ') break;
   for (j = i+1; j < lth; j++)
      if (bfr[j] != ' ') break;

   i = 0;
   for (k = j; k < lth; k++)      
      bfr[i++] = bfr[k];

   bfr[i] = '\0';
   
   return atof(bfr);
}
