// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description : Test ProjectCategorizer. Reads monk1-full, projects bag on
		   attr 0,1,4, builds ID3 decision tree using projected bag.
		   Next, builds ProjectCat that projects on 0,1,4 and uses the
		   decision tree categorizer just made. Runs CatTestResult on
		   the testset and the project_categorizer should get 97.2%
		   accuracy.
  Doesn't test : 
  Enhancements :
  History      : Robert Allen                                       1/27/95
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <ID3Inducer.h>
#include <ProjectCat.h>
#include <CatTestResult.h>
#include <CtrInstList.h>
#include <BoolArray.h>
#include <math.h>

RCSID("MLC++, $RCSfile: t_ProjectCat.c,v $ $Revision: 1.9 $")

void test_results(CtrInstanceList& trainSet)
{

   BoolArray attr(0, trainSet.get_schema().num_attr(), FALSE);
   attr[0] = TRUE;
   attr[1] = TRUE;
   attr[4] = TRUE;

   InstanceBag *projectedData = trainSet.project(attr);
   CtrInstanceList testSet("monk1-full", ".names", ".test");
   InstanceBag *projectedTest = testSet.project(attr);
   
   ID3Inducer inducer("ID3 Inducer");   
   inducer.assign_data(projectedData);
   ASSERT(projectedData == NULL);
   inducer.train();
   
   Categorizer* DTcat = inducer.get_categorizer().copy();

   SchemaRC sch = trainSet.get_schema();
   ProjectCat Pcat("test pc", attr, sch, DTcat);
   ASSERT(DTcat == NULL);

   projectedData = inducer.release_data();
   CatTestResult * fileResult =
      new CatTestResult(inducer.get_categorizer(),
			*projectedData, *projectedTest);

   CatTestResult catResult(Pcat, trainSet, testSet);

   ASSERT(fabs(catResult.accuracy() - 0.972222) < 0.00001);
   
   ASSERT(fileResult->accuracy() == catResult.accuracy());

   Mcout << "Projected File Results" << endl << *fileResult << endl;
   Mcout << "Projected Categorizer Results" << endl << catResult << endl;

   delete fileResult;
   delete projectedData;
   delete projectedTest;
}


main()
{
   Mcout << endl << "t_ProjectCat starting." << endl;

   CtrInstanceList trainSet("monk1-full");
   test_results(trainSet);
   
   Mcout << endl << "t_ProjectCat competed." << endl;
   return 0;
}
      

