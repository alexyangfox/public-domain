// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _WeightState_h
#define _WeightState_h 1

#include <CompState.h>
#include <CtrInstList.h>
#include <IBInducer.h>

#define WEIGHT_INFO 15

class WeightInfo : public AccEstInfo {
   int numWeights;
   Array<int> *lowerBound;
   Array<int> *upperBound;
   Real int2weight(int weightNum, int num) const;
public:
   WeightInfo() : numWeights(0), lowerBound(NULL), upperBound(NULL) { }
   ~WeightInfo() { delete lowerBound; delete upperBound; }
   
   virtual int lower_bound(int);
   virtual int upper_bound(int);
   virtual void display_values(const Array<int>& values,
			       MLCOStream& out = Mcout) const;
   virtual int class_id() const { return WEIGHT_INFO; }
   virtual void compute_bounds(const SchemaRC& schema);
   virtual void check_bounds() const;
   void set_num_weights(int num) {numWeights = num;}
   int get_num_weights() {return numWeights;}
};


class WeightState : public CompState {
public:
   WeightState(Array<int> *& intArray, const AccEstInfo& gI);
   virtual ~WeightState() { }

   virtual CompState *create_state(Array<int>*& info) {
      return new WeightState(info, globalInfo);
   }

   virtual void pre_eval(AccEstInfo *);
   
   // display functions
   virtual void display_info(MLCOStream& stream = Mcout) const;
};


#endif




