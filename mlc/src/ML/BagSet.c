// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Provide basic definitions for bags and sets of instances.
  Assumptions  : 
  Comments     : Training sets will be stored in a set or a bag.  Bags
                   allow multiple instances or contradictory
                   labellings.  Sets enforce uniqueness.
  Complexity   : InstanceBag::free() takes time proportional to the
                    number of instances + number of attributes.
		 InstanceBag::operator=() takes time proportial to
		    the sum of the instances in the lhs and rhs of the =.
		 InstanceBag::display() takes time proportional
		    to the number of instances * the number
		    of attributes per instance.
		 InstanceBag::majority_category() takes time
		    proportional to the number of different categories
		    + the number of instances.
		 InstanceBag::find_labelled/unlabelled() has complexity of 
		    number of instances * complexity of
		    Instance::equal/equal_no_label().
		 InstanceBag::split() takes time proportional to
		    the number of possible categories for the bag label
		    + the number of instances in the bag * the time
		    for categorize() of the given categorizer.
		 InstanceBag::num_instances() takes time
		    proportional to the number of instances in the bag.
		 InstanceBag::split_by_label() takes O(N + L),
		    where N is the number of instances in the bag and L
		    is the number of label values.
		 InstanceBag::remove_inst_with_unknown_attr() takes
		    O(num inst in bag * num attr per inst) time.
		 InstanceBag::remove_conflicting_instances() takes
		    O((num inst in bag)^2 * num attr per instance) time. 
		 InstanceBag::project() takes
		    O(num attributes * (num instances + num attributes)) time.
  Enhancements : Implement Bags using hash table instead of list.
  History      : Robert Allen                                        1/27/95
                    Modify project() after spreading work to Instance &
		    Schema.
                 Richard Long                                        7/29/93
                    Initial revision (.h, .c)
***************************************************************************/

#include <basics.h>
#include <BagSet.h>
#include <mlcIO.h>
#include <checkstream.h>
#include <MRandom.h>
#include <InstanceHash.h>

RCSID("MLC++, File $Revision: 1.133 $")

const MString defaultHeader;  // the empty string

// static variable definition:
Real InstanceBag::normalizeConfInterval = 0.5;

/***************************************************************************
  Description : Returns a reference to the InstancePtrList.
                Causes fatal_error if no list exists.
  Comments    : Private method.
***************************************************************************/
InstancePtrList& InstanceBag::instance_list()
{
   DBG(if (instances == NULL)
          err << "InstanceBag::instance_list(): List has "
                 "not been allocated" << fatal_error);
   return *instances;
}


const InstancePtrList& InstanceBag::instance_list() const
{
   DBG(if (instances == NULL)
          err << "InstanceBag::instance_list() const: List "
                 "has not been allocated" << fatal_error);
   return *instances;
}

/***************************************************************************
  Description : Check integrity constraints.
                We verify that all instances have the same schema at level 0
  Comments    : 
***************************************************************************/

void InstanceBag::OK(int level) const
{
   if (level < 1 && schema != NULL && instances) {
      SchemaRC schemaRC = get_schema();
      for (Pix pix = first(); pix; next(pix)) 
         if (get_instance(pix).get_schema() != schemaRC)
	    err << "InstanceBag::OK() mismatch in schemas for bag and "
	       " instance.   Instance is: \n" << get_instance(pix) <<
	       "\nBag schema is: " << schemaRC << fatal_error;
   }
}
   

/***************************************************************************
  Description : This method sets the schema to NULL.
  Comments    : Protected method.
                This constructor should only be called by
                  InstanceList(const MString&, const MString&,
		               const MString&) constructor.
***************************************************************************/
InstanceBag::InstanceBag()
{
   schema = NULL;
   instances = new InstancePtrList;
   weighted = FALSE;
}


/***************************************************************************
  Description : Takes on values of given bag.
  Comments    : Gets ownership of bag
***************************************************************************/
void InstanceBag::copy(InstanceBag*& bag)
{
   if (bag == this)
      err << "InstanceBag::copy: cannot overwrite self" << fatal_error;
   delete instances;
   schema = new SchemaRC(*bag->schema);
   instances = bag->instances;
   bag->instances = NULL;
   weighted = bag->weighted;
   delete bag;
   bag = NULL;
}



/***************************************************************************
  Description : Returns a pointer to a bag that has the same contents
                   as this bag, with a random ordering of the instances.
  Comments    : Protected method.
                The MRandom parameter allows for duplication of results.
***************************************************************************/
InstanceBag* InstanceBag::shuffle_(MRandom* mrandom,
				   InstanceBagIndex* index) const
{
   return independent_sample(num_instances(), mrandom, index);
}


/***************************************************************************
  Description : Constructor.
  Comments    : 
***************************************************************************/
InstanceBag::InstanceBag(const SchemaRC& schemaRC)
{
   schema = new SchemaRC(schemaRC);
   instances = new InstancePtrList;
   weighted = FALSE;
}


/***************************************************************************
  Description : Copy constructor. Makes a deep copy of the bag.
  Comments    : Assumes that the source bag has at least one instance.
***************************************************************************/
InstanceBag::InstanceBag(const InstanceBag& source,
			 CtorDummy /*dummyArg */)
{
   schema = new SchemaRC(*source.schema);
   instances = new InstancePtrList;
   weighted = source.weighted;
   for (Pix p = source.first(); p; source.next(p)) {
      InstanceRC instRC = source.get_instance(p);
      add_instance(instRC);
   }
}



/****************************************************************************
   Description : Assignment operator. After deleting all the instances
                    it copies all the instances of the source into itself.
   Comments    :
****************************************************************************/
InstanceBag& InstanceBag::operator=(const InstanceBag& source)
{
   if (this != &source) {
      remove_all_instances(); // delete all the instances.
      for (Pix pix = source.first(); pix; source.next(pix)) {
	 const InstanceRC instRC = source.get_instance(pix);
	 add_instance(instRC);
      }
   }
   return *this;
}



/***************************************************************************
  Description : Calls free(). Deletes instances.
  Comments    : 
***************************************************************************/
InstanceBag::~InstanceBag()
{
   DBGSLOW(OK(0));
   remove_all_instances();
   delete instances;
   delete schema;
}


/***************************************************************************
  Description : Returns Schema that matches all instances in the bag.
  Comments    : 
***************************************************************************/
SchemaRC InstanceBag::get_schema()const
{
   if (!schema)
      err << "InstanceBag::get_schema: schema has not been set"
	  << fatal_error;
   return *schema;
}



/***************************************************************************
  Description : Sets schema.
                If the schema is changed after it has been set, it should
		   match any data already present.
  Comments    :
***************************************************************************/
void InstanceBag::set_schema(const SchemaRC& schemaRC)
{
   // If we already have the schema set, save time.
   // This happens when normalizing assigns a get_unique_copy()
   //    schema.
   if (schema == NULL || schemaRC != *schema) {
      delete schema;
      schema = new SchemaRC(schemaRC);
      for (Pix pix = first(); pix; next(pix)) 
	 instance_list()(pix)->set_schema(*schema);
   }
}




/***************************************************************************
  Description : Adds new instance to the bag.
                The Schema for the instance must exactly match the Schema
                  for the InstanceBag.
  Comments    : temp is necessary to prevent warning about failing to
		  propagate changes to non-const temporary parameter.
***************************************************************************/
Pix InstanceBag::add_instance(const InstanceRC& instance)
{
  // causes fatal_error if not equal
  DBG(ASSERT(schema && schema->equal(instance.get_schema(), TRUE)));

  InstanceRC* temp = new InstanceRC(instance);
  return instance_list().append(temp);
}


/***************************************************************************
  Description : Deletes from the bag the instance indicated by
                   the given pix and returns the instance.
		Advances Pix to the next instance in the bag.
  Comments    :
***************************************************************************/
InstanceRC InstanceBag::remove_instance(Pix& pix)
{
   if (!pix)
      err << "InstanceBag::remove_instance: tried to "
	     "dereference a Null Pix" << fatal_error;
   InstanceRC instance = *instance_list()(pix);
   delete instance_list()(pix);
   instance_list().del(pix);
   return instance;
}

/***************************************************************************
  Description : Deletes all instances in the list.
  Comments    : 
***************************************************************************/
void InstanceBag::remove_all_instances()
{
   if (instances != NULL)
      while (!no_instances()) {
	 Pix pix = instance_list().first();
         remove_instance(pix);
      }
}


/***************************************************************************
  Description : Returns TRUE iff the bag contains no instances.
  Comments    : This method takes constant time and should if possible be
                  called instead of num_instances(), which takes
		  linear time.
		Cast away constness because GNU's length() is non-const.
***************************************************************************/
Bool InstanceBag::no_instances() const
{
   return ((InstanceBag*)this)->instance_list().empty();
}


/***************************************************************************
  Description : Returns the number of instances in the bag.
  Comments    : Cast away constness because GNU's length() is non-const.
***************************************************************************/
int InstanceBag::num_instances() const
{
   return ((InstanceBag*)this)->instance_list().length();
}


/***************************************************************************
  Description : Returns the number of categories that the instances
                  in the bag can have.
  Comments    : Only works if the Label is of a nominal attribute.
***************************************************************************/
int InstanceBag::num_categories() const
{
   return nominal_label_info().num_values();
}


/***************************************************************************
  Description : Pix.
  Comments    :
***************************************************************************/
Pix InstanceBag::first() const
{
  return((InstanceBag*)this)->instance_list().first();
}


void InstanceBag::next(Pix& pix) const
{
  if (!pix)
    err << "InstanceBag::next: tried to dereference a Null Pix"
	<< fatal_error;
  ((InstanceBag*)this)->instance_list().next(pix);
}


/***************************************************************************
  Description : Returns a Instance corresponding to the given Pix.
  Comments    : 
***************************************************************************/
InstanceRC InstanceBag::get_instance(const Pix pix) const
{
   if (!pix)
      err << "InstanceBag::get_instance: tried to "
	     "dereference a Null Pix" << fatal_error;
  return *(((InstanceBag*)this)->instance_list()(pix));
}


/***************************************************************************
  Description : Displays all of the  instances in the bag.
		The output is formatted for lines of width lineWidth.
		All lines for an instance after the first line are
		  indented by instanceWrapIndent (defined in Instance.c)
  Comments    : Prints "No instances" if Bag is empty.
***************************************************************************/
void InstanceBag::display(MLCOStream& stream, 
			  Bool protectChars,
			  Bool normalizeReal) const
{
   for (Pix bagPix = first(); bagPix; next(bagPix)) {
      InstanceRC instance = get_instance(bagPix);
      instance.display(stream, protectChars, get_weighted(), normalizeReal);
   }
   if (no_instances())
      stream << "No instances" << endl;
}

// Define operator<< for display()
DEF_DISPLAY(InstanceBag)


/***************************************************************************
  Description : Returns the Category corresponding to the label that
                  occurs most frequently in the Bag.
		  In case of a tie, we prefer the given tieBreaker
		  if it is one of thost tied.
		tieBreaker can be UNKNOWN_CATEGORY_VAL if you prefer
		  the earlier category in the tied ones.
  Comments    : This method is only meaningful for labels with AttrInfo
                  derived from NominalAttrInfo.  Will cause fatal_error
		  otherwise.
                In the case of a tie, returns the Category corresponding
		  to the label which occurs first in the NominalAttrInfo.
		Returns UNKNOWN_CATEGORY_VAL if there are no instances.
***************************************************************************/
Category InstanceBag::majority_category(Category tieBreaker) const
{
   const NominalAttrInfo& nai = nominal_label_info();
   // +1 because of UNKNOWN_CATEGORY_VAL instead of FIRST_CATEGORY_VAL
   Array<int> count(UNKNOWN_CATEGORY_VAL,
		    nai.num_values() + 1, 0);
   for (Pix pix = first(); pix; next(pix)) {
      InstanceRC instance = get_instance(pix);
      count[nai.get_nominal_val(instance.get_label())]++;
   }
   int maxOcurrences = -1;
   Category maxCat = UNKNOWN_CATEGORY_VAL;
   for (int i = count.low(); i <= count.high(); i++)
      if (count[i] > maxOcurrences ||
          (count[i] == maxOcurrences && i == tieBreaker)) {
	 maxOcurrences = count[i];
	 maxCat = i;
    }
   return maxCat;
}


/***************************************************************************
  Description : Returns Pix for instance if it was found, NULL otherwise.
  Comments    : Note that this function requires that the instance AND its
                   label match an instance in the bag.
		   Use find_unlabelled() to match the instance with an
		   instance in the bag regardless of label value.
***************************************************************************/
Pix InstanceBag::find_labelled(const InstanceRC& instance) const
{
   DBG(if (schema && !schema->is_labelled())
          err << "Instance::find_labelled: InstanceBag is not labelled"
              << fatal_error);
   DBG(if (!instance.is_labelled())
          err << "Instance::find_labelled: instance is not labelled"
              << fatal_error);
   DBG(ASSERT(schema && schema->equal(instance.get_schema(), TRUE)));
   
   for (Pix pix = first(); pix; next(pix))
      if (instance.equal(get_instance(pix)))
         return pix;
   return NULL;
}


/***************************************************************************
  Description : Returns Pix for instance if it was found, NULL otherwise.
  Comments    : Note that this function requires that the instance, but
                   NOT its label (if one exists) match an instance in the bag.
		   Use find_labelled() to match the instance with an
		   instance and label in the bag.
***************************************************************************/
Pix InstanceBag::find_unlabelled(const InstanceRC& instance) const
{
   DBG(ASSERT(schema && schema->equal_no_label(instance.get_schema(), TRUE)));
   for (Pix pix = first(); pix; next(pix))
      if (instance.equal_no_label(get_instance(pix)))
         return pix;
   return NULL;
}


/***************************************************************************
  Description : In the returned BagPtrArray BagPtrArray[i] contains
                  all of the instances which the categorizer determines
		  to have category i.
  Comments    : 
***************************************************************************/
BagPtrArray* InstanceBag::split(const Categorizer& cat) const
{
   // Note num_cat() + 1, and NOT num_cat() because the count starts
   //   from UNKNOWN and not from FIRST.
   BagPtrArray& bpa = *new BagPtrArray(UNKNOWN_CATEGORY_VAL,
				       cat.num_categories() + 1);
   for (int i = bpa.low(); i <= bpa.high(); i++)
      bpa[i] = new InstanceBag(get_schema());
   for (Pix bagPix = first(); bagPix; next(bagPix)) {
      InstanceRC instance = get_instance(bagPix);
      bpa[cat.categorize(instance)]->add_instance(instance);
   }
   return &bpa;
}


/***************************************************************************
  Description : Split the bag according to the labels.  Each bag
                  corresponds to all instances having one label value.
  Comments    : Works only for nominal labels.
                If a label value never appears in the training set,
                  the corresponding bag will be empty.
***************************************************************************/
BagPtrArray* InstanceBag::split_by_label() const
{
   const NominalAttrInfo& instance = nominal_label_info();
   // the + 1 is for UNKNOWN_CATEGORY_VAL
   BagPtrArray& bpa = *new BagPtrArray(UNKNOWN_CATEGORY_VAL,
				       instance.num_values() + 1);
   for (int i = bpa.low(); i <= bpa.high(); i++)
      bpa[i] = create_my_type(get_schema());
   for (Pix bagPix = first(); bagPix; next(bagPix)) {
      InstanceRC instance = get_instance(bagPix);
      const NominalAttrInfo& labelInfo = nominal_label_info();
      bpa[labelInfo.get_nominal_val(instance.get_label())]
	                                         ->add_instance(instance);
   }
   return &bpa;
}


/***************************************************************************
  Description : Returns a fatal error.  If these functions are
                   reached, then the cast is illegal.
  Comments    : These should be overridden in the appropriate subclasses.
***************************************************************************/
InstanceList& InstanceBag::cast_to_instance_list()
{
   err << "InstanceBag::cast_to_instance_list: Illegal cast"
       << fatal_error;
   return *(InstanceList*)NULL_REF;  // to avoid compiler warning
}

const InstanceList& InstanceBag::cast_to_instance_list() const
{
   err << "InstanceBag::cast_to_instance_list() const: "
          "Illegal cast" << fatal_error;
   return *(InstanceList*)NULL_REF;  // to avoid compiler warning
}

CtrInstanceBag& InstanceBag::cast_to_ctr_instance_bag()
{
   err << "InstanceBag::cast_to_ctr_instance_bag: "
          "Illegal cast" << fatal_error;
   return *(CtrInstanceBag*)NULL_REF;  // to avoid compiler warning
}

const CtrInstanceBag& InstanceBag::cast_to_ctr_instance_bag() const
{
   err << "InstanceBag::cast_to_ctr_instance_bag() const: "
          "Illegal cast" << fatal_error;
   return *(CtrInstanceBag*)NULL_REF;  // to avoid compiler warning
}

CtrInstanceList& InstanceBag::cast_to_ctr_instance_list()
{
   err << "InstanceBag::cast_to_ctr_instance_list: Illegal cast"
       << fatal_error;
   return *(CtrInstanceList*)NULL_REF;  // to avoid compiler warning
}

const CtrInstanceList& InstanceBag::cast_to_ctr_instance_list() const
{
   err << "InstanceBag::cast_to_ctr_instance_list() const: "
          "Illegal cast" << fatal_error;
   return *(CtrInstanceList*)NULL_REF;  // to avoid compiler warning
}


/***************************************************************************
  Description : Returns a pointer to an array of pointers to the
                   instances in the bag.
  Comments    : This is used for independent_sample() and shuffle().
***************************************************************************/
InstanceBagIndex* InstanceBag::create_bag_index() const
{
   int bagSize = num_instances();
   InstanceBagIndex& index = *new InstanceBagIndex(0, bagSize, NULL);
   int i = 0;
   Pix pix = first();
   for (; i < bagSize && pix; i++, next(pix))
      index[i] = pix;
   ASSERT(i == bagSize && !pix);
   return &index;
}


/***************************************************************************
  Description : Returns a pointer to a bag with "size" instances
                   randomly sampled (without replacement) from this bag.
  Comments    : mrandom and index default to NULL.
                The MRandom parameter allows for duplication of results.
***************************************************************************/
InstanceBag* InstanceBag::independent_sample(int size, MRandom* mrandom,
				             InstanceBagIndex* index) const
{
   return independent_sample(size, NULL,  mrandom, index);
}


/*****************************************************************************
  Description : Returns a pointer to a bag with 'size' instances and
                  another pointer to a bag with the rest of instances.
  Comments    :
*****************************************************************************/
InstanceBag* InstanceBag::independent_sample(int size,
					     InstanceBag *restOfBag,
					     MRandom* mrandom,
					     InstanceBagIndex* index) const
{

   DBG(if (size < 0 || size > num_instances())
          err << "InstanceBag::independent_sample: size(" << size
              << ") must be greater than zero and less than or equal to "
              << num_instances() << fatal_error);
   DBG(if (index != NULL && index->size() != num_instances())
          err << "InstanceBag::independent_sample: The index size("
              << index->size() << ") was not equal to the bag size("
              << num_instances() << ')' << fatal_error);

   InstanceBagIndex& bagIndex = index ? *index : *create_bag_index();
   InstanceBag* bag = create_my_type(get_schema());
   MRandom* mrand = mrandom ? mrandom : new MRandom;
   int maxRandom = bagIndex.size();
   for (int i = 0; i < size; i++) {
      int instNum = mrand->integer(maxRandom);
      maxRandom--;
      const InstanceRC& instance = get_instance(bagIndex[instNum]);
      bag->add_instance(instance);
      Pix temp = bagIndex[instNum];
      bagIndex[instNum] = bagIndex[maxRandom];
      bagIndex[maxRandom] = temp;
   }
   if (restOfBag) {
      int numInstances = restOfBag->num_instances();
      if (numInstances != 0)
	 err << "BagSet::independent_sample: restOfBag is not empty but has "
	    << numInstances << " instances." << fatal_error;
      for (i = 0; i < maxRandom; i++) 
	 restOfBag->add_instance(get_instance(bagIndex[i]));
      ASSERT(restOfBag->num_instances() == bagIndex.size() - size);
   }
   ASSERT(bag->num_instances() == size);
   
   if (mrandom == NULL)
      delete mrand;
   if (index == NULL)
      delete &bagIndex;
   return bag;
}





/***************************************************************************
  Description : Returns a pointer to a bag that has the same contents
                   as this bag, with a random ordering of the instances.
  Comments    : mrandom and index default to NULL.
                The MRandom parameter allows for duplication of results.
***************************************************************************/
InstanceBag* InstanceBag::shuffle(MRandom* mrandom,
				  InstanceBagIndex* index) const
{
   return shuffle_(mrandom, index);
}


/***************************************************************************
  Description : Returns a pointer to a new InstanceBag with the
                   given instance info.
  Comments    : The new bag will own its instances and its instance info.
***************************************************************************/
InstanceBag*
InstanceBag::create_my_type(const SchemaRC& schemaRC) const
{
   return new InstanceBag(schemaRC);
}


/***************************************************************************
  Description : Removes all instances from the bag which have unknown
		attributes.
  Comments    :
***************************************************************************/
void InstanceBag::remove_inst_with_unknown_attr()
{
   Pix instPix = first();
   while (instPix != NULL) {
      Bool hasUnknownAttr = FALSE;
      InstanceRC instance = get_instance(instPix);
      for (int attrNum=0; attrNum < num_attr() && !hasUnknownAttr; attrNum++) {
         const AttrInfo& attrInfo = attr_info(attrNum);
         const AttrValue_& attrValue = instance[attrNum];
         if (attrInfo.is_unknown(attrValue))
            hasUnknownAttr = TRUE;
      }
      if (hasUnknownAttr)
         remove_instance(instPix);
      else 
         next(instPix);
   }
}


/***************************************************************************
  Description : Removes all instances having an unknown in attrNum.
  Comments    :
***************************************************************************/
void InstanceBag::remove_inst_with_unknown_attr(int attrNumX)
{
   Pix instPix = first();
   while (instPix != NULL) {

      Bool hasUnknownAttr = FALSE;
      InstanceRC instance = get_instance(instPix);
      const AttrInfo& attrInfo = attr_info(attrNumX);

      if (attrInfo.is_unknown(instance[attrNumX]))
	 hasUnknownAttr = TRUE;
      if (hasUnknownAttr)
         remove_instance(instPix);
      else 
         next(instPix);
   }
}


/***************************************************************************
  Description : Removes conflicting instances.  Retains the one copy of an
		instance with the majority category if one exists.
  Comments    :
***************************************************************************/
void InstanceBag::remove_conflicting_instances()
{
   // build a hash table.
   InstanceHashTable hashTable(num_instances(),this);
   Category bagMajority = majority_category(UNKNOWN_CATEGORY_VAL);

   for (Pix pix = first(); pix; ) {
      const InstanceRC& inst = get_instance(pix);
      const InstanceBag* ib = hashTable.find(inst);
      if (ib == NULL)
	 remove_instance(pix);
      else {
	 if (inst.nominal_label_info().get_nominal_val(inst.get_label()) ==
	     ib->majority_category(bagMajority)) {
	    hashTable.del_all_unlabelled(inst);
	    next(pix);
	 }
	 else 
	    remove_instance(pix);
      }
   }	 
}




/***************************************************************************
  Description : This function takes an attribute mask which is an array of
                  booleans indicating whether the corresponding attribute
		  should be included in the projection.
		It returns a bag with a new Schema that includes only
		  the attributes with a mask value TRUE.
  Comments    : 
***************************************************************************/
InstanceBag* InstanceBag::project(const BoolArray& attrMask) const
{
   DBGSLOW(OK());
   ASSERT(attrMask.size() == num_attr());

   SchemaRC newSchema = get_schema().project(attrMask);
   
   InstanceBag *newBag = create_my_type(newSchema);

   for (Pix pix = first(); pix; next(pix)) {
      newBag->add_instance(get_instance(pix).project(newSchema, attrMask));
   }
   return newBag;
}


/***************************************************************************
  Description : Set/get whether the bag contains weighted instances.
  Comments    : Warning!  Setting a bag to weighted will affect all the
                   bags that point to the same LabelledInstanceInfo.
***************************************************************************/
void InstanceBag::set_weighted(Bool weight)
{
   weighted = weight;
}


Bool InstanceBag::get_weighted() const
{
   return weighted;
}


/***************************************************************************
  Description : Set/get the weight of the instance indicated by the pix.
  Comments    : Assumes that the bag is weighted.
                Warning!  Setting the weight will affect all bags which
		   point to the same InstancePtrList.
***************************************************************************/
void InstanceBag::set_weight(Pix pix, Real wt)
{
   if (pix == NULL)
      err << "InstanceBag::set_weight: Null pix" << fatal_error;
   set_weighted(TRUE);
   instance_list()(pix)->set_weight(wt);
}


Real InstanceBag::get_weight(Pix pix) const
{
   return get_instance(pix).get_weight();
}


/***************************************************************************
  Description : Returns the sum of the weights of all instances in the bag.
  Comments    :
***************************************************************************/
Real InstanceBag::total_weight() const
{
   Real totalWeight = 0;
   for (Pix pix = first(); pix; next(pix)) {
      totalWeight += get_weight(pix);
   }
   return totalWeight;
}


/***************************************************************************
  Description : Displays the instances using the Buntine format.  If the
                   test flag is set, "TESTITEM\t" is prepended to the
		   line for each instance.
  Comments    : "TESTITEM" is the syntax required by Oliver dgraph program
                   to indicate that an instance is in the test set.  It
		   does not use separate files.  (See /u/mlc/oliver/README).
		The "TESTITEM" prefix is not accounted for in the line
		   wrapping.
***************************************************************************/
void InstanceBag::buntine_display(MLCOStream& stream,  Bool test) const
{
   for (Pix bagPix = first(); bagPix; next(bagPix)) {
      InstanceRC instance = get_instance(bagPix);
      MString prefix;
      if (test)
	 stream << "TESTITEM\t";
      instance.buntine_display(stream);
   }
   if (no_instances())
      stream << "No instances" << endl;
}


/***************************************************************************
  Description : Displays the names file associated with the bag.
  Comments    :
***************************************************************************/
void InstanceBag::display_names(MLCOStream& stream,
				Bool protectChars,
				const MString& header) const
{
   get_schema().display_names(stream, protectChars, header);
}



/*****************************************************************************
  Description : Normalizes attributes using extreme method.
  Comments    : 
*****************************************************************************/
void InstanceBag::extreme_normalize_attr(int attrNum,
					 const RealAttrInfo& rai,
					 RealAttrValue_& min,
					 RealAttrValue_& max) const
{
   for (Pix pix = first(); pix; next(pix)) {
      const InstanceRC& instance = get_instance(pix);
      const AttrValue_& val = instance[attrNum];
      if (!rai.is_unknown(val)) {
	 if (rai.get_real_val(val) < rai.get_real_val(min))
	    rai.set_real_val(min, rai.get_real_val(val));
	 if (rai.get_real_val(val) > rai.get_real_val(max))
	    rai.set_real_val(max, rai.get_real_val(val));
      }
   }
}



/*****************************************************************************
  Description : Normalizes attributes using interquartile method.
  Comments    : 
*****************************************************************************/
void InstanceBag::interquartile_normalize_attr(int attrNum,
					       const RealAttrInfo& rai,
					       RealAttrValue_& min,
					       RealAttrValue_& max) const
{
   StatData data;
   for (Pix pix = first(); pix; next(pix)) {
      const InstanceRC& instance = get_instance(pix);
      const AttrValue_& val = instance[attrNum];
      if (!rai.is_unknown(val)) 
	 data.insert(rai.get_real_val(val));
   }
   Real low= 0;
   Real high = 0;
   if (data.size() > 0) { // should be grater than on before call percentile.
      data.percentile(normalizeConfInterval, low, high);
      rai.set_real_val(min, low);
      rai.set_real_val(max, high);
   }
   else {
      rai.set_real_val(min, 0);
      rai.set_real_val(max, 1);
   }
}

/***************************************************************************
  Description : Determines the minimum and maximum values for the
                   indicated attribute.  Should only be called for real
		   attributes.
  Comments    :
***************************************************************************/
void InstanceBag::normalize_attr(int attrNum, NormalizationMethod method)
{
   if (method == none)
      return;

   RealAttrValue_ min;
   RealAttrValue_ max;

   normalize_attr(attrNum, method, min, max);

   // We are going to cast away constness for the attrInfo,
   //   so we must make sure we have a unique copy of the schema,
   //   or we might modify someone else's schema's attrinfo
   schema->get_unique_copy();
   set_schema(get_schema()); // Change it for the whole bag.
   
   const AttrInfo& ai = get_schema().attr_info(attrNum);
   // Must cast away constness to change max and min
   RealAttrInfo& rai = ((AttrInfo&)ai).cast_to_real();

   rai.set_max(rai.get_real_val(max));
   rai.set_min(rai.get_real_val(min));
}


/***************************************************************************
  Description : Determines the minimum and maximum values for the
                   indicated attribute.  Should only be called for real
		   attributes.
  Comments    :
***************************************************************************/
void InstanceBag::normalize_attr(int attrNum, NormalizationMethod method,
				 RealAttrValue_& min, RealAttrValue_& max)
const
{
   if (method == none)
      return;
   const RealAttrInfo& rai = get_schema().attr_info(attrNum).cast_to_real();
   rai.set_real_val(min, REAL_MAX);
   rai.set_real_val(max, -REAL_MAX);
   switch (method) {
      case none : // to avoid compiler warning message. none is checked
		  // already at the first line.
      case extreme :
	 extreme_normalize_attr(attrNum, rai, min, max);
	 break;
      case interquartile :
	 interquartile_normalize_attr(attrNum, rai, min, max);
	 break;
   }

   if (rai.get_real_val(min) > rai.get_real_val(max)) {
      // Don't abort because this happens in the allhypo dataset.
      Mcerr << "InstanceBag::normalize_attr_extreme: No known values for "
	     "attribute " << rai.name() << " (attribute number " << attrNum
	  << ").  Assuming 0,1" << endl;
      rai.set_real_val(min, 0);
      rai.set_real_val(max, 1);
   }
}

/***************************************************************************
  Description : Normalizes all real attributes in the bag.
  Comments    :
***************************************************************************/
void InstanceBag::normalize_bag(NormalizationMethod method)
{
   for (int attrNum = 0; attrNum < get_schema().num_attr(); attrNum++) {
      const AttrInfo& ai = get_schema().attr_info(attrNum);
      if (ai.can_cast_to_real())
	 normalize_attr(attrNum, method);
   }
}


/***************************************************************************
  Description : Copies this bag, preserving its type through use of the
                create_my_type function.
  Comments    :
***************************************************************************/
InstanceBag *InstanceBag::clone() const
{
  InstanceBag *dest = create_my_type(get_schema());
  
  for(Pix p = first(); p; next(p)) {
    dest->add_instance(get_instance(p));
  }
  return dest;
}

/***************************************************************************
  Description : Checks elements in the bag for equality.
  Comments    : Performs a deep comparison.
***************************************************************************/
Bool InstanceBag::operator==(const InstanceBag& other) const
{
   if(num_instances() != other.num_instances())
      return FALSE;
   Pix pthis, pother;
   for(pthis = first(), pother = other.first();
       pthis && pother; next(pthis), other.next(pother)) {
      if(get_instance(pthis) != other.get_instance(pother))
	 return FALSE;
   }
   return TRUE;
}


/***************************************************************************
  Description : Change the value of an attribute value in each instance
                  to unknown with the given probability.
  Comments    : Do Not try to be smart (as I was) and modify the actual
                  instances because the counters get all messed up.  Ronnyk
***************************************************************************/

void InstanceBag::corrupt_values_to_unknown(Real rate, MRandom& mrandom)
{
   if (rate < 0 || rate > 1)
      err << "InstanceBag::corrupt_values_to_unknown: rate outside [0,1] "
	 << rate << fatal_error;
   
   InstanceBag tmpBag(*this, ctorDummy);
   remove_all_instances();

   for (Pix pix = tmpBag.first(); pix; tmpBag.next(pix)) {
      InstanceRC instance = tmpBag.get_instance(pix);
      for (int i = 0; i < num_attr(); i++) 
	 if (rate > mrandom.real(0,1)) // not equal for zero to work
	       attr_info(i).set_unknown(instance[i]);
      add_instance(instance);
   }
}

