// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _EntropyGainCache_h
#define _EntropyGainCache_h 1

#include <SplitInfoCache.h>



class EntropyGainCacheOption {
   int minSplit;
   Bool useAttrEqCat;
   Real mutualInfoRate;
   Real pessimisticZ;
   Real penaltyPower;
   // Copy ctor OK.

public:
   virtual void OK(int level = 0) const;
   EntropyGainCacheOption();
   ~EntropyGainCacheOption() {DBG(OK());}   
   void set_min_split(int val) {minSplit = val; OK();}
   int get_min_split() const { return minSplit; }
   void set_mutual_info_rate(Real val) {mutualInfoRate = val; OK(); }
   Real get_mutual_info_rate() const { return mutualInfoRate; }
   void set_use_attr_eq_cat_bool(Bool val) {
      useAttrEqCat = val; OK();}
   Bool get_use_attr_eq_cat_bool() const {
      return useAttrEqCat; }   
   void set_pessimisticZ(Real val) {pessimisticZ = val; OK(); }
   Real get_pessimisticZ() const { return pessimisticZ;}
   void set_penalty_power(Real val) {penaltyPower = val; OK(); }
   Real get_penalty_power() const { return penaltyPower;}

   virtual const EntropyGainCacheOption&
                cast_to_entropy_gain_cache_option() const { return *this; }
};
   


class EntropyGainCache : public SplitInfoCache {
   NO_COPY_CTOR(EntropyGainCache);


   virtual void use_attr_cat(int attrNum);
   virtual void use_attr_eq_cat(int attrNum);

protected:
   virtual void build_info_values(int attrNum);
   virtual CtrInstanceBag* split_child(int attrNum, NominalVal ourVal,
				       NominalVal splitVal);

public:
   EntropyGainCacheOption options;   

   EntropyGainCache(CtrInstanceBag*& bag) : SplitInfoCache(bag) {}
   EntropyGainCache(const EntropyGainCache& sic, CtorDummy)
      : SplitInfoCache(sic, ctorDummy), options(sic.options) {}
   virtual SplitInfoCache& child_cache(int attrNum, NominalVal val);
   virtual void display(MLCOStream& stream = Mcout) const;
};

DECLARE_DISPLAY(EntropyGainCache);


#endif





