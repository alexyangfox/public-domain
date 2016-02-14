// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Check the fatal_error routine.
  Doesn't test : Short message after long one (end-of-string problem).
                 This is tested in t_error1.c
  Enhancements : 
  History      : Ronny Kohavi                                        7/17/93
                   Testing new error catching capability
                 Ronny Kohavi                                        7/13/93
                   Initial revision

***************************************************************************/


#include <basics.h>
#include <iostream.h>
#include <errorUnless.h>

RCSID("MLC++, $RCSfile: t_error.c,v $ $Revision: 1.15 $")


void foo(int i)
{
   switch(i) {
   // messages should be big because there was a flushing problem
   case 1: err << "t_error::foo: bugger 1234567890123456789012345"
               << fatal_error; break;
   case 2: err << "Ronny::Kohavi: test"  << fatal_error; break;
   }
   ASSERT(FALSE);
}

void bar()
{
  NOT_IMPLEMENTED
}

void test_is_valid_ptr()
{
   int a = 3;
   int* b = &a;

   ASSERT(is_valid_pointer(b,FALSE));
   b = (int *)4;
   ASSERT(!is_valid_pointer(b,FALSE));   
   b = (int *)((char *)&a + 1);
   ASSERT(!is_valid_pointer(b,FALSE));
   TEST_ERROR("is invalid", is_valid_pointer(b,TRUE));
}
   

main()
{
   Mcout << "Test 1" << endl;
   TEST_ERROR("t_error::foo: bugger", foo(1));
   Mcout << "Test 2" << endl;
   TEST_ERROR("Kohavi", foo(2));
   TEST_ERROR("Unimplemented function called", bar());

   test_is_valid_ptr();

   err << "Program should abort with this long message which"
          " is more than 160 characters long so it should wrap at"
          " least two lines because the default line width is set"
          " to 80 characters and this is more" << fatal_error;
   ASSERT(FALSE); // should never get here.
}
