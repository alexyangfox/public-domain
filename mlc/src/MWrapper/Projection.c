// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The Projection class, along with its supporting classes
                   ProjInst and ProjKey, implements a PROJECTED INSTANCE
		   with destinations.  The destinations of the instance
		   are indexed by the features mentioned in the projection
		   selector.  The destinations refer to individual ProjSets
		   within some ProjLevel, or to the actual labels of the
		   instances.
  Assumptions  : 
  Comments     : The indexing of the destinations is currently implemented
                   as a hash table.  The ProjKey class holds the index
		   and the ProjInst class holds both the index and
		   destination.
		 No instances are ever actually projected.  Instead, the
		   entire instance is retained in each ProjInst, and
		   instances of class FeatureSet are used to keep track
		   of which features are considered relevent.
  Complexity   :
  Enhancements :
  History      : Dan Sommerfield                                   11/18/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <InstList.h>
#include <ProjLevel.h>
#include <ProjGraph.h>
#include <AttrCat.h>
#include <ConstCat.h>

RCSID("MLC++, $RCSfile: Projection.c,v $ $Revision: 1.2 $")

SET_DLLPIX_CLEAR(ProjInst, NULL);

const Real HashTable<ProjKey, ProjInst>::LOAD_FACTOR = 0.5;


/***************************************************************************
  Description : Compares two keys for the hash table
  Comments    : Needed for hashing
***************************************************************************/
Bool ProjKey::operator==(const ProjKey& other) const
{
   return featureSet.instances_equal(instance, other.instance);
}

/***************************************************************************
  Description : Displays a hash table key
  Comments    :
***************************************************************************/
void ProjKey::display(MLCOStream& stream) const
{
   featureSet.display_instance(instance, stream);
}

DEF_DISPLAY(ProjKey);


/***************************************************************************
  Description : Displays a single projected instance.  The bigger version
                  displays destination information as well.
  Comments    :
***************************************************************************/
void ProjInst::display(const FeatureSet& instSet,
		       const FeatureSet& projSet,
		       MLCOStream& stream) const
{
   instSet.display_instance(key.get_instance(), stream);
   stream << " on ";
   projSet.display_instance(key.get_instance(), stream);
   stream << " => " << destination;
}
void ProjInst::display(MLCOStream& stream) const
{
   stream << key << " => " << destination;
}

DEF_DISPLAY(ProjInst);


/***************************************************************************
  Description : Consistency and integrity checks
  Comments    :
***************************************************************************/
void Projection::OK() const
{
   // make sure each instance in the iterList may be found in the
   // hash table
   for(Pix p=iterList.first(); p; iterList.next(p)) {
      ProjKey key(iterList.get_instance(p), owner.dest_features());
      const ProjInst *projInst = find(key);
      
      // the item MUST be in the hash table.  otherwise we messed up.
      if(!projInst) {
	 Mcerr << "Projection::OK: could not find item in hash table "
	       << endl;
	 Mcerr << "Hash Table:" << endl;
	 HashTable<ProjKey,ProjInst>::display(Mcerr);
	 Mcerr << "List:" << endl << iterList << endl;
	 err << "ProjLevel::OK: element in iteration list was not in "
	        "hash table" << fatal_error;
      }

   }
}


/***************************************************************************
  Description : Runs a consistency check on the source and destination
                  levels.  The purpose of this call is to make sure that
		  this Projection belongs to a level which really has the
		  specified source and destination.  This function
		  implements the low-levels of ProjSet::check_consistency.
  Comments    : 
***************************************************************************/
void Projection::check_consistency(const SchemaRC& schema,
				   const ProjLevel *apparentSource,
				   const ProjLevel *apparentDest) const
{
   ASSERT(schema == iterList.get_schema());
   if(source == NULL)
      ASSERT(apparentSource == NULL);
   else {
      ASSERT(apparentSource);
      ASSERT(apparentSource->feature_set_match(&source->owner.inst_features(),
					       &source->owner.dest_features()));
   }

   (void)apparentDest;
}


/***************************************************************************
  Description : Constructs a projection given a bag, instance and projection
                  feature sets, an identifying main instance whose instance
		  portion is equal to instance portions of all other
		  instances stored in this Projection, and an optional
		  source for edge redirection later.
  Comments    :
***************************************************************************/
Projection::Projection(const InstanceBag& bag,
		       ProjLevel& itsOwner,
		       const InstanceRC& mainInst,
		       Projection *src)
   : UniversalHashTable<ProjKey, ProjInst>(bag.num_instances()),
     owner(itsOwner),
     iterList(bag.get_schema()),
     mainInstance(mainInst),
     source(src)
{
   // add the instances from the iteration list
   for(Pix p=bag.first(); p; bag.next(p)) {
      const InstanceRC& instance = bag.get_instance(p);
      if(src) {
	 // if src is defined, then inherit the destination from the source
	 ProjKey key(instance, src->owner.dest_features());
	 const ProjInst *pi = src->find(key);
	 int cat = pi->get_destination();
	 add_instance(instance, cat);
      }
      else {
	 // otherwise, set destination equal to the label
	 const NominalAttrInfo& labelInfo = bag.nominal_label_info();
	 int cat = labelInfo.get_nominal_val(instance.get_label());
	 add_instance(instance, cat);
      }
   }
}

/***************************************************************************
  Description : Hashing function for projections.  Operates only on the
                  instance portion of each instance.
  Comments    :
***************************************************************************/
int Projection::hash(const ProjKey& pKey) const
{
   const InstanceRC instance = pKey.get_instance();
   long hashValue = 0;
   
   int count = instance.num_attr()*sizeof(AttrValue_);
   // The following statement is just for efficiency.
   // Accessing some estimated size will make sure it's allocated.
   (void)((Projection *)this)->randomData.index(count);
   count = 0;
   const Array<int>& selector = owner.dest_features().get_feature_numbers();
   for(int i = selector.low() ; i <= selector.high(); i++ ) {
      AttrValue_ aval = instance[selector[i]];
      const unsigned char* ptr = (const unsigned char *)&aval;
      int storageSize = instance.attr_info(i).storage_size();
      for (int j = 0; j < storageSize; j++) {
	 hashValue += ptr[j] * // constness cast away : randomData, being a
                               // dynamic array, does not allow const function.
	    ((Projection*)this)->randomData.index(count++);
      }
   }
   long temp =  hashValue  % size();
   return temp;
}

/***************************************************************************
  Description : Add an instance to this projection, specifying the starting
                  destination.
  Comments    :
***************************************************************************/
void Projection::add_instance(const InstanceRC& inst, Category dest)
{
   if(!instance_belongs(inst))
      err << "Projection::add_instance: instance in not compatible with "
	 "this projection; features of inst as listed in "
	 "instSet do not agree with features of main instance" <<
	 fatal_error;
   
   // build a ProjInst and store it in the hash table using insert
   // throw out duplicates by checking first to see if we can find
   // the instance in the table.
   ProjInst projInst(inst, dest, owner.dest_features());
   if(!find(projInst.get_key())) {
      insert(projInst);

      // add the instance to the instance list which we use for iteration
      iterList.add_instance(inst);
   }
}

/***************************************************************************
  Description : Determine if an instance belongs in the Projection.  The
                  criterion for belonging is that the instance agrees
		  with the main instance in the projection on the attributes
		  mentioned in the instance selector.
  Comments    :
***************************************************************************/
Bool Projection::instance_belongs(const InstanceRC& inst) const
{
   // an instance belongs if it matches the main instance for this
   // Projection.  We'll accomplish this check by building a key
   // for the passed in instance and the main instance, and
   // comparing them using the instSchema.
   return owner.inst_features().instances_equal(inst, mainInstance);
}

/***************************************************************************
  Description : Determine if two projections contain instances which
                  disagree on projected destinations for the same
		  projected portion.
  Comments    : 
***************************************************************************/
Bool Projection::conflicts_with(const Projection& other) const
{
   // Find the shorter of the two iteration lists
   const InstanceList *shortList;
   if(iterList.num_instances() <= other.iterList.num_instances())
      shortList = &iterList;
   else
      shortList = &other.iterList;

   // Iterate through the list, getting the destinations of each item
   // in the list.  If any destinations conflict, return FALSE
   for(Pix p = shortList->first(); p; shortList->next(p)) {
      const InstanceRC inst = shortList->get_instance(p);

      // find destinations of this and other instance
      ProjKey key(inst, owner.dest_features());
      const ProjInst *thisPI = find(key);
      const ProjInst *otherPI = other.find(key);

      // if both instances are present, then compare destinations
      if(thisPI && otherPI && thisPI->get_destination() !=
	 otherPI->get_destination())	    
	 return TRUE;
   }

   // We say that two projections do not conflict if there are no
   // conflicting destinations present.
   return FALSE;
}

/***************************************************************************
  Description : Updates the source's destination to point to the category
                  to which this projection belongs
  Comments    : Must provide the category since a Projection does not know
                  its category number.
***************************************************************************/
void Projection::update_source(Category newDest)
{
   // go through each ProjInst in this projection, find it in the
   // SOURCE Projection, and update its destination
   // abort on a NULL source pointer
   ASSERT(source);
   for(Pix p=iterList.first(); p; iterList.next(p)) {
      const InstanceRC inst = iterList.get_instance(p);
      ProjKey key(inst, owner.dest_features());
      ProjInst *sourcePI = source->find(key);
      sourcePI->set_destination(newDest);
   }
}


/***************************************************************************
  Description : Projects the instance represented by this Projection down
                  one or more levels, thereby generating a set of
		  Projections.  selector contains the attributes we're
		  projecting downward on and must be in sorted order
		  and also be fully contained in this Projection's
		  projSelector.  NewProjSel and newInstSel are the
		  new selectors for the new level (use project_selector_
		  down to obtain these)
  Comments    :
***************************************************************************/
DynamicArray<Projection *> *Projection::
project_down(const FeatureSet& featureSet, ProjLevel& destLevel) const
{
   // we'll repeatedly split by the attributes mentioned in the selector.
   // each iteration we split all the bags mentioned in bagsToSplit
   // copy the iterList bag here because we need to own it.
   DynamicArray<InstanceBag *> bagsToSplit(1);
   bagsToSplit[0] = iterList.clone();

   // for each attribute in selector, perform a split
   const Array<int>& selector = featureSet.get_feature_numbers();
   for(int i=selector.low(); i<=selector.high(); i++) {
      // set up an attribute categorizer
      AttrCategorizer attrCat(iterList.get_schema(), selector[i], "dummy");

      // create another dynamic array to hold the results
      DynamicArray<InstanceBag *> results(0);

      // split each bag in the list
      for(int bagIndex = bagsToSplit.low(); bagIndex <= bagsToSplit.high();
					    bagIndex++) {
	 BagPtrArray *bpa = bagsToSplit[bagIndex]->split(attrCat);
	 delete bagsToSplit[bagIndex];

	 // add the bags from the split to the results array.  NULL out
	 // the pointers in the array as we do this so that the bpa
	 // destructor won't delete out pointers.  Never add a bag which
	 // has no instances
	 for(int i=bpa->low(); i<=bpa->high(); i++) {
	    if(!(*bpa)[i]->no_instances())
	       results[results.size()] = (*bpa)[i];
	    else
	       delete (*bpa)[i];
	    (*bpa)[i] = NULL;
	 }
	 delete bpa;
      }

      // truncate toSplit array to zero, and append results
      bagsToSplit.truncate(0);
      bagsToSplit.append(results);
   }
      
   // build a Projection from each bag remaining after the split
   DynamicArray<Projection *> *dest =
      new DynamicArray<Projection *>(0);

   for(i=bagsToSplit.low(); i<=bagsToSplit.high(); i++) {
      ASSERT(!bagsToSplit[i]->no_instances());
      Pix p = bagsToSplit[i]->first();
      const InstanceRC& inst = bagsToSplit[i]->get_instance(p);
      (*dest)[i] = new Projection(*(bagsToSplit[i]), destLevel, inst,
				  (Projection *)this);
      delete bagsToSplit[i];
   }

   return dest;
}

/***************************************************************************
  Description : Fill the destinations in the passed in destArray to help
                  the ProjSet know how to connect to the next level
  Comments    : Does a consistency check on the ProjLevel; if we try to
                  write to an entry in the destArray which is filled with
		  a valid destination other then the one we're writing,
		  then we colored the graph incorrectly.
***************************************************************************/
void Projection::fill_destinations(const Categorizer& cat,
				   Array<int>& destArray, Array<int>& counts)
{
   // fill in the destArray with the destinations we find within this
   // Projection
   for(Pix p=iterList.first(); p; iterList.next(p)) {
      ProjKey key(iterList.get_instance(p), owner.dest_features());
      const ProjInst *projInst = find(key);
      const AugCategory aCat = cat.categorize(iterList.get_instance(p));
      if(destArray[aCat] >= 0 &&
	 destArray[aCat] != projInst->get_destination())
	    err << "Projection::fill_destinations: consistency error: "
	    "conflict at category " << aCat << ". "
	    "old value = " << destArray[aCat] << " and new value = "
	    << projInst->get_destination() << fatal_error;
      destArray[aCat] = projInst->get_destination();
      counts[aCat]++;
   }
}

/***************************************************************************
  Description : Displays only the main instance of a projection
  Comments    :
***************************************************************************/
void Projection::display_main_instance(MLCOStream& stream) const
{
   owner.inst_features().display_instance(mainInstance, stream);
}

/***************************************************************************
  Description : Displays all the information contained in a projection;
                  the main instance, and all destinations.
  Comments    :
***************************************************************************/
void Projection::display(MLCOStream& stream) const
{
   DBG(OK());
   
   // display the projected portion of the mainInstance first
   display_main_instance(stream);
   stream << " [";
   owner.inst_features().display_names(iterList.get_schema(), stream);
   stream << "; ";
   owner.dest_features().display_names(iterList.get_schema(), stream);
   stream << endl;
   
   // iterate through the iterList.  Find and display each element
   for(Pix p = iterList.first(); p; iterList.next(p)) {
      ProjKey key(iterList.get_instance(p), owner.dest_features());
      const ProjInst *projInst = find(key);
      ASSERT(projInst);
      stream << "   ";
      projInst->display(owner.inst_features(), owner.dest_features(), stream);
      stream << endl;
   }
}
DEF_DISPLAY(Projection);


