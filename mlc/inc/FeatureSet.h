// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _FeatureSet_h
#define _FeatureSet_h 1

#include <basics.h>
#include <DynamicArray.h>
#include <InstList.h>

class FeatureSet : private DynamicArray<int>
{
   Array<int>& operator=(const Array<int>& other) {
      return Array<int>::operator=(other);
   }
   DynamicArray<int>& operator=(const DynamicArray<int>& other) {
      return DynamicArray<int>::operator=(other);
   }
   
public:
   void OK() const;

   FeatureSet();
   FeatureSet(const FeatureSet& other, CtorDummy dummy);
   FeatureSet(const SchemaRC& schema);
   FeatureSet(const Array<int>& featureArray);

   Bool operator==(const FeatureSet& other) const
   {  if(num_features() != other.num_features()) return FALSE;
      else return ::operator==(*this, other); }
   Bool operator!=(const FeatureSet& other) const
   { return !(operator==(other)); }
   FeatureSet& operator=(const FeatureSet& other) {
      DynamicArray<int>::operator=(other);
      return *this;
   }
   
   void add_feature(int featureNum);
   Bool instances_equal(const InstanceRC& inst1, const InstanceRC& inst2)
        const;
   Bool contains(int elem) const;
   Bool contains(const FeatureSet& other) const;
   void difference(const FeatureSet& other, FeatureSet& diff) const;
   void project_down(const FeatureSet& oldProj, const FeatureSet& oldInst,
		     FeatureSet& newProj, FeatureSet& newInst)
        const;
   const Array<int>& get_feature_numbers() const {
         return *this; }
   int num_features() const { return size(); }
   
   void display_instance(const InstanceRC& inst, MLCOStream& stream = Mcout)
        const;
   void display_names(const SchemaRC& schema, MLCOStream& stream = Mcout)
        const;
   void display(MLCOStream& stream = Mcout) const;
   
};

DECLARE_DISPLAY(FeatureSet);

#endif

