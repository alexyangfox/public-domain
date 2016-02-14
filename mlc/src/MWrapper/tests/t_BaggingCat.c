// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tests given IBCategorizer and DTCategorizer whether
                   BaggingCategorizer becomes one of them with appropriate
		   weight setting.
  Comments     :
  Don't Tests  : 
  History      : Yeogirl Yun                                  5/5/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <IBInducer.h>
#include <IBCategorizer.h>
#include <ID3Inducer.h>
#include <BaggingCat.h>
#include <CatTestResult.h>
#include <IBInducer.h>
#include <InstList.h>

void test_bagging_cat(const MString& dataFile)
{
   Real acc1, acc2;

   IBInducer ib(dataFile);

   InstanceList* data = new InstanceList(dataFile);
   delete ib.assign_data(data);

   ib.initialize();
   ib.set_k_val(1);   
   ib.train();

   IBCategorizer *ibCat1 = (IBCategorizer *)ib.get_free_categorizer();
   ib.initialize();
   ib.set_k_val(1);   
   ib.train();
   IBCategorizer *ibCat1one = (IBCategorizer *)ib.get_free_categorizer();   

   ib.initialize();
   ib.set_k_val(3);
   ib.train();   
   IBCategorizer *ibCat2 = (IBCategorizer *)ib.get_free_categorizer();
   ib.initialize();
   ib.set_k_val(3);
   ib.train();   
   IBCategorizer *ibCat2one = (IBCategorizer *)ib.get_free_categorizer();
   
   PtrArray<Categorizer *> *catSet = new PtrArray<Categorizer *>(2);

   catSet->index(0) = ibCat1;
   catSet->index(1) = ibCat2;

   Array<Real>* weight = new Array<Real>(2);
   weight->index(0) = 1;
   weight->index(1) = 0;

   Mcout << " Testing two IBCat's with weight (1, 0) " << endl;
   {
      BaggingCat bagCat("bagging cat", catSet, weight);
      InstanceList testFile("", dataFile + ".names", dataFile + ".test");

      {
	 CatTestResult result(bagCat,
			      ib.instance_bag(),
			      testFile);
	 acc1 = result.accuracy();
	 Mcout << " BagCat acc : " << acc1 << endl;
      }

      {
	 CatTestResult result((const Categorizer &)*ibCat1one,
			      ib.instance_bag(),
			      testFile);
	 acc2 = result.accuracy();
	 Mcout << " The first IBCat acc : " << acc2 << endl;
      }

      ASSERT(acc1 == acc2);
      Mcout << "Success! " << endl;
   }

   ib.initialize();
   ib.set_k_val(1);   
   ib.train();


   ibCat1 = (IBCategorizer *)ib.get_free_categorizer();

   ib.initialize();
   ib.set_k_val(3);
   ib.train();
   ibCat2 = (IBCategorizer *)ib.get_free_categorizer();

   delete catSet;
   catSet = new PtrArray<Categorizer *>(2);

   catSet->index(0) = ibCat1;
   catSet->index(1) = ibCat2;

   delete weight;
   weight = new Array<Real>(2);
   weight->index(0) = 0;
   weight->index(1) = 1;

   Mcout << " Testing two IBCat's with weight (0, 1) " << endl;
   {
      BaggingCat bagCat("bagging cat", catSet, weight);
      InstanceList testFile("", dataFile + ".names", dataFile + ".test");

      {
	 CatTestResult result(bagCat,
			      ib.instance_bag(),
			      testFile);
	 acc1 = result.accuracy();
	 Mcout << " BatCat acc : " << acc1 << endl;
      }

      {
	 CatTestResult result((const Categorizer &)*ibCat2one,
			      ib.instance_bag(),
			      testFile);
	 acc2 = result.accuracy();
	 Mcout << " The second IBCat acc : " << acc2 << endl;
      }

      ASSERT(acc1 == acc2);
      Mcout << "Success!" << endl;
   }

   delete ibCat1one;
   delete ibCat2one;
   delete catSet;
   delete weight;
}


int main()
{
   test_bagging_cat("monk1");

   return 0;
}
