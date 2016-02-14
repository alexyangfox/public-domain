// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Holte's One-R discretization method.
                 This class performs discretization of Real Attribute values
		   by defining a finite number of intervals based upon
		   the attribute values and labels of instances.
		 A more thorough description of the below discretization method
		   can be found in "Very Simple Classification Rules Perform"
		   "Well on Most Commonly Used Datasets" by Robert C. Holte.
		   "Machine Learning" Vol 11, 1993 pages 63-90
                 We begin scanning the sorted attribute values from low to
		 high. If the current majority label for the accumulated
		 interval does not agree with the majority label of the set
		 of instances having the next attribute value, we create a
		 threshold  between them.  Exceptions are:
                    a) we never create a threshold if there is no strict
		      majority because of a tie.
                    b) we never create a threshold if there aren't at least
                       MIN_INST instances in the majority category.

		 Example:

		 A A A A B A B B C ... <---- Labels
		 0 0 0 1 1 1 1 1 2 ... <---- Attribute Values

		 Now if we simply scanned from left to right, stopping when
		   (i) the current attr val is not the same as the next one,
		   (ii) we have small in the majority, (iii) we don't have a
		   tie, and (iv) the label of the next instance is not the
		   same as the current majority, the above would get into
		   a pathological situation where it would scan all of the
		   0's, try to break at the A:1 instance, but the label
		   would be the same as the current majority label so it would
		   include all of the 1's even though the majority category
		   of that set of attrvalues is B.
		 Our method avoids this by looking at majorities of groups of
		   instances having the same attribute values.
                 In order to obtain a set of OneR discretizors which you can
		   use to discretize a bag, use the function
		   create_OneR_discretizors() and the auxiliary function
		   discretize_bag() to discretize the bag with the
		   discretizors (see RealDiscretizor.c).
		   
  Assumptions: 
  Comments     : 1R discretization requires all intervals (except the right
                   most) to contain more than a predefined number of examples
		   of the same class.
		 The calculation of the majority, tie conditions and index
		   of the label having the majority is greatly facilitated
		   by the LabelValueCounts class.
                 LabelValueCounts is an Array class which will always know 
                   what its maximum value is and can  tell you  whether or
		   not there are two or more elements which have the same
		   value (a tie).
		 The method increment(i) will update the value of element i
		   by one (1) and recalculate the maximum and check whether
		   there is a tie. Users must use this method in order to
		   update values in the array if they would like is_tie()
		   and max() to work correctly. It is used by the OneR
		   discretizor to facilitate creation of the thresholds array.
	         The basic algorithm for OneR discretization can be found
		    in OneR::create_thresholds() and it works as so:
		    a) We are given a schema with an attrNum corresponding
		       to the attribute to be discretized. The RealDiscretizor
		       constructor gives us the RealAttrInfo corresponding to
		       this attribute. (See RealDiscretizor.c) 
                    b) Loop over all the instances in the  attribute and
		       label array which have the same attribute value by
		       calling the static function get_lvc().
		       We keep updating the labelValuecounts until the
		       attribute value changes. Usually this is one but for
		       datasets like SO where there are repeated attribute
		       values, this can be more than small or MIN_INST in
		       the lvc. This makes up the inner loop.
		    c) After obtaining the label value counts for the last
		       majority, we check the accumulated majority (sumLVC)
		       and see whether:
		         (i) The current majority of attribute values is
			     the same as the existing majority
			 (ii) There is a tie between the majority we just
			     found (via get_lvc()) and the existing majority
			     (sumLVC).
		         (iii) The majority value is less than or equal to
			     small.
			 If any of the above 3 conditions are true, we merge
			 the label value counts (add them) and go back to
			 step b) then c) otherwise we do step d).
		      d) If we have at least small in the accumulated majority
		         of label value counts, we create a new threshold at
			 the midpoint between the highest attribute value in
			 sumLVC and the (only) value in lvc.
  
  Complexity   : LabelValueCounts::increment() takes O(numLabels)

                  OneR::create_thresholds() takes O(n Log n + nl) where n is
		   the number of instances and l is the number of label values
		   
		 create_OneR_discretizors() takes O(m * (n Log n + nl)) where
                   m is the number of attributes, n is the number of
		   instances, and l is the number of label values.
		 
  Enhancements :
  History      : Added COMBINE_INTERVALS as #ifdef.  Doesn't seem
                   to help C4.5 nor Naive-Bayes in preliminary tests
                 James Dougherty                                   12/11/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <OneR.h>
#include <Array.h>
#include <Attribute.h>

RCSID("MLC++, $RCSfile: OneR.c,v $ $Revision: 1.9 $")



//Initialize the minimum number of instances per label to the default
// value from Holte's paper.
int OneR::defaultSmall = 6;

/***************************************************************************
  Description : Scans all instances, inserting values into the attribute and
                  label pair array, skipping unknowns and then truncating the
		  array and performing a sort. Complexity: O(nLogn) (it
		  takes time to quick sort, which is O(n^2) worst case, but
		  O(nLogn) average case)
  Comments    : 
***************************************************************************/
static void initialize(const InstanceBag& sourceBag,
		       DynamicArray<AttrAndLabel>& attrAndLabel,
   		       const RealAttrInfo& rai,
		       int attrNum)
{
   int i = 0;
   for ( Pix pix = sourceBag.first(); pix; sourceBag.next(pix)) {
      const InstanceRC& inst = sourceBag.get_instance(pix);
      if(!rai.is_unknown(inst[attrNum])){
	 attrAndLabel.index(i).attrVal = rai.get_real_val(inst[attrNum]);
	 attrAndLabel.index(i).label =
	    inst.nominal_label_info().get_nominal_val(inst.get_label());
	 i++;
      }
   }
   attrAndLabel.truncate(i);
   attrAndLabel.sort();
}


/***************************************************************************
  Description : Resets the LabelValueCounts and then increments the label
                  value count for the current Attribute, stopping when the
		  attribute value changes in the AttributeAndLabelArray.
		  Updates the Attribute Number for the next pass.
  Comments    :
***************************************************************************/
static void get_lvc(int& attrValNum,
		    LabelValueCounts& labelValCounts,
		    const DynamicArray<AttrAndLabel>& aal)
{
   labelValCounts.reset(); //zero out label-value counts
   labelValCounts.increment(aal[attrValNum++].label);
   while(attrValNum < aal.size() &&
	 aal[attrValNum].attrVal == aal[attrValNum -1].attrVal){
      labelValCounts.increment(aal[attrValNum++].label);
   }
}


/***************************************************************************
  Description : Default Constructor
  Comments    :
***************************************************************************/
LabelValueCounts::LabelValueCounts(int size)
   :Array<Category>(size)
{
   reset();
}


/***************************************************************************
  Description : Increments the value of the Array element at
                  index, calculates the new majority value and its index,
		  whether or not there is a tie with this majority and if so,
		  the index (Category) of the element which ties with it.
    Comments    :
***************************************************************************/
void LabelValueCounts::increment(Category index)
{
   elements[index]++;
   //find the max value and its index 
   if (elements[index] > maxValue){
      maxValue = elements[index];
      maxCategory = index;
      tie = FALSE;
   } else if (elements[index] == maxValue){
      tie = TRUE;
   } // else
     // dont touch tie
}


/***************************************************************************
  Description : Resets the label value counts and state variables.
  Comments    :
***************************************************************************/
void LabelValueCounts::reset()
{
   init_values(0);
   maxValue = 0;
   tie = TRUE;
   maxCategory = UNKNOWN_CATEGORY_VAL;
}


/***************************************************************************
  Description : Returns the majority value of the LabelValCounts array. Aborts
                  if there is a tie.
  Comments    :
***************************************************************************/
int LabelValueCounts::majority_value() const
{
   if (tied_majority())
      err << "LabelValueCounts::majority_value: it is an error to invoke this "
	  << "method when there is a tie." << fatal_error;
   return maxValue;
}


/***************************************************************************
  Description : Returns the Category having the majority value. Aborts if
                  there is a tie.
  Comments    :
***************************************************************************/
Category LabelValueCounts::majority_category() const
{
   if (tied_majority())
      err << "LabelValueCounts::majority_category: it is an error to invoke "
	  << "this method when there is a tie." << fatal_error;
   return maxCategory;
}


/***************************************************************************
  Description : operator += for LabelValueCounts. Not defined unless 
                 both the source and destination have the same size.
  Comments    : 
***************************************************************************/
LabelValueCounts& LabelValueCounts::operator+=(const LabelValueCounts& source)
{
   //Invoke base operator += 
   Array<Category>::operator+=(source);

   //calculate the merged maximum and it's index
   maxValue= max(maxCategory);

   //find out whether we have a tie in the new merged array
   tie = FALSE;
   for(int j = maxCategory + 1; j < size(); j++)  // start at maxCategory +1
      if(index(j) == maxValue)                    // since max prefers earlier
	 tie = TRUE;                              // elements during tiebreaker
   return *this;
}


/***************************************************************************
  Description : Builds the thresholds array for the discretizor. See
                  description above.
  Comments    : Protected, pure virtual
***************************************************************************/
void OneR::build_thresholds(const InstanceBag& sourceBag)
{
   //Note: check_thresholds called already
   LabelValueCounts lvc(sourceBag.num_label_values());    // initialized to 0
   LabelValueCounts sumLVC(sourceBag.num_label_values()); // initialized to 0
   int saveAttrValNum, attrValNum = 0;

   //We initialize the array to be of size num_instances to speed things
   //up, but we want the dynamic ability in order to keep the size since
   //we are truncating because of unknowns in initialize()
   DynamicArray<AttrAndLabel> aal(sourceBag.num_instances());

   thresholds = new DynamicArray<RealAttrValue_>(0);
   initialize(sourceBag, aal, *rai,attrNum); //set up the Attr and label array
#  ifdef COMBINE_INTERVALS
   Category lastMajority = -1;
#  endif

   while( attrValNum < aal.high()){
      ASSERT(aal.size() -1 == aal.high());
      saveAttrValNum = attrValNum;
      // attrValNum gets updated and the lvc gets its counts updated from aal
      get_lvc(attrValNum, lvc, aal);

      if (lvc.tied_majority() || sumLVC.tied_majority() ||
	  lvc.majority_category() == sumLVC.majority_category() ||
	  sumLVC.majority_value() <= small) {
	 sumLVC += lvc; //sumLVC gets the old labelValCounts
      }
     else{
	ASSERT(sumLVC.majority_value() >=small);
	ASSERT(saveAttrValNum > 0 && saveAttrValNum <= aal.high());
	Real thresholdVal = (aal[saveAttrValNum].attrVal
			     + aal[saveAttrValNum - 1].attrVal) / 2;
#       ifdef COMBINE_INTERVALS
        // Combine adjacent intervals with same majority
        if (sumLVC.majority_category() == lastMajority) {
           ASSERT(thresholds->size() > 0);
           // override previous one
	   rai->set_real_val(thresholds->index(thresholds->size()-1),
			     thresholdVal);
	} else
	   rai->set_real_val(thresholds->index(thresholds->size()),
			     thresholdVal);
	lastMajority = sumLVC.majority_category();
#       else
	rai->set_real_val(thresholds->index(thresholds->size()),
			     thresholdVal);

#       endif
	sumLVC = lvc;
     }
   }
   LOG(2, "Created " << thresholds->size() << " thresholds for"
                     << " attribute " << rai->name() << endl);
   DBG(OK());
}


/***************************************************************************
  Description : Constructor.
  Comments    : 
***************************************************************************/
OneR::OneR(int attrNum, const SchemaRC& schema, int smallValue)
   :RealDiscretizor(attrNum, schema),
    small(smallValue)
{
   if(small <= 0)
      err << "OneR::OneR: Minimum number of Instances per label must "
	  << "be greater than 0. Value given is " << small << fatal_error;
}


/***************************************************************************
  Description : Copy constructor
  Comments    :
***************************************************************************/
OneR::OneR(const OneR& source, CtorDummy  dummyArg)
   :RealDiscretizor(source, dummyArg),
    small(source.small)
{}


/***************************************************************************
  Description : operator == 
  Comments    :
***************************************************************************/
Bool OneR::operator == (const RealDiscretizor& source)
{
   if (class_id() == source.class_id())
      return (*this) == (const OneR&)source;
   return FALSE;
}

Bool OneR::operator == (const OneR& source)
{
   return equal_shallow(source) && small == source.small;
}


/***************************************************************************
  Description : Returns a deep copy of the object.
  Comments    :
***************************************************************************/
RealDiscretizor* OneR::copy()
{
   return new OneR(*this, ctorDummy);
}


/***************************************************************************
  Description : Returns an Array<RealDiscretizor*> of OneR* using the
                  small values in the Array.
                  Used to generate the discretized Schema and Bag in
		  conjunction with discretize_bag() (see RealDiscretizor.c)
  Comments    :
***************************************************************************/
PtrArray<RealDiscretizor*>* create_OneR_discretizors(LogOptions& logOptions,
						     const InstanceBag& bag,
						     const Array<int>& small)
{
   SchemaRC schema = bag.get_schema();
   PtrArray<RealDiscretizor*>* ord  =
      new PtrArray<RealDiscretizor*>(schema.num_attr());
   for(int k = 0; k < schema.num_attr(); k++){
      const AttrInfo& ai = schema.attr_info(k);
      if ( ai.can_cast_to_nominal() )
	 ord->index(k) = NULL;
      else if ( ai.can_cast_to_real()){
	 OneR* oneRDisc = new OneR(k, schema, small.index(k));
	 oneRDisc->set_log_options(logOptions);
	 oneRDisc->set_log_level(max(0, logOptions.get_log_level() -1));
	 ord->index(k) = oneRDisc;
	 ord->index(k)->create_thresholds(bag);
      }
      else 
	 err << "Unrecognized attribute type" << fatal_error;
   }
   return ord;
}


/***************************************************************************
  Description : Returns an Array<RealDiscretizor*> of OneR*
                  used to generate the discretized Schema and Bag. Used in
		  conjunction with discretize_bag() (see RealDiscretizor.c)
  Comments    : Non-array version, use this when you have to generate a
                  set of Holte discretizors with the same small value.
***************************************************************************/
PtrArray<RealDiscretizor*>* create_OneR_discretizors(LogOptions& logOptions,
						     const InstanceBag& bag,
						     int small)
{
   Array<int> smallValues(0, bag.get_schema().num_attr(), small);
   return create_OneR_discretizors(logOptions, bag, smallValues);
}

