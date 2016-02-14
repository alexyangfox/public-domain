// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : BaggingInd is a inducer, given a main inducer, which
                   generates a set of inducers of its kind. Each inducer
		   has different training set randomly sampled from the
		   main training set. Note that different traning set will
		   make a different inducer.
		 BaggingCat is such a categorizer as contains the categorizers
		   induced by each inducer generated by BaggingInd. For more
		   info, see BaggingCat.c
  Comments     : 
  Complexity   :
  Enhancements :
  Assumptions  :
  History      : Yeogirl Yun                                      5/12/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <BaseInducer.h>
#include <MLCStream.h>
#include <GetOption.h>
#include <BaggingInd.h>
#include <BaggingCat.h>
#include <CtrInstList.h>
#include <env_inducer.h>
#include <Array.h>
#include <MRandom.h>
#include <CatTestResult.h>

const MString BAG_REPLICATIONS_HELP =
   "Number of inducer replications to generate (how many times to run)";

const MString BAG_PROPORTION_HELP =
   "Propotion of the given training set to pass on "
   "for each individual replication";

const MString BAG_UNIF_WEIGHTS_HELP =
   "If TRUE, sets all the weights to 1. Otherwise, sets weights to the "
   "accuracy value obtained from the sampled training data.";

const MString BAG_SEED_HELP =
   "seed for random generator. can duplicate the same result.";

const MString BAG_USE_ABOVE_AVG_WEIGHT_HELP =
   "If TRUE, only categorizers that have weights above averages vote.";

// default option static data members.
int BaggingInd::defaultNumReplication = 10;
Real BaggingInd::defaultProportion = 0.8;
Inducer *BaggingInd::defaultMainInducer = NULL;
Bool BaggingInd::defaultUnifWeights = TRUE;
Bool BaggingInd::defaultUseAboveAvgWeight = FALSE;
unsigned int BaggingInd::defaultRandomSeed = 7258789;

/*****************************************************************************
  Description : Integrity check. Checks option variables.
  Comments    : 
*****************************************************************************/
void BaggingInd::OK(int /* level */) const
{
   if (numReplication < 0) 
      err << "BaggingInd::OK: illegal numReplication value : " <<
	 numReplication << endl << "Should be greater than 0" << fatal_error;

   if (proportion < 0 || proportion > 1) 
      err << "BaggingInd::OK: illegal proportion value : " <<
	 proportion << endl << "Should be in range [0, 1]" << fatal_error;
}


/******************************************************************************
  Description  : Constructor, destructor
  Comments     :
******************************************************************************/
BaggingInd::BaggingInd(const MString& dscr)
   : Inducer(dscr), categorizer(NULL)
{
   set_num_replication();
   set_proportion();
   set_unif_weights(TRUE);
   set_use_above_avg_weight(defaultUseAboveAvgWeight);
   init_rand_num_gen(defaultRandomSeed);
   OK();
}


BaggingInd::~BaggingInd()
{
   OK();
   delete categorizer;
}


/*****************************************************************************
  Description  : Return TRUE iff the class has a valid categorizer.
  Comments     :
*****************************************************************************/
Bool BaggingInd::was_trained(Bool fatal_on_false) const
{
   if( fatal_on_false && categorizer == NULL )
      err << "BaggingInd::was_trained: No categorizer, "
	     "Call train() to create categorizer" << fatal_error;
   return categorizer != NULL;
}




/******************************************************************************
  Description  : Returns the categorizer that the inducer has generated.
  Comments     :
******************************************************************************/
const Categorizer& BaggingInd::get_categorizer() const
{
   was_trained(TRUE);
   return *categorizer;
}


/*****************************************************************************
  Description : Sets the number of replication inducers option.
  Comments    : 
*****************************************************************************/
void BaggingInd::set_num_replication(int num)
{
   numReplication = num;
}


/*****************************************************************************
  Description : Sets proportion value.
  Comments    : 
*****************************************************************************/
void BaggingInd::set_proportion(Real val)
{
   proportion = val;
}



/*****************************************************************************
  Description : Sets an inducer. Gets an ownership.
  Comments    : 
*****************************************************************************/
void BaggingInd::set_main_inducer(Inducer* ind)
{
   if (ind == NULL) 
      err << "BaggingInd::set_main_inducer: setting NULL inducer"
	   << fatal_error;
   mainInducer = ind;
   OK();
}


   

/******************************************************************************
  Description  : Test main inducer on the sample bag and the rest bag, obtain
                   weights from the test's accuracy, and construct BaggingCat
		   with numReplication number of such inducers and weights.
  Comments     : 
******************************************************************************/
void BaggingInd::train()
{
   OK();
   has_data();
   int size = TS->num_instances();

   // create a counter bag because some inducer need a counter bag.
   CtrInstanceBag ctrBag(TS->get_schema());
   for (Pix pix = TS->first(); pix; TS->next(pix)) 
      ctrBag.add_instance(TS->get_instance(pix));
   
   // create category set and weight set
   PtrArray<Categorizer *>* catSet =
      new PtrArray<Categorizer *>(0, numReplication, TRUE); // set to NULL,
                                                            // initially.

   // initially set weights to all 1's but it will be set again by
   // test accuracies if unifWeights option is FALSE.
   Array<Real>* weightSet =  new Array<Real>(0, numReplication, 1);

   int sampleSize = (int)(size * proportion);
   LOG(2, "Creating " << numReplication << " inducers, each trained on " <<
          sampleSize << " instances" << endl);

   // lower the log level
   int saveLogLevel = mainInducer->get_log_level();
   mainInducer->set_log_level(get_log_level() - 1);
   for (int i = 0; i < numReplication; i++) {
      InstanceBag restBag(ctrBag.get_schema());
      CtrInstanceBag* newBag =
	 &(ctrBag.independent_sample(sampleSize, &restBag, &rand_num_gen())->
	   cast_to_ctr_instance_bag());
      // takes ownership of newBag and newRestBag
      CtrInstanceList* newList = new CtrInstanceList(newBag);

      delete mainInducer->assign_data(newList);	       	 
      DBG(ASSERT(newList == NULL));
      mainInducer->train();
      if (!unifWeights) {
	 CatTestResult results(mainInducer->get_categorizer(),
			       mainInducer->instance_bag().
			       cast_to_instance_list(), restBag);
	 weightSet->index(i) =  results.accuracy();
         LOG(2, "Weight for inducer # " << i << " is " <<
   	     weightSet->index(i) << endl);
      }
      catSet->index(i) =  mainInducer->get_categorizer().copy();
      catSet->index(i)->set_log_level(get_log_level() - 1);
   }
   delete mainInducer->release_data(); // release the bag.
   mainInducer->set_log_level(saveLogLevel);
   
   // now create categorizer.
   delete categorizer;
   categorizer = new BaggingCat("Bagging Cat", catSet, weightSet);
   categorizer->set_log_level(get_log_level());
   categorizer->set_use_above_avg_weight(get_use_above_avg_weight());
}



/*****************************************************************************
  Description : Sets the options from environment variables.
  Comments    :
*****************************************************************************/
void BaggingInd::set_user_options(const MString& preFix)
{
   set_num_replication(
      get_option_int(preFix + "BAG_REPLICATIONS",
		     defaultNumReplication,
		     BAG_REPLICATIONS_HELP, FALSE));
   set_proportion(
      get_option_real(preFix + "BAG_PROPORTION",
		      defaultProportion,
		      BAG_PROPORTION_HELP, FALSE));
   set_unif_weights(
      get_option_bool(preFix + "BAG_UNIF_WEIGHTS",
		      defaultUnifWeights,
		      BAG_UNIF_WEIGHTS_HELP, FALSE));

   if (!get_unif_weights()) {
      set_use_above_avg_weight(
	 get_option_bool(preFix + "BAG_USE_ABOVE_AVG_WEIGHT",
			 defaultUseAboveAvgWeight,
			 BAG_USE_ABOVE_AVG_WEIGHT_HELP,
			 FALSE));
   }
   
   init_rand_num_gen(get_option_int(preFix + "BAG_SEED", defaultRandomSeed,
		     BAG_SEED_HELP, TRUE));
   
   // set inducer using environment variable.
   Inducer *ind = &env_inducer("BAG_")->cast_to_inducer();
   set_main_inducer(ind);
}   
  
