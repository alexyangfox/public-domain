// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "Inducer.c".

#ifndef _ODGInducer_h
#define _ODGInducer_h 1

#include <basics.h>
#include <Array.h>
#include <DynamicArray.h>
#include <AugCategory.h>
#include <ConstCat.h>
#include <CtrBag.h>
#include <BagSet.h>
#include <DTCategorizer.h>
#include <CtrInducer.h>
#include <SplitInfo.h>

enum PostProcType { none, set_unknown, merge};

class UsedAttributes {
public: 
   Bool nominalUsed;
   DynamicArray<Real>* thresholdUsed;
   UsedAttributes() { thresholdUsed = new DynamicArray<Real>(0); }
   ~UsedAttributes() { delete thresholdUsed; }
};

class ODGInducer : public CtrInducer {
   // options
   struct ODGOptions {
      Bool debug;
      Bool unknownEdges;            // Have an edge with "unknown" from every
				    // node
      PostProcType postProc; 
      Bool splitPure;               // If true, split pure bags.
      Real growConflictRatio;
      int minSplit;
      Bool cVMerge;
      Bool growSplitPure;
      Bool cvPrune;
   } odgOptions;
 protected: 
   Array<UsedAttributes>* usedAttributes;
   DynamicArray<SplitInfo>* splitInfoArray;
   CGraph *cgraph; // Graph to build this in, or NULL if to create one.
   RDGCategorizer *decisionGraphCat;
   Bool used_threshold(int attrNum, Real threshold);
 public: 
   virtual void OK(int level = 0) const;
   ODGInducer(const MString& dscr, CGraph* aCgraph = NULL);
   virtual ~ODGInducer();
   virtual void train();                               // build structure
   NodePtr induce_oblivious_decision_graph(CGraph& aCgraph);
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const; 
   virtual Categorizer* create_leaf_categorizer(const CtrInstanceBag*,int)
           const;
   virtual int num_nodes() const;
   virtual int num_leaves() const;
   // Best_split finds the best split in the node and returns a
   // categorizer implementing it.  Allocates Catnames
   // which must be freed by the caller.  Pure virtual function
   virtual Categorizer *best_split(Array<UsedAttributes>& usedAttributes,
				   DynamicArray<SplitInfo>& splitInfoArray,
                                   DynamicArray<const CtrInstanceBag*> &,
				   MStringArray*& catNames,
				   Real& minCondEntropy,
				   Real previousEntropy) = 0;
   // Options access and modification functions
   void set_debug(const Bool d) { odgOptions.debug = d; }
   Bool get_debug() const { return odgOptions.debug; }
   void set_unknown_edges(const Bool val) { odgOptions.unknownEdges = val; }
   Bool get_unknown_edges() const { return odgOptions.unknownEdges; }
   void set_post_proc(const PostProcType val) { odgOptions.postProc = val; }
   PostProcType get_post_proc() const { return odgOptions.postProc; }
   void set_split_pure(const Bool val) { odgOptions.splitPure = val; }
   Bool get_split_pure() const { return odgOptions.splitPure; }
   void set_grow_conflict_ratio(const Real val) {
      ASSERT(val >= 0);
      odgOptions.growConflictRatio = val;
   }
   Real get_grow_conflict_ratio() const {return odgOptions.growConflictRatio;}
   void set_min_split(int val) {
      ASSERT(val > 0);
      odgOptions.minSplit = val;
   }
   int get_min_split() const { return odgOptions.minSplit; }
   void set_cv_merge(Bool val) { odgOptions.cVMerge = val; }
   Bool get_cv_merge() const { return odgOptions.cVMerge; }
   void set_grow_split_pure(Bool val) { odgOptions.growSplitPure = val; }
   Bool get_grow_split_pure() const { return odgOptions.growSplitPure; }
   void set_cv_prune(Bool val) { odgOptions.cvPrune = val; }
   Bool get_cv_prune() const { return odgOptions.cvPrune; }
   
   // Get/set all the options.
   const ODGOptions& get_options() const { return odgOptions; }
   void set_options(const ODGOptions& oo) { odgOptions = oo; }
   // get options from user.
   void set_user_options(const MString& prefix="ODG");
   const Array<SplitInfo>& split_info_array() const {return *splitInfoArray; }
   void reset_used_attributes();
   void reset_split_info_array();
   const CGraph& get_graph() {
      return decisionGraphCat->rooted_cat_graph().get_graph(); }
   const CatGraph& get_cat_graph() {
      return decisionGraphCat->rooted_cat_graph(); } 
   NodePtr get_root() {
      return decisionGraphCat->rooted_cat_graph().get_root(); }
};

#endif





