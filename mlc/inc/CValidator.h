// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CrossValidator_h
#define _CrossValidator_h 1

#include <AccEstimator.h>

class BaseInducer;
class LabelledInstanceList;
class CatTestResult;

class CrossValidator : public AccEstimator {
   NO_COPY_CTOR(CrossValidator);
   int numFolds;
   int numTimes;

protected:
   Real fraction;
   virtual Real estimate_time_accuracy(BaseInducer& inducer,
			     InstanceList& dataList, int time, int folds);
public:
   static int  defaultNumFolds;
   static int  defaultNumTimes;
   static int  defaultMaxFolds;      // for auto_set_folds
   static int  defaultAutoFoldTimes; // for auto_set_folds
   static Real defaultStdDevEpsilon; // for auto_set_folds
   static Real defaultAccEpsilon;    // for auto_set_folds
   static int  defaultMaxTimes;        // for auto_estimate
   static Real defaultAutoStdDev;      // for auto_estimate
   CrossValidator(int nFolds = CrossValidator::defaultNumFolds,
		  int nTimes = CrossValidator::defaultNumTimes);
   virtual ~CrossValidator();
   void set_folds(int num); // negative -> leave num out
   void set_times(int num);
   void set_fraction(Real fract);
   get_folds()  {return numFolds;}
   get_times()  {return numTimes;}
   virtual MString description() const;
   virtual Real estimate_accuracy(BaseInducer& inducer, 
				  InstanceList& dataList);
   // Estimate using dumped files.
   virtual Real estimate_accuracy(BaseInducer& inducer,
				  const MString& filestem);
   virtual Real auto_estimate_accuracy(BaseInducer& inducer,
				       InstanceList& dataList,
			     Real desiredStdDev = defaultAutoStdDev,
			     int  maxTimes   = defaultMaxTimes);
   // Auto-settings attempts to set numFolds by an ad-hoc to a reasonable
   //   number such that the variance resulting from the training-set size is
   //   not too big.
   virtual void auto_set_folds(BaseInducer& inducer, InstanceList& dataList,
			       int maxFolds = defaultMaxFolds,
			       int maxTimes = defaultMaxTimes,
			       Real AccEpsilon  = defaultAccEpsilon,
			       Real stdDevEpsilon = defaultStdDevEpsilon);
};

#endif _CrossValidator_h






