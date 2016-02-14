// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _BagCounters_h
#define _BagCounters_h 1

#include <Array.h>
#include <Array2.h>
#include <InstanceRC.h>

class BagCounters {
   // See BagCounters.c for a description of this complicated structure.
   PtrArray<Array2<int>*> valueCounts;
   Array<int> labelCounts;
   PtrArray<Array<int>*> attrCounts;
   DBG_DECLARE(SchemaRC schema;)
   DBG_DECLARE(void check_nominal(int attrNum) const;) 

   NO_COPY_CTOR(BagCounters);

protected:
   // Increment and Decrement are prefix (operation, then return)
   void update_counters(const InstanceRC& instance, int delta);
public:
   int OK(int level = 0) const; // returns sum of label counters.
   // The constructor zeroes all counts.
   BagCounters(const SchemaRC& aSchema);
   // copy constructor
   BagCounters(const BagCounters& source, CtorDummy);
   ~BagCounters();

   // comparison methods.
   Bool operator==(const BagCounters& source) const;
   Bool operator!=(const BagCounters& source) const {
      return !operator==(source);
   }

   // count of label=labelVal, attributeNum = attrVal
   int val_count(NominalVal labelVal, int attrNum,
                 NominalVal attrVal) const;
   // count of attrNum = attrVal (over all labels)
   int attr_count(int attrNum, NominalVal attrVal) const;
   int label_count(NominalVal labelVal) const;
   // count number of existing different values.
   int attr_num_vals(int attrNum) const;
   // count number of actual different labels appearing.
   int label_num_vals() const;
   void add_instance(const InstanceRC& instance);
   void del_instance(const InstanceRC& instance);
   virtual void display(MLCOStream& stream = Mcout) const;
   // accessor functions
   virtual const PtrArray<Array2<int>*>& value_counts() const; 
   virtual const Array<int>& label_counts() const; 
   virtual const PtrArray<Array<int>*>& attr_counts() const; 
};

DECLARE_DISPLAY(BagCounters);


#endif
