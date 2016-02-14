/* MLC++ - Machine Learning Library in -*- C -*- (C code this time)
// See Descrip.txt for terms and conditions relating to use and distribution.
****************************************************************************
  Description  : Return true to centerline_true().
                 When this function is in the library, it will be taken
                   for code linked outside Centerline, but will not
                   be used for code linked in centerline.  Hence this can
                   provide run-time tests to see if we're running in
                   centerline.  See for example usage in error.c
                 Note that this is C code, so we don't include <basics.h>
                 Make sure all function arguments are defined below the 
                   function name (old C support for sun).
  Assumptions  :
  Comments     : See ObjectCenter Reference Manual on centerline_XXX().
  Complexity   : 
  Enhancements :
  History      : Ronny Kohavi                                       8/19/93
                   Initial revision
***************************************************************************/

/* Note no RCSID because this is compiled with cc, not CC */

int centerline_true()
{
   return (0);
}

int centerline_ignore(str)
char *str;
{
   (void)str;
   return 0;
}

int centerline_catch(str)
char *str;
{
   (void)str;
   return 0;
}
