// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the property MinArray maintains kth smallest
                    data items.
  Doesn't test : 
  Enhancements : 
  History      : YeoGirl Yun                                 5/11/94
                   Initial revision(.c)
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <MinArray.h>
#include <InstanceAndDistance.h>
#include <MRandom.h>
#include <Array.h>
#include <MLCStream.h>

const int LARGE_SIZE = 250;
const int SEED = 12432;

Bool is_in(MinArray<InstanceAndDistance>& buf, Real val)
{
   for(int i = 0; i < buf.num_elem(); i++)
      if( val == buf[i].distance ) return TRUE;
   return FALSE;
}


void test_min_maintainability(void)
{
   Array<InstanceAndDistance> buffer(LARGE_SIZE);
   MRandom randomVal(SEED);

   int i;
   for( int k = 1; k < LARGE_SIZE; k++) {
      MinArray<InstanceAndDistance> sample(k);
      for( i = 0; i < LARGE_SIZE; i++) {
	 buffer[i].distance = randomVal.real(0.0,LARGE_SIZE*2.0);	 
	 sample.insert(buffer[i]);
      }
      buffer.sort(); 
      for( i = 0; i < sample.num_elem(); i++ )
	 ASSERT(is_in(sample,buffer.index(i).distance));
   }
}


main()
{
   Mcout << "testing t_MinArray.c" << endl;
   test_min_maintainability();
   Mcout << "Success!" << endl;

   return 0;
}
