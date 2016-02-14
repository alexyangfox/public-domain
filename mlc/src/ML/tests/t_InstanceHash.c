// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.
/***************************************************************************
  Description  : Tests all the methods of InstanceBagSameAttr_,
                     InstanceHashTable_, and InstanceHashTable.
  Doesn't test :
  Enhancements :
  History      : YeoGirl Yun
                   Added a copy constructor for InstanceHashTable_ and
		     InstanceHashTable.                            9/6/94
                 YeoGirl Yun                                       9/2/94
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <InstanceHash.h>
#include <InstList.h>

RCSID("MLC++, $RCSfile: t_InstanceHash.c,v $ $Revision: 1.10 $")

const int estimatedNum = 1000;

//const Real
//  HashTable<InstanceBagSameAttr_, InstanceBagSameAttr_>::LOAD_FACTOR = 0.5;

void instance_bag_same_attr_test()
{
   Mcout << "InstanceBagSameAttr_ test executed" << endl;
   InstanceList bag("t_InstanceHashSmall");
   
   InstanceBagSameAttr_ sameBag(bag.get_instance(bag.first()));
   Pix pixFirst = sameBag.get_bag().first();
   #ifndef MEMCHECK
   TEST_ERROR("there must be at least one instance in the bag after"
              " the deletion", sameBag.remove_instance(pixFirst));
   #endif

   InstanceBagSameAttr_ sameBag2(bag.get_instance(bag.first()));
   ASSERT(sameBag == sameBag2);
   for (Pix pix = bag.first(); pix; bag.next(pix)) {
      const InstanceBagSameAttr_ temp(bag.get_instance(pix));
      if (temp != sameBag) {
	 #ifndef MEMCHECK
	 TEST_ERROR("Instance of different value is being added",
		    sameBag.add_instance(bag.get_instance(pix)));
         #endif
      }
   }
}
      



void instance_hash_table_test()
{
   Mcout << "InstanceHashTable test executed" << endl;
   InstanceList bag("t_InstanceHashSmall");

   // constructor test.
   InstanceHashTable hashTable1(estimatedNum, 2432, &bag);
   hashTable1.OK();
   InstanceHashTable hashTable2(estimatedNum);
   hashTable2.OK();

   // display tests.
   Mcout <<"num of instances : " <<  hashTable1.num_instances() << endl;
   hashTable1.display();
   
   // copy constructor test.
   InstanceHashTable hashTable3(hashTable1, ctorDummy);
   hashTable3.OK();
   
   // insert test.
   for (Pix pix = bag.first(); pix; bag.next(pix)) {
      hashTable2.insert(bag.get_instance(pix));
   }
   hashTable2.OK();
   
   ASSERT(hashTable1.num_instances() == hashTable2.num_instances());
   ASSERT(hashTable1.num_bags() == hashTable2.num_bags());
   ASSERT(hashTable1.num_instances() == hashTable3.num_instances());
   ASSERT(hashTable1.num_bags() == hashTable3.num_bags());   

   
   // num_matched_labelled_instances test.
   ASSERT(hashTable1.num_matched_labelled_instances(bag)
	  == bag.num_instances());
   ASSERT(hashTable2.num_matched_labelled_instances(bag)
	  == bag.num_instances());
   ASSERT(hashTable3.num_matched_labelled_instances(bag)
	  == bag.num_instances());   

   // find_labelled_instance test.
   for (pix = bag.first(); pix; bag.next(pix)) {
      ASSERT(hashTable1.find_labelled_instance(bag.get_instance(pix)));
      ASSERT(hashTable2.find_labelled_instance(bag.get_instance(pix)));      
      ASSERT(hashTable3.find_labelled_instance(bag.get_instance(pix)));
   }
      

   // find member tests.
   for (pix = bag.first(); pix; bag.next(pix)) {
      const InstanceRC& inst = bag.get_instance(pix); 
      const InstanceBag* temp =  hashTable1.find(inst); 
      ASSERT(temp != NULL); 
      const Pix pixToInst = temp->find_labelled(inst); 
      ASSERT(pixToInst != NULL); 
      ASSERT(inst == temp->get_instance(pixToInst));
   }

   // delete and adding of duplicate instances test.
   for (pix = bag.first(); pix; bag.next(pix)) {
      hashTable2.insert(bag.get_instance(pix));
   }
   hashTable2.OK();
   
   int i=0;
   for (pix = bag.first(); pix; bag.next(pix)) {
      hashTable2.del(bag.get_instance(pix));
      i++;
      ASSERT(hashTable2.num_bags() == hashTable1.num_bags());
      ASSERT(hashTable2.num_instances()
	     == (2*hashTable1.num_instances() - i));
   }
}   



main()
{
   Mcout << "t_InstanceHash.c executing" << endl;
   instance_hash_table_test();
   instance_bag_same_attr_test();
   Mcout << "Success!" << endl;
   return 0; // return success to shell
}

