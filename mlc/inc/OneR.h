// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _OneRDiscretizor_h
#define _OneRDiscretizor_h 1

#include <basics.h>
#include <RealDiscretizor.h>
#include <DynamicArray.h>
#include <Attribute.h>


class LabelValueCounts : private Array<Category> {
   Bool tie;
   int maxValue;
   Category maxCategory;
   Category& index(int i) { return Array<Category>::index(i);}
   // The above is to avoid warnings since we need to override the const index
   // method
   NO_COPY_CTOR(LabelValueCounts);
public:
   LabelValueCounts(int size);
   void increment(Category category);
   void reset();
   Bool tied_majority() const { return tie; }
   int majority_value() const; 
   Category majority_category() const;
   const Category& index(int i) const { return Array<Category>::index(i); } 
   int size() const { return Array<Category>::size(); }
   LabelValueCounts& operator +=(const LabelValueCounts&);
};

class AttrAndLabel {
public:   
   Real attrVal;
   Category label;
   AttrAndLabel() :attrVal(-REAL_MAX),label(UNKNOWN_CATEGORY_VAL){}
   int operator<(const AttrAndLabel& item2) const
   {  DBG(ASSERT(label != UNKNOWN_CATEGORY_VAL && attrVal != -REAL_MAX));
      return (attrVal < item2.attrVal);}
   int operator==(const AttrAndLabel& item2) const
   {  DBG(ASSERT(label != UNKNOWN_CATEGORY_VAL && attrVal != -REAL_MAX));
      return (attrVal == item2.attrVal);}
};


class OneR : public RealDiscretizor{
   int small;
protected:
   void build_thresholds(const InstanceBag& sourceBag); //PPV
public:
   static int defaultSmall;
   OneR(int attrNum, const SchemaRC& schema, int small = defaultSmall);
   OneR(const OneR& source, CtorDummy dummyArg);
   int get_small() const { return small;}
   void set_small(int val) { small = val;}
   Bool operator == (const RealDiscretizor& disc);
   Bool operator == (const OneR& disc);
   int class_id() const { return CLASSID_HOLTE1RDISC;}
   RealDiscretizor* copy();
};

PtrArray<RealDiscretizor*>*
create_OneR_discretizors(LogOptions& logOptions,
			 const InstanceBag& bag,
			 const Array<int>& smallValues);

PtrArray<RealDiscretizor*>*
create_OneR_discretizors(LogOptions& logOptions,
			 const InstanceBag& bag,
			 int small = OneR::defaultSmall);

#endif
