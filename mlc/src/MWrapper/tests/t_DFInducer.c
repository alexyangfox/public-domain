// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Tests the Discretizing filter inducer and its acccuracy.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : James Dougherty                                    12/28/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <Inducer.h>
#include <DFInducer.h>
#include <CatTestResult.h>
#include <CtrInstList.h>
#include <math.h>

RCSID("MLC++, $RCSfile: t_DFInducer.c,v $ $Revision: 1.5 $")

#define PUTENV(str) ASSERT(putenv((str)) == 0);


/***************************************************************************
  Description : Runs the Inducer on a couple of datasets
  Comments    :
***************************************************************************/
main()
{
   Mcout << "t_DFInducer executing ... " << endl;
   Mcout << "Loading BC .... " << endl;
   PUTENV("DISCF_INDUCER=naive-bayes");
   PUTENV("LOGLEVEL=2");

   DiscFilterInducer* myInducer = new DiscFilterInducer("breast-cancer");
   myInducer->set_user_options("");
   CtrInstanceList trainSet("", "breast-cancer.names","breast-cancer.data");
   CtrInstanceList testSet("","breast-cancer.names","breast-cancer.test");

   Real acc = myInducer->train_and_test(&trainSet, testSet);
   Real confLow, confHigh;
   CatTestResult::confidence(confLow, confHigh, acc,
			     testSet.num_instances());

   Mcout << "Accuracy: " << MString(acc*100,2) << "% +- "
	 << MString(CatTestResult::theoretical_std_dev(
	    acc, testSet.num_instances())*100, 2)
	 << "% [" << MString(confLow*100, 2) << "% - "
	 << MString(confHigh*100, 2) << "%]" << endl;

   delete myInducer;
   return 0; // return success to shell
}



