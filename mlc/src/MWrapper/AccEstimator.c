// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : AccEstimator is an abstract base class for
                 Accuracy Estimation methods.  Subclasses include
                 CrossValidator (and StratifiedCV), as well as Bootstrap
  Assumptions  : See derived classes.
  Comments     : 
  Complexity   : See derived classes.
  Enhancements : Update AccData / update_acc_data to take into account the
                   different sizes of folds by using weights in AccData.
                 dumpFiles should be made an option (very easy given the
                   current layout).
  History      : Dan Sommerfield                                   10/16/94
                   Minor function revisions.
                 Ronny Kohavi                                       9/12/94
                   Re-engineered Dan's project.
		 Dan Sommerfield                                       4/94
		   Initial revision (.h,.c) (CS229B class project)
                   Based on Rich Long's CValidator.
***************************************************************************/

#include <basics.h>
#include <AccEstimator.h>
#include <CtrInstList.h>
#include <BaseInducer.h>
#include <CatTestResult.h>
#include <NullInducer.h>
#include <math.h>

RCSID("MLC++, $RCSfile: AccEstimator.c,v $ $Revision: 1.29 $")


/***************************************************************************
  Description : Dump data to files if dumpStem != ""
                dumpSuffix is appended to dumpStem before .{names,data,test}
    		   are added.  descr goes into the files as a comment.
  Comments    : Protected method.
                Should this be a more general routine?
***************************************************************************/
void AccEstimator::dump_data(const MString& dumpSuffix, const MString& descr,
			     const InstanceList& trainList,
			     const InstanceList& testList)
{
   if (dumpStem != "") {
      MLCOStream namesStream(dumpStem + dumpSuffix + defaultNamesExt);
      trainList.display_names(namesStream, TRUE, descr);

      MLCOStream trainStream(dumpStem + dumpSuffix + defaultDataExt);
      trainStream << "|" << descr << endl << trainList << endl;

      MLCOStream testStream(dumpStem + dumpSuffix + defaultTestExt);
      testStream << "|" << descr << endl << testList << endl;
   }
}   


/***************************************************************************
  Description : update accuracy data.
  Comments    : Somewhat redundant now that we do not store the actual
                  true/false because we can't use trimming.
		  
***************************************************************************/

void AccEstimator::update_acc_data(int numCorrect, int numIncorrect,
				   AccData *localAccData)
{
   if (numCorrect < 0 || numIncorrect < 0)
      err << "AccEstimator::update_acc_data: Invalid input.  numCorrect = "
	  << numCorrect << " numIncorrect=" << numIncorrect
	  << fatal_error;

   ASSERT(localAccData != NULL);
   localAccData->insert(Real(numCorrect)/(numCorrect + numIncorrect));
}


/***************************************************************************
  Description : Shows how stratified a hold out is
  Comments    : 
***************************************************************************/
void AccEstimator::show_stratification(const InstanceList& trainPart,
				       const InstanceList& testPart) const
{
   LOG(2, "Stratification: Training: ");
   BagPtrArray *trainBPA = trainPart.split_by_label();
   for(int i=trainBPA->low(); i<=trainBPA->high(); i++)
      LOG(2, (*trainBPA)[i]->num_instances() << "/");
   delete trainBPA;

   LOG(2, "  Test: ");
   BagPtrArray *testBPA = testPart.split_by_label();
   for(i=testBPA->low(); i<=testBPA->high(); i++)
      LOG(2, (*testBPA)[i]->num_instances() << "/");
   delete testBPA;

   LOG(2, endl);
}
    
   


/***************************************************************************
  Description : Trains and tests the given inducer using the given test
                   set, and the trainList member.
		Fills in the AccData array.
  Comments    : Protected method.
                The complexity of is that inducer.train() + CatTestResult().
                We could almost get away with a DumpInducer which simply dumps
  		  the data it gets, and then appends to a test file the
		  instances it gets to categorize.  The problem is that when
		  categorizing, the instances are unlabelled.
		If localAccData is NULL, we don't update any statistics,
		but we still return the accuracy.
***************************************************************************/
Real AccEstimator::train_and_test(BaseInducer& inducer,
				  InstanceList* trainList, 
 			          const InstanceList& testList,
				  const MString& dumpSuffix,
				  AccData *localAccData)
{
   // show stratification at loglevel 3
   IFLOG(3, show_stratification(*trainList, testList));
   
   // dump before assign_data, because we'll lose ownership.
   dump_data(dumpSuffix, description(), *trainList, testList);

   Real acc = inducer.train_and_test(trainList, testList);
   
   if(localAccData)
      localAccData->insert(acc);

   int numInstances = testList.num_instances();
   int numCorrect = int(acc * numInstances + 0.5);

   static Bool warnedAlready = FALSE;
   // Nice test, but doesn't work for AccEstimators.
   if (!warnedAlready && 
       fabs(Real(numCorrect) / numInstances - acc) > 0.001) {
       Mcerr << "AccEstimator::train_and_test: suspicious accuracy "
	  << acc << " does not divide well number of instances "
	  << numInstances << " (" << Real(numCorrect) / numInstances
	  << " vs. " << acc << ".  This is OK AccEstimator inducer."
	  << " Further warnings of this type suppressed." << endl;
       warnedAlready = TRUE;
   }

   return acc;
}


/***************************************************************************
  Description : Basic constructor
  Comments    :
***************************************************************************/
AccEstimator::AccEstimator()
   : accData(NULL)
{}


/***************************************************************************
  Description : Destructor.
  Comments    : Only needs to get rid of accuracy data.
***************************************************************************/
AccEstimator::~AccEstimator()
{
  delete accData;
}

/***************************************************************************
  Description : Trains and tests the inducer from a series of data/test
                  files.
		Uses the files fileStem-#.{names,data}
                Does NOT shuffle the files that it reads.
		It deletes the accuracy data by default (not deleting it may
   		  be useful for repeating this process more than once).
                The accuracy result returned is always untrimmed.
  Comments    : Restores the CrossValidator's original training list upon
		   completion. 
***************************************************************************/

Real AccEstimator::estimate_file_accuracy(BaseInducer& inducer, int numFiles,
					  const MString& fStem,
					  AccData *thisAccData)
{
   LOG(1, "AccEstimator::estimate_accuracy: Inducer: " << inducer.description()
       << endl);
   LOG(2, "AccEstimator::estimate_accuracy: Number of files: " 
       << numFiles << endl);

   if (dumpStem != "")
      err << "AccEstimator::estimate_file_accuracy: it does not "
	     "make sense to estimate from a file and dump too" << fatal_error;

   AccData localAccData;
   for (int fileNum = 0; fileNum < numFiles; fileNum++) {
      MString dataFile(fStem + "-" + MString(fileNum, 0));
      Real accuracy = single_file_accuracy(inducer, dataFile, thisAccData);
      localAccData.insert(accuracy);
   }
   ASSERT(localAccData.size() == numFiles);
   LOG(2, "Estimate_Accuracy::estimate_accuracy: Accuracy of set: " << 
       localAccData << endl);

   return localAccData.mean();
}


/***************************************************************************
  Description : Trains and tests the inducer from a single dumped file.
  Comments    : Restores the estimator's original training list upon
		   completion. 
***************************************************************************/

Real AccEstimator::single_file_accuracy(BaseInducer& inducer,
				     const MString& dataFile,
				     AccData *thisAccData)
{
   if (dumpStem != "")
      err << "AccEstimator::single_file_accuracy: it does not "
	 "make sense to estimate from a file and dump too" << fatal_error;

   LOG(3, "AccEstimator::single_file_accuracy: Reading data file: "
       << dataFile << endl);

   // use CtrInstanceList just in case it is a CtrInducer.
   // Not local because train_and_test gets ownership and returns it.
   InstanceList* trainList = new CtrInstanceList(dataFile);
   LOG(3, "AccEstimator::single_file_accuracy: "
       "Number of instances in file " << dataFile << ": "
       << trainList->num_instances() << endl);
   MString testFile(dataFile + defaultTestExt);
   LOG(3, "AccEstimator::single_file_accuracy: Reading test file: "
       << testFile << endl);
   // Test list doesn't need a counter because it is not passed to
   // the inducer.
   InstanceList testList("", dataFile + defaultNamesExt, testFile);
   LOG(3, "AccEstimator::single_file_accuracy:Instances in test list: "
       << testList.num_instances() << endl);
   Real accuracy = train_and_test(inducer, trainList, testList, "dummy",
				  thisAccData);
   LOG(3, "AccEstimator::single_file_accuracy: Accuracy: "
       << accuracy << endl);
   delete trainList; 
 
   LOG(3, "Estimate_Accuracy::estimate_accuracy: Accuracy of file: " << 
       accuracy << endl);

   return accuracy;
}


/***************************************************************************
  Description : Return accData or aborts if it is NULL.
  Comments    : Mostly useful for the testers
***************************************************************************/

const AccData& AccEstimator::get_acc_data() const
{
   check_acc_data();
   return *accData;
}


/***************************************************************************
  Description : Nicely display the accuracy data, or print "no accuracy
                data" if none.
  Comments    :
***************************************************************************/
void AccEstimator::display_acc_data(MLCOStream& out, Real trim,
				    int precision) const
{
   if (accData == NULL)
      out << "no accuracy data";
   else
      accData->display(out, trim, precision);
}


/***************************************************************************
  Description : Verify that accuracy data is available.  Aborts with an error
                message if not.
  Comments    :
***************************************************************************/
void AccEstimator::check_acc_data() const
{
   if (accData == NULL)
      err << "AccEstimator::check_acc_data: Must be called "
	 "after estimate_accuracy.  No accuracy data" << fatal_error;
   
}


/***************************************************************************
  Description : Returns the trimmed mean/standard deviation for the
                   accuracy of the induced Categorizers for the
		   training and test lists generated through cross-validation.
		The standard deviation is the std-dev of the MEAN accuracy.
  Comments    : trim defaults to 0. 
                
***************************************************************************/
Real AccEstimator::accuracy(Real trim) const
{
   check_acc_data();
   return accData->mean(trim);
}



Real AccEstimator::accuracy_std_dev(Real trim) const
{
   check_acc_data();
   return accData->std_dev_of_mean(trim);
}



/***************************************************************************
  Description : File dump: writes out all training lists used in estimation
  Comments    : 
***************************************************************************/

void AccEstimator::dump_files(InstanceList& dataList, const MString& fStem)
{
   if (fStem == "")
      err << "AccEstimator::dump_files: empty filestem" << fatal_error;
  dumpStem = fStem;

  NullInducer inducer(fStem + " Null Inducer", FALSE);
  LOG(1, "AccEstimator::dump_files: estimated accuracies will be 0.0" << endl);
  estimate_accuracy(inducer, dataList);
  dumpStem = "";  // Don't dump in the future.
}


/***************************************************************************
  Description : Display information about this estimator.
                The description is printed only if descrip is TRUE.
                accuracy data info is printed
  Comments    : 
***************************************************************************/

void AccEstimator::display(MLCOStream& out, Bool descrip,
			   Real trim, int precision) const {
   if (descrip)
      out << description() << ": ";
   display_acc_data(out, trim, precision);
}

DEF_DISPLAY(AccEstimator);
