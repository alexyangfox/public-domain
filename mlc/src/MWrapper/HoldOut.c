// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : HoldOut is a very simple accuracy estimator.  It functions
                    by selecting a sample without replacement from the
		    list.  This sample is used as a test list, and the
		    inducer is trained on the remaining instances.
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
  History     :  Dan Sommerfield                                   12/13/94
                   Initial Revision (.c,.h)
		   Added estimate_accuracy using files.
***************************************************************************/

#include <basics.h>
#include <CtrInstList.h>
#include <HoldOut.h>
#include <Inducer.h>

RCSID("MLC++, $RCSfile: HoldOut.c,v $ $Revision: 1.12 $")

const int HoldOut::defaultNumTimes = 1;
const int HoldOut::defaultNumber = 0;
const Real HoldOut::defaultPercent = 0.67;

/***************************************************************************
  Description : Constructor, Destructor
  Comments    : 
***************************************************************************/
HoldOut::HoldOut(int nTimes, int number, Real pct)
{
   set_times(nTimes);
   set_number(number);
   set_percent(pct);
}

HoldOut::~HoldOut()
{
}

/***************************************************************************
  Description : Returns a descriptive string for this accuracy estimator
  Comments    : 
***************************************************************************/
MString HoldOut::description() const
{
   MString holdoutString;
   if(get_number() == 0)
      holdoutString = "hold out " + MString(get_percent()*100, 0) + "%";
   else
      holdoutString = "hold out " + MString(get_number(), 0);
   return MString(get_times(), 0) + " x " + holdoutString;
}

  
   
   

/***************************************************************************
  Description : Trains and tests the inducer.
                The Hold Out method involves holding a certain number of
                  instances aside for testing.  The inducer is trained
		  on the remaining instances.  The method is repeated a
		  number of times and the results are averaged.
  Comments    :
***************************************************************************/
Real HoldOut::estimate_accuracy(BaseInducer& inducer, InstanceList& dataList)
{
   LOG(1, "Inducer: " << inducer.description() << endl);
   LOG(1, "Description: " << description() << endl);
   LOG(6, "Training list: " << dataList << endl);

   // clear out statistical data
   delete accData;
   accData = new AccData;

   // compute number of instances to hold out
   int num = get_number();
   int size = dataList.num_instances();
   if(num >= size)
      err << "HoldOut::estimate_accuracy: number to hold out for training "
	 << "(" << num << ") must not exceed number of instances (" <<
	 size << ")" << fatal_error;
   else if(num <= -size)
      err << "HoldOut::estimate_accuracy: number to hold out for testing "
	 << "(" << -num << ") must not exceed number of instances (" <<
	 size << ")" << fatal_error;
   
   if(num < 0)
      num = size + num;
   else if(num == 0)
      num = (int)(Mround(get_percent() * size, 0));
   ASSERT(num > 0 && num < size);

   // run accuracy estimation
   ASSERT(numTimes != 0);
   for(int time = 0; time < numTimes; time++) {
      // shuffle the list
      InstanceList *shuffledList = dataList.shuffle(&rand_num_gen());

      // pull out the right number of instances from front of the list
      InstanceList *sample = shuffledList->split_prefix(num);

      Real acc = train_and_test(inducer, sample, *shuffledList,
		     "-" + MString(time, 0), accData);
      LOG(2, "Accuracy for time " << time << ": " << acc << endl);

      // clean up
      delete shuffledList;
      delete sample;
   }

   LOG(1, "Accuracy: " << *this << endl);
   accData->insert_cost(numTimes);
   return accuracy();
}

/***************************************************************************
  Description : Trains and tests the inducer using files.
		Uses the files fileStem.names,
		           and fileStem-#,
		where # refers to the #th run of HoldOut.
  Comments    : 
***************************************************************************/
Real HoldOut::estimate_accuracy(BaseInducer& inducer,
				const MString& fileStem)
{
   delete accData;
   accData = new AccData;

   for (int time = 0; time < numTimes; time++) {
      Real acc = single_file_accuracy(inducer,
			fileStem + "-" + MString(time,0), accData);
      LOG(2, "accuracy of time " << time << " is " << acc << endl);
   }

   LOG(1, "Accuracy: " << *this << endl);
   accData->insert_cost(numTimes);
   return accuracy();
}

/***************************************************************************
  Description : Sets the number of times to run hold out
  Comments    : 
***************************************************************************/
void HoldOut::set_times(int nTimes)
{
   if(nTimes <= 0)
      err << "HoldOut::set_times: number of times (" << nTimes << ") "
	 "must be > 0" << fatal_error;
   numTimes = nTimes;
}

/***************************************************************************
  Description : Sets the number of instances to hold out
  Comments    : 
***************************************************************************/
void HoldOut::set_number(int num)
{
   numHoldOut = num;
}

/***************************************************************************
  Description : Sets the percentage to hold out
  Comments    : 
***************************************************************************/
void HoldOut::set_percent(Real pct)
{
   if(pct <= 0 || pct >= 1)
      err << "HoldOut::set_percent: percent to hold out (" << pct << ") "
	 "must by between zero and one" << fatal_error;
   pctHoldOut = pct;
}
