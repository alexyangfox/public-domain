// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test AugCategory
  Doesn't test :
  Enhancements :
  History      : Chia-Hsin Li                                       9/26/94
                   Added operator== test
                 Ronny Kohavi                                       9/13/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <AugCategory.h>
#include <MLCStream.h>

RCSID("MLC++, $RCSfile: t_AugCategory.c,v $ $Revision: 1.13 $")

AugCategory* init()
{
   AugCategory* ac = new AugCategory(3 + FIRST_CATEGORY_VAL, "Three");
   return ac;
}

main()
{
   cout << "t_AugCategory executing" << endl;
   AugCategory* ac = init();
   ASSERT(ac->num() == 3 + FIRST_CATEGORY_VAL);
   ASSERT(ac->description() == "Three");
   ASSERT(*ac == 3 + FIRST_CATEGORY_VAL);  // using operator Category()
   
#ifndef MEMCHECK
   TEST_ERROR("out of range", new AugCategory(UNKNOWN_CATEGORY_VAL - 2, 
                                              "bad"));
#endif

   Mcout <<"Printing ac gives " << *ac << endl;
   delete ac;

   // Test for operator==
   AugCategory *ac1 = new AugCategory(1 + FIRST_CATEGORY_VAL, "One");
   AugCategory *ac2 = new AugCategory(2 + FIRST_CATEGORY_VAL, "Two");

   ASSERT(!(*ac1 == *ac2)) ;
   ASSERT(*ac1 == *ac1);
   delete ac1;
   delete ac2;  
   return 0; // return success to shell
}   
