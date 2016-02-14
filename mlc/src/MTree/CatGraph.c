// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : CatGraph is an directed graph whose nodes have pointers
                   to Categorizers.  Edges are labelled with the category
		   number they match.
		 Each node's first edge must be labelled either
                   UNKNOWN_CATEGORY_VAL or FIRST_CATEGORY_VAL.
                 Each additional edge must be labelled with the next
                   category in ascending order.
		 A CatGraph can have disconnected components (e.g. when
		   implementing more than one categorizer in a CatGraph.)
		 A CatGraph is acyclic (checked by OK()).
  Assumptions  : 
  Comments     : Uses LEDA's paramaterized, directed graph (GRAPH).
                 check_edge_label(): All the numbers of the edges adjacent to
		   a node should be consecutive. That is, we want to make sure
		   that no edges are missing during building the
		   graph. However, we also want to allow different order of
		   the numbers of edges. For example, [0, 2, 1, 3] should be
		   allowed to be an alternate of [0, 1, 2, 3].
  Complexity   : OK(level 0) takes time proportional to the number of nodes 
                   + the number of edges + the time of OK(level 1).
		 OK(level 1) takes time proportional to the number of nodes 
		   * the number of edges.
		 ~CatGraph() takes time proportional to the number of nodes
		   + number of edges.
		 get_child() takes time proportional to the number of edges
		   for the parent node.
		 display() takes time proportional to the number of nodes
		   + the number of edges
		 check_reachable() take O(V+E), where V is the number of 
		   nodes and E is the number of edges in the graph.
		 check_tree() takes time of check_reachable()
  Enhancements : Make get_child() constant time (hard to do with LEDA because
		   its edges are in a list).
                 Provide support for levelled graphs.
  History      : Richard Long                                       8/20/93
                   Initial revision (.c)
		 Richard Long                                       8/19/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <CatGraph.h>
#include <LEDA/graph_alg.h>
#include <string.h>
#include <mlcIO.h>
#include <AugCategory.h>
#include <Categorizer.h>
#include <GetOption.h>

RCSID("MLC++, $RCSfile: CatGraph.c,v $ $Revision: 1.61 $")

/***************************************************************************
  Description : Gets the preferences from the DisplayPref class.
  Comments    : This method applies only to DotPostscript display type.
***************************************************************************/
static void process_DotPoscript_preferences(MLCOStream& stream,
					    const DisplayPref& pref) 
{
   // Remember: These are preferences that only make sense for the
   // Postscript Display
   FloatPair pageSize, graphSize;
   pageSize = pref.typecast_to_DotPostscript().get_page_size();
   graphSize = pref.typecast_to_DotPostscript().get_graph_size();
   DotPostscriptPref::DisplayOrientation orientation;
   orientation = pref.typecast_to_DotPostscript().get_orientation();
   DotPostscriptPref::DotRatioType ratio;
   ratio = pref.typecast_to_DotPostscript().get_ratio();
   
   stream << "page = \"" << pageSize.x << ","
	  << pageSize.y << "\";" << endl
	  << "size = \"" << graphSize.x << ","
	  << graphSize.y << "\";" << endl;
   if (orientation == DotPostscriptPref::DisplayLandscape)
      stream << "orientation = landscape;" << endl;
   else
      stream << "orientation = portrait;" << endl;
   if (ratio == DotPostscriptPref::RatioFill)
      stream << "ratio = fill;" << endl;
}


/***************************************************************************
  Description : Checks that the edge labels for each node are consecutive,
                  starting from either UNKNOWN_CATEGORY_VAL or
                  FIRST_CATEGORY_VAL.
  Comments    : Allow different orders of edges.
                Takes time proportional to the number of nodes times the
                  number of edges.
		Getting an error here indicates a problem with 
		  CatGraph::connect().
***************************************************************************/
static void check_edge_labels(const CGraph& cGraph)
{
  NodePtr nodePtr;

  forall_nodes(nodePtr, cGraph) {
     if (cGraph.outdeg(nodePtr) != 0) {
	EdgePtr edgePtr;
	Array<Category> edgeNumArray(cGraph.outdeg(nodePtr));
	int edgeCount = 0;
	// Put all the infomation of edges to edgeNumArray
	forall_adj_edges(edgePtr, nodePtr) {
	   edgeNumArray[edgeCount] = cGraph.inf(edgePtr)->num();
	   edgeCount++;
	}
	edgeNumArray.sort();
	ASSERT(edgeCount == cGraph.outdeg(nodePtr));
	ASSERT(edgeCount == edgeNumArray.size());
	if (edgeNumArray[0] != UNKNOWN_CATEGORY_VAL &&
	    edgeNumArray[0] != FIRST_CATEGORY_VAL)
	   err << "CatGraph.c::check_edge_labels: Illegal first edge label: " 
	       << edgeNumArray[0] << ".  Should be "
	       << FIRST_NOMINAL_VAL << " or " << UNKNOWN_CATEGORY_VAL
	       << fatal_error;
	for (edgeCount = 1; edgeCount < edgeNumArray.size(); edgeCount++) {
	   if (edgeNumArray[edgeCount] != edgeNumArray[edgeCount-1] + 1)
	      err << "CatGraph.c::check_edge_labels: Illegal edge label: " 
		  << edgeNumArray[edgeCount] << ".  Expecting value: "
		  << edgeNumArray[edgeCount-1]+1 << fatal_error;
	}
     }
  }
}


const MString distDispHelp = "This option specifies whether to display the "
  "distribution of instances of the nodes in the graph while displaying. ";
const Bool defaultDistDisp = FALSE;
/***************************************************************************
  Description : Converts the representation of the graph to dot format
                and directs it to the passed in stream.
  Comments    : Runs in O(e) time.
***************************************************************************/
void CatGraph::convertToDotFormat(MLCOStream& stream,
				  const DisplayPref& pref) const
{
   static Bool displayDistr = 
      get_option_bool("DIST_DISP",
		      defaultDistDisp, distDispHelp, TRUE);

   // send header and open brace to stream.
   stream << "/* Machine generated dot file */\n" << endl
	  << "digraph G { \n" << endl;
   
   // Preferences that only make sense for the Postscript Display
   if (pref.preference_type() == DisplayPref::DotPostscriptDisplay) {
      process_DotPoscript_preferences(stream, pref);
   }

   // We add each node to the dot output.
   NodePtr v; 
   forall_nodes(v,cGraph) {
      stream << "/*  node "<< index(v)<< ":  */" << endl;
      
      stream << "node_" << index(v) <<" [label=\""
	     << get_categorizer(v).description();

      if (displayDistr)
	 stream << "\\n" << get_categorizer(v).get_distr();
	
      stream << "\"]" << endl;
      
      // For each edge, we add a line to the dot file. (use Leda call)
      EdgePtr e;
      forall_adj_edges(e,v) {
	 stream << "node_" << index(v) << "->" << "node_"
		<< index(cGraph.target(e)) << " [label=\""
		<< cGraph.inf(e)->description() << "\"] " << endl;
      }
      ASSERT (v != NULL);
   }
   ASSERT (v == NULL);
   stream << '}' << endl;
}




/***************************************************************************
  Description : Pops up a dotty window with graph.
  Comments    : This method applies only to XStream MLCOStream type.
***************************************************************************/
void CatGraph::process_XStream_display(const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::DotGraphDisplay) {
      MString tmpFile = get_temp_file_name() + ".dot";
      MLCOStream tempfile(tmpFile);
      convertToDotFormat(tempfile, dp);
      tempfile.close();
      if(system(get_env_default("DOTTY", "dotty") + " " + tmpFile))
	 cerr << "CatGraph::display: Call to dotty returns an error.\n";
      remove_file(tmpFile);  // delete the temporary file
   } else 
       err << "invalid combination of display requested, Xstream is "
       "only valid with DotGraphDisplay" << fatal_error;

}


/***************************************************************************
  Description : Employ dot to create a postscript file of the
                receiving graph in a graphical form.
  Comments    : This method applies only to the DotPostscript DisplayPref.
***************************************************************************/
void CatGraph::process_DotPostscript_display(MLCOStream& stream,
				     const DisplayPref& dp) const
{
   MString tmpfile1 = get_temp_file_name();   
   // need to put this is a logical block so that the file is closed
   {
      MLCOStream tempfile(tmpfile1);      
      convertToDotFormat(tempfile, dp);
   }

   MString tmpfile2 = get_temp_file_name();
   if(system(get_env_default("DOT", "dot") + " -Tps " + tmpfile1 +
	     " -o " + tmpfile2))
      cerr << "CatGraph::display: Call to dot failed." << endl; 
   stream.include_file(tmpfile2);     // this feeds the correct output

   remove_file(tmpfile1); remove_file(tmpfile2);
}


/***************************************************************************
  Description : Employ dot to create a postscript file of the
                receiving graph via a dot description.
  Comments    : This method applies only to the DotGraph DisplayPref.
***************************************************************************/
void CatGraph::process_DotGraph_display(MLCOStream& stream,
				     const DisplayPref& dp) const
{
   MString tmpfile1 = get_temp_file_name();
   // need to put this is a logical block so that the file is closed
   {
      MLCOStream tempfile(tmpfile1);      
      convertToDotFormat(tempfile, dp);
   }
   stream.include_file(tmpfile1);   // this feeds the correct output
                                    // to the correct stream.
   remove_file(tmpfile1);
}


/***************************************************************************
  Description : Level 1 checks that all nodes have a categorizer other than
                  BadCategorizer.
		Level 1 checks that graph is acyclic.
		Level 0 does Level 1 checks and also checks that the edge 
		  labels are consecutive for each node.
  Comments    : 
***************************************************************************/
void CatGraph::OK(int level) const
{
  switch (level) {
  case 0:
    check_edge_labels(cGraph);
  
  // Braces are needed for instance of node_array to be declared
  case 1: {
    node_array<int> ord(cGraph); // required argument for TOPSORT
    if (!TOPSORT(cGraph, ord))
      err << "CatGraph::OK: The graph contains a cycle" << fatal_error;

    cGraph.OK(level);
    break;
  }
  default:
    err << "CatGraph::OK: Invalid level: " << level << fatal_error;
  }
}


/***************************************************************************
  Description : Constructors
  Comments    : The third constructor uses the passed in CGraph for
                  all operations and it must stay intact.
***************************************************************************/
CatGraph::CatGraph() : cGraph(*(new CGraph))
{
  graphAlloc = TRUE;
}

CatGraph::CatGraph(NodeInfo*& prototype)
   : cGraph(*(new CGraph(prototype)))
{
  graphAlloc = TRUE;
}

CatGraph::CatGraph(CGraph& aGraph) : cGraph(aGraph)
{
  graphAlloc = FALSE;
}



/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : We do not use the standard issue copy constructor because
                     we want to know exactly when we call the copy
		     constructor.  
		  This will have ownership of the its CGraph.
***************************************************************************/
CatGraph::CatGraph(const CatGraph& source, CtorDummy /*dummyArg*/)
   : cGraph(*(new CGraph(source.cGraph, ctorDummy)))
{
   if (this == &source)
      err << "CatGraph::CatGraph(CatGraph&, int): cannot call copy "
	     "constructor using 'this' pointer as the source"
	 << fatal_error;   
   graphAlloc = TRUE;
}


/***************************************************************************
  Description : If CatGraph owns the CGraph, deletes all NodeInfo,
                  AugCategories and the CGraph.
  Comments    : All nodes must have had Categorizer set already.
***************************************************************************/
CatGraph::~CatGraph()
{
   DBG(OK(0));
   if (graphAlloc) {
      delete &cGraph;
   }
}


/***************************************************************************
  Description : get_graph() returns the graph which may be used by other
                  routines.
  Comments    :
***************************************************************************/
CGraph& CatGraph::get_graph() const
{
   return cGraph;
}


/***************************************************************************
  Description : Allocates new node.  Sets categorizer to badCategorizer. 
  Comments    : Categorizer MUST be set before desctructor is called.
                Actual type of nodeInfo is the same as the type of the
		  prototype stored in the cGraph.
***************************************************************************/
NodePtr CatGraph::create_node(int level)
{
   NodeInfo* nodeInfo = cGraph.get_prototype()->create_my_type(level);
   return cGraph.new_node(nodeInfo);
}


/***************************************************************************
  Description : Allocates new node with the given level.  Sets node to point
                  to given categorizer.
  Comments    : Gets ownership of the Categorizer.
***************************************************************************/
NodePtr CatGraph::create_node(Categorizer*& cat, int level)
{
   NodeInfo* nodeInfo = cGraph.get_prototype()->create_my_type(level);
   nodeInfo->set_categorizer(cat);
   ASSERT(cat == NULL); // nodeInfo gets ownership
   return cGraph.new_node(nodeInfo);
}


/***************************************************************************
  Description : Sets the node to point to the given Categorizer.
  Comments    : Gets ownerships of Categorizer.
                Will cause fatal_error if called more than once for a node.
		Cannot set a node to have a BadCategorizer.
***************************************************************************/
void CatGraph::set_categorizer(const NodePtr nodePtr, Categorizer*& cat)
{
  DBG(check_node_in_graph(nodePtr, TRUE));
  NodeInfo* nodeInfo = cGraph.inf(nodePtr);
  nodeInfo->set_categorizer(cat);
  ASSERT(cat == NULL);
}


/***************************************************************************
  Description : Creates a directed edge from node "from" to node "to".
                Assigns the edge the value "category".
  Comments    : The category given must be the category following the
                  category for the previous edge. 
		The first edge must have label UNKNOWN_CATEGORY_VAL or
                   FIRST_CATEGORY_VAL
		Gets ownership of AugCategory.
***************************************************************************/
void CatGraph::connect(const NodePtr from, const NodePtr to, 
		       AugCategory*& edgeLabel)
{
  DBGSLOW(check_node_in_graph(from, TRUE);
	  check_node_in_graph(to, TRUE));
  if (cGraph.outdeg(from) == 0 && *edgeLabel != FIRST_CATEGORY_VAL && 
      *edgeLabel != UNKNOWN_CATEGORY_VAL)
    err << "CatGraph::connect: The first edge must have label "
        << FIRST_CATEGORY_VAL << " or "
	<< UNKNOWN_CATEGORY_VAL << ".  Given label was " << edgeLabel->num()
	<< fatal_error;
  if (cGraph.outdeg(from) != 0 &&
      *edgeLabel != *cGraph.inf(cGraph.last_adj_edge(from)) + 1)
    err <<"CatGraph::connect: Edge label " 
	<< *cGraph.inf(cGraph.last_adj_edge(from)) + 1 
	<< " must follow edge label " 
	<< cGraph.inf(cGraph.last_adj_edge(from))->num()
	<< "; got edge label " << edgeLabel->num() << fatal_error;
  cGraph.new_edge(from, to, edgeLabel);
  edgeLabel = NULL;
}

/***************************************************************************
  Description : Returns the number of children for the given node.
  Comments    : 
***************************************************************************/
int CatGraph::num_children(const NodePtr parent) const
{
  DBGSLOW(check_node_in_graph(parent, TRUE));
  return cGraph.outdeg(parent);
}


/***************************************************************************
  Description : Returns the node that is the child of the given node 
                  along the arc with the given edgeLabel.
  Comments    :
***************************************************************************/
NodePtr CatGraph::get_child(const NodePtr parent, 
				  const AugCategory& edgeLabel) const
{
  DBGSLOW(check_node_in_graph(parent, TRUE));
  cGraph.init_adj_iterator(parent);
  EdgePtr edgePtr;
  while (cGraph.next_adj_edge(edgePtr, parent)) {
     if (cGraph.inf(edgePtr)->num() == edgeLabel.num()) {
	DBG(if (*cGraph.inf(edgePtr) != edgeLabel)
	    err << "CatGraph::get_child: Edges match in number but not"
                   " in name '" << *cGraph.inf(edgePtr) << "' versus '"
                   << edgeLabel << "'" << fatal_error);
      return cGraph.target(edgePtr);
     }
  }
  
  err << "CatGraph::get_child: Node does not have an edge labelled '" 
      << edgeLabel.description() << "' (" << edgeLabel.num() << ')'
      << fatal_error;

  return NULL; // avoid compiler warning
}


/***************************************************************************
  Description : Prints a representation of the CatGraph, using the
                  Categorizer descriptions to label the nodes.
  Comments    : Takes into account the different combinations of streams
                  and display preferences.  See DisplayPref.c for more
		  details with regards to valid combinations, functionality,
		  and options.
		For all combinations implemented so far, the time is O(e),
		  where e is the number of edges.
***************************************************************************/
void CatGraph::display(MLCOStream& stream, const DisplayPref& dp) const
{
   // XStream is a special case--the only option so far where you don't
   // just send something to the MLCOStream.
   if (stream.output_type() == XStream) {
      process_XStream_display(dp);  
      return;
   }
   
   // Other cases are depend only on DisplayPreference
   switch (dp.preference_type()) {
      
   case DisplayPref::ASCIIDisplay:
      // Note that we're calling get_fstream and not get_stream to avoid
      //   overflow when cout is auto-wrapped (since it's a strstream).
      //   This means that there is no wrapping here.
      cGraph.print(stream.get_fstream());   // this is Leda's print routine
      stream << flush;
      break;
      
   case DisplayPref::DotPostscriptDisplay:
      process_DotPostscript_display(stream, dp);
      break;

   case DisplayPref::DotGraphDisplay:
      process_DotGraph_display(stream,dp);
      break;
      
   default:
      err << "CatGraph::display: Unrecognized output type: " <<
	 stream.output_type() << fatal_error;
   }  // end switch
}

// Define operator<< for display()
DEF_DISPLAY(CatGraph)


/***************************************************************************
  Description : Returns contents of node.
  Comments    : 
***************************************************************************/
const Categorizer& CatGraph::get_categorizer(const NodePtr nodePtr) const
{
  DBGSLOW(check_node_in_graph(nodePtr, TRUE));
  return cGraph.inf(nodePtr)->get_categorizer();
}

int CatGraph::level(const NodePtr nodePtr) const
{
   return cGraph[nodePtr]->level();
}

/***************************************************************************
  Description : Returns true if all of the nodes in the graph can be reached
                  from the given node.
  Comments    : 
***************************************************************************/
Bool CatGraph::check_reachable(const NodePtr source, 
			       Bool fatalOnFalse)const
{
  DBG(check_node_in_graph(source, TRUE));
  node_array<bool> reached(cGraph);  // required argument to DFS
  NodePtr nodePtr;
  forall_nodes(nodePtr, cGraph)
    reached[nodePtr] = false;
  list<NodePtr> nodePtrList= DFS(cGraph, source, reached);
  if (nodePtrList.length() != cGraph.number_of_nodes()) {
    if (fatalOnFalse) 
      err << "CatGraph::check_reachable: Not all nodes were reachable from "
	  << get_categorizer(source).description() << fatal_error;
    return FALSE;
  } else
    return TRUE;
}


/***************************************************************************
  Description : Returns true if the graph is a tree.
                We use (c) => (a) below.
  Comments    : Theorem: Let G(V,E) be a finite graph and n=|V|.  The 
                         following three conditions are equivalent:
			 (a) G is a tree.
			 (b) G is circuit-free and has n-1 edges.
			 (c) G is connected and has n-1 edges.

		Source: Graph Algorithms / Shimon Even, Computer Science 
		          Press. 1979
***************************************************************************/
Bool CatGraph::check_tree(const NodePtr root, Bool fatalOnFalse)const
{
   check_reachable(root, fatalOnFalse);
   if (cGraph.number_of_edges() != cGraph.number_of_nodes() - 1) {
      if (fatalOnFalse)
	 err << "CatGraph::check_tree: A tree with " <<cGraph.number_of_nodes()
	  << " nodes must have " << cGraph.number_of_nodes() - 1 
	  << " edges.  This has " << cGraph.number_of_edges() << " edges" 
	  << fatal_error;
    return FALSE;
  } else
    return TRUE;
}


/***************************************************************************
  Description : Returns TRUE if the NodePtr points to a node that is in the
                  CatGraph.  Otherwise, returns FALSE
  Comments    :
***************************************************************************/
Bool CatGraph::check_node_in_graph(const NodePtr nodePtr, 
				   Bool fatalOnFalse)const
{
  NodePtr nodePtr2;
  forall_nodes(nodePtr2, cGraph) {
    if (nodePtr == nodePtr2)
      return TRUE;
  }
  if (fatalOnFalse)
    err << "CatGraph::check_node_in_graph: The given node "
	<< nodePtr << " is not in this graph" << fatal_error;
  return FALSE;
}


/***************************************************************************
  Description : The number of nodes/leaves in the CatGraph.
  Comments    : Assumes that the CatGraph owns all of the nodes on the CGraph.
***************************************************************************/
int CatGraph::num_nodes() const
{
   return cGraph.number_of_nodes();
}

int CatGraph::num_leaves() const
{
   return cGraph.num_leaves();
}


/***************************************************************************
  Description : Returns a pointer to the AugCategory at the indicated label.
  Comments    :
***************************************************************************/
const AugCategory* CatGraph::get_edge(const NodePtr nodePtr,
				      Category edgeCategory) const
{
  DBGSLOW(check_node_in_graph(nodePtr, TRUE));
  cGraph.init_adj_iterator(nodePtr);
  EdgePtr edgePtr;
  while (cGraph.next_adj_edge(edgePtr, nodePtr))
     if (cGraph.inf(edgePtr)->num() == edgeCategory)
      return cGraph.inf(edgePtr);
  
  err << "CatGraph::get_edge: Node does not have an edge with category " 
      << edgeCategory << fatal_error;

  return NULL; // avoid compiler warning
}

