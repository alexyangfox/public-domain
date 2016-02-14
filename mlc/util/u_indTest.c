// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : General inducer tester.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                   June 13, 95
                   Initial revision
***************************************************************************/


#include <basics.h>
#include <MLCStream.h>
#include <env_inducer.h>
#include <BagSet.h>
#include <InstList.h>
#include <CtrBag.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <LogOptions.h>
#include <GetOption.h>
#include <MEnum.h>
#include <ConstInducer.h>
#include <math.h>
#include <mlcIO.h>

const MString DATAFILE1 = "monk1-full";
const MString DATAFILE2 = "crx";
const int defaultExpectedWarnings = 0;
const MString EXPECTED_WARNINGS_HELP = 
  "Some inducers cannot pass all the tests for some valid reasons. "
  "Such inducers should specify expected number of failures at the "
  "tests. If the number of test failures is equal to the expected number, "
  "no fatal error will be generated.";

int warnings = 0;  // see if there is any warnings.
int expectedWarnings = defaultExpectedWarnings;

/*****************************************************************************
  Description : Checks whether accuracy from train_and_test and CatTestResult
                   using train is the same.
  Comments    :
*****************************************************************************/
void simple_accuracy_check(BaseInducer* inducer,
			   BaseInducer* inducer1,
                           CtrInstanceList& trainList,
			   CtrInstanceList& testList)
{
   Mcout << "doing simple accuracy check... " << endl;
   if (!inducer->can_cast_to_inducer())
      return; // do not do this test.

   if (!inducer1->can_cast_to_inducer())
      return; // do not do this test.

   // the order was changed on purpose.
   Inducer& ind = inducer1->cast_to_inducer();
   Inducer& ind1 = inducer->cast_to_inducer();   
   Real acc1 = ind.train_and_test(&trainList, testList);

   CtrInstanceList *tempPtr = &trainList; // to avoid temp warning.
   delete ind1.assign_data(tempPtr);
   ind1.train();
   ind1.release_data();
   const Categorizer& cat = ind1.get_categorizer();
   CatTestResult result(cat, trainList, testList);
   if (fabs(result.accuracy() - acc1) > REAL_EPSILON) {
      Mcerr << "WARNING u_indTest.c:The accuracies from train_and_test "
	 "and CatTestResult are different (" << acc1 << " <-> " <<
	 result.accuracy() << ")." << endl;
      warnings++;
   }
}



/*****************************************************************************
  Description : Checks wether logging output from two copies of categorizers
                  are the same.
  Comments    :
*****************************************************************************/
void logging_output_check(BaseInducer* inducer,
			  CtrInstanceList& trainList,
			  CtrInstanceList& testList)
{
   Mcout << "doing logging output check... " << endl;
   if (!inducer->can_cast_to_inducer())
      return;  // because of get_categorizer() method.

   Inducer& ind = inducer->cast_to_inducer();
   MString outfile1 = "u_indTest." + ind.description() + ".out1";
   MLCOStream outStream1(outfile1);
   MString outfile11 = outfile1 + ".display";
   MLCOStream outStream11(outfile11);
   const Categorizer& cat = ind.get_categorizer();
   cat.set_log_stream(outStream1);
   cat.set_log_level(2);
   CatTestResult result1(cat, trainList, testList);
   cat.display_struct(outStream11, DisplayPref::ASCIIDisplay);
   outStream11 << endl;
      
   // Open a stream, wet cat1 to it, and copy so that if the copy is working,
   // the output should be to outfile2
   MString outfile2 = "u_indTest." + ind.description() + ".out2";
   MLCOStream outStream2(outfile2);
   MString outfile22 = outfile2 + ".display";
   MLCOStream outStream22(outfile22);

   const Categorizer& cat2 = *cat.copy();
   cat2.set_log_stream(outStream2);
   cat2.display_struct(outStream22, DisplayPref::ASCIIDisplay);
   outStream22 << endl;
   
   if (cat2.get_log_level() != cat.get_log_level()) {
      Mcerr << "WARNING u_indTest.c:logging_output_check: log level is wrong. "
	 "Probable cause is copy ctor not using Categorizer copy ctor"
	    << endl;
      warnings++;
   }
   CatTestResult result2(cat2, trainList, testList);

   outStream1.close();
   outStream2.close();
   outStream11.close();
   outStream22.close();   

   MString diffout = "u_indTest." + ind.description() + ".diff";
   MString command = "diff " + outfile1 + " " + outfile2 + " > " + diffout ;
   if (system(command) != 0) {
      Mcerr << "WARNING u_indTest.c: the outputs of CatTestResult between "
	 "copies of categorizers are different." << endl;
      warnings++;
   }

   command = "diff " + outfile11 + " " + outfile22 + " > " + diffout;
   if (system(command) != 0) {
      Mcerr << "WARNING u_indTest.c: the outputs of display_struct between "
	 "copies of categorizers are different.  See " << outfile11 << " and "
	 << outfile22 << endl;
      warnings++;
   }

   /*
   remove_file(outfile1);
   remove_file(outfile2);
   remove_file(outfile11);
   remove_file(outfile22);
   remove_file(diffout);
   */
}




/*****************************************************************************
  Description : Checks if inducer works with 0 attributes.
  Comments    :
*****************************************************************************/
void zero_attribute_check(BaseInducer* inducer,
			  CtrInstanceList& trainList,
			  CtrInstanceList& testList)
{
   Mcout << "doing zero attribute check..." << endl;
   ConstInducer constInd("const inducer");
   Real accBase = constInd.train_and_test(&trainList, testList);
   BoolArray mask(0, trainList.get_schema().num_attr(), FALSE);
   CtrInstanceList* projectedTrainList =
      &trainList.project(mask)->cast_to_ctr_instance_list();

   projectedTrainList->normalize_bag(InstanceBag::none);
   CtrInstanceList* projectedTestList  =
      &testList.project(mask)->cast_to_ctr_instance_list();

   if (accBase != inducer->train_and_test(projectedTrainList,
					  *projectedTestList)) {
      Mcerr << "WARNING u_indTest.c: the accuracy on 0 attribute training "
	 "set is different from the one from const inducer." << endl;
      warnings++;
   }
   delete projectedTrainList;
   delete projectedTestList;
}


/*****************************************************************************
  Description : This test verifies that there is no problem with training
                  and testing twice on different-sized datasets.
  Comments    :
*****************************************************************************/
void small_large_dataset_test(BaseInducer *inducer)
{
   Mcout << "doing small large dataset test..." << endl;
   CtrInstanceList smallData("monk1-full");
   CtrInstanceList smallTest("monk1-full", ".names", ".test");
   CtrInstanceList largeData("breast");
   CtrInstanceList largeTest("breast", ".names", ".test");
   
   (void)inducer->train_and_test(&smallData, smallTest);
   (void)inducer->train_and_test(&largeData, largeTest);   
}
   


/*****************************************************************************
  Description : Perform a set of tests common to all inducers.
  Comments    :
*****************************************************************************/
void inducer_tester(const MString& datafile)
{
   CtrInstanceList trainList(datafile);
   trainList.normalize_bag(InstanceBag::none);
   CtrInstanceList testList(datafile, ".names", ".test");

   BaseInducer *inducer = env_inducer("");  // assumed to be same inducer
   BaseInducer *inducer1 = env_inducer(""); // because we want the same rand
					    // num
   small_large_dataset_test(inducer);
   simple_accuracy_check(inducer, inducer1, trainList, testList);
   logging_output_check(inducer, trainList, testList);
   zero_attribute_check(inducer, trainList, testList);

   delete inducer;
   delete inducer1;
}




int main()
{
   // Get the number of expected warnings.
   expectedWarnings = get_option_int("EXPECTED_WARNINGS", 
                                      defaultExpectedWarnings,
                                      EXPECTED_WARNINGS_HELP,
                                      TRUE);
                  
   Mcout << "Testing " << DATAFILE1 << endl;
   inducer_tester(DATAFILE1);
   Mcout << "Testing " << DATAFILE2 << endl;
   inducer_tester(DATAFILE2);
   if (warnings != expectedWarnings)
      err << "u_indTest.c: There have been " << warnings << " warning(s) "
	  << " and " << expectedWarnings << " were expected" << fatal_error;

   return 0;
}


