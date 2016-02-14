// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Just allocates a bunch of arrays and tests various operations
  Doesn't test : 
  Enhancements : 
  History      :   James Dougherty                                   5/12/94
                   Dave Manley                                       9/14/93
                   Initial revision
***************************************************************************/

#ifdef __OBJECTCENTER__
const int NUM_SORTED_ELEM = 10;  // Number of elements to sort
#else
const int NUM_SORTED_ELEM = 50000;
#endif

#include <basics.h>
#include <math.h>
#include <errorUnless.h>
#include <MRandom.h>
#include <Array.h>
#include <MLCStream.h>

RCSID("MLC++, $RCSfile: t_Array.c,v $ $Revision: 1.25 $")

const int LOW = 4;
const int HIGH = 10;
const int NUM = 10;
const int MIDDLE = 5;

// These are the ranges for the random numbers returned by MRandom
//  for the sorting tests. 

const int LOWER_BOUND = -1000;
const int UPPER_BOUND =  1000;

//  doubleInt is used to see if there are no problems instantiating
//   structures that do not have operator< defined
struct doubleInt {
  int a;
  int b;
};

// doubleIntWithOpGt is used to see if there are no problems with
//   structures that have operator< defined
class doubleIntWithOpLt{
public:
   int a;
   int b;

   int operator <(const doubleIntWithOpLt& t) const {
      return b < t.b || (b == t.b && a < t.a);
   }

   int operator ==(const doubleIntWithOpLt& t) const{
      return b == t.b && a == t.a;
   }

};

void test_basic_array()
{
   Mcout << "Testing t_Array.c..." << endl;
   
   Array<char> a1(LOW, HIGH - LOW + 1, 'a');
   if (a1.low() != LOW || a1.high() != HIGH)
      err << "size accessors report failure" << fatal_error;
   for (int i = a1.low(); i <= a1.high(); i++)
      if (a1[i] != 'a') 
	 err << "t_Array.c: 3 arg initialization failed" << fatal_error;
   
   Array<char> a3(LOW, NUM, 'a');
   for (i = 0; i < HIGH; i++)
      if (a3.index(i) != 'a') 
	 err << "index() failed" << fatal_error;

   a1.index(0) = 'b';
   a1[HIGH] = 'z';
   if (a1[LOW] != 'b' || a1.index(a1.size()-1) != 'z')
      err << "t_Array.c: Assignment failed" << fatal_error;

   Mcout << "Testing PtrArray.c..." << endl;
   PtrArray<char *> pa1(LOW,HIGH);
   ASSERT (pa1.index(0) == NULL);
   ASSERT (pa1[HIGH] == NULL);
   for (i = 0; i < pa1.size(); i++){
      pa1.index(i) = new char;
      *(pa1.index(i)) = 'a';
   }
      
   Mcout << "Testing output and equality" << endl;
   Array<int> a(1,3,15);
   Array<int> b(1,3,15);
   
   Mcout << "Array of 15s: " << a << endl;
   
   ASSERT(a==b);
   a[3] = 3;
   ASSERT(a!=b && b!= a);
   b[3] = 3;
   ASSERT(a==b && b== a);
   
   a[1] = 13; a[2] = 14; a[3] = 15;
   Mcout << "Array of 13,14,15:" << a << endl;
   
   ASSERT(a.min() == 13); // first
   a[1] = 16;
   ASSERT(a.min() == 14); // middle
   a[2] = 17;
   ASSERT(a.min() == 15); // last
   
   ASSERT(a.max() == 17); // middle
   a[2] = 14;
   ASSERT(a.max() == 16); // first
   a[1] = 13;
   ASSERT(a.max() == 15); // last
   
   // output of Pointer array:
   
   PtrArray <char *> pointerOutputArray(6);
   char* character1 = new char[1]; *character1 = 'M';
   char* character2 = new char[1]; *character2 = 'L';
   char* character3 = new char[1]; *character3 = 'C';
   char* character4 = new char[1]; *character4 = '+';
   char* character5 = new char[1]; *character5 = '+';
   pointerOutputArray[3] = character4;
   pointerOutputArray[1] = character2;
   pointerOutputArray[0] = character1;
   pointerOutputArray[2] = character3;
   pointerOutputArray[pointerOutputArray.size()-1] = character5;
   
   Mcout << "Small MLC++ pointer array:  " << pointerOutputArray <<  endl;

   // Test if zero sized arrays are allowed
   Array<char> zeroArray(-5,0);
   Array<char> zeroArray2(0);
   
   TEST_ERROR("Illegal bounds requested", Array<int> negArray(3, -1));

   // Negative based arrays are ok:
   Array<char> negArray(-99, 3);
   
   Array<char> a_error(73);  
      TEST_ERROR("Cannot assign array sized", a_error = a1);

   Array<char> a2(a1, ctorDummy); // copy constructor
   a2[MIDDLE] = 'c';
   if (a1[MIDDLE] != 'a' || a2[MIDDLE] != 'c' || a1.high() != a2.high())
   err << "t_Array.c Copy on operator= failed" << fatal_error;

   // test "extra-arg" copy constructor.
   Array<char> aCC(a1, ctorDummy);
   aCC[MIDDLE] = 'c';
   if (a1[MIDDLE] != 'a' || aCC[MIDDLE] != 'c' || a1.high() != aCC.high())
   err << "t_Array.c Copy on operator= failed" << fatal_error;
  
   TEST_ERROR("out of bounds", (void)a1[HIGH +1]);
   TEST_ERROR("out of bounds", (void)a1[LOW - 1]);
   TEST_ERROR("out of bounds", (void)a1.index(-1));
   TEST_ERROR("out of bounds", (void)a1.index(HIGH-LOW+2));
}

void test_sort()
{
   Mcout << "Testing array sorting ..." <<endl;
   MRandom mrandom(7258789);
   Array<long> sortItems(NUM_SORTED_ELEM);

   for(int k = 0; k < NUM_SORTED_ELEM; k++)
      sortItems[k] = mrandom.long_int(LOWER_BOUND,UPPER_BOUND);
   sortItems.sort();
   
   for( k = 0; k < NUM_SORTED_ELEM - 1; k++)
      ASSERT(sortItems[k] <= sortItems[k+1]);
   Mcout << "Basic sort passed." << endl;
   
   Mcout << "Now testing pointer array sorting ..." << endl;
   PtrArray<int*> intPtrArray(NUM_SORTED_ELEM);

   for(k = 0; k < NUM_SORTED_ELEM; k++){
      intPtrArray[k] = new int;
      *intPtrArray[k] = mrandom.integer(0,UPPER_BOUND);
   }

#ifdef __OBJECTCENTER__
   Mcout << intPtrArray << endl;
#endif

   intPtrArray.sort();

#ifdef __OBJECTCENTER__
   Mcout << intPtrArray << endl;
#endif

   for( k = 0; k < NUM_SORTED_ELEM - 1; k++)
      ASSERT( *intPtrArray[k] <= *intPtrArray[k+1]);
   Mcout << "Pointer Array sort passed." << endl;

   
   Mcout << "Testing sort w/ user defined type and op < defined..." << endl;
   Array<doubleIntWithOpLt> diArray(NUM_SORTED_ELEM);

   for(k = 0; k < NUM_SORTED_ELEM; k++) {
      diArray[k].b = mrandom.integer(LOWER_BOUND,UPPER_BOUND);
      diArray[k].a = UPPER_BOUND - diArray[k].b;
   }

   diArray.sort();
   for(k = 0; k < NUM_SORTED_ELEM - 1; k++) {
      ASSERT(diArray[k].b <= diArray[k+1].b); 
      ASSERT(diArray[k].a >= diArray[k+1].a);
      ASSERT(diArray[k].a == UPPER_BOUND - diArray[k].b);
   }
   Mcout << "Sorting tests passed." << endl;
}



void test_int_array()
{
   Array<int> intArray1(0, 1000);
   for(int i = intArray1.low(); i <= intArray1.high(); i++)
      intArray1[i] = 5*i;
   for(i = intArray1.low(); i <= intArray1.high(); i++)
      ASSERT(intArray1[i] == 5*i );
   for(i = 0; i < intArray1.size(); i++)
      ASSERT(intArray1[i] == intArray1.index(i) );

   //illegal use of Ctors
   TEST_ERROR("Illegal bounds requested", Array<int> t1Array(0, -1));
   TEST_ERROR("Illegal bounds requested", Array<int> t2Array(-5));
   TEST_ERROR("Illegal bounds requested", Array<int> t3Array(0,-1,0));
   //Note that the copy constructor cannot be tested without a cast

   //Copy Ctor; Initializer Ctor
   Array<int> intArray2(intArray1,ctorDummy);
   Array<int> intArray3(20,20,0);

   // This is to test the const [] operator
   const Array<int>& intArray5(intArray3);
   
   int w,x;
   for(int k=0; k < intArray2.size(); k++) {
      w = 1423 * intArray2[k];
      x = 1423 * intArray2.index(k);
      ASSERT( w == x);
   }

   for( k = intArray3.low(); k < intArray3.high(); k++) {
      intArray3[k]*= intArray3[k] + intArray3[25] + 1;
      ASSERT(intArray5[k] == intArray3[k]);
   }

   TEST_ERROR("reference",x = intArray5[100000]);
   TEST_ERROR("reference",x = intArray5.index(100000));
   
   intArray3.sort();
   
   for(k = intArray1.low(); k <= intArray1.high(); k++) {
      intArray1[k] = intArray2[k];
      ASSERT( intArray1[k] == intArray2.index(k));
      intArray1.index(k) = intArray2.index(k);
      ASSERT( intArray2.index(k) == intArray1[k]);
   }

   intArray1 = intArray2;
   ASSERT( intArray1 == intArray2);
   intArray2 = intArray1;
   ASSERT( intArray2 == intArray1);

   //operator =
   TEST_ERROR("Cannot assign array sized", intArray1 = intArray3);
   intArray1 = intArray2; 

   //operator [], const and non-const
   TEST_ERROR("reference", w = 12 * intArray1[100000]);
   TEST_ERROR("reference", intArray1[100000] = 2);
   //index operator, const and non-const
   TEST_ERROR("reference", w = 12 *intArray1.index(100000));
   TEST_ERROR("reference", intArray1.index(100000) = 2);

   //max and min 
   int dummy = 3;
   Array<int> intArray4(0); //haha
   TEST_ERROR("empty array",intArray4.max());
   TEST_ERROR("empty array",intArray4.min());
   Mcout << "Max of intArray1 " << intArray1.max() << endl;
   Mcout << "Min of intArray1 " << intArray1.min() << endl;
   Mcout << "intArray1.min(3) " << intArray1.min(dummy) << endl;
   Mcout << "intArray1.max(3) " << intArray1.max(dummy) << endl;

}

void misc_tests()
{
   // The following tests will call the Array methods which stores an
   // Array in a binary format.
   Mcout << "Testing Array::write_bin, Array::read_bin ..." << endl;

   MLCOStream binfile("t_Array.out1");
   Array<int> outArray(10);
      for(int w = 0; w < outArray.size(); w++)
      outArray.index(w) = 2 * w;

   Mcout << "Writing the following using Array::write_bin ..." << endl;
   Mcout << outArray << endl;
   outArray.write_bin(binfile);
   binfile << flush; 
 
   MLCIStream infile("t_Array.out1");
   Array<int> inArray(10);
   Mcout << "Now rereading using Array:read_bin..." << endl;
   inArray.read_bin(infile);
   Mcout << inArray << endl;
   Mcout << "Binary array IO successful." << endl;      
}


/***************************************************************************
  Description : Tests operators +=/-= (element wise addition and subtraction)
  Comments    :
***************************************************************************/
void test_element_wise_ops()
{
   Mcout << "Testing element-wise operators ..." << endl;
   //pass one -ints
   {
      Array<int> a(10);
      Array<int> b(10);
      for(int i = 0; i < a.size(); i++){
	 a[i] =  1;
	 b[i] = -1;
      }

      a += b; //a becomes the 0-vector
      for(int j = 0; j < a.size(); j++)
	 ASSERT(0 == a[j] ); 

      a -= b; //a becomes the 1-vector
      for(int k = 0; k < a.size(); k++)
	 ASSERT(1 ==  a[k]);
      
   }
   //pass two -Reals
   {
      Array<Real> a(10);
      Array<Real> b(10);
      for(int i = 0; i < a.size(); i++){
	 a[i] =  1.120000;
	 b[i] = -1.120000;
      }

      a += b; //a becomes the 0-vector
      for(int j = 0; j < a.size(); j++)
	 ASSERT(0 == a[j]);

      a -= b; //a becomes the 1.12-vector :-)
      for(int k = 0; k < a.size(); k++) 
	 ASSERT(fabs(1.12 - a[k]) < REAL_EPSILON);
   }
   Mcout << "element-wise ops check out fine ..." << endl;
}

main()
{
   test_basic_array();
   test_sort();
   test_int_array();
   misc_tests();
   test_element_wise_ops();
   return 0; // return success to shell
}
