// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the read_c45_tree() function defined in
                 C45Tree.c, (declared in C45Inducer.h).
		 Creates a DecisionTree categorizer using c45 datafiles
		 (t_c45Tree.Good-Tree, monk1-full.{names,data,test}
		  t_c45Tree.Evil-Tree).
  Doesn't test : 
  Enhancements : Enhancement for the tester should be to actually
                 compare the trees, but we don't have that ability now.
  History      : James Dougherty                                 05/04/94
                 Initial revision
***************************************************************************/

#include <basics.h>
#include <MString.h>
#include <MLCStream.h>
#include <C45Inducer.h>
#include <errorUnless.h>
#include <CatTestResult.h>
#include <DTCategorizer.h>
#include <InstList.h>
#include <math.h> //for fabs()

RCSID("MLC++, $RCSfile: t_C45Tree.c,v $ $Revision: 1.4 $")


main()
{
   cout << "t_C45Tree executing..." << endl;
   MString filestem = "monk1-full";
   InstanceList instanceList(filestem);

   //Test out a good tree
   MLCIStream instream1("t_c45Tree.Good-Tree");
   DecisionTree *testTree1 = read_c45_tree(instream1,
				   instanceList.get_schema());   

   DTCategorizer testCat(testTree1, "testTree",
			 instanceList.num_label_values());

   CatTestResult catTestResult(testCat, instanceList, filestem);
   cout << "Accuracy is: " << catTestResult.accuracy() << endl;

   //C4.5 gives the error rate which is 100 - accuracy
   //Since the error from running C4.5 on monk1-full is
   // 100 - 24.3 , we assert that the accuracy is around 75.7
   const Real c45Acc = 0.757;
   ASSERT(fabs(catTestResult.accuracy() - c45Acc) < 0.001);
   //Test out a bad tree
   MLCIStream instream2("t_c45Tree.Evil-Tree");
   TEST_ERROR("read_c45_tree: Undefined nodeType",
	      read_c45_tree(instream2, 
			    instanceList.get_schema()));

   delete testTree1;
   return 0; // return success to shell
}   













