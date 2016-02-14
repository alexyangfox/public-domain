// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Checks that loops are detected in fatal_error routine.
                 This is not the main tester.  t_error.c is.
  Doesn't test : 
  Enhancements : 
  History      : Richard Long                                      10/13/93
                   Initial revision
***************************************************************************/


#include <basics.h>
#include <iostream.h>
#include <errorUnless.h>

extern int errLineWidth = 80; 

RCSID("MLC++, $RCSfile: t_error2.c,v $ $Revision: 1.4 $")

/***************************************************************************
  Description : Overwrite the MLClib fatal_abort function so that it now
                  starts a fatal_error loop.
  Comments    :
***************************************************************************/
ostream& fatal_abort(ostream& err2)
{
   err << "t_error2.c::fatal_abort: Start fatal_error loop" << fatal_error;
   return err2; // never reached, but it pleases the compiler
}


main()
{
   cout << "t_error2 running" << endl;
   err << "t_error2" << fatal_error;
}
