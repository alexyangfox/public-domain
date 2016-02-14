// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Entropy related computations.
  Assumptions  :
  Comments     : All logs are base 2 (other bases just scale the entropy).
                 The reason for using log_bin is simply that the examples
                   in Quinlan's C4.5 book use it, and those examples 
                   were used for testing.  It's also a measure of the
                   number of "bits" in information theory, so it's
                   appealing in that sense too.
                 The computation is based on
                 1. "Boolean Feature Discovery in Empirical Learning"
                     / Pagallo and Haussler.
                 2. "A First Course in Probability, 2nd Edition"
                     / Ross, pages 354-359.
                 3. "C4.5: Programs for Machine Learning"
                     / Ross Quinlan, pages 18-24.
  Complexity   : Given that the number of values for var x is |x| and
                    for y it is |y|, the complexity of entropy(_) is O(|y|).
                    The complexity of cond_entropy(_,x) and mutual_info is
                    O(|x||y|).
  Enhancements : Add gain ratio (Quinlan's C4.5 page 23).  This is
                   suppose to give a better measure on multi-way splits.
                 Allow dealing with prior probabilities.
                 The prior probabilities for attributes should come from
                   the attributes themselves.   The problem is of
                   course that of context.  p(x) is different from
                   p(x|y) and after splitting on y in the tree, x's
                   prior is no longer relevant.  Most programs like
                   REDWOOD and CART allow specifying priors only for
                   the labels, and since a split is never done on the
                   label we have no problem.
  History      : Chia-Hsin Li
                   Revised real_mutual_info to allow multiple bags.
                 Brian
                 Ronny Kohavi                                       9/04/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <LogOptions.h>
#include <entropy.h>
#include <math.h>
#include <Array.h>
#include <Array2.h>
#include <CatTestResult.h>

//RCSID("MLC++, $RCSfile: entropy.c,v $ $Revision: 1.35 $")

// Log base 2.  Solaris doesn't have log2().

/***************************************************************************
  Description : Compute the entropy H(Y) for label Y.
  Comments    : H(Y) = -sum_{y in range(Y)} p(y)log(p(y))
                We estimate p(y) by count(y)/num-instances.
                Any p(y) = 0 is ignored.
***************************************************************************/
Real entropy(const LogOptions& logOptions, const CtrInstanceBag& bag)
{
   return entropy(logOptions, bag.counters().label_counts(),
                  bag.num_instances());

}


Real entropy(const LogOptions&,
	     const Array<int>& labelCount, int numTotalInstances)
{
   if (numTotalInstances <= 0)
          err << "entropy:: numTotalInstances (" << numTotalInstances
              << ") is negative or zero" << fatal_error;

   Real H = 0;
   for (NominalVal y = labelCount.low(); y <= labelCount.high(); y++)
      if (labelCount[y] != 0) {
         // We know that num_instances() > 0, so this cannot
	 // be division by zero.
         Real prob_y = (Real)labelCount[y] / numTotalInstances;
         H -= prob_y * log_bin (prob_y);
      }

   return H;
}


/***************************************************************************
  Description : Weighted entropy.  The labelCount array determines
                  the proportional weighting.  The weight per
		  instance of class i is 1/(k*p_i) where k is
		  the number of classes and p_i is the probability
		  of class i in the parent node, i.e., before the split.
		This ensures that each class has equal weight and
		  therefore that the entropy of the parent is log_k
		  as if the classes were equally weighted.
		When a confidence interval is given probabilities
		  are shifted towards 1/numClasses to make the entropy
		  more pessimistic.  The probabilities are then normalized to
		  sum up to 1.
  Comments    : Slightly inefficient because we scan the arrays.
***************************************************************************/

// Pessimistic correction.  Shift everything towards 1/numClasses
static void pessimistic_correction(Real& probY, int numClasses,
				   int numInstances, 
				   Real confIntervalZ)
{
   Real low, high;
   Real worseProb = 1/Real(numClasses);
   
   CatTestResult::confidence(low, high, probY, numInstances, confIntervalZ);
   if (low < worseProb && worseProb < high)
      probY = worseProb;
   else if (fabs(low - worseProb) < fabs(high-worseProb))
      probY = low;
   else
      probY = high;
}

// Normalize the probabilities so that they sum up to 1.
static void normalize_probs(Array<Real>& probs)
{
   Real sum = 0;
   for (int i = 0; i < probs.size(); i++)
      sum += probs.index(i);

   for (i = 0; i < probs.size(); i++)
      probs.index(i) /= sum;
}
   

Real weighted_entropy(const LogOptions& logOptions, 
		      const CtrInstanceBag& bag,
		      const CtrInstanceBag& parentBag,
		      Real confIntervalZ)
{
   return weighted_entropy(logOptions, bag.counters().label_counts(),
			   parentBag.counters().label_counts(),
			   bag.num_instances(),
			   parentBag.num_instances(), confIntervalZ);
}


Real weighted_entropy(const LogOptions&,
		      const Array<int>& labelCount, 
		      const Array<int>& parentLabelCount, 
		      int numTotalInstances,
		      int parentNumInstances,
		      Real confIntervalZ)
{
   if (numTotalInstances <= 0)
          err << "weighted_entropy:: numTotalInstances (" << numTotalInstances
              << ") is negative or zero." << fatal_error;

   if (parentNumInstances <= 0)
          err << "weighted_entropy:: parentNumInstances (" <<parentNumInstances
              << ") is negative or zero." << fatal_error;

   if (parentLabelCount.size() != labelCount.size())
      err << "weighted_entropy:: parentLabelCount size " 
	  << parentLabelCount.size() << " does not match node with size "
	  << labelCount.size() << fatal_error;

   if (confIntervalZ < 0)
      err << "weighted_entropy:: negative confidence intervalZ" << fatal_error;

   // Count number of labels in parent node.
   //   int k  = 0;
   //   for (NominalVal y = labelCount.low(); y <= labelCount.high(); y++)
   //      if (parentLabelCount[y])
   //	 k++;
   // Doesn't really matter if we just use number of label-vals in problem.
   int k = labelCount.size() - 1;

   Real totalWeight = 0; // recompute weighted total
   for (NominalVal y = labelCount.low(); y <= labelCount.high(); y++) {
      Real kp = Real(k*parentLabelCount[y])/parentNumInstances;
      if (kp > REAL_EPSILON)
	 totalWeight += labelCount[y] / kp;
   }

   ASSERT(totalWeight > 0);

   Array<Real> probs(labelCount.low(), labelCount.size(), 0.0);
   for (y = labelCount.low(); y <= labelCount.high(); y++) {
      Real kp = Real(k*parentLabelCount[y])/parentNumInstances;
      if (kp > REAL_EPSILON) {
	 Real probY;
	 if (labelCount[y] == 0) 
	    probY = 0;
	 else
	    probY = (Real)labelCount[y] / totalWeight / kp;
	 if (confIntervalZ > 0)
	    pessimistic_correction(probY, k, numTotalInstances, confIntervalZ);
         probs[y] = probY;
      }
   }

   if (confIntervalZ > 0)
      normalize_probs(probs);
   Real H = 0;
   for (y = probs.low(); y <= probs.high(); y++)
      if (probs[y] > REAL_EPSILON)
	    H -= probs[y] * log_bin (probs[y]);

   return H;
}


/***************************************************************************
  Description : Gini index - weighted.  sum 1-p_i^2, weighted by parents
  Comments    :
***************************************************************************/


Real weighted_gini(const LogOptions& logOptions, 
		      const CtrInstanceBag& bag,
		      const CtrInstanceBag& parentBag,
		      Real confIntervalZ)
{
   return weighted_gini(logOptions, bag.counters().label_counts(),
			   parentBag.counters().label_counts(),
			   bag.num_instances(),
			   parentBag.num_instances(), confIntervalZ);
}


Real weighted_gini(const LogOptions&,
		      const Array<int>& labelCount, 
		      const Array<int>& parentLabelCount, 
		      int numTotalInstances,
		      int parentNumInstances,
		      Real confIntervalZ)
{
   if (numTotalInstances <= 0)
          err << "weighted_gini:: numTotalInstances (" << numTotalInstances
              << ") is negative or zero." << fatal_error;

   if (parentNumInstances <= 0)
          err << "weighted_gini:: parentNumInstances (" <<parentNumInstances
              << ") is negative or zero." << fatal_error;

   if (parentLabelCount.size() != labelCount.size())
      err << "weighted_gini:: parentLabelCount size " 
	  << parentLabelCount.size() << " does not match node with size "
	  << labelCount.size() << fatal_error;

   if (confIntervalZ < 0)
      err << "weighted_gini:: negative confidence intervalZ" << fatal_error;

   // Count number of labels in parent node.
   int k  = 0;
   for (NominalVal y = labelCount.low(); y <= labelCount.high(); y++)
      if (parentLabelCount[y])
	 k++;

   Real totalWeight = 0; // recompute weighted total of our node
   for (y = labelCount.low(); y <= labelCount.high(); y++) {
      Real kp = Real(k*parentLabelCount[y])/parentNumInstances;
      if (kp > REAL_EPSILON && labelCount[y] != 0)
	 totalWeight += labelCount[y] / kp;
   }


   Real H = 1;
   for (y = labelCount.low(); y <= labelCount.high(); y++) {
      Real kp = Real(k*parentLabelCount[y])/parentNumInstances;
      if (kp > REAL_EPSILON) {
	 Real probY;
	 if (labelCount[y] == 0) 
	    probY = 0;
	 else {
	    ASSERT(totalWeight > 0);
	    probY = (Real)labelCount[y] / totalWeight / kp;
	 }
	 if (confIntervalZ > 0)
	    pessimistic_correction(probY, k, numTotalInstances,
				   confIntervalZ);
	 H -= probY * probY;
      }
   }

   return H;
}





/***************************************************************************
  Description : Build the vc and ac array needed for calculating conditional
                  entropy. Both of them concatenates all the vc arrays of the
		  bags. 
  Comments    :                
***************************************************************************/
Array2<int>* build_vc(const LogOptions&,
			    const Array<const CtrInstanceBag*> &currentLevel,
			    int attrNum)
{
   ASSERT(currentLevel[0]);
   const SchemaRC& schema = currentLevel[0]->get_schema();
   int numBags = currentLevel.size();
   int numLabelValues = schema.num_label_values();
   int numAttrValues = schema.num_attr_values(attrNum);

   ASSERT(numBags > 0);
   ASSERT(numLabelValues > 0);
   ASSERT(numAttrValues > 0);
   
   Array2<int> *vc = new Array2<int>(numLabelValues,
				     numBags * numAttrValues);
   int countVcCol = 0;
   for (int bagCount = 0; bagCount < numBags; bagCount++) {
      ASSERT(currentLevel[bagCount]);
      for (int attrValCount = 0; attrValCount < numAttrValues;
				 attrValCount++, countVcCol++) {
	 for (int labelValCount = 0; labelValCount < numLabelValues;
				     labelValCount++) {
	    const BagCounters& bc = currentLevel[bagCount]->counters();
	    (*vc)(labelValCount, countVcCol) =
	       (*(bc.value_counts()[attrNum]))(labelValCount,attrValCount);
	 }
      }
   }
   return vc;
}

Array<int>* build_ac(const LogOptions&,
			   const Array<const CtrInstanceBag*> &currentLevel,
			   int attrNum)
{
   ASSERT(currentLevel[0]);
   const SchemaRC& schema = currentLevel[0]->get_schema();
   int numBags = currentLevel.size();
   int numAttrValues = schema.num_attr_values(attrNum);

   ASSERT(numBags > 0);
   ASSERT(numAttrValues > 0);

   Array<int> *ac = new Array<int>(numBags * numAttrValues);
   
   int countAc = 0;

   for (int bagCount = 0; bagCount < numBags; bagCount++) {
      ASSERT(currentLevel[bagCount]);
      for (int attrCount = 0; attrCount < numAttrValues;
			      attrCount++, countAc++) {
	 const BagCounters& bc = currentLevel[bagCount]->counters();
	 (*ac)[countAc] =
	    (*(bc.attr_counts()[attrNum]))[attrCount];
      }
   }
   return ac;
}

      
/***************************************************************************
  Description : Compute conditional entropy of the label given
                   attribute X
  Comments    : From Ross, Conditional entropy is defined as:
                 H(Y|X) = sum_x H(Y|X=x)*P(X=x).  
                        = sum_x (-sum_y p(Y=y|X=x)log p(Y=y|X=x)) * P(X=x)
                 now derive Pagallo & Haussler's formula
                        = -sum_{x,y} p(Y=y, X=x) log p(Y=y|X=x)
                 Here we estimate p(Y=y, X=x) by counting, but if we
                   have priors on the probabilities of the labels, then
                   p(x,y) = p(x|y)*p(y) = count(x,y)/s(y)* prior(y)
                   and p(x) = sum_y prior(y) count(x,y)/s(y).

                 By counting we get the following:
                 -sum_{x,y} num(Y=y,X=x)/num-rec * log num(Y=y,X=x)/num(X=x)
***************************************************************************/
Real cond_entropy(const LogOptions& logOptions,
		  const CtrInstanceBag& bag, int attrNumX)
{
   return cond_entropy(logOptions, *bag.counters().value_counts()[attrNumX],
                       *bag.counters().attr_counts()[attrNumX],
                       bag.num_instances());
}

Real cond_entropy(const LogOptions&,
		  const Array2<int>& vc, const Array<int>& ac,
		  int numTotalInstances)
{
   DBG(if (numTotalInstances < 0)
          err << "cond_entropy:: numTotalInstances (" << numTotalInstances
              << ") is negative." << fatal_error;
       if (numTotalInstances == 0)
          err << "cond_entropy:: no instances in bag" << fatal_error);

   Real H = 0;
   for (NominalVal y = vc.start_row(); y <= vc.high_row(); y++)
      for (NominalVal x = vc.start_col(); x <= vc.high_col(); x++) {
         int num_xy = vc(y,x);
         int num_x  = ac[x];
         if (num_xy != 0) // beware of log(0)
            H -= num_xy * log_bin((Real)num_xy/num_x);
      }
   H /= numTotalInstances; // We know this won't be division by zero.

   if (H < 0)
      if (H > -REAL_EPSILON)
	 H = 0;
      else
	 err << "cond_entropy: negative entropy " << H << endl
	     << "ac is " << ac << endl
	     << "vc is " << vc << fatal_error;
   
   
   return H;
}

/***************************************************************************
  Description : Compute the J-measure. 
  Comments    : See papers by Goodman and Smyth, such as 
                Data Engineering, v.4, no.4, pp.301-316, 1992.
                The J-measure summed over all values of x gives info-gain.
		The J-measure is
		    sum_y p(x,y)log(p(x,y)/(p(x)p(y)))
		    1/n * sum_y n(x,y)log(n(x,y)*n/(n(x)n(y)))
***************************************************************************/

Real j_measure(const LogOptions& logOptions,
	       const CtrInstanceBag& bag, int attrNumX, NominalVal x)
{
   return j_measure(logOptions, *bag.counters().value_counts()[attrNumX],
                       *bag.counters().attr_counts()[attrNumX], 
                       bag.counters().label_counts(), x, 
                       bag.num_instances());
}

Real j_measure(const LogOptions&,
		  const Array2<int>& vc, const Array<int>& ac,
                  const Array<int>& labelCounts, NominalVal x, 
	          int numTotalInstances)
{
   DBG(if (numTotalInstances < 0)
          err << "j_measure:: numTotalInstances (" << numTotalInstances
              << ") is negative." << fatal_error;
       if (numTotalInstances == 0)
          err << "j_measure:: no instances in bag" << fatal_error);

   Real j = 0;
   for (NominalVal y = vc.start_row(); y <= vc.high_row(); y++) {
      int num_xy = vc(y,x);
      int num_x  = ac[x];
      int num_y  = labelCounts[y];
      if (num_xy != 0) { // beware of log(0) 
	 DBG(ASSERT(num_x > 0 && num_y > 0));
	 j += num_xy * log_bin(numTotalInstances*Real(num_xy)/(num_x * num_y));
      }
   }
   j /= numTotalInstances; // We know this won't be division by zero.

   if (j < 0 && -j < REAL_EPSILON)
      j = 0;  // should never be negative; this accounts
	      // for possible numerical representation errors
   ASSERT(j >= 0);

   return j;
}
   
/***************************************************************************
  Description : Compute the entropy of a two-way split on the given value.
  Comments    :
***************************************************************************/

Real two_way_cond_entropy(const LogOptions& logOptions,
			  const CtrInstanceBag& bag, 
			  int attrNumX, NominalVal val)
{
   return two_way_cond_entropy(logOptions, 
			       *bag.counters().value_counts()[attrNumX],
			       *bag.counters().attr_counts()[attrNumX],
			       bag.num_instances(), val);
}
   


Real two_way_cond_entropy(const LogOptions& logOptions,
			  const Array2<int>& vc, const Array<int>& ac,
			  int numTotalInstances, NominalVal val)
{
   Array2<int> newVc(vc.start_row(), vc.start_col(), vc.num_rows(), 2);
   Array<int>  newAc(ac.low(), 2);

   int vcLow = vc.start_col();
   for (int i = vc.start_row(); i <= vc.high_row(); i++) {
      newVc(i, vcLow) = 0;
      for (int j = vc.start_col(); j <= vc.high_col(); j++)
	 if (j == val)
	    newVc(i, vcLow + 1) = vc(i, j);
         else
	    newVc(i, vcLow) += vc(i,j);
   }

   int acLow = ac.low();
   newAc[acLow] = 0;
   for (i = ac.low(); i <= ac.high(); i++)
      if (i == val)
	 newAc[acLow + 1] = ac[i];
      else
	 newAc[acLow] += ac[i];
   
   return cond_entropy(logOptions, newVc, newAc, numTotalInstances);
}


/***************************************************************************
  Description : Compute the mutual information which is defined as
                  I(Y;X) = H(Y) - H(Y|X).
                Some researches like Quinlan call this "gain."
                This is the amount of information gained about the
                  category value of an instance after we test the
                  variable X.
  Comments    : The mutual information must be >= 0. 
                For a proof see doc/entropy.tex
***************************************************************************/
Real mutual_info(const LogOptions& logOptions,
		 const CtrInstanceBag& bag, int attrNumX)
{
   if (bag.counters().attr_counts()[attrNumX] == NULL)
      err << "entropy::mutual_info: attribute " << attrNumX
	  << " is not nominal (counts array is NULL)" << fatal_error;
   
   return mutual_info(logOptions, *bag.counters().value_counts()[attrNumX],
                      *bag.counters().attr_counts()[attrNumX],
                      bag.counters().label_counts(),
                      bag.num_instances());
}

Real mutual_info(const LogOptions& logOptions,
		 const Array2<int>& vc, const Array<int>& ac,
                 const Array<int>& lc, int numTotalInstances)
{
   Real mi = entropy(logOptions, lc, numTotalInstances) -
             cond_entropy(logOptions, vc, ac, numTotalInstances);
   if (mi < 0 && -mi < REAL_EPSILON)
      mi = 0;  // mutual_info should never be negative; this accounts
	       // for possible numerical representation errors
   ASSERT(mi >= 0);
   return mi;
}


/***************************************************************************
  Description : Given lowerBoundMinSplit, upperBoundMinSplit, and
                  minSplitPercent, it returns the minSplit which is used
		  in real_mutual_info().
  Comments    : This function is called by inducers.
***************************************************************************/
int min_split(const LogOptions&,
	      const CtrInstanceBag &bag,
	      int lowerBoundMinSplit, int upperBoundMinSplit,
	      Real minSplitPercent) 
{
   // Calculate minSplit.
   DBG(if (lowerBoundMinSplit < 1)
          err << "real_mutual_info:  lowerBoundMinSplit ("
              << lowerBoundMinSplit << ") must be at least one"
              << fatal_error);
   int minSplit = (int)(minSplitPercent *
			bag.num_instances()/bag.num_categories());
   if (minSplit <= lowerBoundMinSplit)
      minSplit = lowerBoundMinSplit;
   else if (minSplit > upperBoundMinSplit)
      minSplit = upperBoundMinSplit;

   ASSERT(minSplit >= 1); // must have at least one instance!
   return minSplit;
}

/***************************************************************************
  Description :	Given a bag of instances and the attribute number (attrNumX)
		  of a real attribute, this function will find all reasonable
		  threshold values for the attribute and calculate the mutual
		  information for each one.
		It returns both the best mutual information (in maxMI) and
		  the threshold value where that mutual information is found
		  (in bestThreshold).  It also returns TRUE if a reasonable
		  threshold split was found and FALSE otherwise.
		A reasonable threshold value to split on must split the
		  instances in the bag into two nodes where each node
		  contains some minimum number of instances.  This minimum
		  value (minSplit) is calculated as follows:
		  (1) minSplit = minSplitPercent * num inst / num categories
		  (2) If minSplit is not in the range [lowerBoundMinSplit,
		      upperBoundMinSplit], then minSplit is set to the nearest
		      bound.
		  Note that lowerBoundMinSplit, upperBoundMinSplit, and
		  minSplitPercent are parameters to real_mutual_info.
  Comments    : If multiple threshold values result in the best mutual info
		  the lowest of these thresholds will be returned.
***************************************************************************/
const int UNKNOWN_NODE = 0;
const int LEFT_NODE = 1;
const int RIGHT_NODE = 2;

Bool real_cond_entropy(const LogOptions& logOptions,
		       const CtrInstanceBag& bag, int attrNumX,
		       Real& bestThreshold, Real& minCondEntropy,
		       int minSplit)
{
   Array<const CtrInstanceBag*> bagSet(1);

   bagSet[0] = &bag;
   return real_cond_entropy(logOptions,bagSet, attrNumX, bestThreshold,
			    minCondEntropy, minSplit, bag.num_instances());
}

/***************************************************************************
  Description : 
  Comments    :
***************************************************************************/
Bool check_min_split(Array<int>& ac, int minSplit, int bagNum)
{
   Bool leftFlag = FALSE, rightFlag = FALSE;
   for (int bagCount = 0; bagCount < bagNum; bagCount++) {
      if (ac[bagCount*3+LEFT_NODE] >= minSplit)
	 leftFlag = TRUE;
      if (ac[bagCount*3+RIGHT_NODE] >= minSplit)
	 rightFlag = TRUE;
   }
   return leftFlag && rightFlag;
}

/***************************************************************************
  Description : Compute the real_cond_entropy for a set of bags. 
  Comments    : Initially, all instances are at the right hand side of ac and
                  vc. To find the best threshold, we shift the instances from
		  right to left and calculate their conditional entropy. Then
		  we pick up the one with minimum conditional entropy and
		  return bestThreshold and minCondEntropy as results.
		When two entropies are equal, we prefer the one that
		  splits instances equally into two bins in the hope of making
		  the tree shallower.
		Test show minor effect but it does help pima for different
		  leaf counts.
***************************************************************************/
Bool real_cond_entropy(const LogOptions& logOptions,
		       const Array<const CtrInstanceBag*> &bagset,
		       int attrNumX,
		       Real& bestThreshold,
		       Real& minCondEntropy,
		       int minSplit, int instancesNum)
{
   int bagNum = bagset.size();

   int categoriesNum = bagset[0]->num_categories() + 1; 
   ASSERT(categoriesNum > 0); // categoriesNum must be greater than 0
   
   ASSERT(bagset.size() >= 1);
   ASSERT(bagset[0]);
   const CtrInstanceBag &bagProto = *(bagset[0]);
   const SchemaRC& schema = bagProto.get_schema();
   const NominalAttrInfo& labelInfo = schema.label_info().cast_to_nominal();
   const RealAttrInfo& realAttrInfo =schema.attr_info(attrNumX).cast_to_real();

   // ac (attrCounts) is an array containing the total number of instances on
   //   each side of the threshold for all labels.  It is the sum of the rows
   //   of the vc matrix.
   // Since we want to concatenate all the ac's of each bag in bag, we
   // need 
   Array<int> ac(0, 3 * bagNum, 0);

   // vc (valueCounts) is a matrix containing the number of instances on each
   //   side of the threshold for each possible label.
   Array2<int> vc(categoriesNum , 3 * bagNum, 0);
		      
   // alpArray is an array of attrLabelPairs.  It will contain an entry for
   //   every instance in the bag and will be sorted in increasing order of
   //   attrVals.
   DynamicArray<attrLabelPair> alpArray(instancesNum);
   
   // Initialize ac, vc, and alpArray.
   int instanceCount = 0;
   int bagCount = 0;
   int categoriesCount = 0;
   
   for (int i = 0; i < bagNum; i++) {
      ASSERT(bagset[i]);
      const CtrInstanceBag &bag = *(bagset[i]);
      DBGSLOW(ASSERT(bag.get_schema() == schema));
      DBGSLOW(ASSERT(schema.label_info().cast_to_nominal() == labelInfo));
      for (Pix pix = bag.first(); pix; bag.next(pix)) {
         // Note, operations with instances here are a time bottleneck so
	 //   we get the actual body of the instance to save a little time.
	 const Instance& instance = *bag.get_instance(pix).read_rep();
         const AttrValue_& aVal = instance[attrNumX];
	 const AttrValue_& labVal = instance.get_label();
	 const NominalVal lab =labelInfo.get_nominal_val(labVal);
	 if (realAttrInfo.is_unknown(aVal)) {
	    ++vc(lab, bagCount*3 + UNKNOWN_NODE);
	    ++ac.index(bagCount*3 + UNKNOWN_NODE);
	 } else {
	    DBG(if (labelInfo.is_unknown(labVal))
		err << "Unknown found in data set." << endl);
            attrLabelPair& alp = alpArray.index(instanceCount);	    
	    alp.attrVal =realAttrInfo.get_real_val(aVal);
	    alp.label = labVal;
	    alp.pos = bagCount;
	    alp.bag = i;
	    ++vc(lab, bagCount*3 + RIGHT_NODE);
	    ++ac.index(bagCount*3 + RIGHT_NODE);
	    instanceCount++;
	 }
      }
      bagCount++;
      categoriesCount += bagset[i]->num_categories();
   }
   alpArray.truncate(instanceCount);
   ASSERT(instanceCount == alpArray.size());

   // Sort alpArray.
   alpArray.sort();

   minCondEntropy = REAL_MAX;
   int minDist = instanceCount/2;  // distance from center (equal split)

   FLOG(3,"minSplit = " << minSplit
	   << ", num instances = " << instancesNum
	   << ", num categories = " << categoriesNum << endl);

   for (i = 0; i < instanceCount - 1; i++) {
      const attrLabelPair& alp = alpArray.index(i);
      const attrLabelPair& alpNext = alpArray.index(i+1);
      int bagIndex = alp.pos;
      const NominalVal lab =labelInfo.get_nominal_val(alp.label);

      DBGSLOW(ASSERT(bagset[alp.bag]); 
              ASSERT(bagset[alp.bag]->label_info().
		     cast_to_nominal() == labelInfo);
              ASSERT(labelInfo.get_nominal_val(alpNext.label) >
                     UNKNOWN_NOMINAL_VAL));
      ++ac.index(bagIndex*3 + LEFT_NODE);
      --ac.index(bagIndex*3 + RIGHT_NODE);
      ++vc(lab, bagIndex*3 + LEFT_NODE);
      --vc(lab, bagIndex*3 + RIGHT_NODE);

      // Calculate the entropy only when at least one of the bag has more than
      // minSPlit instances in both left and right bag.
      if (alp.attrVal < alpNext.attrVal -
	  REAL_EPSILON && check_min_split(ac, minSplit, bagNum)) {
         Real thresholdVal = (alp.attrVal + alpNext.attrVal) / 2;
         Real condEntropy = cond_entropy(logOptions, vc,ac,instancesNum);
         FLOG(4,"threshold = " << thresholdVal
                 << ", condEntropy = " << condEntropy << endl);
	 FLOG(5,"ac = " << ac << endl);
	 FLOG(5,"vc = " << vc << endl);

         // if it's better or if it's equal by more towards the center, prefer it.
         if (condEntropy < minCondEntropy - REAL_EPSILON ||
            (fabs(condEntropy - minCondEntropy) < REAL_EPSILON &&
	     abs(instanceCount/2 - i) < minDist)) {
            minCondEntropy = condEntropy;
	    minDist = abs(instanceCount/2 - i);
            bestThreshold = thresholdVal;
         }
      }
   }

   if (minCondEntropy < REAL_MAX) {
      FLOG(3,"best threshold = " << bestThreshold
              << ", best min cond entropy = " << minCondEntropy << endl);
      return TRUE;
   }
   else {
      FLOG(3,"no split" << endl);
      return FALSE;
   }
}

