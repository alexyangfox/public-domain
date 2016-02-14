/******************************************************************************
  Description  : Tests IBCaategorizer and IBInducer.
  Don't test   :
  Comments     : This is a simple version.
  History      : YeoGirl Yun
                   Initial Revision(.c .h)          		   9/09/94
******************************************************************************/

#include <basics.h>
#include <IBInducer.h>
#include <IBCategorizer.h>
#include <CatTestResult.h>
#include <CtrBag.h>
#include <CtrInstList.h>
#include <CValidator.h>

void print_result(const CatTestResult& result,
		  const IBInducer& ib,
		  const MString& dataFile)
{
   Mcout << "=================================================" << endl;
   Mcout << "Data File     : " << dataFile << endl;
   Mcout << "Voting        : ";
   if (ib.get_neighbor_vote() == IBInducer::equal)
      Mcout << "equal" << endl;
   else
      Mcout << "inverse-distance" << endl;
   Mcout << "NNK value     : ";
   if (ib.get_nnk_value() == IBInducer::numNeighbors)
      Mcout << "num-neighbors" << endl;
   else
      Mcout << "num-distances" << endl;   
   
   Mcout << "Editing       : ";
   if (ib.get_editing())
      Mcout << "Yes" << endl;
   else
      Mcout << "No" << endl;
   Mcout << "Normalization : ";
   if (ib.get_norm_method() == InstanceBag::interquartile)
      Mcout << "Interquartile" << endl;
   else if (ib.get_norm_method() == InstanceBag::extreme)
      Mcout << "Extreme" << endl;
   else
      Mcout << "None" << endl;
   Mcout << "Max Epochs    : " << ib.get_editing_max_epochs() << endl;
   Mcout << "K value       : " << ib.get_k_val() << endl;
   Mcout << "Weight Vector : " <<
      ((const IBCategorizer&)(ib.get_categorizer())).get_weights() << endl
	 << result;
   Mcout << "=================================================\n" << endl;   
}   



void test_editing_with_epoch(IBInducer& ib, int epoch,
			     InstanceBag::NormalizationMethod method,
			     const MString& dataFile)
{
   InstanceList* data = new InstanceList(dataFile);
   data->remove_inst_with_unknown_attr();
   ib.initialize();
   
   delete ib.assign_data(data);

   ib.set_editing(TRUE);
   ib.set_editing_max_epochs(epoch);
   ib.set_norm_method(method);
   ib.set_k_val();   
   ib.train();

   
   InstanceList testFile("", dataFile + ".names", dataFile + ".test");
   testFile.remove_inst_with_unknown_attr();
   CatTestResult result(ib.get_categorizer(),
			ib.instance_bag(),
                        testFile);
   print_result(result, ib, dataFile);
}




void test_weight_ib1(IBInducer& ib, int k)
{
   MString dataFile = "monk1"; // since weights are set, data file is also
			      // set. 
   InstanceList* data = new InstanceList(dataFile);
   data->remove_inst_with_unknown_attr();
   ib.initialize();
   delete ib.assign_data(data);
   ib.set_editing(FALSE);
   ib.set_k_val(k);   

   Array<Real> weights(0,6,1.0); // initially all 1's. we know monk1 has 6
			     // attributes.
   weights[0] = 1;
   weights[1] = 1;
   weights[2] = 0;
   weights[3] = 0;
   weights[4] = 1;
   weights[5] = 0;
   ib.set_weights(weights);
   ib.train();

   InstanceList testFile("", dataFile + ".names", dataFile + ".test");
   testFile.remove_inst_with_unknown_attr();
   CatTestResult result(ib.get_categorizer(),
			ib.instance_bag(),
                        testFile);
   print_result(result, ib, dataFile);   
}



Real test_ib1(IBInducer& ib, const MString& dataFile, int k,
	      IBInducer::NeighborVote vote = IBInducer::equal,
	      IBInducer::NnkValue nnkValue = IBInducer::numDistances)
{
   InstanceList* data = new InstanceList(dataFile);
   data->remove_inst_with_unknown_attr();

   ib.initialize();
   delete ib.assign_data(data);
   ib.set_editing(FALSE);   
   ib.set_k_val(k);
   ib.set_neighbor_vote(vote);
   ib.set_nnk_value(nnkValue);
   ib.train();

   InstanceList testFile("", dataFile + ".names", dataFile + ".test");
   testFile.remove_inst_with_unknown_attr();
   CatTestResult result(ib.get_categorizer(),
			ib.instance_bag(),
                        testFile);
   print_result(result, ib, dataFile);
   InstanceList* nullInstList = NULL;
   Real acc = result.accuracy();
   delete ib.assign_data(nullInstList);
   return acc;
}


void test_interquartile(IBInducer& ib,
			InstanceBag::NormalizationMethod  method,
			const MString& dataFile)
{
   InstanceList* data = new InstanceList(dataFile);
   data->remove_inst_with_unknown_attr();

   ib.initialize();
   
   delete ib.assign_data(data);
   ib.set_editing(FALSE);
   ib.set_norm_method(method);
   ib.set_k_val();   
   ib.train();

   
   InstanceList testFile("", dataFile + ".names", dataFile + ".test");
   testFile.remove_inst_with_unknown_attr();
   CatTestResult result(ib.get_categorizer(),
			ib.instance_bag(),
                        testFile);
   print_result(result, ib, dataFile);
}   

main()
{
   IBInducer ib("IBInducer");
   (void)test_ib1(ib, "t_IBInducer2", 1, IBInducer::equal,
		  IBInducer::numNeighbors);
   (void)test_ib1(ib, "t_IBInducer2", 1, IBInducer::equal,
		  IBInducer::numDistances);   
   (void)test_ib1(ib, "t_IBInducer3", 3, IBInducer::inverseDistance,
		  IBInducer::numNeighbors);
   (void)test_ib1(ib, "t_IBInducer3", 3, IBInducer::equal,
		  IBInducer::numNeighbors);         

   test_editing_with_epoch(ib, 1, InstanceBag::interquartile,  "t_IBInducer");
   test_editing_with_epoch(ib, 1, InstanceBag::extreme, "t_IBInducer");
   
   test_interquartile(ib, InstanceBag::interquartile, "t_IBInducer");
   test_interquartile(ib, InstanceBag::extreme, "t_IBInducer");

   test_editing_with_epoch(ib, 1,InstanceBag::interquartile ,  "iris");
   test_editing_with_epoch(ib, 2,InstanceBag::interquartile , "iris");
   test_editing_with_epoch(ib, 3,InstanceBag::interquartile , "iris");

   test_editing_with_epoch(ib, 1,InstanceBag::extreme , "iris");
   test_editing_with_epoch(ib, 2,InstanceBag::extreme , "iris");
   test_editing_with_epoch(ib, 3,InstanceBag::extreme , "iris");         

   // set_weights test
   test_weight_ib1(ib, 1);
   test_weight_ib1(ib, 3);
   test_weight_ib1(ib, 5);   

   Real iris = test_ib1(ib, "iris", 1, IBInducer::equal);
   Real iris1 = test_ib1(ib, "iris", 1, IBInducer::equal);
   ASSERT(iris = iris1);
   
   (void)test_ib1(ib, "iris",3, IBInducer::equal);
   (void)test_ib1(ib, "iris",3, IBInducer::inverseDistance);
   (void)test_ib1(ib, "iris",3, IBInducer::equal, IBInducer::numNeighbors);
   (void)test_ib1(ib, "iris",3, IBInducer::inverseDistance,
		  IBInducer::numNeighbors);
   return 0;
}

   


