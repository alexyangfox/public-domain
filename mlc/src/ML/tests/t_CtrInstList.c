// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Test CtrInstanceList
                 The datafile used is a discretized Golf database.
                 This is Quinlan's favorite example.  C4.5 book page 18.
                 This file is heavily based on t_BagCounters.c.
  Doesn't test : 
  Enhancements :
  History      : Ronny Kohavi                                       9/04/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <CtrInstList.h>
#include <AttrCat.h>
#include <MRandom.h>

RCSID("MLC++, $RCSfile: t_CtrInstList.c,v $ $Revision: 1.15 $")

#ifdef foo
#  include "t_golf.c"
#else
#  include "../../MCore/tests/t_golf.c"
#endif

void compare_counters(CtrInstanceList& bag)
{
   const BagCounters& bc = bag.counters();
   ASSERT(bc.label_count(dontPlay) == 5);
   ASSERT(bc.label_count(play) == 9);
   ASSERT(bc.val_count(dontPlay, 0, sunny) == 3);
   ASSERT(bc.val_count(dontPlay, 0, overcast) == 0);   
   ASSERT(bc.val_count(dontPlay, 0, rain) == 2);   
   ASSERT(bc.val_count(play, 0, sunny) == 2);   
   ASSERT(bc.val_count(play, 0, overcast) == 4);   
   ASSERT(bc.val_count(play, 0, rain) == 3);   
   ASSERT(bc.val_count(play, 3, windyT) == 3);   
   ASSERT(bc.val_count(play, 3, windyF) == 6);   
   ASSERT(bc.val_count(dontPlay, 3, windyT) == 3);   
   ASSERT(bc.val_count(dontPlay, 3, windyF) == 2);   
   ASSERT(bc.OK() == 14);
   ASSERT(bag.num_instances() == 14);
}


/***************************************************************************
  Description :  Splits the bag based on the first attribute.  Displays each
                  bag in "t_CtrBag.out1"
  Comments    :
***************************************************************************/
static void check_split(const CtrInstanceList& splitList)
{
   AttrCategorizer ac(splitList.get_schema(), 0,
		      "AttrCat on outlook");
   CtrBagPtrArray* cbpa = splitList.ctr_split(ac);
   MLCOStream splitStream("t_CtrInstList.out1");
   for (int i = cbpa->low(); i <= cbpa->high(); i++)
      (*cbpa)[i]->display(splitStream);
   delete cbpa;
}


/***************************************************************************
  Description : Checks that del_front() properly updates the counters.
  Comments    :
***************************************************************************/
void check_delete(CtrInstanceList& bag)
{
   bag.remove_front();
   bag.remove_front();
// These instances should have been removed
//sunny, 85, 85, false, Don't Play
//sunny, 80, 90, true, Don't Play

   const BagCounters& bc = bag.counters();
   ASSERT(bc.label_count(dontPlay) == 3);
   ASSERT(bc.label_count(play) == 9);
   ASSERT(bc.val_count(dontPlay, 0, sunny) == 1);
   ASSERT(bc.val_count(dontPlay, 0, overcast) == 0);   
   ASSERT(bc.val_count(dontPlay, 0, rain) == 2);   
   ASSERT(bc.val_count(play, 0, sunny) == 2);   
   ASSERT(bc.val_count(play, 0, overcast) == 4);   
   ASSERT(bc.val_count(play, 0, rain) == 3);   
   ASSERT(bc.val_count(play, 3, windyT) == 3);   
   ASSERT(bc.val_count(play, 3, windyF) == 6);   
   ASSERT(bc.val_count(dontPlay, 3, windyT) == 2);   
   ASSERT(bc.val_count(dontPlay, 3, windyF) == 1);   
   ASSERT(bc.OK() == 12);
   ASSERT(bag.num_instances() == 12);
}

/***************************************************************************
  Description : Check that the casting methods don't cause fatal_errors.
  Comments    :
***************************************************************************/
void check_casts(CtrInstanceList& ctrList)
{
   const CtrInstanceList& constCtrList = ctrList;
   CtrInstanceBag& cib = ctrList.cast_to_ctr_instance_bag();
   const CtrInstanceBag& ccib = constCtrList.cast_to_ctr_instance_bag();
   const CtrInstanceList& ccil = ccib.cast_to_ctr_instance_list();
   CtrInstanceList& cil = cib.cast_to_ctr_instance_list();
}


void check_corrupt_unknown()
{
   MRandom mrandom(4975497);

   CtrInstanceList bag("crx");
   bag.remove_inst_with_unknown_attr(); // no unknowns now.
   
   CtrInstanceBag bag2(bag, ctorDummy);

   bag2.corrupt_values_to_unknown(1, mrandom);
   // make sure everything is unknown.
   for (Pix pix = bag2.first(); pix; bag2.next(pix)) { 
      const InstanceRC& instance = bag2.get_instance(pix);
      for (int i = 0; i < bag2.num_attr(); i++) { // for each attribute
	 const AttrInfo& ai = bag2.attr_info(i);
	 ASSERT(ai.is_unknown(instance[i]));
      }
   }

   Mcout << "Check corrupt unknown part 1 OK" << endl;

   CtrInstanceBag bag3(bag, ctorDummy);
   bag3.corrupt_values_to_unknown(0, mrandom);

   ASSERT(bag == bag3);   
   Mcout << "Check corrupt unknown part 2 OK" << endl;
   
   // corrupt half the instances
   bag.corrupt_values_to_unknown(0.5/bag.num_attr(), mrandom);
   CtrInstanceBag bag4(bag, ctorDummy);

   bag.remove_all_instances();
   bag.OK();
   bag.OK();

   int numInst = bag4.num_instances();
   bag4.remove_inst_with_unknown_attr(); // no unknowns now.
   // Should be over half because two unknowns can be in an instance.
   // The total number of unknowns should be about half.
   Mcout << "Check corrupt had " << numInst << " instances, now "
            " it has " << bag4.num_instances()
	 << " (should be somewhat over a half)" << endl;
   bag4.OK();
   
}



main()
{
   cout << "t_CtrInstList executing" << endl;

   CtrInstanceList bag("t_CtrInstList"); // Discrete Golf database.
   bag.OK(); 
   def_consts(bag);

   compare_counters(bag);

   check_split(bag);

   // majority category is "Play"
   ASSERT(bag.InstanceBag::majority_category() == FIRST_CATEGORY_VAL);
   ASSERT(bag.majority_category() == FIRST_CATEGORY_VAL);
   
   check_delete(bag);

   check_casts(bag);
   
   CtrInstanceList bag2("t_CtrInstList2"); // Golf database.   
   bag2.OK();
   CtrInstanceList bag3(bag2, ctorDummy); // copy-ctor.
   bag3.OK();
   ASSERT(bag2.counters() == bag3.counters());
   bag3.remove_front();
   ASSERT(bag2.counters() != bag3.counters());   
   bag2.remove_front();
   ASSERT(bag2.counters() == bag3.counters());   

   BoolArray mask(4);
   mask[0] = 1; mask[1] = 0; mask[2] = 1; mask[3] = 1;
   InstanceList& bag4 = bag2.project(mask)->cast_to_instance_list();
   delete &bag4;

   check_corrupt_unknown();
   
   return 0; // return success to shell
}   
