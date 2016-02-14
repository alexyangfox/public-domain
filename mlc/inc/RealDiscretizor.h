// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _RealDiscretization_h
#define _RealDiscretization_h 1

#include <basics.h>
#include <InstList.h>
#include <Attribute.h>

//Class ID's for discretizors
const int CLASSID_ENTROPYDISC = 100;
const int CLASSID_BINNINGDISC = 101;
const int CLASSID_HOLTE1RDISC = 102;

class RealDiscretizor{  //ABC

protected:
   int attrNum;
   NominalAttrInfo* nai;
   RealAttrInfo *rai;
   MString descr;
   DynamicArray<RealAttrValue_>* thresholds; 
   void base_copy(const RealDiscretizor& source);
   void check_thresholds();
   virtual Bool equal_shallow(const RealDiscretizor& source);
   virtual void build_thresholds(const InstanceBag& source) = 0; //PPV
   LOG_OPTIONS;
   NO_COPY_CTOR(RealDiscretizor);
public:
   RealDiscretizor(int attrNum, const SchemaRC& schema);
   RealDiscretizor(const RealDiscretizor& source, const CtorDummy dummyArg);
   virtual Bool operator == (const RealDiscretizor& rd) = 0;
   virtual int class_id() const = 0;
   virtual ~RealDiscretizor();
   virtual void OK(int level = 0) const;
   virtual void create_thresholds(const InstanceBag& sourceBag);
   virtual void create_real_thresholds(Array<Real>& newVals);
   virtual NominalAttrInfo* real_to_nominal_info() const;
   virtual NominalAttrValue_ discretize_real_val(const InstanceRC& inst) const;
   virtual void set_description(const MString& desc){descr = desc;}
   virtual MString get_description() const { return descr;}
   virtual void display(MLCOStream& stream = Mcout) const;
   virtual int num_intervals_chosen();
   virtual RealDiscretizor* copy() = 0;
};

SchemaRC gen_discretized_schema(const InstanceBag& bag,
				PtrArray<RealDiscretizor*>* const disc);

void discretize_instance(const PtrArray<RealDiscretizor*>* const disc,
			 const SchemaRC& discretizedSchema,
			 InstanceRC& instanceToBeDiscretized);

InstanceBag*  discretize_bag(const InstanceBag& source,
			     PtrArray<RealDiscretizor*>* const discretizors); 

int num_distinct_vals(const InstanceBag& bag, int attrNum);

DECLARE_DISPLAY(RealDiscretizor);
#endif

