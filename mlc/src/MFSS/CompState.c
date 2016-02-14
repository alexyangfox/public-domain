// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The CompState class represents a state in the search
                   space which is capable of having COMPOUND OPERATORS
		   generated for it.  CompState provides the gen_succ
		   function from State.  A default eval() is provided
		   by AccEstState.
		 The CompState is based on an array of integer values
		   which uniquely describe the state.  Values in the
		   array are subject to boundaries as provided through
		   AccEstInfo's lower_bound() and upper_bound() functions.
  Assumptions  : 
  Comments     :
  Complexity   : Not computed.
  Enhancements :
  History      : Dan Sommerfield                                    4/26/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <State.h>
#include <CompState.h>

//RCSID("MLC++, $RCSfile: CompState.c,v $ $Revision: 1.6 $")

/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
CompState::CompState(Array<int>*& initialValues, const AccEstInfo& gI)
   : AccEstState(initialValues, gI)
{
}


DEF_DISPLAY(CompElem);


/***************************************************************************
  Description : CompState's gen_succ function automatically generates
                  possible successors of this class using the array of
		  values defining this state, and the array of limits
		  assumed to be in the GlobalInfo
  Comments    : Uses private CompElem class defined above.
***************************************************************************/
DblLinkList<State<Array<int>, AccEstInfo> *> *
  CompState::gen_succ(AccEstInfo *info,
		      StateSpace< State<Array<int>, AccEstInfo> > *space,
		      Bool computeReal)
{
   DblLinkList<State<Array<int>, AccEstInfo> *> *successors =
      new DblLinkList<State<Array<int>, AccEstInfo> *>();
   DynamicArray<CompElem> compElemArray(get_info().size() * 2);
   compElemArray.truncate(0);
   
   // create the list of successors.  If possible, add two successors
   // for each element in the values array, one with the element's value
   // one higher, and one with the element's value one lower.
   for (int i = get_info().low(); i <= get_info().high(); i++) {
      int lb = info->lower_bound(i);
      int ub = info->upper_bound(i);
      ASSERT(get_info()[i] >= lb);
      ASSERT(get_info()[i] <= ub);

      // set up upper and lower starts
      CompState *upperState = NULL;
      CompState *lowerState = NULL;
      
      // add the higher version
      if(get_info()[i] < ub) {
	 Array<int> *newValues = new Array<int>(get_info(), ctorDummy);
	 (*newValues)[i]++;
	 upperState = create_state(newValues);
	 upperState->set_log_level(get_log_level());
	 int where = compElemArray.size();
	 compElemArray.index(where).num = i;
	 compElemArray.index(where).value = upperState->get_info().index(i);
      }

      // add in the lower version
      if(get_info()[i] > lb) {
	 Array<int> *newValues = new Array<int>(get_info(), ctorDummy);
	 (*newValues)[i]--;
	 lowerState = create_state(newValues);
	 lowerState->set_log_level(get_log_level());
	 int where = compElemArray.size();
	 compElemArray.index(where).num = i;
	 compElemArray.index(where).value = lowerState->get_info().index(i);
      }

      // always add both states to the graph
      if(upperState)
	 successors->append(upperState);
      if(lowerState)
	 successors->append(lowerState);      
   }

   // Evaluate all states.  We need to evaluate here because we cannot
   // build compound operators without knowing the states' fitness values.
   evaluate_states(info, space, successors, computeReal);

   // generate compound node, if asked for
   if(info->use_compound() && get_info().size() >= 2) {
      LOG(3, "creating compound nodes..." << endl);

      // fill in evaluations in compElemArray.  Eliminate compElems with
      // duplicate feature numbers (num) by setting their evaluations to
      // -1 so they get sorted out.
      DLLPix<State<Array<int>, AccEstInfo> *> pix(*successors,1);
      for(int i=0; pix; pix.next(), i++) {
	 State<Array<int>, AccEstInfo> *state = (*successors)(pix);
	 compElemArray.index(i).eval = state->get_fitness();

	 // check for and "eliminate" the worse of a pair of duplicate
	 // elements (duplicate num's)
	 if(i>0 && (compElemArray.index(i-1).num ==
	    compElemArray.index(i).num)) {
	    if(compElemArray.index(i).eval >
	       compElemArray.index(i-1).eval)
	       compElemArray.index(i-1).eval = -1;
	    else
	       compElemArray.index(i).eval = -1;
	 }	    
      }
      compElemArray.sort();
      LOG(5, "Feature Elem Array: " << compElemArray << endl);

      // set starting position in feature element array
      int startPosition = compElemArray.size() - 2;

      // skip over the compound node which only modifies one feature;
      // this node was already added above.
      Array<int> baseValues(get_info(), ctorDummy);
      Real bestEval = 0;
      if(compElemArray.size()) {
	 bestEval = compElemArray.index(compElemArray.size() - 1).eval;
	 baseValues.index(compElemArray.index(compElemArray.size() - 1).num) =
	    compElemArray.index(compElemArray.size() - 1).value;
      }

      // repeatedly add compound nodes as long as their accuracy improves
      while(startPosition >= 0 &&
	    compElemArray.index(startPosition).eval >= 0) {
	 // changing the base values array as long as our fitness
	 // improves.
	 baseValues.index(compElemArray.index(startPosition).num) =
	    compElemArray.index(startPosition).value;
	 Array<int> *newValues = new Array<int>(baseValues, ctorDummy);

	 LOG(4, "Trying compound: ");
	 IFLOG(4, info->display_values(*newValues, get_log_stream()));
	 
	 // see if its in the graph
	 CompState *compState = create_state(newValues);
	 compState->set_log_level(get_log_level());
	 if(!compState->get_node(space)) {
	    LOG(4, " NOT IN GRAPH" << endl);
	    compState->eval(info, computeReal);
	    compState->set_eval_num(space->get_next_eval_num());
	    successors->append(compState);
	 }
	 else {
	    LOG(4, " in graph" << endl);
	    CompState *tempState = compState;

	    // state pointer is listed as "generic State" in graph, so we
	    // have to cast.
	    compState = (CompState *)
	       (&space->get_state(compState->get_node(space)));
	    delete tempState;
	 }

	 // log compound node
	 LOG(2, "Compound Node " << *compState <<  endl);

	 // if the compound is no improvement over the best node, then
	 // stop generating them
	 if(compState->get_fitness() < bestEval) {
	    LOG(3, "Stopped generating compound nodes" << endl);
	    break;
	 }
	 bestEval = compState->get_fitness();
	 startPosition--;
      }
   }

   space->insert_states(*this, successors);
   return successors;
}



   









