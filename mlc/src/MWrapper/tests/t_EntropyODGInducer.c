// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.

/***************************************************************************
  Description  : Tests EntropyODTInducer methods.
  Doesn't test : 
  Enhancements : 
  History      : Chia-Hsin Li                                       9/08/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <AugCategory.h>
#include <entropy.h>
#include <DynamicArray.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <AttrCat.h>
#include <ThresholdCat.h>
#include <ODGInducer.h>
#include <EntropyODGInducer.h>

RCSID("MLC++, $RCSfile: t_EntropyODGInducer.c,v $ $Revision: 1.4 $")

main()
{
   Mcout << "t_EntropyODTInducer executing" << endl;
      
   EntropyODGInducer inducer("test");
 
   inducer.read_data("monk1-full");
   inducer.train();
   Mcout << "End train" << endl;

#ifdef INTERACTIVE
      DotGraphPref pref;
      MLCOStream out3(XStream);
      inducer.display_struct(out3,pref);
#endif INTERACTIVE
   CatTestResult result(inducer.get_categorizer(),
                        inducer.instance_bag(), "monk1-full");
   result.display(Mcout);
   
   return 0; // return success to shell
}   
