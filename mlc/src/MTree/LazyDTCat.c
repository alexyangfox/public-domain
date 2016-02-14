// Mlc++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.


/***************************************************************************
  Description  : LazyDTCategorizer categorizes an instance by building an
                   optimal path of a decision tree which has the maximal
		   information gain with regard to the test instance and
		   data set in ctrBag.
  Comments     :
  Enhancements :
  History      : Yeogirl Yun                                       1/10/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <LazyDTCat.h>
#include <EntropyGainCache.h>
#include <entropy.h>
#include <ThresholdCat.h>
#include <AttrCat.h>
#include <math.h>
#include <Array.h>

const Real defaultMultiSplitWeight = 0.1;
const Real defaultDelRate = 0.0;
const Bool defaultMultiSplitOn = TRUE;

const int MAX_PATH = 3;
/*****************************************************************************
  Description : Constructor. Assigns a (counter) bag to ctrBag.
  Comments    :
*****************************************************************************/
LazyDTCategorizer::LazyDTCategorizer(const CtrInstanceBag& bag,
				     const MString& dscr)
   : Categorizer(bag.num_categories(), dscr), ctrBag(bag, ctorDummy)
{
   CtrInstanceBag* tmpBag = new CtrInstanceBag(ctrBag, ctorDummy);
   multiSplitRate = defaultMultiSplitWeight;
   delRate = defaultDelRate;
   multiSplitOn = defaultMultiSplitOn;
   cache = new EntropyGainCache(tmpBag);
}   



/***************************************************************************
    Description : Copy constructor with extra argument.
    Comments    : Cache structure is not actually copied but created.
                    This is because EntropyGainCache didn't implement
		    copy operation yet.
***************************************************************************/
LazyDTCategorizer::LazyDTCategorizer(const LazyDTCategorizer& source,
				     const CtorDummy /*dummyArg*/)
   : Categorizer(source, ctorDummy),
     ctrBag(source.ctrBag, ctorDummy)

{
   cache = new EntropyGainCache(*source.cache, ctorDummy);
   set_pessimisticZ(source.get_pessimisticZ());
   set_penalty_power(source.get_penalty_power());
   set_multi_split_rate(source.get_multi_split_rate());
   set_del_rate(source.get_del_rate());
   set_multi_split_bool(source.get_multi_split_bool());
} 



/***************************************************************************
  Description : Prints a readable representation of the categorizer to the
                  given stream. Just prints out the instStore.
  Comments    :
***************************************************************************/
void LazyDTCategorizer::display_struct(MLCOStream& stream,
				      const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::ASCIIDisplay) {
      stream << "LazyDT Categorizer " << description() << endl
	     << "with the following labelled instances in the instStore" <<
	 endl;
      ctrBag.display(stream);
      stream << *cache << endl;
   }
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of this LazyDTCategorizer.
  Comments    :
***************************************************************************/
Categorizer* LazyDTCategorizer::copy() const
{
   return new LazyDTCategorizer(*this, ctorDummy);
}


/***************************************************************************
  Description : Returns TRUE if they have the same instStores.
                NOT_IMPLEMENTED until ctrBag supports operator==().
  Comments    :
***************************************************************************/
Bool LazyDTCategorizer::operator==(const Categorizer& /* cat */) const
{
   err << "LazyDTCategorizer::operator==: NOT_IMPLEMENTED" << fatal_error;
   return TRUE;
}



/*****************************************************************************
  Description : Returns an array of label counts for each label and depth.
                It calls recursively itself and adds up the arrays returned
		   from the leaf and finally returns a final array which
		   contains all the counts of instances for each label for
		   all the possible splits made during the categorization.
  Comments    :
*****************************************************************************/

static void disp_info_vals(const LogOptions& logOptions,
			   const SchemaRC& schema, 
			   const Array<InfoValAndSize>& iv)
{
   for (int i = 0; i < iv.size(); i++) 
      FLOG(logOptions.get_log_level(), setw(3) << i << " info: " << setw(5)
	   << Mround(iv.index(i).infoVal, 4)
	   << " num-instances: " << iv.index(i).numInstances << ' ' 
	   << schema.attr_name(FIRST_CATEGORY_VAL + i) << endl);
}

// Adjust the predicted number of instances returned according
//   to our node proportion.
static Real adjust_num_inst(const LogOptions& logOptions,
			    const Array<int>& labels, 
			    const CtrInstanceBag& bag)
{
   
   NominalVal category;
   int numPredictedInst = labels.max(category);
   category += labels.low(); // because max returns 0 based index.
   int numClass = bag.counters().label_counts()[category];
   Real weightedNumInst = Real(numPredictedInst)/numClass;
   FLOG(2, "Num in prediction " << numPredictedInst << " proportion of "
         << numClass << "=" << weightedNumInst << endl);
   return weightedNumInst;
}
	       

int LazyDTCategorizer::prob_categorize(SplitInfoCache* sic,
					const InstanceRC& instance,
				        Real multiSplitRate,
					Array<int>& labelCount) const
{
   LOG(2, "Count vector: " << sic->get_bag().counters().label_counts() <<
       endl);
   
   // assign infoValues to infoVals for each attributes.
   Array<InfoValAndSize> infoVals(0, instance.num_attr());
   for (int i = 0; i < ctrBag.get_schema().num_attr(); i++) {
      if (!instance.attr_info(i).can_cast_to_nominal())
	 err << "LazyDTCategorizer::prob_categorize: LazyDT does not "
	    "handle real attributes. Please run discretization before running"
	    "lazyDT." << fatal_error;
      const NominalAttrInfo& nai = instance.attr_info(i).cast_to_nominal();
      if (nai.is_unknown(instance[i])) {
	 infoVals[i].infoVal = -REAL_MAX;
	 infoVals[i].numInstances = -1;
      } else {
	 Real infoValue = sic->info_value(i, nai.get_nominal_val(instance[i]));
	 infoVals[i].infoVal = infoValue;
	 infoVals[i].numInstances = sic->num_instances(i,
					 nai.get_nominal_val(instance[i]));
      }
   }

   IFLOG(2, disp_info_vals(get_log_options(),
			   instance.get_schema(), infoVals));
   labelCount.init_values(0);
   // Try all closely-tied attributes.
   int maxDepth = INT_MAX;
   int bestAttr = -1;
   Real bestWeightedInst = -1;
   Real bestInfoVal  = infoVals.max(bestAttr).infoVal;
   if (bestInfoVal != -REAL_MAX) {
      LOG(2, "Best attribute is " << bestAttr << ' '
	  << instance.get_schema().attr_name(bestAttr) << endl);
      if (get_multi_split_bool()) {
	 ASSERT(multiSplitRate >= 0);
	 for (i = 0; i < ctrBag.get_schema().num_attr(); i++) {
	    if (infoVals[i].infoVal >= bestInfoVal*(1-multiSplitRate)) {
	       const NominalAttrInfo& nai =
		  instance.attr_info(i).cast_to_nominal();
	       LOG(2, "Branching to " << i << " (" <<
		   nai.name() << ") with info value " << infoVals[i].infoVal
		   << endl);
	       SplitInfoCache* tpSic =
		  &sic->child_cache(i,nai.get_nominal_val(instance[i]));

	       Array<int> temp(UNKNOWN_CATEGORY_VAL,
			       instance.num_label_values() + 1, 0);
	       int depth = prob_categorize(tpSic, instance, 
					   multiSplitRate, temp); // can do /2
	       Real weightedNumInst = adjust_num_inst(get_log_options(),
						      temp, tpSic->get_bag());
               if (weightedNumInst > bestWeightedInst) {
               // if (depth < maxDepth) {
   	          maxDepth = depth;
   	          labelCount = temp;
		  bestWeightedInst = weightedNumInst;
	       }
	       LOG(2, "Depth " << depth << " gave vector is " << temp
		   << " and total vec is " << labelCount << endl);
	    }
	 }
      } else { // only split at the best node.
	 ASSERT(bestAttr != -1);
	 const NominalAttrInfo& bnai =
	    instance.attr_info(bestAttr).cast_to_nominal();
	 sic = &sic->child_cache(bestAttr,
		 bnai.get_nominal_val(instance[bestAttr]));
	 Array<int> temp(UNKNOWN_CATEGORY_VAL,
			 instance.num_label_values() + 1, 0);
	 maxDepth = prob_categorize(sic, instance, -1, temp);
	 labelCount += temp;
	 LOG(2, "Depth " << maxDepth << " gave vector is " << temp
	     << " and total vec is " << labelCount << endl);
      }
   }
   else { // now we are at the leaves. return label_counts array.
      labelCount = sic->get_bag().counters().label_counts();
      LOG(2, "Leaf returning with " <<labelCount << endl);
      maxDepth = 0;
   }

   return maxDepth + 1;
}
   
   


/*****************************************************************************
  Description : Returns a category for the given instance by building an
                  optimal path of a decision tree on the fly.
  Comments    :
*****************************************************************************/
AugCategory LazyDTCategorizer::categorize(const
						InstanceRC& instance) const
{
   LOG(2, " ***  Test instance : " << instance);
   if (ctrBag.no_instances())
      err << "LazyDTInducer::categorize: zero instances" << fatal_error;

   Category returnCat = UNKNOWN_CATEGORY_VAL;
   if (instance.get_schema().num_attr() == 0)
      returnCat = ctrBag.majority_category();
   else {
      cache->set_log_level(get_log_level() - 2);
      // const cast away because cache structure must be modified during
      // categorization but categorization process is a conceptual
      // const operation. Note that the caching structure is transparent
      // to the users.
      SplitInfoCache* sic = ((LazyDTCategorizer *)this)->cache;

      Array<int> result(UNKNOWN_CATEGORY_VAL, instance.num_label_values() + 1,
			0);

      // long path is a sign of rarely used small caches. should delete them.
      int depth = prob_categorize(sic, instance, get_multi_split_rate(),
				  result);
      LOG(2, "Final probability vector: " << result << endl);
      if (get_del_rate() > 0 && depth > MAX_PATH ) { 
	 LOG(2, "Deleting small caches from depth " << depth << endl);
	 LOG(2, "Deleting rate : " << get_del_rate()<< endl);
	 cache->delete_small_caches(
	    (int)(cache->num_instances()*get_del_rate()));
      }
   
      result.max(returnCat);
      returnCat += result.low(); // because max returns 0 based index.
   }
   ASSERT(returnCat != UNKNOWN_CATEGORY_VAL);
   
   MString strCategory;
   strCategory = ctrBag.get_schema().category_to_label_string(returnCat);
   AugCategory augCat(returnCat, strCategory);
   LOG(2, "  Returning aug category " << augCat << endl);

   return augCat;
}

