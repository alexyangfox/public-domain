// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tests TableCasInd. Run monk1-full with order of 4, 0, 1,
                   and 2.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                   7/9/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <TableCasInd.h>
#include <BagSet.h>
#include <CtrInstList.h>
#include <CatTestResult.h>


void show_result(const Categorizer& cat,
		 const CtrInstanceList& trainSet,
		 const CtrInstanceList& testSet,
		 const MString& datafile,
		 const Array<int>& attrNums)
{
   CatTestResult result(cat,
			trainSet,
			testSet);
   
   Mcout << "DATAFILE : " << datafile << endl;
   Mcout << "ATTR NUM ARRAY : " << attrNums << endl;
   Mcout << "ACCURACY : " << result.accuracy() << endl;
}


void run_inducer(const MString& datafile,
		 const Array<int>& attrNums, MLCOStream& out)
{
   TableCasInd tableCasInd("Table Cascade Inducer",
			   attrNums);

   CtrInstanceList trainSet(datafile, ".names", ".data");
   CtrInstanceList testSet(datafile, ".names", ".test");

   CtrInstanceList *tempBag = &trainSet;
   tableCasInd.assign_data(tempBag);
   tableCasInd.set_log_level(2);
   tableCasInd.set_log_stream(out);
   tableCasInd.train();
   tableCasInd.release_data();

   show_result(tableCasInd.get_categorizer(), trainSet, testSet,
	       datafile, attrNums);
}


void one_attribute_test()
{
   CtrInstanceList trainSet("monk1", ".names", ".data");
   CtrInstanceList testSet("monk1", ".names", ".test");

   Array<int> attrNums(0, 1);
   attrNums[0] = 4;

   TableCasInd tableCasInd("Table Cascade Inducer", attrNums);
   CtrInstanceList *tempBag = &trainSet;
   tableCasInd.assign_data(tempBag);
   tableCasInd.train();
   tableCasInd.release_data();

   show_result(tableCasInd.get_categorizer(), trainSet, testSet,
	       "monk1", attrNums);

   TableCasInd tableCasInd2("Table Cascade Inducer");
   tableCasInd2.set_order(attrNums);
   Mcout << "ACCURACY : " << tableCasInd2.train_and_test(&trainSet, testSet)
      << endl;
}


int main()
{
   Mcout << " testing t_TableCasInd ... " << endl;

   one_attribute_test();

   Array<int> attrNums(0, 4);
   attrNums[0] = 4;
   attrNums[1] = 0;
   attrNums[2] = 1;
   attrNums[3] = 2;
   
   MLCOStream out1("t_TableCasInd.out1");
   run_inducer("monk1-full", attrNums, out1);

#ifndef TESTCENTER
   MLCOStream out2("t_TableCasInd.out2");
   run_inducer("waveform-21", attrNums, out2);
#endif   

   return 0;
}
   
