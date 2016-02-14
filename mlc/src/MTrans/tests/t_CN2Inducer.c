// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : Tester for CN2. Tries all the option combinations on
                   monk1-full.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                     6/15/95
                   Initial revision
***************************************************************************/


#include <basics.h>
#include <CN2Inducer.h>
#include <InstList.h>



void test_CN2(const MString& dataFile)
{
   CN2Inducer cn("CN2 Inducer");

   // set options.
   InstanceList trainSet(dataFile);
   InstanceList testSet("", dataFile + ".names", dataFile + ".test");

   Mcout << "DATAFILE : " << dataFile << endl;
   Real acc = cn.train_and_test(&trainSet, testSet);
   Mcout << "ACCURACY : " << acc << endl << endl;
}


int main()
{
   Mcout << "testing t_CN2Inducer.c" << endl;

   test_CN2("monk1-full");
   test_CN2("glass");
   test_CN2("crx");

   Mcout << "Success!" << endl;
   return 0;
}

