
// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Categorizer which feeds another categorizer with instances
                   that it discretizes using a vector of discretizors
		   and the discretized schema.
  Assumptions  : It is assumed that the routines which create an array of
                   discretizors (see RealDiscretizor.c, OneR.c, EntropyDisc.c,
		   and BinningDisc.c) are used for constructing a DiscCat.

  Comments     :
  Complexity   :
  Enhancements :
  History      : James F. Dougherty                               02/26/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <DiscCat.h>


RCSID("MLC++, $RCSfile: DiscCat.c,v $ $Revision: 1.7 $")


/***************************************************************************
  Description : Constructor
  Comments    : acquires ownership of the categorizer.
***************************************************************************/
DiscCat::DiscCat(const MString& description,
		 PtrArray<RealDiscretizor*>*& discretizors,
		 const SchemaRC& discretizedSchema,
		 Categorizer*& cat)
   :Categorizer(cat->num_categories(), description),
    discSchema(discretizedSchema)
{
   DiscCat::categorizer = cat;
   DiscCat::disc = discretizors;
   cat = NULL;
   discretizors = NULL;
   OK();
}
		 

/***************************************************************************
  Description : Copy constructor
  Comments    :
***************************************************************************/
DiscCat::DiscCat(const DiscCat& source,
		 CtorDummy /* dummyArg */)
   :Categorizer(source, ctorDummy),
    discSchema(source.discSchema),
    categorizer(source.categorizer->copy()),
    disc(NULL)
{
   disc = new PtrArray<RealDiscretizor*>(source.disc->size());
   for(int i = 0; i < disc->size(); i++)
      if (source.disc->index(i))
	 disc->index(i) = source.disc->index(i)->copy();
   OK();
}


/***************************************************************************
  Description : Destructor
  Comments    :
***************************************************************************/
DiscCat::~DiscCat()
{
   delete categorizer;
   delete disc; 
}


/***************************************************************************
  Description : Invariant validation method.
  Comments    :
***************************************************************************/
void DiscCat::OK(int /* level */) const
{
   DBG(ASSERT(categorizer != NULL));
   DBG(ASSERT(disc != NULL));
   DBG(ASSERT(discSchema.num_attr() == disc->size()));

//@@ Check that if attrInfo(k) can cast to real, we have a discretizor
/*   for(int i = 0; i < disc->size(); i++)
      if (disc->index(i)) */
}


/***************************************************************************
  Description : For each instance, categorize discretizes the continuous
                  attributes in the instance and then calls the categorizer
		  to classify it.
  Comments    :
***************************************************************************/
AugCategory DiscCat::categorize(const InstanceRC& instance) const
{
   OK();
   InstanceRC discInst = instance;
   discretize_instance(disc, discSchema, discInst); // changes to a discInst
   return categorizer->categorize(discInst);
}


/***************************************************************************
  Description : Prints out a readable representation of the categorizer.
  Comments    : For dot graphs, we just pass this to out internal
                  categorizer.  For example, running disc-filter
		  on ID3 should display the resulting dot id3.
***************************************************************************/
void DiscCat::display_struct(MLCOStream& stream, const DisplayPref& dp) const
{
    if (dp.preference_type() == DisplayPref::ASCIIDisplay) {
      stream << "Discretizing Categorizer for " << description() << endl
	     << "   Discretized Schema: " << discSchema << endl
	     << "   Discretizors: " << endl;

      for(int i = 0; i < disc->size(); i++)
	 if (disc->index(i))
	    disc->index(i)->display(stream);
      stream << endl;
      categorizer->display_struct(stream, dp);
    } else // unrecognized.  Just pass down
      categorizer->display_struct(stream, dp);
}

/***************************************************************************
  Description : Returns a pointer to a clone of this DiscCat.
  Comments    :
***************************************************************************/
Categorizer* DiscCat::copy() const
{
   return new DiscCat(*this, ctorDummy);
}


/***************************************************************************
  Description : operator ==
  Comments    :
***************************************************************************/
Bool DiscCat::operator ==(const Categorizer& cat) const
{
   if (class_id() == cat.class_id())
      return (*this) == (const DiscCat&)cat;
   return FALSE;
}

Bool DiscCat::operator ==(const DiscCat& dCat) const
{
   if (disc->size() != dCat.disc->size())
      return FALSE;

   for( int i = 0; i < disc->size(); i++){
      // can't use pointer operand for ^
      if ( (disc->index(i) ? 1 : 0) ^ (dCat.disc->index(i) ? 1 : 0))
	 return FALSE;
      if (!(*disc->index(i) == *dCat.disc->index(i)))
	 return FALSE;
   }
   return *categorizer == *dCat.categorizer &&
            discSchema == dCat.discSchema;
            
}




