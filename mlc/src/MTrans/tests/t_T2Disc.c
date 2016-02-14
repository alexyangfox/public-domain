// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests the T2Discretizor.
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                        12/3/95
                   Initial revision based on t_EntropyDisc
***************************************************************************/

#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <T2Disc.h>

RCSID("MLC++, $RCSfile: t_T2Disc.c,v $ $Revision: 1.2 $")

/***************************************************************************
  Description : Discretizes a simple bag using entropy. I use breast with
                  attribute 1 projected out of it since it's small and easy
		  to test.
  Comments    :
***************************************************************************/
void test_t2_discretizor(Bool defaultIntervals)
{
   LogOptions logOptions;
   InstanceList bag("german");  // some attributes have no intervals.
   bag.display();
   Mcout << endl;

   Mcout << "------------Original Schema ----------------" << endl;
   bag.get_schema().display();
   //Create the Discretizors to be used in the process
   //(this must be done first)
   PtrArray<RealDiscretizor*>* T2Disc;
   if (defaultIntervals)
      T2Disc = create_t2_discretizors(logOptions, bag, 0);
   else {
      Array<int> intervals(0, bag.num_attr(), 7);
      intervals[1] = 2;
      T2Disc = create_t2_discretizors(logOptions, bag, intervals);
   }
      
   
   // Create a new bag with the discretized attributes
   InstanceBag* newBag = discretize_bag(bag, T2Disc);
   Mcout << "-------------- New Schema ------------------" << endl;
   (newBag->get_schema()).display();
   Mcout << "-----------Real Discretizors----------------" << endl;


   //Clone and Display each of the discretizors

   PtrArray<RealDiscretizor*> clones(T2Disc->size());
   for(int i = 0; i < T2Disc->size(); i++){
      if(T2Disc->index(i)){
	 T2Disc->index(i)->display();
	 clones.index(i) = new T2Discretizor(
	    (const T2Discretizor&)*T2Disc->index(i), ctorDummy);
	 ASSERT(*clones.index(i) == *T2Disc->index(i));
      }
   }

   
   Mcout << " ----------Discretized bag -----------------" << endl;
   newBag->display();
   

   delete newBag;
   delete T2Disc;
}


main()
{
   Mcout << "t_T2Disc executing" << endl;
   test_t2_discretizor(TRUE);
   test_t2_discretizor(FALSE);
   return 0; // return success to shell
}   
