
// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Categorizer goes down the RootedCatGraph using the 
                   Categorizer at each node until it reaches a node
		   with no children.  That node returns the category.
  Assumptions  : 
  Comments     : 
  Complexity   : OK() has time complexity of RootedCatGraph::OK()
                 categorize() takes time proportional to the depth
		   of the rcGraph * the time of categorize()
		   for the categorizers in the rcGraph.
  Enhancements : 
  History      : Chia-Hsin Li                                      11/23/94
                   Add operator==
                 Richard Long                                       9/04/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <RDGCat.h>
#include <isocat.h>

RCSID("MLC++, $RCSfile: RDGCat.c,v $ $Revision: 1.37 $")

/***************************************************************************
  Description : Calls OK() for the RootedCatGraph.
  Comments    : 
***************************************************************************/
void RDGCategorizer::OK(int /*level*/) const
{
   if (rcGraph != NULL) {
      if (rcGraph->get_root() == NULL)
	 err << "RDGCategorizer::OK: Root not set" << fatal_error;
      rcGraph->OK();
   }
}


/***************************************************************************
  Description : Initializes the RDGCategorizer.  Gets ownership  of
                  RootedCatGraph.
  Comments    : Gets ownership of the rooted graph (rcg).
***************************************************************************/
RDGCategorizer::RDGCategorizer(RootedCatGraph*& rcg, const MString& dscr,
			       int numCat) : Categorizer(numCat, dscr) 
{
   rcGraph = rcg;
   rcg = NULL;
}


/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : We do not use the standard issue copy constructor because
                     we want to know exactly when we call the copy
		     constructor.  
***************************************************************************/
RDGCategorizer::RDGCategorizer(const RDGCategorizer& source,
			       CtorDummy /*dummyArg*/)
   : Categorizer(source, ctorDummy)
{
   rcGraph = new RootedCatGraph(*(source.rcGraph), ctorDummy);
}


/***************************************************************************
  Description : Checks that RootedCatGraph is valid before destruction.
  Comments    : 
***************************************************************************/
RDGCategorizer::~RDGCategorizer()
{
  DBG(OK());
  delete rcGraph;
}


/***************************************************************************
  Description : For each node, call categorize on the instance and travel
                  to the child indicated by the returned category.  Returns
		  the category given by the last reached node (which will
		  have no children).
  Comments    : 
***************************************************************************/
AugCategory RDGCategorizer::categorize(const InstanceRC& inst) const
{
   DBGSLOW(OK());
   NodePtr currentNode = rcGraph->get_root(TRUE);  // get_root() error checks
   while (rcGraph->num_children(currentNode) > 0) {
      const Categorizer& cat = rcGraph->get_categorizer(currentNode);
      const AugCategory nextEdgeLabel = cat.categorize(inst);
      currentNode = rcGraph->get_child(currentNode, nextEdgeLabel);
   }
   return rcGraph->get_categorizer(currentNode).categorize(inst);
}


/***************************************************************************
  Description : Prints a readable representation of the categorizer to the
                  given stream.
  Comments    : 
  ***************************************************************************/
void RDGCategorizer::display_struct(MLCOStream& stream,
				    const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::ASCIIDisplay)
      stream << "Rooted Decision Graph Categorizer " << description()
	     << endl;
   rcGraph->display(stream, dp);
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of this RDGCategorizer.
  Comments    :
***************************************************************************/
Categorizer* RDGCategorizer::copy() const
{
   return new RDGCategorizer(*this, ctorDummy);
}

/***************************************************************************
  Description : Return rooted_cat_graph
  Comments    :
***************************************************************************/

const RootedCatGraph& RDGCategorizer::rooted_cat_graph() const
{
   if (rcGraph == NULL)
      err << "RootedCatGraph::rooted_cat_graph: NULL rcGrph" << fatal_error;

   return *rcGraph;
}

/***************************************************************************
  Description : Returns TRUE if the rooted graphs are isomorphic
  Comments    :
***************************************************************************/

Bool RDGCategorizer::operator==(const Categorizer& cat) const
{
   if (class_id()==cat.class_id())
      return ((*this) == (const RDGCategorizer &) cat);
   return FALSE;
}

Bool RDGCategorizer::operator==(const RDGCategorizer& cat) const
{
   Bool dummy;
   // @@ The problem is that the nodes have to contain a distribution
   // @@  or this doesn't work.  Suggest either forcing a distribution
   // @@  or modifying num_conflicting_instances to return 0 in
   // @@  such cases.
   err << "operator== unsupported now" << endl;
   return (approx_isomorphic(get_log_options(), 
			     *rcGraph,
			    rcGraph->get_root(),
			    *(cat.rcGraph),
			    cat.rcGraph->get_root(), dummy) == 0);
}

/***************************************************************************
  Description : Releases the graph used by this categorizer.  Gives
                  ownership to the caller.
  Comments    :
***************************************************************************/
RootedCatGraph* RDGCategorizer::release_rooted_cat_graph(void)
{
   RootedCatGraph* graph = rcGraph;
   rcGraph = NULL;
   return graph;
}

