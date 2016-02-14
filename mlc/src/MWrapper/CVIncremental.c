// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : An incremental cross validator utilizes the fact that some
                   inducers are incremental.  This allows it to do high-fold
		   cross validation fast, most notably, leave-one-out, or
		   m-fold CV for m instances.
  Assumptions  : The inducer must be an incremental inducer.
  Comments     : 
  Complexity   : The time should be approximately independent of m, i.e.,
                   leave-one-out should be just as fast as leave-two-out.
		 The complexity of estimate_time_accuracy is the
		   time it takes to delete the instances from the inducer and
		   add them back, plus the bag operations which have an
		   expected constant time (hash table + bag) per instance.
  Enhancements : 
  History      : Yeogirl Yun and Ronny Kohavi                       10/10/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <Pix.h>
#include <IncrInducer.h>
#include <CVIncremental.h>
#include <CatTestResult.h>
#include <InstList.h>


RCSID("MLC++, $RCSfile: CVIncremental.c,v $ $Revision: 1.9 $")


/***************************************************************************
  Description : Split a fold from the training set, and incrementally update
                  the inducer.
	        An allocated testList is returned (caller gets ownership).
  Comments    : Local function.
***************************************************************************/

static InstanceList *incremental_split_prefix(IncrInducer& inducer,
					      int numInst)
{
   InstanceList *testList = new InstanceList(
                                inducer.instance_bag().get_schema());

   for (int i = 0; i < numInst; i++) {
      Pix pix = inducer.instance_bag().first();
      ASSERT(pix != NULL);
      testList->add_instance(inducer.del_instance(pix));
      // pix is updated above, and must not be NULL except for the last
      //   instance. 
      ASSERT(pix != NULL || i == numInst - 1);
   }
   return testList;
}

/***************************************************************************
  Description : Unite test list back to traing set, incrementally updating 
                  the inducer.  We delete the testList.
  Comments    :
***************************************************************************/

static void incremental_unite(IncrInducer& inducer, InstanceList* testList)
{
   for (Pix pix = testList->first(); pix; testList->next(pix)) {
      Pix addPix = inducer.add_instance(testList->get_instance(pix));
      ASSERT(addPix != NULL);
   }

   delete testList;
}   


/******************************************************************************
  Description  : Generate folds and estimate their accuracy.
                 Note that leave-one-out is not affected by the "time"
  	  	   parameter because all times will give the same behavior.
  Comments     : 
******************************************************************************/
Real CVIncremental::estimate_time_accuracy(BaseInducer& baseInducer,
			   InstanceList& dataList, int time, int folds)
{
   if (baseInducer.can_cast_to_incr_inducer() == FALSE)
      err << "CVIncremental::estimate_time_accuracy() : Inducer "
	  << baseInducer.description() << " must be an IncrInducer."
	  << fatal_error;

   IncrInducer& inducer = baseInducer.cast_to_incr_inducer();


   InstanceList *shuffledList = dataList.shuffle(&rand_num_gen());
   InstanceBag *oldList = inducer.assign_data(shuffledList);
   inducer.train();

   int totalInstances = dataList.num_instances();
   
   AccData foldData;
   for (int fold = 0; fold < folds; fold++) {
      int numInSplit = totalInstances / folds + 
	 ((totalInstances % folds > fold)? 1:0);

      InstanceList *testList =
	    incremental_split_prefix(inducer, numInSplit);

      dump_data("-" + MString(time, 0) + "-" + MString(fold,0),
		description(),
		inducer.instance_bag().cast_to_instance_list(),
		*testList);

      {  // We must deallocate CatTestResult before we call
	 // incremental_unite, because the test set is deleted by incremental
	 //   unite, and CatTestResult has pointers to test set
	 CatTestResult results(inducer.get_categorizer(),
			    baseInducer.instance_bag(), *testList);

	 update_acc_data(results.num_correct(), results.num_incorrect(),
			 accData);
	 foldData.insert(results.accuracy());
      }

      incremental_unite(inducer, testList);

      DBGSLOW(ASSERT(totalInstances== inducer.instance_bag().num_instances()));
   }

   delete inducer.assign_data(oldList);
   return foldData.mean();
}  





