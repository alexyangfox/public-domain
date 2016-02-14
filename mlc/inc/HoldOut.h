// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _HoldOut_h
#define _HoldOut_h 1

#include <AccEstimator.h>

class HoldOut : public AccEstimator {
   NO_COPY_CTOR(HoldOut);
   int numTimes;          // number of times to run holdout
   int numHoldOut;        // number of instances to hold out
   Real pctHoldOut;       // percentage of instances to hold out

public:
   static const int defaultNumTimes;
   static const int defaultNumber;
   static const Real defaultPercent;
   
   HoldOut(int nTimes = defaultNumTimes, int num = defaultNumber,
	   Real pct = defaultPercent);
   virtual ~HoldOut();
   virtual MString description() const;
   virtual Real estimate_accuracy(BaseInducer& inducer,
				  InstanceList& dataList);
   virtual Real estimate_accuracy(BaseInducer& inducer,
				  const MString& fileStem);

   void set_times(int nTimes);
   int get_times() const { return numTimes; }
   void set_number(int num);
   int get_number() const { return numHoldOut; }
   void set_percent(Real pct);
   Real get_percent() const { return pctHoldOut; }

};

#endif _HoldOut_h

