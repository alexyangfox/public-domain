// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests NullInducer.  
  Doesn't test : About half.
  Enhancements : Finish testing.
  History      : Brian Frasca                                       1/29/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <NullInducer.h>
#include <InstList.h>
#include <CatTestResult.h>

RCSID("MLC++, $RCSfile: t_NullInducer.c,v $ $Revision: 1.8 $")

main()
{
   cout << "Executing t_NullInducer" << endl;

   NullInducer nullInducer("Test t_NullInducer");
   TEST_ERROR("cannot call from a null inducer", nullInducer.read_data(""));
   TEST_ERROR("cannot call from a null inducer", nullInducer.train());
   TEST_ERROR("cannot call from a null inducer", nullInducer.display_struct());
   TEST_ERROR("cannot call from a null inducer", nullInducer.has_data());
   TEST_ERROR("cannot call from a null inducer", nullInducer.was_trained());
   TEST_ERROR("cannot call from a null inducer",
	      nullInducer.instance_bag());
   TEST_ERROR("cannot call from a null inducer", 
	      nullInducer.get_categorizer());

   NullInducer n2("Test t_NullInducer", FALSE);
  
   n2.read_data("monk1-full");
   n2.train();
   CatTestResult result(n2.get_categorizer(),
			n2.instance_bag(), "monk1-full");
   Mcout << result;
   ASSERT(result.accuracy() == 0.0);

   return 0; // return success to shell
}   


