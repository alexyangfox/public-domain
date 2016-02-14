// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BinningRealDiscretizor_h
#define _BinningRealDiscretizor_h 1

#include <RealDiscretizor.h>

class BinningRealDiscretizor : public RealDiscretizor{
   int  numBins;
   NO_COPY_CTOR(BinningRealDiscretizor);
protected:
   void build_thresholds(const InstanceBag& sourceBag); //PPV
public:
   BinningRealDiscretizor(int attrNum,
			  int binSize, const SchemaRC& schema);
   BinningRealDiscretizor(const BinningRealDiscretizor&, CtorDummy);
   Bool operator ==(const RealDiscretizor& source);
   Bool operator ==(const BinningRealDiscretizor& source);
   int class_id() const{ return CLASSID_BINNINGDISC;}
   RealDiscretizor* copy();
};

PtrArray<RealDiscretizor*>*
create_binning_discretizors(LogOptions& logOptions,
			    const InstanceBag& bag,
			    const Array<int>& binCounts);

PtrArray<RealDiscretizor*>*
create_binning_discretizors(LogOptions& logOptions,
			    const InstanceBag& bag,
			    int binCount);
#endif
