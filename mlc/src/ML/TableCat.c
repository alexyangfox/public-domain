// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : A TableCategorizer consists of a table of all possible
                   instances.  Instances are categorized according to
		   label if they are found; otherwise UNKNOWN_AUG_CATEGORY is
		   returned.  If multiple instances exist, the label of
		   the majority instances found is returned.
  Assumptions  : LabelledInstanceInfo has label of type NominalAttrInfo
                 All instances must be labelled.
  Comments     : Currently, the table is implemented as a bag.  Since
                   the categorizer does not get ownership of the bag,
		   if someone adds instances to the bag, it will change
		   the behavior of the categorizer.  This should not
		   be the behavior when the categorizer implementation
		   is improved to use a hash table.
  Complexity   : TableCategorizer::add_instance() takes constant time.
                 TableCategorizer::categorize() has time complexity of
		   InstanceBag::find_unlabelled()
  Enhancements : 
  History      : Yeogirl Yun                                       9/04/94
                   Provides faster operations with InstanceHash.
		   See the description. (.h .c)
                 Richard Long                                      8/12/93
                   Initial Revision (.c)
                 Richard Long                                      8/05/93
                   Initial Revision (.h)
***************************************************************************/

#include <basics.h>
#include <TableCat.h>
#include <BagSet.h>

/***************************************************************************
  Description : Initializes the table with the given InstanceBag
                  and relays the description and number of categories to
		  the Categorizer constructor.
  Comments    : Assumes info->label_info() is of type NominalAttrInfo.
***************************************************************************/
TableCategorizer::TableCategorizer(const InstanceBag& bag,
				   Category defaultCategory,
				   const MString& dscr)
   : Categorizer(bag.num_categories(), dscr), hashTable(bag.num_instances())
{
   defaultCat = defaultCategory;
   for (Pix pix = bag.first(); pix; bag.next(pix))
      hashTable.insert(bag.get_instance(pix));
}




/***************************************************************************
    Description : Copy constructor with extra argument
    Comments    : 
***************************************************************************/
TableCategorizer::TableCategorizer(const TableCategorizer& source,
				   CtorDummy /*dummyArg*/)
   : Categorizer(source, ctorDummy),
     defaultCat(source.defaultCat), hashTable(source.hashTable, ctorDummy)
{
} 



/***************************************************************************
  Description : Returns category if instance found in table;
                   otherwise returns defaultCat.
  Comments    : This will return the category of the majority instances
                   found.
***************************************************************************/
AugCategory TableCategorizer::categorize(const
					       InstanceRC& instance) const
{
   MString strCategory;
   Category cat;
   const InstanceBag* bag = hashTable.find(instance);
   if (bag == NULL) {
      strCategory = instance.get_schema().category_to_label_string(defaultCat);
      cat = defaultCat;
   }
   else {
      cat = bag->majority_category();
      const InstanceRC& inst = bag->get_instance(bag->first());
      strCategory = inst.get_schema().category_to_label_string(cat);
   }
   return AugCategory(cat, strCategory);
}




/***************************************************************************
  Description : Prints a readable representation of the categorizer to the
                   given stream.
  Comments    :
***************************************************************************/
void TableCategorizer::display_struct(MLCOStream& stream,
				      const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::ASCIIDisplay)
      stream << "Table Categorizer " << description() 
             << " with default class " << defaultCat << endl
	     << "and the following labelled instances in the table:" << endl;
   // The hash table is randomized every time, so it's unreliable
   //   to display it for testers and such.
   IFLOG(3, hashTable.display(stream));
}



/***************************************************************************
  Description : Adds an instance to the table.
  Comments    : 
***************************************************************************/
void TableCategorizer::add_instance(const InstanceRC& instance)
{
   hashTable.insert(instance);
}
 

/***************************************************************************
  Description : Deletes an instance from the table.
  Comments    : 
***************************************************************************/
void TableCategorizer::del_instance(const InstanceRC& instance)
{
   hashTable.del(instance);
}



/******************************************************************************
  Description  : Returns TRUE iff the given instance is found in the hash
                   table.
		 Instance must have a label.
  Comments     : 
******************************************************************************/
Bool TableCategorizer::find_instance(const InstanceRC& instance) const
{
   return hashTable.find_labelled_instance(instance);
}



/***************************************************************************
  Description : Returns a pointer to a deep copy of this TableCategorizer.
  Comments    :
***************************************************************************/
Categorizer* TableCategorizer::copy() const
{
   return new TableCategorizer(*this, ctorDummy);
}

/***************************************************************************
  Description : Returns TRUE if (*this == cat)
  Comments    :
***************************************************************************/
Bool TableCategorizer::operator==(const Categorizer &cat) const
{
   err << "TableCategorizer::operator==: not implemented" << fatal_error;
   return cat == cat;
}
