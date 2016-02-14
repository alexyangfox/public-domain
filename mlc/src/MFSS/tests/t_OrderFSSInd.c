// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Order FSS tester
  Doesn't test : Most everything. Just runs train and train&test.
  Enhancements :
  History      : Robert Allen and Dan Sommerfield                  1/27/95
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <ProjectInd.h>
#include <CatTestResult.h>
#include <CtrInstList.h>
#include <OrderFSSInd.h>
#include <ListHOODGInd.h>

RCSID("MLC++, $RCSfile: t_OrderFSSInd.c,v $ $Revision: 1.3 $")


void test_inducer()
{
   Inducer *ind = new ListHOODGInducer("ofss");
   OrderFSSInducer fss("testing", ind);
   fss.set_user_options("OFSS_");

   InstanceList* trainSet =
      new CtrInstanceList("", "monk1-full.names", "monk1-full.data");
   InstanceList* trainSetCp =
      &(trainSet->clone()->cast_to_instance_list());

   fss.assign_data(trainSet);

   fss.train();
   CtrInstanceList testSet("", "monk1-full.names", "monk1-full.test");
   CatTestResult catResult(fss.get_categorizer(),
			   fss.instance_bag(), testSet);

   Mcout << "Projected Categorizer Results" << endl << catResult << endl;

   Mcout << "Proceding with regular train & test" << endl;


   Real acc = fss.train_and_test(trainSetCp, testSet);
   Mcout << "Accuracy found is " << acc << endl;

   delete trainSetCp;
}



main()
{
   Mcout << endl << "t_FSSInducer start." << endl;
   test_inducer();
   
   Mcout << "t_FSSInducer completed." << endl;
   return 0;
}
      
