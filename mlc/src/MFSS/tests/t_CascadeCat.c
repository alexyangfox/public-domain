// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test ProjectCategorizer.
  Doesn't test : 
  Enhancements :
  History      : Robert Allen                                       2/15/95
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <LogOptions.h>
#include <TableInducer.h>
#include <ConstInducer.h>
#include <ProjectInd.h>
#include <CatTestResult.h>
#include <CtrInstList.h>
#include <BoolArray.h>
#include <CascadeCat.h>

RCSID("MLC++, $RCSfile: t_CascadeCat.c,v $ $Revision: 1.4 $")

/*
  Tester: Create the following tablecats using ProjInd and monk1-full
    1. all attributes
    2. 0,1,4
    3. ConstCat predicting no(0).

  Stick these into an array with the last categorizer being the constCat
    predicting a no.
  Run CascadeCat and it should get 100%.
   */


void test_results()
{
   TableInducer indOne("TableInd: test CascadeCat", FALSE);
   indOne.read_data("monk1-full");
   indOne.train();
   Mcout << "table inducer/categorizer created" << endl;
//   indOne.display(Mcout);
   
   ProjectInd indTwo("ProjectInd: test CascadeCat");
   BaseInducer* innerIndPtr =
      new TableInducer("ProjectInd: inner TableInducer", FALSE);
   indTwo.set_wrapped_inducer(innerIndPtr);
   ASSERT(innerIndPtr == NULL);
   BoolArray* attr = new BoolArray(0, 6, FALSE);
   (*attr)[0] = TRUE; (*attr)[1] = TRUE; (*attr)[4] = TRUE;
   indTwo.set_project_mask(attr);
   ASSERT(attr == NULL);
   indTwo.read_data("monk1-full");
   indTwo.train();
   Mcout << "project inducer with TableInducer inside created: "
	 << indTwo << endl;

   
   ConstInducer indThree("ConstInd: test CascadeCat");
   indThree.read_data("monk1-full");
   indThree.train();
   Mcout << "const inducer for fallback category: "  << endl;
//   indThree.display(Mcout);
   
   PtrArray<Categorizer *> * cascadingCats =
      new PtrArray<Categorizer *>(0, 3);
   (*cascadingCats)[0] = indOne.get_categorizer().copy();
   (*cascadingCats)[1] = indTwo.get_categorizer().copy();
   (*cascadingCats)[2] = indThree.get_categorizer().copy();
   
   CascadeCat cc("CascadeCat: testing monk1-full", cascadingCats);
   
   InstanceList testSet("", "monk1-full.names", "monk1-full.test");
   InstanceList trainSet("",  "monk1-full.names", "monk1-full.data");
   CatTestResult catResult(cc, trainSet, testSet);

   Mcout << "Cascaded Categorizer Results" << endl << catResult << endl;
   
}


main()
{
   Mcout << endl << "t_CascadeCat starting." << endl;
   test_results();
   
   Mcout << endl << "t_CascadeCat competed." << endl;
   return 0;
}
      
