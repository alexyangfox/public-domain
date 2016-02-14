// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BaggingInd_h
#define _BaggingInd_h

#include <Inducer.h>
#include <BagSet.h>
#include <MRandom.h>
#include <BaggingCat.h>

class BaggingInd : public Inducer {
   RAND_OPTIONS;
private:   
   BaggingCat* categorizer;

   // options.
   int numReplication;
   Real proportion;
   Inducer *mainInducer;
   Bool unifWeights;
   Bool useAboveAvgWeight;
   unsigned int randomSeed;


public:
   BaggingInd(const MString& dscr);
   virtual ~BaggingInd();

   static int defaultNumReplication;
   static Real defaultProportion;
   static Inducer *defaultMainInducer;
   static Bool defaultUnifWeights;
   static Bool defaultUseAboveAvgWeight;
   static unsigned int defaultRandomSeed;

   virtual void OK(int level = 0) const;
   virtual void train();
   virtual Bool was_trained(Bool fatal_on_false = TRUE) const;
   virtual const Categorizer& get_categorizer() const;

   // options.
   virtual void set_num_replication(int num = defaultNumReplication);
   virtual int get_num_replication() const { return numReplication; }
   virtual void set_proportion(Real val = defaultProportion);
   virtual Real get_proportion() const { return proportion; }   
   virtual void set_main_inducer(Inducer* mainInd = defaultMainInducer);
   virtual const Inducer* get_main_inducer() const { return mainInducer; }
   virtual void set_unif_weights(Bool val) { unifWeights = val; }
   virtual Bool get_unif_weights() const { return unifWeights; }
   virtual void set_user_options(const MString& prefix);
   virtual void set_use_above_avg_weight(Bool val) { useAboveAvgWeight = val; }
   virtual Bool get_use_above_avg_weight() const {
      return useAboveAvgWeight; }
};
#endif
