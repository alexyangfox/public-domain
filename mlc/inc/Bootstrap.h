// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _Bootstrap_h
#define _Bootstrap_h 1

#include <Pix.h>
#include <Array.h>
#include <Array2.h>
#include <LogOptions.h>
#include <MRandom.h>
#include <AccEstimator.h>

class BaseInducer;

typedef struct {
  Pix pix;
  Bool flag;
} SampleElement;


class Bootstrap : public AccEstimator {

  NO_COPY_CTOR(Bootstrap);

public:
  // we want to make this enum public, but we have to declare it before
  // all private/protected declarations which depend on it.
  // Hence the unusual public/private/protected ordering.
  enum BootstrapType { simple = 0, fractional = 2 };

private:
  int numTimes;
  BootstrapType bsType;
  Real bsFract;

protected:
  static const int BootstrapDefaultNumTimes;
  static const BootstrapType BootstrapDefaultType;
  static const Real BootstrapDefaultFraction;

  Real apparentAccuracy;

  virtual void create_bootstrap_sample(InstanceList& originalSample,
			       InstanceList*& bootstrapSample,
			       InstanceList*& unusedSample);

  virtual Real train_and_test(BaseInducer& inducer,
			       InstanceList* trainList,
  			       const InstanceList& testList,
			       const MString& dumpSuffix,
			       AccData *localAccData);
   

public:
  Bootstrap(int nTimes = BootstrapDefaultNumTimes,
	    const BootstrapType type = BootstrapDefaultType,
	    Real fraction = BootstrapDefaultFraction);
   
  // options
  void set_times(int num);
  void set_type(BootstrapType whichType);
  void set_fraction(Real fract);
  get_times() { return numTimes; }
  BootstrapType get_type() { return bsType; }
  Real get_fraction() { return bsFract; }

  // override description and accuracy estimation functions
  virtual MString description(void) const;  
  virtual Real estimate_accuracy(BaseInducer& inducer, InstanceList& dataList);
  virtual Real estimate_accuracy(BaseInducer& inducer, 
				 const MString& filestem);

};

#endif _Bootstrap_h






