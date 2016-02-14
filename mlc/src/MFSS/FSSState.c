// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The FSSState class encaptulates the necessary state
                   information for the state space of feature subsets.
		   It is derived from CompState to allow use of compound
		   operators during the search.
		 See AccEstState for more information on the pure virtual
		   functions this class overrides.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : Dan Sommerfield                                    5/12/94
                   Redesigned to fit with AccEstState
                 Dan Sommerfield                                   12/01/94
                   Complete redesign of classes                    
                 Brian Frasca                                       5/14/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <FSSState.h>
#include <env_inducer.h>

// RCSID("MLC++, $RCSfile: FSSState.c,v $ $Revision: 1.9 $")

void FSSInfo::display_values(const Array<int>& values, MLCOStream& out) const
{
   (void)values;
   (void)out;
}

/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/

FSSState::FSSState(Array<int>*& featureSubset, const AccEstInfo& gI)
  : CompState(featureSubset, gI)
{
   complexity = 0;
   for(int i = 0; i < get_info().size(); i++) {
      ASSERT(get_info()[i] == 0 || get_info()[i] == 1);
      complexity += get_info()[i];
   }
}   



/***************************************************************************
  Description : This function builds/alters the training and test lists to
                  be used by the accuracy estimation process which
		  determines the fitness of this state.
		Specifically, this function makes projected copies of each
		  list.  The testing list is not projected unless one was
		  given to the function on entry.
  Comments    :
***************************************************************************/
void FSSState::construct_lists(AccEstInfo * /* acInfo */,
			       InstanceList *& trainList,
			       InstanceList *& testList)
{
   // convert to BoolArray to use project functions
   BoolArray boolFeatureArray(get_info().low(), get_info().size(), FALSE);
   for(int i=0; i<get_info().size(); i++)
      boolFeatureArray.index(i) = get_info().index(i);
   
   trainList = &(trainList->project(boolFeatureArray)->
		 cast_to_instance_list());
   if(testList)
      testList = &(testList->project(boolFeatureArray)->
		   cast_to_instance_list());
}

/***************************************************************************
  Description : Frees up any space used by creation of the lists.  Because
                  the projected lists are stored copies of the original
		  list, this function needs to delete them.
  Comments    :
***************************************************************************/
void FSSState::destruct_lists(AccEstInfo *,
			      InstanceList *trainList,
			      InstanceList *testList)
{
   delete trainList;
   delete testList;
}

/***************************************************************************
  Description : Displays the state's info.  It just displays the numbers.
  Comments    :
***************************************************************************/
void FSSState::display_info(MLCOStream& stream) const
{
   // output node number if known
   if(get_eval_num() > NOT_EVALUATED)
      stream << "#" << get_eval_num();
   else
      stream << "#?";

   // figure out true indices in array
   DynamicArray<int> trueIndices(get_info().size());
   int lastTrueIndex = 0;
   for(int i=get_info().low(); i<=get_info().high(); i++)
      if(get_info()[i])
	 trueIndices.index(lastTrueIndex++) = i;
   trueIndices.truncate(lastTrueIndex);

   stream << "[" << trueIndices << "]";

// Displaying names is not very useful because the lines turn out
// to be very long and it's hard to see the actual search.
#  ifdef DISPLAY_NAMES
   stream << "[";
   for(i=0; i<lastTrueIndex; i++) {
      stream << globalInfo.trainList->get_schema().
	 attr_name(trueIndices.index(i));
      if(i < lastTrueIndex-1)
	 stream << ", ";
   }
   stream << "]";
#  endif
}







