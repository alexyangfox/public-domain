// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : 
  Doesn't test :
  Enhancements :
  History      : YeoGirl Yun                                       12/27/94
                   Reorganized testers and added newly added member
		   function testers 
                 Richard Long                                      10/01/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <CatTestResult.h>
#include <ConstInducer.h>
#include <CtrInstList.h>

RCSID("MLC++, $RCSfile: t_CatTestResult.c,v $ $Revision: 1.16 $")

void test(const MString& fileName)
{

   InstanceList* constTrainList;
   constTrainList = new InstanceList(fileName);
   InstanceList constTestList(fileName, ".names", ".test");

   ConstInducer constInducer(fileName + " const inducer");
   constInducer.assign_data(constTrainList);
   ASSERT(constTrainList == NULL); // assign data gets ownership
   constInducer.train();

   CatTestResult constResult(constInducer.get_categorizer(),
			     constInducer.instance_bag(),
			     constTestList);
   Mcout << "Const Inducer Results" << endl << constResult;

   // test memorized vs general accuracy
   Mcout << "Number Test Instances Off Train: "
	 << constResult.num_off_train() 
         << "  Number Test Instances On Train: "
	 << constResult.num_on_train() << endl;
   Mcout << "Const overall accuracy: "
	 << constResult.accuracy(CatTestResult::Normal) << endl
	 << "Const memorized accuracy: "
	 << constResult.accuracy(CatTestResult::Memorized) << endl
	 << "Const generalized accuracy: "
	 << constResult.accuracy(CatTestResult::Generalized) << endl;
   
   constResult.display_all();
}


int main()
{
   test("t_CatTestResult");

   return 0;
}
