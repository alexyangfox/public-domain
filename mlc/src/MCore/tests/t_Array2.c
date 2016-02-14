// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests Array2.c by just instantiating and using a few templates
  Doesn't test :
  Enhancements :
  History      : Dave Manley                                       9/28/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <Array2.h>

RCSID("MLC++, $RCSfile: t_Array2.c,v $ $Revision: 1.8 $")


main()
{
   // test constructor, index, and basic accessor functions
   cout << "t_Array2.c executing" << endl;
   Array2<int> a(2,3,1,1,9);
   ASSERT (a.index(0) == 9);
   a.index(0) = 55;
   ASSERT (a.index(0) == 55);
   ASSERT (a.start_row() == 2);
   ASSERT (a.start_col() == 3);

   // check other type of constructors defaults
   Array2<int> b(9,4,7);
   ASSERT (b.num_rows() == 9);
   ASSERT (b.num_cols() == 4);
   ASSERT (b.start_row() == 0);
   ASSERT (b.start_col() == 0);

   // check assignment (deep copy check)
   Array2<int> c(2,3,1,1);
   ASSERT(a != c);
   ASSERT(c != a);
   c = a;
   ASSERT (c(2,3) == 55);
   ASSERT (c == a);
   ASSERT (a == c);
   c(2,3) = 33;
   ASSERT (c(2,3) == 33);
   ASSERT (a(2,3) == 55);
   ASSERT (c != a);
   ASSERT (a != c);

   Array2<int> x1(2,2,2,2,2);
   Array2<int> x2(2,2,2,3,2);
   ASSERT(x1 != x2); // different size shouldn't be equal


   Array2<int> errorArray(54,6,2,4);
   TEST_ERROR("Cannot assign array sized", errorArray = c);
   TEST_ERROR("out of bounds", (void)errorArray(54 - 1, 7));
   TEST_ERROR("out of bounds", (void)errorArray.index(-1));
   
   // check boundry cases
   Array2<int> d(1,3,12,8,2);
   for (int i = d.start_row(); i < d.num_rows() + d.start_row(); i++)
      for (int j = d.start_col(); j < d.high_col(); j++) 
	 d(i,j) = i-j;
   ASSERT(d(8,8) == 0 && 0 == d.index(61));
   ASSERT(d(4,4) == 0 && 0 == d.index(25));
   ASSERT(d(2,3) == -1 && -1 == d.index(8));
   for (i = d.start_row(); i < d.high_row(); i++)
      for (int j = d.start_col(); j < d.num_cols() + d.start_col(); j++) 
	 d(i,j) = i*j;
   ASSERT(d(8,8) == 64 && 64 == d.index(61));
   ASSERT(d(4,4) == 16 && 16 == d.index(25));
   ASSERT(d(2,3) == 6 && 6 == d.index(8));


   // test new "copy constructor"
   Array2<int> cc(c, ctorDummy);
   ASSERT (cc(2,3) == 33);

   
   cout << "Testing PtrArray.c..." << endl;
   {
      PtrArray2<char *> pa1(4,9);
      PtrArray2<char *> dummy(4,9);
      TEST_ERROR("Cannot assign to a PtrArray2", pa1 = dummy);
      ASSERT (pa1.index(0) == NULL);
      ASSERT (pa1(3,8) == NULL);
      for (i = 0; i < pa1.size(); i++){
	 pa1.index(i) = new char;
	 *(pa1.index(i)) = 'a';
      }
   }  
   
   return 0; // return success to shell
}
		  


