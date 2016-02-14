// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : RootedCatGraph instances have a specific root node.
                 All nodes in RootedCatGraphs should be reachable from the
		   root.
  Assumptions  :
  Comments     :
  Complexity   : OK(level 2) constant.
                 OK(level 1) takes time for OK(level 1) + 
		   CatGraph::is_connected()O(V+E), 
                 OK(level 0) takes time proportional to CatGraph::OK()
  Enhancements : Change display() to use new MLCStream.
  History      : Richard Long                                       8/27/93
                   Initial revision (.c)
		 Richard Long                                       8/04/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <RootCatGraph.h>
#include <Categorizer.h>

RCSID("MLC++, $RCSfile: RootCatGraph.c,v $ $Revision: 1.25 $")

/***************************************************************************
  Description : Level 1 checks that the root does not have any parents if
                  it has been set and calls CatGraph::OK()
                Level 0 also checks that all of the nodes are reachable 
		   from root if it has been set.
  Comments    : If the RootedCatGraph does not own the cGraph, then the whole
                  cGraph may not necessarily be reachable from the
		  root, so that check is not performed.
***************************************************************************/
void RootedCatGraph::OK(int level) const
{
   switch (level) {
   case 0:
      CatGraph::OK();
   case 1:
      if (get_root() && graphAlloc)
	 check_reachable(get_root(), TRUE);
   case 2:
      if (get_root())
	 if (cGraph.indeg(root) != 0)
	    err << "RootedCatGraph::OK: Root node cannot have parents, "
	           "but it has " << cGraph.indeg(root) << fatal_error;
      break;
   default:
      err << "RootedCatGraph::OK: Invalid level " << level << fatal_error;
   }
}


/***************************************************************************
  Description : Initializes root to NULL.
  Comments    : 
***************************************************************************/
RootedCatGraph::RootedCatGraph() : CatGraph()
{
   root = NULL;
}

RootedCatGraph::RootedCatGraph(CGraph& grph) : CatGraph(grph)
{
   root = NULL;
}


/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : We do not use the standard issue copy constructor because
                     we want to know exactly when we call the copy
		     constructor.  
***************************************************************************/
RootedCatGraph::RootedCatGraph(const RootedCatGraph& source,
			       CtorDummy /*dummyArg*/)
   : CatGraph(source, ctorDummy)
{
   if (this == &source)
      err << "RootedCatGraph::RootedCatGraph(RootedCatGraph&, int): "
	 "cannot call copy constructor using 'this' pointer as the source"
	 << fatal_error;
   if (source.root == NULL)
      root = NULL;
   else { // have to point root to the corresponding node in the new graph
      const CGraph& srcGraph = source.get_graph();
      NodePtr srcNodePtr = srcGraph.first_node();
      NodePtr thisNodePtr = cGraph.first_node();
      while (srcNodePtr && thisNodePtr) {
	 if (srcNodePtr == source.get_root()) {
	    root = thisNodePtr;
	    break;
	 }
	 srcNodePtr = srcGraph.succ_node(srcNodePtr);
	 thisNodePtr = cGraph.succ_node(thisNodePtr);
      }
      ASSERT(srcNodePtr && thisNodePtr); // should have found source's root
      // Sanity check that both root node have the same description
      DBG(ASSERT(get_categorizer(root).description() ==
		 source.get_categorizer(source.get_root()).description()));
   }
}


/***************************************************************************
  Description : Checks that RootedCatGraph is valid before disposal.
  Comments    : 
***************************************************************************/
RootedCatGraph::~RootedCatGraph()
{
  DBG(OK(1)); // don't call with zero because CatGraph's destructor's calls OK
	      // for base class.
}


/***************************************************************************
  Description : Sets the root of the RootedCatGraph.
  Comments    : 
***************************************************************************/
void RootedCatGraph::set_root(const NodePtr nodePtr)
{
  DBG(if (nodePtr != NULL)
         check_node_in_graph(nodePtr, TRUE));
  root = nodePtr;
}


/***************************************************************************
  Description : If the root has been set, return pointer to root.  If
                  abortOnNULL is FALSE returns NULL if root has not been
		  set.  Otherwise aborts when root is not set.
  Comments    : 
***************************************************************************/
NodePtr RootedCatGraph::get_root(Bool abortOnNoRoot) const
{
  if (root == NULL && abortOnNoRoot)
    err << "RootedCatGraph::get_root: Root has not been set" << fatal_error;
  return root;
}


/***************************************************************************
  Description : Displays the root, as well as CatGraph::display().
  Comments    : This should be able to send everything to the same stream
                  once MLCStream is incorporated.  It wasn't worth the
		  time to fix it for the old style.
***************************************************************************/
void RootedCatGraph::display(MLCOStream& stream,const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::ASCIIDisplay)
      if (root != NULL)
         stream << "Root: " << get_categorizer(root).description()
		<< endl;
      else
         stream << "Warning, no root defined for graph" << endl;
   CatGraph::display(stream, dp);
}
