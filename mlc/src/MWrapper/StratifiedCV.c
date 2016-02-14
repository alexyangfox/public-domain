// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : StratifiedCV divides the training set into separate sets
                   based on the label of each instance.  Cross Validation is
		   then performed, taking instances from each small list in
		   turn.  The overall process is similar to that used with
		   CrossValidator.
  Assumptions  : Assumes that the order and contents of the training
                   list returned by the inducer is the same as the
		   order and contents of the training list passed to
		   the inducer.
		 The training list may be altered by the inducer
                   during training, but it must be returned to the
		   same state.
  Comments     :
  Complexity   : estimate_accuracy() takes O(numTimes*numFolds*
                   O(train_and_test()).
		 auto_estimate() takes O(estimate_accuracy() * number
		   of iterations).  Number of iterations is bounded
		   by Log_2(numFolds).
  Enhancements : Keep the relative order of instances intact.  This
		    seems to be hard if the inducer is allowed to
		    modify its training set (e.g. when it does cross
		    validation) since pointers to the list will change.
		 When a fraction of training set is specified, keep the
		    fraction stratified.  Currently, the fraction is
		    just a straight (random) fraction of the training
		    set.
  History     :  Dan Sommerfield                                   10/15/94
                   Re-engineered to fit new AccEstimator framework.
		 Dan Sommerfield                                    6/01/94
                   Initial revision as CS229B project.
***************************************************************************/

#include <basics.h>
#include <StratifiedCV.h>
#include <CtrInstList.h>
#include <Inducer.h>
#include <stdio.h>
#include <math.h>

RCSID("MLC++, $RCSfile: StratifiedCV.c,v $ $Revision: 1.11 $")


/*****************************************************************************
  Description : Return a short string identifying this estimator.
  Comments    : 
*****************************************************************************/
MString StratifiedCV::description(void) const
{
  return MString("Stratified ") + CrossValidator::description();
}


/*****************************************************************************
  Description : This function uses an array of proportions to distribute
                  num_to_distribute instances among the array "splits".  Each
                  element in the split array indicates how many instances
		  this particular category should receive (of the number
		  given).  Extra instances are distributed probabilistically.
  Comments    : When finished, the other two arrays are updated to reflect
                  the assignments made to the splits array.  These act as
                  counters to make sure all the numbers of instances add up
		  properly.
*****************************************************************************/
void StratifiedCV::calculate_fold_split(Array<int>& splits,
					Array<int>& totalInSplit,
					int& total,
					int numToDistribute) {

   // assume that the lower/upper of all arrays are the same
   ASSERT(splits.low() == totalInSplit.low());
   ASSERT(splits.high() == totalInSplit.high());
   
   // distribute the integral portions to each split,
   // fill a parallel array with remainder information
   // sum up all the remainders (to determine how many extras we
   // have.)
   int instnum;
   Real sum = 0;
   int extras = numToDistribute;
   Array<Real> remainders(splits.low(), splits.size());
   for(instnum = splits.low(); instnum <= splits.high(); instnum++) {
      Real fval = (Real)numToDistribute * ((Real)(totalInSplit[instnum]) /
				       (Real)total);
      splits[instnum] = (int)fval;
      remainders[instnum] = fval - splits[instnum];
      sum += remainders[instnum];
      extras -= splits[instnum];
   }

   // distribute the extras probabilistically across the array
   // while we have extras left, get a random number from 0 to the
   // current sum of all probabilities in the remainder array.
   // Use this number to probabilistically choose an element from the
   // array.  This element receives an extra instance in its split
   // array.  After the extra instance is assigned, reduce the
   // probability for that bag to zero.
   while(extras > 0) {
      Real indexor = rand_num_gen().real(0.0, sum);
      for(instnum = splits.low(); indexor >= remainders[instnum]; instnum++)
	 indexor -= remainders[instnum];

      // add an extra to the instance we just found
      extras --;
      sum -= remainders[instnum];
      remainders[instnum] = 0.0;
      splits[instnum]++;

      // Make sure we actually assigned an instance
      ASSERT(instnum <= splits.high());
   }

   // finally, subtract from totalInSplit and total
   for(instnum = splits.low(); instnum <= splits.high(); instnum++) {
      totalInSplit[instnum] -= splits[instnum];
      total -= splits[instnum];
   }

   // output the contents of the split array
   LOG(6, "Split array:" << splits << endl);
   LOG(6, "Totals array:" << totalInSplit << endl);
   LOG(6, "Running total:" << total << endl << endl);
}


/*****************************************************************************
  Description : This function is responsible for constructing training and
                  test lists for a given fold.  The construction of these
		  lists relies on a preconstructed (once for each TIME, not
		  fold) array which keeps information on how to split up the
		  bags to handle the cross-validation in such a way that the
		  results are not biased and so that no two training or test
		  bags overlap.
  Comments    : The large argument list is needed to allow us to pass in the
                  precomputed array information.  The trainList and testList
		  passed in should be empty.
*****************************************************************************/
void StratifiedCV::split_fold(BagPtrArray *splitList,
			      Array<int>& totalSize,
			      int totalInstances,
			      int& totalCounter,
			      int fold, int folds,
			      InstanceList*& trainList,
			      InstanceList*& testList) {

   ASSERT(trainList->no_instances());
   ASSERT(testList->no_instances());

   // set up an array to tell us how many instances each
   // split should receive.  Use the calculate_fold_split function here
   Array<int> numInSplit(splitList->low(), splitList->size());
   int numInFold = (totalInstances / folds);
   if(totalInstances % folds > fold) numInFold++;
   calculate_fold_split(numInSplit, totalSize, totalCounter, numInFold);
   
   for (int k = splitList->low(); k <= splitList->high(); k++) {
      LOG(4, "Number in split " << fold << 
	  "/" << k << ": " << numInSplit[k] << endl);

      // split the list and copy its parts
      InstanceList *trainSegment = &((*splitList)[k]->
				     cast_to_instance_list());
      InstanceList *testSegment = trainSegment->
	 split_prefix(numInSplit[k]);
      InstanceList *trainSegmentCopy = &(trainSegment->clone()->
					 cast_to_instance_list());
      InstanceList *testSegmentCopy = &(testSegment->clone()->
					cast_to_instance_list());
      
      // unite the originals in "rotated" position and stick the
      // united list back into the array
      trainSegment->unite(testSegment);

      // unite the copies into the testList and trainList
      testList->unite(testSegmentCopy);
      trainList->unite(trainSegmentCopy);
   }
   // We don't print a newline so accuracy can come on the same line in
   // loglevel 3 and higher.
   LOG(3, "Number of instances "
	  "in fold " << fold << ": " << numInFold << ". ");
}


/*****************************************************************************
  Description : Estimates accuracy for a single run of cross validation.
                  Multiple runs are handled by the base class.
  Comments    : 
*****************************************************************************/
Real StratifiedCV::estimate_time_accuracy(BaseInducer& inducer,
			   InstanceList& dataList, int time, int folds)
{
   int totalInstances = dataList.num_instances();

   // create basic lists for cross validation
   InstanceList *shuffledList = dataList.shuffle(&rand_num_gen());
   InstanceList *trainList =
      &(dataList.create_my_type(dataList.get_schema())->
	cast_to_instance_list());
   InstanceList *testList =
      &(dataList.create_my_type(dataList.get_schema())->
	cast_to_instance_list());

   // split the training list by label
   BagPtrArray *splitList = shuffledList->split_by_label();
   LOG(3, "Number of splits: " << splitList->size() << endl);

   // create an array parallel to splitList containing the number of
   // instances in each split.  The elements in this array will decrease
   // as instances are assigned to training and test lists (this array
   // serves as a counter)
   Array<int> totalSize(splitList->low(), splitList->size());
   for(int i=splitList->low(); i <=splitList->high(); i++)
      totalSize[i] = (*splitList)[i]->num_instances();

   // also set up counter of the total number of instances left to
   // be assigned
   int totalCounter = totalInstances;

   // now, run cross validation for each split
   AccData foldData;
   for (int fold = 0; fold < folds; fold++) {
      // calculate the train and test lists using split_fold
      split_fold(splitList, totalSize, totalInstances, totalCounter, 
		 fold, folds, trainList, testList);
	
      // display the lists
      LOG(6, "Test list: " << endl << *testList << endl);
      LOG(6, "Train list: " << endl << *trainList << endl);

      // shuffle training list, then call train-and-test
      InstanceList *shuffledTrainList = trainList->shuffle(&rand_num_gen());

      // pull out an (unstratified) fraction of the training list,
      // if called for
      InstanceList *fractList = shuffledTrainList;
      if(fraction < 1.0) {
	 int numInTrain = shuffledTrainList->num_instances();
	 int numInFract = (int)(fraction * (Real)numInTrain + 0.5);
	 if(numInFract <= 0)
	    err << "StratifiedCV::estimate_time_accuracy: number of "
	       "instances in the fraction <= 0" << fatal_error;
	 fractList = shuffledTrainList->split_prefix(numInFract);
      }
      Real accuracy = train_and_test(inducer, fractList, *testList,
 	         "-" + MString(time, 0) + "-" + MString(fold,0), accData);
      LOG(3, "Acc " << accuracy << endl);
      foldData.insert(accuracy);

      // clean up
      delete shuffledTrainList;
      if(fraction < 1.0)
	 delete fractList;
      trainList->remove_all_instances();
      testList->remove_all_instances();
   }

   // when we're done with all folds, all counters should be zero
   // otherwise, we've assigned things incorrectly
   ASSERT(totalCounter == 0);
   for(int cc = totalSize.low(); cc <= totalSize.high(); cc++) {
      ASSERT(totalSize[cc] == 0);
   }
      
   delete shuffledList;
   delete trainList;
   delete testList;
   delete splitList;
   return foldData.mean();
}

