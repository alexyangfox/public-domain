// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : ListODGInducer is a derived class from ODGInducer.
                 Takes an array of int as argument and returns an item one by
  		   one from that array each time best_split() is called.
		 If the last element of that split infomation array is -1,
		   that means we have to calculate the best split categorizer
		   by calling the caller's best_split function.
  Assumptions  : Caller must be alive while the object is still alive.
                 In addition, the caller pointer must be passed while
		   constructing this object.
		 If the TS in ListODGInducer is not the same object as what
		   the caller passed, this may cause trouble.
  Comments     :                  
  Complexity   : 
  Enhancements : 
  History      : Chia-Hsin Li                                        10/06/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <AugCategory.h>
#include <ConstCat.h>
#include <AttrCat.h>
#include <ThresholdCat.h>
#include <ListODGInducer.h>
#include <EntropyODGInducer.h>

RCSID("MLC++, $RCSfile: ListODGInducer.c,v $ $Revision: 1.4 $")


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
ListODGInducer::~ListODGInducer()
{
   DBG(OK());
}

/***************************************************************************
  Description : Returns the best split for a set of bags.
  Comments    : Nominal attributes that have been used should be skipped for
                  acceleration. For continuous attributes, this doesn't apply
		  because we may have different threshold on the same
		  attribute in different levels.
		While calling caller->best_split, the caller may need the
		  training set. So, we need to assign the TS back to the
		  caller and get it back after calling caller's best_split.
		The caller must release the same TS object as the one it
		  passed.   
***************************************************************************/
Categorizer* ListODGInducer::best_split(Array<UsedAttributes>& usedAttributes,
				DynamicArray<SplitInfo>& splitInfoArray,
				DynamicArray<const CtrInstanceBag*>& currentLevel,
				MStringArray*& catNames,
				Real& minCondEntropy,
				Real previousEntropy)
{
   if (currentSplitInfo > splitInfoArray.high()) {
      LOG(2, "End of split info. " << endl);
      return NULL;
   }
   ASSERT(TS);
   const SchemaRC& schema = TS->get_schema();
   const SplitInfo& si = splitInfoArray[currentSplitInfo];
   currentSplitInfo++;
   if (si.attrNum == -1) { // Recalculate the attribute
      LOG(2, "Calling caller's best_split() at depth " << currentSplitInfo - 1
	  << endl);
      InstanceBag* origBag = release_data();
      InstanceBag* tmpBag = origBag;
      if (caller == NULL)
	 err << "ListODGInducer:: NULL caller" << fatal_error;
      caller->assign_data(tmpBag);
      DynamicArray<SplitInfo> splitInf(splitInfoArray, ctorDummy);
      int saveLogLevel = caller->get_log_level();
      caller->set_log_level(max(get_log_level()-1, 0));
      Categorizer* bestCat = caller->best_split(usedAttributes,splitInf,
		     currentLevel, catNames, minCondEntropy, previousEntropy);
      caller->set_log_level(saveLogLevel);
      tmpBag = caller->release_data();
      ASSERT(tmpBag == origBag);
      assign_data(tmpBag);
      return bestCat;
   }
   catNames = NULL; // so caller won't try to delete anything if we
                    // don't get to the allocation part.
   minCondEntropy = si.condEntropy;
   LOG(2, "Best attribute is " << si.attrNum << " (" <<
       schema.attr_name(si.attrNum) << ")." << endl);
   MString attrName = schema.attr_info(si.attrNum).name();
   if (schema.attr_info(si.attrNum).can_cast_to_nominal()){
      const NominalAttrInfo& nai =
	    schema.attr_info(si.attrNum).cast_to_nominal();
      int size = nai.num_values() + 1; // +1 for unknown
      catNames = new MStringArray(UNKNOWN_CATEGORY_VAL, size);
      (*catNames)[UNKNOWN_CATEGORY_VAL] = "?" ;
      Category cat = FIRST_CATEGORY_VAL;
      for (int i = 1; i < size; i++, cat++)
	 (*catNames)[cat] = nai.get_value(i + UNKNOWN_NOMINAL_VAL);
      return new AttrCategorizer(schema, si.attrNum, attrName);
   }
   else {
      LOG(2, "Threshold = " <<  si.typeInfo.threshold
	  << ". Conditional entropy = " << si.condEntropy << endl);
      catNames = new MStringArray(UNKNOWN_CATEGORY_VAL, 3);
      (*catNames)[UNKNOWN_CATEGORY_VAL] = "?";
      (*catNames)[UNKNOWN_CATEGORY_VAL+1] =
	 "<= " + MString(si.typeInfo.threshold,0);
      (*catNames)[UNKNOWN_CATEGORY_VAL+2] =
	 "> " + MString(si.typeInfo.threshold,0);
      return new ThresholdCategorizer(schema, si.attrNum,
				      si.typeInfo.threshold, attrName);
   }
}

/***************************************************************************
  Description : Induce a decision graph.
  Comments    : Calls ODGInducer::train(). The interesting point is that the
                  ODGIncuer::train() will call LISTODGInducer::best_split
		  back. 
***************************************************************************/
void ListODGInducer::train()
{
   reset_used_attributes();
   currentSplitInfo = 0;
   ASSERT(TS);
   const SchemaRC& schema = TS->get_schema();

   const Array<int>& compute_order(const LogOptions& logOptions,
				   const InstanceBag& bag,
				   const MString preFix = "");

   // If SplitInfoArray is not set, we must compute the order (or
   //    take whatever set_order() was done by OFSS).
   if (splitInfoArray == NULL) {
      ao.compute_order(get_log_options(), *TS);

      const Array<int>& array = ao.get_order();
      Array<SplitInfo> spArray(array.size());

      // Copy our order to split-info.  Make sure they're all nominal.
      for (int i = 0; i < array.size(); i++) {
         if (!schema.attr_info(array[i]).can_cast_to_nominal())
	    err << "ListODGInducer::train: cannot cast attribute to nominal"
                   " and AttrOrder can only be set for nominals."
	           " Please discretize the data first. " << fatal_error;
	 spArray[i].attrNum = array[i];
      }
      set_split_info_array(spArray);
   }    

   ODGInducer::train();
}

/***************************************************************************
  Description : Set the split information array.
  Comments    :
***************************************************************************/
void ListODGInducer::set_split_info_array(const Array<SplitInfo>& spArray)
{
   if (splitInfoArray)
      delete splitInfoArray;
   ao.init(); // make sure we don't have attribute order set now.
   
   splitInfoArray = new DynamicArray<SplitInfo>(spArray.size());
   // Copy elements.
   for (int i = 0; i < splitInfoArray->size(); i++)
      (*splitInfoArray)[i] = spArray[i];
}

/***************************************************************************
  Description : Induce the decision graph. 
  Comments    : Calls ODGIncuer::inducer_oblivious_decision_graph.
***************************************************************************/
NodePtr ListODGInducer::induce_oblivious_decision_graph(CGraph& aCgraph)
{
   currentSplitInfo = 0;
   if (splitInfoArray == NULL)
      err << "Split Information array has not been set yet" << fatal_error;
  return ODGInducer::induce_oblivious_decision_graph(aCgraph);
}


/*****************************************************************************
  Description : Set user options.
  Comments    :
*****************************************************************************/
void ListODGInducer::set_user_options(const MString& prefix)
{
   get_attr_order_info().set_user_options(prefix);
   ODGInducer::set_user_options(prefix);
}
