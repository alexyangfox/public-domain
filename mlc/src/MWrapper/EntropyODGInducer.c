// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : EntropyODGInducer is a derived class from ODGInducer.
                 For best-split categorizer, it calculates the conditional
		   entropy of all the nodes in the same level and  finds the
		   best categorizer.
  Assumptions  : 
  Comments     : best_split calculates the conditional entropy of all the
                   nodes in the same level. For each attribute, we calculate
		   the conditional entropy if splitting on them. Then we pick
		   up the attribute with the minimum conditional entropy. 
		Under two conditions should the best_split returns NULL to stop
		  splitting:
		  1. If the average conflict ratio of the current level is
		     less than get_grow_conflict_ratio().
		  2. If no bag can be splitted such that each of its children
		     has more than get_min_split() instances.
                 
  Complexity   : Complexity: O(num-instances*ra + bags*ac*lc*na)
                   where ra is the number of real attributes, bags is the
		   number of bags at the level, ac is the maximum number of
		   attribute values per attribute, lc is the number of label
		   values, and na is the number of nominal attributes.

  Enhancements : 
  History      : Chia-Hsin Li                                        10/06/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <AugCategory.h>
#include <entropy.h>
#include <DynamicArray.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <ConstCat.h>
#include <AttrCat.h>
#include <ThresholdCat.h>
#include <BagCounters.h>
#include <ODGInducer.h>
#include <EntropyODGInducer.h>
#include <ListODGInducer.h>
#include <isocat.h>
#include <AccEstDispatch.h>

RCSID("MLC++, $RCSfile: EntropyODGInducer.c,v $ $Revision: 1.12 $")

/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
EntropyODGInducer::~EntropyODGInducer()
{
   DBG(OK());
   delete orderVector;
}

/***************************************************************************
  Description : Returns TRUE if at least one of the bags in currentLevel will
                   be split into two or more bags given a nominal attribute
  Comments    : If all bags are pure, returns FALSE. 
***************************************************************************/
static Bool
is_meaningful_split(DynamicArray<const CtrInstanceBag*> &currentLevel,
		    int attrNum)
{
   SchemaRC schema = currentLevel[0]->get_schema();
   ASSERT(schema.attr_info(attrNum).can_cast_to_nominal());
   for (int i = 0; i < currentLevel.size(); i++) {
      ASSERT(currentLevel[i]);
      if (currentLevel[i]->counters().attr_num_vals(attrNum) >= 2)
	 return TRUE;
   }
  return FALSE;
}

/***************************************************************************
  Description : Returns the total number of instances in a set of bags.
  Comments    :
***************************************************************************/
static int num_instances(DynamicArray<const CtrInstanceBag*> &bagSet)
{
   int instanceNumber = 0;
   for (int bagCount = 0; bagCount < bagSet.size(); bagCount++)
      instanceNumber += bagSet[bagCount]->num_instances();
   return instanceNumber;
}

/***************************************************************************
  Description : Returns the number of conflict instances over the number of
                   all instances. 
  Comments    :
***************************************************************************/
Real EntropyODGInducer::average_conflict_ratio(
   DynamicArray<const CtrInstanceBag*> &bagSet)
{
   int conflictInstancesNum = 0;
   
   AugCategory aca(1, "Temporary");
   ConstCategorizer tmpCat("Temporary", aca);
   for (int bagCount = 0; bagCount < bagSet.size(); bagCount++) {
      tmpCat.build_distr(*bagSet[bagCount]);
      conflictInstancesNum += num_conflicting_instances(
	 get_log_options(), tmpCat);
   }
   return (Real(conflictInstancesNum) / TS->num_instances());
}

/***************************************************************************
  Description : 
  Comments    :
***************************************************************************/
void append_split_info(DynamicArray<SplitInfo>& splitInfoArray,
		       int bestAttrNum, SplitInfo::SplitType splitType, 
		       Real minCondEntropy, Real bestThreshold)
{
   SplitInfo& nSplitInfo = splitInfoArray[splitInfoArray.size()];
   nSplitInfo.attrNum = bestAttrNum;
   nSplitInfo.splitType = splitType;
   nSplitInfo.condEntropy = minCondEntropy;
   nSplitInfo.typeInfo.threshold = bestThreshold;
}

/***************************************************************************
  Description : Returns the best split for a set of bags.
  Comments    : Nominal attributes that have been used should be skipped for
                  acceleration. For continuous attributes, this doesn't apply
		  because we may have different threshold on the same
		  attribute in different levels.
***************************************************************************/
Categorizer*
EntropyODGInducer::best_split(Array<UsedAttributes>& usedAttributes,
			      DynamicArray<SplitInfo>& splitInfoArray,
			      DynamicArray<const CtrInstanceBag*>& currentLevel, 
			      MStringArray*& catNames,
			      Real& minCondEntropy,
			      Real previousEntropy)
{
   const SchemaRC& schema = TS->get_schema();
   int numTotalInstances = num_instances(currentLevel);

   catNames = NULL; // so caller won't try to delete anything if we
                    // don't get to the allocation part.
   minCondEntropy = REAL_MAX;
   Real maxMutualInfo = -1;

   if (currentLevel.size() == 0)
      return NULL;

   int bestAttrNum = -1;
   Real bestThreshold = -1;

   Real averageConflictRatio = average_conflict_ratio(currentLevel);
 
   LOG(2, "Conflict ratio: " << averageConflictRatio << endl);
   if (averageConflictRatio <= get_grow_conflict_ratio()) {
      LOG(2,"Nothing good to split on: Average conflict ratio lower than "
	      << "grow conflict ratio." << endl);
      return NULL;
   }
	 
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      LOG(3,"Testing attribute " << attrNum
	  << " (" << schema.attr_name(attrNum) << ")." << endl);

      Real condEntropy, mutualInfo, threshold;
      // If the attribute is nominal and has more than one value for
      //   at least one of the bags, then use cond_entropy to determine
      //   the conditional entropy of splitting on it.
      if (schema.attr_info(attrNum).can_cast_to_nominal()) {
	 if (usedAttributes[attrNum].nominalUsed) 
	    LOG(3,"Attribute " << attrNum << " skipped." << endl);
	 else {
	    // check that at least one of the bags in currentLevel will be
	    // split into two or more bags.
	    if (is_meaningful_split(currentLevel, attrNum)) {
	       Array2<int>* vc = build_vc(get_log_options(),
						currentLevel, attrNum);
	       Array<int>* ac = build_ac(get_log_options(),
					       currentLevel, attrNum);
	       condEntropy = cond_entropy(get_log_options(),
					  *vc, *ac, numTotalInstances);
	       //@@ For test only
	       LOG(3,"Attr:" << schema.attr_name(attrNum)
		     << " condEntropy:" << condEntropy);
	       //@@ Rename condEntropy to mutual_information
	       mutualInfo = (previousEntropy - condEntropy) /
		  log_bin(schema.num_attr_values(attrNum));
	       LOG(3, " mutualInfo:" << mutualInfo << " branches #:"
		     << schema.num_attr_values(attrNum) << endl);
	       delete vc;
	       delete ac;
	       ASSERT(condEntropy >= 0);
	       LOG(3, "condEntropy is " << condEntropy << endl);
	       // To promote stability, only change node if difference is
	       //   over REAL_EPSILON.
	       if (mutualInfo > maxMutualInfo - REAL_EPSILON) {
		  minCondEntropy = condEntropy;
		  maxMutualInfo = mutualInfo;
		  bestAttrNum = attrNum;
	       }
	    } else
	       LOG(3, "It causes no split. Not meaningful split. " << endl);
	 }
      }
      // Otherwise the attribute is real and real_cond_entropy is used to
      //   determine the threshold which gives the conditional entropy
      //   when split on.
      else if (schema.attr_info(attrNum).can_cast_to_real()) {
	 Real condEntropy;
	 if (real_cond_entropy(get_log_options(),
			       currentLevel, attrNum, threshold,
			       condEntropy, get_min_split(),
			       numTotalInstances)) {
            if (condEntropy < 0 && condEntropy > -REAL_EPSILON*10)
	       condEntropy = 0;
	    if (condEntropy < 0)
	       err << "EntropyODGInducer::best_split: bad conditional "
		  "entropy " << condEntropy <<endl;
	    mutualInfo = (previousEntropy - condEntropy);

	    LOG(3, "Threshold is " << threshold << ", condEntropy is "
		<< condEntropy << ", mutualInfo: " << mutualInfo << endl);
	    if (used_threshold(attrNum, threshold))
	       LOG(3, "The threshold " << threshold << " of "
		       << schema.attr_info(attrNum).name()
		       << " has been used before. It suggests "
		       << "no benefit." << endl);
	    else 
	       // Remember the best
	       if (mutualInfo > maxMutualInfo - REAL_EPSILON) {
		  minCondEntropy = condEntropy;
		  maxMutualInfo = mutualInfo;
		  bestAttrNum = attrNum;
		  bestThreshold = threshold;
	       }
	 }
	 else {
	    LOG(3, "It causes no split. " << endl);
	    LOG(3, "Cannot find proper threshold to satify the MIN_SPLIT " << 
		   "criteria " << endl);
	 }
      }
      else
	 err << "Unrespected type of attribute :" 
	     << schema.attr_info(attrNum)
	     << fatal_error;
   }
   
   if (bestAttrNum == -1) {
      LOG(2, "Nothing good to split on: No good categorizer found." << endl);
      return NULL; 
   } else {
      LOG(2, "Best attribute is " << bestAttrNum << " (" <<
	  schema.attr_name(bestAttrNum) << ")." << endl);
      MString attrName = schema.attr_info(bestAttrNum).name();
      if (schema.attr_info(bestAttrNum).can_cast_to_nominal()){
	 const NominalAttrInfo& nai =
	    schema.attr_info(bestAttrNum).cast_to_nominal();
	 int size = nai.num_values() + 1; // +1 for unknown
	 catNames = new MStringArray(UNKNOWN_CATEGORY_VAL, size);
	 (*catNames)[UNKNOWN_CATEGORY_VAL] = "?" ;
	 Category cat = FIRST_CATEGORY_VAL;
	 for (int i = 1; i < size; i++, cat++)
	    (*catNames)[cat] = nai.get_value(i + UNKNOWN_NOMINAL_VAL);
	 // Set the split information.
	 usedAttributes[bestAttrNum].nominalUsed = TRUE;
	 append_split_info(splitInfoArray, bestAttrNum,
			   SplitInfo::nominalSplit, minCondEntropy, 0);
	 return new AttrCategorizer(schema, bestAttrNum, attrName);
      }
      else {
	 LOG(2, "Threshold = " <<  bestThreshold << ". Conditional entropy = "
	     << minCondEntropy
	     << endl);
	 // If the best attribute is real, then catNames is an array of
	 //   3 MStrings: unknown, left node (less than threshold), or
	 //   right node (greater than or equal to threshold).
	 // Append bestThreshold to usedAttributes.thresholdUsed.
	 catNames = new MStringArray(UNKNOWN_CATEGORY_VAL, 3);
	 (*catNames)[UNKNOWN_CATEGORY_VAL] = "?";
	 (*catNames)[UNKNOWN_CATEGORY_VAL+1] =
	    "<= " + MString(bestThreshold,0);
	 (*catNames)[UNKNOWN_CATEGORY_VAL+2] =
	    "> " + MString(bestThreshold,0);
	 // Set the split information.
	 int arraySize = usedAttributes[bestAttrNum].thresholdUsed->size();
	 (*(usedAttributes[bestAttrNum].thresholdUsed))[arraySize]
	    = bestThreshold;
	 append_split_info(splitInfoArray, bestAttrNum,
			   SplitInfo::realThresholdSplit, minCondEntropy,
			   bestThreshold);
	 return new ThresholdCategorizer(schema, bestAttrNum,
					 bestThreshold, attrName);
      }
   }
}

const int DEFAULT_ODG_CV_FOLDS = 5;
const int DEFAULT_ODG_CV_TIMES = 2;

/***************************************************************************
  Description : Through cross validation, find the best depth which makes the
                   graph have the best estimated accuracy.
  Comments    : 
***************************************************************************/
int EntropyODGInducer::best_depth()
{
   ListODGInducer listInducer("List ODG Inducer", this);

   // Give the data set to List Inducer.
   InstanceBag* origBag = release_data();
   InstanceBag* bagPtr = origBag;
   listInducer.assign_data(bagPtr);

   DynamicArray<SplitInfo> splitInf(*splitInfoArray, ctorDummy);
   listInducer.set_options(get_options());
   listInducer.set_log_level(max(get_log_level() - 2, 0));

   if (get_cv_merge())
      listInducer.set_post_proc(merge);
   else {
      listInducer.set_post_proc(set_unknown);
      listInducer.set_split_pure(FALSE);
   }
   Real maxAcc = -1;
   int bestDepth = -1;
   
   AccEstDispatch accEst;
   accEst.set_cv_folds(DEFAULT_ODG_CV_FOLDS);
   accEst.set_cv_times(DEFAULT_ODG_CV_TIMES);
   accEst.set_user_options("ODG_");
   accEst.set_log_level(max(get_log_level() - 2, 0));

   for (int depth = splitInf.size(); depth >= 0; depth--) {
      splitInf.truncate(depth);
      listInducer.set_split_info_array(splitInf);
      Real acc = accEst.estimate_accuracy(listInducer,
	       listInducer.TS_with_counters().cast_to_ctr_instance_list());
      LOG(2, "Estimated accuracy of depth " << depth << " is : " << acc
	  << "." << endl);
      if (acc >= maxAcc - REAL_EPSILON) {
	 maxAcc = acc;
	 bestDepth = depth;
      }
   }
   LOG(1, "Best estimated accuracy: " << maxAcc << " at depth "
       << bestDepth << "." << endl);
   // Restore the data to EntropyODGInducer
   InstanceBag* newBag = listInducer.release_data();
   ASSERT(origBag == newBag);
   assign_data(newBag);
   
   return bestDepth;
}

/***************************************************************************
  Description : Induce a decision graph.
  Comments    : Must be called after read_data().
                Erases any previously created DT.
***************************************************************************/

// private help function.  extra marks used attrs
// Pretty dumb to use this if you have continuous variables, since there may
// be many cutpoints used at different levels.  We only show the first use.
BoolArray *EntropyODGInducer::setup_order_vec()
{
   // set up the order vector by going through the split info array
   delete orderVector;
   orderVector = new DynamicArray<int>(0);
   BoolArray *extras = new BoolArray(0, TS->num_attr(), FALSE);
   for(int elt=0; elt<splitInfoArray->size(); elt++) {
      int which = splitInfoArray->index(elt).attrNum;
      if ((*extras)[which] == FALSE) {
	 (*extras)[which] = TRUE;
	 orderVector->index(orderVector->size()) = which;
      }
   }
   return extras;
}


void EntropyODGInducer::train()
{
   const SchemaRC& schema = TS->get_schema();

   reset_used_attributes();
   reset_split_info_array();

   if (!get_cv_prune()) {
      ODGInducer::train();
      ASSERT(splitInfoArray);
      delete setup_order_vec();
      return;
   }

   // Used for accounting information.
   static int totalNodesNum = 0;
   static int totalLeavesNum = 0;
   static int callCount = 0;
   
   has_data();
   DBG(OK());

   delete decisionGraphCat; // remove any existing graph categorizer.
   decisionGraphCat = NULL;
   
   // firstRG is only used for first time inducing to get the splitInfoArray
   RootedCatGraph *firstRG = new RootedCatGraph;
   LOG(4, "Training with bag\n"  << instance_bag() << endl);

   // Induce the graph without merging. Then splitInfoArray is ready.
   PostProcType oldPostProc = get_post_proc();
   set_post_proc(set_unknown);
   Bool saveSplitPure = get_split_pure();
   set_split_pure(get_grow_split_pure());
   induce_oblivious_decision_graph(firstRG->get_graph());
   set_split_pure(saveSplitPure);
   delete firstRG;

   // Find the best depth of the tree.
   int bestDepth = best_depth();
   splitInfoArray->truncate(bestDepth);

   // Construct listInducer.
   // Transfer the ownership of TS from this inducer to listInducer.
   InstanceBag* origBag = release_data();
   InstanceBag* bagPtr = origBag;
   set_post_proc(oldPostProc);
   ListODGInducer listInducer("List ODG Inducer", this);
   listInducer.assign_data(bagPtr);
   ASSERT(bagPtr == NULL);
   listInducer.set_options(get_options());
   listInducer.set_split_info_array(*splitInfoArray);

   // Use list inducer to induce the decision graph for EntropyODGInducer.
   RootedCatGraph *RG = (cgraph == NULL) ? new RootedCatGraph
      : new RootedCatGraph(*cgraph);
   RG->set_root(listInducer.induce_oblivious_decision_graph(RG->get_graph()));
   InstanceBag* newBag = listInducer.release_data();

   // Make sure that the data returned at last is the original one.
   ASSERT(origBag == newBag);
   assign_data(newBag);
   decisionGraphCat =
      new RDGCategorizer(RG, description(),TS->num_categories());

   BoolArray* extras = setup_order_vec();

   // add in extras
   //for(int idx=0; idx<extras->size(); idx++)
   //   if(!(*extras)[idx])
   //	 orderVector->index(orderVector->size()) = idx;
   //ASSERT(orderVector->size() == TS->num_attr());
   delete extras;
   
   // Accounting information.
   callCount++;
   totalNodesNum += num_nodes();
   totalLeavesNum += num_leaves();
   GLOBLOG(1, "Graph has " << num_nodes() << " nodes, and "
          << num_leaves() << " leaves." << endl);
   GLOBLOG(1, "Average nodes for all runs:" << Real(totalNodesNum)/callCount
	   << " Average leaves: " << Real(totalLeavesNum)/callCount
	   << endl);

}


