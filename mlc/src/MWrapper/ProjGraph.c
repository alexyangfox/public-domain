// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The ProjGraph class uses the LEDA graph class to implement
                   a graph of ProjSets.  Functions in ProjGraph attempt to
		   color the graph, which ultimately results in a tighter
		   grouping of Projections into ProjSets.  The graph can
		   be displayed/output in dot format if desired.
  Assumptions  :
  Comments     :
  Complexity   : The worst case time cost of coloring is O(N*N*C), where
                   N is the number of nodes in the graph, and C is the
		   number of colors needed.
  Enhancements : Allow a more modular coloring algorithm.
                 Use a better method for tie-breaking on which color to pick
		   when there is a choice.
  History      : Dan Sommerfield                                    11/11/94
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <ProjGraph.h>
#include <mlcIO.h>
#include <ProjLevel.h>


RCSID("MLC++, $RCSfile: ProjGraph.c,v $ $Revision: 1.2 $")

/***************************************************************************
  Description : Gets the preferences from the DisplayPref class.
  Comments    : This method applies only to DotPostscript display type.
***************************************************************************/
void ProjGraph::process_DotPoscript_preferences(MLCOStream& stream,
					       const DisplayPref& pref) const
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
  Description : Protected Functions
  Comments    :
***************************************************************************/
void ProjGraph::free()
{
   NodePtr nodePtr;
   forall_nodes(nodePtr, *this)
      delete inf(nodePtr);
   clear();
}

/***************************************************************************
  Description : Converts the representation of the graph to dot format
                and directs it to the passed in stream.
  Comments    : Runs in O(e) time.
***************************************************************************/
void ProjGraph::convertToDotFormat(MLCOStream& stream,
					const DisplayPref& pref) const
{
   // send header and open brace to stream.
   stream << "/* Machine generated dot file */\n" << endl
	  << "graph G { \n" << endl;
   
   // Preferences that only make sense for the Postscript Display
   if (pref.preference_type() == DisplayPref::DotPostscriptDisplay) {
      process_DotPoscript_preferences(stream, pref);
   }

   // We add each node to the dot output.
   NodePtr v; 
   forall_nodes(v,*this) {
      stream << "/*  node "<< index(v) << ":  */" << endl;
      
      stream << "node_" << index(v) <<" [label=\"";
      get_proj_set(v).display_for_graph(stream);
      stream << "\""
	     << graph_options(v)
	     << "]" << endl;
      // For each edge, we add a line to the dot file. (use Leda call)
      EdgePtr e;
      forall_adj_edges(e,v) {
	 stream << "node_" << index(v) << "--" << "node_"
		<< index(target(e)) << " [label=\""
		<< MString(inf(e),0) << "\"] " << endl;
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
void ProjGraph::process_XStream_display(const DisplayPref& dp) const
{
   DBG(if (dp.preference_type() != DisplayPref::DotGraphDisplay)
          err << "ProjGraph::process_XStream_display:  invalid "
              "combination of display requested, Xstream is only "
              "valid with DotGraphDisplay" << fatal_error);

   MString tmpfile = get_temp_file_name();
   // need to put this is a logical block so that the file is closed
   // make call to dotty.  If this is a common requirement, possibly
   // create some method with concomitant comment on correct usage
   {
      MLCOStream tempfile(tmpfile);
      convertToDotFormat(tempfile, dp);
   }
   
   if (system(get_env_default("DOTTY", "dotty") + " " + tmpfile))
      err << "ProjGraph::display: Call to dotty returns an error"
	  << fatal_error;
   
   remove_file(tmpfile);  // delete the temporary file
}


/***************************************************************************
  Description : Employ dot to create a postscript file of the
                receiving graph in a graphical form.
  Comments    : This method applies only to the DotPostscript DisplayPref.
***************************************************************************/
void ProjGraph::process_DotPostscript_display(MLCOStream& stream,
						   const DisplayPref& dp) const
{
   MString tmpfile1 = get_temp_file_name();   
   // need to put this is a logical block so that the file is closed
   {
      MLCOStream tempfile(tmpfile1);      
      convertToDotFormat(tempfile, dp);
   }

   MString tmpfile2 = get_temp_file_name();
   if (system(get_env_default("DOT", "dot") + " -Tps " + tmpfile1 +
	     " -o " + tmpfile2))
      err << "ProjGraph::display: Call to dot failed." << fatal_error;
   stream.include_file(tmpfile2);     // this feeds the correct output

   remove_file(tmpfile1); remove_file(tmpfile2);
}


/***************************************************************************
  Description : Employ dot to create a postscript file of the
                receiving graph via a dot description.
  Comments    : This method applies only to the DotGraph DisplayPref.
***************************************************************************/
void ProjGraph::process_DotGraph_display(MLCOStream& stream,
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
  Description : Initialize the coloring algorithm by resetting the list
                  of available colors at each node.
  Comments    : 
***************************************************************************/
void ProjGraph::init_coloring(int maxColors)
{
   for(int i=nodeArray.low(); i<=nodeArray.high(); i++)
      get_proj_set(nodeArray[i]).reset(maxColors);
}

/***************************************************************************
  Description : Assigns a color to a node.  In the process, this function
                  declares this color a NOGOOD in all adjacent nodes.
  Comments    : 
***************************************************************************/
void ProjGraph::assign_color(NodePtr n, int color)
{
   // set the color at this node now
   get_proj_set(n).set_color(color);

   // iterate over all adjacent nodes, adding this color as a NOGOOD
   // to each.
   NodePtr v;
   forall_adj_nodes(v, n)
      get_proj_set(v).add_nogood(color);
}


/***************************************************************************
  Description : Searches through the graph to find the next most appropriate
                  node on which to run the coloring algorithm.  We pick
		  the most constrained node (the one with the fewest colors
		  left), and break ties by picking a node of maximum degree.
  Comments    : 
***************************************************************************/
NodePtr ProjGraph::get_node_to_color()
{
   // find node with least number of colors left.  If more than one,
   // choose the node with the highest indegree
   NodePtr bestNode = nodeArray[nodeArray.low()];
   for(int i=nodeArray.low()+1; i<=nodeArray.high(); i++) {

      // see if this node is better than the best node
      // only set as best node if it is not colored
      if(get_proj_set(nodeArray[i]).get_color() < 0) {
	 if(get_proj_set(bestNode).get_color() >= 0)
	    bestNode = nodeArray[i];
	 else if(get_proj_set(nodeArray[i]).colors_left() <
	    get_proj_set(bestNode).colors_left())
	    bestNode = nodeArray[i];
	 else if(get_proj_set(nodeArray[i]).colors_left() ==
		 get_proj_set(bestNode).colors_left() &&
		 outdeg(nodeArray[i]) > outdeg(bestNode))
	    bestNode = nodeArray[i];
      }
   }

   // if the "best node" has already been colored, then return NULL
   // (this only happens if there are no nodes remaining to be
   // colored)
   if(get_proj_set(bestNode).get_color() >= 0)
      return NULL;

   return bestNode;
}



/***************************************************************************
  Description : Constructor/Destructor.
  Comments    :
***************************************************************************/
ProjGraph::ProjGraph()
   : GRAPH<ProjSet*,int>(),
     nodeArray(0)
{
}

ProjGraph::~ProjGraph()
{
   free();
}

/***************************************************************************
  Description : Set the dot-output options for this node.  Uses a static
                  array of colors for nice color output.
  Comments    : This array is not an MLC++ array to make it easier to
                  initialize.
***************************************************************************/
// array of color names
static char *colName[] = {
   "red", "green", "blue", "yellow", "cyan", "magenta",
   "violet", "darkgreen", "indigo", "gold", "snow", "gray",
   "khaki", "seagreen", "lightskyblue", "palevioletred", "pink", "darkorange",
   "whitesmoke", "lightgray", "darkgray", "slateblue", "sienna", "tan"
};

MString ProjGraph::graph_options(NodePtr v) const
{
   int color = get_proj_set(v).get_color();
   MString colorName;
   if(color < 24)
      colorName = MString(colName[color]);
   else
      colorName = MString("color") + MString(color, 0);
   if(color < 0)
      return "";
   else
      return MString("style=filled,color=") + colorName;
}


/***************************************************************************
  Description : Graph Manipulation
  Comments    :
***************************************************************************/
NodePtr ProjGraph::create_node(ProjSet*& proj)
{
   NodePtr nodePtr = new_node(proj);
   proj = NULL;
   nodeArray[nodeArray.size()] = nodePtr;
   return nodePtr;
}

void ProjGraph::connect(NodePtr from, NodePtr to, int edge)
{
   new_edge(from, to, edge);
}


/***************************************************************************
  Description : Adds edges to the graph between any two nodes which have
                  conflicts.  The degree of the edge indicates the number
		  of conflicts.
  Comments    : 
***************************************************************************/
void ProjGraph::add_conflict_edges()
{
   int i,j;
   for(i=nodeArray.low(); i<=nodeArray.high(); i++)
      for(j=i+1; j<=nodeArray.high(); j++) {
	 int num = get_proj_set(nodeArray[i]).conflicts_with(
	    get_proj_set(nodeArray[j]));
	 if(num)
	    connect(nodeArray[i], nodeArray[j], num);
      }
}

/***************************************************************************
  Description : Accessor Functions
  Comments    :
***************************************************************************/
ProjSet& ProjGraph::get_proj_set(NodePtr node) const
{
   return *inf(node);
}


/***************************************************************************
  Description : Perform a graph coloring with maximum maxColors.
                Returns TRUE if coloring succeeded, FALSE if not.
  Comments    : Assumes make_undirected() has been called on the graph.
***************************************************************************/
Bool ProjGraph::color(int maxColors)
{
   init_coloring(maxColors);
   NodePtr nodeToColor;
   while(nodeToColor = get_node_to_color()) {
      LOG(3, "try to color: " << get_proj_set(nodeToColor) << endl);
      ProjSet& pSet = get_proj_set(nodeToColor);
      int color = pSet.get_first_ok_color();
      LOG(3, "trying color " << color << endl);
      if(color < 0) {
	 return FALSE;  // failure--couldn't find a color
      }
      assign_color(nodeToColor, color);
   }

   // log the whole list
   LOG(3, "node list: " << endl);
   for(int i=nodeArray.low(); i<=nodeArray.high(); i++)
      LOG(3, get_proj_set(nodeArray[i]) << endl);
      
   return TRUE;
}


/***************************************************************************
  Description : Display Functions
  Comments    :
***************************************************************************/
void ProjGraph::display(MLCOStream& stream, const DisplayPref& dp) const
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
      print(stream.get_stream());    // this is Leda's print routine
      stream << flush;
      break;

   case DisplayPref::DotPostscriptDisplay:
      process_DotPostscript_display(stream, dp);
      break;

   case DisplayPref::DotGraphDisplay:
      process_DotGraph_display(stream,dp);
      break;

   default:
      err << "ProjGraph<>::display: Unrecognized output type: "
          << stream.output_type() << fatal_error;
   }
}

DEF_DISPLAY(ProjGraph)


/***************************************************************************
  Description : Prints a readable representation of the state.
  Comments    : Required for LEDA
***************************************************************************/
void Print(const ProjSet*& nodeInfo, ostream& stream)
{
   MLCOStream tmpStream(stream, FileStream);
   tmpStream << *nodeInfo;
}

/***************************************************************************
  Description : Read a graph from another representation.
  Comments    : Required for LEDA template instantiation, but we don't
                  implement.
***************************************************************************/
void Read(const ProjSet*& /*nodeInfo*/, istream& /*stream*/)
{
   err << "ProjGraph::Read: sorry, not implemented" << fatal_error;
}


/***************************************************************************
  Description : Builds an array of ProjSets representing the contents of
                  the graph.  Each ProjSet represents one color.
  Comments    : Takes ownership of the graph's ProjSets.
***************************************************************************/
Array<ProjSet *> *ProjGraph::get_colored_sets(int max)
{
   // build a NULL array to hold sets
   Array<ProjSet *> newSets(0, max, NULL);

   // go through the old sets and place into the new arrays depending on
   // color, creating new sets as needed.
   int actNumSets = 0;
   NodePtr n;
   forall_nodes(n, *this) {
      ProjSet *projSet = inf(n);
      int index = projSet->get_color();
      if(newSets[index])
	 newSets[index]->add(projSet);
      else {
	 newSets[index] = new ProjSet(0);
	 newSets[index]->add(projSet);
	 actNumSets++;
      }
      assign(n, NULL); // indicate that graph loses ownership of node
   }

   // check consistency; the first actNum sets should be valid and the
   // rest should be NULL
   DBG(
      for(int i=0; i<newSets.size(); i++) {
	 if(i < actNumSets)
	    ASSERT(newSets.index(i));
	 else
	    ASSERT(!newSets.index(i));
      }
   );

   // we actually want to return a smaller array.  Create one and copy in
   // the elements
   Array<ProjSet *> *retSets = new Array<ProjSet *>(actNumSets);
   for(int whichSet = 0; whichSet < actNumSets; whichSet++)
      retSets->index(whichSet) = newSets.index(whichSet);
   return retSets;
}


