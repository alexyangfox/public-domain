// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : LazyDTInducer is a variation of decision tree inducers. It
                   builds an optimal path of a decision tree based on the
		   data set and test or query instance.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                      1/10/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <LazyDTInducer.h>
#include <GetOption.h>

const MString minSplitHelp = "This option chooses the minimum instances "
   "which the child the instance is classified into must have.";

const MString useAttrEqCatHelp = "Drop instances one attribute value at each "
   "split.";

const MString mutualInfoRateHelp = "It should be between 0 and 1, and denotes "
   "the proportion to use entropyGain versus minMutualInfo to decide to "
   "split on an attribute. The value of 0 means only the use of "
   "entropyGain while 1 means only the use of minMutualInfo.";

const MString multiSplitRateHelp = "The ratio you're "
   "allowed to be off from the best split and still be allowed to vote.";

const MString delMemoryHelp = "With this option true, small caches are "
   "deleted. ";

const MString delRateHelp = "Any cache that contains less than delRate "
   "percent of the root's instances is deleted after it is used.";

const MString multiSplitOnHelp = "Use multi splits on almost equally best "
   "nodes depending on multiSplitRate option.";

const MString pessimisticZHelp = "Pessimistic Z-value for entropy.  Higher "
   "numbers make it more pessimistic and favor larger number of instances.";


const MString penaltyPowerHelp = "Power to raise the proportion of instances "
   "in the child before multiplying by gain.";


const int defaultMinSplit = 1;
const Bool defaultUseAttrEqCat = TRUE;
const Bool defaultMultiSplitOn = TRUE;
const Real defaultMutualInfoRate = 0;
const Real defaultMultiSplitRate = 0.1;
const Real defaultDelRate = 0.0;
const Real defaultPessimisticZ = 0;
const Real defaultPenaltyPower = 0;

/*****************************************************************************
  Description : Constructor. Passes over the description argument to
                  CtrInducer.
  Comments    :
*****************************************************************************/
LazyDTInducer::LazyDTInducer(const MString& description)
   : CtrInducer(description)
{
   categorizer = NULL;
   minSplit = defaultMinSplit;
   mutualInfoRate = defaultMutualInfoRate;
   useAttrEqCat = defaultUseAttrEqCat;
   multiSplitRate = defaultMultiSplitRate;
   delRate = defaultDelRate;
   multiSplitOn = defaultMultiSplitOn;
   pessimisticZ = defaultPessimisticZ;
   penaltyPower = defaultPenaltyPower;
}   
			     

/*****************************************************************************
  Description : Returns TRUE iff the inducer was trained. Aborts when
                  fatal_on_false is TRUE and the inducer was not trained.
  Comments    :
*****************************************************************************/
Bool LazyDTInducer::was_trained(Bool fatal_on_false) const
{
   if( fatal_on_false && categorizer == NULL )
      err << "LazyDTInducer::was_trained: No categorizer, "
	     "Call train() to create categorizer" << fatal_error;
   return categorizer != NULL;
}      
   

/*****************************************************************************
  Description : Returns the const version of the categorizer. Does not give
                  the ownership up. 
	        The inducer must have been trained before. Otherwise, it
		  aborts.
  Comments    :
*****************************************************************************/
const Categorizer& LazyDTInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *categorizer;
}      


/*****************************************************************************
  Description : Builds an categorizer by sending the dataset to
                  LazyDTCategorizer. Learning occurs when categorization
		  occurs. 
  Comments    :
*****************************************************************************/
void LazyDTInducer::train()
{
   OK();
   has_data();

   delete categorizer;
   categorizer = new LazyDTCategorizer(TS_with_counters(), description());
   categorizer->set_log_options(get_log_options());
   categorizer->set_min_split(minSplit);
   categorizer->set_mutual_info_rate(mutualInfoRate);
   categorizer->set_multi_split_rate(multiSplitRate);
   categorizer->set_multi_split_bool(multiSplitOn);   
   categorizer->set_del_rate(delRate);
   categorizer->set_use_attr_eq_cat_bool(useAttrEqCat);         
   categorizer->set_pessimisticZ(pessimisticZ);
   categorizer->set_penalty_power(penaltyPower);
}


/*****************************************************************************
  Description : Sets the minSplit value which will be sent to LazyDTCategorizer
                  after a training.
  Comments    :
*****************************************************************************/
void LazyDTInducer::set_min_split(int val)
{
   if (val <= 0) 
      err << "LazyDTInducer::set_min_split: minSplit value should be greater "
	 "than 0. The input value was : " << val << fatal_error;
   minSplit = val;
}

/*****************************************************************************
  Description : Sets mutualInfoRate value.
  Comments    :
*****************************************************************************/
void LazyDTInducer::set_mutual_info_rate(Real val)
{
   if (val < 0 || val > 1) 
      err << "LazyDTInducer::set_mutual_info_rate: it should be [0, 1]. "
	 "The input value was : " << val << fatal_error;
   mutualInfoRate = val;
}



/*****************************************************************************
  Description : Sets the user options. Gets the option values from
                  environment variables.
  Comments    :
*****************************************************************************/
void LazyDTInducer::set_user_options(const MString& prefix)
{
   minSplit = 
      get_option_int_range(prefix + "MIN_SPLIT",
			   defaultMinSplit, 1, INT_MAX, minSplitHelp, TRUE);
   useAttrEqCat =
      get_option_bool(prefix + "ATTR_EQ_CAT",
		      defaultUseAttrEqCat, useAttrEqCatHelp, TRUE);

   mutualInfoRate =
      get_option_real_range(prefix + "MINFO_RATE",
			    defaultMutualInfoRate, 0, 1, mutualInfoRateHelp,
			    TRUE);
   multiSplitOn = 
      get_option_bool(prefix + "USE_MSPLIT",
		      defaultMultiSplitOn, multiSplitOnHelp, TRUE);

   if (multiSplitOn)
      multiSplitRate =
	 get_option_real_range(prefix + "MSPLIT_RATE",
			       defaultMultiSplitRate, 0, 1, multiSplitRateHelp,
			       TRUE);
   else
      multiSplitRate = 0;

   delRate =
      get_option_real_range(prefix + "DEL_RATE",
			    defaultDelRate, 0, 1, delRateHelp,
			    TRUE);            

   pessimisticZ =
      get_option_real_range(prefix + "PESSIMISTIC_Z",
			    defaultPessimisticZ, 0, 10, pessimisticZHelp,
			    TRUE);            

   penaltyPower  = 
      get_option_real_range(prefix + "PENALTY_POWER",
			    defaultPenaltyPower, 0, 10, penaltyPowerHelp,
			    TRUE);            

}


