// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Implement ID3 algorithm which is a top-down decision-tree
                   induction algorithm.
  Assumptions  : 
  Comments     : Uses mutual information (original gain criteria),
                   and not the more recent information gain ratio.
  Complexity   : See TDDTInducer.
                 Our split() method uses entropy and takes time O(vy) where
                   v is the total number of attribute values (over all
                   attributes) and y is the number of label values.
                   This can be derived by noting that mutual_info is
                   computed for each attribute.  
                 Node categorizers (for predict) are AttrCategorizer and
                   take constant timel, thus the overall prediction
                   time is O(path-length).  See TDDTInducer.h
  Enhancements : Compute entropy once for the node, and pass it along
                   to avoid multiple computations like we do now.
  History      : Yeogirl Yun                                        7/4/95
                   Added copy constructor.              
                 Ronny Kohavi                                       9/08/93
                   Initial revision (.h,.c)
***************************************************************************/
#include <basics.h>
#include <ID3Inducer.h>
#include <AttrCat.h>
#include <ThresholdCat.h>
#include <entropy.h>

RCSID("MLC++, $RCSfile: ID3Inducer.c,v $ $Revision: 1.39 $")

/***************************************************************************
  Description : Constructor, destructor.
  Comments    :
***************************************************************************/
ID3Inducer::ID3Inducer(const MString& dscr, CGraph* aCgraph) :
        TDDTInducer(dscr, aCgraph), level(0)
{
}

ID3Inducer::ID3Inducer(const ID3Inducer& source, CtorDummy)
   : TDDTInducer(source, ctorDummy)
{
   set_level(source.get_level());
}

ID3Inducer::~ID3Inducer() {};

/***************************************************************************
  Description : Find the best attribute split using mutual
                  information (information gain).
                Return an AttrCategorizer that splits on that attribute.
                Return NULL if there is nothing good to split on.
  Comments    : We break ties in mutual information towards earlier
                  attributes.
***************************************************************************/
Categorizer* ID3Inducer::best_split(MStringArray*& catNames) const
{
   catNames = NULL; // so caller won't try to delete anything if we
                    // don't get to the allocation part.

   if (counters().label_num_vals() == 1)
      return NULL; // if we have one label value, we're done.

   if (get_max_level() > 0 && get_level() >= get_max_level()) {
      LOG(2, "Maximum level " << get_max_level() << " reached " << endl);
      return NULL;
   }

   LOG(3, counters());
   SplitInfo bestSi;
   int numAttr = TS->get_schema().num_attr();
   
   for (int attrNum = 0; attrNum < numAttr; attrNum++) {
      SplitInfo si = split_info(attrNum);
      // Remember the best.  REAL_EPSILON is added because on monk1,
      // the difference is 1e-16, and we want to tie break exactly as
      // C4.5 does.  
      if (si.mutualInfo > bestSi.mutualInfo + REAL_EPSILON)
	 bestSi = si;
   }
      
   Categorizer* bestCat = splitInfo_to_cat(bestSi, catNames);
   if (bestCat)
      bestCat->build_distr(TS_with_counters());
   return bestCat;
}


/***************************************************************************
  Description : Create an Inducer for recursive calls.
  Comments    : Since TDDTInducer is an abstract class, it can't
                  do the recursive call.
***************************************************************************/

TDDTInducer* ID3Inducer::create_subinducer(const MString& dscr,
                                          CGraph& aCgraph) const
{
   ID3Inducer *inducer = new ID3Inducer(dscr, &aCgraph);

   inducer->copy_options(*this);
   inducer->set_level(get_level() + 1);
   return inducer;
}


/***************************************************************************
  Description : Find a good split for the given attribute, and return
                  relevant information in splitInfo.
  Comments    :
***************************************************************************/


// ok_to_split checks if it is OK to make a split on the nominal
//   attribute by making sure at least two branches have more than
//   minSplit instances.
static Bool ok_to_split(int attrNum, const BagCounters& counters, int minSplit)
{
   // If there aren't two values, we clearly can't split.
   if (counters.attr_num_vals(attrNum) < 2)
      return FALSE;

   if (minSplit <= 1)
      return TRUE; // We know there are two values, and each must have at
		   // least one instance.

   Array<int>* ac = counters.attr_counts()[attrNum];
   if (ac == NULL)
      err << "ID3Inducer::ok_to_split: No counters" << fatal_error;
   
   int numAboveMin = 0;
   for (int i = ac->low(); numAboveMin < 2 && i <= ac->high(); i++)
      if ((*ac)[i] >= minSplit)
	 numAboveMin++;

   return (numAboveMin >= 2);
}


SplitInfo ID3Inducer::split_info(int attrNum) const
{
   SchemaRC schema = TS->get_schema();

   if (attrNum < 0 || attrNum > schema.num_attr())
      err << "ID3Inducer::split_info: attrNum " << attrNum
	  << " not in range 0 to " << schema.num_attr() << fatal_error;
   
   LOG(2, "Testing attribute " << attrNum << " (" 
       << TS->get_schema().attr_name(attrNum) << "): ");

   Real mutualInfo;

   
   SplitInfo si; // "noReasonableSplit" by default
   si.attrNum = attrNum;
   int minSplit = min_split(get_log_options(), TS_with_counters(),
			    get_lower_bound_min_split_weight(),
			    get_upper_bound_min_split_weight(),
			    get_min_split_weight_percent());
   
   // If the attribute is nominal and has more than one value, then use
   //   mutual_info to determine the information gained by splitting on it.
   if (schema.attr_info(attrNum).can_cast_to_nominal()) {
      if (ok_to_split(attrNum, TS_with_counters().counters(), minSplit)) {
	 mutualInfo = mutual_info(get_log_options(),
				  TS_with_counters(), attrNum);
	 if (get_split_by() == normalizedMutualInfo)
            // There may be only one value and it's useful because of unknowns.
            // We therefore add max(2,val)
	    mutualInfo = mutualInfo 
	       / log_bin(max(2, schema.num_attr_values(attrNum)));
	 ASSERT(mutualInfo >= 0);
	 LOG(2, "Mutual info is " << mutualInfo << endl);
	 si.splitType = SplitInfo::nominalSplit;
	 si.mutualInfo = mutualInfo;
      }
   }
   // Otherwise the attribute is real and real_mutual_info is used to
   //   determine the threshold which gives the maximum information gain
   //   when split on.      
   else {
      Real condEntropy, threshold;
      if (real_cond_entropy(get_log_options(),
			    TS_with_counters(), attrNum, threshold,
			    condEntropy, minSplit)) {
	 Real bagEntropy = entropy(get_log_options(),
			      TS_with_counters().counters().label_counts(),
			      TS_with_counters().num_instances());
         mutualInfo = bagEntropy - condEntropy;
	 if (mutualInfo < 0 && -mutualInfo < REAL_EPSILON)
             mutualInfo = 0; // mutual_info should never be negative;
	       //  this accounts for possible numerical representation errors
	 if (mutualInfo < 0)  {
	    Mcerr << "Problem with mutual info.  Value is " << mutualInfo
                << " which is negative.  Bag is " << TS_with_counters() << endl;
            err << "Split attempted on attribute " << attrNum 
	        << " with threshold " << threshold << endl
	        << " conditional entropy is " << condEntropy
	        << " our entropy is " << bagEntropy << fatal_error;
	 }
	    
	 LOG(2, "Threshold is " << threshold << ", mutual info is "
	     << mutualInfo << endl);
	 si.splitType = SplitInfo::realThresholdSplit;
	 si.typeInfo.threshold = threshold;
	 si.mutualInfo = mutualInfo;
      }
   }
      
   if (si.splitType == SplitInfo::noReasonableSplit)
      LOG(2, "No reasonable split" << endl);      

   return si;
}

/***************************************************************************
  Description : Build categorizer for the given splitInfo.
                If the attribute number is -1, return NULL (no good split).
  Comments    :
***************************************************************************/

Categorizer* ID3Inducer::splitInfo_to_cat(const SplitInfo& si,
					  MStringArray*& catNames) const
{
   int attrNum = si.attrNum;
   SchemaRC schema = TS->get_schema();
   
   if (attrNum == -1) {
      LOG(2, "Nothing good to split on" << endl);
      return NULL; // nothing good to split on
   }

   // Else, build the categorizer
   ASSERT(si.mutualInfo >= 0);
   
   LOG(2, "Creating split on attribute " << attrNum << " ("
       << schema.attr_name(attrNum) << ") at level " << level << endl);

   MString attrName = schema.attr_info(attrNum).name();
   if (get_debug())
      attrName += " (#=" + MString(TS->num_instances(),0) +
	 "/E="  + MString(entropy(get_log_options(), TS_with_counters()),3) +
	 "/MI=" + MString(si.mutualInfo,3) + ")";

   if (schema.attr_info(attrNum).can_cast_to_nominal()) {
      ASSERT(si.splitType == SplitInfo::nominalSplit);
      const NominalAttrInfo& nai = schema.attr_info(attrNum).cast_to_nominal();
      int size = nai.num_values() + 1; // +1 for unknown
      catNames = new MStringArray(UNKNOWN_CATEGORY_VAL, size);
      (*catNames)[UNKNOWN_CATEGORY_VAL] = "?";
      Category cat = FIRST_CATEGORY_VAL;
      for (int i = 1; i < size; i++, cat++)
	 (*catNames)[cat] = nai.get_value(i + UNKNOWN_NOMINAL_VAL);
      
      return new AttrCategorizer(schema, attrNum, attrName);
   }
   else if (schema.attr_info(attrNum).can_cast_to_real()) {
      ASSERT(si.splitType == SplitInfo::realThresholdSplit);
      // If the best attribute is real, then catNames is an array of
      //   3 MStrings: unknown, left node (less than threshold), or
      //   right node (greater than or equal to threshold).
      catNames = new MStringArray(UNKNOWN_CATEGORY_VAL, 3);
      (*catNames)[UNKNOWN_CATEGORY_VAL] = "?";
      (*catNames)[UNKNOWN_CATEGORY_VAL+1] =
	 "<= " + MString(si.typeInfo.threshold,0);
      (*catNames)[UNKNOWN_CATEGORY_VAL+2] =
	 "> " + MString(si.typeInfo.threshold,0);
      return new ThresholdCategorizer(schema, attrNum, si.typeInfo.threshold,
				      attrName);
   }
   else {
      err << "ID3Inducer::SplitInfo_to_cat: unrecognized attribute type"
	  << " for attribute " << attrNum << fatal_error;
      return NULL;
   }
}




/*****************************************************************************
  Description : Returns the pointer to the copy of ID3Inducer with the
                  same settings.
  Comments    :
*****************************************************************************/
Inducer* ID3Inducer::copy() const
{
   Inducer *ind = new ID3Inducer(*this, ctorDummy);
   return ind;
}
