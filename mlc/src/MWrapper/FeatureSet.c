// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The FeatureSet class implements a set of features which may
                   be used to select portions of instances for different
		   purposes.
		 The feature set is implemented as an array of attribute
		   numbers.
  Assumptions  : The array of integers which implements this set should
                   always be sorted.
  Comments     : 
  Complexity   : For all complexity statements, n refers to the number of
                   features in the set.
		 OK() takes O(n) because it verifies that the array is
		   sorted.
		 The copy constructor and the constructor which takes an
		   integer array each take O(n)
		 Operator== takes O(n)
		 Operator= takes O(n)
		 add_feature takes O(n) because we must keep the array
		   sorted.
		 instances_equal takes O(n)
		 both forms of contains() take O(n)
		 all the display functions take O(n)
  Enhancements : Make this class into a full-fledged Set class
                 Use binary search to speed up contains()
  History      : Dan Sommerfield                                   11/18/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <DynamicArray.h>
#include <FeatureSet.h>

RCSID("MLC++, $RCSfile: FeatureSet.c,v $ $Revision: 1.2 $")

/***************************************************************************
  Description : Verify that a selector is in sorted order and contains
                  all elements >= 0
  Comments    :
***************************************************************************/
void FeatureSet::OK() const
{
   int last = -1;
   for(int i=0; i<size(); i++) {
      if(index(i) < 0)
	 err << "FeatureSet::OK: element in set [" << *this << "] "
	    "is less than 0" << fatal_error;
      if(index(i) == last)
	 err << "FeatureSet::OK: duplicate element (" << last << ") in "
	    "set << [" << *this << "]" << fatal_error;
      if(index(i) < last)
	 err << "FeatureSet::OK: set [" << *this << "] is not "
	    "in sorted order" << fatal_error;
      last = index(i);
   }
}

   
/***************************************************************************
  Description : Constructors
  Comments    : The constructor which takes an Array does not use the
                  Dynamic Array's copy constructor becuase there is no
		  constructor for a Dynamic Array which takes an Array.
***************************************************************************/
FeatureSet::FeatureSet()
   : DynamicArray<int> (0)
{
}

FeatureSet::FeatureSet(const FeatureSet& other, CtorDummy)
   : DynamicArray<int> (other, ctorDummy)
{
   DBG(OK());
}

FeatureSet::FeatureSet(const SchemaRC& schema)
   : DynamicArray<int> (schema.num_attr())
{
   for(int i=0; i<schema.num_attr(); i++)
      index(i) = i;
}

FeatureSet::FeatureSet(const Array<int>& featureArray)
   : DynamicArray<int> (0)
{
   append(featureArray);
   sort();
}


/***************************************************************************
  Description : Adds a new feature to the feature set
  Comments    : Keeps the set sorted and aborts on any attempt to add a
                  duplicate feature.
***************************************************************************/
void FeatureSet::add_feature(int featureNum)
{
   // check for negative feature number
   if(featureNum < 0)
      err << "FeatureSet::add_feature: attempting to add negative "
	 "feature number (" << featureNum << ")" << fatal_error;
   
   // figure out the index at which to add the feature
   for(int i=0; i<size() && index(i) < featureNum; i++)
      ; /* NULL */

   // check for duplicates
   if(i<size() && featureNum == index(i))
      err << "FeatureSet::add_feature: attempting to add duplicate "
	 "feature number (" << featureNum << ") to set [" << *this << "]"
	 << fatal_error;

   // move the rest of the array forward
   for(int j=size(); j>i; j--)
      index(j) = index(j-1);

   // insert the new element
   index(i) = featureNum;

   DBG(OK());
}

/***************************************************************************
  Description : Compares two instances using only the features mentioned in
                  this set.
  Comments    :
***************************************************************************/
Bool FeatureSet::instances_equal(const InstanceRC& inst1,
				 const InstanceRC& inst2) const
{
   for(int i=0; i<size(); i++) {
      DBGSLOW(ASSERT(inst1.attr_info(index(i)) == inst2.attr_info(index(i))));
      if(!inst1.attr_info(index(i)).
	 _equal_value(inst1[index(i)], inst2[index(i)]))
	 return FALSE;
   }

   return TRUE;
}


/***************************************************************************
  Description : Check the set for containment of a single element
  Comments    :
***************************************************************************/
Bool FeatureSet::contains(int elem) const
{
   DBGSLOW(OK());
   for(int i=0; i<size() && index(i) <= elem; i++) {
      if(index(i) == elem)
	 return TRUE;
   }
   return FALSE;
}

/***************************************************************************
  Description : Check the set for containment of another set
  Comments    : Can do in O(n) because arrays are sorted
***************************************************************************/
Bool FeatureSet::contains(const FeatureSet& other) const
{
   DBGSLOW(OK());
   DBGSLOW(other.OK());
   int otherIdx = 0;
   for(int i=0; i<size() && otherIdx < other.size(); i++) {
      if(other.index(otherIdx) == index(i))
	 otherIdx++;
   }
   return(otherIdx == other.size());
}


/***************************************************************************
  Description : Find the difference of two feature sets.  Unlike a normal
                  set difference, the "little" set must be completely
		  included in this set.
  Comments    : 
***************************************************************************/
void FeatureSet::difference(const FeatureSet& little, FeatureSet& diff) const
{
   if(size() < little.size())
      err << "FeatureSet::difference: attempting to subtract a larget set "
	 "from a smaller set" << fatal_error;

   DBGSLOW(OK());
   DBGSLOW(little.OK());

   DBG(
      if(!contains(little))
      err << "FeatureSet::difference: This set does not contain the little "
	 "set" << fatal_error;
   );

   // step through big and little together until we find a difference.
   // add each difference to the difference array we created.
   diff.truncate(0);
   
   // because these arrays are sorted, the difference array we return
   // will be sorted too
   int bigIdx = 0, littleIdx = 0;
   while(bigIdx<size()) {
      if(littleIdx >= little.size() ||
	 index(bigIdx) != little.index(littleIdx))
	 diff.index(diff.size()) = index(bigIdx++);
      else {
	 bigIdx++;
	 littleIdx++;
      }
   }

   DBG(diff.OK());
}
	 

/***************************************************************************
  Description : Subtracts this set from the oldProj, and
                  adds it to oldInst at the same time.  newProj
		  and newInst hold the results.
  Comments    : 
***************************************************************************/
void FeatureSet::project_down(const FeatureSet& oldProj,
			      const FeatureSet& oldInst,
			      FeatureSet& newProj,
			      FeatureSet& newInst) const
{
   // verify all input selectors
   DBGSLOW(
      oldProj.OK();
      oldInst.OK();
      OK();
      ASSERT(oldProj.contains(*this));
   );

   newProj.truncate(0);
   newInst.truncate(0);
   
   int newProjIndex = 0;
   int newInstIndex = 0;

   // copy contents of instance selector into newInst.
   // can't use copy constructor because newInst is an already-constructed
   // feature set passed to us.
   for(; newInstIndex < oldInst.size(); newInstIndex++)
      newInst.index(newInstIndex) = oldInst.index(newInstIndex);

   int deltaIndex = 0;
   for(int projIndex = oldProj.low(); projIndex <= oldProj.high();
					  projIndex++) {
      if(deltaIndex < size() &&
	 index(deltaIndex) == oldProj[projIndex])
	 // add this index to the new inst
	 newInst.index(newInstIndex++) = index(deltaIndex++);	 
      else
	 // add this index to the new proj
	 newProj.index(newProjIndex++) = oldProj.index(projIndex);
   }

   // integrity check: if we did not use up the entire delta, then
   // we have a problem
   ASSERT(deltaIndex == size());

   // the new instance array may not be sorted;  sort it now.
   newInst.sort();

   // verify all output selectors
   DBG(
      newInst.OK();
      newProj.OK();
      ASSERT(newInst.contains(*this));
   );
}

/***************************************************************************
  Description : Displays only those attributes of an instance which are
                  mentioned in sel
  Comments    :
***************************************************************************/
void FeatureSet::display_instance(const InstanceRC& inst, MLCOStream& stream)
  const
{
   // display (empty) if the set is empty
   if(size() == 0)
      stream << "(empty)";
   
   // display only those attributes which are mentioned
   else {
      for(int i = 0; i < size(); i++) {
	 stream << inst.attr_info(index(i)).
	    attrValue_to_string(inst[index(i)]);
	 if(i < size()-1)
	    stream << ", ";
      }
   }
}   


/***************************************************************************
  Description : Displays the list of features as a list of names
  Comments    :
***************************************************************************/
void FeatureSet::display_names(const SchemaRC& schema,
			       MLCOStream& stream) const
{
   for(int i=0; i<size(); i++) {
      stream << schema.attr_name(index(i));
      if(i<size()-1)
	 stream << ", ";
   }
}

/***************************************************************************
  Description : Default display: displays as list of numbers
  Comments    :
***************************************************************************/
void FeatureSet::display(MLCOStream& stream) const
{
   DynamicArray<int>::display(stream);
}


DEF_DISPLAY(FeatureSet);



