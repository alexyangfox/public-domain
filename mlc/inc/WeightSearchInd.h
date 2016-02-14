// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _WeightSearchInducer_h
#define _WeightSearchInducer_h 1

#include <SearchInducer.h>
#include <WeightState.h>

class WeightSearchInducer : public SearchInducer {
   Bool startWithFSS;
public:
   WeightSearchInducer(const MString& description, BaseInducer* ind = NULL);
   virtual ~WeightSearchInducer();

   virtual void set_user_options(const MString& prefix);
   void display(MLCOStream& stream = Mcout) const;

   virtual AccEstInfo *create_global_info() const {
      return new WeightInfo(); }
   virtual Array<int> *create_initial_info(InstanceBag*);
   virtual AccEstState *create_initial_state(Array<int> *&initialInfo,
					     const AccEstInfo& gI) const
      { return new WeightState(initialInfo, gI); }

   virtual Categorizer *state_to_categorizer(
      const State<Array<int>, AccEstInfo>& state) const;
         
};

DECLARE_DISPLAY(WeightSearchInducer);

#endif
