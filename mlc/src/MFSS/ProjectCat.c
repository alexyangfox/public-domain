// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Categorizer which wraps around another categorizer.
                   ProjectCat categorizes instances by projecting attributes
		   from the instance using a boolean mask, then passes the
		   projected instance to the wrapped categorizer for
		   categorization.
		   
  Assumptions  : 
  Comments     : 
  Complexity   : ProjectCat(desc, mask) is O(s) + cost of schema::project()
                   where s = number of AttrInfos in the original schema.
                 ProjectCat(ProjectCat, ctorDummy) - the copy
		   constructor - is O(c+s), s is defined as above, and
		   c = the time required to copy the wrapped categorizer, which
		   depends on the type of categorizer being wrapped.
		 ~ProjectCat() is O(c) where c = the complexity
		   of the wrapped categorizer's destructor.
		 categorize(instance) is O(m+c), where m = the number of
		   of attributes of the schema of the instance; this is
		   required to project the instance.  c is the complexity
		   of the categorize function of the wrapped categorizer.
  Enhancements : 
  History      : Robert Allen                                       1/27/95
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <ProjectCat.h>

RCSID("MLC++, $RCSfile: ProjectCat.c,v $ $Revision: 1.8 $")

/***************************************************************************
  Description : 
  Comments    : 
***************************************************************************/
void ProjectCat::OK(int /* level */) const
{
   ASSERT(categorizer != NULL);
   ASSERT(shortSchema.num_attr() == attrMask.num_true());
}


/***************************************************************************
  Description : Initializes the ProjectCat.  
  Comments    : Owns the categorizer passed in.
***************************************************************************/
ProjectCat::ProjectCat( const MString& dscr,
			const BoolArray& attrmask,
			const SchemaRC& schema,
			Categorizer*& cat )
   : Categorizer(cat->num_categories(), dscr),
     attrMask(attrmask, ctorDummy), fullSchema(schema),
     shortSchema(schema.project(attrmask))
{
   categorizer = cat;
   cat = NULL;			// take ownership
}


/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : 
***************************************************************************/
ProjectCat::ProjectCat(const ProjectCat& source,
		       CtorDummy /* dummyArg */)
   : Categorizer(source, ctorDummy),
     attrMask(source.attrMask, ctorDummy),
     fullSchema(source.fullSchema),
     shortSchema(source.shortSchema)
{
   categorizer = source.categorizer->copy();
}


/***************************************************************************
  Description : Destructor
  Comments    : 
***************************************************************************/
ProjectCat::~ProjectCat()
{
   delete categorizer;
}


/***************************************************************************
  Description : For each instance, project the attributes of the instance
                  and then call the Categorizer to classify it.
  Comments    : 
***************************************************************************/
AugCategory ProjectCat::categorize(const InstanceRC& inst) const
{
   DBG(OK());
   return categorizer->categorize(inst.project(shortSchema, attrMask));
}


/***************************************************************************
  Description : Prints a readable representation of the categorizer to the
                  given stream.
  Comments    : 
  ***************************************************************************/
void ProjectCat::display_struct(MLCOStream& stream,
				const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::ASCIIDisplay) {
      stream << "Projecting Categorizer for " << description() << endl
	     << "   Attribute Mask: " << attrMask << endl
	     << "   Projected Schema: " << shortSchema << endl
	     << "   Using Categorizer: ";
      categorizer->display_struct(stream, dp);
      stream << endl;
   } else // unrecognized.  Just pass down
      categorizer->display_struct(stream, dp);
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of this ProjectCat.
  Comments    :
***************************************************************************/
Categorizer* ProjectCat::copy() const
{
   return new ProjectCat(*this, ctorDummy);
}


/***************************************************************************
  Description : Checks the argument categorizer to make sure its the same
                  type as this.  If so, use the specific operator ==.
  Comments    : Relies on unique categorizer #define in Categorizer.h
***************************************************************************/
Bool ProjectCat::operator==(const Categorizer &cat) const
{
   if ( class_id() == cat.class_id() )
      return (*this) == (const ProjectCat &) cat;
   return FALSE;
}

/***************************************************************************
  Description : Compares all elements of class for equality.
  Comments    :
***************************************************************************/
Bool ProjectCat::operator==(const ProjectCat &cat) const
{
   return ( *categorizer == *(cat.categorizer) &&
	    attrMask == cat.attrMask &&
	    fullSchema == cat.fullSchema &&
	    shortSchema == cat.shortSchema );
}



