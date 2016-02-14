// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The Bootstrap is an accuracy estimation method developed
		   by Bradley Efron (Efron, An Introduction to the Bootstrap,
		   1993).
		 This class implements a simplified version of bootstrap
		   (simple bootstrap), along with the usual .632 bootstrap
		   method.  Optionally, the user may choose a weighting for
		   the bootstrap sample accuracy other than .632.

  Assumptions  : Assumes that the order and contents of the training
                    list returned by the inducer is the same as the
		    order and contents of the training list passed to
		    the inducer.
		 The training list may be altered by the inducer
                    during training, but it must be returned to the
		    same state.
  Comments     : 

  Complexity   : estimate_accuracy takes O(numTimes *
                       O(Inducer::train_and_test())).
  Enhancements : Keep the relative order of instances intact.  This
		    seems to be hard if the inducer is allowed to
		    modify its training set (e.g. when it does cross
		    validation) since pointers to the list will change.
  History     :  Dan Sommerfield                                    10/4/94
                   Refitted to connect with new AccEstimator class.
		   Added estimate_accuracy using files.
		 Dan Sommerfield                                    6/01/94
                   Initial revision as CS229B project.
***************************************************************************/

#include <basics.h>
#include <CtrInstList.h>
#include <Bootstrap.h>
#include <Inducer.h>

RCSID("MLC++, $RCSfile: Bootstrap.c,v $ $Revision: 1.8 $")


const int Bootstrap::BootstrapDefaultNumTimes = 50;
const Bootstrap::BootstrapType Bootstrap::BootstrapDefaultType = fractional;
const Real Bootstrap::BootstrapDefaultFraction = 0.632;


// constants for dump suffixes used by refined bootstrap when estimating
// from dumped files.  This suffix is used specially to get apparent
// accuracy from a dumped file.
const MString BootstrapAASuffix = "-A";


/*****************************************************************************
 Description : create_bootstrap_sample builds two lists which are returned
                 through output parameters.  BootstrapSample is the bootstrap
	         sample itself, and UnusedSample is the portion of the
		 original sample that was never used in forming the bootstrap
		 sample.
	       UnusedSample is used only for the .632 and fractional bootstrap
	         methods.
	       Caller GETS ownership of the two new lists.
 Comments    : UnusedSample is unnecessarily computed for simple bootstrap.
*****************************************************************************/

void Bootstrap::create_bootstrap_sample(InstanceList& originalSample,
					InstanceList*& bootstrapSample,
					InstanceList*& unusedSample)
{
   int i = 0;

   // create an array of sample_elements;
   int numSamples = originalSample.num_instances();
   Array<SampleElement> sampleArray(numSamples);

   // initialize the array
   for(Pix p = originalSample.first(); 
       p; originalSample.next(p), i++) {
      sampleArray[i].pix = p;
      sampleArray[i].flag = FALSE;
   }

   // create the bootstrapSample by sampling randomly from the array.
   // this implements a random sample WITH replacement.
   bootstrapSample = &(originalSample.create_my_type(
      originalSample.get_schema())->cast_to_instance_list());
   for(i=0; i<numSamples; i++) {
      int randomIndex = rand_num_gen().integer(numSamples);
      bootstrapSample->
	 add_instance(originalSample.get_instance
		      (sampleArray[randomIndex].pix));
      sampleArray[randomIndex].flag = TRUE;
   }

   // run through the array finding any index with a FALSE flag.  Add these
   // to the unusedSample list
   unusedSample = &(originalSample.create_my_type(
      originalSample.get_schema())->cast_to_instance_list());
   for(i=0; i<numSamples; i++) {
      if(sampleArray[i].flag == FALSE) {
	 unusedSample->
	    add_instance(originalSample.get_instance(sampleArray[i].pix));
      }
   }

   LOG(2, "Total samples: " <<
       numSamples << "\tUnique Used Samples: " << 
       bootstrapSample->num_instances() - unusedSample->num_instances()
       << "\tUnused Samples: " <<
       unusedSample->num_instances() << endl);
   LOG(3, "bootstrap sample:" << endl << *bootstrapSample << endl);
   LOG(3, "unused sample:" << endl << *unusedSample << endl);

   // size of bootstrap sample should equal size of original sample
   ASSERT(bootstrapSample->num_instances() ==
	  originalSample.num_instances());
}


/****************************************************************************
 Description : options setting functions
 Comments    :
****************************************************************************/

void Bootstrap::set_times(int num) {
   if(num <= 0)
      err << "Bootstrap::set_times: illegal number of times: " << num
	  << fatal_error;
   numTimes = num;
}

void Bootstrap::set_type(BootstrapType type) {
   if(type == simple || type == fractional)
      bsType = type;
   else
      err << "Bootstrap::set_type: illegal type:" << type << fatal_error;
}

void Bootstrap::set_fraction(Real fract) {
   if(fract <= 0.0 || fract > 1.0)
      err << "Bootstrap::set_fraction: Illegal value: " << fract
	  << ".  Must be between 0.0 and 1.0" << fatal_error;
   bsFract = fract;
}



/****************************************************************************
  Description : train_and_test, modified to apply the bootstrap formula
                  for the appropriate type of bootstrap at each sample.
  Comments    :
****************************************************************************/
Real Bootstrap::train_and_test(BaseInducer& inducer,
			       InstanceList* trainList,
  			       const InstanceList& testList,
			       const MString& dumpSuffix,
			       AccData *localAccData)
{
   Real accuracy = AccEstimator::train_and_test(inducer, trainList, testList,
						dumpSuffix, NULL);

   // now apply the bootstrap forumla if we're doing a fractional bootstrap
   // do NOT compute if localAccData = none.  Then, we're getting apparent
   // accuracy.
   // error = error*bsFract + apparent * (1 - bsFract)
   //       = (1-acc)*bsFract + (1-apparent)*(1-bsFract)
   //       = bsFract - acc*bsFract + 1 - bsFract - apparent*(1-bsFract)
   //         1 - acc*bsFract - apparent*(1-bsFract)
   // accuracy = 1 - error = acc*bsFract + apparent*(1-bsFract)
   if(bsType == fractional && localAccData)
      accuracy = accuracy * bsFract + apparentAccuracy * (1.0 - bsFract);

   // insert the accuracy now
   if(localAccData)
      localAccData->insert(accuracy);

   // log the accuracy
   LOG(2, "Individual accuracy: " << accuracy << endl);
   return accuracy;
}


/*****************************************************************************
 Description : constructor.  Allows immediate options setting.
 Comments    : Set statistical data to NULL
*****************************************************************************/
Bootstrap::Bootstrap(int nTimes,
		     const BootstrapType type,
		     Real fract) {
   set_type(type);
   set_times(nTimes);
   set_fraction(fract);
   accData = NULL;
}


/*****************************************************************************
 Description:  This function returns a full description of the accuracy
               estimation method, including the specific type (i.e. type of
               bootstrap)
*****************************************************************************/

MString Bootstrap::description(void) const {
   MString typeName;
   switch(bsType) {
      case simple:
	 typeName = "simple";
	 break;
      case fractional:
	 typeName = MString(bsFract, 0) + " fractional";
	 break;
      default:
	 ASSERT(FALSE);    // should never get here
	 break;
   }

   return MString(numTimes, 0) + "x " + typeName + " Bootstrap";
}


/****************************************************************************
 Description : estimate_accuracy for Bootstrap does all the work.  It builds
               bootstrap sample, then calls train_accuracy to get train for
	       the appropriate type of bootstrap method.
 Comments    :
*****************************************************************************/

Real Bootstrap::estimate_accuracy(BaseInducer& inducer, InstanceList& dataList)
{
  
   LOG(2, "Inducer: " << inducer.description() << endl);
   LOG(2, "Number of times: "<< numTimes << endl);
   LOG(3, "Training list: " << dataList << endl);

   // clear out statistical data
   delete accData;
   accData = new AccData;
  
   // we will need the "apparent accuracy"--that is, the accuracy of
   // the inducer when trained AND tested on the training set.
   // Apparent accuracy tends to be very high, even 1.0 for many inducers
   // assuming there are no conflicting instances.
   InstanceList *dataPtr = &dataList;
   apparentAccuracy = train_and_test(inducer, dataPtr, dataList, 
				     BootstrapAASuffix, NULL);
 
   ASSERT(numTimes != 0);  
   for(int time = 0; time < numTimes; time++) {
    
      // create a bootstrap sample
      InstanceList *testList;
      InstanceList *trainList;
      create_bootstrap_sample(dataList, trainList, testList);
    
      // make sure test sample has elements if we're using fractional
      // bootstrap.
      // An empty test set occurs if the sampling process happens to
      // sample the entire dataset (very rare for all but very small
      // datasets)
      if(bsType == fractional && testList->no_instances()) {
	 LOG(2, "Bootstrap: empty test list at time: " << time << endl);
	 time--;   // force repeat of this run of the loop
      }    
      else
	 train_and_test(inducer, trainList, *testList,
			"-" + MString(time, 0), accData);
    
      delete testList;
      delete trainList;
   }
  
   LOG(2, "Accuracy: " << *this << endl);

   accData->insert_cost(numTimes);
   return accuracy();
}


/*****************************************************************************
  Description : Trains and tests the inducer using files.
                Uses the files fileStem.names, fileStem-T.data, and
		  fileStem-T.test, where T is an integer in the range
		  [0, numTimes-1].
		Fractional bootstrap also use the file fileStem-A.data,
		  and fileStem-A.test to compute apparent accuracy.
  Comments:     Most of the work is done before the files are dumped.
*****************************************************************************/
Real Bootstrap::estimate_accuracy(BaseInducer& inducer, 
				  const MString& fileStem) {

   delete accData;
   accData = new AccData();

   // get apparent accuracy first, if needed.
   if(bsType != simple) {
      apparentAccuracy = single_file_accuracy(inducer, fileStem +
					      BootstrapAASuffix, NULL);
   }

   // get results using estimate_file_accuracy
   estimate_file_accuracy(inducer, numTimes, fileStem, accData);

   LOG(2, "Accuracy: " << *this << endl);

   accData->insert_cost(numTimes);
   return accuracy();
}


