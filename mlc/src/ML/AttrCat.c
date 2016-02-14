// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Categorize an instance by the value of a single attribute.
  Assumptions  : The attribute must be of a type derived from Nominal.
  Comments     : 
  Complexity   : Constructor takes O(num-attributes) because of attr_info
                   (should be improved to const-time).
                 Categorize() takes constant time.
  Enhancements :
  History      : Chia-Hsin Li                                      11/23/94
                   Add operator==
                 Ronny Kohavi                                       8/02/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <AttrCat.h>

RCSID("MLC++, $RCSfile: AttrCat.c,v $ $Revision: 1.31 $")

/***************************************************************************
  Description : Check that the attribute number is Nominal, and return
                  the number of values it has.
  Comments    : Static function used to help the constructor call the
                  base class constructor.
***************************************************************************/
static int num_nominal_attr(const SchemaRC& schema, int attrNum)
{
   if (attrNum < 0)
      err << "AttrCat.c::num_nominal_attr: Bad attribute number: " 
          << attrNum << fatal_error;

   // find the matching schema
   const NominalAttrInfo& nai = schema.nominal_attr_info(attrNum);
   return nai.num_values();
}


/***************************************************************************
  Description : Construct an AttrCategorizer.
  Comments    : SchemaRC kept for debug purposes in categorize().
***************************************************************************/
AttrCategorizer::AttrCategorizer(const SchemaRC& schema, int attributeNum,
                                 const MString& dscr) : 
   Categorizer(num_nominal_attr(schema, attributeNum), dscr),
   attrInfo(schema.attr_info(attributeNum).clone()),
   attrNum(attributeNum)
{
   // Checks are done in num_nominal_attr
}


AttrCategorizer::AttrCategorizer(const AttrInfo& ainfo, int attributeNum,
				 const MString& dscr)
   : Categorizer(ainfo.cast_to_nominal().num_values(), dscr),
   attrInfo(ainfo.clone()),
   attrNum(attributeNum)
{
   if (attrNum < 0)
      err << "AttrCategorizer::AttrCategorizer: Bad attribute number: " 
          << attrNum << fatal_error;
}


/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : We do not use the standard issue copy constructor because
                     we want to know exactly when we call the copy
		     constructor.  
***************************************************************************/
AttrCategorizer::AttrCategorizer(const AttrCategorizer& source,
				 CtorDummy /*dummyArg*/)
   : Categorizer(source, ctorDummy), attrInfo(source.attrInfo->clone()),
     attrNum(source.attrNum)
{
}


/***************************************************************************
  Description : Categorize an instance.
  Comments    : Makes sure the AttributeInfo of the given instances matches
                  the one we have.
***************************************************************************/
AugCategory AttrCategorizer::categorize(const InstanceRC& instance) const
{
   DBG(attrInfo->equal(instance.attr_info(attrNum), TRUE));
   AttrValue_ val = instance[attrNum];
   MStringRC dscr = attrInfo->attrValue_to_string(val);
   AugCategory ac(attrInfo->get_nominal_val(val), dscr);
   return ac;
}


/***************************************************************************
  Description : Prints a readable representation of the Categorizer to the
                  given stream.
  Comments    :
***************************************************************************/
void AttrCategorizer::display_struct(MLCOStream& stream,
				     const DisplayPref& dp) const
{
   if (stream.output_type() == XStream)
      err << "AttrCategorizer::display_struct: Xstream is not a supported "
             "stream"  << fatal_error;

   if (dp.preference_type() != DisplayPref::ASCIIDisplay)
      err << "AttrCategorizer::display_struct: Only ASCIIDisplay is "
             "supported by AttrCategorizer"  << fatal_error;
      
   stream << "Attribute Categorizer " << description()
	  << " categorizing on attribute " << attrInfo->name() << endl;
}



/***************************************************************************
  Description : Returns a pointer to a deep copy of this AttrCategorizer.
  Comments    :
***************************************************************************/
Categorizer* AttrCategorizer::copy() const
{ 
   return new AttrCategorizer(*this, ctorDummy);
}


/***************************************************************************
  Description : Returns TRUE if (*this == cat)
  Comments    :
***************************************************************************/
Bool AttrCategorizer::operator==(const Categorizer &cat) const
{
   if (class_id() == cat.class_id())
      return (*this) == (const AttrCategorizer &) cat;
   return FALSE;
}

Bool AttrCategorizer::operator==(const AttrCategorizer &cat) const
{
   return ((attrInfo == cat.attrInfo) &&
	   (attrNum == cat.attrNum) &&
 	   (num_categories() == cat.num_categories()) &&
	   (description()  == cat.description()));
     
}
