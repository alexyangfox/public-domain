// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tests BaggingInd. Run heart and glass with and without
                   BaggingInd using ID3 as a BAG_INDUCER.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                  May 28 95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <MLCStream.h>
#include <BaseInducer.h>
#include <Inducer.h>
#include <BaggingInd.h>
#include <ID3Inducer.h>
#include <BagSet.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <BaggingCat.h>

/*****************************************************************************
  Description : Show the result of bagging inducer.
  Comments    :
*****************************************************************************/
void show_result(const BaggingInd& bagInd, Real acc, const MString& dataFile)
{
   Mcout << " -------------------------------------" << endl;
   Mcout << " INDUCER : Bagging Inducer" << endl;
   Mcout << " DATAFILE : " << dataFile << endl;
   Mcout << " BAG_INDUCER : ID3 " << endl;
   if (bagInd.get_unif_weights() == TRUE)
      Mcout << " BAG_UNIF_WEIGHT : TRUE" << endl;
   else
      Mcout << " BAG_UNIF_WEIGHT : FALSE" << endl;
   if (bagInd.get_use_above_avg_weight() == TRUE)
      Mcout << " BAG_USE_ABOVE_AVG_WEIGHT : TRUE" << endl;
   else
      Mcout << " BAG_USE_ABOVE_AVG_WEIGHT : FALSE" << endl;
   Mcout << " BAG_REPLICATIONS : " << bagInd.get_num_replication() << endl;
   Mcout << " BAG_PROPORTION : " << bagInd.get_proportion() << endl;
   Mcout << " Accuracy : " << acc << endl;
   Mcout << " -------------------------------------" << endl << endl;   
}
   


/*****************************************************************************
  Description : Runs Bagging Inducer using ID3 on a given datafile.
  Comments    :
*****************************************************************************/
void small_test(const MString& dataFile,
		BaggingInd& bagInd,
		CtrInstanceList& trainSet,
		CtrInstanceList& testSet,
		Bool unifOption,
		Bool avgWeightOption)
{
   bagInd.set_unif_weights(unifOption);
   bagInd.set_use_above_avg_weight(avgWeightOption);
   Real acc = bagInd.train_and_test(&trainSet, testSet);
   show_result(bagInd, acc, dataFile);

   // this is safe cast, since we know it returns BaggingCat.
   BaggingCat bagCat((BaggingCat &)bagInd.get_categorizer(), ctorDummy);

   CatTestResult results(bagCat, trainSet, testSet);
   Mcout << " Using copied categorizer, accuracy : " << results.accuracy()
	 << endl;
   if (results.accuracy() != acc)
      err << "Accuracy dismatch ! " << fatal_error;
   

   Mcout << " Displaying BaggingCat : " << bagCat;
}



/*****************************************************************************
  Description : Runs Bagging Inducer using ID3 on a given datafile.
  Comments    :
*****************************************************************************/
void run_bagging(const MString& dataFile)
{
   BaggingInd bagInd("Bagging Inducer");
   ID3Inducer id3Ind("ID3 Inducer");

   // set the main inducer.
   Inducer *mainInducer = &id3Ind.cast_to_inducer();
   bagInd.set_main_inducer(mainInducer);

   CtrInstanceList trainSet(dataFile);
   CtrInstanceList testSet("", dataFile + ".names", dataFile + ".test");

#ifdef TESTCENTER
   bagInd.set_num_replication(2);
#endif   

   small_test(dataFile, bagInd, trainSet, testSet, TRUE, TRUE);
   small_test(dataFile, bagInd, trainSet, testSet, TRUE, FALSE);
   small_test(dataFile, bagInd, trainSet, testSet, FALSE, TRUE);
   small_test(dataFile, bagInd, trainSet, testSet, FALSE, FALSE);   
}



/*****************************************************************************
  Description : Run ID3 inducer on a given data file.
  Comments    :
*****************************************************************************/
void run_id3(const MString& dataFile)
{
   ID3Inducer id3Ind("ID3 Inducer");

   CtrInstanceList trainSet(dataFile);
   CtrInstanceList testSet("", dataFile + ".names", dataFile + ".test");
   Real acc = id3Ind.train_and_test(&trainSet, testSet);

   Mcout << " ID3 Inducer accuracy on " << dataFile << " : " << acc << endl;
}


int main()
{
   Mcout << " testing t_BaggingInd.c ... " << endl << endl;

   run_id3("heart");
   run_bagging("heart");

   run_id3("glass");
   run_bagging("glass");

   Mcout << " Success! " << endl;
   return 0;
}
