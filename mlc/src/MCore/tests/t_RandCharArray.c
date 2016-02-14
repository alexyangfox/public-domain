// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the property that it randomly initializes newly
		   extended areas.
  Doesn't test : Normal array and dynamic array operations which were
                   tested already.
  Enhancements : 
  History      : YeoGirl Yun                                   4/29/94
                   Initial revision(.c)
***************************************************************************/
#include <basics.h>
#include <RandCharArray.h>
#include <MLCStream.h>


void test_initialization()
{
   RandCharArray univDA(0,3242);

   univDA[300] = 'A'; // assignment causing extension.
   // must be initialized randomly
   Mcout << "Showing univDA - initialization : " <<  univDA << endl; 
}


main() {
   Mcout << "Testing RandCharArray.." << endl;
   test_initialization();
      
   return 0;
}





