// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests BoolArray constructors and printing.  All other
		   BoolArray functions are tested in the Array tester.
  Doesn't test :
  Enhancements :
  History      : Brian Frasca                                       5/07/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <BoolArray.h>

RCSID("MLC++, $RCSfile: t_BoolArray.c,v $ $Revision: 1.6 $")

/***************************************************************************
  Description : Tests that the BoolArray(const Array<int>& source) is working
                  correctly.
  Comments    :
***************************************************************************/
void bool_array_from_int_array()
{
   Array<int> ai(1000);

   for(int i = 0; i < ai.size(); i++)
      ai.index(i) = i % 10;

   BoolArray b(ai);

   for(i = 0; i < ai.size(); i++)
      if(ai.index(i))
	 ASSERT(b.index(i));

   ASSERT(b.size() == ai.size());
}


main()
{
   bool_array_from_int_array();
   
   BoolArray a(3);
   //a[0]=3; a[1]=2;
   a[0]=TRUE; a[1]=FALSE; a[2]=TRUE;
   Mcout << a << endl;
   Mcout << "a = " << a << endl;

   Mcout << "t_BoolArray executing" << endl;

   // Test num_true and num_false
   ASSERT(a.num_true() == 2);
   ASSERT(a.num_false() == 1);

   // Test constructors.
   BoolArray ba1(0,5);
   BoolArray ba2(5);
   BoolArray ba3(0,5,TRUE);
   BoolArray ba4(ba1,ctorDummy);

   // Test display.
   Mcout << "BoolArray = " << ba3 << endl;
   Mcout << ba3 << endl;
   const BoolArray& ba5 = ba3;
   Mcout << "BoolArray = " << ba5 << endl;

   // Test get_true_indexes().
   BoolArray ba6(0,5);
   ba6[0] = ba6[1] = ba6[3] = TRUE;
   ba6[2] = ba6[4] = FALSE;
   ASSERT(ba6.get_true_indexes() == "0, 1, 3");
   ba6[0] = FALSE;
   ba6[4] = TRUE;
   ASSERT(ba6.get_true_indexes() == "1, 3, 4");

   return 0; // return success to shell
}


