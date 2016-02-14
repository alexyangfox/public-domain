// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is a file to test some part of MLC++.  It is NOT part of the
//   MLC++ library, but is designed to test portions of it.

// @@ I suggest this file should be like t_TDDTInducer.c; #error..... because
// this is an abstract class. I will changed them later.

/***************************************************************************
  Description  : Tests ODGInducer (and TDDTInducer) methods.
  Doesn't test : 
  Enhancements : 
  History      : Chia-Hsin Li                                       9/08/93
                   Initial revision
***************************************************************************/
#include <basics.h>
#include <errorUnless.h>
#include <AugCategory.h>
#include <entropy.h>
#include <DynamicArray.h>
#include <CtrInstList.h>
#include <CatTestResult.h>
#include <AttrCat.h>
#include <ThresholdCat.h>
#include <ODGInducer.h>

RCSID("MLC++, $RCSfile: t_ODGInducer.c,v $ $Revision: 1.3 $")

// If you compile with -DINTERACTIVE, it will show the monks graph in X.


void DummyODTInducer::OK(int /*level*/) const
{
   ODTInducer::OK();
}

DummyODTInducer::~DummyODTInducer()
{
   DBG(OK());
}

static int count_empty_bags(DynamicArray<const CtrInstanceBag*>* currentBag)
{
   int count = 0;
   for (int i = 0; i < currentBag->size(); i++)
      if (!(*currentBag)[i])
	 count++;
   return count;
}

const Array2<int>& DummyODTInducer::build_vc(
   DynamicArray<const CtrInstanceBag*> *currentBag, int attrNum) const 
{
   SchemaRC& schema = TS->get_schema();
   int nonemptyBagNum = currentBag->size() - count_empty_bags(currentBag);
   int numLabelValues = schema.num_label_values();
   int numAttrValues = schema.num_attr_values(attrNum);

   ASSERT(nonemptyBagNum > 0);
   ASSERT(numLabelValues > 0);
   ASSERT(numAttrValues > 0);
   
   Array2<int> *vc = new Array2<int>(numLabelValues,
				     nonemptyBagNum * numAttrValues);
   int countVcCol = 0;
   for (int i = 0; i < currentBag->size(); i++)
      if ((*currentBag)[i]) {
	  for (int col = 0; col < numAttrValues; col++) {
	     for (int row = 0; row < numLabelValues; row++) {
		(*vc)(row, countVcCol) =
	    (*((*currentBag)[i]->counters().value_counts()[attrNum]))(row,col);
	     }
	     countVcCol++;
	  }
      }
   return *vc;
}

const Array<int>& DummyODTInducer::build_ac(
   DynamicArray<const CtrInstanceBag*> *currentBag, int attrNum) const 
{
   SchemaRC& schema = TS->get_schema();
   int nonemptyBagNum = currentBag->size() - count_empty_bags(currentBag);
   int numAttrValues = schema.num_attr_values(attrNum);

   ASSERT(nonemptyBagNum > 0);
   ASSERT(numAttrValues > 0);

   Array<int> *ac = new Array<int>(nonemptyBagNum * numAttrValues);
   
   int countAc = 0;

   for (int i = 0; i < currentBag->size(); i++) {
      if ((*currentBag)[i])
	 for (int j = 0; j < numAttrValues; j++) {
	    (*ac)[countAc] =
	       (*((*currentBag)[i]->counters().attr_counts()[attrNum]))[j];
	    countAc++;
	 }
   }
   return *ac;
}

static
Bool
is_meaningful_split(DynamicArray<const CtrInstanceBag*>*currentBag,int attrNum)
{
   for (int i = 0; i < currentBag->size(); i++) {
      if ((*currentBag)[i]) {
	 if ((*currentBag)[i]->counters().attr_num_vals(attrNum) >= 2)
	    return TRUE;
      }
   }
  return FALSE;
}

Categorizer*
DummyODTInducer::best_split(DynamicArray<const CtrInstanceBag*> *currentBag,
			    MStringArray*& catNames) const 
{
   SchemaRC& schema = TS->get_schema();
   int numTotalInstances = TS->num_instances();
   
   catNames = NULL; // so caller won't try to delete anything if we
                    // don't get to the allocation part.

   int bestAttrNum = -1;
   //@@ try to find a better way to express maximum of real;
   Real minCondEntropy = REAL_MAX;
   
   for (int attrNum = 0; attrNum < schema.num_attr(); attrNum++) {
      LOG(3, "DummyODTInducer::best_split: Testing attribute " << attrNum
	  << " (" << schema.attr_name(attrNum) << ")." << endl);

      Real condEntropy;
      
      // If the attribute is nominal and has more than one value for
      //   at least one of the bags, then use cond_entropy to determine
      //   the conditional entropy of splitting on it.
      if (schema.attr_info(attrNum).can_cast_to_nominal()) {
	 // check that at least one of the bags in currentBag will be split
	 //   into two or more bags.
         // is_meaningful_split() which will return TRUE if one
	 //   of the   bags splits into 2 or more bags
         if (is_meaningful_split())
	    const Array2<int>& vc = build_vc(currentBag, attrNum);
	    const Array<int>& ac = build_ac(currentBag, attrNum);
            condEntropy = cond_entropy(vc, ac, numTotalInstances);
            ASSERT(condEntropy >= 0);
            LOG(2, "\tcondEntropy is " << condEntropy << endl);
            // To promote stability, only change node if difference is
	    //   over REAL_EPSILON.
            if (condEntropy < minCondEntropy - REAL_EPSILON) {
               minCondEntropy = condEntropy;
               bestAttrNum = attrNum;
            }
         }
         else
            LOG(2, "\tIt causes no split" << endl);
      } else //@@ fix this
	 err << "Unable to handle reals at this stage." << fatal_error;
   }

/*************************** NOT implemented in this stage ************* 
      // Otherwise the attribute is real and real_mutual_info is used to
      //   determine the threshold which gives the maximum information gain
      //   when split on.
      else if (real_mutual_info(TS_with_counters(), attrNum, threshold,
                                mutualInfo,
                                get_lower_bound_min_split_weight(),
                                get_upper_bound_min_split_weight(),
                                get_min_split_weight_percent())) {
         ASSERT(mutualInfo >= 0);
         LOG(2, "\tThreshold is " << threshold << ", mutual info is "
             << mutualInfo << endl);
         // Remember the best
         if (mutualInfo > maxMutualInfo) {
            maxMutualInfo = mutualInfo;
            bestAttrNum = attrNum;
            bestThreshold = threshold;
         }
      }
      else
         LOG(2, "It causes no split" << endl);
   }
*********************************/   
   
   if (bestAttrNum == -1) {
      LOG(2, "Nothing good to split on" << endl);
      return NULL; // nothing good to split on
   } else {
      LOG(2, "best attribute is " << bestAttrNum << " (" <<
	  schema.attr_name(bestAttrNum) << ')' << endl);
      
      MString attrName = schema.attr_info(bestAttrNum).name();
      if (schema.attr_info(bestAttrNum).can_cast_to_nominal()){
	 const NominalAttrInfo& nai =
	    schema.attr_info(bestAttrNum).cast_to_nominal();
	 int size = nai.num_values() + 1; // +1 for unknown
	 catNames = new MStringArray(UNKNOWN_CATEGORY_VAL, size);
	 (*catNames)[UNKNOWN_CATEGORY_VAL] = "Unknown " + nai.name();
	 Category cat = FIRST_CATEGORY_VAL;
	 for (int i = 1; i < size; i++, cat++)
	    (*catNames)[cat] = nai.get_value(i + UNKNOWN_NOMINAL_VAL);
	 return new AttrCategorizer(schema, bestAttrNum, attrName);
      }
      else {
	 // If the best attribute is real, then catNames is an array of
	 //   3 MStrings: unknown, left node (less than threshold), or
	 //   right node (greater than or equal to threshold).
	 int bestThreshold = 0;
	 catNames = new MStringArray(UNKNOWN_CATEGORY_VAL, 3);
	 (*catNames)[UNKNOWN_CATEGORY_VAL] =
	    "Unknown " + schema.attr_info(bestAttrNum).name();
	 (*catNames)[UNKNOWN_CATEGORY_VAL+1] =
	    "<= " + MString(bestThreshold,0);
	 (*catNames)[UNKNOWN_CATEGORY_VAL+2] =
	    "> " + MString(bestThreshold,0);
	 return new ThresholdCategorizer(schema, bestAttrNum,
					 bestThreshold, attrName);
      }
   }
}

DummyODTInducer::DummyODTInducer(const MString& dscr, CGraph* aCgraph) :
   ODTInducer(dscr, aCgraph)
{
}

main()
{
   Mcout << "t_ODTInducer executing" << endl;
      
   DummyODTInducer inducer("test");
 
   inducer.read_data("monk1-full");
   inducer.train();
   Mcout << "End train" << endl;

//#ifdef INTERACTIVE
      DotGraphPref pref;
      MLCOStream out3(XStream);
      inducer.display_struct(out3,pref);
//#endif INTERACTIVE
   CatTestResult result(inducer.get_categorizer(),
                        inducer.instance_bag(), "monk1-full");
   result.display(Mcout);
   
   return 0; // return success to shell
}   
