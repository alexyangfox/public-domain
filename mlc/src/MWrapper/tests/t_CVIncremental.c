// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test CVIncremental by making sure it behaves as
                     cross_validator. 
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi and YeoGirl Yun                      10/15/94
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <math.h>
#include <CValidator.h>
#include <CtrInstList.h>
#include <TableInducer.h>
#include <CVIncremental.h>

RCSID("MLC++, $RCSfile: t_CVIncremental.c,v $ $Revision: 1.6 $")

const MString fileStem  = "/tmp/t_lensesCV";
const MString namesFile = "t_CValidator.names"; // yes, use the same file as 
const MString dataFile  = "t_CValidator.data";  // t_CValidator.


#ifdef INTERACTIVE
  const int NUM_TIMES = 1;
  const int MAX_NUM_INST = 1;
#else
  const int NUM_TIMES = 2;
  const int MAX_NUM_INST = 5;
#endif
const int RAND_SEED = 7258789;


// numInst is the number of instances in a fold.
void compare_validators(InstanceList& data, int times, int numInst)
{
   Mcout << "Comparing " << times << 'x' << -numInst << " cross validation."
	 << endl;

   CrossValidator crossValidator(-numInst, times);
   crossValidator.init_rand_num_gen(RAND_SEED);
   TableInducer tabInducer("Table Inducer", TRUE);
   Real acc1 = crossValidator.estimate_accuracy(tabInducer, data);
   Real stdDev1 = crossValidator.accuracy_std_dev();
   Mcout << "Accuracy for CValidator: " << acc1 << " +- " << stdDev1 << endl;

   CVIncremental incrValidator(-numInst, times);
   incrValidator.init_rand_num_gen(RAND_SEED);
   Real acc2 = incrValidator.estimate_accuracy(tabInducer, data);
   Real stdDev2 = incrValidator.accuracy_std_dev();
   Mcout << "Accuracy for CVIncremental : " << acc2 << " +- " << stdDev2
	 << "\n" << endl;
   ASSERT(fabs(acc1  - acc2) < REAL_EPSILON);
   ASSERT(fabs(stdDev1 - stdDev2) < REAL_EPSILON);
}


main()
{
   Mcout << "t_CVIncremental executing" << endl;
   CtrInstanceList monk("monk1-full");
   CtrInstanceList orgBag("", namesFile, dataFile);
   
   // project, removing an attribute so we'll have some duplicates.
   BoolArray mask(4);
   mask[0] = 1; mask[1] = 0; mask[2] = 1; mask[3] = 1;
   CtrInstanceList& bag = orgBag.project(mask)->cast_to_ctr_instance_list();
   Mcout << bag << endl;


   MLCOStream out1("t_CVIncremental.out1");

   for (int times = 1; times <= NUM_TIMES; times++)
      for (int numInst = 1; numInst <= MAX_NUM_INST; numInst++) {
	 compare_validators(bag, times, numInst);
#ifndef INTERACTIVE
	 if (numInst == 3 && times == 1)
	    compare_validators(monk, times, numInst);
#endif
      }

   delete &bag;


   return 0;
}


