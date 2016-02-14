// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Entropy Discretizor. This class discretizes real attributes
                   by using the conditional entropy. 
		   The greatest increase in information gain of  a single
		   threshold split is chosen each time for the best
		   decrease in entropy.
		 If minSplit == 0, a Multi-Interval Discretization technique
                   is used which uses the MDLP (Minimum Description Length
		   Principle) in order to determine the halting criteria (i.e.
		   of when to stop splitting the bags).
		 For a more complete description of this technique, see
		   "Multi-Interval Discretization of Continuous-Valued"
		   "Attributes for Classification Learning" Fayyad/Irani
		   IJCAI 93

  Assumptions  :
  Comments     : 
  Complexity   :
  Enhancements :
  History      : James Dougherty                                    12/28/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <math.h>
#include <entropy.h>
#include <EntropyDisc.h>
#include <ThresholdCat.h>
#include <LogOptions.h>

RCSID("MLC++, $RCSfile: EntropyDisc.c,v $ $Revision: 1.14 $")

// Initialize the default minSplit to 1
int EntropyDiscretizor::defaultMinSplit = 1;

//forward decl for build_thresholds
int max(const DynamicArray<BagAndBestSplitEntropy*>& babse);

/***************************************************************************
  Description : BagAndBestSplitEntropy constructor. 
  Comments    :
***************************************************************************/
BagAndBestSplitEntropy::BagAndBestSplitEntropy(const LogOptions& logOptions,
					       CtrInstanceBag*& newBag,
					       int attrNum, int minSplit)
   : bag(NULL),
     bestSplit(REAL_MAX),
     bestGain(-REAL_MAX)
{
   LogOptions llogOptions;
   int logLevel = logOptions.get_log_level();
   logOptions.set_log_level(logLevel -1);
   assign_bag(logOptions, newBag, attrNum, minSplit);
   logOptions.set_log_level(logLevel);
}


/***************************************************************************
  Description : Obtains ownership of the bag, computes the entropy and
                  best split. Note that we don't compute the entropy if
		  all the instances in the bag have the same label value,
		  thereby halting the iterative splitting procedure used
		  by create_thresholds().
  Comments    : 
***************************************************************************/
void BagAndBestSplitEntropy::assign_bag(const LogOptions& logOptions,
					CtrInstanceBag*& srcBag,
					int attrNumX, int minSplit)
{
   delete bag;
   bag = srcBag;
   srcBag = NULL;
   bestSplit = REAL_MAX;
   bestGain = -REAL_MAX;
   if (bag->counters().label_num_vals() <= 1)
      return;
   Real bestEntropy;
   Bool splitOK = real_cond_entropy(logOptions, *bag, attrNumX, bestSplit,
				    bestEntropy, minSplit);
   ASSERT(splitOK || bestEntropy == REAL_MAX);
   if (splitOK) {
      Real e = entropy(logOptions, *bag);
      bestGain = e - bestEntropy;
      if (bestGain < 0 && bestGain > -REAL_EPSILON)
	 bestGain = 0;  // gain should never be negative; this accounts
                        // for possible numerical representation errors
      ASSERT(bestGain >= 0);

   }
}


/***************************************************************************
  Description : Returns the CtrInstanceBag after checking that it has been
                  set via assign_bag().
  Comments    :
***************************************************************************/
CtrInstanceBag& BagAndBestSplitEntropy::get_bag() const
{
   if (!bag){
      err << "BagAndBestSplitEntropy::get_bag : assign_bag must be "
	  << "invoked first." << fatal_error;
   }
   return *bag;
}


/***************************************************************************
  Description : Builds the thresholds array. 
  Comments    : Protected, pure virtual
***************************************************************************/
void EntropyDiscretizor::build_thresholds(const InstanceBag& sourceBag)
{
   InstanceBag* newBag = sourceBag.clone();
   //remove all of the unknown values              
   newBag->remove_inst_with_unknown_attr(attrNum); 

   //Make a counter bag for the BagAndBestSplit real_cond_entropy()
   //@@ real_cond_entropy should be changed from CtrInstanceList to InstanceBag
   CtrInstanceBag* ctrBag= new CtrInstanceBag(newBag->get_schema());
   for(Pix p = newBag->first(); p; newBag->next(p))
      ctrBag->add_instance(newBag->get_instance(p));
   
   DynamicArray<BagAndBestSplitEntropy*> babse(0);
   DynamicArray<Real> threshVals(0);
   int bestIndex = 0;

   babse.index(bestIndex) = new BagAndBestSplitEntropy(get_log_options(),
						      ctrBag, attrNum,
						      minSplit);
   if( 0 == numIntervals){       //Use MDL technique by Fayyad & Irani
      while (bestIndex < babse.size()) {
	 LOG(2, "Index is " << bestIndex << " Gain is "
	     << babse.index(bestIndex)->best_gain() << endl);
	 if (babse.index(bestIndex)->best_gain() != -REAL_MAX  &&
	     mdl_condition(babse, bestIndex)){

	    Real bestSplit = babse.index(bestIndex)->best_split();
	    ASSERT(REAL_MAX != bestSplit);
	    threshVals.index(threshVals.size()) = bestSplit;
	    split_bag(bestIndex, babse);

	 } else
	    bestIndex++;
      }
   } else{                //use recursive Bag splitting technique
      while (babse.index(bestIndex)->best_gain() > -REAL_MAX
	     && babse.size() < numIntervals) {
	 Real bestSplit = babse.index(bestIndex)->best_split();
	 ASSERT(bestSplit != -REAL_MAX); // sanity check
	 ASSERT(bestSplit != REAL_MAX); // sanity check
	 // new threshold
	 threshVals.index(threshVals.size()) = bestSplit;
	 //split the bag
	 split_bag(bestIndex, babse);
	 //find the index of the BagAndBestSplit with the largest gain
	 bestIndex = max(babse);
      }
   }

   threshVals.sort();    // sort the thresholds first so OK() succeeds
   // Have the RealDiscretizor allocate and copy the thresholds we just made.
   create_real_thresholds(threshVals);
   
   if (num_intervals_chosen() < numIntervals)
      LOG(1, "Unable to create " << numIntervals << " intervals for "
	     << "attribute " << descr << ". Created only "
	     << num_intervals_chosen() << endl);
   else
      LOG(1, "Created " << num_intervals_chosen() << " intervals "
	     << " for attribute " <<  descr << "."  << endl);
   

   for(int c = 0; c < babse.size(); c++)
      delete babse.index(c);
   delete newBag;
   delete ctrBag;
   
   DBG(OK());
}


/***************************************************************************
  Description : EntropyDiscretizor constructor
  Comments    :
***************************************************************************/
EntropyDiscretizor::EntropyDiscretizor(int attrNum, const SchemaRC& schema,
				       int numberOfIntervals, int minSplits)
   :RealDiscretizor(attrNum, schema),
    minSplit(minSplits),
    numIntervals(numberOfIntervals)
{}


/***************************************************************************
  Description : Copy constructor 
  Comments    :
***************************************************************************/
EntropyDiscretizor::EntropyDiscretizor(const EntropyDiscretizor& source,
				       CtorDummy  dummyArg )
   :RealDiscretizor(source, dummyArg)
{
   minSplit = source.minSplit;
   numIntervals = source.numIntervals;
}


/***************************************************************************
  Description : operator == 
  Comments    :
***************************************************************************/
Bool EntropyDiscretizor::operator ==(const RealDiscretizor& src)
{
   if (class_id() == src.class_id())
      return (*this) == (const EntropyDiscretizor&) src;
   return FALSE;
}

Bool EntropyDiscretizor::operator ==(const EntropyDiscretizor& src)
{
   return equal_shallow(src) && minSplit == src.minSplit &&
                            numIntervals == src.numIntervals;
}


/***************************************************************************
  Description : Split's the bag into a left and right and puts the two
                  halves in the BagAndBestSplitEntropy array.
  Comments    :
***************************************************************************/
void EntropyDiscretizor::split_bag(int minIndex,
			          DynamicArray<BagAndBestSplitEntropy*>& babse)
{
   // Create a threshold categorizor to use while splitting the bag
   ThresholdCategorizer
      threshCat(babse.index(minIndex)->get_bag().get_schema(), attrNum,
		babse.index(minIndex)->best_split(), "ThresholdCat");
   
   //Split the bag into 3 (the leftmost has all the unknowns).
   CtrBagPtrArray* splitBags =
      babse.index(minIndex)->get_bag().ctr_split(threshCat);
   
   ASSERT(3 == splitBags->size());     //Unknown, left, right (-1,0,1)
   ASSERT((*splitBags)[UNKNOWN_CATEGORY_VAL]->no_instances());
   
   LOG(2, "Split bag with: left (" <<
       (*splitBags)[FIRST_CATEGORY_VAL]->num_instances() << ") instances"
       << " and right ("
       << (*splitBags)[FIRST_CATEGORY_VAL+1]->num_instances()
       << ") instances" << endl);
   
   // Out with the last best split, put in the new left split
   delete babse.index(minIndex);
   babse.index(minIndex) =
      new BagAndBestSplitEntropy( get_log_options(),
				  (*splitBags)[FIRST_CATEGORY_VAL],
				  attrNum, minSplit);
   // add the rightmost split bag
   babse.index(babse.size()) =
      new BagAndBestSplitEntropy( get_log_options(),
				  (*splitBags)[FIRST_CATEGORY_VAL + 1],
				  attrNum, minSplit);
   delete splitBags;
}


/***************************************************************************
  Description : Returns true if the split satisfies the minimum description
                  length condition.
  Comments    :
***************************************************************************/
Bool EntropyDiscretizor::mdl_condition(DynamicArray<BagAndBestSplitEntropy*>&
				       babse, int minIndex)
{
   LogOptions logOptions;
   logOptions.set_log_level( max(0, get_log_level() - 1));

   Real gain = babse.index(minIndex)->best_gain();
   
   const CtrInstanceBag& orgBag = babse.index(minIndex)->get_bag();
   Real e = entropy(logOptions, orgBag);

   int k = orgBag.counters().label_num_vals();
      
   // Create a threshold categorizor to use while splitting the bag
   ThresholdCategorizer
      threshCat(orgBag.get_schema(), attrNum,
		babse.index(minIndex)->best_split(), "ThresholdCat");
   
   //Split the bag into 3 (the leftmost has all the unknowns).
   CtrBagPtrArray* splitBags =orgBag.ctr_split(threshCat);
   
   ASSERT(3 == splitBags->size());     //Unknown, left, right (-1,0,1)
   ASSERT((*splitBags)[UNKNOWN_CATEGORY_VAL]->no_instances());

   const CtrInstanceBag& ctrBag1 = *((*splitBags)[FIRST_CATEGORY_VAL]);
   Real e1 = entropy(logOptions, ctrBag1);
   int k1 = ctrBag1.counters().label_num_vals();

   const CtrInstanceBag& ctrBag2 = *((*splitBags)[FIRST_CATEGORY_VAL + 1]);
   Real e2 = entropy(logOptions, ctrBag2);
   int k2 = ctrBag2.counters().label_num_vals();

   delete splitBags;
   int n = orgBag.num_instances();

   LOG(2, "gain " << gain << " k=" << k << " n=" << n << endl);
   LOG(2, "left-bag entropy " << e1 << " k1=" << k1 << endl);
   LOG(2, "right-bag entropy " << e2 << " k2=" << k2 << endl);
   
   Real subTerm = k * e - k1 * e1 - k2 * e2;
   Real delta = log_bin(pow(3,k) - 2) - subTerm;
   Real condition = log_bin(n - 1) / n  + delta / n; 
      
   if (gain > condition)
      return TRUE;
   return FALSE;
}

/***************************************************************************
  Description : Returns the max on a DynamicArray<BagAndBestSplitEntropy*>
  Comments    :
***************************************************************************/
int max(const DynamicArray<BagAndBestSplitEntropy*>& babse)
{
  if (babse.size() == 0)
      err << "max(const DynamicArray<BagAndBestSplitEntropy*>&)"
	  << "- empty array" << fatal_error;

  const BagAndBestSplitEntropy* max = babse.index(0);
   int idx = 0;
   for (int i = 1; i < babse.size(); i++)
      if (*babse.index(i) > *max) {
         max = babse.index(i);
         idx = i;
      }
   return idx;
}


/***************************************************************************
  Description : Makes a clone of the object.
  Comments    :
***************************************************************************/
RealDiscretizor* EntropyDiscretizor::copy()
{
   RealDiscretizor * nd = new EntropyDiscretizor(*this, ctorDummy);
   DBG(ASSERT(nd != NULL));
   return nd;
}


/***************************************************************************
  Description :  Returns an Array<RealDiscretizor*> of EntropyDiscretizor*
                  using the array of numIntervals in order to generate the
		  discretized Schema and Bag. This routine can be used in
		  conjunction with discretize_bag(). (see RealDiscretizor.c)
  Comments    : 
***************************************************************************/
PtrArray<RealDiscretizor*>*
create_Entropy_discretizors(LogOptions& logOptions,
			    const InstanceBag& bag,
			    int minSplit,
			    const Array<int>& numInterVals)
{
   SchemaRC schema = bag.get_schema();
   PtrArray<RealDiscretizor*>* ed
      = new PtrArray<RealDiscretizor*>(schema.num_attr());
   
   for(int k = 0; k < schema.num_attr(); k++){
      const AttrInfo& ai = schema.attr_info(k);
      if ( ai.can_cast_to_nominal() )
	 ed->index(k) = NULL;
      else if ( ai.can_cast_to_real()){
	 EntropyDiscretizor* entropyDisc =
	    new EntropyDiscretizor(k, schema, numInterVals.index(k), minSplit);
	 entropyDisc->set_log_options(logOptions);
	 entropyDisc->set_log_level(max(0, logOptions.get_log_level() - 1));
	 ed->index(k) = entropyDisc;
	 ed->index(k)->create_thresholds(bag);
      }
      else 
	 err << "Unrecognized attribute type" << fatal_error;
   }
   return ed;
}


/***************************************************************************
   Description : Returns an Array<RealDiscretizor*> of EntropyDiscretizor*
                  used to generate the discretized Schema and Bag. Used in
		  conjunction with discretize_bag(). (see RealDiscretizor.c)
  Comments    : Non Array version, use this if the discretization 
                  requires all RealDiscretizor's to have the same number
		  of intervals.
***************************************************************************/
PtrArray<RealDiscretizor*>* create_Entropy_discretizors(LogOptions& logOptions,
						     const InstanceBag& bag,
						     int minSplit,
						     int numInterVals)
{
   Array<int> intervals(0, bag.get_schema().num_attr(), numInterVals);
   return create_Entropy_discretizors(logOptions, bag, minSplit, intervals);
}
