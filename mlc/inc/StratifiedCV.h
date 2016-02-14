// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _StratifiedCV_h
#define _StratifiedCV_h 1

#include <InstList.h>
#include <CValidator.h>

class StratifiedCV : public CrossValidator {
   
   NO_COPY_CTOR(StratifiedCV);

protected:
   virtual void calculate_fold_split(Array<int>& splits,
				Array<int>& totalInSplit,
				int& total,
				int numToDistribute);

   virtual void split_fold(BagPtrArray *splitList,
			   Array<int>& totalSize,
			   int totalInstances,
			   int& totalCounter,
			   int fold, int folds,
			   InstanceList*& trainList,
			   InstanceList*& testList);

   virtual Real estimate_time_accuracy(BaseInducer& inducer,
		       InstanceList& dataList, int time, int folds);
	 


public:

   StratifiedCV(int nFolds = CrossValidator::defaultNumFolds,
		int nTimes = CrossValidator::defaultNumTimes) :
      CrossValidator(nFolds, nTimes) { }

   virtual MString description(void) const;

};

#endif



