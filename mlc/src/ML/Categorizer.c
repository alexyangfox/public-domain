// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Abstract base class for Categorizers.
  Assumptions  : Number of categories must be strictly positive
                  (greater than zero).  Description cannot be empty or NULL.
  Comments     : Subclassed by many categorizers -- see appropriate .h
                   files (e.g. ConstCategorizer.h, AttrCategorizer.h).
                 Note that the label for categorizers is always a
                   category (discrete).
  Complexity   :
  Enhancements :
  History      : Chia-Hsin Li                                      11/29/94
                   Add function building probability distribution array.
                 Ronny Kohavi                                       8/03/93
                   Initial revision
***************************************************************************/

#include <basics.h>
#include <ConstCat.h>
#include <CtrBag.h>

RCSID("MLC++, $RCSfile: Categorizer.c,v $ $Revision: 1.18 $")

/***************************************************************************
  Description : Writes the Categorizer's description to the stream.
  Comments    : Protected method.
                Used in Graph::display.
***************************************************************************/
void Categorizer::short_display(ostream& stream) const
{
  stream << description();
}



Categorizer::Categorizer(int noCat, const MString& dscr) :
   numCat(noCat), descr(dscr)
{
   instancesNum = 0;
   distrArray = NULL;
   if (num_categories() <= 0)
      err << "Categorizer::Categorizer: number of categories must be "
         "positive (" << num_categories() << " is invalid)" << fatal_error;

   if (description() == "")
      err << "Categorizer::Categorizer: empty description" << fatal_error;
}

/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : We do not use the standard issue copy constructor because
                     we want to know exactly when we call the copy
		     constructor.  
***************************************************************************/
Categorizer::Categorizer(const Categorizer& source,
			 CtorDummy /*dummyArg*/)
: numCat(source.numCat), descr(source.descr)
{
   instancesNum = 0;
   distrArray = NULL;
   set_log_options(source.get_log_options());

}

/***************************************************************************
  Description : Destructor.
  Comments    :
***************************************************************************/
Categorizer::~Categorizer()
{
   delete distrArray;
}

/***************************************************************************
  Description : Build the probability distribution.
  Comments    :
***************************************************************************/
void Categorizer::build_distr(const CtrInstanceBag& bag)
{
   const SchemaRC schema = bag.get_schema();
   // Total number of instances in the bag
   instancesNum = bag.num_instances();
   
   int numLabelValue = schema.num_label_values();

   // copy the label count array in the bag to distrArray
   delete distrArray; // distrArray is set to NULL in constructor.
   distrArray = new Array<int>(numLabelValue);
   for (int labelCount = 0; labelCount < numLabelValue; labelCount++)
      (*distrArray)[labelCount] = bag.counters().label_counts()[labelCount];
}

/***************************************************************************
  Description : Copy the probability array.
  Comments    :
***************************************************************************/
void Categorizer::set_distr(const Array<int>& val)
{
   ASSERT(&val);

   delete distrArray;
   distrArray = new Array<int>(val.size());
   instancesNum = 0;
   for (int i = 0; i < val.size(); i++) {
      instancesNum += val[i];
      (*distrArray)[i] = val[i];
   }
}

/***************************************************************************
  Description : Add the distribution to the categorizer's distribution.
  Comments    :
***************************************************************************/
void Categorizer::add_distr(const Array<int>& val)
{
   if (distrArray == NULL)
      err << "Categorizer::add_distr: no distribution array present"
	  << fatal_error;
   
   ASSERT(&val);
   ASSERT(val.size() == distrArray->size());
   
   for (int i = 0; i < val.size(); i++) {
      (*distrArray)[i] += val[i];
      instancesNum += val[i];
   }
}


/***************************************************************************
  Description : Find the majority of distribution array.
  Comments    :
***************************************************************************/
Category Categorizer::majority_category() const
{
   if (distrArray == NULL)
      if (class_id() == CLASS_CONST_CATEGORIZER)
	 return ((const ConstCategorizer*)this)->get_category();
      else
	 err << "Categorizer::majority_category: but no distribution Array"
             << fatal_error;

   int maxInstances = -1;
   Category maxCat = -1;
   
   for (Category cat = 0; cat < distrArray->size(); cat++)
      if ((*distrArray)[cat] > maxInstances) {
	 maxInstances = (*distrArray)[cat];
	 maxCat = cat;
      }
   ASSERT(maxInstances >= 0);
   ASSERT(maxCat >= 0);
   
   return maxCat;
}

