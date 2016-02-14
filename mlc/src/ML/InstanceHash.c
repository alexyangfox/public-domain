// MLC++ - Machine Learning in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/*****************************************************************************
  Description  : InstanceHashTable is a universal hash table of
                    InstanceBagSameAttr_. But it has a little
		    different behaviour from normal hash tables in
		    that it allows the addition of the same
		    instances(actually in the form of
		    InstanceBagSameAttr_ containing instances).
		    it provides a nice and clean interface of
		    hashing operations such as insert/del/find for
		    instances.
		 InstanceBagSameAttr_ is specially defined to be used as 
		    template arguments to InstanceHashTable_. It contains
		    a pointer to InstanceBag and defines special member
		    functions necessary for HashTable restrictions.
		 InstanceHashTable_ is a hash table class for
		    InstanceBagSameAttr_.
  Assumptions  :
  Comments     : Notice that the design of InstanceHashTable is a bit
                    sophisticated. To overload hash function
		    appropriate to instances, InstanceHashTable_ is
		    derived from UniversalHashTable with its KeyType
		    and DataType templetes being InstanceBagSameAttr_.
		    InstanceHashTable_ is then a data member of
		    InstanceHashTable, totally hidden from the users.
		    Finally, InstanceHashTable abstracts a hash table
		    solely for instances without being derived from
		    any other hash tables.
  Complexity   : 
  Enhancements : Extension to unlabelled instances.
                 OK() member of InstanceHashTable must check if numInstances
		   is the real number of instances in the hash table. 
  History      : Yeogirl Yun                                      9/1/94
                   initial revision(.h .c)
*****************************************************************************/ 


#include <basics.h>
#include <InstanceRC.h>
#include <InstanceHash.h>


// First, create InstanceBagSameAttr_ class having a pointer to
// InstanceBag class. 
// This is to support a data type consistent with the restrictions of
// UnivHashTable class. Note that InstanceBagSameAttr_ serves as both
// KeyType and DataType of UnivHashTable class.

// InstanceBagSameAttr_ contains(via pointer to InstanceBag)
// duplicates of the same instances with possible disparity between
// the instance's labels. 


/*****************************************************************************
   Description : Chckes that there are at least more than one instance
                    in the bag and all the instances have the same
		    value except for the labels. 
   Comments    :
*****************************************************************************/
void InstanceBagSameAttr_::OK(int /*level*/) const
{
   if (bag->no_instances())
      err << "InstanceBagSameAttr_::OK() : no instances in the"
	     " bag"  << fatal_error;
   Pix pix = bag->first();
   const InstanceRC firstInstance = bag->get_instance(pix);
   for (bag->next(pix); pix; bag->next(pix))
      if (!firstInstance.equal_no_label(bag->get_instance(pix))) {
	 err << "InstanceBagSameAttr_::OK() : "
	        " different instance exists within the bag : "
	     << endl << firstInstance << endl <<
		bag->get_instance(pix) << fatal_error;
      }
}



/*****************************************************************************
   Description : Regular copy constructor. It does the deep copy of
                    InstanceBag.
   Comments    : The normal copy constructor is necessary for DblLinkList in
                    hash table.
*****************************************************************************/
InstanceBagSameAttr_::InstanceBagSameAttr_(const InstanceBagSameAttr_& source)
{
   bag = new InstanceHashBag(*source.bag, ctorDummy);
}




/*****************************************************************************
   Description : Assignment operation. It deletes the current InstanceBag
                     and make a deep copy of the source's InstanceBag.
   Comments    :
*****************************************************************************/
InstanceBagSameAttr_& InstanceBagSameAttr_::operator=
                                            (const InstanceBagSameAttr_& source)
{
   if (this != &source) {
      delete bag;
      bag = new InstanceHashBag(*source.bag, ctorDummy);
   }
   return *this;
}





/*****************************************************************************
   Description : Constructor with one instance.
                 Creates an InstanceBagSameAttr_ and insert the
		    given instance.
		 Unlabelled instance will cause a fatal error.
   Comments    : Should only this constructor be used
*****************************************************************************/
InstanceBagSameAttr_::InstanceBagSameAttr_(const InstanceRC&
						 instance)
{
   bag = new InstanceHashBag(instance.get_schema());
   instance.is_labelled(TRUE); // force that every instance has a label.
   bag->add_instance(instance);
}


/*****************************************************************************
   Description : Destructor. Delete bag.
   Comments    : 
*****************************************************************************/
InstanceBagSameAttr_::~InstanceBagSameAttr_()
{ 
   delete bag;
}


/*****************************************************************************
   Description : Add instances into the bag.
                 It check the pre-existing instance is the same as the
                    given one. 
   Comments    : Notice that InstanceBagSameAttr_ must have at least
                    one instance, so there is always at least one instacne.
*****************************************************************************/
Pix InstanceBagSameAttr_::add_instance(const InstanceRC& instance)
{
   if (!instance.equal_no_label(bag->get_instance(bag->first())))
      err << "InstanceBagSameAttr_::add_instance() : Instance of different"
             " value is being added"  << endl
	  << "Instance is: " << bag->get_instance(bag->first())
	  << fatal_error;
   return bag->add_instance(instance);
}



/*****************************************************************************
   Description : Remove instances from the bag. It checks if the bag
                    would have at least one instance after the
		    deletion. If not, it aborts.
   Comments    :
*****************************************************************************/
InstanceRC InstanceBagSameAttr_::remove_instance(Pix& pix)
{
   if (bag->num_instances() <= 1)
      err << "InstanceBagSameAttr_::remove_instance() : there must be at"
             " least one instance in the bag after the deletion"
	  << fatal_error;
   return bag->remove_instance(pix);
}



/*****************************************************************************
   Description : Return TRUE iff the two bags are the same.
                 It does not compare the labels.
   Comments    : Notice that only the first instance needs to be
                    compared since all the other have the same
		    instance value with the first. 
*****************************************************************************/
Bool InstanceBagSameAttr_::operator==(const InstanceBagSameAttr_&
					 source) const
{
   return  bag->get_instance(bag->first()).
              equal_no_label(source.get_bag().
			     get_instance(source.get_bag().first()));
}



/*****************************************************************************
   Description : Output the contents of the bag.
   Comments    : It is implemented because HashTable requires it.
                    Notice that this class will be used as a KeyType and
		    DataType of HashTable.
*****************************************************************************/
MLCOStream& operator<<(MLCOStream& stream, const InstanceBagSameAttr_&
		    source)
{
   source.get_bag().display(stream);
   return stream;
}


// ------------------------ InstanceHashTable_ ---------------------------





/*****************************************************************************
   Description : Copy constructor. Should not be called. 
   Comments    :
*****************************************************************************/
InstanceHashTable_::InstanceHashTable_(const InstanceHashTable_&)
   : UniversalHashTable<InstanceBagSameAttr_, InstanceBagSameAttr_>(0)
{
   err << "InstanceHashTable_::InstanceHashTable_: Use dummy argument copy "
          "constructor." << fatal_error;
}



/*****************************************************************************
   Description : Copy constructor with dummy argument.
   Comments    :
*****************************************************************************/
InstanceHashTable_::InstanceHashTable_(const InstanceHashTable_& source,
				       CtorDummy)
   : UniversalHashTable<InstanceBagSameAttr_, InstanceBagSameAttr_>
        (source.num_items())
{
   // after creating an empty hash table.
   // it merges the source hash tabel using its own version of hash function.
   merge(source);
}



/*****************************************************************************
   Description : Constructor with the estimated number of data elements.
   Comments    : Data elements are not instances but
                    InstanceBagSameAttr_ objects.
*****************************************************************************/
InstanceHashTable_::InstanceHashTable_(int estimatedNum)
   : UniversalHashTable<InstanceBagSameAttr_,
                        InstanceBagSameAttr_>(estimatedNum)
{
}




/*****************************************************************************
   Description : Constructor with the estimated number of data elements and
                   seed.
   Comments    : Data elements are not instances but
                   InstanceBagSameAttr_ objects.
*****************************************************************************/
InstanceHashTable_::InstanceHashTable_(int estimatedNum, unsigned int seed)
   : UniversalHashTable<InstanceBagSameAttr_,
                        InstanceBagSameAttr_>(estimatedNum, seed)
{
}




/*****************************************************************************
   Description : Return the hash value of the given key.
                 We cycle through each attribute in turn, and not simply
		    through "the string," because it is conceivable that some
		    attributes may be of varying length (tree structured
		    attributes for example, or strings), and those are special
		    in the sense that you have to follow a pointer and hash
		    the value of the actual data.  Writing the code this way
		    makes it extensible.
   Comments    : Protected member.
*****************************************************************************/
int InstanceHashTable_::hash(const InstanceBagSameAttr_& instanceBag) const
{
   long hashValue = 0;

   // note : To iterate from the end helps speed execution time since
   //        it obviates unnecesary iterative extensions of array.
   //        For details, see the DynamicArray.c file.
   const InstanceRC& instance =
      instanceBag.get_bag().get_instance(instanceBag.get_bag().first());

   int count = instance.num_attr()*sizeof(AttrValue_);
   // The following statement is just for efficiency.
   // Accessing some estimated size will make sure it's allocated.
   (void)((InstanceHashTable_ *)this)->randomData.index(count);
   count = 0;
   for(int i = 0 ; i < instance.num_attr(); i++ ) {
      const unsigned char* ptr = (const unsigned char *)&instance[i].value;
      int storageSize = instance.attr_info(i).storage_size();
      for (int j = 0; j < storageSize; j++) {
	 hashValue += ptr[j] * // constness cast away : randomData, being a
                               // dynamic array, does not allow const function.
	    ((InstanceHashTable_
	      *)this)->randomData.index(count++);
      }
   }
   long temp =  hashValue  % size();
   return temp;
}
   





// -------------------------- InstanceHashTable ---------------------------


/*****************************************************************************
   Description : Constructor with estimated number of instances in the
                    hash table and an instance bag.
		 If a non-null instance bag is given, all the instance
		    in the bag will be inserted into the hash table.
   Comments    :
*****************************************************************************/
InstanceHashTable::InstanceHashTable(int estimatedNum, const
				     InstanceBag* bag)
   : hashTable(estimatedNum)
{
   numInstances = 0;
   if (bag != NULL) 
      for (Pix pix = bag->first(); pix; bag->next(pix))
	 insert(bag->get_instance(pix));
}



/*****************************************************************************
   Description : Constructor with estimated number of instances in the
                    hash table, seed, and an instance bag.
		 If a non-null instance bag is given, all the instance
		    in the bag will be inserted into the hash table.
   Comments    :
*****************************************************************************/
InstanceHashTable::InstanceHashTable(int estimatedNum,
				     unsigned int seed,
				     const InstanceBag* bag)
   : hashTable(estimatedNum, seed)
{
   numInstances = 0;
   if (bag != NULL) 
      for (Pix pix = bag->first(); pix; bag->next(pix)) 
	 insert(bag->get_instance(pix));
}



/*****************************************************************************
   Description : Copy constructor with a dummy argument.
   Comments    :
*****************************************************************************/
InstanceHashTable::InstanceHashTable(const InstanceHashTable& source,
				     CtorDummy)
   : hashTable(source.hashTable, ctorDummy)
{
   numInstances = source.num_instances();
}




/*****************************************************************************
   Description : Return TRUE iff the given instance with the label is
                    found on the hash table.
   Comments    :
*****************************************************************************/
Bool InstanceHashTable::find_labelled_instance(const InstanceRC&
					       instance) const
{
   instance.is_labelled(TRUE); // force that every instance has a
			       // label.
   const InstanceBag* bag = find(instance);
   if (bag == NULL)
      return FALSE;
   else if (bag->find_labelled(instance) == NULL)
      return FALSE;
   return TRUE;
}
   



/*****************************************************************************
   Description : Return the nubmer of instances matched with the bag
                    instances in the hash table.
   Comments    :
*****************************************************************************/
int InstanceHashTable::num_matched_labelled_instances(
	                                         const InstanceBag& bag) const
{
   int count = 0;
   for (Pix pix = bag.first(); pix; bag.next(pix))
      if (find_labelled_instance(bag.get_instance(pix)))
	 count++;
   return count;
}


/*****************************************************************************
   Description : Add an instance into the hash table by converting the
                    instance into a InstanceBagSameAttr_ of the one
		    instance.
		 Abort if the instance is unlabelled.
		 If there exists such an instance, it merge the
		    existing InstanceBagSameAttr_ with the just
		    created one.
		 Return the InstanceBagSameAttr_ the instance has
		    been put to. This can be either a merged one or a
		    just created one of the single instance.
   Comments    : 
*****************************************************************************/
void InstanceHashTable::insert(const InstanceRC& instance)
{
   instance.is_labelled(TRUE); // force that every instance has a
			       // label.
   numInstances++;
   // create a bag of the one instance.
   InstanceBagSameAttr_ temp(instance);

   InstanceBagSameAttr_ * bag = hashTable.find(temp);
   if (bag == NULL) // no such instance.
      hashTable.insert(temp);
   else
      bag->add_instance(instance);
}


/*****************************************************************************
   Description : Removes all the instances irregardless of the label values.
                 It will actually delete all the instances in the matched
		   InstanceBagSameAttr_ with the given instance.
   Comments    :
*****************************************************************************/
void InstanceHashTable::del_all_unlabelled(const InstanceRC& instance)
{
   // create a bag of the one instance.   
   InstanceBagSameAttr_ temp(instance);         
   InstanceBagSameAttr_* bag = hashTable.find(temp);
   const InstanceBag& instBag = bag->get_bag();
   if (bag == NULL) {
      Mcerr << "InstanceHashTable::delete_instance() : no such"
             " instance : " << endl << instance << endl;
      err << "aborting... " << fatal_error;
   }
   numInstances -= instBag.num_instances();
   hashTable.del(temp);
}
   



/*****************************************************************************
   Description : Delete the given instance in the hash table.
                 First, it finds the bag containing the instance and
		    delete only one such instance(notice that there
		    could be several such instances in the bag).
		 If there is only one instance in a bag and it is the
		    one to be deleted, the entire bag is discarded
		    from the hash table.
		 Abort if there is no such instance.
   Comments    : 
*****************************************************************************/
void InstanceHashTable::del(const InstanceRC& instance)
{
   numInstances--;
   // create a bag of the one instance.   
   InstanceBagSameAttr_ temp(instance);         
   InstanceBagSameAttr_* bag = hashTable.find(temp);
   const InstanceBag& instBag = bag->get_bag();
   if (bag == NULL) {
      Mcerr << "InstanceHashTable::delete_instance() : no such"
             " instance : " << endl << instance << endl;
      err << "aborting... " << fatal_error;
   }
   if (instBag.num_instances() > 1) {
      for (Pix pix = instBag.first(); pix; instBag.next(pix))
	 if (instBag.get_instance(pix) == instance) { // this equal compares
					              // the labels.
	    bag->remove_instance(pix);
	    return;
	 }
   }
   else  // delete the bag itself.
      hashTable.del(temp);
}
	    
	    


/*****************************************************************************
   Description : Returns the pointer to the InstanceBag matching the given key.
		 Returns NULL if there is no such element.
   Comments    : This is const version.
*****************************************************************************/
const InstanceHashBag* InstanceHashTable::
   find(const InstanceRC& instance) const
{
   instance.is_labelled(TRUE); // force every instance to have a label.
   // create a bag of the one instance.
   InstanceBagSameAttr_ temp(instance);
   const InstanceBagSameAttr_* ibsa = hashTable.find(temp);
   if (ibsa == NULL)
      return NULL;
   else
      return &(ibsa->get_bag());
}



	 

	    
	    
   

   
