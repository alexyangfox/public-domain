// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : BFSearch implements a best-first search FunctionOptimizer.
                   See FunctionOptimizer.c and the function comments below
		   for more information.
  Assumptions  :
  Comments     :
  Complexity   : Depends on space being searched.
  Enhancements :
  History      : Brian Frasca                                       5/21/94
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <BFSearch.h>
#include <DblLinkList.h>
#include <GetOption.h>

// RCSID("MLC++, $RCSfile: BFSearch.c,v $ $Revision: 1.9 $")


// Option information.  Must be #define's instead of const declarations
// becuase this file is templated.
#define maxExpansionsHelp "This option ..."
#define epsilonHelp "This option..."
#define DEFAULT_MAX_STALE 5
#define DEFAULT_EPSILON 0.001


/***************************************************************************
  Description : Returns TRUE iff the termination condition has been reached.
                The termination condition is when the number of consecutive
		  non-improving node expansions is greater than some maximum
		  value (set through an option).
		Empty open list and exceeding the limit number of
		  evaluations will also terminate the search.
  Comments    : This is a protected method.
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
Bool BFSearch<LocalInfo, GlobalInfo>::terminate()
{
   if (openList.empty()) {
      LOG(2, "Terminating due to empty open list" << endl);
      return TRUE;
   }
   
   ASSERT(get_eval_limit() >= 0);
   ASSERT(graph->get_num_states() >= 0);
   ASSERT(numNonImprovingExpansions >= 0);
   ASSERT(maxNonImprovingExpansions > 0);
   if (numNonImprovingExpansions >= maxNonImprovingExpansions) {
      LOG(2, "Terminating because number of non-improving expansions "
	  "hit the maximum (" << maxNonImprovingExpansions <<
	  ")" << endl);
      return TRUE;
   }

   if (get_eval_limit() && graph->get_num_states() > get_eval_limit()) {
      LOG(2, "Terminating because more nodes were evaluated than "
	  "EVAL_LIMIT (" << graph->get_num_states() << ">"
	  << get_eval_limit() << ")" << endl);
      return TRUE;
   }
   
   return FALSE;
}

template <class LocalInfo, class GlobalInfo>
void BFSearch<LocalInfo, GlobalInfo>::set_defaults()
{
   StateSpaceSearch<LocalInfo, GlobalInfo>::set_defaults();
   set_max_non_improving_expansions(DEFAULT_MAX_STALE);
   set_min_expansion_improvement(DEFAULT_EPSILON);
}

template <class LocalInfo, class GlobalInfo>
void BFSearch<LocalInfo, GlobalInfo>::set_user_options(const MString& stem)
{
   StateSpaceSearch<LocalInfo, GlobalInfo>::set_user_options(stem);
   set_max_non_improving_expansions(
      get_option_int(stem + "MAX_STALE", maxNonImprovingExpansions,
		     maxExpansionsHelp, FALSE));
   set_min_expansion_improvement(
      get_option_real(stem + "EPSILON", minExpansionImprovement,
		      epsilonHelp, FALSE));
}



/***************************************************************************
  Description : This function does a best-first search beginning with the
                  given state.  It stops when the termination condition is
		  reached (see above).
  Comments    : The given starting node is deleted in the destructor of
                  StateSpaceSearch (the base class) or when search() is
		  called again.
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
const State<LocalInfo, GlobalInfo>& BFSearch<LocalInfo, GlobalInfo>::
search(State<LocalInfo, GlobalInfo>* initialState, GlobalInfo *gInfo,
       const MString& displayFileName)
{
   // Setup variables.
   numExpansions = 0;
   numNonImprovingExpansions = 0;
   bestNode = NULL;
   Real bestEval = MIN_EVALUATION;
   delete graph;
   graph = new StateSpace< State<LocalInfo, GlobalInfo> >;
   graph->set_log_level(get_log_level()-1);

   // Start with empty open/closed lists.
   openList.free();
   closedList.free();
   
   // Evalute initial node and put in on the open list.
   initialState->eval(gInfo, get_show_real_accuracy() == always);
   initialState->set_eval_num(graph->get_next_eval_num());
   openList.append(graph->create_node(initialState));

   while (!terminate()) {

      // Find the best node on the open list.  Note that there must be some
      // node on the open list or else we wouldn't pass the termination
      // condition.
      Real bestOpenNodeFitness = MIN_EVALUATION;
      DLLPix<NodePtr> bestOpenNodePix(openList,0);
      for (DLLPix<NodePtr> nodePix(openList,1); nodePix; ++nodePix) {
	 const State<LocalInfo, GlobalInfo>& tmpState =
	    graph->get_state(openList(nodePix));
	 if (tmpState.get_fitness() > bestOpenNodeFitness) {
	    bestOpenNodeFitness = tmpState.get_fitness();
	    bestOpenNodePix = nodePix;
	 }
      }
      ASSERT(bestOpenNodePix);

      if (bestOpenNodeFitness > 1 -
                  	  minExpansionImprovement) {
	 bestNode = openList(bestOpenNodePix);

	 // terminating because got 100% (or near) accuracy
	 if(get_show_real_accuracy() == bestOnly)
	    graph->get_state(bestNode).eval(gInfo, TRUE, FALSE);
	 LOG(2, "Terminating because we found a node with near 100% accuracy"
	     << endl);
	 break; 
      }

      // Save best node and its fitness.
      NodePtr parentNode = openList(bestOpenNodePix);
      Real parentEval = graph->get_state(parentNode).get_fitness();
      ASSERT(parentEval == bestOpenNodeFitness);

      // if we're showing real accuracy on best nodes only, get
      // the real accuracy of the best node now
      if(get_show_real_accuracy() == bestOnly)
	 graph->get_state(parentNode).eval(gInfo, TRUE, FALSE);
      
      // Remove best node from open list and put in on the closed list.
      closedList.append(parentNode);
      openList.del(bestOpenNodePix);

      // Update number of non-improving expansions.
      if (parentEval > bestEval + minExpansionImprovement)
	 numNonImprovingExpansions = 0;
      else
	 numNonImprovingExpansions++;
      numExpansions++;
      
      // Remember best node added to the closed list.
      if (parentEval > bestEval) {
	 bestEval = parentEval;
	 bestNode = parentNode;
	 LOG(1, "new best node (" << graph->get_num_states() << " evals) " <<
	     graph->get_state(bestNode) << endl);
      }

      LOG(2, "expanding " << graph->get_state(parentNode) << endl);
      LOG(2, numNonImprovingExpansions << " non-improving expansions."
	  << endl);

      // If termination condition is reached, exit without evaluating nodes
      // from final expansion.
      if (numNonImprovingExpansions >= maxNonImprovingExpansions) {
	 LOG(2, "Terminating because number of non-improving expansions "
	     "hit the maximum (" << maxNonImprovingExpansions <<
	     ")" << endl);
	 break;
      }
      
      // For all successors which are not in the graph (on open or closed
      // lists), evaluate them and put them on the open list.
      Bool computeRealNow = (get_show_real_accuracy() == always);
      DblLinkList<State<LocalInfo, GlobalInfo>*>* successors =
	 graph->get_state(parentNode).gen_succ(gInfo, graph, computeRealNow);
      while (!successors->empty()) {
	 NodePtr childNode = successors->remove_front()->get_node(graph);
	 ASSERT(childNode);
   	 openList.append(childNode);
      }
      delete successors;
   }
   LOG(1, "final best node "
       << graph->get_state(bestNode) << endl);
   LOG(1, "expanded " << numExpansions << " nodes" << endl);
   LOG(2, "evaluated " << graph->get_num_states() << " nodes" << endl);
  
   graph->set_final_node(bestNode);

   // if we're showing real accuracy on the final state only, compute
   // real accuracy here
   if(get_show_real_accuracy() == finalOnly)
      graph->get_state(bestNode).eval(gInfo, TRUE, FALSE);
   
   // display the state space, if called for
   if(displayFileName) {
      MLCOStream out(displayFileName);
      DotGraphPref pref;
      graph->display(out, pref);
   }
   return graph->get_state(bestNode);
}
