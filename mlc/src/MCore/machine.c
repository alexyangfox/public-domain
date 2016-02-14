// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Machine dependent routines.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : James Dougherty                               10/16/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <string.h>

RCSID("MLC++, $RCSfile: machine.c,v $ $Revision: 1.3 $")

void overlap_byte_copy(char* dest, char* src, int len)
{
#if defined(SUNOS)
   bcopy(src,dest,len);
#else
   memmove(dest,src,len);
#endif
}







