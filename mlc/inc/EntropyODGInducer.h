// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "Inducer.c".

#ifndef _EntropyODGInducer_h
#define _EntropyODGInducer_h 1

#include <DTCategorizer.h>
#include <CtrBag.h>
#include <CtrInducer.h>
#include <ODGInducer.h>
#include <Array.h>
#include <DynamicArray.h>
#include <SplitInfo.h>


class EntropyODGInducer : public ODGInducer {
   DynamicArray<int> *orderVector;
   BoolArray* setup_order_vec();
public: 
   EntropyODGInducer(const MString& dscr, CGraph* aCgraph = NULL) :
      ODGInducer(dscr, aCgraph) {orderVector=NULL;}
   ~EntropyODGInducer();
   Real EntropyODGInducer::average_conflict_ratio(
      DynamicArray<const CtrInstanceBag*> &bagSet);
   virtual Categorizer *best_split(Array<UsedAttributes>& usedAttributes,
				   DynamicArray<SplitInfo>& splitInfoArray,
                                   DynamicArray<const CtrInstanceBag*> &,
				   MStringArray*& catNames,
				   Real& minCondEntropy,
				   Real previousEntropy);
   const Array<int>& get_order_vector() {
      was_trained();
      ASSERT(orderVector);
      return *orderVector;
   }
   int best_depth();
   virtual void train();
};

#endif
