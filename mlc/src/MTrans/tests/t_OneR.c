// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the OneR discretizor.
  Doesn't test : 
  Enhancements :
  History      : James Dougherty                                     12/28/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <RealDiscretizor.h>
#include <OneR.h>

RCSID("MLC++, $RCSfile: t_OneR.c,v $ $Revision: 1.5 $")


/***************************************************************************
  Description : Tests the basic functionality of the label value
                  counts array class.
  Comments    :
***************************************************************************/
void test_basic_lvc()
{
   LabelValueCounts lvc(10);
   LabelValueCounts sumLvc(10);
   for(int i = 0; i < 3; i++){
      lvc.increment(0); //lvc[0] is 1
      lvc.increment(1);
      lvc.increment(0); //lvc[0] is 2
      lvc.increment(1);
      ASSERT(lvc.tied_majority());
      #ifndef MEMCHECK
      TEST_ERROR("LabelValueCounts::majority_category: it is an error to "
		 "invoke this method when there is a tie.",
		 lvc.majority_category() );

      TEST_ERROR("LabelValueCounts::majority_value: it is an error to invoke "
		 "this method when there is a tie.", lvc.majority_value() );
      #endif
      lvc.increment(0); //lvc[0] is 3
      ASSERT(lvc.majority_value() == 3);
      ASSERT(lvc.majority_category() == 0);
      lvc.increment(1);
      ASSERT(lvc.tied_majority());
      lvc.increment(1);
      ASSERT(lvc.majority_category() == 1);
      ASSERT(lvc.majority_value() == 4);
      sumLvc.increment(0);
      ASSERT( 1 + 4*i == sumLvc.majority_value());
      sumLvc += lvc;
      lvc.reset();
   }
}


/***************************************************************************
  Description : Tests that the OneR discretizor is working correctly
  Comments    :
***************************************************************************/
void test_oneR_discretizor()
{
   InstanceList bag("t_OneR");
   bag.display();
   Mcout << endl;
   LogOptions logOptions;
   Mcout << "------------Original Schema ----------------" << endl;
   bag.get_schema().display();
   //Create the Discretizors to be used in the process
   //(this must be done first)
   PtrArray<RealDiscretizor*>* oneRDisc =
      create_OneR_discretizors(logOptions, bag,3);
   
   // Create a new bag with the discretized attributes
   InstanceBag* newBag = discretize_bag(bag, oneRDisc);
   Mcout << "-------------- New Schema ------------------" << endl;
   (newBag->get_schema()).display();
   Mcout << "-----------Real Discretizors----------------" << endl;

   PtrArray<RealDiscretizor*>* clones = new
      PtrArray<RealDiscretizor*>(oneRDisc->size());
   
   //Clone and Display each of the discretizors
   for(int i = 0; i < oneRDisc->size(); i++){
      if(oneRDisc->index(i)){
	 oneRDisc->index(i)->display();
	 clones->index(i) = new OneR((class OneR&)
				     *oneRDisc->index(i),ctorDummy);
	ASSERT(*clones->index(i) == *oneRDisc->index(i)); 
      }
   }

   Mcout << " ----------Discretized bag -----------------" << endl;
   newBag->display();
   
   delete oneRDisc;
   delete clones;
   delete newBag;
}

main()
{
   Mcout << "t_OneR executing" << endl;
   test_basic_lvc();
   test_oneR_discretizor();
   Mcout << "t_OneR successful" << endl;
   return 0; // return success to shell
}   
