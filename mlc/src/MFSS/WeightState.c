// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The WeightState class encapsulates the necessary state
                   information for the state space of FSS on weights.
	         The array of ints indicating the weight factor,
		   which is then translated into real weights between 0 and 1.
  Assumptions  :
  Comments     :
  Complexity   :
  Enhancements :
  History      : Ronny Kohavi                                 12/30/95
                   Based on DiscState by Dan.
***************************************************************************/

#include <basics.h>
#include <WeightState.h>
#include <env_inducer.h>
#include <IBInducer.h>


// RCSID("MLC++, $RCSfile: WeightState.c,v $ $Revision: 1.5 $")

/***************************************************************************
  Description : Convert int to weight
  Comments    : private
***************************************************************************/

Real WeightInfo::int2weight(int weightNum, int num) const
{
   // @@ should really be in OK() if there was one.
   int lbound = lowerBound->index(weightNum);
   int ubound = upperBound->index(weightNum);
   ASSERT(lbound == 0);
   ASSERT(ubound == numWeights); // right now all have same weight, but
                                 // this might change in the future.

   ASSERT(numWeights > 0);
   if (num > numWeights)
      err << "WeightState::int2weight: illegal weight:" << num
	  << " Number of weights: " << numWeights << fatal_error;
   return num / Real(numWeights);
}
   


/***************************************************************************
  Description : Determine the lower bound for an element in the feature
                  array.
  Comments    :
***************************************************************************/
int WeightInfo::lower_bound(int which)
{
   DBG(check_bounds());
   return lowerBound->index(which);
}

/***************************************************************************
  Description : Determine the upper bound for an element in the feature
                  array
  Comments    :
***************************************************************************/
int WeightInfo::upper_bound(int which)
{
   DBG(check_bounds());
   return upperBound->index(which);
}

/***************************************************************************
  Description : Display a representation of the values in the array.
  Comments    : 
***************************************************************************/
void WeightInfo::display_values(const Array<int>& values,
				MLCOStream& stream) const
{
   for (int i = 0; i < values.size() - 1; i++)
      stream <<  int2weight(i, values.index(i)) << ", ";

   if (values.size() > 0)
      stream << int2weight(i, values.index(i));  // no trailing comma.
}


/***************************************************************************
  Description : Pre-compute upper and lower bounds.
  Comments    :
***************************************************************************/
void WeightInfo::compute_bounds(const SchemaRC& schema)
{
   delete lowerBound;
   delete upperBound;

   ASSERT(numWeights > 0);
   lowerBound = new Array<int>(0, schema.num_attr(), 0);
   upperBound = new Array<int>(0, schema.num_attr(), numWeights);
}

/***************************************************************************
  Description : Make sure that the bounds arrays exist and are of the
                  correct length
  Comments    :
***************************************************************************/
void WeightInfo::check_bounds() const
{
   if(lowerBound == NULL || upperBound == NULL)
      err << "WeightState::check_bounds: bounds arrays are NULL" <<fatal_error;

   if(lowerBound->size() != trainList->num_attr() ||
      upperBound->size() != trainList->num_attr())
      err << "WeightState::check_bounds: bounds arrays are of incorrect size: "
	 << "size = " << lowerBound->size() << "(should be " <<
	 trainList->num_attr() << ")" << fatal_error;
}


/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
WeightState::WeightState(Array<int> *& featureSubset, const AccEstInfo& gI)
   : CompState(featureSubset, gI)
{
   complexity = 0;
   for(int i = 0; i < get_info().size(); i++) {
      if (get_info()[i])
	 complexity += get_info()[i];
   }
}

/***************************************************************************
  Description : Set the discretization vector before evaluating here.
  Comments    : This function is called immediately before evaluation takes
                  place.
***************************************************************************/
void WeightState::pre_eval(AccEstInfo *globalInfo)
{
   // we only support IBInducer now
   if (IB_INDUCER != globalInfo->inducer->class_id())
      err << "WeightState::eval: baseInducer not a IBInducer." << fatal_error;
   
   // safe cast to only supp
   IBInducer& ibInd = *((class IBInducer*)globalInfo->inducer);

   Array<Real> weights(get_info().size());
   for (int i = 0; i < get_info().size(); i++)
      weights[i] = get_info()[i] / Real(globalInfo->upper_bound(i));
   ibInd.set_weights(weights);
   LOG(4, "Weights are: " << weights << endl);
}


/***************************************************************************
  Description : Display the identifying string for this state
  Comments    :
***************************************************************************/
void WeightState::display_info(MLCOStream& stream) const
{
   if(get_eval_num() > NOT_EVALUATED)
      stream << "#" << get_eval_num();
   else
      stream << "#?";
   stream << " ["; 
   globalInfo.display_values(get_info(), stream);
   stream << "]";
}

