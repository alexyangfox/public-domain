// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/******************************************************************************
  Description  : IBCategorizer is a structure to categorize a given instance's
                    class(label) based on its structure. The structure is
		    formed by IBInducer.
  Assumptions  :
  Comments     :
  Enhancements :
  History      : Chia-Hsin Li
                   Added operator==                             10/10/94
                 Yeogirl Yun
                   Initial Revision (.c .h)                     9/09/94
******************************************************************************/


#include <basics.h>
#include <IBCategorizer.h>
#include <DisplayPref.h>
#include <InstanceRC.h>
#include <MLCStream.h>
#include <BagMinArray.h>
#include <IBInducer.h>

const int DEFAULT_K_VAL = 1;


/*****************************************************************************
  Description : Checks that the number of instances in instStore and in the
                  hash table be the same.
  Comments    : 
*****************************************************************************/
void IBCategorizer::OK(int level) const
{
   (void)level;
   if (instStore.num_instances() != hashTable.num_instances())
      err << "IBCategorizer::OK: # of instances in instSore and in hashTable "
	     "are not the same. ( # instSore " << instStore.num_instances()
	  << " : # hashTable " << hashTable.num_instances() << " ) " <<
	 fatal_error;
}



/*****************************************************************************
   Description : Returns the distance of real value between the two given
                   instances.
		 It uses the normalized Euclidean distance.
   Comments    : Protected member.
*****************************************************************************/
Real IBCategorizer::distance(const InstanceRC& inst1,
			     const InstanceRC& inst2) const
{
   Real dist = 0;
   const SchemaRC& schema = instStore.get_schema();
   int  numAttr = schema.num_attr();
   for (int i = 0; i < numAttr; i++)
      if (weightVector->index(i) != 0)
	 dist += squareReal(weightVector->index(i) *
			    schema.attr_info(i).distance(inst1[i], inst2[i])); 
   return dist;
}




/******************************************************************************
  Description  : Initialize IBategorizer by relaying the given
                   InstanceBag, default category value and
		   description up to TableCategorizer. Default
		   category value is UNKNOWN_CATEGORY_VAL for all IBL
		   algorithms.
  Comments     :
******************************************************************************/
IBCategorizer::IBCategorizer(const MString& dscr,
			     const InstanceBag& bag)
: TableCategorizer(bag,UNKNOWN_CATEGORY_VAL,dscr),
  instStore(bag.get_schema()),
  vote(IBInducer::inverseDistance),
  nnkValue(IBInducer::numDistances)
{
   weightVector = new Array<Real>(0, bag.num_attr(),1.0);
   set_k_val(DEFAULT_K_VAL);
   // to avoid ownership transfer, InstList(InstanceBag*&) is not used.
   // this is consistent with the dummy copy constructor below where no such
   // ownership transfer is possible(as far as I know).
   for (Pix pix = bag.first(); pix; bag.next(pix))
      instStore.add_instance(bag.get_instance(pix));
}



/******************************************************************************
  Description  : Copy constructor with a dummy argument.
  Comments     :
******************************************************************************/
IBCategorizer::IBCategorizer(const IBCategorizer& source,
			     CtorDummy)
: TableCategorizer(source, ctorDummy),
  instStore(source.instStore.get_schema()),
  kVal(source.kVal),
  vote(source.vote),
  nnkValue(source.nnkValue)
{
   weightVector = new Array<Real>(*source.weightVector, ctorDummy);
   for (Pix pix = source.instStore.first(); pix; source.instStore.next(pix))
      instStore.add_instance(source.instStore.get_instance(pix));
}


/******************************************************************************
  Description  : Destructor.
  Comments     :
******************************************************************************/

IBCategorizer::~IBCategorizer()
{
   delete weightVector;
}
   

/******************************************************************************
  Description  : Prints a readable representation of the categorizer to the
                  given stream.
  Comments     :
******************************************************************************/
void IBCategorizer::display_struct(MLCOStream& stream,
				      const DisplayPref& dp) const
{
   if (dp.preference_type() == DisplayPref::ASCIIDisplay)
   stream << "IBCategorizer " << description() << endl
	  << "with the following labelled instances"
	     " in the Concept Description:" << endl;
   instStore.display(stream);
}




/******************************************************************************
  Description  : Categorize the given instances' label based on weightVector,
                   instStore and TableCategorizer methods.
  Comments     : It takes O(n) where n is the number of instances in instSore.
******************************************************************************/
AugCategory IBCategorizer::categorize(const InstanceRC& instance) const
{
   Category cat;

   BagMinArray minArray(get_k_val());

   IFLOG(2, get_log_stream() << "Given Instance (before normalization): ";
	 InstanceRC normInst(instance);
	 normInst.set_schema(instStore.get_schema());
	 normInst.display(get_log_stream(), FALSE, FALSE, FALSE);
         get_log_stream() << "Given Instance (after normalization): ";
         normInst.display(get_log_stream(), FALSE, FALSE, TRUE););
   
   // collect k nearest neighbors.
   for (Pix pix = instStore.first(); pix; instStore.next(pix)) {
      InstanceRC inst(instStore.get_instance(pix));
      // update schema if we're going to display normalized instances
      //    after the loop.  Note that this is a copy-on-write operation
      //    so we don't want it executed if the log level is not high.
      IFLOG(3, inst.set_schema(instStore.get_schema()));
      Real dist = distance(inst,instance);
      minArray.insert(inst, dist);
   }

   LOG(3, "IBCategorizer::categorize: nearest neighbors: " << endl
       << minArray << endl);
   LOG(3, "Normalized nearest neighbors:" << endl);
   IFLOG(3, minArray.display(get_log_stream(), TRUE));
   LOG(3, endl);
   
   cat = major_category(minArray);
   static MString strCategory;
   strCategory = instStore.get_schema().category_to_label_string(cat);
   IFLOG(2, get_log_stream() << "Categorized value : " << strCategory <<
	 endl;); 
   return AugCategory(cat,strCategory);	    
}	    
	    

/*****************************************************************************
  Description : Adds an instance into instStore and hashTable.
  Comments    : 
*****************************************************************************/
void IBCategorizer::add_instance(const InstanceRC& inst)
{
   OK();
   TableCategorizer::add_instance(inst);
   instStore.add_instance(inst);
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of this IBCategorizer.
  Comments    :
***************************************************************************/
Categorizer* IBCategorizer::copy() const
{
   return new IBCategorizer(*this, ctorDummy);
}



/*****************************************************************************
  Description : Sets the weight vector.
  Comments    : 
*****************************************************************************/
void IBCategorizer::set_weights(const Array<Real>& weights)
{
   delete weightVector;
   weightVector = new Array<Real>(weights, ctorDummy);
}


/***************************************************************************
  Description : set number of nearest neighbors.
  Comments    :
***************************************************************************/
void IBCategorizer::set_k_val(int k)
{
   if (k <= 0)
      err << "IBCategorizer::set_k_val() : illegal k value : " << k 
	  << fatal_error;
   kVal = k;
}

/***************************************************************************
  Description : Returns TRUE if (*this == cat)
  Comments    :
***************************************************************************/
Bool IBCategorizer::operator==(const Categorizer& ) const
{
   err << "IBCategorizer::operator==: not implemented" << fatal_error;
   return FALSE;
}

Bool IBCategorizer::operator==(const IBCategorizer& ) const
{
   err << "IBCategorizer::operator==: not implemented" << fatal_error;
   return FALSE;
}



/*****************************************************************************
  Description : Returns the majority category based on neighbor vote and
                  nnk value. 
  Comments    :
*****************************************************************************/
Category IBCategorizer::major_category(const BagMinArray& minArray) const
{
   if (vote == IBInducer::inverseDistance) {
      int numCategory = instStore.get_schema().num_label_values();
      Array<Real> categoryArray(0,numCategory,0);

      int count = 0;
      for (int i = 0; i < minArray.num_elem(); i++) {
	 const InstanceBag& bag = minArray[i].get_bag();
	 Real dist = minArray[i].get_distance();
	 if (dist == 0)
	    return bag.majority_category();
	 else 
	    for (Pix pix = bag.first(); pix; bag.next(pix)) {
	       const InstanceRC& inst = bag.get_instance(pix);
	       const AttrValue_& label = inst.get_label();
	       Category cat = inst.label_info().get_nominal_val(label);
	       categoryArray[cat - FIRST_CATEGORY_VAL] += 1/dist;
	       if (nnkValue == IBInducer::numNeighbors && count++ >= kVal) 
		  goto returnCategory; // OK, ok it is ugly goto but nice
	                               // and efficient solution.
	                               // I think this may be one of
	                               // nine cases Knuth metioned where
	                               // goto statement is a nice solution.
	    }
      }
      returnCategory:  
      int cat;
      categoryArray.max(cat);
      return FIRST_CATEGORY_VAL + cat;
   }
   else {
         // choose the majority category of the k nearest neighbors.
      InstanceBag temp(instStore.get_schema());
      LOG(3, minArray);
      int count = 0;
      for (int i = 0; i < minArray.num_elem(); i++) {
	 const InstanceBag& bag = minArray[i].get_bag();
	 for (Pix pix = bag.first(); pix; bag.next(pix)) {
	    temp.add_instance(bag.get_instance(pix));	    
	    if (nnkValue == IBInducer::numNeighbors && count++ >= kVal)
	       return temp.majority_category();
	 }
      }
      return temp.majority_category();
   }
}
