// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : All methods other than constructor, destructor, and 
                   description cause fatal_error.
		 Only one BadCategorizer can be instantiated.  That instance
		   is "badCategorizer" which is declared in this file and
		   declared extern in "BadCat.h"  This allows faster
		   comparison using is_bad_categorizer() and prevents some
		   unnecessary waste of memory.
  Assumptions  : 
  Comments     : Used to initialize objects that must be changed to a valid
                   categorizer.
  Complexity   : 
  Enhancements : 
  History      : Richard Long                                       8/25/93
                   Changes so that only 1 BadCategorizer can be instantiated.
                 Richard Long                                       8/04/93
                   Initial revision (.c)
                 Richard Long                                       8/03/93
                   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <BadCat.h>

RCSID("MLC++, $RCSfile: BadCat.c,v $ $Revision: 1.18 $")

// There must be one definition for the static "instantiated" member.
Bool BadCategorizer::instantiated = FALSE;

BadCategorizer badCategorizer;


/***************************************************************************
  Description : Checks that BadCategorizer has not been instantiated.
                Updates "instantiated".
  Comments    : 
***************************************************************************/
BadCategorizer::BadCategorizer() : Categorizer(1, BadCatDscr)
{
  if (instantiated)
    err << "BadCategorizer::BadCategorizer: There is already an instance "
	   "of BadCategorizer" << fatal_error;
  instantiated = TRUE;
}


/***************************************************************************
  Description : The following methods cause fatal_errors.
  Comments    :
***************************************************************************/
int BadCategorizer::num_categories() const
{
  err << "BadCategorizer::num_categories: Bad categorizer" << fatal_error;
  return 0;  // to avoid error message
}


AugCategory BadCategorizer::categorize(const InstanceRC&) const
{
  err << "BadCategorizer::categorize: Bad categorizer" << fatal_error;
  return *((AugCategory*)NULL_REF); // to avoid error message
}

void BadCategorizer::display_struct(MLCOStream& /*stream*/,
				    const DisplayPref& /*dp*/) const
{
   err << "BadCategorizer::display_struct: Bad categorizer" << fatal_error;
}

Categorizer* BadCategorizer::copy() const
{
   err << "BadCategorizer::copy: Bad Categorizer" << fatal_error;
   return NULL;   // to avoid warning for no return value
}

/***************************************************************************
  Description : Returns TRUE if the given categorizer is a bad categorizer
                  and FALSE otherwise.
  Comments    : When DBG code is not used, this method depends on the fact 
                  that BadCategorizer can only be instantiated once.
***************************************************************************/
Bool BadCategorizer::is_bad_categorizer(const Categorizer& cat)
{
  DBG(if (&cat != &badCategorizer && cat.description() == BadCatDscr)
         err << "BadCategorizer::is_bad_categorizer: More than 1 "
                "BadCategorizer has been instantiated" << fatal_error);
  return (&cat == &badCategorizer);
}

/***************************************************************************
  Description : Returns TRUE if (*this == cat)
  Comments    :
***************************************************************************/
Bool BadCategorizer::operator==(const Categorizer&) const
{
   err << "BadCategorizer::operator==: Bad Categorizer" << fatal_error;
   return FALSE;
}
