// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : ConstCategorizer always gives the same category.
  Assumptions  : Categories must be greater than UNKNOWN_CATEGORY_VAL.
                 For safety reasons, an upper bound of
                 MAX_NUM_CATEGORIES is exists.
  Comments     :
  Complexity   :
  Enhancements :
  History      : Chia-Hsin Li                                       11/24/94
                   Add operator==
                 Ronny Kohavi                                        7/17/93
                   Initial revision

***************************************************************************/

#include <basics.h>
#include <ConstCat.h>

RCSID("MLC++, $RCSfile: ConstCat.c,v $ $Revision: 1.23 $")

/***************************************************************************
  Description : Constructor just assigns the class.
  Comments    : Checks done by AugCategory constructor.
***************************************************************************/
ConstCategorizer::ConstCategorizer(const MString& dscr,
				   const AugCategory& augCat)
   :  Categorizer(1, dscr), category(augCat)
{}

/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : We do not use the standard issue copy constructor because
                     we want to know exactly when we call the copy
		     constructor.
***************************************************************************/
ConstCategorizer::ConstCategorizer(const ConstCategorizer& source,
				   CtorDummy /* dummyArg */)
   : Categorizer(source, ctorDummy), category(source.category)
{
}


/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
ConstCategorizer::~ConstCategorizer()
{}


/***************************************************************************
  Description : Prints a readable representation of the Categorizer to the
                  given stream.
  Comments    : 
***************************************************************************/
void ConstCategorizer::display_struct(MLCOStream& stream,
				      const DisplayPref& dp) const
{
   if (stream.output_type() == XStream)
      err << "ConstCategorizer::display_struct: Xstream is not a valid "
          << "stream for this display_struct"  << fatal_error;

   if (dp.preference_type() != DisplayPref::ASCIIDisplay)
      err << "ConstCategorizer::display_struct: Only ASCIIDisplay is "
          << "valid for this display_struct"  << fatal_error;
      

   stream << "Constant Categorizer " << description()
	  << " categorizing as " << category << endl;
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of this ConstCategorizer.
  Comments    :
***************************************************************************/
Categorizer* ConstCategorizer::copy() const
{
   return new ConstCategorizer(*this, ctorDummy);
}

/***************************************************************************
  Description : Returns TRUE if (*this == cat)
  Comments    :
***************************************************************************/

Bool ConstCategorizer::operator==(const Categorizer &cat) const
{
   if (class_id() == cat.class_id())
      return (*this) == (const ConstCategorizer &) cat;
   return FALSE;
}


Bool ConstCategorizer::operator==(const ConstCategorizer &cat) const 
{
   return ((category.num() == cat.category.num()) &&
	   (category.description() == cat.category.description()) &&
	   (num_categories() == cat.num_categories()) &&
	   (description()  == cat.description()));
}





