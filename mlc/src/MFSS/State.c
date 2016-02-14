// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The State class encaptulates all necessary information
                   for a node in a generic state space.
		 This class has two template arguments.  The first,
		   LocalInfo, specifies some important information local to
		   each state.  The second, GlobalInfo, specifies the
		   type of the information shared by all states.
		 If the derived state is a feature subset, for example,
		   the LocalInfo might be an array of booleans where each
		   element of the array indicates whether the corresponding
		   feature is present in the subset.  The GlobalInfo might
		   include information on how to estimate the accuracy of
		   an inducer on the subset.
		 State also has two pure virtual functions:
		   (1) eval() - The evaluation function returns a Real value
		       indicating the fitness of the state (how good it is).
		       Higher fitness values indicate better states.  This
		       function should also store the fitness value in the
		       fitness data member.
		   (2) gen_succ() - The successor function returns a list of
		       pointers to all states which can be reached from the
		       current state by applying one operator.
  Assumptions  : State::display assumes that class LocalInfo has
                   operator<< defined. 
		 State::operator== assumes that class LocalInfo has
		   operator== defined.
  Comments     :
  Complexity   : Not computed.
  Enhancements :
  History      : Dan Sommerfield                                   12/01/94
                   Redesigned class
                 Brian Frasca                                       4/27/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <BoolArray.h>
#include <MLCStream.h>
#include <State.h>

// RCSID("MLC++, $RCSfile: State.c,v $ $Revision: 1.10 $")

/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
template <class LocalInfo,class GlobalInfo>
State<LocalInfo,GlobalInfo>::State(LocalInfo*& initStateInfo,
				   const GlobalInfo& gI)
   : nodeNotCached(TRUE),
     nodePtr(NULL),
     stateInfo(initStateInfo),
     globalInfo(gI),
     fitness(MIN_EVALUATION),
     stdDev(UNDEFINED_VARIANCE),
     localEvalNum(NOT_EVALUATED),
     description(""),
     graphOptions(""),
     complexity(0),
     evalCost(0)
{
   ASSERT(initStateInfo != NULL);
   DBG(initStateInfo = NULL); // State gains ownership.
}


/***************************************************************************
  Description : Destructor
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
State<LocalInfo, GlobalInfo>::~State()
{
   delete stateInfo;
}

/***************************************************************************
  Description : The evaluate states function evalutes a list of states at
                  once.  It checks each state for membership in the graph
		  pointed to by pGraph.  States which belong are replaced
		  by their counterparts in the graph.  This avoids
		  multiple evaluations of the same state (unless explicitly
		  called for)
  Comments    : This function lives here instead of in StateSpace because
                  it uses GlobalInfo.  StateSpace is not templated directly
		  on GlobalInfo, so this function had to be placed somewhere
		  that was.
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void State<LocalInfo, GlobalInfo>::
evaluate_states(GlobalInfo *gInfo,
		StateSpace<State<LocalInfo, GlobalInfo> >* pGraph,
		DblLinkList<State<LocalInfo, GlobalInfo>*>* states,
		Bool computeReal)
{
   LOG(2, "Evaluating States:" << endl);
   
   DLLPix<State<LocalInfo, GlobalInfo> *> statePix(*states);
   for(statePix.first(); statePix; statePix.next()) {
      // see if state is in the graph
      State<LocalInfo, GlobalInfo> *state = (*states)(statePix);
      State<LocalInfo, GlobalInfo> *temp = (*states)(statePix);
      NodePtr nodePtr = state->get_node(pGraph);
      if(nodePtr) {
	 // its in the graph, so replace state in list with the one we found
	 (*states)(statePix) = &(pGraph->get_state(nodePtr));
	 LOG(2, "Node " << *((*states)(statePix))
	     << " (in graph)" << endl);
	 delete temp;
      }
      else {
	 // its not in the graph, so evaluate it
	 state->eval(gInfo, computeReal);
	 state->set_eval_num(pGraph->get_next_eval_num());
	 LOG(2, "Node " << *state << endl);
      }
   }
}
   

/***************************************************************************
  Description : Sets the LocalInfo for the state.  The state takes ownership
                  of the info.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void State<LocalInfo, GlobalInfo>::set_info(LocalInfo*& newStateInfo)
{
   delete stateInfo;		// Out with the old.
   stateInfo = newStateInfo;	// In with the new.
   DBG(newStateInfo = NULL);	// State gains ownership.
}


/***************************************************************************
  Description : Sets the fitness value for this state.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void State<LocalInfo, GlobalInfo>::set_fitness(Real newFitness)
{
   fitness = newFitness;
}


/***************************************************************************
  Description : Returns the number identifying this state.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
int State<LocalInfo, GlobalInfo>::get_eval_num() const
{
   return localEvalNum;
}

/***************************************************************************
  Description : Returns a pointer to the node in the StateSpace which
                  represents this state.  Caches the value of this node
		  once it has been found so subsequent calls do not
		  perform the search.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
NodePtr State<LocalInfo, GlobalInfo>::get_node(StateSpace<State<LocalInfo,
					       GlobalInfo> > *pGraph)
{
   if(nodeNotCached) {
      // look for the state if we don't know whether we're in or not
      nodePtr = pGraph->find_state(*this);
      nodeNotCached = FALSE;
   }

   return nodePtr;
}


/***************************************************************************
  Description : Sets the evaluation number of this state.  The evaluation
                  number cannot be known by the state itself--it is
		  provided by the search, which calls this function to
		  set the number.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void State<LocalInfo, GlobalInfo>::set_eval_num(int evalNum)
{
   localEvalNum = evalNum;
}


/***************************************************************************
  Description : Sets the state's description.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void State<LocalInfo, GlobalInfo>::
  set_description(const MString& newDescription)
{
   description = newDescription;
}


/***************************************************************************
  Description : The "GraphOptions" string is inserted in the node description
                  of the .dot file (which is used by dot and dotty to produce
		  Graphs). 
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void State<LocalInfo, GlobalInfo>::
set_graph_options(const MString& newGraphOptions)
{
   graphOptions = newGraphOptions;
}


/***************************************************************************
  Description : Returns TRUE iff the stateInfo of the two states is the same.
                This function assumes that operator== is defined for class
		LocalInfo (the stateInfo).
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
Bool State<LocalInfo, GlobalInfo>::
operator==(const State<LocalInfo, GlobalInfo>& compareState) const
{
   return *stateInfo == compareState.get_info();
}


/***************************************************************************
  Description : Display functions.  These use the primitive display_info and
                  display_stats functions.  display_for_graph should be
		  used to display info in dot output graphs.  Derived
		  classes should override display_info and display_stats.
  Comments    :
***************************************************************************/
template <class LocalInfo, class GlobalInfo>
void State<LocalInfo, GlobalInfo>::display_for_graph(
                             MLCOStream& stream) const {
      display_info(stream);
      stream << ": " << MString(fitness, 2);
}

template <class LocalInfo, class GlobalInfo>
void State<LocalInfo, GlobalInfo>::display(MLCOStream& stream) const
{
      display_info(stream);
      stream << ": ";
      display_stats(stream);
}

// Define operator<< for State.  Cannot use DEF_DISPLAY because the two
// argument template fools cpp into thinking we're giving two arguments
// to the macro.
template <class LocalInfo, class GlobalInfo>
MLCOStream& operator<<(MLCOStream& s, const State<LocalInfo, GlobalInfo>& c)
{ c.display(s); return s; }





