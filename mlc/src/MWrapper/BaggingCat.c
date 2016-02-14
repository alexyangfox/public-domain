// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper categorizer which holds an ordered Set of child
                   categorizers and their weights. BaggingCat categorizes
		   an instance by voting through each categorizer weighted
		   by its weight value.
  Assumptions  : The number of categories among categorizers should
                   be the same.
  Comments     : 
  Complexity   :
  Enhancements : 
  History      : Yeogirl Yun, Robert Allen                          5/4/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <assert.h>
#include <BaggingCat.h>

RCSID("MLC++, $RCSfile: BaggingCat.c,v $ $Revision: 1.13 $")

Bool BaggingCat::defaultUseAboveAvgWeight = FALSE;

/***************************************************************************
  Description :  Check all the categorizers in set.
  Comments    : 
***************************************************************************/
void BaggingCat::OK(int level) const
{
   for (int i=0; i < catSet->size(); i++) {
      if (catSet->index(i) == NULL)
	 err << "BaggingCat::OK: Categorizer in set at index #" << i
	     << " is null" << fatal_error;
         catSet->index(i)->OK(level);
   }

   if (catSet->size() != weightSet->size())
      err << "BaggingCat::OK: the number of category set(" << catSet->size()
	 << ") does not match the number of weight set(" <<
	 weightSet->size() << "). " << fatal_error;
}


/***************************************************************************
  Description : Helper function for constructor to get size of categorizer.
                It asserts the same number of categories among
		  categoriezers.
  Comments    :
***************************************************************************/
static int num_cat(PtrArray<Categorizer *>*& cats)
{
   if (cats->size() <= 0)
      err << "BagingCat::BaggingCat: illegal number of categorizers : "
	  << cats->size() << fatal_error;

   if (cats->index(0) == NULL)
      err << "BaggingCat::BaggingCat: Categorizer in set at index #" << 0
	     << " is null" << fatal_error;
   int numCat = cats->index(0)->num_categories();

   for (int i=1; i < cats->size(); i++) {
      if (cats->index(i) == NULL)
	 err << "BaggingCat::BaggingCat: Categorizer in set at index #" << i
	     << " is null" << fatal_error;
      if (cats->index(i)->num_categories() != numCat)
	 err << "BaggingCat::BaggingCat: different number of categories "
	    " for Bagging categorizers" << fatal_error;
   }
   return numCat;
}  
      

/***************************************************************************
  Description : Initializes the BaggingCat.  
  Comments    : Owns the set of categorizers and weights passed in.
***************************************************************************/
BaggingCat::BaggingCat(const MString& dscr,
		       PtrArray<Categorizer *>*& cats,
		       Array<Real>*& weights)
   : Categorizer(num_cat(cats), dscr),
     numCorrect(0, weights->size(), 0),
     numCategorize(0)    
{
   if (weights->size() != cats->size())
      err << "BaggingCat::BaggingCat: the size of weights (" <<
	 weights->size() << ") and categories (" << cats->size() <<
	 ") are not the same" << fatal_error;
   
   catSet = cats;
   cats = NULL;			// take ownership
   weightSet = weights;
   weights = NULL;		// take ownership

   useAboveAvgWeight = defaultUseAboveAvgWeight;
   avgWeight = 0;
   for (int i = 0; i < weightSet->size(); i++) 
      avgWeight += weightSet->index(i);
   avgWeight /= weightSet->size();

   DBG(OK());
}


/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : Making a copy of all the categorizers pointed to by
                    the catSet array is required since multiple ownership
		    must be avoided.  Time and space requirements may be high
		    and depend on the type of wrapped categorizers.
***************************************************************************/
BaggingCat::BaggingCat(const BaggingCat& source,
		       const CtorDummy )
   : Categorizer(source, ctorDummy),
     numCorrect(0, source.numCorrect.size(), 0),     
     numCategorize(source.numCategorize),
     useAboveAvgWeight(source.useAboveAvgWeight),
     avgWeight(source.avgWeight)
{
   int setSize = source.get_categorizer_set().size();
   catSet = new PtrArray<Categorizer *>(setSize);
   weightSet = new Array<Real>(*source.weightSet, ctorDummy);

   for (int i=0; i < setSize; i++) {
      catSet->index(i) = source.get_categorizer_set().index(i)->copy();
      weightSet->index(i) = source.weightSet->index(i);
   }
   DBG(OK());   
}


/***************************************************************************
  Description : Destructor
  Comments    : 
***************************************************************************/
BaggingCat::~BaggingCat()
{
   DBGSLOW(OK());
   IFLOG(2,
     if (numCategorize > 0) // avoid dividing by zero
        if (debugLevel > 0)
           get_log_stream() << 
              "No statistics on BaggingCat because debugLevel > 0" << endl;
        else {
           get_log_stream() << "Statistics of classifiers on test set" << endl;
           for (int i = 0; i < numCorrect.size(); i++)
   	  get_log_stream() << "Accuracy of classifier " << i << " is "
   	                   << Real(numCorrect.index(i))/numCategorize << endl;
        });
     
   delete catSet;
   delete weightSet;   
}


/***************************************************************************
  Description : Categorize by voting with weights for the instance by each
                  categorizer.
  Comments    : 
***************************************************************************/
AugCategory BaggingCat::categorize(const InstanceRC& inst) const
{
   DBGSLOW(OK());

   Array<Real> voteArray(UNKNOWN_CATEGORY_VAL, num_categories() + 1, 0.0);

   LOG(3, " caetegory size : " << catSet->size() << endl);
   LOG(3, " weight set size : " << weightSet->size() << endl);
   LOG(3, " vote array size : " << voteArray.size() << endl);
   for (int i=0; i < catSet->size(); i++) {
      LOG(3, " current weight : " << weightSet->index(i) <<
	  "avg weight : " << avgWeight << endl);
      if (!get_use_above_avg_weight() ||
	  weightSet->index(i) >= avgWeight) {
	 Category cat = catSet->index(i)->categorize(inst);
	 LOG(3, " categorized value using " << i << "th categorizer :"  << cat
	     << endl);
	 voteArray[catSet-> index(i)->categorize(inst)] +=
	    weightSet->index(i);
	 IFLOG(2, 
	       Category correctCat =
	       inst.label_info().get_nominal_val(inst.get_label());
	       if (correctCat != UNKNOWN_CATEGORY_VAL && cat == correctCat)
	       // cast because of logical constness (statistics)
	       ((BaggingCat *)this)->numCorrect[i]++);
      }
      else
	 LOG(3, "skipping test due to the option of useAboveAvgWeight"
	     << endl);
   }

   ((BaggingCat *)this)->numCategorize++; // Cast for statistics
   Category finalCat;
   voteArray.max(finalCat);
   finalCat += UNKNOWN_CATEGORY_VAL;

   MStringRC categoryString =
      inst.get_schema().category_to_label_string(finalCat);
   
   AugCategory augCat(finalCat, categoryString);
   LOG(3, " Returning the category : " << categoryString << endl);

   return augCat;
}


/***************************************************************************
  Description : Prints a readable representation of the categorizer to the
                  given stream.
  Comments    : 
***************************************************************************/
void BaggingCat::display_struct(MLCOStream& stream,
				const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::ASCIIDisplay) {
      stream << "Bagging Categorizer: " << description() << endl
	     << "   Using Categorizers: "  << endl;

      for (int i=0; i < catSet->size(); i++) {
	 catSet->index(i)->display_struct(stream, dp);
	 stream << " Weight : " << weightSet->index(i) << endl;
      }
	 
      stream << endl << endl;
   }
}


/***************************************************************************
  Description : Allows operator << to be used on this class through macros.
  Comments    :
***************************************************************************/
void BaggingCat::display(MLCOStream& stream, const DisplayPref& dp) const
{
   display_struct(stream, dp);
}

/***************************************************************************
  Description : Returns a pointer to a deep copy of this BaggingCat.
  Comments    : Uses copy constructor - makes copy of all wrapped categorizers.
***************************************************************************/
Categorizer* BaggingCat::copy() const
{
   return new BaggingCat(*this, ctorDummy);
}


/***************************************************************************
  Description : Checks the argument categorizer to make sure its the same
                  type as this.  If so, use the specific operator ==.
  Comments    : Relies on unique categorizer #define in Categorizer.h
***************************************************************************/
Bool BaggingCat::operator==(const Categorizer &cat) const
{
   if ( class_id() == cat.class_id() )
      return (*this) == (const BaggingCat &) cat;
   return FALSE;
}

/***************************************************************************
  Description : Compares all elements of class for equality.
  Comments    :
***************************************************************************/
Bool BaggingCat::operator==(const BaggingCat &cat) const
{
   return (*catSet == *(cat.catSet) &&
	   *weightSet == *(cat.weightSet));
}


DEF_DISPLAY(BaggingCat);





