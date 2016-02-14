// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : AccEstState implements a state in the search space whose
                   fitness is based on accuracy estimation using an
		   induction algorithm and some data set.  It therefore
		   provides the eval() function.
		 The class AccEstInfo is provided to serve as the base
		   class for global state information for use in all
		   classes derived from AccEstState.  It is derived from
		   StateGlobalInfo.
		 AccEstState is not a template class; it is based on a
		   State which is always templated on an integer array
		   for the local info and some derivative of AccEstState
		   for the global info.  AccEstInfo provides the class_id()
		   function so that classes derived from AccEstState may
		   safely cast the global info type (which must be derived
		   from AccEstInfo) to a more specific type.
		 AccEstState has two more abstract classes derived from it.
		   These are CompState and OrderState.  CompState implements
		   A state which is based on accuracy estimation and which
		   is capable of generating Compound Operators.  OrderState
		   implements a state in the space of orderings.

		 Classes derived from AccEstState must provide the following
		   functions (pure virtuals):
		 pre_eval:  this function is called by eval() immediately
		   before accuracy estimation.  Use this function to modify
		   some aspect of the inducer (such as settings) being used
		   in estimation.  Typical uses of pre_eval are to change
		   inducer settings based on values in the LocalInfo.
		   The default pre_eval does nothing.
		 construct_lists:  this function allows you to alter the
		   training and test lists in the global info for use in
		   accuracy estimation.  The trainList and testList parameters
		   are input/output.  On input they point to the training
		   and testing lists in the global info.  On output they
		   should point to the lists for use in accuracy estimation.
		   testList will be NULL if one is not available.
		   The default does nothing.
		 destruct_lists:  if construct_lists allocates storage, use
		   destruct_lists to free it.  Otherwise leave the default,
		   which does nothing.
		 Look at DiscState for an example of how to use all three
		   pure virtual functions.
		 You must also provide an eval() function if you do not
		   derive from CompState or OrderState.
		   
  Assumptions  :
  Comments     : This class makes no assumptions about the array of int
                   which serves as the LocalInfo.  Derived classes may
		   use this array as they please.
  Complexity   : Complexity of eval() depends on the induction algorithm
                   and accuracy estimation method being used.
  Enhancements : AccEstInfo anticipates CompState right now.  Remove this
                   anticipation.
  History      : Dan Sommerfield                                    5/07/95
                   Initial revision (.h,.c)
                 Brian Frasca                                       5/14/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <GetOption.h>
#include <AccEstState.h>
#include <env_inducer.h>


// RCSID("MLC++, $RCSfile: AccEstState.c,v $ $Revision: 1.11 $")


// use compound option
const MString useCompoundHelp = "This option specifies whether or not to "
  "combine information about generated states in the search in an attempt "
  "to generate a better state more quickly.";

// complexity penalty option
const MString complexityPenaltyHelp = "This option specifies a multiplier "
  "which determines how much complexity of a state hurts its fitness.";
const Real defaultComplexityPenalty = 0;


// required for template instantiation
typedef State<Array<int>, AccEstInfo> AccEstStateTypedef;
SET_DLLPIX_CLEAR(AccEstStateTypedef *,NULL);

/***************************************************************************
  Description : Constructor
  Comments    : 
***************************************************************************/
AccEstInfo::AccEstInfo()
   : inducer(NULL),
     trainList(NULL),
     testList(NULL),
     seed(7258789),
     useCompound(TRUE),
     complexityPenalty(0)
{
}


/***************************************************************************
  Description : Sets all user options used within this state's global info
  Comments    : Does not set all options (such as the files).
                Does not set Inducer, so that it may be set from outside.
***************************************************************************/
void AccEstInfo::set_user_options(const MString& prefix)
{
   useCompound =
      get_option_bool(prefix + "USE_COMPOUND", TRUE,
		      useCompoundHelp, TRUE);

   complexityPenalty =
      get_option_real(prefix + "CMPLX_PENALTY",
		      defaultComplexityPenalty, complexityPenaltyHelp, TRUE);

   accEst.set_user_options(prefix);
   seed = accEst.get_seed();
}


/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
AccEstState::AccEstState(Array<int>*& initialInfo, const AccEstInfo& gI)
   : State<Array<int>, AccEstInfo>(initialInfo, gI)
{
   numEvaluations = 0;
   complexity = 0;
}

/***************************************************************************
  Description : The evaluation function uses the accuracy estimate found by
                  using the options given to OptionAccEst.  The training set
		  consists of only those features present in the subset.
		  The test set (used to determine real accuracy) is the
		  projected version of the original test set.
		The type parameter specifies which accuracy to compute in
		  determining fitness.  Choice is between real accuracy
		  only, estimated accuracy only, or both.
  Comments    : We need to insure that the seed passed to the accuracy
                  estimator is the same for each state.  However, if we
		  reevaluate the same state, we want a different seed to
		  avoid duplication of results.  Therefore, we reseed the
		  accuracy estimator with its original seed + the number
		  of node evaluations.
***************************************************************************/
Real AccEstState::eval(AccEstInfo *acInfo, Bool computeReal,
		       Bool computeEstimated)
{
   // it is an error to set both compute booleans to false
   if(!computeReal && !computeEstimated)
      err << "FSSState::eval: one of computeReal or computeEstimated must "
	 "be TRUE" << fatal_error;

   // construct training and test lists for use in evaluation.  The
   // defaults are just the global info values themselves
   InstanceList *estTrainList = acInfo->trainList;
   InstanceList *estTestList = acInfo->testList;
   if(!computeReal)
      estTestList = NULL;
   construct_lists(acInfo, estTrainList, estTestList);
 
   // set seed to insure comparable results across all states,
   // but add the number of evaluations so that we don't wind up repeating
   // the save evaluation when we evaluate multiple times.
   acInfo->accEst.set_seed(acInfo->seed + numEvaluations);

   // Invoke any pre-evaluation functions here
   pre_eval(acInfo);
   
   // Evaluate state accuracy using the accuracy estimator
   // specified in AccEstInfo.
   // Results of multiple estimations are accumulated into the accData
   // that we store within this state.
   Real acc;
   if(computeReal && computeEstimated) {
      acc = acInfo->accEst.estimate_accuracy(*acInfo->inducer,
					     *estTrainList,
					     *estTestList,
					     &accData);
   }
   else if(computeReal) {
      // temporarily set acc estimator to "test-set" to get the
      // real accuracy.
      AccEstDispatch::AccEstimationMethod oldMethod =
	 acInfo->accEst.get_accuracy_estimator();
      acInfo->accEst.set_accuracy_estimator(AccEstDispatch::testSet);
      acc = acInfo->accEst.estimate_accuracy(*acInfo->inducer,
					     *estTrainList,
					     *estTestList);
      acInfo->accEst.set_accuracy_estimator(oldMethod);
      accData.set_real(acInfo->accEst.get_acc_data().real_accuracy(),
		       estTestList->num_instances());
   }
   else if(computeEstimated) {
	 // leaving out a test set causes real accuracy not to be
	 // computed.
	 acc = acInfo->accEst.estimate_accuracy(*acInfo->inducer,
						*estTrainList,
						&accData);
   }
   else
      ASSERT(FALSE);

   if(computeEstimated) {
      numEvaluations++;   
      evalCost = accData.get_cost();
      fitness = acc -
	 acInfo->complexityPenalty * complexity;
      LOG(5, "Computing standard deviation.  Size = " << accData.size() <<
	  endl);
      if(accData.has_std_dev())
	 stdDev = accData.std_dev();
      else
	 stdDev = UNDEFINED_VARIANCE;
   }

   destruct_lists(acInfo, estTrainList, estTestList);
   return fitness;
}


/***************************************************************************
  Description : Sets the final state to be filled in the pGraph.
  Comments    : This function is called from StateSpace.
***************************************************************************/
void AccEstState::set_final_state()
{
   set_graph_options(",style=filled,color=gray95");
}

/***************************************************************************
  Description : display_stats displays summary statistics for this state.
                  These include fitness, standard deviation, real accuracy,
		  and number of times evaluated.
  Comments    :
***************************************************************************/
void AccEstState::display_stats(MLCOStream& stream) const
{
   if(numEvaluations == 0) {
      stream << "<unevaluated>";
   }
   else {
      stream << "accuracy: " << accData <<
	        " cost: " << accData.get_cost() <<
	        " complexity: " << complexity;
   }
}

/***************************************************************************
  Description : displays an identifying string for this state.  The string
                  includes the node number (if it is in the graph), and
		  the feature subset.  This function does NOT print the
		  summary statistics (use display_stats for that).
  Comments    : The global info (acInfo) may be NULL.  In this case, no
                  local information will be printed for this state by
		  default.  This can be changed by derived classes.
***************************************************************************/
void AccEstState::display_info(MLCOStream& stream) const
{
   if(get_eval_num() > NOT_EVALUATED)
      stream << "#" << get_eval_num();
   else
      stream << "#?";

   stream << " [";
   globalInfo.display_values(get_info(), stream);
   stream << "]";
}

/***************************************************************************
  Description : Override display_for_graph to use the entire accData we
                  store instead of just the fitness (what we use in
		  State)
  Comments    :
***************************************************************************/
void AccEstState::display_for_graph(MLCOStream& stream) const {
      display_info(stream);
      stream << "\\n";
      accData.dot_display(stream);
}
   









