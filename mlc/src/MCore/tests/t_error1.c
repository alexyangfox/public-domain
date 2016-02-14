// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Check the fatal_error routine on short message after long
                 This is not the main tester.  t_error.c is.
  Doesn't test : 
  Enhancements : 
  History      : Ronny Kohavi                                        8/25/93
                   Initial revision

***************************************************************************/


#include <basics.h>
#include <iostream.h>
#include <errorUnless.h>

RCSID("MLC++, $RCSfile: t_error1.c,v $ $Revision: 1.5 $")


main()
{
   Mcout << "t_error1 running" << endl;
   TEST_ERROR("long message", err << "This is a long message which "
              "should be long enough so that if there is a problem "
              "with EOS, we will find out" << fatal_error);
   err << "short message" << fatal_error;
   ASSERT(FALSE); // should never get here.
}
