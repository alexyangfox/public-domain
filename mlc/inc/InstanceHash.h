// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _InstanceHash_h
#define _InstanceHash_h 1


#include <basics.h>
#include <CtrBag.h>
#include <MLCStream.h>
#include <UnivHashTable.h>

// This class will be used only for the purpose of providing KeyType
// and DataType for the InstanceHashTable class.
// It has a regular copy constructor(default one) instead of dummy
// argument copy constructor because it will be used as a template
// argument of DblLinkList.

#define InstanceHashBag InstanceBag

class InstanceBagSameAttr_ {
private:
   InstanceHashBag* bag;

public:
   // create a bag with the instance.   
   InstanceBagSameAttr_(const InstanceRC& instance);
   // normal copy constructor.
   InstanceBagSameAttr_(const InstanceBagSameAttr_& source);
   InstanceBagSameAttr_& operator=(const InstanceBagSameAttr_& source);
   ~InstanceBagSameAttr_();

   virtual void OK(int level=0) const;
   const InstanceHashBag& get_bag() const { return *bag; }
   // length is required for universal hashing KeyType.
   int length() const { return bag->get_instance(bag->first()).num_attr(); }

   // return TRUE iff the instances in the bags match EXCLUDING the
   // label (note that this is different from the usual semantics of
   // operator=().
   Bool operator==(const InstanceBagSameAttr_& source) const;
   Bool operator!=(const InstanceBagSameAttr_& source) const {
      return !(operator==(source));
   }
					       
   // the same as operator==() because KeyType and DataType are the
   // same.
   Bool contains_key(const InstanceBagSameAttr_& source) const {
      return operator==(source);
   }

   InstanceBagSameAttr_ get_key() const { return *this; }
   
   // overwrite methods to meet the condition that there must be at
   // least one element and all the instances are the same.
   virtual Pix add_instance(const InstanceRC& instance);
   virtual InstanceRC remove_instance(Pix& pix);

   // operator<<() is neeed by HashTable class.
   friend MLCOStream& operator<<(MLCOStream& stream, const
				 InstanceBagSameAttr_& source);
};   



// InstanceHashTable_ is a universal hash table specifically
// designed for InstanceBagSameAttr_ data types. The purpose of defining
// this class is to redefine virtual hash() function in UniversalHashTable.

class InstanceHashTable_ :
   public UniversalHashTable<InstanceBagSameAttr_,
                             InstanceBagSameAttr_> {
private:
   // can't make NO_COPY_CTOR2() because of two template arguments.
   InstanceHashTable_(const InstanceHashTable_& source);

public:
   InstanceHashTable_(int estimatedNum);
   InstanceHashTable_(int estimatedNum, unsigned int seed);   
   InstanceHashTable_(const InstanceHashTable_& source, CtorDummy);
   virtual int hash(const InstanceBagSameAttr_& instance) const;
};


// InstanceHashTable is clean outer interface to the user of hash
// table of instances. Notice that it is not derived from any hash
// table but has a private data member of type InstanceHashTable_.

class InstanceHashTable {
private:
   NO_COPY_CTOR(InstanceHashTable);
   InstanceHashTable_ hashTable;
   int numInstances; // it counts all the instances in each bag in the
                     // hash table.
   
public:
   void OK() const { hashTable.ok_members(); }
   InstanceHashTable(int estimatedNum, const InstanceBag* bag = NULL);
   InstanceHashTable(int estimatedNum, unsigned int seed, const InstanceBag*
		     bag = NULL);    
   InstanceHashTable(const InstanceHashTable& source, CtorDummy);
   
   // accessors
   int num_instances() const { return numInstances; }
   int num_bags() const { return hashTable.num_items(); }

   // return the number of matched instances in the hash table with
   // the bag instances.
   int num_matched_labelled_instances(const InstanceBag& bag) const;
   Bool find_labelled_instance(const InstanceRC& instance) const;
   // add new instance into the hash table.
   // it is allowed to add duplicate instances.
   void insert(const InstanceRC& instance);
   void del(const InstanceRC& instance);
   void del_all_unlabelled(const InstanceRC& instance);
   const InstanceHashBag* find(const InstanceRC& instance) const;
   void display(MLCOStream& strm = Mcout) const { hashTable.display(strm); }
};
#endif





