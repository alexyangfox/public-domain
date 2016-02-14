// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tester for OC1. Tries all the option combinations on
                   monk1-full.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                     6/15/95
                   Initial revision
***************************************************************************/


#include <basics.h>
#include <OC1Inducer.h>
#include <InstList.h>



void test_oc1(Bool axisParallelOnly,
	      Bool cartLinearCombinationMode,
	      Real pruningRate,
	      const MString& dataFile)
{
   OC1Inducer oc1("OC1 Inducer");

   // set options.
   oc1.set_axis_parallel_only_opt(axisParallelOnly);
   oc1.set_cart_linear_combination_mode(cartLinearCombinationMode);
   oc1.set_pruning_rate(pruningRate);
      
   InstanceList trainSet(dataFile);
   InstanceList testSet("", dataFile + ".names", dataFile + ".test");
   
   Mcout << "DATAFILE : " << dataFile << endl;
   Mcout << "AXIS PARALLEL ONLY : ";
   if (oc1.get_axis_parallel_only_opt())
      Mcout << "TRUE" << endl;
   else
      Mcout << "FALSE" << endl;
   Mcout << "CART LINEAR COMBINATION MODe : ";
   if (oc1.get_cart_linear_combination_mode())
      Mcout << "TRUE" << endl;
   else
      Mcout << "FALSE" << endl;
   Mcout << "PRUNING RATE : " << oc1.get_pruning_rate() << endl;
   Mcout << "ACCURACY : " << oc1.train_and_test(&trainSet, testSet) << endl;
}


int main()
{
   Mcout << "testing t_OC1Inducer.c" << endl;

   test_oc1(TRUE, TRUE, 0.2, "monk1-full");
   test_oc1(TRUE, FALSE, 0.2, "monk1-full");   
   test_oc1(FALSE, TRUE, 0.2, "monk1-full");
   test_oc1(FALSE, FALSE, 0.2, "monk1-full");

   return 0;
}

