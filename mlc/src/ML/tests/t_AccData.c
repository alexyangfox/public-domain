// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test AccData
  Doesn't test : We don't test functions like accuracy and std_dev
                   thoroughly because t_StatData does this.
  Enhancements : Cache estimated accuracy/std_dev
  History      : Dan Sommerfield                                   4/15/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <AccData.h>

RCSID("MLC++, $RCSfile: t_AccData.c,v $ $Revision: 1.5 $")

void test_estimated()
{
   AccData accData;

   ASSERT(!accData.has_estimated());
   accData.insert(0.3);
   accData.insert(0.4);
   accData.insert(0.5);

   Mcout << "est only: " << accData << endl;

   ASSERT(accData.accuracy() == accData.mean());
   ASSERT(accData.std_dev() == accData.std_dev_of_mean());
}

void test_real()
{
   AccData accData;

   ASSERT(!accData.has_real());
   accData.set_real(0.75, 20);

   Mcout << "real only: " << accData << endl;

   // make sure that the real values are equal to the estimated values.
   // this should be true because we have real accuracy set, but not
   // estimated.
   ASSERT(accData.accuracy() == accData.real_accuracy());
   ASSERT(accData.std_dev() == accData.theo_std_dev());

   Real eLow, eHigh, rLow, rHigh;
   accData.confidence(eLow, eHigh);
   accData.theo_confidence(rLow, rHigh);
   ASSERT(eLow == rLow);
   ASSERT(eHigh == rHigh);
}

void test_both()
{
   AccData accData;

   // create a full AccData and display it
   accData.insert(0.3);
   accData.insert(0.4);
   accData.insert(0.5);
   accData.set_real(0.5, 20);
   Mcout << "both: " << accData << endl;

   // clear the AccData
   accData.clear();
   Mcout << "clear: " << accData << endl;
   ASSERT(!accData.has_real());
   ASSERT(!accData.has_estimated());
}

void test_no_variance()
{
   AccData accData;

   // create an AccData with no variance for real or estimated
   accData.insert(0.5);
   accData.set_real(0.9, 0);
   Mcout << "no variance (0 real): " << accData << endl;
   ASSERT(!accData.has_std_dev());
   ASSERT(!accData.has_theo_std_dev());

   // repeat, but with 1 real trial
   accData.set_real(0.8, 1);
   Mcout << "no variance (1 real): " << accData << endl;
   ASSERT(!accData.has_theo_std_dev());
}

void test_append()
{
   AccData accData1;
   AccData accData2;

   accData1.insert(0.1);
   accData1.set_real(0.9, 0);
   accData1.insert_cost(3);
   Mcout << "append (1): " << accData1 << endl;
   
   accData2.insert(0.3);
   accData2.set_real(0.9, 0);
   accData2.insert_cost(4);
   Mcout << "append (2): " << accData2 << endl;
   
   accData2.append(accData1);
   Mcout << "append (2+1): " << accData2 << endl;
   ASSERT(accData2.get_cost() == 7);

   accData1 = accData2;
   Mcout << "assign -> 1: " << accData1 << endl;

   AccData emptyAccData;
   emptyAccData.append(accData1);
   Mcout << "append 1 -> empty: " << emptyAccData << endl;

   AccData empty2;
   accData2.append(empty2);
   Mcout << "append empty -> 2: " << accData2 << endl;
}


void test_errors()
{
   AccData accData;

   // out-of-range set_real
   TEST_ERROR("AccData::set_real: Real accuracy", accData.set_real(-1.0, 0));
   TEST_ERROR("AccData::set_real: Real accuracy", accData.set_real(2.0, 0));

   // bad test set size in set_real
   TEST_ERROR("AccData::set_real: test set", accData.set_real(0.5, -1));

   // test all check_* functions
   AccData badAccData;
   TEST_ERROR("AccData::check_real", badAccData.check_real());
   TEST_ERROR("AccData::check_estimated", badAccData.check_estimated());
   TEST_ERROR("AccData::check_std_dev", badAccData.check_std_dev());
   TEST_ERROR("AccData::check_theo_std_dev", badAccData.check_theo_std_dev());

   // append with incompatible reals
   AccData bad1;
   AccData bad2;
   bad1.set_real(0.1, 1);
   bad2.set_real(0.2, 1);
   TEST_ERROR("AccData::append", bad1.append(bad2));

   // append with incompatible n
   bad1.set_real(0.1, 1);
   bad2.set_real(0.1, 2);
   TEST_ERROR("AccData::append", bad1.append(bad2));
}


main()
{
   Mcout << "t_AccData executing" << endl;
   
   test_estimated();
   test_real();
   test_both();
   test_no_variance();
   test_append();
   test_errors();
   return 0;
}

