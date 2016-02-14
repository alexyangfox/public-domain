// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : SASearch implements a StateSpaceSearch which does simulated
                   annealing.  See SSSearch.c and the function
		   comments below for more information.
  Assumptions  :
  Comments     :
  Complexity   : Depends on space being searched.
  Enhancements :
  History      : Brian Frasca                                       5/21/94
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <math.h>
#include <SANode.h>
#include <DblLinkList.h>
#include <StatData.h>
#include <sim_anneal.h>
#include <GetOption.h>
#include <SASearch.h>

// RCSID("MLC++, $RCSfile: SASearch.c,v $ $Revision: 1.17 $")

// option defaults.  Must be #define's rather than const declarations
// because this file is templated
#define  DEFAULT_MAX_STALE 5
#define  DEFAULT_EPSILON 0.001
#define  DEFAULT_INITIAL_LAMBDA 0.001
#define  DEFAULT_MAX_EVALUATIONS 5
#define  DEFAULT_MIN_EXP_EVALUATIONS 5
#define  DEFAULT_SAS_SEED 7258789

// option help strings.  Must be #define's rather than const declarations
// because this file is templated
#define maxNonImprovingExpansionsHelp "Number of non-improving expansions " \
   "that cause the search to be considered stale (hence terminate)"
#define minExpansionImprovementHelp "Improvement of less than this epsilon " \
   "is considered a non-improvement (search is still stale)"
#define initialLambdaHelp "The temperature in simulated annealing.  Higher " \
   "values cause more randomness"
#define maxEvaluationsHelp "Maximum evaluations per node"
#define minExpEvaluationsHelp "Minimum evaluations before expanding"
#define seedHelp "The seed for the simulated annealing random generator"


template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::display_nodelist(MLCOStream& stream)
const
{
   ASSERT(nodeList.low() == 0);
   int limit = 5;
   IFLOG(4, limit = nodeList.high());
   if(limit >= nodeList.high())
      limit = nodeList.high();
   for(int i=nodeList.low(); i<=limit; i++) {
      ASSERT(nodeList[i].fitness == graph->get_state(nodeList[i].node).
	     get_fitness());
      stream << i << ": " << graph->get_state(nodeList[i].node) << endl;
   }
}

/***************************************************************************
  Description : Re-evaluate a node at least min_exp_evaluations().
                This is called whenever the node is winning a race and we want
   		  to make sure it isn't a fluke
  Comments    :
***************************************************************************/

template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::reeval_node(int nodeNum,
    GlobalInfo *gInfo, NodePtr& bestRetired,
    StateSpace< State<LocalInfo, GlobalInfo> >& graph)
{
   SANode& saNode = nodeList[nodeNum];
   // Re-evaluate node if it has not reached its max.
   if (saNode.numEvaluations < get_max_evaluations()) {
      saNode.numEvaluations++;

      // when reevaluating, never get the real accuracy
      saNode.fitness = graph.get_state(saNode.node).eval(gInfo, FALSE, TRUE);
      LOG(1, "re-evaluated node " <<
	  graph.get_state(saNode.node) << endl);
      LOG(3, "Re-eval fitness = " << saNode.fitness
	  << endl);
      numReEvaluations++;
   } else {
      LOG(2, "node "
	  << graph.get_state(saNode.node)
	  << " has reached maximum number of evaluations ("
	  << get_max_evaluations() << ')' << endl);

      // if the newly retired node has a better evaluation than
      // the best retired node so far, replace the best retired node.
      if(bestRetired == NULL || saNode.fitness > graph.get_state(bestRetired).
	        get_fitness()) {
	 bestRetired = saNode.node;
	 LOG(2, "new best node (" << graph.get_num_states() + numReEvaluations
	     << " evals {retired}) " << graph.get_state(bestRetired) <<
	    endl);

	 // if the best retired node has no real accuracy, give it one
	 // if we're in best accuracy mode
	 if(get_show_real_accuracy() == bestOnly &&
	    graph.get_state(bestRetired).get_real_accuracy() < 0) {
	    graph.get_state(bestRetired).eval(gInfo, TRUE, FALSE);
	    LOG(2, "needed real accuracy for best retired node" << endl);
	 }
      }

      // yank the maxed-out node out of the list, then use
      // truncate to reduce the list's size
      nodeList[nodeNum] = nodeList[nodeList.high()];
      nodeList.truncate(nodeList.size() - 1);
   }
}


/***************************************************************************
  Description : Returns TRUE iff the termination condition has been reached.
                The termination condition is when the number of consecutive
		  non-improving node expansions is greater than some maximum
		  value (set through an option).
  Comments    : This is a protected method.
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
Bool SASearch<LocalInfo, GlobalInfo>::terminate()
{
   if (nodeList.size() == 0) {
      LOG(2, "Terminating due to empty node list" << endl);
      return TRUE;
   }
   
   ASSERT(get_eval_limit() >= 0);
   ASSERT(graph->get_num_states() >= 0);
   ASSERT(numNonImprovingExpansions >= 0);
   ASSERT(get_max_non_improving_expansions() > 0);

   if (numNonImprovingExpansions >= get_max_non_improving_expansions()) {
       LOG(2, "Terminating because number of non-improving expansions "
	  "hit the maximum (" << get_max_non_improving_expansions() <<
	  ")" << endl);
       return TRUE;
   }

   if (get_eval_limit() && graph->get_num_states()+numReEvaluations >
       get_eval_limit()) {
      LOG(2, "Terminating because more nodes were evaluated than "
	  "EVAL_LIMIT (" << graph->get_num_states()+numReEvaluations << ">"
	  << get_eval_limit() << ")" << endl);
      return TRUE;
   }
   
   return FALSE;
}


/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
SASearch<LocalInfo, GlobalInfo>::SASearch()
   : StateSpaceSearch<LocalInfo, GlobalInfo>(), nodeList(0)
{
   numReEvaluations = 0;
   set_defaults();
}


/***************************************************************************
  Description : Options functions.  Use set_options to set all options
                  from the user/shell.  Use set_defaults to restore
		  default values.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::set_defaults()
{
   StateSpaceSearch<LocalInfo, GlobalInfo>::set_defaults();
   set_max_non_improving_expansions(DEFAULT_MAX_STALE);
   set_min_expansion_improvement(DEFAULT_EPSILON);
   set_max_evaluations(DEFAULT_MAX_EVALUATIONS);
   set_min_exp_evaluations(DEFAULT_MIN_EXP_EVALUATIONS);
   set_initial_lambda(DEFAULT_INITIAL_LAMBDA);
}

template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::set_user_options(const MString& stem)
{
   StateSpaceSearch<LocalInfo, GlobalInfo>::set_user_options(stem);
   set_max_non_improving_expansions(
      get_option_int(stem + "MAX_STALE",
		     get_max_non_improving_expansions(),
		     maxNonImprovingExpansionsHelp,
		     TRUE));
   set_min_expansion_improvement(
      get_option_real(stem + "EPSILON",
		      get_min_expansion_improvement(),
		      minExpansionImprovementHelp,
		      TRUE));
   set_max_evaluations(
      get_option_int(stem + "MAX_EVALS",
		     get_max_evaluations(),
		     maxEvaluationsHelp,
		     TRUE));
   set_min_exp_evaluations(
      get_option_int(stem + "MIN_EXP_EVALS",
		     get_min_exp_evaluations(),
		     minExpEvaluationsHelp,
		     TRUE));
   set_initial_lambda(
      get_option_real(stem + "LAMBDA",
		      get_initial_lambda(),
		      initialLambdaHelp,
		      TRUE));
   init_rand_num_gen(
      get_option_int(stem + "SAS_SEED",
		     DEFAULT_SAS_SEED,
		     seedHelp,
		     TRUE));
}

template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::
set_max_non_improving_expansions(int maxExpansions)
{
   if (maxExpansions < 1)
      err << "SASearch::set_max_non_improving_expansions: "
	  "maxExpansions (" << maxExpansions << ") must be at least 1"
	  << fatal_error;
   maxNumberOfNonImprovingExpansions = maxExpansions;
}

template <class LocalInfo, class GlobalInfo>
int SASearch<LocalInfo, GlobalInfo>::get_max_non_improving_expansions() const
{
   return maxNumberOfNonImprovingExpansions;
}

template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::
set_min_expansion_improvement(Real minImprovement)
{
   minIncreaseForImprovingExpansion = minImprovement;
}

template <class LocalInfo, class GlobalInfo>
Real SASearch<LocalInfo, GlobalInfo>::get_min_expansion_improvement() const
{
   return minIncreaseForImprovingExpansion;
}

template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::set_max_evaluations(int maxEval)
{
   if (maxEval < 1)
      err << "SASearch::set_max_evaluations: maxEval ("
	  << maxEval << ") must be greater than 0" << fatal_error;
   maxEvaluations = maxEval;
}

template <class LocalInfo, class GlobalInfo>
int SASearch<LocalInfo, GlobalInfo>::get_max_evaluations() const
{
   return maxEvaluations;
}

template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::set_min_exp_evaluations(int minEval)
{
   if (minEval < 1 || minEval > get_max_evaluations())
      err << "SASearch::set_min_exp_evaluations: minEval ("
	  << minEval << ") must be >0 and <max_evaluations"
          << get_max_evaluations() << fatal_error;
   minExpEvaluations = minEval;
}

template <class LocalInfo, class GlobalInfo>
int SASearch<LocalInfo, GlobalInfo>::get_min_exp_evaluations() const
{
   return minExpEvaluations;
}



template <class LocalInfo, class GlobalInfo>
void SASearch<LocalInfo, GlobalInfo>::set_initial_lambda(Real initLambda)
{
   if (initLambda <= 0)
      err << "SASearch::set_initial_lambda: initLambda ("
	  << initLambda << ") must be greater than 0" << fatal_error;
   initialLambda = initLambda;
}

template <class LocalInfo, class GlobalInfo>
Real SASearch<LocalInfo, GlobalInfo>::get_initial_lambda() const
{
   return initialLambda;
}

/***************************************************************************
  Description : This function performs simulated annealing beginning with
                  the
                  given state.  It stops when the termination condition is
		  reached (see above).
		SASearch takes ownership of the given starting State.
  Comments    : The given starting node is deleted in the destructor of
                  StateSpaceSearch (the base class) or when search() is
		  called again.
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
const State<LocalInfo, GlobalInfo>& SASearch<LocalInfo, GlobalInfo>::
search(State<LocalInfo, GlobalInfo>* initialState, GlobalInfo *gInfo,
       const MString& displayFileName)
{
   // Initial setup.
   nodeList.truncate(0);   // make sure node list has 0 elements
   delete graph;
   graph = new StateSpace< State<LocalInfo, GlobalInfo> >;
   if(get_log_level() > 0)
      graph->set_log_level(get_log_level() - 1);
   Real lambda = get_initial_lambda();
   NodePtr bestRetired = NULL;
   numReEvaluations = 0;
   numExpansions = 0;
   
   numNonImprovingExpansions = 0;
   
   // Evalute initial node and put on node list.
   Real initialEval = initialState->eval(gInfo,
					 get_show_real_accuracy() == always);
   initialState->set_eval_num(graph->get_next_eval_num());
   NodePtr initialNode = graph->create_node(initialState);
   ASSERT(initialNode != NULL);
   nodeList[nodeList.size()] = SANode(initialNode, initialEval);
   Real bestEval = MIN_EVALUATION;
   NodePtr bestNode = NULL;
   Bool lastIterationWasExpansion = FALSE;

   LOG(1, "Beginning SASearch" << endl);
   LOG(1, "initial node = "
       << graph->get_state(initialNode).get_eval_num() << endl
       << graph->get_state(initialNode) << endl);

   while (!terminate()) {
      // Use the sim_anneal function to pick a node randomly.  Use the
      // standard-deviation of the first node to determine lambda.
      // If the first node has no standard deviation, always pick it.
      int nodeNum;

      // pick a lambda
      Real saLambda = graph->get_state(nodeList[0].node).get_std_dev();
      if(nodeList.size() > 1 && saLambda && saLambda != UNDEFINED_VARIANCE) {

	 // build a real array of accuracies for sim_anneal
	 Array<Real> accArray(nodeList.low(), nodeList.size());
	 for(int i=nodeList.low(); i<=nodeList.high(); i++)
	    accArray[i] = graph->get_state(nodeList[i].node).get_fitness();

	 // call sim_anneal to pick the slot
	 nodeNum = sim_anneal(accArray, saLambda*lambda, rand_num_gen());
      }
      else
	 nodeNum = 0;
      
      Real chosenEval = graph->get_state(nodeList[nodeNum].node).
	 get_fitness();
      LOG(2, "picked slot " << nodeNum << ": "
	  << graph->get_state(nodeList[nodeNum].node) << endl);

      if (nodeList[nodeNum].isExpanded ||
	  nodeList[nodeNum].numEvaluations < get_min_exp_evaluations())  {
	 lastIterationWasExpansion = FALSE;
         reeval_node(nodeNum, gInfo, bestRetired, *graph);
      } else {
	 // Evaluate all successors not already in the graph and add them
	 // to the node list.
	 lastIterationWasExpansion = TRUE;
	 numNonImprovingExpansions++;
	 LOG(1, "expanded node "
	     << graph->get_state(nodeList[nodeNum].node)
	     << endl);
	 nodeList[nodeNum].isExpanded = TRUE;
	 DblLinkList<State<LocalInfo, GlobalInfo>*>* successors =
	    graph->get_state(nodeList[nodeNum].node).gen_succ(gInfo, graph,
				  get_show_real_accuracy() == always);
	 while (!successors->empty()) {
	    State<LocalInfo, GlobalInfo>* childState =
	       successors->remove_front();
	    NodePtr childNode = childState->get_node(graph);
	    ASSERT(childNode);
	    Real childEval = childState->get_fitness();
	    ASSERT(childEval > MIN_EVALUATION);
	    nodeList[nodeList.size()] = SANode(childNode, childEval);
	 }
	 delete successors;
      }

      // Make sure firstNode has been evaluated enough times.
      // Sort list of nodes (best first) and display best node.
      nodeList.sort();
      IFLOG(3, display_nodelist(get_log_stream()));

      // If node 0 is the last node and gets retired, then we have
      // no nodes remaining in the nodeList and we must exit this
      // loop.
      while (nodeList.size() > 0 &&
	     nodeList[0].numEvaluations < get_min_exp_evaluations()) {
	 reeval_node(0, gInfo, bestRetired, *graph);
	 nodeList.sort();
         IFLOG(3, display_nodelist(get_log_stream()));
      }

      if(nodeList.size() > 0) {
	 NodePtr firstNode = nodeList[0].node;
	 ASSERT(firstNode != NULL);
	 Real firstEval = graph->get_state(firstNode).get_fitness();
	 // If two nodes have the same accuracy, one gets evaluated
	 //    and drops, we have a new best node.
	 // Another possibility is that the first node's fitness changed.
	 if (firstNode != bestNode || firstEval != bestEval) {
	    bestEval = firstEval;
	    if (bestRetired && graph->get_state(bestRetired).get_fitness() >
		bestEval) {
	       LOG(2, "best node is retired " << graph->get_state(bestRetired)
		   << endl);
	       bestNode = firstNode;
	    }
	    else if (firstNode != bestNode) {
	       if(!bestNode ||
		  fabs(firstEval - graph->get_state(bestNode).get_fitness()) >
		  get_min_expansion_improvement()) {
		  numNonImprovingExpansions = 0;
		  if (bestNode) {
		     LOG(2, "Resetting stale to 0.  First is " << firstEval
			 << " best is "
			 << graph->get_state(bestNode).get_fitness() << endl);
		  }
	       } else
		  LOG(3, "Non-stale improvement" << endl);
	       bestNode = firstNode;

	       // get real accuracy for best node if called for
	       if(get_show_real_accuracy() == bestOnly)
		  graph->get_state(bestNode).eval(gInfo, TRUE, FALSE);
	    
	       LOG(1, "new best node (" << graph->get_num_states() +
		   numReEvaluations << " evals) "
		   << graph->get_state(bestNode) << endl);
	    } else {
	       LOG(2, "re-evaluated best node (" << graph->get_num_states() +
		   numReEvaluations << " evals) "
		   << graph->get_state(bestNode) << endl);
	    }
	 }
      }
      if(lastIterationWasExpansion)
	 numExpansions++;
      
      LOG(1, "Iteration complete.  ");
      if (lastIterationWasExpansion)
	 LOG(1, "Expansion (" << numNonImprovingExpansions << " stale).  ");
      else
	 LOG(1, "Reevaluation.  ");
      LOG(1, endl);

      LOG(1, "Total evaluations: " << graph->get_num_states()+numReEvaluations
	  << " (" << graph->get_num_states() << " new + " <<
	  numReEvaluations << "old)" << endl);

      NodePtr apparentBest;
      if(bestRetired && graph->get_state(bestNode).get_fitness() <
	 graph->get_state(bestRetired).get_fitness()) {
	 LOG(1, "Current best (retired) ");
	 apparentBest = bestRetired;
      }
      else {
	 LOG(1, "Current best ");
	 apparentBest = bestNode;
      }

      LOG(1, "(" << graph->get_num_states() + numReEvaluations << " evals) ");
      LOG(1, graph->get_state(apparentBest) << endl);
   }

   // get real accuracy for final node if called for
   if(bestRetired && graph->get_state(bestRetired).get_fitness() >=
      graph->get_state(bestNode).get_fitness()) {
      LOG(2, "Final best node is best retired node" << endl);
      bestNode = bestRetired;
   }
   if(get_show_real_accuracy() == finalOnly)
      graph->get_state(bestNode).eval(gInfo, TRUE, FALSE);
   // @@ temporary hack to get this working.  Should be an ASSERT
   // else if(get_show_real_accuracy() == bestOnly)
      // we're supposed to have a real accuracy here, so make sure we
      // actually have one.
      // ASSERT(graph->get_state(bestNode).get_real_accuracy() >= 0);
   else if(get_show_real_accuracy() == bestOnly && 
	   graph->get_state(bestNode).get_real_accuracy() < 0) {
      LOG(1, "WARNING: final best node has no real accuracy! "
	  " computing now..." << endl);
      graph->get_state(bestNode).eval(gInfo, TRUE, FALSE);
   }
   
   LOG(1, "final best node = "
       << graph->get_state(bestNode) << endl);

   LOG(1, "expanded " << numExpansions << " nodes" << endl);
   LOG(2, "evaluated " << graph->get_num_states()+numReEvaluations
       << " nodes" << endl);
   graph->set_final_node(bestNode);
   
   // display the state space, if called for
   if(displayFileName) {
      MLCOStream out(displayFileName);
      DotGraphPref pref;
      graph->display(out, pref);
   }
   ASSERT(bestNode);
   return graph->get_state(bestNode);
}





