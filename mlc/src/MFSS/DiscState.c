// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The DiscState class encapsulates the necessary state
                   information for the state space of FSS on optimal
		   discretization.

		   This information includes:
		     (1) An array of ints indicating the presence
		         (or absence) of each possible feature.
		     (2) The gen_succ() function which generates all
		         possible successor states from the current state.
			 The successor states are states which can be
			 reached by adding or removing a feature from the
			 current state.  This function is also responsible
			 for evaluating successors and adding them to the
			 existing state space.
		     (3) An eval() function which determines the "fitness"
		         of a state.  The eval() function runs an accuracy
			 estimator using the inducer and the projected
			 traiing set to determine the fitness of the
			 feature subset.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : Dan Sommerfield                                6/12/95
                   Refitted to new search framework
                 James Dougherty
                   Initial revision (.h,.c)                    01/25/95
***************************************************************************/

#include <basics.h>
#include <DiscState.h>
#include <DFInducer.h>
#include <env_inducer.h>

//RCSID("MLC++, $RCSfile: DiscState.c,v $ $Revision: 1.5 $")

/***************************************************************************
  Description : Determine the lower bound for an element in the feature
                  array.
  Comments    :
***************************************************************************/
int DiscInfo::lower_bound(int which)
{
   DBG(check_bounds());
   return lowerBound->index(which);
}

/***************************************************************************
  Description : Determine the upper bound for an element in the feature
                  array
  Comments    :
***************************************************************************/
int DiscInfo::upper_bound(int which)
{
   DBG(check_bounds());
   return upperBound->index(which);
}

/***************************************************************************
  Description : Display a representation of the values in the array.
  Comments    : 
***************************************************************************/
void DiscInfo::display_values(const Array<int>& values, MLCOStream& out) const
{
   out << values;
}


/***************************************************************************
  Description : Pre-compute upper and lower bounds.
  Comments    :
***************************************************************************/
void DiscInfo::compute_bounds(const SchemaRC& /* schema */)
{
   err << "DiscInfo::compute_bounds: should not be called" << fatal_error;
}

// Compute bounds based on initial node.  Allow only one up one down for reals
void DiscInfo::compute_bounds(const SchemaRC& schema,
			      const Array<int>* initial)
{
   delete lowerBound;
   delete upperBound;

   if (initial && initial->size() != schema.num_attr())
      err << "DiscInfo::compute_bounds: array size mismatch" << fatal_error;

   lowerBound = new Array<int>(schema.num_attr());
   upperBound = new Array<int>(schema.num_attr());
   
   for(int i=0; i<schema.num_attr(); i++) {
      if(schema.attr_info(i).can_cast_to_nominal()) {
	 if (does_fss()) {
	    lowerBound->index(i) = 0;
	    upperBound->index(i) = 1;
	 }
	 else {
	    lowerBound->index(i) = 1;
	    upperBound->index(i) = 1;
	 }
      }
      else if( schema.attr_info(i).can_cast_to_real() ) {
	 if (initial) {
	    lowerBound->index(i) = max(initial->index(i) - 1, 1);
	    upperBound->index(i) = initial->index(i) + 1;
	 } else {
	    lowerBound->index(i) = 1;
	    upperBound->index(i) = SHORT_MAX;
	 }
      }
   }
}



/***************************************************************************
  Description : Make sure that the bounds arrays exist and are of the
                  correct length
  Comments    :
***************************************************************************/
void DiscInfo::check_bounds() const
{
   if(lowerBound == NULL || upperBound == NULL)
      err << "DiscState::check_bounds: bounds arrays are NULL" << fatal_error;
   if(lowerBound->size() != trainList->num_attr() ||
      upperBound->size() != trainList->num_attr())
      err << "DiscState::check_bounds: bounds arrays are of incorrect size: "
	 << "size = " << lowerBound->size() << "(should be " <<
	 trainList->num_attr() << ")" << fatal_error;
}


/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
DiscState::DiscState(Array<int> *& featureSubset, const AccEstInfo& gI)
   : CompState(featureSubset, gI)
{
   complexity = 0;
   for(int i = 0; i < get_info().size(); i++) {
      if (get_info()[i])
	 complexity += get_info()[i];
   }
}

/***************************************************************************
  Description : construct_lists builds both training and test lists for
                  the evaluation at once.
		The training set consists of only those features present in
		  the subset.  The test set (used to determine real accuracy)
		  is the projected version of the original test set.
  Comments    :
***************************************************************************/
void DiscState::construct_lists(AccEstInfo *,
				InstanceList *& trainList,
				InstanceList *& testList)
{
   // Build the discretizors
   LogOptions logOptions(get_log_options());
   logOptions.set_log_level(max(0,get_log_level() - 1));

   // Set the training list to the bag we just discretized
   // Note that we create a Boolean representation of
   // the discretization vector.
   const SchemaRC& schema = trainList->get_schema();
   BoolArray attrMask(0, get_info().size());
   // A single interval for reals is almost the same as projecting
   //   it out.  We project because it's faster to learn and there's
   //   really no way to get down to 0 as we have the setup.
   //   (should be something to think about).
   for (int i = 0; i < get_info().size(); i++)
      if (schema.attr_info(i).can_cast_to_real())
	 attrMask[i] = get_info()[i] > 1;
      else
	 attrMask[i] = get_info()[i] > 0;

   trainList = &(trainList->project(attrMask)->cast_to_instance_list());
   if(testList)
      testList = &(testList->project(attrMask)->
		   cast_to_instance_list());
}

/***************************************************************************
  Description : Since we created the new lists in construct_lists with new,
                  we have to delete them here.
  Comments    :
***************************************************************************/
void DiscState::destruct_lists(AccEstInfo *,
			       InstanceList *trainList,
			       InstanceList *testList)
{
   delete trainList;
   delete testList;
}

/***************************************************************************
  Description : Set the discretization vector before evaluating here.
  Comments    : This function is called immediately before evaluation takes
                  place.
***************************************************************************/
void DiscState::pre_eval(AccEstInfo *globalInfo)
{
   // we need to cast the inducer, so check that the cast is safe first
   if ( DF_INDUCER != globalInfo->inducer->class_id())
      err << "DiscState::eval: baseInducer not a DFInducer." << fatal_error;
   
   DiscFilterInducer& dtb = *((class DiscFilterInducer*)
				 globalInfo->inducer);

   const SchemaRC& schema = globalInfo->trainList->get_schema();
   // Remove from discVect those elements that have 0 
   DynamicArray<int> discVect(0);
   for (int i = 0; i < get_info().size(); i++) {
      if (schema.attr_info(i).can_cast_to_real()) {
	 if (get_info()[i] > 1)
	    discVect[discVect.size()] = get_info()[i];
      } else {
	 if (get_info()[i] > 0)
	    discVect[discVect.size()] = get_info()[i];
      }
   }

   LOG(3, "Vector after projection is " << discVect << endl);
   dtb.set_disc(discVect);
}


/***************************************************************************
  Description : Display the identifying string for this state
  Comments    :
***************************************************************************/
void DiscState::display_info(MLCOStream& stream) const
{
   if(get_eval_num() > NOT_EVALUATED)
      stream << "#" << get_eval_num();
   else
      stream << "#?";
   stream << " [" << get_info() << "]";
}
