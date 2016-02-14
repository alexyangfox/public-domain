// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test ListHOODGInducer
  Doesn't test : 
  Enhancements : 
  History      : Dan Sommerfield                                   5/31/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <CtrInstList.h>
#include <ListHOODGInd.h>
#include <CatTestResult.h>

void basic_test(void)
{
   InstanceList *instList = new InstanceList("monk1-full");

   // create order vector with "correct" attributes for monk1
   Array<int> orderVector(3);
   orderVector[0] = 4;
   orderVector[1] = 0;
   orderVector[2] = 1;

   // create the HOODGInducer
   ListHOODGInducer ind("test ListHOODG");

   // set its order vector and train
   ind.set_order(orderVector);
   ind.assign_data(instList);
   ind.train();

   CatTestResult result(ind.get_categorizer(),
			ind.instance_bag().cast_to_instance_list(),
			"monk1-full");
   Mcout << "results: " << endl << result;

   MLCOStream out("t_ListHOODGInd.out1");
   DotGraphPref pref;
   ind.display_struct(out, pref);

   orderVector[0] = 0;
   orderVector[1] = 1;
   orderVector[2] = 4;
   ind.set_order(orderVector);
   ind.train();
   CatTestResult result1(ind.get_categorizer(),
			ind.instance_bag().cast_to_instance_list(),
			"monk1-full");
   Mcout << "results: " << endl << result1;

   MLCOStream out1("t_ListHOODGInd.out2");
   DotGraphPref pref1;
   ind.display_struct(out1, pref1);
}

main()
{
   Mcout << "t_ListHOODGInd starting" << endl;
   basic_test();
   Mcout << "success" << endl;
   return 0;
}

   
   
