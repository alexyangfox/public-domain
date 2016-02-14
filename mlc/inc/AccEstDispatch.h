// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _AccEstDispatch_h
#define _AccEstDispatch_h 1

#include <AccData.h>
#include <CtrInstList.h>
#include <BaseInducer.h>
#include <MEnum.h>
#include <LogOptions.h>
#include <AccEstimator.h>

class AccEstDispatch {
   LOG_OPTIONS;
   NO_COPY_CTOR(AccEstDispatch);
   
   // AccEstimator level options
   Real accTrim, stdDevTrim;
   unsigned int randSeed;

   // CV options
   int cvFolds, cvTimes, maxTimes;
   Real cvFraction;
   Real desStdDev;

   // Bootstrap options
   int bootstrapTimes;
   Real bootstrapFraction;

   // HoldOut options
   int hoTimes;
   int hoNumber;
   Real hoPercent;

   // results
   AccData accData;
   int actualTimes;
   
public:
   // note: AccEstimationMethod is public but must be declared BEFORE
   // accEstimationMethod, which is private.  Hence, the violation of
   // ordering rules here.
   enum  AccEstimationMethod {cv, stratCV, testSet, bootstrap, holdOut};
   static MEnum accEstimationMethodEnum;

private:
   AccEstimationMethod accEstimationMethod;
   Real estimate_accuracy(BaseInducer& inducer,
			  InstanceList *trainBag,
			  InstanceList *testBag,
			  AccData *accData);
   
public:   
   AccEstDispatch() { set_defaults(); }

   void set_defaults();
   void set_user_options(const MString& prefix = emptyString);

   // almost like the copy constructor, but doesn't copy accData.  
   void copy_options(const AccEstDispatch& source);

   Real estimate_accuracy(BaseInducer& inducer,
			  const InstanceList& trainList,
			  const InstanceList& testList,
			  AccData *accData = NULL);
   Real estimate_accuracy(BaseInducer& inducer,
			  const InstanceList& trainList,
			  AccData *accData = NULL);
      
   // generic parameters
   void set_accuracy_estimator(AccEstimationMethod accEst);
   AccEstimationMethod get_accuracy_estimator() const
   { return accEstimationMethod; }
   void set_acc_trim(Real trim) { accTrim = trim; }
   Real get_acc_trim() const { return accTrim; }
   void set_seed(const unsigned int seed) { randSeed = seed; }
   unsigned int get_seed() const { return randSeed; }

   // Parameters for cross validation.
   void set_cv_folds(int folds);
   int  get_cv_folds() const { return cvFolds; }
   void set_cv_times(int times) { cvTimes = times; }
   int  get_cv_times() const { return cvTimes; }
   void set_cv_fraction(Real fract) { cvFraction = fract; }
   Real get_cv_fraction() const { return cvFraction; }
   void set_max_times(int times) { maxTimes = times; }
   int  get_max_times() const { return maxTimes; }
   void set_desired_std_dev(Real dstd) { desStdDev = dstd; }
   Real get_desired_std_dev() const { return desStdDev; }

   // Parameters for bootstrap.
   void set_bootstrap_times(int times);
   int  get_bootstrap_times() const { return bootstrapTimes; }
   void set_bootstrap_fraction(Real fract);
   Real get_bootstrap_fraction() const { return bootstrapFraction; }

   // Parameters for holdout
   void set_holdout_times(int times) { hoTimes = times; }
   int get_holdout_times() const { return hoTimes; }
   void set_holdout_number(int num) { hoNumber = num; }
   int get_holdout_number() const { return hoNumber; }
   void set_holdout_percent(Real pct) { hoPercent = pct; }
   Real get_holdout_percent() const { return hoPercent; }

   // get the accuracy
   virtual const AccData& get_acc_data() const;

   // get number of times auto-cv was actually run
   virtual int get_actual_times() const { return actualTimes; }
   
   // display:  we can display settings, accuracy information, or both
   void display_settings(MLCOStream& stream) const;
   void display_accuracy(MLCOStream& stream) const;
   void display(MLCOStream& stream) const;
};

DECLARE_DISPLAY(AccEstDispatch);


#endif
