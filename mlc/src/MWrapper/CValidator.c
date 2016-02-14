// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : CrossValidator divides the training set into
                   "numFolds" mutually exclusive test partitions of
		   approximately equal size.  The instances not found
		   in each test partition are used to train the
		   inducer.  This means that each instance appears in
		   exactly one test set.  The average accuracy and
		   variance for the accuracy measurements are recorded.
		   (See Weiss & Kulikowski. Computer Systems That
		    Learn, pp. 31-33)
		   (See also Classification and Regression Trees, pp. 75-77)
		 The CrossValidator allows repeating the above process more
   		   than once in order to increase the confidence.
		 If the number of folds, k, is negative, leave-k-out will be
  		   done.  Thus k=-1 is equivalent to leave-one-out.
		 Note that the mean accuracy is not necessarily the
		   average of the fold accuracies because we don't put weights
		   on the different folds which may have different sizes.
		 We assume independence when computing variance/std-dev,
		   which is approximately true for one cross validation, but
		   this assumption holds even less when you repeat cross
		   validation more than once.
		 The Standard deviation is computed as the standard deviation
		   of the folds.  A different method used in CART is to say
		   that it is sqrt(p(1-p)/m) where m is the number of
		   instances in the whole dataset.  This assumes that
		   every instance tested is a Bernoulli experiment, and we
		   thus get a binomial distribution with the above variance.
		   The problem with the above approach (which was implemented
		   than changed back to the current method) is that instances
		   are not really independent.  A bad fold can ruin your day.
                   We decided that it is better to be able to trim by fold,
		   and keep the information as folds instead of at the
		   instance level.
  Assumptions  : Assumes that the order and contents of the training
                    list returned by the inducer is the same as the
		    order and contents of the training list passed to
		    the inducer.
		 The training list may be altered by the inducer
                    during training, but it must be returned to the
		    same state.
		 The number of instances must be >=2.
  Comments     : The implementation of CrossValidator uses the
		    split_prefix() and unite() methods of
		    InstanceList to repeatedly remove the
		    first (100/numFolds)% of the instances for the
		    test set, and then return them at the end of
		    the list.  Thus, the order of the training
		    list is changed during cross-validation, but
		    restored at the end.
		 There is no method that returns the array of
		    CatTestResults.  The reason is that each
		    CatTestResult points to a test list, but the test
		    lists exist only during estimate_accuracy().  They
		    are either deleted in the case that they were read
		    from a file, or united with the training list in
		    the case that they were split from there.  This
		    problem may be solved in the future if reference
		    counts are used...
  Complexity   : estimate_accuracy() takes O(numTimes*numFolds*
                   O(train_and_test()).
		 auto_estimate() takes O(estimate_accuracy() * number
		   of iterations).  Number of iterations is bounded
		   by Log_2(numFolds).
  Enhancements : Improve the auto functions which are pretty much a hack.
                 Improve StatData so that it gets weights, and thus
		   we could implement the accuracy exactly using
		   the number of instances in each fold.  Std-dev would be a
		   bit more complex because the folds have different variance.
		   At this stage, it looks like this isn't critical.
                 Keep the relative order of instances intact.  This
		    seems to be hard if the inducer is allowed to
		    modify its training set (e.g. when it does cross
		    validation) since pointers to the list will change.
  History     :  Ronny Kohavi                                       9/16/94
                   Re-engineered Dan's project.
		 Dan Sommerfield                                    6/01/94
                   Made into a subclass of AccEstimator.
                 Richard Long                                       1/03/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <CValidator.h>
#include <CtrInstList.h>
#include <Inducer.h>
#include <stdio.h>
#include <math.h>

RCSID("MLC++, $RCSfile: CValidator.c,v $ $Revision: 1.61 $")


int  CrossValidator::defaultNumFolds = 10;
int  CrossValidator::defaultNumTimes = 1;
int  CrossValidator::defaultMaxFolds = 20;         // for auto_set_folds
Real CrossValidator::defaultStdDevEpsilon = 0.001; // for auto_set_folds
Real CrossValidator::defaultAccEpsilon = 0.005;    // for auto_set_folds
int  CrossValidator::defaultMaxTimes = 10;         // for auto_estimate
Real CrossValidator::defaultAutoStdDev = 0.01;     // for auto_estimate,
					           //   auto_set_folds 

/***************************************************************************
  Description : Estimate accuracy for a single time (multi fold).
  Comments    : protected function
***************************************************************************/

Real CrossValidator::estimate_time_accuracy(BaseInducer& inducer,
					    InstanceList& dataList, 
					    int time, int folds)
{
   int totalInstances = dataList.num_instances();

   InstanceList *shuffledList = dataList.shuffle(&rand_num_gen());
   AccData foldData;
   for (int fold = 0; fold < folds; fold++) {
      int numInSplit = totalInstances / folds +
	 ((totalInstances % folds > fold)? 1:0);
      LOG(3, "Number of instances "
	  "in fold " << fold << ": " << numInSplit << ". ");
      InstanceList* testList = shuffledList->split_prefix(numInSplit);
      InstanceList* fractList = shuffledList;

      if(fraction < 1.0) {
	 Real numInFract = fraction * (Real)(shuffledList->num_instances());
	 int intNumInFract = (int)(numInFract + 0.5);
	 if(intNumInFract == 0)
	    err << "CrossValidator::estimate_time_accuracy: "
	       "No instances left in cv fraction" << fatal_error;
	 fractList = shuffledList->split_prefix(intNumInFract);
      }
      
      LOG(6, "Training bag is:" << endl << *fractList << endl);
      LOG(6, "Test bag is:" << endl << *testList << endl);
      Real accuracy = train_and_test(inducer, fractList, *testList,
 	         "-" + MString(time, 0) + "-" + MString(fold,0), accData);
      LOG(3, "Acc " << accuracy << endl);
      foldData.insert(accuracy);

      if(fraction < 1.0) {
	 // clean up fractional list
	 fractList->unite(shuffledList);
	 shuffledList = fractList;
      }
      
      shuffledList->unite(testList);
      ASSERT(testList == NULL);
   }
   LOG(2, "fold " << foldData << ". ");
   delete shuffledList;

   return foldData.mean();
}   



/***************************************************************************
  Description : Contructors for cross-validation.
  Comments    : See estimate_accuracy with same arguments.
***************************************************************************/
CrossValidator::CrossValidator(int nFolds, int nTimes)
{
   set_folds(nFolds);
   set_times(nTimes);
   set_fraction(1.0);
}


/***************************************************************************
  Description : Destructor.
  Comments    : 
***************************************************************************/
CrossValidator::~CrossValidator()
{
}

/***************************************************************************
  Description : Options
  Comments    :
***************************************************************************/

void CrossValidator::set_folds(int num)
{
   if (num == 0)
      err << "CrossValidator::set_folds: num folds (" << num << ") == 0"
	  << fatal_error;

   numFolds = num;
}
   
void CrossValidator::set_times(int num)
{
   if (num <= 0)
      err << "CrossValidator::set_times: num times (" << num << ") <= 0"
	  << fatal_error;
   numTimes = num;

}

void CrossValidator::set_fraction(Real fract)
{
   if( fract <= 0 || fract > 1.0)
      err << "CrossValidator::set_fraction: " << fract << " is out "
	 "of the range (0,1]" << fatal_error;
   fraction = fract;
}



/***************************************************************************
  Description : prints identifying string for this estimator.  This
                includes number of times and folds.
  Comments    : 
***************************************************************************/
MString CrossValidator::description() const {
  return MString(numTimes,0) + "x" + 
         MString(numFolds,0) + " Cross validator";
}

/***************************************************************************
  Description : Trains and tests the inducer using "numFolds"-fold
                   cross-validation, and repeated numTimes times.
	        If numFolds is negative, it means leave-k-out, where k is 
  		  abs(numFolds).
  Comments    : Shuffles the trainList before each time cross
                   validation is performed.
		Use init_rand_num_gen(seed) to achieve 
		   reproducible results; otherwise results may vary 
		   because of variations in the shuffling of the data.
***************************************************************************/

// Helper function to help support leave-k-out
static MString compute_folds(int& numFolds, int totalInstances)
{
   if (totalInstances == 0)
	 err << "CrossValidator::estimate_accuracy: 0 instances in dataList"
	     << fatal_error;

   if (totalInstances == 1)
      err << "CrossValidator::estimate_accuracy: Cannot estimate "
	 "accuracy for 1 instance" << fatal_error; 

   if (numFolds == 0 || numFolds > totalInstances)
      err << "CrossValidator::estimate_accuracy: number of folds ("
	  << numFolds << ") is invalid for data with "
	  << totalInstances << " instances" << fatal_error;
   
   MString foldsStr;
   if (numFolds > 0) 
      foldsStr = MString(numFolds, 0);
   else { // leave-abs(numFolds)-out
      int actualFolds = int(ceil(Real(totalInstances) / abs(numFolds)) + 0.5);
      if (actualFolds < 2)
	 err << "CrossValidator::estimate_accuracy: number of folds ("
	     << numFolds << ") will leave no training instances"
	     << fatal_error;
      foldsStr = MString(actualFolds, 0) + " (leave "
             + MString(abs(numFolds),0) + " out)";
      numFolds = actualFolds;
      ASSERT(numFolds > 1);
   }
   return foldsStr;
}



Real CrossValidator::estimate_accuracy(BaseInducer& inducer, 
				       InstanceList& dataList)
{
   // copy dataList in slow debug mode to check for ordering problems
   InstanceList *dataListPtr = &dataList;
   DBGSLOW(dataListPtr = &(dataList.clone()->cast_to_instance_list()));
   
   int totalInstances = dataList.num_instances();
   int actualFolds = numFolds; // cannot be negative
   MString foldsStr = compute_folds(actualFolds, totalInstances);

   LOG(1, "Inducer: " << inducer.description() << endl);
   LOG(1, "Number of folds: " << foldsStr << ", Number of times: " 
       << numTimes << endl);
   delete accData;
   accData = new AccData;
   for (int time = 0; time < numTimes; time++) {
      estimate_time_accuracy(inducer, *dataListPtr, time, actualFolds);
      if (numTimes > 1) // Don't print this if it's only once because we get
			// the same output below
         LOG(2, "Overall: " << *this << endl);
   }

   // check the copied data list ordering in slow debug mode to
   // make sure CValidator does not mess up list ordering
   DBGSLOW(
      if(!(dataList == *dataListPtr))
        err << "CrossValidator::estimate_accuracy: ordering of dataList "
               "changed during cross-validation" << endl;
      delete dataListPtr;
   );

   if (accData->size() != numTimes * actualFolds)
      err << "CrossValidator:estimate_accuracy accuracy size "
	  << accData->size() << " does not match expected size "
	  << numTimes * actualFolds
	  << ". Probable cause: train_and_test not updating accData"
	  << fatal_error;
   
   LOG(1, "Untrimmed accuracy " << *this << endl);

   // set cost within this accData
   accData->insert_cost(actualFolds * numTimes);
   return accuracy();
}

/***************************************************************************
  Description : Automatically estimate accuracy to the given std-dev level.
  Comments    :
***************************************************************************/

Real CrossValidator::auto_estimate_accuracy(BaseInducer& inducer,
					    InstanceList& dataList,
					    Real desiredStdDev,
					    int  maxTimes)
{
   if (numFolds == -1)
      err << "CrossValidator::auto_estimate_accuracy: it does not make "
	     " sense to do leave-one-out multiple times" << fatal_error;

   int totalInstances = dataList.num_instances();
   int actualFolds = numFolds; // cannot be negative
   MString foldsStr = compute_folds(actualFolds, totalInstances);

   LOG(1, "Inducer: " << inducer.description() << endl);
   LOG(1, "Number of folds: " << actualFolds << ". Looping until "
          "std-dev of mean =" << desiredStdDev << endl);
   delete accData;
   accData = new AccData;
   int time = 0;
   do {
      set_times(time + 1); // so asserts will work.
      estimate_time_accuracy(inducer, dataList, time, actualFolds);
      // The NULL in the statement below belongs to the else in the LOG()
      //   macro.   The compiler crashes without it!  If you take it out and
      //   it works, leave it out.
#     if defined(CFRONT)
      LOG(2, "Overall acc: " << *this << endl) NULL;
#     else
      LOG(2, "Overall acc: " << *this << endl);
#     endif
   } while (++time < maxTimes && accuracy_std_dev() > desiredStdDev);

   set_times(time); // so caller can do do get_times();

   if (accData->size() != actualFolds * numTimes)
      err << "CrossValidator:auto_estimate_accuracy accuracy size "
	  << accData->size() << " does not match expected size "
	  << actualFolds * time
	  << ". Probable cause: train_and_test not updating accData"
	  << fatal_error;
   
   LOG(1, "Untrimmed accuracy " << *this << endl);

   accData->insert_cost(actualFolds * get_times());
   
   return accuracy();
}




/***************************************************************************
  Description : Trains and tests the inducer using files.
		Uses the files fileStem.names, fileStem-T-F.data, and
		   fileStem-T-F.test, where T is an integer in the range
		   [0, numTimes-1] and F is an integer in the range
		   [0, numFolds-1].
  Comments    : Unimplemented in this version
***************************************************************************/

Real CrossValidator::estimate_accuracy(BaseInducer& inducer,
				       const MString& fileStem)
{
   delete accData;
   accData = new AccData;

   for (int time = 0; time < numTimes; time++) {
      Real acc = estimate_file_accuracy(inducer, numFolds,
			fileStem + "-" + MString(time,0), accData);
      LOG(2, "fold accuracy for set " << time << " is " << acc << endl);
   }

   LOG(1, "accuracy: " << *this << endl);

   return accuracy();
}


/***************************************************************************
  Description : Attempt to find good values for the numFolds that will
                  be suitable for the current training set size.
                This is an expensive operation that is useful if you
		  intend to do many cross validations on variants
		  of this dataset.
                It decreases the number of folds until the accuracy or std-dev
   		  deteriorates.
  Comments    :
***************************************************************************/

void CrossValidator::auto_set_folds(BaseInducer& inducer,
				    InstanceList& dataList,
				    int maxFolds,
				    int maxTimes,
				    Real accEpsilon,
				    Real stdDevEpsilon)

{
   int saveLogLevel = get_log_level();
   int saveNumTimes = get_times();
   set_folds(maxFolds); 
   Real prevStdDev = 1;       // high number for StdDev
   Real prevAcc = 0;          // low number.
   int  prevFolds = maxFolds; // in case we stop on first iteration.

   if (maxFolds < 2)
      err << "CrossValidator::auto_set_folds: maxFolds: " << maxFolds
	  << " less than 2" << fatal_error;
   
   if (stdDevEpsilon < 0)
      err << "CrossValidator::auto_set_folds: stdDevEpsilon: "
	  << stdDevEpsilon << " negative" << fatal_error;

   if (accEpsilon < 0)
      err << "CrossValidator::auto_set_folds: accEpsilon: "
	  << accEpsilon << " negative" << fatal_error;

   if (maxFolds > dataList.num_instances()) {
      maxFolds = dataList.num_instances();
      LOG(1, "Max folds set to number of instances: " << maxFolds << endl);
   }

   do {
      LOG(2, "auto_set_folds trying " << get_folds() << " folds..." << flush);
      set_log_level(max(0, saveLogLevel - 2)); // estimate at loglevel-2
      auto_estimate_accuracy(inducer, dataList, defaultAutoStdDev, maxTimes);
      set_log_level(saveLogLevel);

      // The real std-dev is higher since we're suppose to find
      //   a good value for 1xfolds.
      Real stdDev = accuracy_std_dev() * sqrt(get_times());
      Real acc = accuracy();

      LOG(2, "mean " << Mround(accuracy(), 4) * 100 << '%' 
          << " +- " << Mround(stdDev, 4) * 100 << '%' 
	  << " (average of " << get_times() << ")" <<  endl);

      // If one of the following is true, we stop and return previous number
      // of folds:
      //   1. Accuracy deteriorates by accEpsilon.
      //   2. Std-dev goes up by stdDevEpsilon.

      if (acc <= prevAcc - accEpsilon ||
	  stdDev >= prevStdDev + stdDevEpsilon) {
	 set_times(saveNumTimes);
         ASSERT(prevFolds >= 2);
	 set_folds(prevFolds);
	 LOG(1, "Fold setting set to " << get_folds()
	     << " (significant deterioration or above std-dev threshold)"
	     << endl);
	 return;
      }

      prevStdDev = stdDev;
      prevAcc = acc;
      prevFolds = get_folds();
      int newFolds = get_folds() / 2;
      if (newFolds < 2)
	 newFolds = 2;
      set_folds(newFolds);
   } while (prevFolds > 2);

   set_times(saveNumTimes);
   LOG(1, "Fold setting set to " <<  get_folds()
           << " (minimum possible)" << endl);
   return;
   }



