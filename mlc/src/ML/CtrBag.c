// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide the same operations as InstanceBag,
                   but keep counters updated for queries.
  Assumptions  : See BagSet.c 
  Comments     : 
  Complexity   : The complexity of add/remove_instance() are the same as for
                    the InstanceBag plus O(m) where m is the
                    number of attributes in an instance (see BagCounters.c)
                 The constructors/destructors are the same as for
                    InstanceBag plus the construction/destruction time
                    for the counters.
		 majority_category() is O(l) where l is the number of labels.
  Enhancements :
  History      : Richard Long                                       6/14/94
                   Converted to use InstanceRC and SchemaRC.
                 Ronny Kohavi                                       9/03/93
                   Initial revision (.c, .h)
***************************************************************************/

#include <basics.h>
#include <CtrBag.h>

RCSID("MLC++, $RCSfile: CtrBag.c,v $ $Revision: 1.47 $")

/***************************************************************************
  Description : Make sure the counters were initialized.
  Comments    : Private.
***************************************************************************/
void CtrInstanceBag::check_init_counters()
{
   if (bagCounters == NULL)
      err << "CtrInstanceBag::check_counters counters not initialized yet"
	  << fatal_error;
}


/***************************************************************************
  Description : Check that the counters agree with the actual number
                  of instances.
  Comments    :
***************************************************************************/
void CtrInstanceBag::OK(int /*level*/) const
{
   int num;
   if ((num = counters().OK()) != num_instances())
      err << "CtrInstanceBag::OK: Counters claim there are " << num
	  << " instances, but there are " << num_instances()
	  << fatal_error;
}

/***************************************************************************
  Description : Constructors.  See BagSet.c
  Comments    : Here we just create the correct counters structure.
***************************************************************************/
// Protected.  Should only be called by CtrInstanceList constructor
// that gets file names as parameters
CtrInstanceBag::CtrInstanceBag()
   : InstanceBag()
{
   bagCounters = NULL;
}

CtrInstanceBag::CtrInstanceBag(const SchemaRC& schemaRC)
   : InstanceBag(schemaRC)
{
   bagCounters = new BagCounters(schemaRC);
}

CtrInstanceBag::CtrInstanceBag(const CtrInstanceBag& source, 
			       CtorDummy /* dummyArg */)
     : InstanceBag(source, ctorDummy)
{
   // Note that the copy constructor for the base class won't copy the
   // counters even though it's calling add, because virtual funcs for
   // derived classes are disabled during construction
   bagCounters = new BagCounters(source.counters(), ctorDummy);
   DBGSLOW(OK());
}



/***************************************************************************
  Description : Destructor.
  Comments    : Private method.
                The destruction sequence is pretty complex here.  First,
                  the destructor here is called, then the destructor
                  for InstanceBag is called.
***************************************************************************/

CtrInstanceBag::~CtrInstanceBag()
{
   delete bagCounters;
   bagCounters = NULL;
}


// 
/***************************************************************************
  Description : Takes on values of given bag.
  Comments    : Protected method.
                Gets ownership of bag.
***************************************************************************/
// Causes fatal_error; use copy(CtrInstanceBag*&)
void CtrInstanceBag::copy(InstanceBag*& /*bag*/)
{
   err << "CtrInstanceBag::copy(InstanceBag*&): Cannot "
          "make a CtrInstanceBag from a InstanceBag"
       << fatal_error;
}


void CtrInstanceBag::copy(CtrInstanceBag*& lcib)
{
   if (lcib == this)
      err << "CtrInstanceBag::copy: cannot overwrite self"
	  << fatal_error;
   delete bagCounters;
   bagCounters = lcib->bagCounters;
   lcib->bagCounters = NULL;
   // CtrInstanceBag::copy() requires a InstanceBag*&
   InstanceBag* lib = lcib;
   InstanceBag::copy(lib);
   DBG(ASSERT(lib == NULL));
   lcib = NULL; // the virtual destructor in
		// InstanceBag::copy() should have deleted this
}


/***************************************************************************
  Description : Allocates new BagCounters for the private member.
  Comments    : Protected method.
                Should only be called if bagCounters == NULL.
***************************************************************************/
void CtrInstanceBag::new_counters()
{
   if (bagCounters != NULL)
      err << "CtrInstanceBag::new_counters: Bag counters already "
	     "exist for this bag" << fatal_error;
   bagCounters = new BagCounters(get_schema());
}


/***************************************************************************
  Description : Override the default behavior of InstanceBag
                  to update the counters.
  Comments    : Protected method.
***************************************************************************/

Pix CtrInstanceBag::add_instance(const InstanceRC& instance)
{
   check_init_counters();
   bagCounters->add_instance(instance);
   return InstanceBag::add_instance(instance);
}


InstanceRC CtrInstanceBag::remove_instance(Pix& pix)
{
   InstanceRC instance = InstanceBag::remove_instance(pix);
   bagCounters->del_instance(instance);
   return instance;
}


/***************************************************************************
  Description : Returns the Category corresponding to the label that
                  occurs most frequently in the Bag.
  Comments    : This method is only meaningful for AttrInfo derived from
                  NominalAttrInfo.  Will cause fatal_error otherwise.
                In the case of a tie, returns the Category corresponding
		  to the label which occurs first in the NominalAttrInfo.
		Returns UNKNOWN_CATEGORY_VAL if there are no instances.
		Using counters is more efficient than InstanceBag
                  implementation.
***************************************************************************/

Category CtrInstanceBag::majority_category(Category tieBreaker) const
{
   const NominalAttrInfo& nai = nominal_label_info();
   int maxOcurrences = -1;
   Category maxCat = UNKNOWN_CATEGORY_VAL;
   for (NominalVal i = UNKNOWN_NOMINAL_VAL;
	i <= UNKNOWN_NOMINAL_VAL + nai.num_values(); i++) {
      int numOccurrences = bagCounters->label_count(i);
      if (numOccurrences > maxOcurrences ||
          (numOccurrences == maxOcurrences && i == tieBreaker)) {
	 maxOcurrences = numOccurrences;
	 maxCat = i;
      }
   }
       
   return maxCat;
}       


/***************************************************************************
  Description : In the returned CtrBagPtrArray CtrBagPtrArray[i] contains
                  all of the instances which the categorizer determines
		  to have category i.
  Comments    : 
***************************************************************************/
CtrBagPtrArray* CtrInstanceBag::ctr_split(const Categorizer& cat) const
{
   // the + 1 is for UNKNOWN_CATEGORY_VAL
   CtrBagPtrArray* bcpa = new CtrBagPtrArray(UNKNOWN_CATEGORY_VAL,
					     cat.num_categories() + 1);
   for (int i = bcpa->low(); i <= bcpa->high(); i++)
      (*bcpa)[i] = new CtrInstanceBag(get_schema());
   for (Pix libPix = first(); libPix; next(libPix)) {
      const InstanceRC& instance = get_instance(libPix);
      (*bcpa)[cat.categorize(instance)]->add_instance(instance);
   }
   return bcpa;
}




/***************************************************************************
  Description : Return BagCounters reference
  Comments    : When this is called, the class must exist (or abort)
***************************************************************************/
const BagCounters& CtrInstanceBag::counters() const
{
   if (bagCounters == NULL)
      err << "CtrInstanceBag::counters: counters not initialized yet"
	  << fatal_error;
   
   return *bagCounters;
}


/***************************************************************************
  Description : Returns a reference to a CtrInstanceBag.
  Comments    :
***************************************************************************/
CtrInstanceBag&
CtrInstanceBag::cast_to_ctr_instance_bag()
{
   return *this;
}

const CtrInstanceBag&
CtrInstanceBag::cast_to_ctr_instance_bag() const
{
   return *this;
}


/***************************************************************************
  Description : Returns a pointer to a new CtrInstanceBag with the
                   given schema.
  Comments    : 
***************************************************************************/
InstanceBag* CtrInstanceBag::create_my_type(const SchemaRC& schemaRC) const
{
   return new CtrInstanceBag(schemaRC);
}


/***************************************************************************
  Description : Returns a pointer to a counter bag that has the same contents
                   as this bag, with a random ordering of the instances.
  Comments    : mrandom and index default to NULL.
                The MRandom parameter allows for duplication of results.
***************************************************************************/
CtrInstanceBag* CtrInstanceBag::shuffle(MRandom* mrandom,
					InstanceBagIndex* index) const
{
   return &shuffle_(mrandom, index)->cast_to_ctr_instance_bag();
}


/***************************************************************************
  Description : Returns CtrInstanceBag* which points to a bag that
                  contains only the instances that has/(NOT) the same
		  attribute value as the given value on the given
		  attribute when exclude is FALSE/(TRUE).
  Comments    : Returns the ownership.
***************************************************************************/
CtrInstanceBag* CtrInstanceBag::find_attr_val(int attrNum,
					      NominalVal val,
					      Bool exclude,
					      Bool excludeUnknowns) const
{
   if (!attr_info(attrNum).can_cast_to_nominal()) 
      err << "CtrInstanceBag::ctr_split_by_attr_val: only nominal attribute "
	 "is allowed to call this function. " << fatal_error;

   const NominalAttrInfo& nai = attr_info(attrNum).cast_to_nominal();

   CtrInstanceBag* bag = new CtrInstanceBag(get_schema());
   for (Pix libPix = first(); libPix; next(libPix)) {
      const InstanceRC& instance = get_instance(libPix);
      // if include unknowns or if exclude them but this is known
      if (!excludeUnknowns || !nai.is_unknown(instance[attrNum]))
	 if (exclude) {
	    if (val != nai.get_nominal_val(instance[attrNum]))
	       bag->add_instance(instance);
	 }
	 else {
	    if (val == nai.get_nominal_val(instance[attrNum]))
	       bag->add_instance(instance);
	 }
   }

   return bag;
}
