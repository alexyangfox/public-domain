// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Tests the OneRInducer and its acccuracy.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : James Dougherty                                    12/28/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <Inducer.h>
#include <OneRInducer.h>
#include <CatTestResult.h>
#include <CtrInstList.h>
#include <math.h>

RCSID("MLC++, $RCSfile: t_OneRInducer.c,v $ $Revision: 1.3 $")



/***************************************************************************
  Description : Runs the Inducer on a couple of datasets
  Comments    :
***************************************************************************/
main()
{
   Mcout << "t_OneRInducer executing ... " << endl;
   Mcout << "Loading BC .... " << endl;
   OneRInducer* myInducer = new OneRInducer("breast-cancer", 3);
   InstanceList trainSet("", "breast-cancer.names","breast-cancer.data");
   InstanceList testSet("","breast-cancer.names","breast-cancer.test");

   Real acc = myInducer->train_and_test(&trainSet, testSet);
   Real confLow, confHigh;
   CatTestResult::confidence(confLow, confHigh, acc,
			     testSet.num_instances());

   Mcout << "Accuracy: " << MString(acc*100,4) << "% +- "
	 << MString(CatTestResult::theoretical_std_dev(
	    acc, testSet.num_instances())*100, 4)
	 << "% [" << MString(confLow*100, 4) << "% - "
	 << MString(confHigh*100, 4) << "%]" << endl;

   delete myInducer;
   return 0; // return success to shell
}

