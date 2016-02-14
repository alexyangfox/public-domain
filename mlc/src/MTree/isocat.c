// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The isomorphic_category functions verify the isomorphism
                   of two given acyclic directed graphics.
  Assumptions  : The given graphs must be acyclic. The label of edges
                 adjacent to the same node must be different.
  Comments     : We use merge_conflict to implement exact isomorphic. If the
		   conflicting instances after merging doesn't increase, we
		   can say that the trees are isomorphic unless there is a
		   tight. A tight is, for example, Yes(1,1) and
		   No(0,1). Merging doesn't increase the conflicting instances
		   but the nodes are not isomorphic.
		 approx_isomorphic() verifies the approximate isomorphism of 2
		   graphs. It calculate the difference between the sum of
		   internal conflict of both graphs and the internal conflict
		   if we merge both graphs. If the differenc / (the number of
		   instances in both graphs) is less than the matching ratio
		   allowed, they are considered isomorphic. It reads the
		   environment variable: ISO_CONFLICT_RATIO as the matching
		   ratio with default 0. 
		 We assume that a UNKNOWN node matches everything. While
		   merging 2 isomorphic graphs, we merge the UNKNOWN node with
		   another node at the corresponding position of another
		   subgraph.
		 In addition, if one or more of the subgraphs are UNKNOWN, 
		   we don't deal them as special cases; we calculate the
		   merging conflict of them. The matching conflict ratio will
		   turn out to be 0 as a result. Than they are isomorphic.
		 merge_sub_graph() merges 2 isomorphic graphs and assumes that
		   they are isomorphic. It merges graphs by looking at each
		   cooresponding node in the graphs and comparing their
		   edges. If one of the subgraph is a leaf node, merges them at
		   once. If not, call recursively until one of them is a
		   leaf. If the graphs are not isomorphic, some part of the
		   graph will not be merged. 
		 merge_graph() merges nodes for each level of the graph from
		   top down. For each level, it finds the best node pair with
		   minimum conflict ratio and maximum number of instances. If
		   such node pair found, it merges them and restarts the
		   selection loop until every node in the level are leaves.
		 After merging the graph, the number of leaves should ALWAYS
		   less or equal the the number of label values. For nodes in
		   the middle of the tree, the only possibility for leaves is
		   UNKNOWN nodes. However, they are merged by
		   merge_graph(). For the most bottom level, all the leaves
		   with the same label should be merged since merging nodes
		   with the same label doesn't increase the conflict
		   ratio. That is, approx_isomorphic always returns TRUE for
		   those leaves.
		 
  Complexity   : delete_unconnected_graph takes the time of the number of
		   nodes in the graph.
		 approx_isomorphic takes the time of sum(square of number of
		   edges) for each node.
		 merge_sub_graph takes the same time as approx_isomorphic.
		 For each level of the graph,  merge_graph takes cubic(number
		   of nodes in this level) * the time taken by
		   (approx_isomorphic + merge_sub_graph). 
		 Traversing the graph takes (num-nodes)
                 Comparing labels of edges adjacent to nodes takes sqare of
		   number of adjacent edges.
  Enhancements : Eleminate the ALPHA flag.
  History      : Chia-Hsin Li                                    11/15/94
                     Added operations for merging isomorphic nodes. Also added
		     functions which checks the approximate isomorphism of 2
		     graphs. The exact isomorphism is removed from this
		     file. The removed on can be find in version ?.
                 Chia-Hsin Li                                     9/12/94
                     Initial revision.
***************************************************************************/

#include <math.h>
#include <basics.h>
#include <Array.h>
#include <DynamicArray.h>
#include <errorUnless.h>
#include <MLCStream.h>
#include <ID3Inducer.h>
#include <isocat.h>
#include <CGraph.h>
#include <DblLinkList.h>
#include <ConstCat.h>
#include <GetOption.h>
#include <CatTestResult.h>

RCSID("MLC++, $RCSfile: isocat.c,v $ $Revision: 1.3 $")

/***************************************************************************
  Description : Check if n1 and n2 are not NULL.
  Comments    :
***************************************************************************/
#ifndef FAST
static void check_valid_node_ptr(NodePtr n1, NodePtr n2)
{
   if (!n1 || !n2)
      err << "NULL node pointer. " << fatal_error;
}
#endif

/***************************************************************************
  Description : Check if there are duplicate edges that are adjacent to the
                  node n in graph G. The function will returns TRUE while
		  there are duplicate edes and return false if all edges are
		  different from each other.
  Comments    :
***************************************************************************/

#ifndef FAST
static void check_duplicate_edges(node n,const CGraph &G)
{
   EdgePtr iterEdge;
   int outDegree=G.outdeg(n);
   Array<AugCategory*> edgeArray(outDegree);
   int edgeCount=0;
   int i,j;

   // if out degree is less than 1, the node is a leaf
   if (outDegree < 1) return;
   
   forall_adj_edges(iterEdge,n) {
      edgeArray[edgeCount]=G[iterEdge];
      edgeCount++;
   }

   for (i=0;i<outDegree;i++)
      for (j=i+1;j<outDegree;j++)
	 if ( *edgeArray[i] == *edgeArray[j] && i!=j)
	    err << "Duplicate sibling edges found in the first graph "
		<< "you passed" << fatal_error;
}
#endif

/***************************************************************************
  Description : Deletes all graphs that is not reachable from the root of the
                  graph.
  Comments    : After merging, there are some part of the graph becomming
                  unconnected. 
***************************************************************************/
static void delete_unconnected_graph(CatGraph &catG, NodePtr root)
{
   CGraph &G = catG.get_graph();
   NodePtr iterNode;
   EdgePtr iterEdge;
   int nodeNum = G.number_of_nodes();
   Array<NodePtr> nodeArray(nodeNum);

   int nodeCount = 0;
   forall_nodes(iterNode, G) {
      nodeArray[nodeCount] = iterNode;
      nodeCount++;
   }
   for (nodeCount = 0; nodeCount < nodeNum; nodeCount++) {
      iterNode = nodeArray[nodeCount];
      // Delete the node and its adjacent edges only if the node is not a root
      // and its indegree is 0. That is, the node is not reachable from root.
      if ((G.indeg(iterNode) == 0) && (iterNode != root)) {
	 iterEdge = G.first_adj_edge(iterNode);
	 while (iterEdge) {
	    EdgePtr tmpEdgePtr = iterEdge;
	    iterEdge = G.adj_succ(iterEdge);
	    delete G[tmpEdgePtr];
	    G.del_edge(tmpEdgePtr);
	 }
	 delete G[iterNode];
	 G.del_node(iterNode);
      }
   }
}

/***************************************************************************
  Description : Returns the number of conflict instnaces if we merge 2
                  categorizers.
  Comments    :
***************************************************************************/
static int merge_cat_conflict(const Categorizer& cat1,
			      const Categorizer& cat2) 
{
   const Array<int>& distr1 = cat1.get_distr();
   const Array<int>& distr2 = cat2.get_distr();

   ASSERT(distr1.size() == distr2.size());
   Array<int> distr(distr1.size());

   // Find the majority if we merge these 2 categorizer.
   Category majorCat = -1;
   int maxInstances = -1;
   Category cat;
   for (cat = 0; cat < distr1.size(); cat++)
      if (maxInstances < (distr1[cat] + distr2[cat])) {
	 maxInstances = distr1[cat] + distr2[cat];
	 majorCat = cat;
      }

   ASSERT(maxInstances != -1);
   // Calculate the conflict number of merged distribution
   int totalConflict = 0;
   for (cat = 0; cat < distr1.size(); cat++) {
      if (cat != majorCat)
	 totalConflict += distr1[cat] + distr2[cat];
   }
   return totalConflict;
}


/***************************************************************************
  Description : Returns the number of conflict instances in a graph.
  Comments    : The graph must have NO loop. Otherwise, this function will
                  enter an infinite loop.
***************************************************************************/
static int internal_conflict(const LogOptions& logOptions,
			     const CatGraph &catG, const NodePtr n)
{
   const CGraph& G = catG.get_graph();
   if (G.outdeg(n) == 0) { // leaf node.
      const Categorizer &cat = G[n]->get_categorizer();
      ASSERT(cat.class_id() == CLASS_CONST_CATEGORIZER);
      return num_conflicting_instances(logOptions, cat);
   }

   EdgePtr iterEdge;
   int conflictNum = 0;
   forall_adj_edges(iterEdge, n)
      conflictNum += internal_conflict(logOptions, catG, target(iterEdge));
   return conflictNum;
}

/***************************************************************************
  Description : Returns the number of conflict instances if we merge these
                  2 subgraphs. changeLabel indicates whether any labels of the
		  trees are changed or not during the merge.
  Comments    : If the outdegrees of n1 and n2 are different, one of n1 and n2
                  MUST be a leaf. In addition, the leaf must be a UNKNOWN
		  node. If not, this function fails.
		n1 and n2 must be different for this fuction to succeed.
***************************************************************************/
static int
acyc_merge_conflict_recursive(const LogOptions& logOptions, 
                              const CatGraph& catG1, const NodePtr n1,
			      const CatGraph& catG2, const NodePtr n2,
			      Bool& changeLabel)
{
   DBG(check_valid_node_ptr(n1, n2));
   const CGraph& G1 = catG1.get_graph();
   const CGraph& G2 = catG2.get_graph();
   
   ASSERT(n1 != n2);
   
   EdgePtr iterEdge1,iterEdge2;
   int outDegree1(G1.outdeg(n1));
   int outDegree2(G2.outdeg(n2));

   DBG(check_duplicate_edges(n1,G1));
   DBG(check_duplicate_edges(n2,G2));

   if ((outDegree1 == 0) && (outDegree2 == 0)) {
      // If the categorys are different, the labels are changed.
      ASSERT(G1[n1]->get_categorizer().class_id() == CLASS_CONST_CATEGORIZER);
      ASSERT(G2[n2]->get_categorizer().class_id() == CLASS_CONST_CATEGORIZER);
      const AugCategory& ac1 =
	 ((ConstCategorizer&)(G1[n1]->get_categorizer())).get_category();
      const AugCategory& ac2 =
	 ((ConstCategorizer&)(G2[n2]->get_categorizer())).get_category();      
      if (((ConstCategorizer&)(G1[n1]->get_categorizer())).get_category() !=
          (((ConstCategorizer&)(G2[n2])->get_categorizer())).get_category())
	 changeLabel = TRUE;
      return
	 merge_cat_conflict(
	    G1[n1]->get_categorizer(), G2[n2]->get_categorizer());
   }
   // If the outdegrees are different, one of the node MUST be a leaf.
   if (outDegree1 == 0) {    // n1 is a leaf node
      // Merge a leaf with a non-leaf. The label will be changed if merged.
      changeLabel = TRUE;
      return internal_conflict(logOptions, catG1, n1)
	 + internal_conflict(logOptions, catG2, n2);
   }
   if (outDegree2 == 0) {   // n2 is a leaf node
      // Merge a leaf with a non-leaf. The label will be changed if merged.
      changeLabel = TRUE;
      return internal_conflict(logOptions, catG1, n1)
	 + internal_conflict(logOptions, catG2, n2);
   }
   if (outDegree1 != outDegree2) {
      err << "Node [" << index(n1) << "]"
	  << G1[n1]->get_categorizer().description() << " and node["
	  << index(n2) << "]" << G2[n2]->get_categorizer().description()
	  << "are the same level but have different numbers of branches. "
	  << fatal_error;
   }

   // Add the merge conflict of its children
   int mergeConflict = 0;
   iterEdge1 = G1.first_adj_edge(n1);
   while (iterEdge1) {
      int edgeMatchCount=0;
      iterEdge2 = G2.first_adj_edge(n2);
      while (iterEdge2) {
	 if ( (*G1[iterEdge1]) == (*G2[iterEdge2])) { // the edges match
	    edgeMatchCount ++;
	    mergeConflict +=
	       acyc_merge_conflict_recursive(logOptions,
					     catG1, target(iterEdge1),
					     catG2, target(iterEdge2),
					     changeLabel);
	 }
	 iterEdge2 = G2.adj_succ(iterEdge2);	    
      }
      if (edgeMatchCount==0)
	 err << "Can not find matched edges." << fatal_error;
      iterEdge1 = G1.adj_succ(iterEdge1);
   }
   return mergeConflict;
}

int
acyclic_graph_merge_conflict(const LogOptions& logOptions,
			     const CatGraph& catG1, const NodePtr n1,
			     const CatGraph& catG2, const NodePtr n2,
			     Bool& changeLabel)
{
   changeLabel = FALSE;
   return acyc_merge_conflict_recursive(logOptions,
					catG1, n1, catG2, n2, changeLabel);
}
/***************************************************************************
  Description : Find the edge between node p and c. Helper function of
                  merge_node.
  Comments    :
***************************************************************************/
static EdgePtr find_edge(NodePtr p, NodePtr c)
{
   DBG(ASSERT(graph_of(p) == graph_of(c)));
   EdgePtr iterEdge;
   forall_adj_edges(iterEdge, p) {
      if (target(iterEdge) == c) {
	 return iterEdge;
      }
   }
   err << "Can not find the edge between two nodes. " << fatal_error;
   // The following code is useless but is necessary for compiler.
   return NULL;
}


/***************************************************************************
  Description : Merges two nodes. If one of the nodes is UNKNOWN, reassign the
                   edges pointing to the UNKNOWN node to another node.
  Comments    : The reason of checking UNKNOWN node is that UNKNOWN node are
                   supposed to match everything. 
***************************************************************************/
static void merge_node(const LogOptions& logOptions,
		       const NominalAttrInfo& nai, 
		       CatGraph& catG,
		       EdgePtr e1, EdgePtr e2)

{
   CGraph& G = catG.get_graph();
   NodePtr target1 = target(e1);
   NodePtr target2 = target(e2);

   Categorizer&
      cat1 = G[target1]->get_categorizer();
   Categorizer&
      cat2 = G[target2]->get_categorizer();
   Categorizer* mergeCat = NULL;

   if (G.outdeg(target1) == 0)
      ASSERT(cat1.class_id() == CLASS_CONST_CATEGORIZER);
   if (G.outdeg(target2) == 0)
      ASSERT(cat2.class_id() == CLASS_CONST_CATEGORIZER);
   FLOG(4, "Collapse node [" << index(target2) << "]" 
	   << G[target2]->get_categorizer().description()
	   << "(Distr:" << G[target2]->get_categorizer().get_distr()
	   << ") to node [" << index(target1) << "]"
	   << G[target1]->get_categorizer().description()
	   << "(Distr:" << G[target1]->get_categorizer().get_distr()
	   << ")" << endl);
   if (G.outdeg(target1) != 0)
      mergeCat = cat1.copy();
   else
      if (G.outdeg(target2) != 0) {
	 EdgePtr iterEdge;
	 forall_adj_edges(iterEdge, target2) {
	    AugCategory* edge =
	       new AugCategory(G[iterEdge]->num(),
			       G[iterEdge]->description());
	    catG.connect(target1, G.target(iterEdge), edge);
	    ASSERT(edge == NULL);
	 }
	 mergeCat = cat2.copy();
      }
      else
	 mergeCat = cat1.copy();
	 
   ASSERT(mergeCat);
   mergeCat->set_distr(cat1.get_distr());
   mergeCat->add_distr(cat2.get_distr());
   // If mergeCat is a ConstCategorizer, we have to recalculate its majority.
   if (mergeCat->class_id() == CLASS_CONST_CATEGORIZER) {
      ASSERT(cat1.class_id() == CLASS_CONST_CATEGORIZER);
      ASSERT(cat2.class_id() == CLASS_CONST_CATEGORIZER);
      Category majority = (mergeCat->num_instances()) ?
	 mergeCat->majority_category() :
	 UNKNOWN_CATEGORY_VAL;
      MString categoryString;
      if (majority==UNKNOWN_CATEGORY_VAL)
	 categoryString = "?";
      else
	 categoryString = nai.get_value(majority);
      MString descr(categoryString);
      AugCategory augMajority(majority, categoryString);
      Categorizer *leafCat =
	 new ConstCategorizer(descr, augMajority);
      leafCat->set_distr(mergeCat->get_distr());
      delete mergeCat;
      mergeCat = leafCat;
   }
   G[target1]->set_categorizer(mergeCat);
   ASSERT(mergeCat == NULL);
    FLOG(4, "The result node [" << index(target1) << "]"
	   << G[target1]->get_categorizer().description()
	   << " (Distr:" << G[target1]->get_categorizer().get_distr()
	   << ")" << endl);
   
   // Break all incomming edges of target2
   // Don't want to use reference here. The reason is:
   //   del_edge changes the parents list of target1. If we delete the
   //   edge, pix will be incorrect. Therefore, we had better copy the
   //   whole DblLinkList instead of using reference.
   const DblLinkList<NodePtr>
      parentList(G[target2]->get_parents(), ctorDummy);
   for (DLLPix<NodePtr> pix(parentList,1); pix; pix.next()) {
      NodePtr parentNode = parentList(pix);
      EdgePtr edgePtr = find_edge(parentNode, target2);
      G.new_edge(parentNode, target1, G[edgePtr]);
      G.del_edge(edgePtr);
   }
}

/***************************************************************************
  Description : Merges 2 isomorphic graphs.
  Comments    : 
***************************************************************************/
static void merge_sub_graph(const LogOptions& logOptions,
			    const NominalAttrInfo& nai,
			    CatGraph& catG, NodePtr n1, NodePtr n2)
{
   DBG(check_valid_node_ptr(n1, n2));
   CGraph& G = catG.get_graph();

   // Stupid to merge a node with itself.
   ASSERT(n1 != n2);
   
   int outDegree1(G.outdeg(n1));
   int outDegree2(G.outdeg(n2));

   if (outDegree1 == 0)
      return;
   if (outDegree2 == 0)
      return;
   ASSERT(outDegree1 == outDegree2);

   EdgePtr iterEdge1 = G.first_adj_edge(n1);
   while (iterEdge1) {
      Bool mergeFlag = FALSE;
      EdgePtr iterEdge2 = G.first_adj_edge(n2);
      while (iterEdge2) {
	 if ( (*G[iterEdge1]) == (*G[iterEdge2])) {
	    NodePtr nextN1 = target(iterEdge1);
	    NodePtr nextN2 = target(iterEdge2);
	    if (nextN1 != nextN2) {
	       if ( G.outdeg(nextN1) == 0 ||
		    G.outdeg(nextN2) == 0)
		  merge_node(logOptions, nai, catG, iterEdge1, iterEdge2);
	       else {
		  merge_sub_graph(logOptions, nai, catG, nextN1, nextN2);
		  merge_node(logOptions, nai, catG, iterEdge1, iterEdge2);
	       }
	       mergeFlag = TRUE;
	       // Restart the merging loop. That is, break 2 loops.
	       iterEdge2 = NULL;
	       iterEdge1 = G.first_adj_edge(n1);
	    }
	 }
	 // If something merged, break the loop. Otherwise, go to next
	 // iteration. 
	 if (!mergeFlag)
	    iterEdge2 = G.adj_succ(iterEdge2);
      }
      if (!mergeFlag)
	 iterEdge1 = G.adj_succ(iterEdge1);
   }
}

/***************************************************************************
  Description : Returns (the internal conflict of merged graph minus the sum of
                  internal conflict ratio of n1 and n2) / num of instances.
  Comments    : 
***************************************************************************/
Real approx_isomorphic(const LogOptions& logOptions,
		       const CatGraph &catG1, NodePtr n1,
		       const CatGraph &catG2, NodePtr n2, Bool& changeLabel)
{
   DBG(check_valid_node_ptr(n1, n2));
   const CGraph& G1 = catG1.get_graph();
   const CGraph& G2 = catG2.get_graph();
   Real conflictRatio = 0;
   
   // This is only for acceleration. The ratio will be zero if one of the
   // nodes is UNKNOWN.
   if (G1.outdeg(n1) == 0) {
      ASSERT(G1[n1]->get_categorizer().class_id() == CLASS_CONST_CATEGORIZER);
      const ConstCategorizer& tmpCat =
	 (const ConstCategorizer&) G1[n1]->get_categorizer(); 
      if (tmpCat.get_category() == UNKNOWN_CATEGORY_VAL)
	 return 0;
   }

   if (G2.outdeg(n2) == 0) {
      ASSERT(G2[n2]->get_categorizer().class_id() == CLASS_CONST_CATEGORIZER);
      const ConstCategorizer& tmpCat =
	 (const ConstCategorizer&) G2[n2]->get_categorizer();
      if (tmpCat.get_category() == UNKNOWN_CATEGORY_VAL)
	 return 0;
   }
   int internalConflictSum =
      internal_conflict(logOptions, catG1, n1)
      + internal_conflict(logOptions, catG2,n2);
   int numInstances = G1[n1]->get_categorizer().num_instances() +
                       G2[n2]->get_categorizer().num_instances();
   ASSERT(numInstances != 0);
   int resultConflict = acyclic_graph_merge_conflict(
      logOptions, catG1, n1, catG2, n2, changeLabel);
   conflictRatio = Real(resultConflict - internalConflictSum )
                        / numInstances;
   ASSERT(resultConflict >= internalConflictSum);
   FLOG(3, "Number of total instances in nodes to be matched: "
	   << numInstances << "." << endl);
   if (resultConflict - internalConflictSum) {
      FLOG(3, "If merging  "
	      << G1[n1]->get_categorizer().description() << " and "
	      << G2[n2]->get_categorizer().description() << ", " 
	      << "the number of conflicting instances inscreased from "
	      << internalConflictSum << " to " << resultConflict << "." 
	      << endl);
   }
   else {
      FLOG(3, "No increase in conflicting instances." << endl);
   }
   FLOG(3, "Conflict ratio of merged graph: " << conflictRatio << "." 
	   << endl); 
   
   return conflictRatio;
}

/***************************************************************************
  Description : Helper function of merge_graph.
  Comments    :
***************************************************************************/
static void build_next_level(CGraph& G,
			     DynamicArray<NodePtr>& nextLevel,
			     DynamicArray<EdgePtr>& edges,
			     DynamicArray<NodePtr>& currentLevel,
			     int& nextNodeCount,
			     int currentNodeCount)
{
   for (int nodeCount = 0; nodeCount < currentNodeCount; nodeCount++) {
      EdgePtr iterEdge;
      if (currentLevel[nodeCount]) {
	 forall_adj_edges(iterEdge, currentLevel[nodeCount]) {
	    nextLevel[nextNodeCount] = G.target(iterEdge);
	    edges[nextNodeCount] = iterEdge;
	    nextNodeCount++;
	 }
      }
   }
}

/***************************************************************************
  Description : Return the pessimistic error number which is the the number of
                  instances times the higher bound of the confidence interval
		  of (conflictNum / n). 
  Comments    : 
***************************************************************************/
Real pessimistic_error_num(int conflictNum, int n, Real z)
{
   if (n == 0)
      return 0;
   Real confLow, confHigh;
   Real error = Real(conflictNum) / n;
   CatTestResult::confidence(confLow, confHigh, error, n, z);
   return Real(n * confHigh);
}

/***************************************************************************
  Description : Returns the number of leaves in catG beginning with node n.
  Comments    :
***************************************************************************/
int num_leaves(const CatGraph& catG, NodePtr n)
{
   CGraph &G = catG.get_graph();
   if (G.outdeg(n) == 0)
      return 1;
   int numLeaves = 0;
   NodePtr iterNode;
   forall_adj_nodes(iterNode, n)
      numLeaves+=num_leaves(catG, iterNode);
   return numLeaves;
}

int num_leaves(const CatGraph& catG, NodePtr n1, NodePtr n2)
{
   CGraph &G = catG.get_graph();
   // If both of the nodes are leaves, this still work fine since num_leaves
   //   of leaf node is also 1.
   if (G.outdeg(n1) == 0)
      return num_leaves(catG, n2);
   if (G.outdeg(n2) == 0)
      return num_leaves(catG, n1);
   int numLeaves = 0;
   EdgePtr iterEdge1, iterEdge2;
   forall_adj_edges(iterEdge1, n1)
      forall_adj_edges(iterEdge2, n2) 
	 if (*G[iterEdge1] == *G[iterEdge2])
	    numLeaves += num_leaves(catG,
				    G.target(iterEdge1), G.target(iterEdge2));
   return numLeaves;
}

/***************************************************************************
  Description : Returns TRUE if the categorizer is ConstCategorizer and it's
                  category is UNKNOWN_CATEGORY_VAL.
  Comments    :
***************************************************************************/
Bool is_unknown(Categorizer &cat)
{
   if (cat.class_id() != CLASS_CONST_CATEGORIZER)
      return FALSE;
   return (((ConstCategorizer&)cat).get_category() == UNKNOWN_CATEGORY_VAL);
}

#ifdef ALPHA
/***************************************************************************
  Description : Find the bestnode pair, bestNode1 and bestNode2. Helper
                  function of merge_graph. 
  Comments    : For speeding up, if we find a node pair which doesn't change
                  any label of both trees and increase the conflict, we are
		  sure that the merge doesn't hurt. Thus, they can be merged
		  at once. The function is returned at that point.
***************************************************************************/
static Bool best_node_pair(const LogOptions& logOptions, CatGraph& catG,
			   DynamicArray<NodePtr>& currentLevel,
			   int currentNodeCount,
			   int& bestNode1, int& bestNode2, Real alpha) 
{
   CGraph &G = catG.get_graph();
   // Among all the nodes pairs, find the node pair with minimum
   // conflicts ratio and maximum of instances in both of them.
   Bool bestFound = FALSE;
   bestNode1 = -1;
   bestNode2 = -1;
   Real maxGain = -1;
   int maxInstancesNum = -1;
   int lConfNum, rConfNum, mConfNum;
   for (int nCount1 = 0; nCount1 < currentNodeCount; nCount1++ ) {
      NodePtr n1 = currentLevel[nCount1];
      if (n1) {
	 for (int nCount2 = nCount1 + 1;
	      nCount2 < currentNodeCount; nCount2++) {
	    NodePtr n2 = currentLevel[nCount2];
	    if (n2) {
	       const Categorizer& tmpCat1 =
		  G[currentLevel[nCount1]]->get_categorizer();
	       const Categorizer& tmpCat2 =
		  G[currentLevel[nCount2]]->get_categorizer();
	       Bool changeLabel;
	       int numLeftConflict = internal_conflict(logOptions, catG, n1),
		  numRightConflict = internal_conflict(logOptions, catG, n2),
		  numMergeConflict = acyclic_graph_merge_conflict(
		     logOptions, catG, n1, catG, n2, changeLabel);
	       // This is for speeding up.
	       // If the merge doesn't change the label, then we can merge it
	       //   at once.
	       if (!changeLabel) {
		  FLOG(3, "Exactly isomorphic graphs:[" << index(n1) << "]"
			  << G[n1]->get_categorizer().description()
			  << "(Distr:" << G[n1]->get_categorizer().get_distr()
			  << ") and [" << index(n2) << "]"
			  << G[n2]->get_categorizer().description()
			  << "(Distr:" << G[n2]->get_categorizer().get_distr()
			  << ") are selected to merge" << endl);
		  FLOG(3, "Left-Conflict=" << numLeftConflict
			  << " Right-Conflict=" << numRightConflict
			  << " Merge-Conflict=" << numMergeConflict
			  << "." << endl);		  
 		  ASSERT(numMergeConflict ==
			 numLeftConflict + numRightConflict);
		  bestNode1 = nCount1;
		  bestNode2 = nCount2;
		  return TRUE;
	       }
	       int leftNumLeaves = num_leaves(catG, n1);
	       int rightNumLeaves = num_leaves(catG, n2);
	       int mergeNumLeaves = num_leaves(catG, n1, n2);
	       Real leftError = numLeftConflict + alpha * leftNumLeaves;
	       Real rightError = numRightConflict + alpha * rightNumLeaves;
	       Real mergeError = numMergeConflict + alpha * mergeNumLeaves;
	       Real gain =  (leftError + rightError) - mergeError;
	       
	       FLOG(4, "Node [" << index(n1) << "](" 
		       << tmpCat1.description() << ") has " << numLeftConflict
		       << " conflicts and  " << leftNumLeaves
		       << " leaves and gives " << leftError
		       << " errors." << endl);
	       FLOG(4, "Node [" << index(n2) << "](" 
		       << tmpCat2.description() << ") has "<< numRightConflict
		       << " conflicts and  " << rightNumLeaves
		       << " leaves and gives " << rightError
		       << " errors." << endl);
	       FLOG(4, "Merged node has "  << numMergeConflict
		       << " conflicts and  " << mergeNumLeaves
		       << " leaves and gives " << mergeError
		       << " errors." << endl);
	       FLOG(4, "Gain = " << gain << "." << endl);
	       int numMergeInstances = tmpCat1.num_instances() 
		  + tmpCat2.num_instances();
	       if (gain >= 0)
		  if ((maxGain < gain) ||
		      ((maxGain == gain) &&
		       (maxInstancesNum < numMergeInstances))
		     ) {
		     bestFound = TRUE;
		     maxGain = gain;
		     maxInstancesNum = numMergeInstances;
		     bestNode1 = nCount1;
		     bestNode2 = nCount2;
		     // The following is for log information only
		     lConfNum = numLeftConflict;
		     rConfNum = numRightConflict;
		     mConfNum = numMergeConflict; 
		  }
	    }
	 }
      }
   }
   if (bestFound) {
      const Categorizer&
	 tmpCat1 = G[currentLevel[bestNode1]]->get_categorizer();
      const Categorizer&
	 tmpCat2 = G[currentLevel[bestNode2]]->get_categorizer();
      FLOG(3, "Best nodes selected to merge: node ["
	      << index(currentLevel[bestNode1]) << "]"
	      << tmpCat1.description() << "(Distr:"
	      << tmpCat1.get_distr() << ") and node ["
	      << index(currentLevel[bestNode2]) << "]"
	      << tmpCat2.description() << "(Distr:"
	      << tmpCat2.get_distr() << ")." << endl);
      FLOG(3, "Left-Conflict=" << lConfNum << " Right-Conflict="
	      << rConfNum << " Merge-Conflict=" << mConfNum << "." << endl);
   }
   else 
      FLOG(3, "Nothing good to be merged" << endl);
   return bestFound;
}
#else
/***************************************************************************
  Description : Find the best node pair, bestNode1 and bestNode2. Helper
                  function of merge_graph. 
  Comments    : For speeding up, if we find a node pair which doesn't change
                  any label of both trees and increase the conflict, we are
		  sure that the merge doesn't hurt. Thus, they can be merged
		  at once. The function is returned at that point.
***************************************************************************/
static Bool best_node_pair(const LogOptions& logOptions, CatGraph& catG,
			   DynamicArray<NodePtr>& currentLevel,
			   int currentNodeCount,
			   int& bestNode1, int& bestNode2, Real pruneCoeff) 
{
   CGraph &G = catG.get_graph();
   // Among all the nodes pairs, find the node pair with minimum
   // conflicts ratio and maximum of instances in both of them.
   Bool bestFound = FALSE;
   bestNode1 = -1;
   bestNode2 = -1;
   Real maxGain = -1;
   int maxInstancesNum = -1;
   int lConfNum, rConfNum, mConfNum;
   for (int nCount1 = 0; nCount1 < currentNodeCount; nCount1++ ) {
      NodePtr n1 = currentLevel[nCount1];
      if (n1 && !is_unknown(G[n1]->get_categorizer())) {
	 for (int nCount2 = nCount1 + 1;
	      nCount2 < currentNodeCount; nCount2++) {
	    NodePtr n2 = currentLevel[nCount2];
	    if (n2 && !is_unknown(G[n2]->get_categorizer())) {
	       const Categorizer& tmpCat1 =
		  G[currentLevel[nCount1]]->get_categorizer();
	       const Categorizer& tmpCat2 =
		  G[currentLevel[nCount2]]->get_categorizer();
	       Bool changeLabel;
	       int numLeftConflict = internal_conflict(logOptions, catG, n1),
		  numRightConflict = internal_conflict(logOptions, catG, n2),
		  numMergeConflict = acyclic_graph_merge_conflict(
		     logOptions, catG, n1, catG, n2, changeLabel);
	       // This is for speeding up.
	       // If the merge doesn't change the label, then we can merge it
	       //   at once.
	       if (!changeLabel) {
		  FLOG(3, "Exactly isomorphic graphs: [" << index(n1) << "]"
			<< G[n1]->get_categorizer().description()
			<< "(Distr:" << G[n1]->get_categorizer().get_distr()
			<< ") and [" << index(n2) << "]"
			<< G[n2]->get_categorizer().description()
			<< "(Distr:" << G[n2]->get_categorizer().get_distr()
			<< ") are selected to merge" << endl);
		  FLOG(3, "Left-Conflict=" << numLeftConflict
			  << " Right-Conflict=" << numRightConflict
			  << " Merge-Conflict=" << numMergeConflict
			  << "." << endl);		  
 		  ASSERT(numMergeConflict ==
			 numLeftConflict + numRightConflict);
		  bestNode1 = nCount1;
		  bestNode2 = nCount2;
		  return TRUE;
	       }
	       int numLeftInstances = tmpCat1.num_instances(),
		  numRightInstances = tmpCat2.num_instances();
	       int numMergeInstances = numLeftInstances + numRightInstances;
	       Real numLeftError = pessimistic_error_num(numLeftConflict,
						    tmpCat1.num_instances(),
						    pruneCoeff);
	       Real numRightError = pessimistic_error_num(numRightConflict,
						     tmpCat2.num_instances(),
						     pruneCoeff);
	       Real numMergeError = pessimistic_error_num(numMergeConflict,
						     numMergeInstances,
						     pruneCoeff);
	       ASSERT(numLeftError >= 0);
	       ASSERT(numRightError >= 0);
	       ASSERT(numMergeError >= 0);
	       Real numGain = (numLeftError + numRightError) - numMergeError;
	       
	       FLOG(4, "Node [" << index(n1) << "](" 
		       << tmpCat1.description() << ") has " << numLeftConflict
		       << " conflicts out of " << numLeftInstances
		       << " instances and gives " << numLeftError
		       << " errors." << endl);
	       FLOG(4, "Node["  << index(n2) << "]("
		       << tmpCat2.description() << ") has " << numRightConflict
		       << " conflicts out of " << numRightInstances
		       << " instances and gives " << numRightError
		       << " errors." << endl);
	       FLOG(4, "Merged node has "<< numMergeConflict
		       << " conflicts out of " << numMergeInstances
		       << " instances and gives " << numMergeError
		       << " errors." << endl);
	       FLOG(4, "Gain = " << numGain << "." << endl);
	       
	       if (numGain >= 0)
		  if ((maxGain < numGain) ||
		      ((maxGain == numGain) &&
		       (maxInstancesNum < numMergeInstances))
		     ) {
		     bestFound = TRUE;
		     maxGain = numGain;
		     maxInstancesNum = numMergeInstances;
		     bestNode1 = nCount1;
		     bestNode2 = nCount2;
		     // The following is for log information only
		     lConfNum = numLeftConflict;
		     rConfNum = numRightConflict;
		     mConfNum = numMergeConflict; 
		  }
	    }
	 }
      }
   }
   if (bestFound) {
      const Categorizer&
	 tmpCat1 = G[currentLevel[bestNode1]]->get_categorizer();
      const Categorizer&
	 tmpCat2 = G[currentLevel[bestNode2]]->get_categorizer();
      FLOG(3, "Best nodes selected to merge: node ["
	      << index(currentLevel[bestNode1]) << "]"
	      << tmpCat1.description() << "(Distr:"
	      << tmpCat1.get_distr() << ") and node ["
	      << index(currentLevel[bestNode2]) << "]"
	      << tmpCat2.description() << "(Distr:"
	      << tmpCat2.get_distr() << ")." << endl);
      FLOG(3, "Left-Conflict=" << lConfNum << " Right-Conflict="
	      << rConfNum << " Merge-Conflict=" << mConfNum << "." << endl);
   }
   else 
      FLOG(3, "Nothing good to be merged" << endl);
   return bestFound;
}

#endif
/***************************************************************************
  Description : Merges isomorphic graphs.
  Comments    : Find all the nodes in next level of root and edges to next
                  level. For each node in the current level, find the best
		  node pair with the minimum conflict ratio and the largest
		  number of instances in both of them. Merge the nodes in the
		  pairs. 
***************************************************************************/
#ifdef ALPHA
const MString alphaHelp = "The alpha ratio."; 
const Real defaultAlpha = 0.01;
#else
const MString pruneCoeffHelp = "This option chooses the pessimistic prunning "
  "coefficient of confidential interval.";
const Real defaultPruneCoeff = 1.65;
#endif


void merge_graph(const LogOptions& logOptions, const NominalAttrInfo& nai, 
		 RootedCatGraph& catG, NodePtr root)
{
#ifdef ALPHA   
   static Real alpha = 
      get_option_real_range("ALPHA",
			    defaultAlpha, 0, REAL_MAX,
			    alphaHelp, TRUE);
#else   
   static Real pruneCoeff = 
      get_option_real_range("PRUNE_COEFF",
			    defaultPruneCoeff, 0, REAL_MAX,
			    pruneCoeffHelp, TRUE);
#endif

   CGraph &G = catG.get_graph();

   DynamicArray<NodePtr> nextLevel(0);
   DynamicArray<NodePtr> currentLevel(0);
   DynamicArray<EdgePtr> edges(0);
   currentLevel[0] = root;


   int nextNodeCount;
   int currentNodeCount = 1; 
   int levelCount= 0 ; 
   do {  // For every level.
      FLOG(2, "--------- Level " << levelCount++ << " ---------" << endl);
      nextNodeCount = 0;
      // Build the array needed for the next level.
      build_next_level(G, nextLevel, edges, currentLevel, nextNodeCount,
		       currentNodeCount); 
      FLOG(2,"Number of non-leaf nodes in next level = " << nextNodeCount
	      << endl); 
      if (nextNodeCount > 0) {
	 currentLevel = nextLevel;
	 currentNodeCount = nextNodeCount;
	 Bool bestFound;
	 do {
	    int bestNode1, bestNode2;
#ifdef ALPHA 	    
	    bestFound = best_node_pair(logOptions, catG, currentLevel,
				       currentNodeCount, bestNode1,
				       bestNode2, alpha);
#else
	    bestFound = best_node_pair(logOptions, catG, currentLevel,
				       currentNodeCount, bestNode1,
				       bestNode2, pruneCoeff);
#endif	    
	    // Merge the best node pair
	    if (bestFound) {
	       merge_sub_graph(logOptions, nai, catG, currentLevel[bestNode1],
			       currentLevel[bestNode2]); 
	       merge_node(logOptions, nai,
			  catG, edges[bestNode1], edges[bestNode2]);
	       // if (currentLevel[bestNode1] is merged to
	       // currentLevel[bestNode2], set currentLevel[nCount1]
	       // to NULL and and restart the bestNode selection loop.
	       // If currentLevel[bestNode1] is collapse, its indegree will be
	       // 0.
	       if (G.indeg(currentLevel[bestNode1]) == 0) {
		  ASSERT(G.indeg(currentLevel[bestNode2]) > 0);
		  currentLevel[bestNode1] = NULL;
	       }
	       else {
		  ASSERT(G.indeg(currentLevel[bestNode2]) == 0);
		  currentLevel[bestNode2] = NULL;
	       }
	    }
	 } while (bestFound);
      }
   } while (nextNodeCount > 0);
   // Delete garbages.

   delete_unconnected_graph(catG, catG.get_root());
}


/***************************************************************************
  Description : Returns TRUE if all the outgoing edges go to the same target.
  Comments    :
***************************************************************************/
Bool const_node(CGraph &G, NodePtr n)
{
   ASSERT(n);
   // Leaf can not be pruned.
   if (G.outdeg(n) == 0) 
      return FALSE;
   // Root can not be pruned.
   if (G.indeg(n) == 0)
      return FALSE;
   EdgePtr iterEdge;
   NodePtr target = NULL;
   forall_adj_edges(iterEdge, n) {
      if (target ==NULL)
	 target = G.target(iterEdge);
      else
	 if (target != G.target(iterEdge))
	    return FALSE;
   }
   return TRUE;
}

/***************************************************************************
  Description : Prune the constant nodes in the graph, catG. Constant nodes
                  means those nodes whose outgoing edges going to the same
		  target.
  Comments    :
***************************************************************************/
void prune_graph(const LogOptions& logOptions, RootedCatGraph& catG)
{
   CGraph &G = catG.get_graph();
   Bool pruned;
   do {
      pruned = FALSE;
      NodePtr iterNode = G.first_node();
      while (iterNode) {
	 if (const_node(G, iterNode)) {
	    FLOG(3, "Node [" << index(iterNode) << "]"
		    << G[iterNode]->get_categorizer().description()
		    << " is pruned." << endl);
	    NodePtr target = G.target(G.first_adj_edge(iterNode));
	    // Connect the parents of iterNode to its target.
	    const DblLinkList<NodePtr>
	       parentList(G[iterNode]->get_parents(), ctorDummy);
	    for (DLLPix<NodePtr> pix(parentList,1); pix; pix.next()) {
	       NodePtr parentNode = parentList(pix);
	       EdgePtr edgePtr = find_edge(parentNode, iterNode);
	       G.new_edge(parentNode, target, G[edgePtr]);
	       G.del_edge(edgePtr);
	    }
	    // delete all the ourgoing edges from iterNode
	    EdgePtr iterEdge = G.first_adj_edge(iterNode);
	    while (iterEdge) {
	       EdgePtr tmpEdgePtr = iterEdge;
	       iterEdge = G.adj_succ(iterEdge);
	       delete G[tmpEdgePtr];
	       G.del_edge(tmpEdgePtr);
	    }
	    // delete iterNode
	    NodePtr tmpIterNode = iterNode;
	    iterNode = G.succ_node(iterNode);
	    delete G[tmpIterNode];
	    G.del_node(tmpIterNode);
	    pruned = TRUE;
	 }
	 else
	    iterNode = G.succ_node(iterNode);
      }
   } while (pruned);
}

/***************************************************************************
  Description : Calculate the number of conflicting instances.
  Comments    :
***************************************************************************/
int num_conflicting_instances(const LogOptions& ,
			      const Categorizer& cat)
{
   Category majorCat = cat.majority_category();

   return cat.num_instances() - cat.get_distr()[majorCat];
}

/***************************************************************************
  Description : Calculate the conflict ratio of the probability distibution
                  array of cat.
  Comments    :
***************************************************************************/
Real conflict_ratio(const LogOptions& logOptions, const Categorizer& cat)
{
   int conflictNum = num_conflicting_instances(logOptions, cat);
   int instancesNum = cat.num_instances();
   
   ASSERT(conflictNum >= 0);
   ASSERT(instancesNum > 0);
   return (Real(conflictNum) / Real(instancesNum));
}




