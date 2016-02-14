// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Categorize an instance by comparing the value of a single
		   attribute to a threshold value.  All instances for which
		   the attribute is less than or equal to the threshold value
		   are put into one category and those for which the attribute
		   is greater than the threshold value are put into a second
		   category.
  Assumptions  : The attribute must be Real.
  Comments     : 
  Complexity   : Constructor takes O(num-attributes) because of attr_info
                   (should be improved to const-time).
                 Categorize() takes constant time.
  Enhancements : This could be extended to categorize any linear attributes
		   rather than only real attributes.
  History      : Chia-Hsin Li                                      11/23/94
                   Add operator==
                 Brian Frasca                                       4/12/94
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <ThresholdCat.h>

RCSID("MLC++, $RCSfile: ThresholdCat.c,v $ $Revision: 1.17 $")

/***************************************************************************
  Description : Construct a ThresholdCategorizer.
  Comments    : There are always two possible categories for a Threshold
		  Categorizer (indicating which side of the threshold a
                  given instance falls).
***************************************************************************/
ThresholdCategorizer::ThresholdCategorizer(const SchemaRC& schema,
                                           int attributeNum,
                                           Real threshold,
                                           const MString& dscr)
   : Categorizer(2, dscr), attrInfo(schema.attr_info(attributeNum)),
     attrNum(attributeNum), thresholdVal(threshold)
{
   LTEDscr = "<= " + MString(thresholdVal,0);
   GTDscr =  "> " + MString(thresholdVal,0);
}


ThresholdCategorizer::ThresholdCategorizer(const AttrInfo& ai,
					   int attributeNum,
                                           Real threshold,
                                           const MString& dscr)
   : Categorizer(2, dscr), attrInfo(ai), 
     attrNum(attributeNum), thresholdVal(threshold)
{
   LTEDscr = "<= " + MString(thresholdVal,0);
   GTDscr =  "> " + MString(thresholdVal,0);
}


/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : We do not use the standard issue copy constructor because
                     we want to know exactly when we call the copy
		     constructor.  
***************************************************************************/
ThresholdCategorizer::ThresholdCategorizer(const ThresholdCategorizer& source,
				           CtorDummy)
   : Categorizer(source, ctorDummy), attrInfo(source.attrInfo),
     attrNum(source.attrNum), thresholdVal(source.thresholdVal), 
     LTEDscr(source.LTEDscr), 
     GTDscr(source.GTDscr)
{
}


/***************************************************************************
  Description : Categorize an instance.
  Comments    : Makes sure the AttributeInfo of the given instances matches
                  the one we have.
***************************************************************************/
AugCategory ThresholdCategorizer::categorize(const InstanceRC& instance) const
{
   static MString unknownReal("?");
   DBG(attrInfo.equal(instance.attr_info(attrNum), TRUE));
   if (attrInfo.is_unknown(instance[attrNum]))
      return AugCategory(UNKNOWN_CATEGORY_VAL, unknownReal);
   else if (attrInfo.get_real_val(instance[attrNum]) <= thresholdVal)
      // Do not use ternary operator here.  It counts as an assignment and the
      // lucid compiler complained
      return AugCategory(FIRST_CATEGORY_VAL, LTEDscr);
   else
      return AugCategory(FIRST_CATEGORY_VAL+1, GTDscr);
}


/***************************************************************************
  Description : Prints a readable representation of the Categorizer to the
                  given stream.
  Comments    :
***************************************************************************/
void ThresholdCategorizer::display_struct(MLCOStream& stream,
                                          const DisplayPref& dp) const
{
   if (stream.output_type() == XStream)
      err << "ThresholdCategorizer::display_struct: Xstream is not a "
          << "supported stream."  << fatal_error;

   if (dp.preference_type() != DisplayPref::ASCIIDisplay)
      err << "ThresholdCategorizer::display_struct: Only ASCIIDisplay is "
          << "supported by ThresholdCategorizer."  << fatal_error;
      
   stream << "Threshold Categorizer " << description()
	  << " categorizing on attribute " << attrInfo.name() << endl;
}



/***************************************************************************
  Description : Returns a pointer to a deep copy of this ThresholdCategorizer.
  Comments    :
***************************************************************************/
Categorizer* ThresholdCategorizer::copy() const
{ 
   return new ThresholdCategorizer(*this, ctorDummy);
}

/***************************************************************************
  Description : Returns TRUE if cat is equal to *this
  Comments    :
***************************************************************************/

Bool ThresholdCategorizer::operator==(const Categorizer &cat) const
{
   if (class_id() == cat.class_id())
      return (*this) == (const ThresholdCategorizer&) cat; // safe down cast
   return FALSE;
}

Bool ThresholdCategorizer::operator==(const ThresholdCategorizer &cat) const
{
   return ((attrInfo == cat.attrInfo ) &&
	   (attrNum == cat.attrNum) &&
	   (thresholdVal == cat.thresholdVal) &&
	   (LTEDscr == cat.LTEDscr) &&
	   (GTDscr == cat.GTDscr) &&
	   (num_categories() == cat.num_categories()) &&
	   (description()  == cat.description()));

}







