/******************************************************************************
  Description  : Tests TableCategorizer and TableInducer.
  Don't test   :
  Comments     : 
  History      : YeoGirl Yun
                   Initial Revision(.c .h)          		   5/01/94
******************************************************************************/

#include <basics.h>
#include <TableInducer.h>
#include <TableCat.h>
#include <CatTestResult.h>
#include <MLCStream.h>
#include <CtrInstList.h>

RCSID("MLC++, $RCSfile: t_TableInducer.c,v $ $Revision: 1.16 $")

/******************************************************************************
  Description  : This function test the correctness of TableInducer
                   without default category. Four datafiles,
		   monk1-full1, parity5+5, crx, and vote are tested.
		 Returns the number of correct predictions.
  Comments     :
******************************************************************************/
void test_on_datafile(const MString& fileStem, const int expectedCorrect)
{
   TableInducer tabInducer(fileStem,FALSE);
   tabInducer.read_data(fileStem);
   tabInducer.train();

   CatTestResult result(tabInducer.get_categorizer(),
			tabInducer.instance_bag(),
			fileStem);
   ASSERT(result.num_correct() == expectedCorrect);
   Mcout <<  "TableInducer : " << fileStem << " tests OK without"
             " default category." << endl;

   // tests del_instance, add_instance, and find_instance methods.
   int numInst = tabInducer.instance_bag().num_instances();
   for (int i = 0; i < numInst; i++) {
      Pix pix = tabInducer.instance_bag().first();
      const InstanceRC inst =
	 tabInducer.del_instance(pix);
      const AugCategory& ac =
	 tabInducer.get_categorizer().categorize(inst);
      if (((TableCategorizer&)(tabInducer.get_categorizer())).
	  find_instance(inst)) {
	 // duplicate instances.
	 const Category correctCat =
	    inst.label_info().get_nominal_val(inst.get_label());
	 ASSERT((Category)ac == correctCat);
      }
      else // unique instance
	 ASSERT((Category)ac == UNKNOWN_CATEGORY_VAL);
      tabInducer.add_instance(inst);
      const AugCategory& ac2 =  tabInducer.get_categorizer().categorize(inst);
      ASSERT((Category)ac2 != UNKNOWN_CATEGORY_VAL);
   }
}


/******************************************************************************
  Description  : This functions tests TableInducer with default
                   category capability. 'expectedCorrect' is the
		   number of correct predictions without default category.
  Comments     :
******************************************************************************/
void test_default_category(const MString& fileStem, int expectedCorrect)
{
   CtrInstanceList trainData(fileStem);

   // split train data by categories, in order to test only majority
   //   category instances in the training set.
   BagPtrArray *bpa = trainData.split_by_label();
   
   // now test using only majority category instances without default
   //   category behaviour in order to know how many instances with
   //   the majority category in the test set.
   TableInducer inducer("majority category only",FALSE);
   inducer.assign_data((*bpa)[trainData.majority_category()]);
   inducer.train();

   CatTestResult result(inducer.get_categorizer(),
                	inducer.instance_bag(),
			fileStem);
   int s1 = result.num_correct();

   // now split test data by categories, in order to get the number of
   // instances with majority category in the test set.
   InstanceList testData(fileStem,".names",".test");
      
   BagPtrArray *tbpa = testData.split_by_label();

   int s2 = (*tbpa)[testData.majority_category()]->num_instances();
   
   // now test whole training set with default category behaviour.
   TableInducer testInducer("Table Inducer with default category",TRUE);
   testInducer.read_data(fileStem);
   testInducer.train();
   CatTestResult result2(testInducer.get_categorizer(),
			 testInducer.instance_bag(),
			 fileStem);
   int s3 = result2.num_correct();

   ASSERT( s3 == expectedCorrect + s2 - s1 );
   Mcout << "TableInducer : " << fileStem << " default category"
	         " behaviour tests OK." << endl;
   // delete bag array pointers because ownerships were transferred.
   delete bpa;
   delete tbpa;
}
   

main()
{
   // tests TableInducer without default category behaviour.
   test_on_datafile("monk1-full",124);
   test_on_datafile("parity5+5",100);
   test_on_datafile("crx",0);
   test_on_datafile("vote",43);
   
   // tests TableInducer with default category behaviour.
   test_default_category("monk1-full",124);
   test_default_category("parity5+5",100);
   test_default_category("crx",0); 
   test_default_category("vote",43);

   return 0;
}
