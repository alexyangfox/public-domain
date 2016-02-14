// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Incompete tester for C45 auto-param Inducer.
  Doesn't test : 
  Enhancements :
  History      : 
***************************************************************************/
#include <basics.h>
#include <C45APInducer.h>
#include <InstList.h>
#include <FileNames.h>

RCSID("MLC++, $RCSfile: t_C45APInducer.c,v $ $Revision: 1.3 $")


void test_inducer()
{
   InstanceList trainList("monk1-full");
   InstanceList testList("", "monk1-full.names", "monk1-full.test");
   C45APInducer inducer("C45 test");
   inducer.set_user_options("AP_");
   Real acc = inducer.train_and_test(&trainList, testList);
   Mcout << "Accuracy found is " << acc << endl;
}



main()
{
   Mcout << endl << "t_C45APInducer start." << endl;
   test_inducer();
   
   Mcout << "t_C45APInducer completed." << endl;
   return 0;
}
      
