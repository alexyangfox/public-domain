// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _SplitInfoCache_h 
#define _SplitInfoCache_h 1

#include <CtrBag.h>
#include <Array.h>
#include <LogOptions.h>

const Real UNSET_INFO_VAL = -REAL_MAX + 1;

class SplitInfoCache;

// this class was introduced to speed up.
class CacheInfo {
public:   
   SplitInfoCache* sic;
   Real infoValue;
   int  numInstances;
   NominalVal splitVal;

   CacheInfo() { init(); }
   void init() {
       sic = NULL; infoValue = UNSET_INFO_VAL; 
       splitVal = UNKNOWN_NOMINAL_VAL - 1; numInstances = -1;
   } 
   CacheInfo(Real infoVal, NominalVal val) {
      sic = NULL;
      infoValue = infoVal;
      splitVal = val;
      numInstances = -1;
   }
   ~CacheInfo();
};

//ABC
class SplitInfoCache {
   NO_COPY_CTOR(SplitInfoCache);
   LOG_OPTIONS;
   CtrInstanceBag *bag;
   int numInstances;
protected:

   PtrArray< Array<CacheInfo> *> infoMatrix; // sparse matrix
   virtual void build_info_values(int attrNum) = 0;
   virtual CtrInstanceBag* split_child(int attrNum, NominalVal ourVal,
				       NominalVal splitVal) = 0;
public:
   virtual void OK(int level = 0) const;
   SplitInfoCache(CtrInstanceBag*& bag);
   SplitInfoCache(const SplitInfoCache& sic, CtorDummy);
   virtual ~SplitInfoCache();

   const CtrInstanceBag& get_bag() {ASSERT(bag != NULL); return *bag;}
   int num_instances() const { return numInstances; }
   Real info_value(int attrNum, NominalVal val) const;   
   // non-const because it returns a pointer to non-const member.
   virtual SplitInfoCache& child_cache(int attrNum, NominalVal val) = 0;
   virtual const CtrInstanceBag& get_bag() const { return *bag; }
   virtual void delete_small_caches(int minInstances);
   int num_instances(int attrNum, NominalVal val) const;
};




#endif
