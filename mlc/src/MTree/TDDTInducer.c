// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Top-down decision-tree (TDDT) inducer induces decision trees
                   top-down by building smaller training sets and
                   inducing trees for them recursively.
		 The decision tree built has categorizers at each node,
		   and these determine how to branch, i.e., to which child to
		   branch, or whether to classify.  The common cases are:
		   AttrCategorizers, which simply return a given value for an
		   attribute in the node, and ThresholdCategorizers,
		   which return 0 or one based on whether an attribute
		   is less than or greater than a given threshold (valid
		   only for real attributes).  The leaves are usually
		   constant categorizers, i.e., they just return a constant
                   value independent of the instance.
		 The induction algorithm calls best_split, a pure virtual
		   function, to determine the best root split.  Once the split
		   has been chosen, the data in the node is split according to
		   the categorizer best_split returns.  A node is formed, and
		   the algorithm is called recursively with each of the
		   children.  Once each child returns with a subtree, we
		   connect them to the root we split.
		 ID3Inducer, for example, implements the best_split using
		   information gain, but other methods are possible.
	           best_split() can return any categorizer, thus opening
		   the possibility for oblique trees with perceptrons
		   at nodes, recursive trees, etc.  The leaves can
		   also be of any classifier, thus perceptron-trees
		   (Utgoff) can be created, or a nearest-neighbor within
		   a leaf, etc.
  Assumptions  : 
  Comments     : Abstract base class.
  Complexity   : The complexity of train() is proportional to the number of
                   nodes in the resulting tree times the time for deciding
                   on the split() categorizer (done by the derived classes).
		 predict() takes time proportional to the sum of the
                   categorizers time over the path from the root to a
                   leaf node. 
  Enhancements : We may speed things up by having an option to test
                   only splits where the class label changes.  For some
		   measures (e.g., entropy), it can be shown that a split will
		   never be made between two instances with the same class
		   label (Fayyad IJCAI 93 page 1022, Machine Learning journal
		   Vol 8, no 1, page 87, 1992).
                 We may wish to discretize the real values first.
                   By making them linear discrete, we can use the regular
		   counters and things will be faster (note however
		   that the split will usually remain special since
		   it's a binary threshold split, not a multi-way split).
		 Another problem is with attributes that have many values,
		   for example social-security-number.  Computing all cut
		   points can be very expensive.  We may want to skip such
		   attributes by claiming that each value must have at least
		   some number of instances.  Utgoff in ML94 (page 322)
		   mentions that ID slows his system down considerably.  The
		   problem of course is that if you threshold, it sometimes
		   make sense to split on such attributes.  Taken to an
		   extreme, if we had a real "real-value," all values would be
		   different with probability 1, and hence we would skip such
		   an attribute.
                 To speed things up, we may want to have an Inducer
                    that accepts a decision tree and builds stuff
                    in it (vs. getting a graph). 
                    Other options allow for doing the recursion
                    by calling a function instead of creating the
                    actual class.  The advantage of the current method is
                    that it allows a subclass to keep track of the
                    number of levels (useful for lookahead or something).
                    Yet another option is to "recycle" inducers by
                    using our "this" and just changing the training set.
                 We currently split instances but keep the original
                   structure, that is, we don't actually delete the
                   attribute tested on.  It may be faster in some
                   cases to actually create a new List without the
                   attribute.  The disadvantage is that for
                   multi-valued attributes we may wish to branch
                   again, so we can't always delete.  The same goes
                   for tests which are not attributes (e.g., conjunctions). 
  History      : Chia-Hsin Li                                       1/03/95
                   Added Options.
                 Ronny Kohavi                                       9/06/93
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <TDDTInducer.h>
#include <ConstCat.h>
#include <CtrBag.h>
#include <GetOption.h>
#include <SplitInfo.h>

RCSID("MLC++, $RCSfile: TDDTInducer.c,v $ $Revision: 1.62 $")

const MString maxLevelHelp = "The maximum number of levels to grow.  0 "
  "implies no limit.";
const int defaultMaxLevel = 0;

const MString lBMSWHelp = "This option specifies the value of lower bound "
  "of the weight while calculating the minimum split "
  "(overrides weight option).";
const int defaultLBMSW = 1;

const MString uBMSWHelp = "This option specifies the value of upper bound "
  "of the weight while calculating the minimum split (overrides lower bound).";
const int defaultUBMSW = 25;

const MString mSWPHelp = "This options chooses the value of "
  "the weight percent while calculating the minimum split.";
const Real defaultMSWP = 0;

const MString debugHelp = "This option specifies whether to display the "
  "debug information while displaying the graph.";
const Bool defaultDebug = FALSE;

const MString unknownEdgesHelp = "This option specifies whether or not to "
  "allow outgoing UNKNOWN edges from each node. ";
const Bool defaultUnknownEdges = TRUE;

/***************************************************************************
  Description : Initialize arguments to their default values.
  Comments    :
***************************************************************************/
void TDDTInducer::set_user_options(const MString& prefix )
{
   tddtOptions.maxLevel = get_option_int(prefix + 
        "MAX_LEVEL", defaultMaxLevel, maxLevelHelp, TRUE);
   tddtOptions.lowerBoundMinSplitWeight =
      get_option_int(prefix + "LBOUND_MIN_SPLIT",
		     defaultLBMSW, lBMSWHelp, TRUE);
   tddtOptions.upperBoundMinSplitWeight =
      get_option_int(prefix + "UBOUND_MIN_SPLIT",
		     defaultUBMSW, uBMSWHelp, TRUE);
   tddtOptions.minSplitWeightPercent =
      get_option_real(prefix + "MIN_SPLIT_WEIGHT",
		      defaultMSWP, mSWPHelp, TRUE);
   tddtOptions.debug =
      get_option_bool(prefix + "DEBUG",
		      defaultDebug, debugHelp, TRUE);
   tddtOptions.unknownEdges =
      get_option_bool(prefix + "UNKNOWN_EDGES",
		      defaultUnknownEdges, unknownEdgesHelp, TRUE);
   tddtOptions.splitBy = 
      get_option_enum(prefix + "SPLIT_BY",
		      splitByEnum, defaultSplitBy, splitByHelp, TRUE);
}



/***************************************************************************
  Description : Constructor, destructor
  Comments    : 
***************************************************************************/
TDDTInducer::TDDTInducer(const MString& dscr, CGraph* aCgraph) :
   CtrInducer(dscr)
{
   cgraph = aCgraph; // save this until we actually construct the tree.
   decisionTreeCat = NULL;

   tddtOptions.maxLevel = defaultMaxLevel;
   tddtOptions.lowerBoundMinSplitWeight = defaultLBMSW;
   tddtOptions.upperBoundMinSplitWeight = defaultUBMSW;
   tddtOptions.minSplitWeightPercent = defaultMSWP;
   tddtOptions.debug = defaultDebug;
   tddtOptions.unknownEdges = defaultUnknownEdges;
   tddtOptions.splitBy = defaultSplitBy;
}
   

/*****************************************************************************
  Description : Copy constructor.
  Comments    :
*****************************************************************************/
TDDTInducer::TDDTInducer(const TDDTInducer& source, CtorDummy, CGraph* aCgraph)
   : CtrInducer(source, ctorDummy)
{
   cgraph = aCgraph;
   decisionTreeCat = NULL;   
   copy_options(source);
}
      

/***************************************************************************
  Description : Perform CtrInducer::OK, decisionTree::OK() if it's not NULL.
  Comments    :
***************************************************************************/
void TDDTInducer::OK(int /*level*/) const
{
   CtrInducer::OK();
   if (decisionTreeCat)
      decisionTreeCat->OK();
}



/***************************************************************************
  Description : Induce a decision tree.
  Comments    : Must be called after read_data().
                Erases any previously created DT.
***************************************************************************/
void TDDTInducer::train()
{
   has_data();
   DBG(OK());
   delete decisionTreeCat; // remove any existing tree categorizer.
  
   // The decisionTree either creates a new graph, or gets ours.
   DecisionTree *DT = (cgraph == NULL) ? new DecisionTree
                                       : new DecisionTree(*cgraph);
                    
   // induce_DT returns the root.
   LOG(4, "TDDTInducer:: training with bag\n"
       << instance_bag() << endl);
   Category majority = TS->majority_category();
   DT->set_root(induce_decision_tree(DT->get_graph(), majority));
   decisionTreeCat = new DTCategorizer(DT, description(),TS->num_categories());
   LOG(1, "Tree has " << num_nodes() << " nodes, and "
          << num_leaves() << " leaves." << endl);
}



/***************************************************************************
  Description : Return TRUE iff the class has a valid decisionTree
                  categorizer.
  Comments    :
***************************************************************************/
Bool TDDTInducer::was_trained(Bool fatalOnFalse) const
{
   if (fatalOnFalse && decisionTreeCat == NULL)
      err << "TDDTInducer::was_trained: No decision tree categorizer. "
             " Call train() to create categorizer" << fatal_error;
   return decisionTreeCat != NULL;
}


/***************************************************************************
  Description : Returns the categorizer that the Inducer has generated.
  Comments    :
***************************************************************************/
const Categorizer& TDDTInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *decisionTreeCat;
}

/***************************************************************************
  Description : Returns the RDGCategorizer that the Inducer has generated.
  Comments    : Returns as an RDGCategorizer to avoid casting.
***************************************************************************/
const RDGCategorizer& TDDTInducer::get_rdg_categorizer() const
{
   was_trained(TRUE);
   return *decisionTreeCat;
}

/***************************************************************************
  Description : Returns the RDGCategorizer that the Inducer has generated.
  Comments    : Non-const version.
***************************************************************************/
RDGCategorizer& TDDTInducer::get_rdg_categorizer()
{
   was_trained(TRUE);
   return *decisionTreeCat;
}


/***************************************************************************
  Description : Create a string to name the subinducer.
  Comments    : We just append some basic info.  
***************************************************************************/
MString TDDTInducer::name_sub_inducer(MString catDescr, Category catNum) const
{
   return description() + " Cat=" + catDescr + " child =" +
          MString(catNum,0);
}


/***************************************************************************
  Description : Create a leaf categorizer (has no children).
                We currently create a ConstCategorizer with a 
                  description and the majority category.
  Comments    : Note that the augCategory will contain the correct
                  string, but the description will contain more
                  information which may help when displaying the
                  graph.
                  The augCategory string must be the same for
                  CatTestResult to work properly (it compares
                  the actual string for debugging purposes).
***************************************************************************/
Categorizer* TDDTInducer::create_leaf_categorizer(int numInstances,
						Category preferredMajority) const
{
   // Find majority category.  If there is only one instance, then
   //   it is obviously the majority.
   Category majority = TS->majority_category(preferredMajority);
   MString categoryString = TS->get_schema().
				       category_to_label_string(majority);
   MString descr(categoryString);
   if (get_debug())
      descr += " (#=" + MString(numInstances,0) + ")";
   AugCategory augMajority(majority, categoryString);
   LOG(3, "TDDTInducer::create_leaf_categorizer: leaf is:" <<
       augMajority << endl); 
   Categorizer* leafCat =  new ConstCategorizer(descr, augMajority);
   ASSERT(leafCat);
   leafCat->build_distr(TS_with_counters());
   return leafCat;
}



// Connects given two nodes on the given graph with given edge label and value
static void connect(CatGraph& catGraph, NodePtr from, NodePtr to,
		    Category edgeVal, const MString& edgeName)
{
   AugCategory* edge = new AugCategory(edgeVal, edgeName);
   catGraph.connect(from, to, edge);
   ASSERT(edge == NULL); // connect() gets ownership

}

/***************************************************************************
  Description : Induce a decision tree in the given graph.  Returns a
                  pointer to the root of the decision tree.
  Comments    :
***************************************************************************/
NodePtr TDDTInducer::induce_decision_tree(CGraph& aCgraph, Category preferredMajority) const
{
   has_data(TRUE);
   if (TS->no_instances())
      err << "TDDTInducer::induce_decision_tree: zero instances"
          << fatal_error;
   
   // Create a decision tree object to allow building nodes in the CGraph.
   DecisionTree decisionTree(aCgraph);

   LOG(4, "TDDTInducer::induce_decision_tree:  training set =" << endl
       << *TS << endl);

   Array<MString>* catNames;
   Categorizer* splitCat = best_split(catNames);
   Categorizer* rootCat = splitCat ? splitCat :
                create_leaf_categorizer(TS->num_instances(), preferredMajority);
   decisionTree.set_root(decisionTree.create_node(rootCat));
   if (splitCat) {
      CtrBagPtrArray& bags = *(TS_with_counters().ctr_split(*splitCat));
      int splitAttr = get_split_attribute(splitCat);
      CGraph& treeGraph = decisionTree.get_graph();
      for (Category cat = bags.low(); cat <= bags.high(); cat++) {
         NodePtr child;
         if (bags[cat]->no_instances()) {// No instances with this
	                                          // value.  Make it a
	                                          // leaf (majority), unless
	                                          // category unknown
	    if (get_unknown_edges() || cat != UNKNOWN_CATEGORY_VAL) { 
               LOG(3, "TDDTInducer::induce_decision_tree: Category: " << cat 
                   << " empty.  Assigning majority" << endl);
 	       Categorizer* constCat = create_leaf_categorizer(0,preferredMajority);
	       child = decisionTree.create_node(constCat);
	       initialize_node_info(treeGraph, child, splitAttr);
	       connect(decisionTree, decisionTree.get_root(), child,
		       cat, (*catNames)[cat]);
	    }
	 } else { // Solve the problem recursively.
            LOG(3, "TDDTInducer::induce_decision_tree: Recursive call"<<endl);
            TDDTInducer* childInducer = create_subinducer(
                   name_sub_inducer(splitCat->description(),cat), aCgraph);
            childInducer->assign_data(bags[cat]);
            Category myMajority = TS->majority_category(preferredMajority);
            child = childInducer->induce_decision_tree(aCgraph, myMajority);
	    set_node_info(treeGraph, child, splitAttr, childInducer, cat);
            delete childInducer;
	    connect(decisionTree, decisionTree.get_root(), child,
		    cat, (*catNames)[cat]);
         }
      }
      delete &bags;
   }
   delete catNames; DBG(catNames = NULL);
   return decisionTree.get_root();
}
           

/***************************************************************************
  Description : Returns the number of nodes (categorizers) in the
                   decision tree, and number of leaves.
  Comments    :
***************************************************************************/
int TDDTInducer::num_nodes() const
{
   was_trained(TRUE);
   return decisionTreeCat->num_nodes();
}

int TDDTInducer::num_leaves() const
{
   was_trained(TRUE);
   return decisionTreeCat->num_leaves();
}


   
/***************************************************************************
  Description : get/set for Options.
  Comments    :
***************************************************************************/


TDDTInducer::~TDDTInducer()
{
   DBG(OK());
   delete decisionTreeCat;
}

void TDDTInducer::set_c45_options()
{
   set_lower_bound_min_split_weight(2);
   set_upper_bound_min_split_weight(25);
   set_min_split_weight_percent(0.1);
}

void TDDTInducer::copy_options(const TDDTInducer& inducer) 
{
   set_log_options(inducer.get_log_options());
   set_max_level(inducer.get_max_level());
   set_debug(inducer.get_debug());
   set_debug(inducer.get_debug());
   set_lower_bound_min_split_weight(
      inducer.get_lower_bound_min_split_weight());
   set_upper_bound_min_split_weight(
      inducer.get_upper_bound_min_split_weight());
   set_min_split_weight_percent(inducer.get_min_split_weight_percent());
   set_unknown_edges(inducer.get_unknown_edges());
}
