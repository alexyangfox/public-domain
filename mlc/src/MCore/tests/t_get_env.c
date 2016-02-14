// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : tests the function in get_env.c
  Doesn't test :
  Enhancements :
  History      : Dave Manley                                       9/27/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>

RCSID("MLC++, $RCSfile: t_get_env.c,v $ $Revision: 1.8 $")


main()
{
   cout << "t_get_env.c executing" << endl;

   MString retval;
   retval = get_env_default("anserine", "test");
   ASSERT(retval == "test");

   MString tst = getenv("MLCDIR");
   retval = get_env_default("MLCDIR", "Bart H. Stroustrup");
   ASSERT(tst == retval);

   // make sure that get_env_default returns different strings
   // for same environment variable.
   MString tst2 = get_env_default("MLCDIR", "asdf");
   ASSERT(&tst2 != &tst);
   
   retval = get_env("MLCDIR");
   ASSERT(tst == retval);

#ifndef MEMCHECK
   TEST_ERROR("get_env: Environment variable FOORONNYK undefined",
              get_env("FOORONNYK"));
#endif
   
   // Make sure empty environment variable is the same as undefined.
   ASSERT(putenv("LOGLEVEL=") == 0);
   ASSERT(get_env_default("LOGLEVEL", "3") == "3");


   return 0; // return success to shell
}   
