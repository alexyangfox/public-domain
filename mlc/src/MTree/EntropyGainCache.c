// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : EntropyGainCache is derived off SplitInfoCache. It caches
                   entropy gain between parent and child bags.
  Comments     :
  Assumptions  :
  Enhancements :
  Complexity   :
  History      : Yeogirl Yun, Ronny Kohavi                    2/28/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <entropy.h>
#include <AttrCat.h>
#include <AttrEqCat.h>
#include <EntropyGainCache.h>
#include <math.h>

const int defaultMinSplit = 1;
const int defaultUseAttrEqCat = FALSE;
const Real defaultMutualInfoRate = 0;
const Real defaultPessimisticZ = 0;
const Real defaultPenaltyPower = 0;


/***************************************************************************
  Description : Constructor for EntropyGainCacheOption
  Comments    :
***************************************************************************/

EntropyGainCacheOption::EntropyGainCacheOption()
   : minSplit(defaultMinSplit),
     useAttrEqCat(defaultUseAttrEqCat),
     mutualInfoRate(defaultMutualInfoRate),
     pessimisticZ(defaultPessimisticZ),
     penaltyPower(defaultPenaltyPower)
{}


/***************************************************************************
  Description : OK for EntropyGainCacheOption. 
  Comments    :
***************************************************************************/

void EntropyGainCacheOption::OK(int /* level  */) const
{
   if (minSplit < 1)
      err << "EntropyGainCacheOption::OK : minSplit should be >= 1. "
	 "It is set " << minSplit << fatal_error;

   if (mutualInfoRate < 0 || mutualInfoRate > 1)
      err << "EntropyGainCacheOption::OK: bad mutual information rate "
	  << mutualInfoRate << fatal_error;

   if (pessimisticZ < 0 || pessimisticZ > 10)
      err << "EntropyGainCacheOption::OK: bad pessimistic Z value "
	  << pessimisticZ << fatal_error;

   // Penalty power could really be nagative or > 1.  This is safety only
   if (penaltyPower < 0 || penaltyPower > 10)
      err << "EntropyGainCacheOption::OK: bad penalty power "
	  << penaltyPower << fatal_error;
}



/*****************************************************************************
  Description : Build the infoMatrix(attrNum, val) element by copying sub
                  bag and storing entropy gain.
  Comments    :
*****************************************************************************/

static void init_info_matrix(Array<CacheInfo>*& row, int numValues)
{
   row = new Array<CacheInfo>(UNKNOWN_NOMINAL_VAL, numValues + 1);
   for (int i = 0; i < numValues + 1; i++) {
      row->index(i).infoValue = -REAL_MAX;
      row->index(i).numInstances = -1;
   }
}



void EntropyGainCache::build_info_values(int attrNum)
{
   const NominalAttrInfo& nai = get_bag().
      attr_info(attrNum).cast_to_nominal();   

   if (get_bag().counters().label_num_vals() == 1) {
      LOG(2, "Build_info: pure bag when checking attribute " 
	  << attrNum << endl);
      init_info_matrix(infoMatrix[attrNum], nai.num_values());
   }
   else if (get_bag().counters().attr_num_vals(attrNum) == 1) {
      LOG(2, "Build_info: only one value for attribute " << attrNum << endl);
      init_info_matrix(infoMatrix[attrNum], nai.num_values());      
   }
   else if (num_instances() <= options.get_min_split()) {
      LOG(2, "Build_info: attribute " << attrNum << " has <= "
	  << options.get_min_split() << " instance(s)." << endl);
      init_info_matrix(infoMatrix[attrNum], nai.num_values());      
   }
   else {
      const NominalAttrInfo& nai =
	 get_bag().attr_info(attrNum).cast_to_nominal();

      LOG(2, "Building info for attribute " << nai.name() << " ("
	  << attrNum << ')' << endl);

      // create one row of this matrix.
      infoMatrix[attrNum] = new Array<CacheInfo>
	 (UNKNOWN_NOMINAL_VAL, nai.num_values() + 1);
   
      if (!options.get_use_attr_eq_cat_bool()) 
	 use_attr_cat(attrNum);
      else // Remove attribute value at a time
	 use_attr_eq_cat(attrNum);
   }
}



/*****************************************************************************
  Description : Returns CtrInstanceBag* after actually subcopying the bag from
                  the current bag.
  Comments    : Returns the ownership of the bag.
*****************************************************************************/
CtrInstanceBag* EntropyGainCache::split_child(int attrNum, NominalVal ourVal,
					      NominalVal splitVal)
{
   ASSERT(ourVal >= UNKNOWN_NOMINAL_VAL);
   ASSERT(splitVal >= UNKNOWN_NOMINAL_VAL);
   ASSERT(options.get_use_attr_eq_cat_bool() || (splitVal == ourVal));

   LOG(2, "split_child splitting on attribute " << attrNum 
       << " with value " << splitVal << " ours is " << ourVal << " exclude "
       << bool_to_string(ourVal != splitVal) << endl);
   return get_bag().find_attr_val(attrNum, splitVal, ourVal != splitVal, TRUE);
}


/*****************************************************************************
  Description : Builds matrix using AttrCategorizer.
  Comments    :
*****************************************************************************/
void EntropyGainCache::use_attr_cat(int attrNum)
{
   const NominalAttrInfo& nai = get_bag().attr_info(attrNum).cast_to_nominal();
   Real parentWeightedEntropy =  // This is really log-k
      weighted_entropy(get_log_options(),get_bag(),get_bag());
   Array<int> parentLabelCount(get_bag().counters().label_counts(),
				ctorDummy);
   const Array2<int>* valueCounts(get_bag().counters().
				  value_counts()[attrNum]);

   for (int j = UNKNOWN_NOMINAL_VAL; j < nai.num_values(); j++) {
      // Profiling showed setting the string takes 30% of the time for this
      // routine, so we only set it if we will display it!
      MString attrValStr;
      IFLOG(2, attrValStr = nai.get_value(j) + " (" + MString(j,0) + ")");
      Array<int> labelCount(parentLabelCount.low(),parentLabelCount.size());
      int numInst = 0;
      for (int label = labelCount.low(); label <= labelCount.high();
					 label++) {
	 labelCount[label] = (*valueCounts)(label,j);
	 numInst += labelCount[label];
      }
      
      if (numInst == 0) {
	 LOG(2, "Value " << attrValStr << " has no instances" << endl);
	 (*infoMatrix[attrNum])[j].infoValue = -REAL_MAX;
	 (*infoMatrix[attrNum])[j].numInstances = -1;
      } 
      // Watch out for division by zero when taking log_bin
      else if (get_bag().get_schema().num_attr_values(attrNum) == 1) {
	 LOG(2, "Value " << attrValStr << " has one attribute value" << endl);
	 (*infoMatrix[attrNum])[j].infoValue = -REAL_MAX;
	 (*infoMatrix[attrNum])[j].numInstances = -1;
      } else {
	 Real entropy = weighted_entropy(get_log_options(),
			 labelCount, parentLabelCount, numInst, 
			 num_instances(), options.get_pessimisticZ());
	 Real gain = parentWeightedEntropy - entropy;
         // Penalize for multi-way splits
	 gain /= log_bin(get_bag().get_schema().num_attr_values(attrNum));
	 Real miRate = options.get_mutual_info_rate();
	 if (miRate > 0) {
	    Real mutualInfo = mutual_info(get_log_options(), get_bag(),
					  attrNum);
	    gain = (1 - miRate) * gain + miRate * mutualInfo;
	 }
	 if (gain < 0 && gain >= -REAL_EPSILON)
	    gain = 0;
	 ASSERT(gain >= 0);

	 LOG(2, "Value " << attrValStr << " has gain " << gain << endl);
	 (*infoMatrix[attrNum])[j].infoValue = gain;
         (*infoMatrix[attrNum])[j].splitVal = j;
         (*infoMatrix[attrNum])[j].numInstances = numInst;
      }
   }
}


/*****************************************************************************
  Description : Builds matrix using AttrEqCategorizer.
  Comments    :
*****************************************************************************/
void EntropyGainCache::use_attr_eq_cat(int attrNum)
{
   const NominalAttrInfo& nai = get_bag().attr_info(attrNum).cast_to_nominal();
    Real parentWeightedEntropy =  // This is really log-k
       weighted_entropy(get_log_options(),get_bag(),get_bag());
   Array<int> parentLabelCount(get_bag().counters().label_counts(),
				ctorDummy);
   const Array2<int>* valueCounts(get_bag().counters().
				  value_counts()[attrNum]);
   ASSERT(valueCounts != NULL);
   Real parentEntropy = entropy(get_log_options(), get_bag());
   for (int i = UNKNOWN_NOMINAL_VAL; i < nai.num_values(); i++) {
      // Profiling showed setting the string takes 30% of the time for this
      // routine, so we only set it if we will display it!
      MString outerAttrValStr;
      IFLOG(2, outerAttrValStr = nai.get_value(i)+ " (" + MString(i,0) + ")");
      LOG(2, "Out value " << outerAttrValStr << endl);
      Real bestGain = -REAL_MAX;
      int bestAttrVal = UNKNOWN_NOMINAL_VAL - 1;
      int bestNumInst = -1;

      Real miVal = -REAL_MAX;      

      for (int j = UNKNOWN_NOMINAL_VAL + 1; j < nai.num_values(); j++) {

	 if (options.get_mutual_info_rate() > 0)  {
	    Real condEntropy = 
	       two_way_cond_entropy(get_log_options(), get_bag(), attrNum, j);
	    miVal = parentEntropy - condEntropy; // mutual info
	 }

	 Array<int> labelCount(parentLabelCount, ctorDummy);
	 int childNumInst = 0;
	 for (int label = labelCount.low(); label <= labelCount.high();
					    label++) {
	    int numUnknownsVal = (*valueCounts)(label, UNKNOWN_NOMINAL_VAL);
	    if (i != j) // exclude value j
	       labelCount[label] -= (*valueCounts)(label,j) + numUnknownsVal;
	    else // splitting on our value
	       labelCount[label] = (*valueCounts)(label,j);
	    ASSERT(labelCount[label] >= 0);
	    childNumInst += labelCount[label];
	 }
         // i != j can be removed and this will allow splits on equality.
	 DBG(ASSERT(get_bag().num_instances() == num_instances()));
	 if (childNumInst != 0 && childNumInst != num_instances() &&
	    i != j) {
  	    Real ent = weighted_entropy(get_log_options(),
			    labelCount, parentLabelCount, childNumInst,
			    num_instances(), options.get_pessimisticZ());
	    Real gain = (parentWeightedEntropy - ent) * 
	                pow(Real(childNumInst)/num_instances(), 
			    options.get_penalty_power());
	    if (options.get_mutual_info_rate() > 0) {
	       ASSERT(miVal != -REAL_MAX);
	       gain = gain * (1 - options.get_mutual_info_rate()) +
		  miVal * options.get_mutual_info_rate();
	    }
	    if (gain < 0 && gain >= -REAL_EPSILON)
	       gain = 0;
	    ASSERT(gain >= 0);
	    LOG(3, "Inner Value " << nai.get_value(j) << " (" <<
		j << ")" << " has gain " << gain
		<< " with " << childNumInst << " instances" << endl);
	    if (gain > bestGain) {
               //  || (fabs(gain - bestGain)<REAL_EPSILON &&
	       // childNumInst > bestNumInst)){
	       bestGain = gain;
	       bestAttrVal = j;
	       bestNumInst = childNumInst;
	    }
	 }
      }
      (*infoMatrix[attrNum])[i].infoValue = bestGain;
      (*infoMatrix[attrNum])[i].splitVal = bestAttrVal;
      (*infoMatrix[attrNum])[i].numInstances = bestNumInst;
      if (bestGain == -REAL_MAX)
	 LOG(2, "No reasonable split" << endl);
      else
	 LOG(2, "best gain for value " << i << " is " << bestGain
	     << " with split on " << nai.get_value(bestAttrVal) << " ("
	     << bestAttrVal << ")" << endl);
   }
}


/*****************************************************************************
  Description : Returns the matrix cell specified by the given attribute
                  number and value.
  Comments    :
*****************************************************************************/

// verify_gain checks the consistency of the gain infoValue.
// Does everything the "hard" way by using the split-off bag,
//   and splitting a bag for mutual-info.  SLOWWWWW.
void verify_gain(const LogOptions& logOptions,
		 const CtrInstanceBag& parentBag, 
		 const CtrInstanceBag& childBag, Real infoValue,
		 Real pessimisticZ, Real penaltyPower,
		 int attrNum, NominalVal splitVal, Real miRate)
{
   
   Real parentWeightedEntropy =  // log-k
      weighted_entropy(logOptions, parentBag, parentBag);
   Real childWeightedEntropy = 
      weighted_entropy(logOptions, childBag, parentBag, pessimisticZ);

   Real gain = (parentWeightedEntropy - childWeightedEntropy)*
               pow(Real(childBag.num_instances())/parentBag.num_instances(), 
			penaltyPower);
   if (gain < 0 && gain >= -REAL_EPSILON)
      gain = 0;
   ASSERT(gain >= 0);

   if (miRate > 0) {
      Real parentEntropy = entropy(logOptions, parentBag);
      AttrEqCategorizer ac(parentBag.get_schema(), attrNum, splitVal,
			   "verify gain categorizer", TRUE);
      CtrBagPtrArray& bagArray = *(parentBag.ctr_split(ac));
      ASSERT(bagArray.size() == 3);
      // This categorizer never returns unknowns separately.
      ASSERT(bagArray.index(0)->num_instances() == 0);
      Real condEntropy = 0;
      for (int i = 1; i < 3; i++)
	 if (bagArray.index(i)->num_instances() != 0)
	    condEntropy += entropy(logOptions, *bagArray.index(i)) *
	       Real(bagArray.index(i)->num_instances()) / 
	       parentBag.num_instances();

      delete &bagArray;
      Real miVal = parentEntropy - condEntropy;
      gain = gain * (1 - miRate) + miVal * miRate;
   }

   if (fabs(gain - infoValue) > REAL_EPSILON)
      err << "verify_gain: mismatch in gain: " << gain << " versus info-value "
	  << infoValue << " miRate=" << miRate << endl
	  << "Parent bag is " << parentWeightedEntropy
	  << " child bag is " << childWeightedEntropy << fatal_error;
}


SplitInfoCache& EntropyGainCache::child_cache(int attrNum, NominalVal val)
{
   LOG(2, "Child bag for attribute " << attrNum << " value " << val << endl);
   
   // should be initialized by using build_info_value
   const NominalAttrInfo& nai = get_bag().get_schema().attr_info(attrNum).
      cast_to_nominal();
   if (val < UNKNOWN_NOMINAL_VAL || val > nai.num_values()) 
      err << "SplitInfoCache::child_cache: illegal attribute value : "
	 << val << fatal_error;
   if (attrNum < 0 || attrNum >= get_bag().get_schema().num_attr())
      err << "SplitInfoCache::child_cache: illegal attribute num : "
	 << attrNum << fatal_error;

   CacheInfo& ci = (*infoMatrix[attrNum])[val];

   if (ci.infoValue == UNSET_INFO_VAL)
      err << "SplitInfoCache::child_cache: child_bag on non-built attribute "
	  << attrNum << fatal_error;

   if (ci.sic == NULL) {
      // EntropyGainCache gets ownership so a temporary variable is needed
      CtrInstanceBag* tp= split_child(attrNum, val, ci.splitVal);
      LOG(4, "Split bag is " << *tp << endl);

      DBG(
	 ASSERT(tp->num_instances() == ci.numInstances);
	 Real infoVal = ci.infoValue;
         if (!options.get_use_attr_eq_cat_bool())
	    infoVal *=log_bin(get_bag().get_schema().num_attr_values(attrNum));
	 verify_gain(get_log_options(), get_bag(), *tp, infoVal, 
		     options.get_pessimisticZ(), options.get_penalty_power(),
		     attrNum, ci.splitVal, options.get_mutual_info_rate());
      );
      EntropyGainCache *egc = new EntropyGainCache(tp);
      egc->set_log_options(get_log_options());
      egc->options = options;
      ci.sic = egc;
   }

   SplitInfoCache& sic = *ci.sic;
   LOG(4, "Bag is:" << endl << ci.sic->get_bag());
   return sic;
}



/***************************************************************************
  Description : Display basic cache information
  Comments    :
***************************************************************************/

void EntropyGainCache::display(MLCOStream& stream) const
{
   stream << "Min split = " << options.get_min_split() << endl;
   stream << "Use not-equal attr = " <<
      bool_to_string(options.get_use_attr_eq_cat_bool()) << endl;
}




DEF_DISPLAY(EntropyGainCache);



