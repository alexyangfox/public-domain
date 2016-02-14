// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The OrderState class represents a state in the search
                   space containing an ordered subset of features.
		   This state is to be used for feature subset selection
		   type algorithms with inducers which are sensitive to
		   the ordering of features (i.e. COODG, etc).
		 The integer array from AccEstState represents the feature
		   numbers and ordering (order within the array is
		   significant) to use in the wrapped inducer -- an
		   order vector.
  Assumptions  : Derived classes must use pre_eval to set the ordering in
                   wrapped inducer.
  Comments     :
  Complexity   : Not computed.
  Enhancements :
  History      : Dan Sommerfield                                    5/28/95
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <State.h>
#include <OrderState.h>

//RCSID("MLC++, $RCSfile: OrderState.c,v $ $Revision: 1.7 $")

/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
OrderState::OrderState(Array<int>*& initialValues, const AccEstInfo& gI)
   : AccEstState(initialValues, gI)
{
   complexity = get_info().size();
}


/***************************************************************************
  Description : OrderState's gen_succ function automatically generates
                  possible successors of this class
  Comments    : No compound states are generated.
***************************************************************************/
DblLinkList<State<Array<int>, AccEstInfo> *> *
  OrderState::gen_succ(AccEstInfo *info,
		      StateSpace< State<Array<int>, AccEstInfo> > *space,
		      Bool computeReal)
{
   DblLinkList<State<Array<int>, AccEstInfo> *> *successors =
      new DblLinkList<State<Array<int>, AccEstInfo> *>();

   // create a BoolArray to identify the features which are eligible for
   // adding to the ordering
   BoolArray addArray(0, info->trainList->num_attr(), TRUE);
   for(int i = get_info().low(); i <= get_info().high(); i++) {
      ASSERT(addArray.index(get_info()[i]) == TRUE);
      addArray.index(get_info()[i]) = FALSE;
   }
        
   // create the list of successors.
   // We will use three possible operators:
   //   ADD, which adds a new feature to the end of the ordering, 
   //   SWAP, which swaps two adjacent features in the ordering.
   //   DEL, which deletes a feature.

   // Perform the additions
   for(i = 0; i < addArray.size(); i++)
      if(addArray.index(i)) {
 	 Array<int> *newValues = new Array<int>(get_info().low(),
						get_info().size()+1);
	 for(int j = get_info().low(); j <= get_info().high(); j++)
	    (*newValues)[j] = get_info()[j];
	 (*newValues)[newValues->high()] = i;
	 OrderState *newState = create_state(newValues);
	 newState->set_log_level(get_log_level());
	 successors->append(newState);
      }

   // Perform the swaps
   for (i = get_info().low(); i < get_info().high(); i++) {
      // swap the ith feature with the (i+1)th
      Array<int> *newValues = new Array<int>(get_info(), ctorDummy);
      int temp = (*newValues)[i];
      (*newValues)[i] = (*newValues)[i+1];
      (*newValues)[i+1] = temp;
      OrderState *newState = create_state(newValues);
      newState->set_log_level(get_log_level());
      successors->append(newState);
   }

   // Delete
   for (i = get_info().low(); i <= get_info().high(); i++) {
      Array<int> *newValues = new Array<int>(get_info().low(),
					     get_info().size()-1);
      for(int j = get_info().low(); j < get_info().high(); j++) 
	 (*newValues)[j] = get_info()[j + (j >= i)];
      OrderState *newState = create_state(newValues);
      newState->set_log_level(get_log_level());
      successors->append(newState);
   }


   // Evaluate all states and add to graph
   evaluate_states(info, space, successors, computeReal);
   space->insert_states(*this, successors);
   return successors;
}

/***************************************************************************
  Description : Display the ordering for this state
  Comments    : 
***************************************************************************/
void OrderState::display_info(MLCOStream& stream) const
{
   if(get_eval_num() > NOT_EVALUATED)
      stream << "#" << get_eval_num();
   else
      stream << "#?";
   stream << " [" << get_info() << "]";
}


   









