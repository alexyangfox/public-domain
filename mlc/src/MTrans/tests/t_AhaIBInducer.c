// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tests AhaIBInducer. Run it on monk1 using four different
                   options.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                   June 2, 1995
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <AhaIBInducer.h>
#include <InstList.h>



void test_ib(IBClass ibClass, const MString& dataFile)
{
   AhaIBInducer ib("Aha IB Inducer");

   ib.set_ib_class(ibClass);
   InstanceList trainSet(dataFile);
   InstanceList testSet("", dataFile + ".names", dataFile + ".test");
   
   Mcout << " DATAFILE : " << dataFile << endl;
   Mcout << " IB CLASS : IB" << ibClass + 1 << endl;
   Mcout << " Accuracy   : " << ib.train_and_test(&trainSet, testSet) << endl;
}


void test_warning()
{
   AhaIBInducer ib("Aha IB Inducer");

   InstanceList trainSet("monk1");
   InstanceList testSet(trainSet.get_schema()); // empty test bag.

   Mcout << " Running aha's IBL, should have warning messages " << endl;
   Real acc = ib.train_and_test(&trainSet, testSet);
   Mcout << " Accuracy : " << acc << endl;
}


int main()
{
   Mcout << "testing t_AhaIBInducer.c" << endl;

   test_ib(ib1, "crx");
   test_ib(ib2, "crx");
   test_ib(ib3, "crx");
   test_ib(ib4, "crx");

   test_warning();
   return 0;
}

