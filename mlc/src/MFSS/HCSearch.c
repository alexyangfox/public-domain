// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : HCSearch implements a hill-climbing FunctionOptimizer.
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
#include <DblLinkList.h>
#include <StateSpace.h>
#include <HCSearch.h>

// RCSID("MLC++, $RCSfile: HCSearch.c,v $ $Revision: 1.8 $")

     
/***************************************************************************
  Description : Returns TRUE iff there was no improvement after expanding
                  the current best node.  This occurs when the previous
		  best node and the current best node are the same.
  Comments    : This is a protected method.
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
Bool HCSearch<LocalInfo, GlobalInfo>::terminate()
{
   if (prevBestNode == bestNode) {
      LOG(2, "Terminating because we cannot find a better node" << endl);
      return TRUE;
   }
   
   ASSERT(get_eval_limit() >= 0);
   ASSERT(graph->get_num_states() >= 0);
   if (get_eval_limit() && graph->get_num_states() > get_eval_limit()) {
      LOG(2, "Terminating because more nodes were evaluated than "
	  "EVAL_LIMIT (" << graph->get_num_states() << ">"
	  << get_eval_limit() << ")" << endl);
      return TRUE;
   }
   
   return FALSE;
}


/***************************************************************************
  Description : Given an initial State, this function does a hill-climbing
                  search to find a local extreme.  The hill-climbing search
		  is strictly increasing.
		HCSearch takes ownership of the given starting State.
  Comments    : The given starting node is deleted in the destructor of
                  StateSpaceSearch (the base class) or when search() is
		  called again.
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
const State<LocalInfo, GlobalInfo>& HCSearch<LocalInfo, GlobalInfo>::
search(State<LocalInfo, GlobalInfo>* initialState, GlobalInfo *gInfo,
       const MString& displayFileName)
{
   delete graph;
   graph = new StateSpace< State<LocalInfo, GlobalInfo> >;
   numExpansions = 0;
   Real prevBestEval = MIN_EVALUATION;
   prevBestNode = NULL;
   Real bestEval  = initialState->eval(gInfo,
				       get_show_real_accuracy() == always);
   initialState->set_eval_num(graph->get_next_eval_num());
   bestNode  = graph->create_node(initialState);
   graph->set_initial_node(bestNode);
   if(get_show_real_accuracy() == bestOnly)
      graph->get_state(bestNode).eval(gInfo, TRUE, FALSE);

   while (!terminate()) {
      prevBestNode = bestNode;
      prevBestEval = bestEval;

      // display new best node
      LOG(1, "new best node (" << graph->get_num_states() << " evals) " <<
	  graph->get_state(bestNode) << endl);
      
      DblLinkList<State<LocalInfo, GlobalInfo>*>* successors =
	 graph->get_state(prevBestNode).gen_succ(gInfo, graph,
				      get_show_real_accuracy() == always);
      while (!successors->empty()) {
	 numExpansions++;
         State<LocalInfo, GlobalInfo>* childState =
	    successors->remove_front();
         Real childEval = childState->get_fitness();
	 ASSERT(childEval > MIN_EVALUATION);
         NodePtr childNode = childState->get_node(graph);
	 ASSERT(childNode);
         if (childEval > bestEval) {
            bestEval = childEval;
            bestNode = childNode;
         }
      }
      delete successors;
      if(get_show_real_accuracy() == bestOnly)
	 graph->get_state(bestNode).eval(gInfo, TRUE, FALSE);
   }

   // get real accuracy on final node if needed
   if(get_show_real_accuracy() == finalOnly)
      graph->get_state(bestNode).eval(gInfo, TRUE, FALSE);
   LOG(1, "final best node " << graph->get_state(bestNode) << endl);

   LOG(1, "expanded " << numExpansions << " nodes" << endl);
   LOG(2, "evaluated " << graph->get_num_states() << " nodes" << endl);
   graph->set_final_node(bestNode);

   // display the state space, if called for
   if(displayFileName) {
      MLCOStream out(displayFileName);
      DotGraphPref pref;
      graph->display(out, pref);
   }
   return graph->get_state(bestNode);
}



