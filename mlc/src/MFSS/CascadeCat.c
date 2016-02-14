// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Wrapper categorizer which holds an ordered list of child
                   categorizers.  Each call to categorize() will attempt to
		   have the first child categorize the instance; if it returns
		   unknown, the second child is tried, etc.
  Assumptions  : It is an error for the last child categorizer to return
                   unknown.
  Comments     : 
  Complexity   :
  Enhancements : 
  History      : Yeogirl Yun                                        7/8/95
                   Re-engineered the initial version
                 Robert Allen                                       2/17/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <CascadeCat.h>

RCSID("MLC++, $RCSfile: CascadeCat.c,v $ $Revision: 1.8 $")

/***************************************************************************
  Description :  Check all the categorizers in list.
  Comments    : 
***************************************************************************/
void CascadeCat::OK(int level) const
{
   for (int i=0; i < catList->size(); i++) {
      if (catList->index(i) == NULL)
	 err << "CascadeCat::OK: Categorizer in list at index #" << i
	     << " is null" << fatal_error;
         catList->index(i)->OK(level);
   }
}

/***************************************************************************
  Description : Helper function for constructor to get size of categorizer.
  Comments    :
***************************************************************************/
static int num_cats(PtrArray<Categorizer *>*& cats)
{
   int numCat = 0;
   for (int i=0; i < cats->size(); i++) {
      if (cats->index(i) == NULL)
	 err << "CascadeCat::CascadeCat: Categorizer in list at index #" << i
	     << " is null" << fatal_error;
      if (cats->index(i)->num_categories() > numCat)
	 numCat = cats->index(i)->num_categories();
   }
   return numCat;
}  
      
/***************************************************************************
  Description : Initializes the CascadeCat.  
  Comments    : Owns the list of categorizers passed in.
***************************************************************************/
CascadeCat::CascadeCat( const MString& dscr,
			PtrArray<Categorizer *>*& cats )
   : Categorizer(num_cats(cats), dscr)
{
   catList = cats;
   cats = NULL;			// take ownership
}


/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : Making a copy of all the categorizers pointed to by
                    the catList array is required since multiple ownership
		    must be avoided.  Time and space requirements may be high
		    and depend on the type of wrapped categorizers.
***************************************************************************/
CascadeCat::CascadeCat(const CascadeCat& source,
		       const CtorDummy )
   : Categorizer(source, ctorDummy)
{
   int listSize = source.get_categorizer_list().size();
   catList = new PtrArray<Categorizer *>(listSize);

   for (int i=0; i < listSize; i++)
      catList->index(i) = source.get_categorizer_list().index(i)->copy();
}


/***************************************************************************
  Description : Destructor
  Comments    : 
***************************************************************************/
CascadeCat::~CascadeCat()
{
   delete catList;
}


/***************************************************************************
  Description : Try the categorizers in catList until one returns a
                  category.
  Comments    : 
***************************************************************************/
AugCategory CascadeCat::categorize(const InstanceRC& inst) const
{
   DBGSLOW(OK());

   Category category;
   for (int i=0; i < catList->size() && (category = catList->index(i)->
			 categorize(inst)) == UNKNOWN_CATEGORY_VAL; i++)
      ; // null
   
   ASSERT(category != UNKNOWN_CATEGORY_VAL || i == catList->size());
   if (category == UNKNOWN_CATEGORY_VAL)
      err << "CascadeCat::categorize: no known category returned from "
	 "list of categorizers.  Last categorizer " << catList->size() <<
	 " should never return UNKNOWN_CATEGORY_VAL. Instance: " << inst <<
	  ", category returned by " << catList->index(i-1)->description() <<
	  ": " << category << "!" << fatal_error;


   MStringRC categoryString =
      inst.get_schema().category_to_label_string(category);
   
   AugCategory augCat(category, categoryString);

   LOG(2, "First category returned was " << augCat
       << " by categorizer #" << i << endl);
   LOG(3, "For instance: " << inst << endl);
   
   return augCat;
}


/***************************************************************************
  Description : Prints a readable representation of the categorizer to the
                  given stream.
  Comments    : 
  ***************************************************************************/
void CascadeCat::display_struct(MLCOStream& stream,
				const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::ASCIIDisplay) {
      stream << "Cascading Categorizer: " << description() << endl
	     << "   Using Categorizers: "  << endl;

      for (int i=0; i < catList->size(); i++) 
	 catList->index(i)->display_struct(stream, dp);
      stream << endl << endl;
   }
}


/***************************************************************************
  Description : Allows operator << to be used on this class through macros.
  Comments    :
***************************************************************************/
void CascadeCat::display(MLCOStream& stream, const DisplayPref& dp) const
{
   display_struct(stream, dp);
}

/***************************************************************************
  Description : Returns a pointer to a deep copy of this CascadeCat.
  Comments    : Uses copy constructor - makes copy of all wrapped categorizers.
***************************************************************************/
Categorizer* CascadeCat::copy() const
{
   return new CascadeCat(*this, ctorDummy);
}


/***************************************************************************
  Description : Checks the argument categorizer to make sure its the same
                  type as this.  If so, use the specific operator ==.
  Comments    : Relies on unique categorizer #define in Categorizer.h
***************************************************************************/
Bool CascadeCat::operator==(const Categorizer &cat) const
{
   if ( class_id() == cat.class_id() )
      return (*this) == (const CascadeCat &) cat;
   return FALSE;
}

/***************************************************************************
  Description : Compares all elements of class for equality.
  Comments    :
***************************************************************************/
Bool CascadeCat::operator==(const CascadeCat &cat) const
{
   return ( *catList == *(cat.catList) );
}



DEF_DISPLAY(CascadeCat);




