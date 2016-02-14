// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution

// This is a file to test some part of MLC++. It is NOT part of the
//    MLC++ library, but is designed to test portions of it.

/*****************************************************************************
  Description  : This file tests UnivHashTable class in three ways.
                   1. random insert() and del() tests on small hash table.
		   2. sequential insert() and del() tests on large hash table.
		   3. test for duplicate data item.
		   4. test for copy constructor and display routine.
  Doesn't test :
  Enhancements :
  History      : YeoGirl Yun
                   Added copy constructor and display test.         9/05/94
                 YeoGirl Yun                                        4/17/94
                   Initial revision
*****************************************************************************/

#include <basics.h>
#include <MRandom.h>
#include <errorUnless.h>
#include <MLCStream.h>

#include <UnivHashTable.h>
   
#ifdef __OBJECTCENTER__
   const int ITERATION = 30;
   const int LARGE_SIZE = 10;
   const int SMALL_SIZE = 3;
#else
   const int ITERATION = 300;
   const int LARGE_SIZE = 100;
   const int SMALL_SIZE = 30;
#endif   


//---------------------------------------------------------------------------//
// simple data structure for tests

class IntKey {
public:   
   int i;
   int length() const { return sizeof(int); }
   int operator==(const IntKey& intKey) { return i == intKey.i; }
   void display(MLCOStream& stream) const { stream << i; }
};
DEF_DISPLAY(IntKey);

class StudentDataType {
public :
  IntKey studentId; 
  MString name; // this is key field
  
  StudentDataType() { studentId.i = 0; };
  Bool contains_key(const IntKey& keyValue) const {
     return studentId.i == keyValue.i;
  }

   IntKey get_key() const { return studentId; }   
   StudentDataType& operator=(const StudentDataType& s) {
    studentId = s.studentId;
    name = s.name;
    return *this;
  }
  Bool operator==(const StudentDataType& source) const {
     return ( name == source.name && studentId.i == source.studentId.i );
  }
   
  void display(MLCOStream& stream=Mcout) const {
     stream << name << " " << studentId.i << endl;
  }
};

DEF_DISPLAY(StudentDataType);


//---------------------------------------------------------------------------//

const Real HashTable<IntKey, StudentDataType>::LOAD_FACTOR = 0.5;

typedef UniversalHashTable<IntKey, StudentDataType> StudentHT;

SET_DLLPIX_CLEAR(StudentDataType,NULL);

/*****************************************************************************
  Description  : Tests random inserts and dels in a small hash table
  Comments     :
*****************************************************************************/
void small_random_test(StudentDataType* data)
{

   Array<Bool> boolArray(0,SMALL_SIZE,FALSE); // set initial value FALSE
   StudentHT smallHT(SMALL_SIZE, 2324); // with seed.
   MRandom ranVal(2341); // with seed.

   for(int i = 0; i < ITERATION; i++ ) {
      int index;
      if( boolArray[( index = ranVal.integer(SMALL_SIZE))]) { 
	 // already inserted, delete that data
	 smallHT.del(data[index].studentId);
	 boolArray[index] = FALSE;
      }
      else { // not yet inserted. insert that data
	 smallHT.insert(data[index]);
	 boolArray[index] = TRUE;
      }
      if( (i % (ITERATION/10)) == 0 )
         Mcout << "#"<< i << " random small test .. "<<endl;
   }
	    
   // now see it maches between boolArray and StudentHT
   Mcout << "find test ... "<<endl;
   for( i = 0; i < SMALL_SIZE; i++ ) {
      if( boolArray[i] )  // check
	 ASSERT( *smallHT.find(data[i].studentId) == data[i] );
      else
	 ASSERT( smallHT.find(data[i].studentId) == NULL );
   }
   Mcout << "find test ..OK. " << endl;

   // display test.
   smallHT.display();   

   // delete all the items in small hash table
   for( i = 0; i < SMALL_SIZE; i++ ) {
      if( smallHT.del(data[i].studentId) )
	 boolArray[i] = FALSE;
   }

   // assert all delete's has been done perfect
   ASSERT(smallHT.num_items() == 0);

   Mcout << "O.K. random tests in small hash table" << endl;
}


void large_sequential_test(StudentDataType *data)
{
   StudentHT largeHT(LARGE_SIZE);
   
   // now test insert method of HT by inserting all the data
   // into largeHT
   Mcout << "Checking insert() ...." << endl;
   for(int i = 0; i < LARGE_SIZE/2; i++) 
      largeHT.insert(data[i]);
   
   // test copy constructor.
   StudentHT largeHT2(largeHT, ctorDummy);
   
   // check that they have the same data elements.
   Mcout << "Checking find() ...." << endl;
   for( i = 0; i < LARGE_SIZE/2; i++) {
      ASSERT(largeHT.find(data[i].studentId));
      ASSERT( data[i] == *(largeHT.find(data[i].studentId)) );
      ASSERT(largeHT2.find(data[i].studentId));
      ASSERT( data[i] == *(largeHT2.find(data[i].studentId)) );
   }
   
   // now check del method
   Mcout << "Checking del() .... " << endl;
   for( i = 0; i < LARGE_SIZE; i++) {
      largeHT.del(data[i].studentId);
      largeHT2.del(data[i].studentId);
   }
   
   // finally assert that all the data were deleted
   ASSERT(largeHT.num_items() == 0 );
   ASSERT(largeHT2.num_items() == 0 );
   
   Mcout << "O.K. : large-scale sequential test " << endl;
}


void duplicate_data_test(StudentDataType *data)
{
      
   // initialize HashTable
   //  usual size is 2 times expected number of data
   StudentHT sample(SMALL_SIZE);

   // test for duplicate data
   sample.insert(data[0]);
   sample.insert(data[1]);
   sample.insert(data[2]);
   sample.del(data[0].studentId);
   sample.del(data[1].studentId);
   TEST_ERROR("insert duplicate data",sample.insert(data[2]));
   sample.del(data[2].studentId);

   Mcout << "O.K. : duplicate data test " << endl;
}


main()
{

   // generate data for tests
   StudentDataType data[LARGE_SIZE];
   MRandom ranVal(2342);  // with seed.


   char buffer[LARGE_SIZE + 1];
   
   // generate student type data for various tests.
   // all the keys are distict.
   for(int i = 0; i < LARGE_SIZE; i++) {
      data[i].studentId.i = INT_MAX - i;

      // the length of key is varying from 1 to LARGE_SIZE + 1
      for(int k = 0; k < i + 1; k++) 
	 buffer[k] = 'A' +  ranVal.integer('Z' - 'A');
      buffer[k] = NULL;
      data[i].name = buffer;
   }

   // now do the tests
   duplicate_data_test(data);
   small_random_test(data);
   large_sequential_test(data);

   return 0;
}
  
