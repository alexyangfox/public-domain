// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : 
  Assumptions  : 
  Comments     :                  
  Complexity   : 
  Enhancements : 
  History      : Dan Sommerfield                                    3/7/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <ProjLevel.h>
#include <ProjGraph.h>
#include <COODGInducer.h>
#include <RootCatGraph.h>
#include <ConstCat.h>
#include <RDGCat.h>
#include <AttrCat.h>
#include <GetOption.h>
#include <FeatureSet.h>
#include <math.h>

RCSID("MLC++, $RCSfile: COODGInducer.c,v $ $Revision: 1.2 $")

const MEnum methodMEnum = MEnum("top-down", COODGInducer::topDown) <<
                          MEnum("bottom-up", COODGInducer::bottomUp) <<
                          MEnum("recursive", COODGInducer::recursive);
const MString methodHelp = "This option specifies the method to use for "
  "constructing the OODG.";

const MEnum displayMEnum = MEnum("none", COODGInducer::dispNone) <<
                           MEnum("normal", COODGInducer::dispNormal) <<
                           MEnum("full", COODGInducer::dispFull);
const MString displayHelp = "This option specifies how much (if any) "
  "information to display in a graph";

const MString pruneHelp = "This option specifies whether or not to "
  "remove nodes from the graph if all of their edges point to the "
  "same destination.";

const MEnum splitMEnum = MEnum("top", COODGInducer::splitTop) <<
                         MEnum("bottom", COODGInducer::splitBottom) <<
                         MEnum("largest", COODGInducer::splitLargest) <<
                         MEnum("smallest", COODGInducer::splitSmallest);
const MString splitHelp = "This option specifies the criteria to use when "
  "choosing how to split up the levels in recursive COODG.  Top and bottom "
  "are equivalent to the top-down and bottom-up algorithms.";



/***************************************************************************
  Description : OK function.
  Comments    :
***************************************************************************/
void COODGInducer::OK(int level) const
{
   CtrInducer::OK(level);   
}


/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
COODGInducer::COODGInducer(const MString& desc)
   : CtrInducer(desc),
     rdgCat(NULL),
     method(topDown)
{
}


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
COODGInducer::~COODGInducer()
{
   DBG(OK());
   delete rdgCat;
}

/***************************************************************************
  Description : Sets all inducer options according to user choices.
                This includes choice of order vector.
  Comments    :
***************************************************************************/
void COODGInducer::set_user_options(const MString& prefix)
{
   set_method(get_option_enum(prefix + "METHOD", methodMEnum,
			      topDown, methodHelp, FALSE));
   attrOrder.set_user_options(prefix);
   set_display_mode(get_option_enum(prefix + "DISPLAY", displayMEnum,
				    dispNone, displayHelp, TRUE));
   set_prune(get_option_bool(prefix + "PRUNE", TRUE, pruneHelp, TRUE));

   if(method == recursive)
      set_split_type(get_option_enum(prefix + "SPLIT", splitMEnum,
				     splitSmallest, splitHelp, TRUE));
}





void COODGInducer::train_top_down(CatGraph& graph,
				  Array<ProjLevel *>& levels,
				  const FeatureSet& includedSet)
{
   int num = levels.size();

   // build, color, and insert each level into the graph
   levels[0] = new ProjLevel(*TS, includedSet, displayMode == dispFull);
   levels[0]->insert_in_graph(graph);
   LOG(2, "top level:" << endl << *(levels[0]));
   
   for(int i=1; i<num; i++) {
      FeatureSet selector;
      selector.add_feature(attrOrder.get_order().index(i-1));
      levels[i] = new ProjLevel(*(levels[i-1]), selector);
      levels[i]->color_level(10);
      levels[i]->insert_in_graph(graph);
      LOG(2, "level " << i << ":" << endl << *(levels[i]));
   }

   // update sources of all but the first level, so we can
   // connect edges correctly
   for(i=1; i<num; i++)
      levels[i]->update_source();

   // connect edges
   for(i=0; i<num-1; i++)
      levels[i]->connect_to_level(graph, *(levels[i+1]),
				  TS->get_schema());

   // connect last level to leaves
   levels[num-1]->connect_to_leaves(graph, TS->get_schema());
}


void COODGInducer::train_bottom_up(CatGraph& graph,
				   Array<ProjLevel *>& levels,
				   const FeatureSet& includedSet)
{
   int num = levels.size();

   // build the topmost level
   ProjLevel *topLevel = new ProjLevel(*TS, includedSet,
				       displayMode == dispFull);
   LOG(2, "top level:" << endl << *topLevel);

   // for each level from num-1 up to 1, project down by the appropriate
   // piece of the order vector,  Then call update_source on the new
   // level.
   DynamicArray<int> partialVector(0);
   const Array<int>& orderVector = attrOrder.get_order();
   partialVector.append(orderVector);
   for(int i=num-1; i>=0; i--) {
      partialVector.truncate(partialVector.size()-1);

      FeatureSet fSet(partialVector);
      ProjLevel *lower = NULL;
      if(i < num-1)
	 lower = levels[i+1];
      levels[i] = new ProjLevel(*topLevel, fSet, lower);
      levels[i]->color_level(10);
      levels[i]->update_source();
      levels[i]->insert_in_graph(graph);
      LOG(2, "level " << i << ":" << endl << *(levels[i]));
   }
   delete topLevel;
	 
   // connect each level to the next lowest one
   for(i=num-2; i>=0; i--) {
      LOG(2, "Connect " << i << " to " << i+1 << endl);
      levels[i]->connect_to_level(graph, *(levels[i+1]),
				  TS->get_schema());
   }
   LOG(2, "Connect to leaves" << endl);
   levels[num-1]->connect_to_leaves(graph, TS->get_schema());

}

/***************************************************************************
  Description : Recursively build the OODG stored by this Inducer.  The
                  top and bottom parameters specify which portion of the
		  OODG to build.  If top and bottom are adjacent levels,
		  then this function just connects the levels together.
		  The bottom parameter may optionally be NULL, indicating
		  that the category leaves should be used as the bottom
		  level.  The top parameter may never be NULL.
  Comments    : The top level must have already been built to use this
                  function.  It will get rebuilt later, as well.
***************************************************************************/

void COODGInducer::train_recursively(CatGraph& graph,
				     Array<ProjLevel *>& levels,
				     ProjLevel *top,
				     ProjLevel *bottom,
				     Bool topHalf)
{
   int num = levels.size();
   ASSERT(top);
   const Array<int>& orderVector = attrOrder.get_order();
   
   // determine indices of top and bottom.  If bottom is NULL, then set
   // its index to levels.size();
   int topIndex=-1, botIndex=-1;
   for(int i=0; i<num; i++) {
      if(levels.index(i) == top)
	 topIndex = i;
      if(levels.index(i) == bottom)
	 botIndex = i;
   }
   if(!bottom)
      botIndex = num;

   // integrity checks on the indices
   ASSERT(topIndex >= 0);
   ASSERT(topIndex < num);
   ASSERT(botIndex >= 0);
   ASSERT(botIndex <= num);
   ASSERT(topIndex < botIndex);

   // clean up levels between bottom and top indices
   for(i=topIndex+1; i<botIndex; i++) {
      delete levels.index(i);
      levels.index(i) = NULL;
   }
   
   LOG(2, "Recurse on levels " << topIndex << " to " << botIndex << endl);

   // if this is a tophalf of a recursion, then we need to regenerate the
   // top level so that its projecting to the bottom
   FeatureSet fSet;
   if(topHalf) {
      LOG(2, "(top half)" << endl);
      if(bottom)
	 bottom->update_source();
      levels[topIndex] = new ProjLevel(*top, fSet, bottom);
      delete top;
      top = levels[topIndex];
   }

   // if levels are adjacent, then return
   if(botIndex - topIndex == 1)
      return;
   
   // The levels are split apart.  Build all levels between top and
   // bottom by projecting down from top using bottom as a constraint.
   // delete what was there before to avoid memory leaks.
   // use closeness to center as tie-breaker for min and max levels
   int maxLevel = -1;
   int maxSize = -1;
   int minLevel = -1;
   int minSize = INT_MAX;
   Real middle = ((Real)topIndex + (Real)botIndex) * 0.5;
   for(i=topIndex+1; i<botIndex; i++) {
      LOG(2, "Building " << i << ":" << endl);
      delete(levels[i]);
      fSet.add_feature(orderVector.index(i-1));

      levels[i] = new ProjLevel(*top, fSet, bottom);
      levels[i]->color_level(10);
      if(levels[i]->get_width() > maxSize ||
	 levels[i]->get_width() == maxSize &&
	 fabs(i-middle) < fabs(maxLevel-middle)) {
	 maxSize = levels[i]->get_width();
	 maxLevel = i;
      }
      if(levels[i]->get_width() < minSize ||
	 levels[i]->get_width() == minSize &&
	 fabs(i-middle) < fabs(minLevel-middle)) {
	 minSize = levels[i]->get_width();
	 minLevel = i;
      }
      LOG(2, *(levels[i]));
   }

   // split on the widest level (found above)
   LOG(2, "Widest level is " << maxLevel << " (width = " << maxSize
       << ")" << endl);
   LOG(2, "Narrowest level is " << minLevel << " (width = " << minSize
       << ")" << endl);
   ASSERT(maxSize >= 0);
   ASSERT(maxLevel >= 0);
   ASSERT(minSize < INT_MAX);
   ASSERT(minLevel >= 0);

   // pick a level depending on split criterion
   int whichLevel;
   switch(splitType) {
      case splitTop:
	 whichLevel = topIndex+1;
	 break;
      case splitBottom:
	 whichLevel = botIndex-1;
	 break;
      case splitLargest:
	 whichLevel = maxLevel;
	 break;
      case splitSmallest:
	 whichLevel = minLevel;
	 break;
      default:
	 ASSERT(FALSE);
   }
   train_recursively(graph, levels, top, levels[whichLevel], TRUE);
   train_recursively(graph, levels, levels[whichLevel], bottom, FALSE);   
}

void COODGInducer::add_and_connect(CatGraph& graph,
				   Array<ProjLevel *>& levels)
{
   for(int i=levels.low(); i<=levels.high(); i++)
      levels[i]->insert_in_graph(graph);
   for(i=levels.low(); i<levels.high(); i++)
      levels[i]->connect_to_level(graph, *(levels[i+1]), TS->get_schema());
   levels[levels.high()]->connect_to_leaves(graph, TS->get_schema());
}


/***************************************************************************
  Description : Induce a decision graph.
  Comments    : Must be called after read_data().
                Erases any previously created graph.
***************************************************************************/
void COODGInducer::train()
{
   // we must have data, and we must have an order vector
   has_data();

   // compute order
   attrOrder.compute_order(get_log_options(), *TS);
   const Array<int>& orderVector = attrOrder.get_order();
   OK();

   // if the vector is of length 0, then build a const categorizer
   if(orderVector.size() == 0) {
      Category cat = TS->majority_category();
      SchemaRC schema = TS->get_schema();
      AugCategory aca(cat, schema.category_to_label_string(cat));
      rdgCat = new ConstCategorizer(description(), aca);
      return;
   }
   
   // start with an empty CatGraph and fill it in
   RootedCatGraph *graph = new RootedCatGraph;

   // create an array to hold all the levels.   
   int num = orderVector.size();
   Array<ProjLevel *> levels(num);

   // empty the array
   for(int i=0; i<num; i++)
      levels[i] = NULL;

   switch(method) {
      case topDown:
	 train_top_down(*graph, levels, orderVector);
	 break;
      case bottomUp:
	 train_bottom_up(*graph, levels, orderVector);
	 break;
      case recursive:
	 levels[0] = new ProjLevel(*TS, orderVector, displayMode == dispFull);
	 train_recursively(*graph, levels, levels[0], NULL, FALSE);
	 add_and_connect(*graph, levels);
	 break;
      default:
	 ASSERT(FALSE);
   }

   // prune the graph
   if(prune)
      prune_graph(*graph);

   // set the root to the node generated by the toplevel ProjLevel
   graph->set_root(levels[0]->get_toplevel_node());

   // output the graph in .dot format if display mode calls for it
   if(displayMode != dispNone) {
      MLCOStream out(get_option_string("DOT_OUTPUT", "t_ProjLevel.dot"));
      DotGraphPref pref;
      graph->display(out, pref);
   }

   // build the RDGCategorizer
   delete rdgCat;
   rdgCat = new RDGCategorizer(graph, description() + " categorizer",
			       TS->nominal_label_info().num_values() + 1);

   // delete contents of all levels
   for(i=0; i<num; i++)
      delete levels[i];
}

Bool COODGInducer::was_trained(Bool fatalOnFalse) const
{
   if(!rdgCat && fatalOnFalse)
      err << "COODGInducer::was_trained: not trained" << fatal_error;
   return (rdgCat != NULL);
}

const Categorizer& COODGInducer::get_categorizer() const
{
   was_trained();
   return *rdgCat;
}

