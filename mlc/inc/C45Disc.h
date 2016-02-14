// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _C45Discretizor_h
#define _C45Discretizor_h 1

// @@ move to RealDiscretizor.h
const int CLASSID_C45DISC = 103;

#include <RealDiscretizor.h>
#include <C45Inducer.h>

class C45Discretizor : public RealDiscretizor{
   NO_COPY_CTOR(C45Discretizor);
   C45Inducer ind;
   void build_thresholds(const InstanceBag& sourceBag); 
public:
   static MString defaultC45Flags;
   C45Discretizor(int attrNum, const SchemaRC& schema);
   C45Discretizor(int attrNum, const SchemaRC& schema, const MString& c45Flags);
   C45Discretizor(const C45Discretizor& disc, CtorDummy);
   virtual void set_log_level(int level) const
     { RealDiscretizor::set_log_level(level); ind.set_log_level(level);}
   Bool operator== (const RealDiscretizor& disc);
   Bool operator== (const C45Discretizor& disc);
   int class_id() const { return CLASSID_C45DISC;}
   RealDiscretizor* copy(); //returns copy of self
};


PtrArray<RealDiscretizor*>*
create_c45_discretizors(LogOptions& logOptions,
			const InstanceBag& bag);

#endif

