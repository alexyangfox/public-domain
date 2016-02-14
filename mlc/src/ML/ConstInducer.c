// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : ConstInducer categorizes everything by the majority label.
  Assumptions  : 
  Comments     : Uses ConstCategorizer
  Complexity   : ConstInducer::train() has time complexity of 
                   LabelledInstanceBag::majority_category()
  Enhancements : Update the display_struct to use new file conventions.
  History      : Richard Long                                       8/17/93
                   Initial revision (.c)
		 Richard Long                                       8/17/93
		   Initial revision (.h)
***************************************************************************/

#include <basics.h>
#include <ConstInducer.h>
#include <checkstream.h>
#include <BagSet.h>
#include <ConstCat.h>

RCSID("MLC++, $RCSfile: ConstInducer.c,v $ $Revision: 1.21 $")

/***************************************************************************
  Description : Call Inducer::OK()
  Comments    : 
***************************************************************************/
void ConstInducer::OK(int /*level*/) const
{
   Inducer::OK();
}


/***************************************************************************
  Description : Deallocates categorizer.
  Comments    : 
***************************************************************************/
ConstInducer::~ConstInducer()
{
  DBG(OK());
  delete categorizer;
}


/***************************************************************************
  Description : Initializes categorizer with TS->majority_category()
  Comments    : Must be called after read_data()
***************************************************************************/
void ConstInducer::train()
{
   has_data(); // make sure we have a training set.
   Category cat = TS->majority_category();
   SchemaRC schema = TS->get_schema();
   AugCategory aca(cat, schema.category_to_label_string(cat));
   categorizer = new ConstCategorizer(description(), aca);
   ASSERT(categorizer);
}



/***************************************************************************
  Description : Return TRUE iff the class has a valid categorizer.
  Comments    :
***************************************************************************/
Bool ConstInducer::was_trained(Bool fatalOnFalse) const
{
   if (fatalOnFalse && categorizer == NULL)
      err << "Inducer::was_trained: No categorizer. "
             " Call train() to create categorizer" << fatal_error;
   return categorizer != NULL;
}


/***************************************************************************
  Description : Returns the categorizer that the inducer has generated.
  Comments    :
***************************************************************************/
const Categorizer& ConstInducer::get_categorizer() const
{
   was_trained(TRUE); // checks that categorizer exists
   return *categorizer;
}
