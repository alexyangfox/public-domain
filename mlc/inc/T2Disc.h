// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _T2Discretizor_h
#define _T2Discretizor_h 1

// @@ move to RealDiscretizor.h
const int CLASSID_T2Disc = 104;

#include <RealDiscretizor.h>

class T2Discretizor : public RealDiscretizor{
   NO_COPY_CTOR(T2Discretizor);
   void run_t2(const InstanceBag& bag, int numIntervals);
   void build_thresholds(const InstanceBag& sourceBag); 
   int numIntervals;
public:
   T2Discretizor(int attrNum, const SchemaRC& schema, int theNumIntervals);
   T2Discretizor(const T2Discretizor& disc, CtorDummy);
   Bool operator== (const RealDiscretizor& disc);
   Bool operator== (const T2Discretizor& disc);
   int class_id() const { return CLASSID_T2Disc;}
   RealDiscretizor* copy(); //returns copy of self
};

PtrArray<RealDiscretizor*>*
create_t2_discretizors(LogOptions& logOptions,
		       const InstanceBag& bag,
		       const Array<int>& numInterVals);

PtrArray<RealDiscretizor*>*
create_t2_discretizors(LogOptions& logOptions,
		       const InstanceBag& bag,
		       int numInterVals);

#endif

