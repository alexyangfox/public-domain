// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Abstract Base Class for building oblivious decision
                   trees and graphs.
                 ODGInducer calls best_split() to find the best split
                   for ALL nodes at the current level.  The data
                   in each node are split according to the categorizer
                   returned by best_split(), and the process continues
                   until best_split returns NULL.
                 After the Oblivious decision graph is built, we
                   have a merge phase to try and merge isomorphic
                   subtrees.  In the merging phase, subtrees are merged
                   if the number of errors in the merged graph is not
                   much larger than in the original graph.
                   See isocat.c for more information.
		 The following options are used to set the behavior of
		   ODInducer:
		   
		   DEBUG option specifies whether not to display debug
		     information in the decision graph. The debug information
		     includes the number of instances and the conditional 
		     entropy in each node. Default is FALSE;

		   UNKNOWN_EDGES specifies whether or not to allow outgoing
		     edges labeled UNKNOWN from the nodes. Default is FALSE;

		   POST_PROC specifies how we do the post processing. <none>
		     means we don't do anything after the tree is
		     built. <set_unknown> means unknown nodes will be set to
		     the majority of their parents. <merge> means approximately
		     isomorphic trees will be merged to graphs" Default is
		     <merge>. 

		   SPLIT_PURE specifies whether or not to continue splitting
		     pure nodes. Default is TRUE.

		   GROW_CONFLICT_RATIO chooses the average conflict ratio
		     allowed in a level of the graph. If the average conflict
		     ratio at a level is less than this option, stop
		     splitting. 

		   ISO_CONFLICT_RATIO chooses the maximum conflict ratio which
		     is the internal conflicting instances increased by
		     merging the graph over the total number of instances in
		     both original graphs. Default is 0.

		   MIN_SPLIT chooses the minimum instances which at least one
		     of the left and right child nodes of a level must have
		     more than in a split. Default is 1.

		   CV_MERGE This option specifies whether or not to merge
		     graph while doing the cross validation on the depth of
		     the graph. Default is FALSE.

		   CV_PRUNE This option specifies whether or not to prune the
		     tree using cross validation. The depth with best
		     estimated accuracy will be picked up to reconstruct the
		     tree. Default is TRUE.

		   GROW_SPLIT_PURE specifies whether or not to split pure
		     nodes while building the first temporary tree. Default is
		     FALSE. 

  Assumptions  : best_split() must return NULL when best-split categorizer can
                   not be found. If not so, induce_oblivious_decision_graph
		   will loop infinitily.
  Comments     : Instead of computing the cross entropy of a child node, the
		   derived classes of ODGInducer should calculate the cross
		   entropy of ALL splitted nodes for best-split categorizer.
		 induce_oblivious_decision_graph(): For each level, find the
		   non-empty bags and corresponding nodes. We call best_split
		   which is implemented by the derived class to find the
		   best-split categorizer. The bags in the current level are
		   then splitted into next level according the the best-split
		   cateogrizer. Then we connect the bags in next level to the
		   current level.
		 The above mechanism may have problem: Since we don't know the
		   best-split categorizer for the next level, how can we
		   create node without categorizer? To solve this problem,
		   every node in the next level is set to BadCategorizer. Once
		   the best-split categorizer is determined, we set the
		   categorizer of every node in CURRENT level to the best one.
  Complexity   : Building the oblivious decision tree (train()) is
                   O(sum_{i=1}^{max-level} best-split-time(i) + n)
                   where n is the number of instances in the training set.
                   The merge phase is
                   O(sum_{i=1}^{max-level} m(i)^2*isocheck)
                   where m is the number of nodes at level i and
                   isocheck is the time to check isomorphism.
                   The time to check isomorphism could be
                   the number of nodes in the two subtrees, which
                   in the worst case is O(num-levels-below*num-instances),
                   but is much smaller in practice.  The complexity
                   is largest about half way down the tree in the worst
                   case, and equals
                        O(min(num-inst, 2^(max-level/2))^2*
                              max-level*num-instances),
                   which is quadratic in the number of instances
                   and linear in the number of levels.
  Enhancements : 
  History      : Chia-Hsin Li                                        10/06/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <LEDA/graph.h>
#include <Array.h>
#include <DynamicArray.h>
#include <AugCategory.h>
#include <ConstCat.h>
#include <BadCat.h>
#include <CtrBag.h>
#include <BagSet.h>
#include <ODGInducer.h>
#include <isocat.h>
#include <entropy.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: ODGInducer.c,v $ $Revision: 1.3 $")

// Debug information option
const MString debugHelp = "This option specifies whether not to display debug "
  "information in the decision graph. The debug information includes the "
  "number of instances and the conditional entropy in each node.";
const Bool defaultDebug = FALSE;

// Unknown edges information option
const MString unknownEdgesHelp = "This option specifies whether or not to "
  "allow outgoing UNKNOWN edges from each node. ";
const Bool defaultUnknownEdges = FALSE;

// Post Process option
const MEnum postProcEnum = MEnum("none", none)
  << MEnum("set_unknown", set_unknown) << MEnum("merge", merge);
const MString postProcHelp = "This option specifies how we do the post "
  "processing. none means we don't do anything after the tree is built. "
  "set_unknown means unknown nodes will be set to the majority of their "
  "parents. merge means approximately isomorphic trees will be merged to "
  "graphs";
const PostProcType defaultPostProc = merge;

// Split pure nodes option
const MString splitPureHelp = "This option specifies whether or not to "
  "continue splitting pure nodes.";
Bool defaultSplitPure = TRUE;

// Grow Conflict Ratio option
const MString growConflictRatioHelp = "This option chooses the average "
  "conflict ratio allowed in a level of the graph. If the average conflict "
  "ratio at a level is less than this option, stop splitting.";
const Real defaultGrowConflictRatio = 0.001;

// Isomorphism Conflict Ratio option
const MString isoConflictRatioHelp = "This options chooses the maximum "
  "conflict ratio which is internal conflicting instances increased by "
  "merging the graph over the total number of instances in both original "
  "graphs.";

// Min split option.
const MString minSplitHelp = "This option chooses the minimum instances "
  "which at least one of the left and right child nodes of a level must have  "
  "more than in a split.";
const int defaultMinSplit = 1;

// Cross Validation merge option
const MString cVMergeHelp = "This option specifies whether or not to merge "
  "graph while doing the cross validation on the depth of the graph";
const Bool defaultCVMerge = FALSE;

// Cross Validation prunning on tree's depth.
const MString cvPruneHelp =
  "This option specifies whether or not to prune the "
  "tree using cross validation. The depth with best estimated accuracy will "
  "be picked up to reconstruct the tree. ";
const Bool defaultCVPrune = TRUE;

// Split pure or not while growing the tree at the first phase.
const MString growSplitPureHelp = "This option specifies whether or not to "
  "split pure nodes while building the first temporary tree. "; 
const Bool defaultGrowSplitPure = FALSE;



/***************************************************************************
  Description : Returns TRUE if the threshold of attrNum has been used.
  Comments    :
***************************************************************************/
Bool ODGInducer::used_threshold(int attrNum, Real threshold)
{
   int arraySize=(*usedAttributes)[attrNum].thresholdUsed->size();
   for (int i = 0; i < arraySize; i++)
      if ((*(*usedAttributes)[attrNum].thresholdUsed)[i] == threshold)
	 return TRUE;
   return FALSE;
}

/***************************************************************************
  Description : Perform CtrInducer::OK, RDGCat::OK ()if it's not NULL.
  Comments    : If the isomorphic nodes in the graph are merged, the
		   number of leaves should be less or equal than the number of
		   label values.
		Remove the @@ comment if we can merge UNKNOWN nodes with its
		   siblings.
***************************************************************************/
void ODGInducer::OK(int /*level*/) const
{
   // After merging, the number of leaves should be equal to the number of
   // classes of the label.
/*@@
  if (get_post_proc() == merge)
      if (decisionGraphCat)
	 ASSERT(!has_data(FALSE) || decisionGraphCat->num_leaves()	
		<= TS_with_counters().get_schema().num_label_values());
*/ 
   CtrInducer::OK();
   if (decisionGraphCat)
      decisionGraphCat->OK();
}


/***************************************************************************
  Description : Constructor, destructor
  Comments    : 
***************************************************************************/
ODGInducer::ODGInducer(const MString& dscr, CGraph* aCgraph) :
   CtrInducer(dscr)
{
   cgraph = aCgraph; // save this until we actually construct the graph
   decisionGraphCat = NULL;
   usedAttributes=NULL;
   splitInfoArray=NULL;

   // Set all the options with reasonable values.
   odgOptions.debug = defaultDebug;
   odgOptions.unknownEdges = defaultUnknownEdges;
   odgOptions.postProc = defaultPostProc;
   odgOptions.splitPure = defaultSplitPure;
   odgOptions.growConflictRatio = defaultGrowConflictRatio;
   odgOptions.minSplit = defaultMinSplit;
   odgOptions.cVMerge = defaultCVMerge;
   odgOptions.cvPrune = defaultCVPrune;
   odgOptions.growSplitPure = defaultGrowSplitPure; 
}

   
ODGInducer::~ODGInducer()
{
   DBG(OK());
   delete decisionGraphCat;
   delete splitInfoArray;
   delete usedAttributes;
}

/***************************************************************************
  Description : Induce a decision graph.
  Comments    : Must be called after read_data().
                Erases any previously created DT.
***************************************************************************/
void ODGInducer::train()
{
   has_data();
   DBG(OK());
   delete decisionGraphCat; // remove any existing graph categorizer.
   decisionGraphCat = NULL;
  
   // The RootedCatGraph either creates a new graph, or gets ours.
   RootedCatGraph *RG = (cgraph == NULL) ? new RootedCatGraph
                                       : new RootedCatGraph(*cgraph);
                    
   LOG(4, "Training with bag\n"
       << instance_bag() << endl);
   RG->set_root(induce_oblivious_decision_graph(RG->get_graph()));
   decisionGraphCat =
      new RDGCategorizer(RG, description(),TS->num_categories());
}

/***************************************************************************
  Description : Return TRUE iff the class has a valid decision graph
                  categorizer.
  Comments    :
***************************************************************************/
Bool ODGInducer::was_trained(Bool fatalOnFalse) const
{
   if (fatalOnFalse && decisionGraphCat == NULL)
      err << "No decision graph categorizer. "
             " Call train() to create categorizer" << fatal_error;
   return decisionGraphCat != NULL;
}

/***************************************************************************
  Description : Returns the categorizer that the Inducer has generated.
  Comments    :
***************************************************************************/
const Categorizer& ODGInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *decisionGraphCat;
}

/***************************************************************************
  Description : Create a leaf categorizer (has no children).
                We currently create a ConstCategorizer with a 
                  description and the majority category.
		Creates a ConstCategorizer with UNKOWN_CATEGORY if there is
		no instance in the bag.
  Comments    : Note that the augCategory will contain the correct
                  string, but the description will contain more
                  information which may help when displaying the
                  graph.
                The augCategory string must be the same for
                  CatTestResult to work properly (it compares
                  the actual string for debugging purposes).
***************************************************************************/
Categorizer* ODGInducer::create_leaf_categorizer(const CtrInstanceBag *bag,
						 int numInstances) const 
{
   // Find majority category.  
   Category majority = numInstances ? bag->majority_category() :
      UNKNOWN_CATEGORY_VAL;
   
   MString categoryString = bag->get_schema().
      category_to_label_string(majority);
   MString descr(categoryString);
   AugCategory augMajority(majority, categoryString);
   Categorizer *leafCat =  new ConstCategorizer(descr, augMajority);
   leafCat->build_distr(*bag);
   LOG(4, "leaf is:" << augMajority << "(Distr:"
       << leafCat->get_distr() << ")" << endl);
   return leafCat;
}

/***************************************************************************
  Description : Connects given two nodes on the given graph with given edge
                  label and value.
  Comments    :
***************************************************************************/
static void connect(CatGraph& catGraph, NodePtr from, NodePtr to,
		    const Category edgeVal, const MStringRC& edgeName)
{
   AugCategory* edge = new AugCategory(edgeVal, edgeName);
   catGraph.connect(from, to, edge);
   ASSERT(edge == NULL); // connect() gets ownership
}

/***************************************************************************
  Description : Set UNKNOWN nodes to the majority of its parent.
  Comments    : This function is called when the POST_PROC option is set
                   to set_unknown.
		Instead of a iterative function, we changed it to a recursive
		   function because we may want some different process on
		   unknown nodes.
***************************************************************************/
static void set_unknown_recursive(NodePtr root, CGraph& G,
				  const CtrInstanceBag *TS)
{
   const Categorizer &cat = G[root]->get_categorizer();
   NodePtr iterNode;
   
   Category majority = cat.majority_category();
   forall_adj_nodes(iterNode, root) {
      if (G.outdeg(iterNode) == 0) {// leaf node
	 ASSERT(G[iterNode]->get_categorizer().class_id() ==
		CLASS_CONST_CATEGORIZER);
	 ConstCategorizer *childCat =
	    (ConstCategorizer*) &G[iterNode]->get_categorizer();

	 if (childCat->get_category() == UNKNOWN_CATEGORY_VAL) {
	    // Set the unknown node to the majority of its parent which is 'n'.
	    MString categoryString = TS->get_schema().
	       category_to_label_string(majority);
	    MString descr(categoryString);
	    
	    AugCategory augMajority(majority, categoryString);
	    Categorizer *tmpCat =  new ConstCategorizer(descr, augMajority);
	    tmpCat->set_distr(cat.get_distr());
	    G[iterNode]->set_categorizer(tmpCat);
	 }
      }
      else // not a leaf
	 set_unknown_recursive(iterNode, G, TS);
   }
}

static
void set_unknown_nodes(RootedCatGraph& decisionGraph,
		       const CtrInstanceBag* TS) 
{
   CGraph& G = decisionGraph.get_graph();
   NodePtr rootNode = decisionGraph.get_root();

   set_unknown_recursive(rootNode, G, TS);
}

/***************************************************************************
  Description : Find the majority of node n in graph G.
  Comments    :
***************************************************************************/
static NodePtr find_majority(CGraph& G, NodePtr n)
{
   NodePtr majority = NULL;
   int maxInstNum = -1;
   NodePtr iterNode;
   forall_adj_nodes(iterNode, n) 
     if (G[iterNode]->get_categorizer().num_instances() > maxInstNum) {
       majority = iterNode;
       maxInstNum = G[iterNode]->get_categorizer().num_instances();
     }
   ASSERT(majority);
   return majority;
}

/***************************************************************************
  Description : Merges the unknown nodes in the graph, decisionGraph, to
                   their siblings.
  Comments    :
***************************************************************************/
static void merge_unknown_nodes(RootedCatGraph& decisionGraph)
{
  CGraph& G = decisionGraph.get_graph();
  NodePtr rootNode = decisionGraph.get_root();
  NodePtr iterNode;
  forall_nodes(iterNode, G) {
    if (G.outdeg(iterNode) > 0) {
      NodePtr majorityNode = find_majority(G, iterNode);
      EdgePtr iterEdge = G.first_adj_edge(iterNode);
      while (iterEdge) {
	NodePtr target = G.target(iterEdge);
	EdgePtr tmpEdge = iterEdge;
	iterEdge = G.adj_succ(iterEdge);
	if (is_unknown(G[target]->get_categorizer())) {
	  G.new_edge(iterNode, majorityNode, G[tmpEdge]);
	  G.del_edge(tmpEdge);
	  G.del_node(target);
	}
      }
    }
  }
}


/***************************************************************************
  Description : Helper function of induce_oblivious_decision_graph().
                It adds the entropy description to the categorizer's
		  description.
  Comments    :	We can not put this in best_split since we only call best_split
	          once per level instead of calling once per bag. Therefore, we
	          have to modify the categorizer's description after the
	          categorizer is initialized.
		Thus, the function is called in ODGInducer, not in its derived
		  class.  
***************************************************************************/
static void add_debug_info(NodePtr n, Categorizer &cat,
			   const CtrInstanceBag& bag,
			   Real prevMinCondEntropy,
			   Real minCondEntropy)
{
   LogOptions opt;
   opt.set_log_level(0);
   Real bagEntropy = entropy(opt, bag);
   MString catDescr = cat.description();
   catDescr.prepend("[" + MString(index(n),0) + "]");
   catDescr += "(/E=" +  MString(bagEntropy,3);
   catDescr += "/LMI=" + MString(prevMinCondEntropy - minCondEntropy, 3) + ")";
   cat.set_description(catDescr);
}

/***************************************************************************
  Description : Induce a decision graph in the given graph.  Returns a
                  pointer to the root of the decision graph.
  Comments    : Note the best_split function. After returning from the
                  funtion, the second argument must return a allocated data.
		The graph is built from top down. The following is the psuedo
		   code of induce_decision_graph.

		     Initialize the data structures of level 0;
		     Set current level to level 0;
                     while (split_cat !=NULL) {
		       Split the bags in current level to next level.
		       Connect the nodes in next level to current
		         level. Categorizer in each node is badCategorizer.
		       Set current level to next level.
		       Find the best split categorizer of current level.
		       Replace the badCategorizer with bestSplit categorizer.
		     }
***************************************************************************/
NodePtr ODGInducer::induce_oblivious_decision_graph(CGraph& aCgraph)
{
   has_data(TRUE);
   if (TS->no_instances())
      err << "Zero instances"
          << fatal_error;
   
   // Create a decision graph object to allow building nodes in the CGraph.
   RootedCatGraph decisionGraph(aCgraph);

   DynamicArray<NodePtr> currentNodeSet(0);
   // currentLevel(0) is const.  All others are really non-const and we own
   // them, doing the appropriate cast for the delete
   DynamicArray<const CtrInstanceBag*> currentLevel(0);

   currentLevel[0] = (CtrInstanceBag *)&TS_with_counters();
      
   Array<MString>* catNames = NULL;
   Array<MString>* previousCatNames = NULL;
   // This is for debug information at current stage.
   Real minCondEntropy;
   int levelCount = 0;
   LOG(2, "-------- Level " << levelCount << " --------" << endl);
   Real previousEntropy = entropy(get_log_options(),TS_with_counters());
   Categorizer* splitCat = best_split(*usedAttributes, *splitInfoArray,
				      currentLevel, catNames, minCondEntropy,
				      previousEntropy);
   previousEntropy = minCondEntropy;

   Real currentMinCondEntropy = entropy(get_log_options(), TS_with_counters());

   Categorizer* rootCat;
   if (splitCat) {
      rootCat = splitCat->copy();
      // Add debug information
      currentMinCondEntropy = minCondEntropy;
      rootCat->build_distr(TS_with_counters());
   }
   else {
      rootCat = create_leaf_categorizer(&TS_with_counters(),
					TS->num_instances());
      minCondEntropy = -1;
   }

   NodePtr tmpNode = decisionGraph.create_node(rootCat);
   decisionGraph.set_root(tmpNode);

   if (splitCat && get_debug()) 
      add_debug_info(tmpNode, aCgraph[tmpNode]->get_categorizer(),
		     TS_with_counters(),
		     currentMinCondEntropy, minCondEntropy);

   // If can not split on root, return a leaf node.
   if (!splitCat)
      return decisionGraph.get_root();
   // Initially, there is only one InstanceBag.
   currentNodeSet[0] = decisionGraph.get_root();
   int nodeNumber = 1;
   do {
      LOG(2, "Nodes:" << nodeNumber << " Non-empty bags:"
	  << currentNodeSet.size() << "." << endl);
      nodeNumber = 0;
      int countBag = 0;
      // The reason we use dynamic array here is because calculating the size
      // of the array is more expensive. 
      DynamicArray<NodePtr> newNodeSet(0);
      DynamicArray<const CtrInstanceBag*> nextLevel(0);

      // Split currentLevel to nextLevel.
      // Connect the next level to current level.
      // Assign BagCategorizer to nonempty-bag node. Replace them after
      // bestSplit has been obtained.
      DBG_DECLARE(int instanceCount = 0;) // For assertion.
      for (int i=0; i < currentLevel.size(); i++) {
	 ASSERT(currentLevel[i]);
	 CtrBagPtrArray *nextLevelBags = 
	    (currentLevel[i]->ctr_split(*splitCat));
	 nodeNumber += nextLevelBags->size();
	 // Don't create unknown edges if unknownEdges is FALSE and it's a
	 //   empty bag.
	 int lowerBound = nextLevelBags->low()
	    + (odgOptions.unknownEdges == FALSE &&
	       (*nextLevelBags)[nextLevelBags->low()]->no_instances());
	 if (odgOptions.unknownEdges==FALSE &&
	     !(*nextLevelBags)[nextLevelBags->low()]->no_instances())
	    err << "UNKNOWN_EDGES set to FALSE but unknown attribute labels "
		<< "found." << fatal_error;
	 for (Category cat = lowerBound; cat <= nextLevelBags->high(); cat++) {
	    Bool splitPureNode = get_split_pure() ? FALSE :
	       ((*nextLevelBags)[cat]->counters().label_num_vals() == 1);
	    if ((*nextLevelBags)[cat]->no_instances() || splitPureNode) {
	       // Create a leaf node. Connect it(next level) to current
	       // level. Since we don't want to split pure or empty bags,
	       // these bags are not copied to nextLevel.
	       DBG(instanceCount += (*nextLevelBags)[cat]->num_instances());
	       Categorizer* leafCat =
		  create_leaf_categorizer((*nextLevelBags)[cat],
				  (*nextLevelBags)[cat]->num_instances());
	       leafCat->build_distr(*(*nextLevelBags)[cat]);
	       NodePtr tmpNode = decisionGraph.create_node(leafCat);
	       AugCategory edgeAug(cat, (*catNames)[cat]);
	       connect(decisionGraph,
		       currentNodeSet[i], tmpNode,
		       edgeAug.num(), edgeAug.description());
	    } else {
	       // Create a BadCategorizer node.
	       // Connect the node (next level) to current level.
	       DBG(instanceCount += (*nextLevelBags)[cat]->num_instances());
	       Categorizer *badCat = &badCategorizer;
	       nextLevel[countBag] = (*nextLevelBags)[cat];
	       // grab the ownership from nextLevelBags
	       (*nextLevelBags)[cat] = NULL;
	       newNodeSet[countBag] = decisionGraph.create_node(badCat);
	       AugCategory *tmpAug = new AugCategory(cat, (*catNames)[cat]);
	       connect(decisionGraph,
		       currentNodeSet[i], newNodeSet[countBag],
		       tmpAug->num(), tmpAug->description());
	       delete tmpAug;
	       countBag++;
	    }
	 }
	 delete nextLevelBags;
      }
      DBG(LOG(3, "Total of " << instanceCount << " instances." << endl));
      ASSERT(nextLevel.size() == countBag);
      ASSERT(newNodeSet.size() == nextLevel.size());
      DBG(
      if (get_split_pure() && odgOptions.unknownEdges)
	 ASSERT(instanceCount == TS_with_counters().num_instances()));

      // We can not delete currentLevel[i] if it is rootLevel because
      // currentLevel[0] is TS.
      if (levelCount !=0)
	 for (i = 0; i < currentLevel.size(); i++) {
            // Cast constness away.  Accept for first level, we really
	    //   do own these.
	    delete (CtrInstanceBag*)currentLevel[i];
	    currentLevel[i] = NULL;
	 }

      levelCount++;
      LOG(2, "-------- Level " << levelCount << " --------" << endl);   
      
      currentLevel = nextLevel;
      currentNodeSet = newNodeSet;
      ASSERT(nextLevel.size() == newNodeSet.size());
      delete splitCat;
      delete previousCatNames;
      previousCatNames = catNames;
      catNames = NULL;

      splitCat = best_split(*usedAttributes, *splitInfoArray,
			    currentLevel, catNames,
			    minCondEntropy,previousEntropy); 
      previousEntropy = minCondEntropy;
      // Once we know the best split, we should replace the BagCategorizer to
      // splitCat.
      for (int nodeCount = 0; nodeCount < currentNodeSet.size(); nodeCount++){
	 Categorizer *tmpCat;
	 if (splitCat){
	    tmpCat = splitCat->copy();
	    // Add the debug information.
	    if (get_debug())
	       add_debug_info(currentNodeSet[nodeCount], *tmpCat,
				       *nextLevel[nodeCount],
				       currentMinCondEntropy, minCondEntropy);
	    currentMinCondEntropy = minCondEntropy;
	    tmpCat->build_distr(*currentLevel[nodeCount]);
	    LOG(3, "Bag " << nodeCount << " has " <<
		    currentLevel[nodeCount]->num_instances() << " instances"
		    << " with distribution (" << tmpCat->get_distr() << ")"
		    << endl); 
	 }
	 else
	    tmpCat = create_leaf_categorizer(currentLevel[nodeCount],
				    currentLevel[nodeCount]->num_instances());
	 CGraph& cG = decisionGraph.get_graph();
	 cG[currentNodeSet[nodeCount]]->set_categorizer(tmpCat);
      }
   } while (splitCat);
   LOG(2, "Node:" << nodeNumber << " Non-empty bag:"
       << currentNodeSet.size() << "." << endl);   
   if (levelCount != 0)
      for (int i = 0; i < currentLevel.size(); i++) {
	 delete (CtrInstanceBag*)currentLevel[i];
	 currentLevel[i] =  NULL;
      }
 
   delete previousCatNames;
   delete catNames;

   switch (odgOptions.postProc) {
      case merge: 
	 merge_graph(get_log_options(), TS->get_schema().nominal_label_info(),
		     decisionGraph, decisionGraph.get_root());
//	 set_unknown_nodes(decisionGraph, &TS_with_counters());
	 merge_unknown_nodes(decisionGraph);
	 prune_graph(get_log_options(), decisionGraph);
	 break;
      case set_unknown:
	 set_unknown_nodes(decisionGraph, &TS_with_counters());
	 break;
      case none:
	 break;
   }
   return decisionGraph.get_root();
}

/***************************************************************************
  Description : Returns the number of nodes (categorizers) in the
                   decision graph, and number of leaves.
  Comments    :
***************************************************************************/
int ODGInducer::num_nodes() const
{
   was_trained(TRUE);
   return decisionGraphCat->num_nodes();
}

int ODGInducer::num_leaves() const
{
   was_trained(TRUE);
   return decisionGraphCat->num_leaves();
}

/***************************************************************************
  Description : Initialize arguments to their default values.
  Comments    : The compiler has problem with the following codes:

                   splitPure =  (postProc == merge) ? 
		      get_option_bool(prefix + "SPLIT_PURE",
		                      TRUE, splitPureHelp, TRUE):
		      get_option_bool(prefix + "SPLIT_PURE",
				      FALSE, splitPureHelp, TRUE);  

                 The compiler destructs "SPLIT_PURE twice.
***************************************************************************/
void ODGInducer::set_user_options(const MString& prefix )
{
   decisionGraphCat = NULL;

   odgOptions.debug =
      get_option_bool(prefix + "DEBUG",
		      odgOptions.debug, debugHelp, TRUE);
   odgOptions.unknownEdges =
      get_option_bool(prefix + "UNKNOWN_EDGES",
		      odgOptions.unknownEdges, unknownEdgesHelp, TRUE);
   odgOptions.postProc =
      get_option_enum(prefix + "POST_PROC", postProcEnum,
		      odgOptions.postProc, postProcHelp, TRUE);

   if (odgOptions.postProc == merge)
      odgOptions.splitPure = get_option_bool(prefix + "SPLIT_PURE",
				  TRUE, splitPureHelp, TRUE);
   else 
      odgOptions.splitPure = get_option_bool(prefix + "SPLIT_PURE",
				  FALSE, splitPureHelp, TRUE);
					     
   odgOptions.growConflictRatio =
      get_option_real_range(prefix + "GROW_CONF_RATIO",
				       odgOptions.growConflictRatio, 0, 1,
				       growConflictRatioHelp, TRUE);

   odgOptions.minSplit = 
      get_option_int_range(prefix + "MIN_SPLIT",
		   odgOptions.minSplit, 1, INT_MAX, minSplitHelp, TRUE);
   
   odgOptions.cVMerge =
      get_option_bool(prefix + "CV_MERGE",
		      odgOptions.cVMerge, cVMergeHelp, TRUE);

   odgOptions.cvPrune =
      get_option_bool(prefix + "CV_PRUNE",
		      odgOptions.cvPrune, cvPruneHelp, TRUE);
   
   odgOptions.growSplitPure =
      get_option_bool(prefix + "GROW_SPLIT_PURE",
		      odgOptions.growSplitPure, growSplitPureHelp, TRUE);
}

/***************************************************************************
  Description : Initialize the used attributes array and the split information
                   array.
  Comments    :
***************************************************************************/
void ODGInducer::reset_used_attributes()
{
   const SchemaRC& schema = TS->get_schema();
   delete usedAttributes;
   usedAttributes = NULL;
   usedAttributes = new Array<UsedAttributes>(0, schema.num_attr());
   for (int i = 0; i < usedAttributes->size(); i++)
      (*usedAttributes)[i].nominalUsed = FALSE;
}

void ODGInducer::reset_split_info_array()
{
   delete splitInfoArray;
   splitInfoArray = NULL;
   splitInfoArray = new DynamicArray<SplitInfo>(0);
}

