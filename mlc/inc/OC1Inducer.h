// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _OC1Inducer_h
#define _OC1Inducer_h 1

#include <BaseInducer.h>
#include <Inducer.h>
#include <MLCStream.h>
#include <LogOptions.h>

#define OC1_INDUCER 25

class OC1Inducer : public BaseInducer {
private:   
   MString pgmName;   

   unsigned int randomSeed;   
   Bool axisParallelOnly;
   Bool cartLinearCombinationMode;
   Real pruningRate;
   MString redirectString;

public:
   static MString defaultPgmName;
   static unsigned int defaultRandomSeed;
   static Bool defaultAxisParallelOnly;
   static Bool defaultCartLinearCombinationMode;
   static Real defaultPruningRate;
   static MString defaultRedirectString;
   
   OC1Inducer(const MString& description, 
	      const MString& thePgmName = defaultPgmName);

   virtual ~OC1Inducer();
   virtual int class_id() const { return OC1_INDUCER; }
   virtual Real train_and_test(InstanceBag* trainingSet,
			       const InstanceBag& testList);
   virtual void set_pgm_name(const MString& name) { pgmName = name; }
   virtual MString get_pgm_name() const { return pgmName; }
   virtual void set_seed(unsigned int seed) { randomSeed  = seed; }
   virtual unsigned int get_seed() const { return randomSeed; }
   virtual void set_axis_parallel_only_opt(Bool val)
   { axisParallelOnly = val; }
   virtual Bool get_axis_parallel_only_opt() const { return axisParallelOnly;}
   virtual void set_cart_linear_combination_mode(Bool val)
   { cartLinearCombinationMode = val; }
   virtual Bool get_cart_linear_combination_mode() const
   { return cartLinearCombinationMode; }
   virtual void set_pruning_rate(Real val);
   virtual Real get_pruning_rate() const { return pruningRate; }
   virtual void set_redirect_string(const MString& val);
   virtual MString get_redirect_string() const
   { return redirectString; }
   
   virtual void set_user_options(const MString& prefix);
};
#endif





