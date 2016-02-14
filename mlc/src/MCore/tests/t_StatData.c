// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test StatData.
  Doesn't test : There was no obvious way to test the trimmed variance.
                 We are basing our result on one dubious example in Rice.
                   It is dubious because there are bugs in his table of
                   data which we fixed from another page.
  Enhancements :
  History      : James Dougherty and Ronny Kohavi                   5/23/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <math.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <StatData.h>

RCSID("MLC++, $RCSfile: t_StatData.c,v $ $Revision: 1.11 $")

void basic_test()
{

#ifdef mathematica_code_to_verify_results
   <<Statistics`DescriptiveStatistics`
   data={3,4,5,6,8}
   N[Mean[data]]
   N[Variance[data]]
   N[StandardDeviation[data]]
   N[TrimmedMean[data,0.2]]
   N[VarianceOfSampleMean[data]]
#endif
      
   StatData statData;
   for(int i = 3; i < 7; i++)
       statData.insert(i);
   statData.insert(8); // 3-7 is too simply and we can't see the effect of
                       // trimming.
   ASSERT(statData.size() == 5);
   for(i = 0; i < statData.size() - 1; i++)
       ASSERT(statData[i] == i+3);

   Mcout << statData << endl; // check display function.
   ASSERT(statData[statData.size() - 1] == 8);       
   ASSERT(fabs(Mround(statData.mean(),4) - 5.2) < REAL_EPSILON);
   ASSERT(fabs(Mround(statData.variance(), 4) - 3.7) < REAL_EPSILON);
   ASSERT(fabs(Mround(statData.std_dev(), 4) - 1.9235) < REAL_EPSILON);
   ASSERT(fabs(Mround(statData.mean(0.2),4) - 5) < REAL_EPSILON);
   ASSERT(fabs(Mround(statData.variance_of_mean(),4) - 0.74) < REAL_EPSILON);

   // The following were taken by calling the function.
   // They could not be verified by mathematica.
   ASSERT(fabs(Mround(statData.variance(0.2), 3) - 2.778) < REAL_EPSILON);
   ASSERT(fabs(Mround(statData.variance_of_mean(0.2), 3) - 0.556) <
	  REAL_EPSILON);
   ASSERT(fabs(Mround(statData.std_dev(0.2), 3) - 1.667) < REAL_EPSILON);
   ASSERT(fabs(Mround(statData.std_dev_of_mean(0.2), 3) - 0.745) < REAL_EPSILON);

   // percentile tests.
   StatData simple;
   for (i = 10; i >= 1; i--)
      simple.insert(i);

   Real low= 0, high = 0;
   for (i = 1; i <= 4; i++) {
      simple.percentile(1-(Real)i/5,low,high);
      Mcout << "low=" << low << ", high=" << high << endl;
      ASSERT( low == i+1 &&  high == 10 - i);
   }
}   

void rice_test()
{
   // This is the data file from Rice "Mathematical Statistics and
   // Data Analysis" [10.4.1] p. 328                             
   int setSize;
   MLCIStream infile("t_StatData.data");
   infile >> setSize;
   StatData platinumData;
   for(int j = 0; j < setSize; j++) {
       Real tempValue;
       infile >> tempValue; // op[] here would be nice.
       platinumData.insert(tempValue);
   }
   // All these asserts are from the mathematica code above, with the
   //   data from the file.
   ASSERT(fabs(Mround(platinumData.mean(),3) - 137.046) < REAL_EPSILON);
   ASSERT(fabs(Mround(platinumData.variance(),4) - 19.7906) < REAL_EPSILON);
   ASSERT(fabs(Mround(platinumData.variance(),4) - 19.7906) < REAL_EPSILON);
   ASSERT(fabs(Mround(platinumData.std_dev(),3) - Mround(4.4486, 3)
	       < REAL_EPSILON));
   ASSERT(fabs(Mround(platinumData.variance_of_mean(),3) - Mround(0.7611,3)
	       < REAL_EPSILON));
   // The trimmed mean and variance of mean are in Rice page 333.
   // The mean in the book is 135.29 which is our result rounded.
   ASSERT(fabs(Mround(platinumData.mean(0.2), 3) - 135.288) < REAL_EPSILON);
   ASSERT(fabs(Mround(platinumData.std_dev_of_mean(0.2), 2) - Mround(0.254,2)
	       < REAL_EPSILON));
}

void test_errors()
{
   StatData foo;

   TEST_ERROR("out of bound", (void)foo[2]);
   TEST_ERROR("no data", foo.mean());
   TEST_ERROR("no data", foo.mean());
   foo.insert(3);

   TEST_ERROR("not in the range [0,0.5", foo.mean(0.7));
   TEST_ERROR("not in the range [0,0.5", foo.mean(-3));
   ASSERT(foo.variance() == UNDEFINED_VARIANCE);
}

void test_histogram()
{
   StatData foo;

   foo.insert(-6); 
   foo.insert(-5.5); 
   foo.insert(-5); 
   foo.insert(-3); foo.insert(-3);
   foo.insert(1);  foo.insert(1);
   foo.insert(3);

   foo.display_math_histogram(2, 2);
   Mcout << "mean is " << foo.mean() << endl;
}


void test_append()
{
   StatData foo;
   StatData bar;

   foo.insert(1);
   foo.insert(2);
   foo.insert(3);
   Mcout << foo << endl;

   bar.insert(4);
   bar.insert(5);
   Mcout << bar << endl;

   foo.append(bar);
   Mcout << foo << endl;
   ASSERT(foo.mean() == 3);
}

void test_squared_error()
{
   StatData foo;

   foo.insert(2.2);
   foo.insert(3.3);
   foo.insert(4);
   
   Real er = squareReal(3-2.2)+squareReal(3-3.3)+squareReal(3-4);
   ASSERT(fabs(er - foo.squaredError(3)) < REAL_EPSILON);
}

main()
{
   Mcout << "t_StatData executing" << endl;

   basic_test();
   rice_test();
   test_errors();
   test_histogram();
   test_append();
   test_squared_error();
   
   return 0; // return success to shell

}   




