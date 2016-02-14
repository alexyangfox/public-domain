// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : To test that the numbers average out correctly, we
                    check that the mean falls in a 99% confidence interval.
                    We get N numbers (N = 10,000) in the range 0,T (T=50)
                    According to Mathematical Statistics and Data Analysis /
                    Rice, page 266/13,  Mean[X] = T/2 (method of moments),
                    V = Var(Mean[X]) = T^2/3N.
                    Hence the 99 percent confidence interval is
                               Mean[X] +- z(.005)sqrt(V)
                    where z is the point of a normal distribution where the
                    tail is 0.005. 
                    This z point (Mathematica) is:
                       Quantile[NormalDistribution[0,1],.995]= 2.576
                    Thus the mean should be 25+-2.576*Sqrt(50^2/30,000) =
                       25+-0.75  99% of the time.
                    The variance of a uniform distribution is (b-a)^2/12,
                    which is 208.3 in our case.  Without deriving a confidence
                    interval, we'll simply test that it falls within 4 times
                    the mean confidence, i.e. 208.3 +- 4*.75 = 208.3 +- 3.
                    (this is a silly interval, but probably big enough for
                     us.)                                           Ronnyk
  Doesn't test :
  Enhancements :
  History      : Svetlozar Nestorov                            12/19/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <math.h>
#include <sys/types.h>     
#include <sys/time.h>
#include <random.h>  // not declared in Solaris
#include <Array.h>
#include <Array2.h>
#include <MRandom.h>

RCSID("MLC++, $RCSfile: t_MRandom.c,v $ $Revision: 1.7 $")

#define SEED1 5
#define SEED2 4325242
#define INT_RANGE 7,23
#define SEED3 13
#define INT_RANGE2 11,31
#define SEED11 5643664
#define SEED12 289213

// the probability that there is a repeating sequence of length PARAM_LEN
// among the first PARAM_NUM is between PARAM_NUM/PARAM_LEN * coeff
// and PARAM_NUM * coeff where coeff=(1-2^(-PARAM_LEN))^(PARAM_NUM-PARAM_LEN)
#define PARAM_LEN 40   
#ifdef __OBJECTCENTER__
  #define PARAM_NUM 400
#else
  #define PARAM_NUM 400
#endif

#define PARAM_N 10000
#define PARAM_T 50

static double square(double num) {return num*num;}

void independence_test()
{
   Array<MRandom> ranGens(10);
   Array2<int> ranNums(10, 101);  // ranNums[i][j] stores the j-th random
                                  // number produced by the i-th generator.
                                  // ranNums[i][0] stores how many random
                                  // numbers have been generated so far. 
   for (int i=0; i<10; i++) {
      ranGens[i].init(SEED3);
      ranNums(i, 0) = 0;
   }

   // every generator produces several random numbers in INT_RANGE2.
   // The exact number of random numbers produced by each generator is
   //   in the file "t_MRandom.cin". 
   for (i=0; i<10; i++) {
      int num = 0;
      cin >> num; // generate num random numbers using the i-th generator
      for (int j=1; j<=num; j++)
	 ranNums(i,j) = ranGens[i].integer(INT_RANGE2);
      ranNums(i,0) = num;
   }
   // Again every generator produces several random numbers.
   for (i=0; i<10; i++) {
      int num;
      cin >> num;
      for (int j=1; j<=num; j++)
	    ranNums(i,j+ranNums(i,0)) = ranGens[i].integer(INT_RANGE2);
      ranNums(i,0) += num;
   }
   // complete to 100 numbers for each generator.
   for (i=0; i<10; i++)  
      for (int j=ranNums(i,0)+1; j<=100; j++)
	 ranNums(i,j) = ranGens[i].integer(INT_RANGE2);
   for (int j=1; j<101; j++)   
      for (i=1; i<10; i++)
	 ASSERT(ranNums(i,j) == ranNums(0,j));
}

// test the mean and the variance
void variance_mean_test()
{
 
   MRandom uniRand(SEED11);
   MRandom duniRand(SEED12);
   Real sum, var, dsum, dvar;
   sum = var = dsum = dvar = 0;
   for (int i=0; i<PARAM_N; i++) {
      Real result = uniRand.real(0, PARAM_T);
      sum += result;
      var += square(result-PARAM_T/2);
      int dresult = uniRand.integer(PARAM_T);
      dsum += dresult;
      dvar += square(dresult-Real(PARAM_T-1)/2);
   }

   ASSERT(fabs(sum/PARAM_N - PARAM_T/2)<0.75);
   ASSERT(fabs(var/PARAM_N - square(PARAM_T)/12)<3);
   ASSERT(fabs(dsum/PARAM_N - Real(PARAM_T-1)/2)<0.75);
   ASSERT(fabs(dvar/PARAM_N - square(PARAM_T)/12)<3);
}

// test that there is not a repeating sequence of length PARAM_LEN among the
// first PARAM_NUM least significant bits of the value returned by random()
void randomness_test()
{
   cout << "Testing for randomness, be patient..." << endl;
   
   MRandom randGen(1);
   // we do not use the safe MLC arrays because we need bb to be char*
   //   for the initialization of myString
   char bb[PARAM_NUM+1];
      
   // make a string of all bits returned, A for 0 and B for 1
   for (int i = 0; i<PARAM_NUM; i++)
      bb[i] = 'A' + randGen.boolean(); 
   bb[PARAM_NUM] = 0;
   MString myString(bb);   

   
   for (i=0; i < PARAM_NUM-PARAM_LEN-1; i++)
   {
      // take a substring of length PARAM_LEN and check that it is not
      // contained in the string starting from position i+1.
      MString checkString(bb+i, PARAM_LEN);
      if (myString.index(checkString, i+1)!=-1)
	 err << "t_MRandom.c: Test for randomness failed. Variable i"
	        " was " << i << fatal_error;
   }
}

void sequence_test()
{
   MLCOStream file("t_MRandom.out1");
   MRandom r(9333126);
   
   for (int i = 0; i < 100; i++)
      file << r.integer(1000) << endl;

   for (i = 0; i < 100; i++)
      file << r.real(0,1) << endl;

}

main()
{  
   cout << "t_MRandom executing" << endl;

   sequence_test();

   // test constructors
   MRandom rGen1(SEED1);
   MRandom rGen3(SEED2);

   // test init
   int val = rGen1.integer(INT_RANGE);
   for (int i=0; i<23; i++)
      rGen1.integer(i+8);
   // just tests integer(count) and long_int(count)
   rGen1.long_int(4565);
   rGen1.integer(34);
   
   rGen1.init(SEED1);
   ASSERT(val == rGen1.integer(INT_RANGE));
   // test for high == low
   ASSERT(rGen1.long_int(9, 9) == 9);
   ASSERT(fabs(rGen1.real(9.87, 9.87) - 9.87) < REAL_EPSILON);

   // test error messages in integer and real
   TEST_ERROR("long_int: The lower", rGen1.integer(9, 2));
   TEST_ERROR("real: The lower", rGen1.real(-4.86, -4.87));
   TEST_ERROR("real: The lower", rGen1.real(7.56, 6.29));

   // test error messages about using setstate with the current state
   TEST_ERROR("MRandom.c::MLC_random: The current state is passed as a new "
	      "state", Berkeley_setstate((char *)&rGen1); rGen1.integer(9));
   TEST_ERROR("MRandom::init: The current state is initialized as a new state",
	      Berkeley_setstate((char *)&rGen1); rGen1.init(1));
   
   independence_test();
   variance_mean_test();
   randomness_test();


   cout << "Success!!!" << endl;
   return 0; // return success to shell
}






