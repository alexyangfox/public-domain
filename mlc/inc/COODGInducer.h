// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "Inducer.c".

#ifndef _COODGInducer_h
#define _COODGInducer_h 1

#include <DTCategorizer.h>
#include <CtrBag.h>
#include <CtrInducer.h>
#include <Array.h>
#include <DynamicArray.h>
#include <RDGCat.h>
#include <ProjLevel.h>
#include <ProjGraph.h>
#include <AttrOrder.h>

#define COODG_INDUCER 120

class COODGInducer : public CtrInducer {
   Categorizer *rdgCat;

   virtual void OK(int level=0) const;

public:
   enum Method { topDown, bottomUp, recursive };
   enum DisplayMode { dispNone, dispNormal, dispFull };
   enum SplitType { splitTop, splitBottom, splitLargest, splitSmallest };
private:
   Method method;
   DisplayMode displayMode;
   Bool prune;
   SplitType splitType;
   AttrOrder attrOrder;

protected:
   // different training methods
   virtual void train_top_down(CatGraph& graph,
                               Array<ProjLevel *>& levels,
			       const FeatureSet& includedSet);
   virtual void train_bottom_up(CatGraph& graph,
				Array<ProjLevel *>& levels,
				const FeatureSet& includedSet);
   virtual void train_recursively(CatGraph& graph,
				  Array<ProjLevel *>& levels,
				  ProjLevel *top,
				  ProjLevel *bottom,
				  Bool topHalf);
   virtual void add_and_connect(CatGraph& graph,
				Array<ProjLevel *>& levels);
   
public: 
   COODGInducer(const MString& dscr);
   ~COODGInducer();

   int class_id() const { return COODG_INDUCER; }
   
   void set_user_options(const MString& prefix);
   void set_order(const Array<int>& order) { attrOrder.set_order(order); }
   void set_method(Method meth) { method = meth; }
   Method get_method(void) const { return method; }
   void set_display_mode(DisplayMode mode) { displayMode = mode; }
   DisplayMode get_display_mode(void) const { return displayMode; }
   void set_prune(Bool pr) { prune = pr; }
   Bool get_prune() const { return prune; }
   void set_split_type(SplitType type) { splitType = type; }
   SplitType get_split_type() { return splitType; }
   
   virtual void train();
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const;
   virtual AttrOrder& get_attr_order_info() { return attrOrder; }
};

#endif
