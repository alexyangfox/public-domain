// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _IBInducer_h
#define _IBInducer_h

#include <Inducer.h>
#include <BagSet.h>

class IBCategorizer;

class IBInducer : public Inducer {
public:
   enum NeighborVote { equal, inverseDistance };
   enum NnkValue { numNeighbors, numDistances };
private:   
   IBCategorizer* categorizer;
   int kVal; // number of nearest neighbors to look at
   Bool editing;
   Bool normMethod;
   Array<Real> *weights;  // stored array of weights
   int maxEpochs;
   NeighborVote vote;
   NnkValue nnkValue;
   Bool manualWeights;
public:
   IBInducer(const MString& dscr);
   IBInducer(const IBInducer& source, CtorDummy);
   
   virtual ~IBInducer();
   virtual int class_id() const {return IB_INDUCER;}
   virtual void OK(int level = 0) const;
   virtual void train();
   virtual Bool was_trained(Bool fatal_on_false = TRUE) const;
   virtual const Categorizer& get_categorizer() const;
   virtual void set_weights(const Array<Real>& weights);
   virtual void set_editing(Bool edit = FALSE);
   virtual Bool get_editing() const { return editing; }
   virtual void set_editing_max_epochs(int num = 3);
   virtual int get_editing_max_epochs() const { return maxEpochs; }
   virtual int get_k_val() const { return kVal; }
   virtual void set_k_val(int k = 1);
   virtual void set_norm_method(InstanceBag::NormalizationMethod
				= InstanceBag::interquartile);
   virtual Bool get_norm_method() const { return normMethod; }
   virtual void initialize();
   virtual Categorizer* get_free_categorizer();
   virtual void set_user_options(const MString& prefix);
   virtual void set_neighbor_vote(NeighborVote v = inverseDistance)
   { vote = v; }
   virtual void set_nnk_value(NnkValue val = numDistances) { nnkValue = val; }
   virtual NnkValue get_nnk_value() const { return nnkValue; }
   virtual NeighborVote get_neighbor_vote() const { return vote; }
   virtual void clear();
   virtual void init();
   virtual Inducer* copy() const;
   /* virtual */ Bool get_manual_weights() const {return manualWeights;}
   /* virtual */ void set_manual_weights(Bool val = FALSE)
      { manualWeights = val;}
};
#endif
