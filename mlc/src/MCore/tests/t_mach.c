// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests machine.c functions
  Doesn't test :
  Enhancements :
  History      : James Dougherty                                     10/16/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <machine.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <string.h>

RCSID("MLC++, $RCSfile: t_mach.c,v $ $Revision: 1.4 $")

void test_overlap_byte_copy()
{
   char  buffer[100];
   char* str  = "ABC DEF GHI JKL";

   Mcout << str << endl;
   strcpy(buffer,str);

   overlap_byte_copy(buffer + 3, buffer + 7, 11);
   ASSERT(0 == strcmp(buffer, "ABC GHI JKL"));
   Mcout << buffer << endl;

   overlap_byte_copy(buffer + 3, buffer + 7, 5);
   ASSERT(0 == strcmp(buffer, "ABC JKL"));
   Mcout << buffer << endl;

   overlap_byte_copy(buffer, buffer + 4, 4);
   Mcout << buffer << endl;
   ASSERT(0 == strcmp(buffer, "JKL"));
}


main()
{
   Mcout << "t_machSunOS5.3.c ..." << endl;
   test_overlap_byte_copy();
   return 0; // return success to shell
}   
