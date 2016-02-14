// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _CtrInstanceBag_h
#define _CtrInstanceBag_h 1

#include <BagSet.h>
#include <BagCounters.h>

class CtrInstanceBag; //defined later in the file
typedef PtrArray<CtrInstanceBag*> CtrBagPtrArray;

// Note that InstanceBag is VIRTUAL so that
//   CtrInstanceList which does multiple inheritance from this class
//   and from InstanceList will have only one copy of the Bag.
class CtrInstanceBag : public virtual InstanceBag {
   BagCounters* bagCounters;
   void CtrInstanceBag::check_init_counters();
   NO_COPY_CTOR(CtrInstanceBag);
protected:
   CtrInstanceBag();
   virtual void new_counters();
   // Causes fatal_error; use copy(CtrInstanceBag*&)
   virtual void copy(InstanceBag*& lib);
   // Takes on values of given bag; gets ownership of bag
   virtual void copy(CtrInstanceBag*& lcib);
public:
   virtual void OK(int level = 0) const;
   CtrInstanceBag(const SchemaRC& schemaRC);
   CtrInstanceBag(const CtrInstanceBag& source, CtorDummy);
   virtual ~CtrInstanceBag(); 
   virtual Pix add_instance(const InstanceRC& instance);
   virtual InstanceRC remove_instance(Pix& pix);
   virtual Category majority_category(Category tieBreaker = UNKNOWN_CATEGORY_VAL) const;
   virtual CtrBagPtrArray* ctr_split(const Categorizer& cat) const;
   virtual const BagCounters& counters() const;
   virtual CtrInstanceBag& cast_to_ctr_instance_bag();
   virtual const CtrInstanceBag& cast_to_ctr_instance_bag() const;
   virtual InstanceBag* create_my_type(const SchemaRC& schemaRC) const;
   virtual CtrInstanceBag *find_attr_val(int attrNum,
					 NominalVal val,
					 Bool exclude,
					 Bool excludeUnknowns) const;   
   CtrInstanceBag* shuffle(MRandom* mrandom = NULL,
			   InstanceBagIndex* index = NULL) const;
};
#endif

