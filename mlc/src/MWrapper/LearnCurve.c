// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : A learning curve is used to indicate how much
                    training information an inducer needs to learn a
		    concept.  The inducer is trained using a certain
		    number of instances chosen at random from the
		    given bag, and trained several times at that level.
		    The average accuracy predicting the rest of the instances
		    at that level is recorded. Generally the learning
		    curve is done at 10% increments of the bag size.
		    (i.e. given 10%, 20%, ..., 90% of the instances
		    to train on.)
                 The LearnCurveInfo structure stores the number of
                    instances that should be used for a given training
		    level and the number of times that the inducer
		    should be trained at that level.
		 learn_curve() calculates a LearnCurveResult for the given
		    Inducer, training instances, and LearnCurveInfo.
		 If the fileStem is not the empty string, each training bag
		    generated is dumped to a file called
		    fileStem-S-N.data, where fileStem is set by the
		    constructor or set_file_stem(), S is the size of
		    the bag, and N is the number of the test for that size.
		    N starts at 0.
  Assumptions  :
  Comments     :
  Complexity   : LearnCurve() takes O(S), where S is the size of the
                    info array.
                 learn_curve() takes O(NT), where N is the total
                    number of times that the inducer is trained, and T
		    is the training time for the inducer.
		 display() take O(N), where N is the total number of 
                    times that the inducer is trained.
		 get_average_accuracy() takes O(M), where M is the
		    number of times that the inducer was trained at the
		    given level.
  Enhancements :
  History      : Richard Long                                       1/15/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <LearnCurve.h>
#include <BaseInducer.h>
#include <InstList.h>
#include <CatTestResult.h>
#include <StatData.h>

RCSID("MLC++, $RCSfile: LearnCurve.c,v $ $Revision: 1.21 $")

const MString dataExt(".data");
const MString defaultFileStem;  // the empty string

/***************************************************************************
  Description : Checks that info is valid.
  Comments    :
***************************************************************************/
void LearnCurve::OK(int /* level */) const
{
   ASSERT(info.size() == result.size());  // The user should not be
					  // able to change this.
}


/***************************************************************************
  Description : Constructor.  Destructor.
  Comments    : If the fileName is not the empty string, the bags used to
                   train the inducer will be dumped during learn_curve().
		fileName defaults to "".
***************************************************************************/
LearnCurve::LearnCurve(const Array<LearnCurveInfo>& learnCurveInfoArray,
		       const MString& fileName)
   : info(learnCurveInfoArray, ctorDummy), 
     result(learnCurveInfoArray.low(), learnCurveInfoArray.size())
{
   fileStem = fileName;
   if (info.size() == 0)
          err << "LearnCurve::LearnCurve: Info array is of size 0"
              << fatal_error;
   for (int i = info.low(); i <= info.high(); i++) {
      if (info[i].trainSize <= 0)
	 err << "LearnCurve::LearnCurve: train size (" << info[i].trainSize
	     << ") must be a positive number" << fatal_error;
      if (info[i].numTrainings <= 0)
	 err << "LearnCurve::LearnCurve: number of trainings ("
	     << info[i].numTrainings << ") must be a positive number"
	     << fatal_error;
   };
}

LearnCurve::~LearnCurve()
{
   DBG(OK());
}


/***************************************************************************
  Description : Get/set for file stem.
  Comments    : Used to dump files from learn_curve().
***************************************************************************/
const MString& LearnCurve::get_file_stem() const
{
   return fileStem;
}

void LearnCurve::set_file_stem(const MString& fileName)
{
   fileStem = fileName;
}


/***************************************************************************
  Description : Trains the given inducer on portions of the given
                   training bag as indicated by the LearnCurveInfo.
		   Each portion is tested on the testBag, which may be the
		   same bag.
  Comments    : If the file stem is set, the generated training bags are
		   dumped to files.
***************************************************************************/
void LearnCurve::learn_curve(BaseInducer& inducer, InstanceList& trainBag)
{
   DBG(OK());
   LOG(1, "Inducer is " << inducer.description() << endl);
   if (get_file_stem() != "")
      LOG(1, "LearnCurve::learn_curve: File stem is " << get_file_stem()
	  << endl);
   int saveLogLevel = inducer.get_log_level();
   inducer.set_log_level(max(0, saveLogLevel - 2)); // estimate at loglevel-2

   InstanceBagIndex* index = trainBag.create_bag_index();
   
   for (int i = info.low(); i <= info.high(); i++) {
      // Can't have equality because we won't have instances to train on.
      if (info[i].trainSize >= trainBag.num_instances())
	     err << "LearnCurve::learn_curve: train size ("
	         << info[i].trainSize << ") is >= than the bag size ("
	         << trainBag.num_instances() << ')' << fatal_error;

      result[i] = new Array<Real>(info[i].numTrainings);
      Array<Real>& testAccuracy = *result[i];
      LOG(1, "Train Size: " << info[i].trainSize << "; training "
	     << info[i].numTrainings << " times..." << flush);
      LOG(2, endl); // we'll show accuracies, so let's start a new line
      StatData stats;
      for (int j = 0; j < info[i].numTrainings; j++) {
         InstanceList* testBag = trainBag.shuffle(&rand_num_gen());
         InstanceBag*  sampleBag = testBag->split_prefix(info[i].trainSize);
	 if (get_file_stem() != "") {
	    MLCOStream outFile(get_file_stem() + "-"
			       + MString(info[i].trainSize,0) + "-"
			       + MString(j,0) + dataExt);
	    sampleBag->display(outFile);
	 }
	 Real acc = inducer.train_and_test(sampleBag, *testBag);

	 delete sampleBag;
	 delete testBag;
	 LOG(2, "Accuracy= " << acc << endl);
	 testAccuracy[j] = acc;
	 stats.insert(acc);
      }
      LOG(1, "Mean accuracy: " << MString(stats.mean()*100,2) << "% +- " 
             << MString(stats.std_dev_of_mean()*100,2) << '%' << endl);
   }
   // Set inducer's training bag to NULL and delete the last one
   // assigned to it.
   delete index;
   inducer.set_log_level(saveLogLevel);
}


/***************************************************************************
  Description : Returns the (average)accuracy calculated in the last
                   call to learn_curve() corresponding to the given level
		   (and traingNum).
  Comments    : The level is zero based.
                learn_curve() should be called first.
***************************************************************************/
Real LearnCurve::get_accuracy(int level, int trainingNum) const
{
   return (*result[level])[trainingNum];
}


Real LearnCurve::get_average_accuracy(int level) const
{
   DBG(OK());
   Real totalAccuracy = 0;
   for (int i = 0; i < info[level].numTrainings; i++)
      totalAccuracy += get_accuracy(level, i);
   return totalAccuracy / info[level].numTrainings;
}


/***************************************************************************
  Description : Displays the accuracies for all trainings and the
                   average accuracies at every level from the last
		   call to learn_curve().
  Comments    : learn_curve() should be called first.
***************************************************************************/
void LearnCurve::display(MLCOStream& stream) const
{
   for (int i = result.low(); i <= result.high(); i++) {
      stream << "Train Size: " << info[i].trainSize << "; trained "
	     << info[i].numTrainings << " times." << endl; 
      for (int j = 0; j < result[i]->size(); j++)
	 stream << "\tAccuracy on training " << j << ": "
		<< (*result[i])[j] << endl;
      stream << "Average accuracy: " << get_average_accuracy(i)
	     << endl << endl;
   }
}

DEF_DISPLAY(LearnCurve)
