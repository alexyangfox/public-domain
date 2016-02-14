// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file "Inducer.c".

#ifndef _OODGInducer_h
#define _OODGInducer_h 1

#include <Inducer.h>
#include <RDGCat.h>
#include <ProjBag.h>
#include <Array.h>

typedef Array<NodePtr> NodeArray;

class OODGInducer : public Inducer {
   // Options
   Bool drawConstNodes;     // Draw nodes which do not really branch
                            //   (this creates a levelled graph).
   Bool nodeNameWithBagNum; // Add bag number to node.  Helps debugging.
   Real pruneRatio;         // For pruning

   // Saved mapping
   Array<int> *defaultMap;

   CGraph *cgraph; // Graph to build this in, or NULL if to create one.
   RDGCategorizer *decisionGraphCat;

   // Definitely not the nicest input parameters.
   void connect_const_dest(RootedCatGraph& dg,
	   Category bagNum, int numInstances,
	   Real totalWeight, const AttrInfo& splitAttrInfo,
	   const NodePtr destNode, NodePtr& newNode) const;

   void connect_by_val(RootedCatGraph& dg,
		       Category bagNum, int numInstances,
		       Real totalWeight, const AttrInfo& splitAttrInfo,
		       int splitAttrNum, const Array<NominalVal>& dest,
		       const Array<Real>& destCounts, const NodeArray& na, 
		       NominalVal defaultDest, NodePtr& newNode) const;


public:
   virtual void OK(int level = 0) const;
   // Given a cgraph, the graph will be formed there.
   OODGInducer(const MString& dscr, CGraph* aCgraph = NULL);
   virtual ~OODGInducer();

   virtual void train();                            // build structure
   virtual Bool was_trained(Bool fatalOnFalse = TRUE) const;
   virtual const Categorizer& get_categorizer() const;

   // init_train() and end_train() are called before training starts
   //   and after training ends.  They are useful for statistics.
   virtual void init_train() {}
   virtual void end_train()  {}

   // create_leafs creates the leafs to start the bottom-up induction.
   virtual NodeArray *create_leaf_nodes(RootedCatGraph& dg,
                                        const BagPtrArray& bpa) const;
   // create_interior_nodes creates a decision graph level from
   //   ProjInfoPtrList to the given NodeArray, then redefines the
   //   NodeArray to be the new NodeArray.
   virtual BagPtrArray* create_interior_nodes(RootedCatGraph& dg,
                 const AttrInfo& splitAttrInfo, int splitAttrNum,
		 ProjInfoPtrList*& pipl,
                        Category defaultDestBag, NodeArray*& na) const;
   // Actual Inducer.  It builds the decision graph.
   virtual NodePtr induce_decision_graph(RootedCatGraph& dg,
					 BagPtrArray*& bpa, NodeArray*& na,
					 Array<int>& attrMap);
   // find_cover() creates a BagPtrArray and a NodeArray from ProjBag.
   //   The BagPtrArray is a list of bags, each one containing the instances
   //   that should "fall" into the corresponding Node in the NodeArray
   //   when that instance is classified using the decision graph.
   virtual ProjInfoPtrList* find_cover(int attrNum,
                                       const BagPtrArray& bpa) const = 0;
   // best_attr() finds the best attribute to delete from the instances
   //   in order to do the projection.  Pure virtual function.
   virtual int best_split(const BagPtrArray& bpa) = 0;
   virtual void prune_nodes(ProjInfoPtrList& pipl) = 0;
   virtual int num_nodes() const;
   virtual int num_leaves() const;

   // Options
   void set_draw_const_nodes(Bool val);
   Bool get_draw_const_nodes() const;
   void set_node_name_with_bag_num(Bool val);
   Bool get_node_name_with_bag_num() const;
   void set_prune_ratio(Real ratio);
   Real get_prune_ratio() const;

   virtual void set_default_map(const Array<int>& map);
   virtual const Array<int>& get_default_map();
};

#endif

