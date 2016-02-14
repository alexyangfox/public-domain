// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _LazyDTInducer_h
#define _LazyDTInducer_h 1

#include <CtrInducer.h>
#include <LazyDTInducer.h>
#include <LazyDTCat.h>

class LazyDTInducer : public CtrInducer {
   LazyDTCategorizer* categorizer;
   int minSplit;
   Real mutualInfoRate;
   Bool useAttrEqCat;
   Real multiSplitRate;
   Real delRate;
   Bool multiSplitOn;
   Real pessimisticZ;
   Real penaltyPower;
   
public:
   virtual void OK(int /* level */ = 0) const {} 
   LazyDTInducer(const MString& description);
   virtual ~LazyDTInducer() {delete categorizer;}
   virtual void train();
   virtual Bool was_trained(Bool fatal_on_false = TRUE) const;
   virtual const Categorizer& get_categorizer() const;
   virtual void set_min_split(int val);
   virtual int get_min_split() const { return minSplit; }
   virtual void set_mutual_info_rate(Real val);
   virtual Real get_mutual_info_rate() const { return mutualInfoRate; }
   virtual void set_use_attr_eq_cat_bool(Bool val) { useAttrEqCat = val; }
   virtual Bool get_use_attr_eq_cat_bool() const { return useAttrEqCat; }
   virtual Real get_del_rate() const { return delRate; }
   virtual void set_del_rate(Real val) { delRate = val; }
   virtual void set_multi_split_rate(Real val) { multiSplitRate = val; }
   virtual Real get_multi_split_rate() const { return multiSplitRate; }
   virtual Bool get_multi_split_bool() const { return multiSplitOn; }
   virtual void set_multi_split_bool(Bool val) { multiSplitOn = val; }
   virtual void set_pessimisticZ(Real val) { pessimisticZ = val;}
   virtual Real get_pessimisticZ() const { return pessimisticZ; }
   virtual void set_user_options(const MString& prefix);
   virtual Real get_penalty_power() const { return penaltyPower;}
   virtual void set_penalty_power(Real val) {penaltyPower = val; }
};   
#endif

