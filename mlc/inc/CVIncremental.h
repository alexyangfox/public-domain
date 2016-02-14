// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CVIncremental_h
#define _CVIncremental_h 1

#include <CValidator.h>

class CVIncremental : public CrossValidator {
   NO_COPY_CTOR(CVIncremental);
protected:
   virtual Real estimate_time_accuracy(BaseInducer& inducer,
			       InstanceList&, int time, int folds);
public:
   CVIncremental(int nFolds = CrossValidator::defaultNumFolds,
		  int nTimes = CrossValidator::defaultNumTimes) : 
      CrossValidator(nFolds,nTimes) {}
   virtual ~CVIncremental() {}
};
#endif
