// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test centerLine_true() function.
                 Link this outside CenterLine to get "Not
                 CenterLine" and in CenterLine to get "In CenterLine"
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                       8/19/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>

RCSID("MLC++, $RCSfile: t_centerline.c,v $ $Revision: 1.3 $")


main()
{
   if (centerline_true())
      cout << "In CenterLine" << endl;
   else
      cout << "Not CenterLine" << endl;

   return 0; // return success to shell
}   
