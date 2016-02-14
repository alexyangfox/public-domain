// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the C45Discretizor.
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                        12/3/95
                   Initial revision based on t_EntropyDisc
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <C45Disc.h>

RCSID("MLC++, $RCSfile: t_C45Disc.c,v $ $Revision: 1.1 $")

/***************************************************************************
  Description : Discretizes a simple bag using entropy. I use breast with
                  attribute 1 projected out of it since it's small and easy
		  to test.
  Comments    :
***************************************************************************/
void test_c45_discretizor()
{
   LogOptions logOptions;
   InstanceList bag("crx");
   bag.display();
   Mcout << endl;

   Mcout << "------------Original Schema ----------------" << endl;
   bag.get_schema().display();
   //Create the Discretizors to be used in the process
   //(this must be done first)
   PtrArray<RealDiscretizor*>* C45Disc=
      create_c45_discretizors(logOptions, bag);
   
   // Create a new bag with the discretized attributes
   InstanceBag* newBag = discretize_bag(bag, C45Disc);
   Mcout << "-------------- New Schema ------------------" << endl;
   (newBag->get_schema()).display();
   Mcout << "-----------Real Discretizors----------------" << endl;


   //Clone and Display each of the discretizors

   PtrArray<RealDiscretizor*> clones(C45Disc->size());
   for(int i = 0; i < C45Disc->size(); i++){
      if(C45Disc->index(i)){
	 C45Disc->index(i)->display();
	 clones.index(i) = new C45Discretizor(
	    (const C45Discretizor&)*C45Disc->index(i), ctorDummy);
	 ASSERT(*clones.index(i) == *C45Disc->index(i));
      }
   }

   
   Mcout << " ----------Discretized bag -----------------" << endl;
   newBag->display();
   

   delete newBag;
   delete C45Disc;
}


main()
{
   Mcout << "t_C45Disc executing" << endl;
   test_c45_discretizor();
   return 0; // return success to shell
}   
