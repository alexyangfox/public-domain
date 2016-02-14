// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Categorize an instance by the value of a single attribute
                    and its one of the values. 
  Assumptions  : The attribute must be of a type derived from Nominal.
  Comments     : 
  Complexity   : Constructor takes O(num-attributes) because of attr_info
                   (should be improved to const-time).
                 Categorize() takes constant time.
  Enhancements :
  History      : Yeogirl Yun                                       3/2/95
                 Initial revision
***************************************************************************/

#include <basics.h>
#include <AttrEqCat.h>


/*****************************************************************************
  Description : Checks whether given attribute value is valid.
  Comments    :
*****************************************************************************/
static void check_attr_val(const NominalAttrInfo& nai, NominalVal val)
{
   // check whether the given attribute value is valid.
   int numValues = nai.num_values();
   if (val < UNKNOWN_NOMINAL_VAL || val >= numValues)
      err << "AttrEqCat.c::check_attr_val: illegal attribute value "
	 ": " << val << "(should be in [" << UNKNOWN_NOMINAL_VAL << ", " <<
	 numValues - 1 << "])." << fatal_error;
}


/***************************************************************************
  Description : Construct an AttrEqCategorizer.
  Comments    : SchemaRC kept for debug purposes in categorize().
***************************************************************************/
AttrEqCategorizer::AttrEqCategorizer(const SchemaRC& schema, int attributeNum,
				     NominalVal val, const MString& dscr,
				     Bool separateUnknown) : 
   Categorizer(2, dscr),
   attrInfo(schema.attr_info(attributeNum).clone()),
   attrNum(attributeNum), 
   attrVal(val), 
   equal("EQUAL"),
   notEqual("NOT EQUAL"),
   unknown("?"),
   separateUnknowns(separateUnknown)
{
   check_attr_val(schema.attr_info(attributeNum).cast_to_nominal(), val);
}


AttrEqCategorizer::AttrEqCategorizer(const AttrInfo& ainfo, int attributeNum,
				     NominalVal val, const MString& dscr,
				     Bool separateUnknown)
   : Categorizer(2, dscr),
     attrInfo(ainfo.clone()),
     attrNum(attributeNum),
     attrVal(val), 
     equal("EQUAL"),
     notEqual("NOT EQUAL"),
     unknown("?"),
     separateUnknowns(separateUnknown)

{
   check_attr_val(ainfo.cast_to_nominal(), val);   
}


/***************************************************************************
  Description : Copy constructor which takes an extra argument.
  Comments    : We do not use the standard issue copy constructor because
                   we want to know exactly when we call the copy
                   constructor.  
***************************************************************************/
AttrEqCategorizer::AttrEqCategorizer(const AttrEqCategorizer& source,
				 CtorDummy /*dummyArg*/)
   : Categorizer(source, ctorDummy), attrInfo(source.attrInfo->clone()),
     attrNum(source.attrNum), attrVal(source.attrVal),
     equal(source.equal),
     notEqual(source.notEqual),
     unknown(source.unknown),
     separateUnknowns(source.separateUnknowns)
     
{
}


/***************************************************************************
  Description : Categorize an instance.
                Returns FIRST_CATEGORY_VAL + 1 when equal and
		  FIRST_CATEGORY_VAL when not equal.
		Unknowns are considered not-equal.
  Comments    : Makes sure the AttributeInfo of the given instances matches
                  the one we have.
***************************************************************************/
AugCategory AttrEqCategorizer::categorize(const InstanceRC&
						instance) const
{
   DBG(attrInfo->equal(instance.attr_info(attrNum), TRUE));
   AttrValue_ val = instance[attrNum];

   Category cat;
   if (attrInfo->get_nominal_val(val) == attrVal) {
      cat = FIRST_CATEGORY_VAL + 1;
      AugCategory ac(cat, equal);
      return ac;
   } else if (separateUnknowns && attrInfo->is_unknown(val)) {
      cat = UNKNOWN_CATEGORY_VAL;
      AugCategory ac(cat, unknown);
      return ac;
   } else {
      cat = FIRST_CATEGORY_VAL;
      AugCategory ac(cat, notEqual);
      return ac;
   }
}


/***************************************************************************
  Description : Prints a readable representation of the Categorizer to the
                  given stream.
  Comments    :
***************************************************************************/
void AttrEqCategorizer::display_struct(MLCOStream& stream,
				     const DisplayPref& dp) const
{
   if (stream.output_type() == XStream)
      err << "AttrEqCategorizer::display_struct: Xstream is not a supported "
             "stream"  << fatal_error;

   if (dp.preference_type() != DisplayPref::ASCIIDisplay)
      err << "AttrEqCategorizer::display_struct: Only ASCIIDisplay is "
             "supported by AttrEqCategorizer"  << fatal_error;
      
   stream << "Attribute Value Equal Categorizer " << description()
	  << " categorizing on attribute " << attrInfo->name() <<
             " and attribute value : " << attrVal << endl;
}



/***************************************************************************
  Description : Returns a pointer to a deep copy of this AttrEqCategorizer.
  Comments    :
***************************************************************************/
Categorizer* AttrEqCategorizer::copy() const
{ 
   return new AttrEqCategorizer(*this, ctorDummy);
}


/***************************************************************************
  Description : Returns TRUE if (*this == cat)
  Comments    :
***************************************************************************/
Bool AttrEqCategorizer::operator==(const Categorizer &cat) const
{
   if (class_id() == cat.class_id())
      return (*this) == (const AttrEqCategorizer &) cat;
   return FALSE;
}

Bool AttrEqCategorizer::operator==(const AttrEqCategorizer &cat) const
{
   return ((attrInfo == cat.attrInfo) &&
	   (attrNum == cat.attrNum) &&
 	   (num_categories() == cat.num_categories()) &&
	   (attrVal == cat.attrVal) &&
	   (separateUnknowns == cat.separateUnknowns) &&
	   (description()  == cat.description()));
     
}
