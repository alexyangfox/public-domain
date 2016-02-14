// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The AccEstDispatch class implements a small framework for
                   accuracy estimation.  The method used and all parameters
		   are set with options.  The main purpose of this class
		   is to provide a consistent option interface for
		   other classes which use accuracy estimation.
		 This class also uses GetOption to get options from the
		   environment and/or the user.  To avoid needlessly
		   reloading options, this class should be instantiated
		   only once per set of accuracy-estimation parameters
		   needed.
  Comments     : This was not made a subclass of AccEstimatior because we
                   cannot determine until runtime which AccEstimator we'll
		   be using.
  Complexity   : See AccEstimator and subclasses.
  Enhancements :
  History      : Yeogirl Yun                                        6/19/95
                   Added copy_options. 
                 Dan Sommerfield                                    11/4/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <BaseInducer.h>
#include <CatTestResult.h>
#include <InstList.h>
#include <AccEstDispatch.h>
#include <StratifiedCV.h>
#include <CVIncremental.h>
#include <Bootstrap.h>
#include <HoldOut.h>
#include <GetOption.h>
#include <math.h>

RCSID("MLC++, $RCSfile: AccEstDispatch.c,v $ $Revision: 1.30 $")

// definition of static MEnum
MEnum AccEstDispatch::accEstimationMethodEnum =
  MEnum("cv", AccEstDispatch::cv) << MEnum("strat-cv", AccEstDispatch::stratCV) <<
  MEnum("bootstrap", AccEstDispatch::bootstrap) <<
  MEnum("hold-out", AccEstDispatch::holdOut) <<
  MEnum("test-set", AccEstDispatch::testSet);

// default values for options
const AccEstDispatch::AccEstimationMethod DEFAULT_ACCURACY_ESTIMATOR =
      AccEstDispatch::cv;
const int DEFAULT_SEED = 7258789;
const int DEFAULT_CV_FOLDS = 10;
const int DEFAULT_CV_TIMES = 1;
const Real DEFAULT_CV_FRACT = 1.0;
const int DEFAULT_MAX_TIMES = 5;
const Real DEFAULT_DES_STD_DEV = 0.01;
const Real DEFAULT_ACC_TRIM = 0.0;
const int DEFAULT_BOOTSTRAP_TIMES = 10;
const Real DEFAULT_BOOTSTRAP_FRACTION = 0.632;
const int DEFAULT_HOLDOUT_TIMES = 1;
const int DEFAULT_HOLDOUT_NUMBER = 0;
const Real DEFAULT_HOLDOUT_PERCENT = Real(2)/3;

// help strings for options
const char *accEstimationMethodHelp =
   "This option selects the method of accuracy estimation to use in "
   "evaluating states.  We also allow use of the real accuracy (when "
   "available) to get an upper bound on performance of the search.";
const char *seedHelp =
   "This option specifies a specific seed for the random number generator.";   
const char *cvFoldsHelp =
   "This option specifies the number of folds to use for cross-validation."
   "  Specifying a negative number -k produces the leave-k-out algorithm."
   "  It is an error to choose zero or one.";
const char *cvTimesHelp =
   "This option specifies the number of times to run cross-validation.  "
   "Choosing zero times will cause the program to automatically select "
   "an appropriate number in order to minimize variance.";
const char *cvFractHelp =
   "This option specifies the fraction of the proposed training set to "
   "use within each fold of cross-validation.  Choosing 1.0 performs "
   "standard cross-validation.";
const char *maxTimesHelp =
   "This option specifies the maximum number of times to try to run "
   "cross-validation while attempting to achieve the desired standard "
   "deviation.";
const char *desStdDevHelp =
   "This option specifies the desired standard deviation for an automatic"
   " run of cross-validation.";
const char *accTrimHelp =
   "This option specifies the trim used for determining accuracy means.";
const char *bootstrapTimesHelp =
   "This option specifies the number of times to run bootstrap.";
const char *bootstrapFractionHelp =
   "This option specifies the weight given to the bootstrap sample in the "
   "bootstrap formula.  Typical values are 0.632 and 0.5.";
const char *holdoutTimesHelp =
   "This option specifies the number of times to repeat holdout.";
const char *holdoutNumberHelp =
   "This option specifies an exact number of instances to hold out for "
   "training or testing.  Select a positive number to hold out instances "
   "for training.  Select a negative number to hold out instances for "
   "testing.  Select zero to choose a percentage instead.";
const char *holdoutPercentHelp =
   "This option specifies a percentage of instances to hold out for "
   "training.";


/***************************************************************************
  Description : sets all options to their default values
  Comments    : 
***************************************************************************/
void AccEstDispatch::set_defaults()
{
   set_accuracy_estimator(DEFAULT_ACCURACY_ESTIMATOR);
   set_seed(DEFAULT_SEED);
   set_cv_folds(DEFAULT_CV_FOLDS);
   set_cv_times(DEFAULT_CV_TIMES);
   set_cv_fraction(DEFAULT_CV_FRACT);
   set_max_times(DEFAULT_MAX_TIMES);
   set_desired_std_dev(DEFAULT_DES_STD_DEV);
   set_acc_trim(DEFAULT_ACC_TRIM);
   set_bootstrap_times(DEFAULT_BOOTSTRAP_TIMES);
   set_bootstrap_fraction(DEFAULT_BOOTSTRAP_FRACTION);
   set_holdout_times(DEFAULT_HOLDOUT_TIMES);
   set_holdout_number(DEFAULT_HOLDOUT_NUMBER);
   set_holdout_percent(DEFAULT_HOLDOUT_PERCENT);
}



/*****************************************************************************
  Description : copy options but not accData member. 
  Comments    : It is almost like the normal copy constructor. But it doesn't
                  copy accData member.
*****************************************************************************/
void AccEstDispatch::copy_options(const AccEstDispatch& source)
{
   set_accuracy_estimator(source.get_accuracy_estimator());
   set_seed(source.get_seed());
   set_cv_folds(source.get_cv_folds());
   set_cv_times(source.get_cv_times());
   set_cv_fraction(source.get_cv_fraction());
   set_max_times(source.get_max_times());
   set_desired_std_dev(source.get_desired_std_dev());
   set_acc_trim(source.get_acc_trim());
   set_bootstrap_times(source.get_bootstrap_times());
   set_bootstrap_fraction(source.get_bootstrap_fraction());
   set_holdout_times(source.get_holdout_times());
   set_holdout_number(source.get_holdout_number());
   set_holdout_percent(source.get_holdout_percent());
}   

/***************************************************************************
  Description : sets all options from the user or shell
  Comments    : 
***************************************************************************/
void AccEstDispatch::set_user_options(const MString& prefix)
{
   set_accuracy_estimator(
      get_option_enum(
	 (prefix + "ACC_ESTIMATOR"), accEstimationMethodEnum,
          get_accuracy_estimator(), accEstimationMethodHelp,
          FALSE));
   set_seed(
      get_option_int(prefix + "ACC_EST_SEED", get_seed(),
		     seedHelp, TRUE));
   set_acc_trim(
      get_option_real(prefix + "ACC_TRIM", get_acc_trim(),
		      accTrimHelp, TRUE));

   if(accEstimationMethod == cv || accEstimationMethod == stratCV) {
      set_cv_folds(
	 get_option_int(prefix + "CV_FOLDS", get_cv_folds(),
			cvFoldsHelp, FALSE));

      set_cv_times(
	 get_option_int(prefix + "CV_TIMES", get_cv_times(),
			cvTimesHelp, TRUE));

      set_cv_fraction(
	 get_option_real_range(prefix + "CV_FRACT", get_cv_fraction(),
			       0.0, 1.0,
			       cvFractHelp, TRUE));

      if(get_cv_times() == 0) {
	 set_desired_std_dev(
	    get_option_real(prefix + "DES_STD_DEV", get_desired_std_dev(),
			    desStdDevHelp, TRUE));
         set_max_times(
	    get_option_int(prefix + "MAX_TIMES", get_max_times(),
			   maxTimesHelp, TRUE));
      }		   
   }
   else if(accEstimationMethod == bootstrap) {
      set_bootstrap_times(
	 get_option_int(prefix + "BS_TIMES", get_bootstrap_times(),
			bootstrapTimesHelp, TRUE));
      set_bootstrap_fraction(
	 get_option_real(prefix + "BS_FRACTION",
			 get_bootstrap_fraction(),
			 bootstrapFractionHelp, TRUE));
   }
   else if(accEstimationMethod == holdOut) {
      set_holdout_times(
	 get_option_int(prefix + "HO_TIMES", get_holdout_times(),
			holdoutTimesHelp, TRUE));
      set_holdout_number(
	 get_option_int(prefix + "HO_NUMBER", get_holdout_number(),
			holdoutNumberHelp, TRUE));
      if(get_holdout_number() == 0)
	 set_holdout_percent(
	    get_option_real_range(prefix + "HO_PERCENT",
				  get_holdout_percent(),
				  0, 1,
				  holdoutPercentHelp,
				  TRUE));
   }
   else if(accEstimationMethod == testSet) {
      // no more extra options for testSet
   }
   else
      ASSERT(FALSE);
      
}


/***************************************************************************
  Description : Estimate_accuracy uses the accEstimation member to choose
                  which accuracy estimator to build and apply.  The
		  appropriate options are then set within this estimator
		  before it is run on the inducer and training list.
		  The test list is passed in to determine the real
		  accuracy, which is useful when collecting statistics,
		  or as an upper bound on estimation performance.
  Comments    : 
***************************************************************************/
Real AccEstDispatch::estimate_accuracy(BaseInducer& baseInducer,
				     const InstanceList& trainList,
				     const InstanceList& testList,
				     AccData *accData)
{
   // create train and test bags as copies of passed-in bags
   InstanceList *trainBag = &(trainList.clone()->
			      cast_to_instance_list());
   InstanceList *testBag = &(testList.clone()->
			     cast_to_instance_list());

   // call the protected version
   return estimate_accuracy(baseInducer, trainBag, testBag, accData);
}
Real AccEstDispatch::estimate_accuracy(BaseInducer& baseInducer,
				     const InstanceList& trainList,
				     AccData *accData)
{
   InstanceList *trainBag = &(trainList.clone()->
			     cast_to_instance_list());
   return estimate_accuracy(baseInducer, trainBag, NULL, accData);
}

/***************************************************************************
  Description : _estimate_accuracy implementes the common features of
                  the two versions of estimate_accuracy.
  Comments    : Private method.
                The real accuracy gets computed more than once if we
		  reevaluate the same node repeatedly.  We might be able to
		  optimize this by making this class cache real accuracies??
                  Then again, this might not be important, since accuracy
		  estimation takes a long time anyway.
		Adds 1 to the random number seed after we're done so that
		  multiple accuracy estimations will be different.
***************************************************************************/
Real AccEstDispatch::estimate_accuracy(BaseInducer& baseInducer,
				     InstanceList *trainBag,
				     InstanceList *testBag,
				     AccData *providedAccData)
{
   if (!trainBag)
      err << "AccEstDispatch::_estimate_accuracy: Null training bag" 
          << fatal_error;
   if (trainBag->num_instances() == 0)
      err << "AccEstDispatch::_estimate_accuracy: no instances in training bag"
          << fatal_error;
   
   if (testBag && testBag->num_instances() == 0)
      err << "AccEstDispatch::_estimate_accuracy:no instances in test bag"
          << fatal_error;

   // this pointer keeps track of the AccEstimator to use.
   AccEstimator *accEstimator = NULL;
   
   // set up an accuracy estimator.  The one to use depends on the method
   // we selected in the options.  Set the options for each estimator
   // here, too.
   if (accEstimationMethod == stratCV) {
      StratifiedCV* crossValidator = new StratifiedCV;
      crossValidator->set_log_level(max(0,get_log_level()));
      crossValidator->init_rand_num_gen(get_seed());
      crossValidator->set_folds(min(get_cv_folds(),
					trainBag->num_instances()));
      crossValidator->set_fraction(get_cv_fraction());
      if (get_cv_times() == 0)
         crossValidator->auto_estimate_accuracy(baseInducer,
						*trainBag,
						desStdDev,
						maxTimes);
      else {
         crossValidator->set_times(get_cv_times());
	 crossValidator->estimate_accuracy(baseInducer, *trainBag);
      }
      actualTimes = crossValidator->get_times();
      accEstimator = crossValidator;
   } else if (accEstimationMethod == cv) {
      CrossValidator* crossValidator;
      if (baseInducer.can_cast_to_incr_inducer())
	 crossValidator = new CVIncremental;
       else
	 crossValidator = new CrossValidator;

      crossValidator->set_log_level(max(0,get_log_level()));
      crossValidator->set_fraction(get_cv_fraction());

      crossValidator->init_rand_num_gen(get_seed());
      crossValidator->set_folds(min(get_cv_folds(),
					trainBag->num_instances()));

      if (get_cv_times() == 0)      
         crossValidator->auto_estimate_accuracy(baseInducer,
						*trainBag,
						desStdDev,
						maxTimes);
      else {
 	 crossValidator->set_times(get_cv_times());
	 crossValidator->estimate_accuracy(baseInducer, *trainBag);
      }
      actualTimes = crossValidator->get_times();
      accEstimator = crossValidator;
   } else if (accEstimationMethod == bootstrap){
      actualTimes = get_bootstrap_times();
      Bootstrap *bootstrap = new Bootstrap(get_bootstrap_times());
      bootstrap->set_fraction(get_bootstrap_fraction());
      bootstrap->set_type(Bootstrap::fractional);
      bootstrap->set_log_level(max(0,get_log_level()));
      bootstrap->init_rand_num_gen(get_seed());
      bootstrap->estimate_accuracy(baseInducer, *trainBag);
      accEstimator = bootstrap;
   } else if (accEstimationMethod == holdOut) {
      HoldOut *holdout = new HoldOut(get_holdout_times(),
				     get_holdout_number(),
				     get_holdout_percent());
      holdout->set_log_level(max(0,get_log_level()));
      holdout->init_rand_num_gen(get_seed());
      holdout->estimate_accuracy(baseInducer, *trainBag);
      accEstimator = holdout;
   } else if (accEstimationMethod == testSet){
      actualTimes = 0; // We didn't do any estimation runs

      // it is an error to use testSet if we have no real accuracy
      if(testBag == NULL) {
	 if(providedAccData) {
	    // if an AccData was provided, but we have no test set,
	    // then just return the value we had before rather than
	    // aborting.  We do this to fix a design bug in SASearch
	    // which would cause an abort every time a node is
	    // reevaluated under testSet.
	    // Based on the control flow here, the net effect here is
	    // to do nothing.
	 }
	 else {
	    err << "AccEstDispatch::estimate_accuracy: cannot use real "
	       "accuracy when no testing set is provided" << fatal_error;
	 }
      }
 
   } else
      err << "AccEstDispatch::estimate_accuracy: invalid accuracy estimator"
	  << fatal_error;

   // clear the current AccData first.
   accData.clear();
   
   if(accEstimator) {
      // if an AccData was provided, append its results first
      if(providedAccData)
	 accData.append(*providedAccData);

      // now accumulate results into accData
      accData.append(accEstimator->get_acc_data());
   }

   // set real statistics if we have a test bag
   if(testBag)
      accData.set_real(baseInducer.train_and_test(trainBag, *testBag),
		       testBag->num_instances());

   delete accEstimator;
   delete trainBag;
   delete testBag;

   // if an AccData was provided, accumulate new results into it
   if(providedAccData) {
      providedAccData->clear();
      providedAccData->append(accData);
   }   
   return accData.accuracy(accTrim);
}


/***************************************************************************
  Description : Options-setting functions
  Comments    :
***************************************************************************/
void AccEstDispatch::
set_accuracy_estimator(AccEstimationMethod accEstimation)
{
   if (accEstimation != cv && accEstimation != stratCV &&
	   accEstimation != testSet && accEstimation != bootstrap &&
           accEstimation != holdOut)
           err << "AccEstDispatch::set_accuracy_estimator:  accuracy "
                  "estimator must be either cv, stratCV, testSet, "
                  "bootstrap, or holdOut" << fatal_error;
   accEstimationMethod = accEstimation;
}

void AccEstDispatch::set_cv_folds(int folds)
{
   if(folds == 0 || folds == 1)
      err << "AccEstDispatch::set_cv_folds: picking " << folds << " folds "
	 "is illegal" << fatal_error;
   cvFolds = folds;
}

void AccEstDispatch::set_bootstrap_times(int times)
{
   if (times < 1)
      err << "AccEstDispatchimator::set_bootstrap_times: times (" << times
	  << ") must be at least 1" << fatal_error;
   bootstrapTimes = times;
}

void AccEstDispatch::set_bootstrap_fraction(Real fract)
{
   if(fract <= 0.0 || fract > 1.0)
      err << "AccEstDispatchimator::set_bootstrap_fraction: fraction (" <<
	 fract << ") must be between 0.0 and 1.0" << fatal_error;
   bootstrapFraction = fract;
}


const AccData& AccEstDispatch::get_acc_data() const
{
   return accData;
}

/***************************************************************************
  Description : Display current settings of this dispatcher
  Comments    : 
***************************************************************************/
void AccEstDispatch::display_settings(MLCOStream& stream) const
{
   // display generic parameters first
   AccEstimationMethod acMethod = get_accuracy_estimator();
   MString acMethodString = accEstimationMethodEnum.name_from_value(acMethod);
   stream << "Method: " << acMethodString << endl;
   stream << "Trim: " << get_acc_trim() << endl;
   stream << "Seed: " << get_seed() << endl;

   // CV parameters
   if(acMethod == cv || acMethod == stratCV) {
      stream << "Folds: " << get_cv_folds();
      if(cvTimes >= 1)
	 stream << ",  Times: " << get_cv_times() << endl;
      else {
	 stream << ",  Desired std dev: " << get_desired_std_dev() <<
	           ",  Max times: " << get_max_times() << endl;
      }
   }

   // bootstrap parameters
   if(acMethod == bootstrap) {
      stream << "Times: " << get_bootstrap_times() << ",  Fraction: " <<
	 get_bootstrap_fraction() << endl;
   }

   // holdout parameters
   if(acMethod == holdOut) {
      stream << "Times: " << get_holdout_times();
      if(get_holdout_number() == 0)
	 stream << ",  Percent: " << get_holdout_percent() << endl;
      else
	 stream << ",  Number: " << get_holdout_number() << endl;
   }
}

/***************************************************************************
  Description : Convenience function for displaying accuracy
  Comments    : 
***************************************************************************/
void AccEstDispatch::display_accuracy(MLCOStream& stream) const
{
   accData.display(stream, accTrim);
}

/***************************************************************************
  Description : Display everything about the accuracy estimator
  Comments    : 
***************************************************************************/
void AccEstDispatch::display(MLCOStream& stream) const
{
   display_settings(stream);
   stream << "Accuracy: ";
   display_accuracy(stream);
   stream << endl;
}

DEF_DISPLAY(AccEstDispatch);

