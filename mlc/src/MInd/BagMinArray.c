#include <basics.h>
#include <BagMinArray.h>
#include <BagAndDistance.h>


/*****************************************************************************
  Description : Integrity check. All the bags must have differnt distances.
  Comments    :
*****************************************************************************/
void BagMinArray::OK(int /* level = 0*/) const
{
   ASSERT((*this)[maxIndex] == maxValue);
   for (int i = 0; i < num_elem(); i++) {
      for (int j = i+1; j < num_elem(); j++) 
	 if ((*this)[i] == (*this)[j])
	    err << "BagMinArray::OK : same distances have different bags:"
	        << " The first element : " << (*this)[i] << " The second"
	           " element : " << (*this)[j] << fatal_error;
      if ((*this)[i] > maxValue ) 
	 err << "BagMinArray::OK : some array element is greater than"
	     << "the maximum value : " << i << fatal_error;
   }
}	       


/*****************************************************************************
  Description : Inserts instance and distance. If the given distance Bag
                  exists, it just adds the instance to the bag. Otherwise,
		  it creates a bag and inserts it.
  Comments    : Time critical.  
*****************************************************************************/
void BagMinArray::insert(const InstanceRC& inst, Real distance)
{
   // BagAndDistance elem() is extremely expensive, so we try
   //   to be smart and delay it if possible.
   if (numMinElements < size()) {
      if (numMinElements == 0) {
         BagAndDistance elem(inst, distance);
	 maxValue = elem;
	 maxIndex = 0;
         (*this)[numMinElements++] = elem;
	 return;
      }
      if (distance > maxValue.get_distance()) {
         BagAndDistance elem(inst, distance);
	 maxValue = elem;
	 maxIndex = numMinElements;
         (*this)[numMinElements++] = elem;
	 return;
      }
      else {  // Less than max.  Can we find this distance?
	 for (int i = 0; i < numMinElements; i++) {
	    if (operator[](i).get_distance() == distance) {
	       operator[](i).insert(inst, distance);
	       return;
	    }
	 }
         // can't find it, so add it.
         BagAndDistance elem(inst, distance);
         (*this)[numMinElements++] = elem;
      }
      return;
   } else if (distance > maxValue.get_distance())
      return; // common case.
   else if (distance == maxValue.get_distance()) {
      operator[](maxIndex).insert(inst, distance);
   }
   else {
      ASSERT(distance < maxValue.get_distance());
      // Can we find it?
      for (int i = 0; i < numMinElements; i++)
	 if (operator[](i).get_distance() == distance) {
	    operator[](i).insert(inst, distance);
	    return;
	 }      
      // No, so let's replace max index with this.
      BagAndDistance elem(inst, distance);
      operator[](maxIndex) = elem;
      maxValue = max(maxIndex); // note this updates maxIndex
   }
   DBGSLOW(OK(0));      
}


/*****************************************************************************
  Description : Displays array.
  Comments    :
*****************************************************************************/
void BagMinArray::display(MLCOStream& stream, Bool normalizeReal) const
{
   stream << "The number of elements in BagMinArray : "  << num_elem() << endl;
   for (int i = 0; i < num_elem(); i++) {
      index(i).display(stream, normalizeReal);
      stream << endl;
   }
}

DEF_DISPLAY(BagMinArray)


/*****************************************************************************
  Description : Computes the majority category of the BagMinArray based on
                 the IBInducer and FastIBInducer characteristics of
		 nnk-value (whether to count each instance or each
		 unique instance) and neighbor vote (whether each
		 votes equally or based on distance).
  Comments    : 
*****************************************************************************/
Category BagMinArray::major_category(const IBInducer::NeighborVote vote,
					   const IBInducer::NnkValue nnkValue)
     const
{
  if (vote == IBInducer::inverseDistance) {
    int numCategory = (*this)[base].get_bag().get_schema().num_label_values();
    Array<Real> categoryArray(0,numCategory,0);
    
    int count = 0;
    for (int i = 0; i < num_elem(); i++) {
      const InstanceBag& bag = (*this)[i].get_bag();
      Real dist = (*this)[i].get_distance();
      if (dist == 0)
	return bag.majority_category();
      else 
	for (Pix pix = bag.first(); pix; bag.next(pix)) {
	  const InstanceRC& inst = bag.get_instance(pix);
	  const AttrValue_& label = inst.get_label();
	  Category cat = inst.label_info().get_nominal_val(label);
	  categoryArray[cat - FIRST_CATEGORY_VAL] += 1/dist;
	  if (nnkValue == IBInducer::numNeighbors && count++ >= num_elem()) 
	    goto returnCategory; // OK, ok it is ugly goto but nice
	  // and efficient solution.
	  // I think this may be one of
	  // nine cases Knuth metioned where
	  // goto statement is a nice solution.
	}
    }
  returnCategory:  
    int cat;
    lose_lsb_array(categoryArray);
    categoryArray.max(cat);
    GLOBLOG(3, categoryArray << endl);
    GLOBLOG(3, "Diff is : " << categoryArray[0] - categoryArray[1] <<
	    " REAL_EPSILON is : " << REAL_EPSILON << endl);
    return FIRST_CATEGORY_VAL + cat;
  }
  else {
    // choose the majority category of the k nearest neighbors.
    InstanceBag temp((*this)[0].get_bag().get_schema());
    //LOG(3, (*this));
    int count = 0;
    for (int i = 0; i < num_elem(); i++) {
      const InstanceBag& bag = (*this)[i].get_bag();
      for (Pix pix = bag.first(); pix; bag.next(pix)) {
	temp.add_instance(bag.get_instance(pix));	    
	if (nnkValue == IBInducer::numNeighbors && count++ >= num_elem())
	  return temp.majority_category();
      }
    }
    return temp.majority_category();
  }
  
}
