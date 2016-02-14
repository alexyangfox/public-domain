// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "Inducer.c".

#ifndef _TDDTInducer_h
#define _TDDTInducer_h 1

#include <DTCategorizer.h>
#include <CtrInducer.h>
#include <SplitInfo.h>

// Could be moved if a better file is found
typedef Array<MString> MStringArray;

class TDDTInducer : public CtrInducer {
   // options
   struct TDDTOptions{
      int maxLevel;                 // max level to grow.
      int lowerBoundMinSplitWeight; // The lower bound for the minimum weight
				    //   of instances in a node.
      int upperBoundMinSplitWeight; // The upper bound for the same.
      Real minSplitWeightPercent;   // The percent (p) used to calculate the
				    // min weight of instances in a node (m).
				    // m = p * num instances / num categories
      Bool debug;
      Bool unknownEdges;            // Have an edge with "unknown" from every
				    // node 
      SplitByType splitBy;
   } tddtOptions;

   CGraph *cgraph; // Graph to build this in, or NULL if to create one.
   DTCategorizer *decisionTreeCat;

protected:
   // functions called within induce_decision_tree
   virtual int get_split_attribute(Categorizer *) const { return -1; }
   virtual void initialize_node_info(CGraph& treeGraph,
				     NodePtr child, int splitAttr) const
     { (void)treeGraph;  (void)child;  (void)splitAttr; }
   virtual void set_node_info(CGraph& /* treeGraph */,
			      NodePtr /* child */,
			      int /* splitAttr */,
			      TDDTInducer * /* childInducer */,
			      Category /* cat */) const { }
   
public:
   virtual void OK(int level = 0) const;
   // Given a cgraph, the tree will be formed there.
   TDDTInducer(const MString& dscr, CGraph* aCgraph = NULL);
   TDDTInducer(const TDDTInducer& source, CtorDummy, CGraph* aCgraph = NULL);
   virtual ~TDDTInducer();

   virtual void train();                                    // build structure
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const;
   virtual const RDGCategorizer& get_rdg_categorizer() const;
   virtual RDGCategorizer& get_rdg_categorizer();
   
   // This creates a name for the sub-inducer.  
   virtual MString name_sub_inducer(MString catDescr, Category catNum) const;
   // create_leaf_cat creates a leaf categorizer when splitting is done.
   virtual Categorizer* create_leaf_categorizer(int numInstances,
						Category preferredMajority) const;
   // Actual Inducer.  It builds the tree in CGraph and returns
   //   a pointer to the root.
   virtual NodePtr induce_decision_tree(CGraph&, Category preferredMajority) const;

   // Best_split finds the best split in the node and returns a
   // categorizer implementing it.  Allocated Catnames
   // which must be freed by the caller.  Pure virtual function
   virtual Categorizer *best_split(MStringArray*& catNames) const = 0;
   // create_subinducer creates the Inducer for calling recursively.
   // Note that since this is an abstract class, it can't create a copy
   //   of itself.
   // Important: you MUST copy all settable options to the created inducer.  A
   //   good way to do this is to update copy_options by adding new copies and
   //   calling TDDTInducer::copy.
   virtual TDDTInducer* create_subinducer(const MString& dscr,
                                          CGraph& aCgraph) const = 0;
   virtual int num_nodes() const;
   virtual int num_leaves() const;

   // options.  Don't forget to update copy_options.
   void set_c45_options();
   void set_debug(Bool val) { tddtOptions.debug = val; }
   Bool get_debug() const { return tddtOptions.debug; }
   int get_max_level() const {return tddtOptions.maxLevel;}
   void set_max_level(int level)
      { tddtOptions.maxLevel = level;}
   void set_lower_bound_min_split_weight(int val)
      { tddtOptions.lowerBoundMinSplitWeight = val; }
   int get_lower_bound_min_split_weight() const
      { return tddtOptions.lowerBoundMinSplitWeight; }
   void set_upper_bound_min_split_weight(int val)
      { tddtOptions.upperBoundMinSplitWeight = val; }
   int get_upper_bound_min_split_weight() const
      { return tddtOptions.upperBoundMinSplitWeight; }
   void set_min_split_weight_percent(Real val)
      { tddtOptions.minSplitWeightPercent = val; }
   Real get_min_split_weight_percent() const
      { return tddtOptions.minSplitWeightPercent; }
   void set_unknown_edges(Bool val) { tddtOptions.unknownEdges = val; }
   Bool get_unknown_edges() const { return tddtOptions.unknownEdges; }
   SplitByType get_split_by() const { return tddtOptions.splitBy; }
   void set_split_by(const SplitByType val) { tddtOptions.splitBy = val; }
   TDDTOptions get_options() const { return tddtOptions; }
   void set_options(TDDTOptions val) { tddtOptions = val; }
   void set_user_options(const MString& prefix);
   void copy_options(const TDDTInducer& inducer);

};

#endif
