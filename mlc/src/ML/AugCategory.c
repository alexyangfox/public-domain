// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide support for augmented categories.
                 An augmented category has extra information besides
                   the category number.  The minimal augmentation
                   implemented here also gives a string description.
                 operator<< is implemented to allow for easy printing.
  Assumptions  :
  Comments     : This could be subclassed to provide certainty levels
                   and other augmentations relevant for categories.
                 Note that since this class is so simple, the
                   functions are NOT virtual in order to avoid
                   construction of the virtual table.
  Complexity   : 
  Enhancements : 
  History      : Chia-Hsin Li                                       9/26/94
                   Added AugCategory::operator==
		   Deleted AugCategoryAlloc.
                 Richard Long                                       9/28/93
                   Added AugCategoryAlloc.
                 Ronny Kohavi                                       9/13/93
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <AugCategory.h>
#include <MLCStream.h>
#include <MStringRC.h>

RCSID("MLC++, $RCSfile: AugCategory.c,v $ $Revision: 1.19 $")

static const MStringRC UNKNOWN_CATEGORY_DESCRIPTION("Unknown Category");
const AugCategory UNKNOWN_AUG_CATEGORY(UNKNOWN_CATEGORY_VAL,
				       UNKNOWN_CATEGORY_DESCRIPTION);
				       

/***************************************************************************
  Description : Constructor
  Comments    :
***************************************************************************/
AugCategory::AugCategory(Category aCat, const MStringRC& dscr) :
   cat(aCat), catDscr(dscr)
{
   if (dscr == "")
      err << "AugCategory::AugCategory: Empty description for category "
	  << aCat << fatal_error;
   
   if (cat < UNKNOWN_CATEGORY_VAL || cat > UNKNOWN_CATEGORY_VAL +
       MAX_NUM_CATEGORIES)
      err << "AugCategory::AugCategory: category " << aCat << " out of "
             "range (legal values are " << UNKNOWN_CATEGORY_VAL << " to "
          << UNKNOWN_CATEGORY_VAL + MAX_NUM_CATEGORIES << fatal_error;
}      
   

/***************************************************************************
  Description : Copy constructor.
  Comments    : For some reason, the default copy constructor would not
                  work because of const members and virtual functions.
***************************************************************************/
AugCategory::AugCategory(const AugCategory& ac) : cat(ac.num()),
                                                  catDscr(ac.description())
{
   DBG(ASSERT(ac.description() != ""));
}


/***************************************************************************
  Description : Prints the description and the category number.
  Comments    : 
***************************************************************************/
void AugCategory::display(MLCOStream& stream) const
{
   stream << description() << " (" << num() << ')';
}

/***************************************************************************
  Description : Operator == for AugCategory
  Comments    :
***************************************************************************/
Bool AugCategory::operator==(const AugCategory& ac) const
{
   if (cat != ac.cat)
      return FALSE;
   else {
      DBG(if (catDscr != ac.catDscr)
	     err << "AugCategory::operator== edges match in number "
	            " but not in name " << ac << " vs " << *this
	         << fatal_error);
      return TRUE;
   }
}

DEF_DISPLAY(AugCategory);

