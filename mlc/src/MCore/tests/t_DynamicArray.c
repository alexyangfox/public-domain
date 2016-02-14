// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests extendibility and initialization of exteneded area.
  Doesn't test : Normal array operations which were tested already.
  Enhancements : 
  History      : YeoGirl Yun                                   4/28/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <DynamicArray.h>
#include <MLCStream.h>
#include <MRandom.h>

#ifdef __OBJECTCENTER__
const int SMALL_ITER = 12;
const int LARGE_ITER = 50;
#else
const int SMALL_ITER = 100;
const int LARGE_ITER = 1000;
#endif


void test_constructors()
{
   DynamicArray<int> d1(-23,30,-1); // base, size, initial value
   DynamicArray<int> d2(100,-6666); // size, initial value
   DynamicArray<int> d3(d2,ctorDummy); // copy constructor


   ASSERT( d1[-23] == -1 );
   ASSERT( d1.size() == 30 );
   d1[30] = 23;
   ASSERT( d1.size() == 30 - (-23 )+ 1);
   ASSERT( d2[99] == -6666 );
   ASSERT( d2.size() == 100 );
   d2[120] = 23;
   ASSERT( d2.size() == 120 + 1 );
   for(int i = 0; i < 100; i++ )
      ASSERT(d3.index(i) == d2.index(i) );

   Mcout << "OK. - test constructors"<<endl;
}


void test_extendibility()
{
   DynamicArray<int> intDA1(10); 
   DynamicArray<int> intDA2(10); 

   intDA1.set_increment_size(-1);
   intDA2.set_increment_size(1);

   // test size of 10 array with a LARGE_ITER number of data items.
   // must be extened to be enough to store the data items.
   ASSERT( intDA1.size() == 10 );
   for(int i = 0 ; i < LARGE_ITER; i++)
      intDA1[i] = i;
   ASSERT( intDA1.size() == LARGE_ITER );
 

   // test size of 10 array with a SMALL_ITER number of data items,
   // but this time increment by one in every extension.
   ASSERT( intDA2.size() == 10 );
   for( i = 0; i < SMALL_ITER; i++ )
      intDA2[i] = i;
   ASSERT( intDA2.size() == SMALL_ITER );


   intDA1 = intDA2; // try to assign small array into large array
                    // intDA1 should shrink down to the size of intDA2.
   ASSERT(intDA1.size() == intDA2.size() &&
	  intDA1.size() == SMALL_ITER );
   
   // A big happened when we copied an array that was allocated
   //   to a bigger size than the real size.  Here we test it before
   //   fixing the bug.

   DynamicArray<int> intDA3(2,3);
   intDA3.set_increment_size(100);
   intDA3[0] = 3;
   intDA3[1] = 3;
   intDA3[2] = 3;
   intDA3[3] = 3; // now it's of size 100, but only 4 in Array;
   DynamicArray<int> intDA4(intDA3, ctorDummy); // this truncates to 4
   ASSERT(intDA4.size() == 4);
   intDA4[50] = 4; // this must extend back to 50.

   Mcout << "OK. - test extendibility" << endl;
   

}


const int initValue = -1211; // arbitrary integer.

void test_initialization()
{
   DynamicArray<int> intDA1(10,initValue);

   intDA1[300] = 89; // assignment causing extension.
   // must be initialized with -1211   
   for(int i = 0; i < 300; i++ )
      ASSERT( intDA1.index(i) == -1211 &&
	      intDA1[i] == -1211 );
   Mcout << "OK. - test initialization" << endl;
}


const int INITIAL_VALUE = 78;

void test_const_operations()
{
   const DynamicArray<int> constArray(100,INITIAL_VALUE); // initialize with 78
   for( int i = 0; i < 100; i++ ) 
      ASSERT(constArray[i] == INITIAL_VALUE &&
	     constArray.index(i) == INITIAL_VALUE);
   Mcout << "OK. - test const operations" << endl;
}


void test_truncate()
{
   Mcout << "testing truncate method.. " << endl;
   const int k = 1000;
   const int base = 50;

   DynamicArray<int> dArray(0,0,-1);
   for (int i = 0; i < k; i++)
      dArray[i] = i + base;


   MRandom rand(7258789);
   Array<int> regArray(0,k,-1);
   
   while (dArray.size() > 0) {
      int size = dArray.size();
      int index = rand.integer(size);
      regArray[size -1] = dArray[index];
      dArray[index] = dArray[size - 1];
      dArray.truncate(size - 1);
      dArray[size + 3] = 3333;
      ASSERT(dArray[size] == -1);
      ASSERT(dArray[size + 1] == -1);
      ASSERT(dArray[size + 2] == -1);
      ASSERT(dArray[size + 3] == 3333);
      dArray.truncate(size - 1);
   }
   // see if it has random numbers from base to base + k-1
   Mcout << regArray << endl;   
}   



// It seems that we cannot delete an array which was allocated with size 0,
// if it has a virtual destructor.

class ClassWithVirtDest {
public:
   int i;
   ClassWithVirtDest() {i = 3;}
   virtual ~ClassWithVirtDest() {}
};

void show_bug_with_virt_dest()
{
   ClassWithVirtDest* foo = new ClassWithVirtDest[0];
   ASSERT(foo != NULL);
   delete[] foo;
}

// This test makes sure we don't have the above problem in DynamicArray.
void test_zero_size_array()
{
   DynamicArray<ClassWithVirtDest> foo(0);
   int j = foo[0].i;
}


// test append function
void test_append()
{
   DynamicArray<int> a1(5);
   Array<int> a2(4);

   a1[0] = 0; a1[1] = 1; a1[2] = 2; a1[3] = 3; a1[4] = 4;
   a2[0] = 5; a2[1] = 6; a2[2] = 7; a2[3] = 8;
   a1.append(a2);

   for(int i=a1.low(); i<=a1.high(); i++) {
      Mcout << i << ": " << a1[i] << " ";
      ASSERT(i == a1[i]);
   }

   Mcout << endl;
}


main() {

   Mcout << "Testing DynamicArray.." << endl;

   // show_bug_with_virt_dest();

   test_truncate();
   test_constructors();
   test_extendibility();
   test_initialization();
   test_const_operations();
   test_zero_size_array();
   test_append();
   return 0;
}





