// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests that the EntropyDiscretizor is working properly.
  Doesn't test :
  Enhancements :
  History      : James Dougherty                                   12/28/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <EntropyDisc.h>

RCSID("MLC++, $RCSfile: t_EntropyDisc.c,v $ $Revision: 1.10 $")

/***************************************************************************
  Description : Discretizes a simple bag using entropy. I use breast with
                  attribute 1 projected out of it since it's small and easy
		  to test.
  Comments    :
***************************************************************************/
void test_entropy_discretizor()
{
   LogOptions logOptions;
   InstanceList bag("t_EntropyDisc");
//   InstanceList bag("golf");
   bag.display();
   Mcout << endl;

   Mcout << "------------Original Schema ----------------" << endl;
   bag.get_schema().display();
   //Create the Discretizors to be used in the process
   //(this must be done first)
   PtrArray<RealDiscretizor*>* entropyDisc=
      create_Entropy_discretizors(logOptions, bag,1,100);
   
   // Create a new bag with the discretized attributes
   InstanceBag* newBag = discretize_bag(bag, entropyDisc);
   Mcout << "-------------- New Schema ------------------" << endl;
   (newBag->get_schema()).display();
   Mcout << "-----------Real Discretizors----------------" << endl;


   //Clone and Display each of the discretizors

   PtrArray<RealDiscretizor*> clones(entropyDisc->size());
   for(int i = 0; i < entropyDisc->size(); i++){
      if(entropyDisc->index(i)){
	 entropyDisc->index(i)->display();
	 clones.index(i) = new EntropyDiscretizor(
	    (const EntropyDiscretizor&)*entropyDisc->index(i), ctorDummy);
	 ASSERT(*clones.index(i) == *entropyDisc->index(i));
      }
   }

   
   Mcout << " ----------Discretized bag -----------------" << endl;
   newBag->display();
   

   delete newBag;
   delete entropyDisc;
}


main()
{
   Mcout << "t_EntropyDisc executing" << endl;
   test_entropy_discretizor();
   return 0; // return success to shell
}   
