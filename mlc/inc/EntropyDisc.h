// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _EntropyDiscretizor_h
#define _EntropyDiscretizor_h 1

#include <RealDiscretizor.h>
#include <CtrInstList.h>

class BagAndBestSplitEntropy {
   CtrInstanceBag* bag;                 
   Real bestSplit;                      
   Real bestGain;
   NO_COPY_CTOR(BagAndBestSplitEntropy);
   Bool operator=(const BagAndBestSplitEntropy&)
   { err << "EntropyDisc.h: no support for operator="<< fatal_error; return 0;}

public:
   BagAndBestSplitEntropy(const LogOptions& logOptions,
			  CtrInstanceBag*& bag, int attrNum, int minSplit); 
   BagAndBestSplitEntropy(const BagAndBestSplitEntropy& babse, CtorDummy);
   ~BagAndBestSplitEntropy() { delete bag; }
   // Gets ownership of bag.
   void assign_bag(const LogOptions& logOptions, CtrInstanceBag*& bag,
		   int attrNum, int minSplit);
   CtrInstanceBag& get_bag() const;
   Real best_split() const { return bestSplit; }
   Real best_gain() const { return bestGain; }
   operator >(const BagAndBestSplitEntropy& b) const
   { return bestGain > b.bestGain; }
   operator ==(const BagAndBestSplitEntropy& b) const
   { return bestGain == b.bestGain; }
};


class EntropyDiscretizor : public RealDiscretizor{
   int minSplit;
   int numIntervals;
   void split_bag(int minIndex,
		  DynamicArray<BagAndBestSplitEntropy*>& babse);
   Bool mdl_condition(DynamicArray<BagAndBestSplitEntropy*>& babse,
		      int minIndex);
   NO_COPY_CTOR(EntropyDiscretizor);
   void build_thresholds(const InstanceBag& sourceBag); //PPV
public:
   static int defaultMinSplit;
   EntropyDiscretizor(int attrNum, const SchemaRC& schema,
		      int numIntervals, int minSplit = 0);
   EntropyDiscretizor(const EntropyDiscretizor& disc, CtorDummy);
   Bool operator == (const RealDiscretizor& disc);
   Bool operator == (const EntropyDiscretizor& disc);
   int class_id() const { return CLASSID_ENTROPYDISC;}
   RealDiscretizor* copy(); //returns copy of self
};


PtrArray<RealDiscretizor*>*
create_Entropy_discretizors(LogOptions& logOptions,
			    const InstanceBag& bag,
			    int minSplit,
			    const Array<int>& numInterVals);

PtrArray<RealDiscretizor*>*
create_Entropy_discretizors(LogOptions& logOptions,
			    const InstanceBag& bag,
			    int minSplit, int numInterVals=10);
#endif
