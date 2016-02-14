// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

// This is an include file.  For a full description of the class and
// functions, see the file name with a "c" suffix.

#ifndef _entropy_h
#define _entropy_h 1

#include <CtrBag.h>
#include <LogOptions.h>

// Note that this should theoretically be declared as a private class
// to entropy.c, but because we generate an array of this type, the
// template mechanism fails.
// operator<, operator= only compare the real value.
// bag and pos are used only in real_mutual_info.
class attrLabelPair {
public:   
   int pos;  // pos is the bag number where the instance goes to.
   int bag;  // bag is the bag where the instance comes from.
   Real attrVal;
   AttrValue_ label;
   int operator<(const attrLabelPair& item2) const
      {return (attrVal < item2.attrVal);}
   int operator==(const attrLabelPair& item2) const
      {return (attrVal == item2.attrVal);}
};


// Entropy H(Y) for the label Y
Real entropy(const LogOptions& , const CtrInstanceBag&);
Real entropy(const LogOptions& , const Array<int>& labelCount,
	     int   numTotalInstances);

// Entropy weighted by the parent.  Used in LazyDT.
// Confidence interval gives a pessimistic estimate shifted towards
//   p = 1/num-classes.
Real weighted_entropy(const LogOptions& logOptions, 
		      const CtrInstanceBag& bag,
		      const CtrInstanceBag& parentBag,
		      Real confIntervalZ = 0);
Real weighted_entropy(const LogOptions&,
		      const Array<int>& labelCount, 
		      const Array<int>& parentLabelCount, 
		      int numTotalInstances,
		      int parentNumInstances, Real confIntervalZ = 0);


// Gini weighted by the parent.  Used in LazyDT.
// Confidence interval gives a pessimistic estimate shifted towards
//   p = 1/num-classes.
Real weighted_gini(const LogOptions& logOptions, 
		      const CtrInstanceBag& bag,
		      const CtrInstanceBag& parentBag,
		      Real confIntervalZ = 0);
Real weighted_gini(const LogOptions&,
		      const Array<int>& labelCount, 
		      const Array<int>& parentLabelCount, 
		      int numTotalInstances,
		      int parentNumInstances, Real confIntervalZ = 0);


Array2<int>* build_vc(const LogOptions&,
			    const Array<const CtrInstanceBag*> &currentLevel,
			    int attrNum);
Array<int>* build_ac(const LogOptions&,
			   const Array<const CtrInstanceBag*> &currentLevel,
			   int attrNum);

// Conditional entropy H(Y|X) for the label Y
Real cond_entropy(const LogOptions&,
		  const CtrInstanceBag&, int attrNumX);
Real cond_entropy(const LogOptions&,
		  const Array2<int>& vc, const Array<int>& ac,
                  int numTotalInstances);

Real j_measure(const LogOptions& logOptions,
	       const CtrInstanceBag& bag, int attrNumX, NominalVal x);
Real j_measure(const LogOptions&,
		  const Array2<int>& vc, const Array<int>& ac,
                  const Array<int>& labelCounts, NominalVal x, 
	          int numTotalInstances);

Real two_way_cond_entropy(const LogOptions&,
			  const CtrInstanceBag&, int attrNum, NominalVal val);
Real two_way_cond_entropy(const LogOptions&,
			  const Array2<int>& vc, const Array<int>& ac,
                          int numTotalInstances, NominalVal val);

// Mutual information I(Y;X) = H(Y) - H(Y|X) for label Y, attribute X
// Mutual information is called "gain" by some researches like Quinlan.
Real mutual_info(const LogOptions&,
		 const CtrInstanceBag&, int attrNumX);
Real mutual_info(const LogOptions&,
		 const Array2<int>& vc, const Array<int>& ac,
                 const Array<int>& lc,  int numTotalInstances);

// Caculate the minSplit.
int min_split(const LogOptions&,
	      const CtrInstanceBag &bag,
	      int lowerBoundMinSplit, int upperBoundMinSplit,
	      Real minSplitPercent);

// Returns TRUE iff split was found.
Bool real_cond_entropy(const LogOptions& logOptions,
		       const CtrInstanceBag& bag, int attrNumX,
		       Real& threshold, Real& minCondEntropy, int minSplit);

Bool real_cond_entropy(const LogOptions& logOptions,
		       const Array<const CtrInstanceBag*> &currentBag,
		       int attrNumX,
		       Real& bestThreshold,
		       Real& minCondEntropy,
		       int minSplit, int instanceNum );

#endif
