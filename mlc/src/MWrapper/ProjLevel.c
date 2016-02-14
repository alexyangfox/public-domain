
// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The ProjLevel class contains the structure used to manage
                   a single level in an OODG as it is being built.
		   Because this level must belong to an OODG, it has a
		   conceptual SOURCE (a higher level which projects into
		   this level), and DESTINATION (a lower level into which
		   this level projects).
  Assumptions  : 
  Comments     : This class relies heavily on the supporting classes
                   ProjSet and Projection.  Many of the main methods
		   of ProjLevel actually filter down through these other
		   classes.  Also, some of the level information,
		   such as DESTINATION, is distributed throughout the
		   other classes to help them work better.
  Complexity   : Not computed.
  Enhancements :
  History      : Dan Sommerfield                                   11/18/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <ProjLevel.h>
#include <ProjGraph.h>
#include <AttrCat.h>
#include <ConstCat.h>

#include <LEDA/graph.h>

RCSID("MLC++, $RCSfile: ProjLevel.c,v $ $Revision: 1.2 $")


int ProjLevel::colorIndex = 0;


/***************************************************************************
  Description : OK function
  Comments    :
***************************************************************************/
void ProjLevel::OK() const
{
   // check feature sets
   instIndices.OK();
   projIndices.OK();
   notInstIndices.OK();

   // make sure feature sets "add up"
   DBGSLOW(
      FeatureSet fullSet(levelSchema);
      ASSERT(fullSet.contains(instIndices));
      ASSERT(fullSet.contains(projIndices));
      ASSERT(fullSet.contains(notInstIndices));
      ASSERT(notInstIndices.contains(projIndices));
      DynamicArray<int> builder(0);
      builder.append(instIndices.get_feature_numbers());
      builder.append(notInstIndices.get_feature_numbers());
      FeatureSet newFullSet(builder);
      ASSERT(newFullSet == fullSet);
   );
      
   // run a consistency check on each ProjSet in the level
   DBGSLOW(
      for(int i=0; i<projSets.size(); i++) {
	 projSets.index(i)->OK();
	 projSets.index(i)->check_consistency(levelSchema,
					      levelSource,
					      levelDest);
      }
   );
}


   
/***************************************************************************
  Description : Construct a toplevel ProjLevel from a single bag.
                  Its destination defaults to NULL, meaning that this
		  level will project directly to the leaves of the OODG.
  Comments    :
***************************************************************************/
ProjLevel::ProjLevel(const InstanceBag& bag, int displayLev) :
   projSets(1),
   projIndices(bag.get_schema()),
   notInstIndices(bag.get_schema()),
   levelSchema(bag.get_schema()),
   levelSource(NULL),
   levelDest(NULL),
   displayLevel(displayLev)
{
   // the bag must have at least one instance
   if(bag.num_instances() == 0)
      err << "ProjLevel::ProjLevel: bag has no instances" << fatal_error;

   // use the first instance in the bag as the main instance of the toplevel
   // projection
   Pix p = bag.first();
   const InstanceRC& mainInst = bag.get_instance(p);

   Projection *proj =
      new Projection(bag, *this, mainInst, NULL);

   // make a ProjSet out of this Projection and place it in the projSets array
   // the projSet gets ownership.
   projSets[0] = new ProjSet(proj, displayLevel);

   DBG(OK());
}

/***************************************************************************
  Description : Construct a toplevel ProjLevel from a single bag
                  Allow specification of projIndices as separate from
		  origProjIndices.
		The purpose of this function is to allow creation of a
		  toplevel ProjLevel where only some of the features
		  are relevent.
  Comments    :
***************************************************************************/
ProjLevel::ProjLevel(const InstanceBag& bag, const FeatureSet& projSel,
		     int displayLev)
   : projSets(1),
     projIndices(projSel, ctorDummy),
     notInstIndices(bag.get_schema()),
     levelSchema(bag.get_schema()),
     levelSource(NULL),
     levelDest(NULL),
     displayLevel(displayLev)
{
   // the bag must have at least one instance
   if(bag.num_instances() == 0)
      err << "ProjLevel::ProjLevel: bag has no instances" << fatal_error;
   
   // use the first instance in the bag as the main instance of the toplevel
   // projection
   Pix p = bag.first();
   const InstanceRC& mainInst = bag.get_instance(p);

   Projection *proj =
      new Projection(bag, *this, mainInst, NULL);

   // make a ProjSet out of this Projection and place it in the projSets array
   projSets[0] = new ProjSet(proj, displayLevel);

   DBG(OK());
}



/***************************************************************************
  Description : Construct a ProjLevel from an existing level by projecting
                  down and interpolating.  projectBy specifies attributes
		  to project down on.  Lower level allows optional
		  specification of a lower level to interpolate between.
		If a lower level is specified, then the projection will
		  be directed toward that level.  This lower level will
		  also be used to constrain the coloring process.
  Comments    :
***************************************************************************/
ProjLevel::ProjLevel(const ProjLevel& level, const FeatureSet& projectBy,
		     const ProjLevel *lower) :
   projSets(0),
   levelSchema(level.levelSchema),
   levelSource(&level),
   levelDest(lower),
   displayLevel(level.displayLevel)
{
   // compute the new feature sets (inst, proj and not-inst)
   projectBy.project_down(level.notInstIndices, level.instIndices,
			  notInstIndices, instIndices);

   // if there's a lower level, redirect to that level.  This will
   // effectively change the projected portion.
   if(lower)
      direct_to_level(*lower);
   else {
      level.projIndices.difference(projectBy, projIndices);
      //@@ old version-- may need it again
      //@@ projIndices = notInstIndices;
   }
      

   // go through the original level's projSets, calling project_down
   // on each.  Accumulate the results in this level's projSets.
   for(int i=level.projSets.low(); i<=level.projSets.high(); i++) {
      Array<ProjSet *> *psa = 
	 level.projSets[i]->project_down(projectBy, *this);
      projSets.append(*psa);
      delete psa;
   }

   DBG(OK());
}

/***************************************************************************
  Description : Destructor
  Comments    : We need to call free() for each projSet to free up the
                  projections stored in each one.
***************************************************************************/
ProjLevel::~ProjLevel()
{
   DBG(OK());
   
   for(int i=0; i<projSets.size(); i++)
      delete projSets.index(i);
}


/***************************************************************************
  Description : Builds a ProjGraph representing all conflicts between
                  ProjSets contained in this level
  Comments    :
***************************************************************************/
ProjGraph *ProjLevel::create_conflict_graph()
{
   // create a new graph
   ProjGraph *projGraph = new ProjGraph;

   // add each node now.  Use the projSets we already have
   // note that this takes ownership of the ProjSets away from us
   // so we will truncate our projSets array when we're done.
   for(int i = projSets.low(); i <= projSets.high(); i++) {
      ProjSet *pSet = (ProjSet *)(projSets[i]);  // cast away constness
      projGraph->create_node(pSet);
   }

   // add edges
   projGraph->add_conflict_edges();
   projSets.truncate(0);

   return projGraph;
}

/***************************************************************************
  Description : Rebuilds the ProjSets based on the coloring of the level.
                Num_colors specifies an upper bound on the number of colors.
		  If fewer colors were used, the rebuilding will still
		  succeed.
  Comments    : Call after coloring the level.
***************************************************************************/
void ProjLevel::rebuild_sets(ProjGraph *graph, int num_colors)
{
   // get a new array of projection sets, grouped by color, from the
   // graph.
   Array<ProjSet *> *newSets = graph->get_colored_sets(num_colors);
   projSets.append(*newSets);
   delete newSets;
}
   
/***************************************************************************
  Description : Colors this level using the specified number of colors.
                  If coloring fails, then retry with more colors until it
		  works.  Returns the actual number of colors used.
  Comments    : If the actual number of colors used is below the minimum
                  number of colors to use, this function will return the
		  minimum.
***************************************************************************/
int ProjLevel::color_level(int numColors)
{
   // build the graph first
   ProjGraph *graph = create_conflict_graph();
   graph->make_undirected();

   // color the graph with increasing number of colors until it works
   while(!graph->color(numColors))
      numColors++;

   // dump output in dot format if the option is set
   if(displayLevel) {
      GLOBLOG(2, "Coloring result in color." << colorIndex << ".dot" << endl);
      MLCOStream out(MString("color.") + MString(colorIndex++,0) + ".dot");
      DotGraphPref pref;
      graph->display(out, pref);

      GLOBLOG(2, "Dumping special format graph in graph." << colorIndex-1 <<
	      ".txt" << endl);
      MLCOStream out2(MString("graph.") + MString(colorIndex-1,0) + ".txt");
      special_display(out2);
      rebuild_sets(graph, numColors);
      out2 << "Colors used: " << projSets.size() << endl;
   }
   else {
      // get back the ProjSets for this graph
      rebuild_sets(graph, numColors);
   }

   delete graph;
   return numColors;
}


/***************************************************************************
  Description : Display colored graph in a special format
  Comments    : For output of graphs to external graph-coloring facilities
***************************************************************************/
void ProjLevel::special_display(MLCOStream& out) const
{
   out << projSets.size() << endl;
   for(int i=projSets.low(); i<=projSets.high(); i++) {
      for(int j=i+1; j<=projSets.high(); j++) {
	 if(projSets[i]->conflicts_with(*(projSets[j])))
	    out << i << "  " << j << endl;
      }
   }
}



/***************************************************************************
  Description : Correctly updates the previous level's destinations
  Comments    :
***************************************************************************/
void ProjLevel::update_source(void)
{
   DBG(OK());
   
   // go through all projection sets in this level and update
   // the destination Category of each is just the array index of
   // the projLevel.
   for(int i=projSets.low(); i<=projSets.high(); i++)
      projSets[i]->update_source(i);
}


/***************************************************************************
  Description : Insert the nodes within this level into the provided
                  CatGraph.
  Comments    : We store the node pointers within each ProjSet as they
                  are determined.  We need this to know how to connect
		  later.
***************************************************************************/
void ProjLevel::insert_in_graph(CatGraph& graph)
{
   DBG(OK());
   
   for(int i=projSets.low(); i<=projSets.high(); i++) {
      // create a node for each projSet
      projSets[i]->set_node(graph.create_node());
      projSets[i]->set_id(index(projSets[i]->get_node()));
   }
}

/***************************************************************************
  Description : Connects this ProjLevel to the next level down.
  Comments    : 
***************************************************************************/
void ProjLevel::connect_to_level(CatGraph& graph, ProjLevel& other,
				 const SchemaRC& schema)
{
   // difference this level's proj selector with the next level's
   // proj selector to get the split attribute for this level
   FeatureSet split;
   notInstIndices.difference(other.notInstIndices, split);
   if(split.num_features() != 1)
      err << "ProjLevel::connect_to_level: attempting to connect nonadjacent "
	 "levels" << fatal_error;
   int splitAttr = split.get_feature_numbers().index(0);

   // assign the attribute categorizer to all nodes at this level
   // the nodes get ownership of the categorizers so we have to
   // build a new one each time through the loop
   // after assigning the categorizer, tell each ProjSet to connect
   // into the next level.
   for(int i=projSets.low(); i<=projSets.high(); i++) 
      projSets[i]->connect_to_level(graph, other, splitAttr, schema);
}

/***************************************************************************
  Description : Connects this ProjLevel to a set of leaf nodes which get
                  added automatically by this function.
  Comments    : Only works on the second-to-bottom level ProjLevel
                  (we ASSERT this)
***************************************************************************/
void ProjLevel::connect_to_leaves(CatGraph& graph, const SchemaRC& schema)
{
   ASSERT(projIndices.num_features() == 1);

   int splitAttr = projIndices.get_feature_numbers().index(0);
   const MString& name = schema.attr_name(splitAttr);

   // build an array to hold all the nodes used for the final categories
   const NominalAttrInfo& info = schema.nominal_label_info();
   Array<NodePtr> destArray(UNKNOWN_CATEGORY_VAL, info.num_values()+1);

   // for each possible value, build a node.  Name it with the appropriate
   // value name (using a const categorizer)
   for(int i=destArray.low(); i<=destArray.high(); i++) {
      if(i != UNKNOWN_CATEGORY_VAL) {
	 Categorizer *constCat;
	 const MString& valName = info.get_value(i);
         constCat = new ConstCategorizer(valName,
					 AugCategory(i, valName));
	 destArray[i] = graph.create_node(constCat);

      }
   }

   // connect each ProjSet (node) within this level up to the
   // destinations
   for(i=projSets.low(); i<=projSets.high(); i++)
      projSets[i]->connect_to_leaves(graph, destArray, splitAttr, schema);

   // find nodes without any connections and get rid of them
   // we do so to avoid leaving unreachable leaf nodes around.
   for(i=destArray.low(); i<=destArray.high(); i++) {
      if(graph.get_graph().indeg(destArray[i]) == 0)
	 graph.get_graph().del_node(destArray[i]);
   }
}

/***************************************************************************
  Description : Change the projected instances so that this ProjLevel
                  appears to point to the other level.
  Comments    :
***************************************************************************/
void ProjLevel::direct_to_level(const ProjLevel& other)
{
   ASSERT(notInstIndices.num_features() >= 1);
   
   // difference this level's proj selector with the next level's
   // proj selector.  Make this the new projIndices.
   notInstIndices.difference(other.notInstIndices, projIndices);

   // reset the destination level
   levelDest = &other;

   DBG(OK());
}


/***************************************************************************
  Description : Retrieve toplevel node for a ProjLevel
                Only works on a toplevel ProjLevel (checked)
  Comments    : 
***************************************************************************/
NodePtr ProjLevel::get_toplevel_node() const
{
   if(!is_top_level())
      err << "ProjLevel::get_toplevel_node: not top level" << fatal_error;
   ASSERT(projSets.size() == 1);
   return projSets.index(0)->get_node();
}

      
/***************************************************************************
  Description : Displays header information and all ProjSets in the
                  level.
  Comments    :
***************************************************************************/
void ProjLevel::display(MLCOStream& out) const
{
   // display general level information
   out << "Instance attributes: " << instIndices << endl;
   out << "Project attributes: " << projIndices << endl;
   out << "Not Instance attributes: " << notInstIndices << endl;
   
   // display each projSet in the level
   for(int i = projSets.low(); i <= projSets.high(); i++) {
      out << "Set #" << i << ":" << endl;
      projSets[i]->display(out);
      out << endl;
   }
}
DEF_DISPLAY(ProjLevel);


/***************************************************************************
  Description : Returns TRUE if all the outgoing edges go to the same target.
  Comments    :
***************************************************************************/
static Bool const_node(CGraph &G, NodePtr n)
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
  Description : Find the edge between node p and c. Helper function of
                  prune_graph.
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
  Description : Prune the constant nodes in the graph, catG. Constant nodes
                  means those nodes whose outgoing edges going to the same
		  target.
  Comments    : global function
***************************************************************************/
void prune_graph(CatGraph& catG)
{
   CGraph &G = catG.get_graph();
   Bool pruned;
   do {
      pruned = FALSE;
      NodePtr iterNode = G.first_node();
      while (iterNode) {
	 if (const_node(G, iterNode)) {
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








