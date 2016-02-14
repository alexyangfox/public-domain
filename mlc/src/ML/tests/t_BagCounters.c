// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.

/***************************************************************************
  Description  : Test t_BagCounters class
                 The datafile used is a discretized Golf database.
  Doesn't test :
  Enhancements :
  History      : Ronny Kohavi                                       9/01/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <InstList.h>
#include <CtrInstList.h>
#include <BagCounters.h>

RCSID("MLC++, $RCSfile: t_BagCounters.c,v $ $Revision: 1.18 $")
   
// define constants for stuff related to file.
#include "../../MCore/tests/t_golf.c" 

// See if the first instance matches the desired numbers.
void add_first_instance(const InstanceList& bag, BagCounters& bc)
{
   Pix pix = bag.first();
   InstanceRC instance = bag.get_instance(pix);
   bc.add_instance(instance);
   ASSERT(bc.label_count(dontPlay) == 1);
   ASSERT(bc.label_count(play) == 0);
   ASSERT(bc.val_count(dontPlay, 0, sunny) == 1);
   ASSERT(bc.val_count(dontPlay, 0, overcast) == 0);   
   ASSERT(bc.val_count(dontPlay, 0, rain) == 0);   
   ASSERT(bc.val_count(play, 0, sunny) == 0);   
   ASSERT(bc.val_count(play, 0, overcast) == 0);   
   ASSERT(bc.val_count(play, 0, rain) == 0);   
   ASSERT(bc.val_count(play, 3, windyT) == 0);   
   ASSERT(bc.val_count(play, 3, windyF) == 0);   
   ASSERT(bc.val_count(dontPlay, 3, windyT) == 0);   
   ASSERT(bc.val_count(dontPlay, 3, windyF) == 1);   

   ASSERT(bc.attr_count(0, sunny) == 1);
   ASSERT(bc.attr_count(0, overcast) == 0);
   ASSERT(bc.attr_count(0, rain) == 0);
   ASSERT(bc.attr_count(3, windyT) == 0);
   ASSERT(bc.attr_count(3, windyF) == 1);

   ASSERT(bc.attr_num_vals(0) == 1);
   ASSERT(bc.attr_num_vals(3) == 1);
   ASSERT(bc.label_num_vals() == 1);

}

void del_first_instance(const InstanceList& bag, BagCounters& bc)
{
   Pix pix = bag.first();
   InstanceRC instance = bag.get_instance(pix);
   bc.del_instance(instance);
   ASSERT(bc.label_count(dontPlay) == 0);
   ASSERT(bc.label_count(play) == 0);
   ASSERT(bc.val_count(dontPlay, 0, sunny) == 0);
   ASSERT(bc.val_count(dontPlay, 0, overcast) == 0);   
   ASSERT(bc.val_count(dontPlay, 0, rain) == 0);   
   ASSERT(bc.val_count(play, 0, sunny) == 0);   
   ASSERT(bc.val_count(play, 0, overcast) == 0);   
   ASSERT(bc.val_count(play, 0, rain) == 0);   
   ASSERT(bc.val_count(play, 3, windyT) == 0);   
   ASSERT(bc.val_count(play, 3, windyF) == 0);   
   ASSERT(bc.val_count(dontPlay, 3, windyT) == 0);   
   ASSERT(bc.val_count(dontPlay, 3, windyF) == 0);   
   ASSERT(bc.OK() == 0);  // Now we have zero.
}

void test_all_instances(const InstanceList& bag, BagCounters& bc)
{
   for (Pix pix = bag.first(); pix; bag.next(pix)) {
      InstanceRC instance = bag.get_instance(pix);
      bc.add_instance(instance);
   }
   
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

   ASSERT(bc.attr_count(0, sunny) == 5);
   ASSERT(bc.attr_count(0, overcast) == 4);
   ASSERT(bc.attr_count(0, rain) == 5);
   ASSERT(bc.attr_count(3, windyT) == 6);
   ASSERT(bc.attr_count(3, windyF) == 8);

   ASSERT(bc.attr_num_vals(0) == 3);
   ASSERT(bc.attr_num_vals(3) == 2);
   ASSERT(bc.label_num_vals() == 2);

   ASSERT(bc.OK() == 14);
   ASSERT(bag.num_instances() == 14);
}


void test1()
{
   InstanceList bag("t_BagCounters");
   BagCounters bc(bag.get_schema());
   ASSERT(bc.OK() == 0); // Do we have zero instances, and are we ready?
   
   def_consts(bag);
   BagCounters bc1(bc, ctorDummy); // should be empty
   add_first_instance(bag, bc);
   BagCounters bc2(bc, ctorDummy); // one instance
   ASSERT(bc1 != bc);
   ASSERT(bc2 == bc);  
   add_first_instance(bag, bc1);
   ASSERT(bc1 == bc);
   del_first_instance(bag,bc);
   ASSERT(bc1 != bc);
   ASSERT(bc2 != bc);
   del_first_instance(bag,bc1);
   ASSERT(bc1 == bc);

   test_all_instances(bag,bc);
   BagCounters bc3(bc, ctorDummy);
   ASSERT(bc3 == bc);    
}



main()
{
   cout << "t_BagCounters executing" << endl;

   test1();
   
   // The classical Golf database.  Shall we play, or not?
   InstanceList bag("t_BagCounters");

   BagCounters bc(bag.get_schema());
   ASSERT(bc.OK() == 0); // Do we have zero instances, and are we ready?

   def_consts(bag);
   add_first_instance(bag, bc);
   del_first_instance(bag,bc);

   test_all_instances(bag,bc);
   Mcout << bc;
   Mcout << "Testing accessor functions" << endl;
   ASSERT(bc.value_counts()[0] != NULL);
   Mcout << "label_counts:" << endl << bc.label_counts() << endl
          << "attr_counts:" << endl << *bc.attr_counts()[0] << endl;

   // Test BagCounters on dataset with only real attributes.
   CtrInstanceList ctrList("t_BagCounters-real");
   Mcout << "Real instances:  " << ctrList.counters().OK() << endl;

   return 0;                    // return success to shell
}   


