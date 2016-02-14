// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _LazyDTCategorizer_h
#define _LazyDTCategorizer_h 1

#include <Categorizer.h>
#include <CtrBag.h>
#include <SplitInfo.h>
#include <EntropyGainCache.h>

class InfoValAndSize {
public:
   Real infoVal;
   int  numInstances;
   InfoValAndSize() {infoVal = -REAL_MAX; numInstances = -1;}
   int operator<(const InfoValAndSize& b) const {
      return infoVal < b.infoVal || 
	 (infoVal == b.infoVal && numInstances < b.numInstances);}
   int operator>(const InfoValAndSize& b) const {
      return infoVal > b.infoVal || 
	 (infoVal == b.infoVal && numInstances > b.numInstances);}
   int operator==(const InfoValAndSize& b) const {
      return infoVal == b.infoVal;}
//      return infoVal == b.infoVal && numInstances == b.numInstances;}
};

class LazyDTCategorizer : public Categorizer {
private:
   NO_COPY_CTOR(LazyDTCategorizer);
   CtrInstanceBag ctrBag;
   EntropyGainCache *cache;

   Real multiSplitRate;
   Real delRate;
   Bool multiSplitOn;
   virtual int prob_categorize(SplitInfoCache* sic,
			       const InstanceRC&,
			       Real multiSplitRate,
			       Array<int>& result) const;      

public:
   LazyDTCategorizer(const CtrInstanceBag& bag, const MString& dscr);   
   LazyDTCategorizer(const LazyDTCategorizer& source,
		     const CtorDummy dummyArg);
   virtual ~LazyDTCategorizer() {delete cache;}

   virtual AugCategory categorize(const InstanceRC&) const;
   virtual void display_struct(MLCOStream& stream = Mcout,
                          const DisplayPref& dp = defaultDisplayPref) const;
   virtual Categorizer* copy() const;   
   // Returns the class id
   virtual int class_id() const { return CLASS_LAZYDT_CATEGORIZER; }
   virtual Bool operator==(const Categorizer &cat) const;
   virtual void set_min_split(int val) { cache->options.set_min_split(val); }
   virtual Real get_min_split() const { return cache->options.get_min_split();}
   virtual void set_mutual_info_rate(Real val) {
      cache->options.set_mutual_info_rate(val); }
   virtual Real get_mutual_info_rate() const {
      return cache->options.get_mutual_info_rate(); }
   virtual void set_multi_split_rate(Real val) {
      multiSplitRate = val; }
   virtual Real get_del_rate() const {
      return delRate; }
   virtual void set_del_rate(Real val) {
      delRate = val; }
   virtual Bool get_multi_split_bool() const {
      return multiSplitOn; }
   virtual void set_multi_split_bool(Bool val) {
      multiSplitOn = val; }
   virtual Real get_multi_split_rate() const {
      return multiSplitRate; }
   virtual void set_pessimisticZ(Real val) {
      cache->options.set_pessimisticZ(val);}
   virtual Real get_pessimisticZ() const
      { return cache->options.get_pessimisticZ();}
   virtual void set_use_attr_eq_cat_bool(Bool val)
      { cache->options.set_use_attr_eq_cat_bool(val);}
   virtual Bool get_use_attr_eq_cat_bool() const 
      { return cache->options.get_use_attr_eq_cat_bool();}
   virtual void set_penalty_power(Real val)
      { cache->options.set_penalty_power(val);}   
   virtual Real get_penalty_power() const 
      { return cache->options.get_penalty_power();}
};

#endif



