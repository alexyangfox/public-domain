// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the new_handler implemented in new_safe.
  Doesn't test :
  Enhancements :
  History      : Richard Long                                      10/13/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>

RCSID("MLC++, $RCSfile: t_safe_new.c,v $ $Revision: 1.2 $")


main()
{
   cout << "t_safe_new executing" << endl;

   // Interestingly, I had TRUE in the while loop, and the SGI
   // compiler declared the error message that TEST_ERROR has
   // after the statement as unreachable (correct)!
   TEST_ERROR("safe_new.c::out_of_memory_handler: Out of memory",
      int i = 0; while(i < INT_MAX) { char* big = new char[INT_MAX]; i++; });
   
   return 0; // return success to shell
}   
