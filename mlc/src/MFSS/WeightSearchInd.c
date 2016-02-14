// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper inducer for optimal weights.
                 We search the space of weights for the ones with the highest
                    estimated accuracy.
  Assumptions  :
  Comments     : 
  Complexity   : not computed.
  Enhancements : Avoid the need to specify the inducer twice, although
                   this could be a useful feature.
  History      : Ronny Kohavi                                      13/30/95
                   Based on DiscSearchInducer and IBIBWInducer by Yogo.
***************************************************************************/

#include <basics.h>
#include <WeightSearchInd.h>
#include <CtrInstList.h>
#include <GetOption.h>
#include <FSSInducer.h>


RCSID("MLC++, $RCSfile: DiscSearchInd.c,v $ $Revision: 1.8 $")


// use compound option
const MString useCompoundHelp = "This option specifies whether or not to "
  "combine information about generated states in the search in an attempt "
  "to generate a better state more quickly.";

// complexity penalty option
const MString complexityPenaltyHelp = "This option specifies a multiplier "
  "which determines how much complexity of a state hurts its fitness.";

const int defaultNumWeights = 2;
const MString numWeightsHelp = "The number of weights between 0 and 1,"
   " excluding 0";

const Bool defaultStartFSS = FALSE;
const MString FSSHelp = "Start with FSS before weights";

/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
WeightSearchInducer::WeightSearchInducer(const MString& description,
				     BaseInducer *ind)
   : SearchInducer(description, ind),
     startWithFSS(defaultStartFSS)

{     
   // establish global info
   globalInfo = create_global_info();
   ASSERT(globalInfo->class_id() == WEIGHT_INFO);
   WeightInfo *weightInfo = (WeightInfo *)globalInfo;
   weightInfo->set_num_weights(defaultNumWeights);
}


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
WeightSearchInducer::~WeightSearchInducer()
{
}


/***************************************************************************
  Description : Gather options from the user
  Comments    :
***************************************************************************/
void WeightSearchInducer::set_user_options(const MString& prefix)
{
   // check global info
   has_global_info();
   ASSERT(globalInfo->class_id() == WEIGHT_INFO);
   WeightInfo *weightInfo = (WeightInfo *)globalInfo;

   SearchInducer::set_user_options(prefix);

   int numWeights = get_option_int(prefix + "NUM_WEIGHTS", defaultNumWeights,
		     numWeightsHelp, FALSE);
   weightInfo->set_num_weights(numWeights);

   startWithFSS = get_option_bool(prefix + "START_WITH_FSS", 
				  defaultStartFSS, FSSHelp, FALSE);
}


   
/***************************************************************************
  Description : Display info
  Comments    :
***************************************************************************/
void WeightSearchInducer::display(MLCOStream& stream) const
{
   has_global_info();
   globalInfo->accEst.display_settings(stream);
}

DEF_DISPLAY(WeightSearchInducer);


/***************************************************************************
  Description : Determine initial values for state
  Comments    : Side effect of building the global info's bounds.
***************************************************************************/
Array<int> *WeightSearchInducer::create_initial_info(InstanceBag*) 
{
   // compute global bounds for info
   has_global_info();
   ASSERT(globalInfo->class_id() == WEIGHT_INFO);
   WeightInfo *weightInfo = (WeightInfo *)globalInfo;
   ASSERT(globalInfo->trainList);
   weightInfo->compute_bounds(globalInfo->trainList->get_schema());

   if (startWithFSS) {
      FSSInducer ind("fss for weights");
      ind.set_user_options("WEIGHT_FSS_");
      ind.set_log_level(get_log_level());
      InstanceBag* data = globalInfo->trainList;
      ind.assign_data(data);
      ind.train();
      data = ind.release_data();
      ASSERT(globalInfo->trainList == data); // make sure we gave it back.
      const State<Array<int>, AccEstInfo>& finState = ind.get_final_state();
      Array<int>* initState = new Array<int>(finState.get_info(), ctorDummy);
      // Transforms 1's to maximum weight
      for (int i = 0; i < initState->size(); i++)
	 if (initState->index(i) != 0)
	    initState->index(i) = weightInfo->get_num_weights();
      return initState;
   } else // Start in the middle.
      return new Array<int>(0, globalInfo->trainList->num_attr(),
			    weightInfo->get_num_weights() / 2);
}


/***************************************************************************
  Description : Convert a state description to a Categorizer
  Comments    : This function is conceptually const, although it needs
                  to temporarily un-const members inside, so we need to
		  cast away constness.
***************************************************************************/
Categorizer *WeightSearchInducer::state_to_categorizer(
   const State<Array<int>, AccEstInfo>& state) const
{
   ASSERT(globalInfo);
   if (IB_INDUCER != globalInfo->inducer->class_id())
      err << "WeightState::eval: baseInducer not a IBInducer." << fatal_error;

   IBInducer& ibInd = *((class IBInducer*)globalInfo->inducer);

   Array<Real> weights(state.get_info().size());
   for (int i = 0; i < state.get_info().size(); i++)
      weights[i] = state.get_info()[i] / Real(globalInfo->upper_bound(i));
   ibInd.set_weights(weights);

   WeightSearchInducer *thisNC = (WeightSearchInducer *)this;
   ibInd.assign_data(thisNC->TS);			// sets TS to NULL
   ibInd.train();

   Categorizer *theCat = ibInd.get_categorizer().copy();
   
   // restore state
   thisNC->TS = ibInd.release_data();

   return theCat;
}



