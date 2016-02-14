// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The RealDiscretizor class is an ABC for discretization
                   of continuous attributes. Using the
		   RealAttrInfo, the RealDiscretizor creates a
		   NominalAttrInfo. The thresholds array created
		   by build_thresholds defines the discretization intervals
		   (a pure virtual function).
		 Once the thresholds array has been created via
		   create_thresholds() and a NominalAttrInfo has been
		   created with real_to_nominal_info(), the user can obtain
		   the Nominal Attribute value of the real attribute within
		   the instance by calling discretize_real_val() with the
		   Instance to be discretized.
		 The RealDiscretizor requires that all derived classes define
		   the method create_thresholds() in order to allocate and
		   initialize the values within the threshold array.
		   It also requires that the first line of code executed
		   within this method is the invariant validation routine
		   check_thresholds().
		 discretize_real_val() finds the appropriate range that a
		   real attribute would lie within and returns the index
		   (as a NominalAttrValue_) that matches the NominalAttrInfo
		   in the discretized schema.
		 display() outputs the values of the thresholds array as
		   ranges. It assumes that the thresholds will always have the
		   endpoints of -INF, +INF. 

  Assumptions  : 
  Comments     : Unknowns remain unknowns even if there are zero thresholds.
  
  Complexity   : OK() takes O(numBins)
                 real_to_nominal_info() takes O(numBins)
		 discretize_real_val() takes O(numBins)
		 display() takes O(numThresholds)
		 gen_discretized_schema() takes O(numBins * numAttr's)
		 discretize_bag() takes
		 O(numInstances * numAttributes * numBins)
		 
  Enhancements : Binary search for threshold value.
  History      : James Dougherty                                      11/11/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <RealDiscretizor.h>
#include <Attribute.h>
#include <math.h>

RCSID("MLC++, $RCSfile: RealDiscretizor.c,v $ $Revision: 1.16 $")

/***************************************************************************
  Description : copy helper method
  Comments    : protected method
***************************************************************************/
void RealDiscretizor::base_copy(const RealDiscretizor& source)
{
   DBG(ASSERT(&source != this));
   descr = source.descr;
   attrNum = source.attrNum;

   delete nai, nai = NULL;
   if ( source.nai)
      nai = &source.nai->clone()->cast_to_nominal();

   delete rai, rai = NULL;
   if (source.rai)
      rai = &source.rai->clone()->cast_to_real();

   delete thresholds, thresholds = NULL;
   if (source.thresholds)
      thresholds = new
	 DynamicArray<RealAttrValue_>(*source.thresholds, ctorDummy);
}


/***************************************************************************
  Description : Checks that the caller has not already allocated the
                 thresholds array or the nai; used by create_thresholds()
  Comments    : protected method
***************************************************************************/
void RealDiscretizor::check_thresholds()
{
   if (thresholds || nai)
      err << "RealDiscretizor::check_thresholds: "
	     "method cannot be called twice. "
	  << fatal_error;
}


/***************************************************************************
  Description : Shallow equal method, returns true if the base class
                  member variables are equal to the sources.
  Comments    : Helper for defining operator ==, protected
***************************************************************************/
Bool RealDiscretizor::equal_shallow(const RealDiscretizor& src)
{
   // Bool to avoid CC warning
   if (Bool(thresholds != NULL) ^ Bool(src.thresholds!= NULL))
      return FALSE;

   if (thresholds && src.thresholds){
      if (src.thresholds->size() != thresholds->size())
	 return FALSE;
      else
	 for(int i = 0; i < thresholds->size(); i++)
	  if(!rai->_equal_value(thresholds->index(i),src.thresholds->index(i)))
	     return FALSE;
   }
   
   if (Bool(nai != NULL) ^ Bool(src.nai != NULL))
      return FALSE;

   if (nai && src.nai && !nai->equal(*src.nai, FALSE)) //not fatal on false
      return FALSE;

   if (Bool(rai != NULL) ^ Bool(src.rai !=NULL))
      return FALSE;

   if (rai && src.rai && !rai->equal(*src.rai, FALSE))
      return FALSE;
   
   return descr == src.descr &&
          attrNum == src.attrNum;
}


/***************************************************************************
  Description : RealDiscretizor constructor.
  Comments    :
***************************************************************************/
RealDiscretizor::RealDiscretizor(int attributeNumber, const SchemaRC& schema)
   : attrNum(attributeNumber),
     nai(NULL),
     rai(new RealAttrInfo(schema.attr_info(attrNum).cast_to_real(),ctorDummy)),
     descr(rai->name()),
     thresholds(NULL)
{}

/***************************************************************************
  Description : Copy constructor
  Comments    :
***************************************************************************/
RealDiscretizor::RealDiscretizor(const RealDiscretizor& source,
				 const CtorDummy /* dummyArg */)
   :nai(NULL),
    rai(NULL),
    thresholds(NULL)
{
   base_copy(source);
}


/***************************************************************************
  Description : Destructor
  Comments    :
***************************************************************************/
RealDiscretizor::~RealDiscretizor()
{
   delete thresholds;
   delete nai;
   delete rai;
}


/***************************************************************************
  Description :  Checks that the values in the thresholds array are strictly
                   increasing.
  Comments    :
***************************************************************************/
void RealDiscretizor::OK(int /*level */) const
{
   if( NULL != thresholds)
      for(int i = 0; i < thresholds->size() - 1; i++)
	 if(! rai->less_than(thresholds->index(i), thresholds->index(i+1)))
	    err << "RealDiscretizor::OK() thresholds are not strictly "
	           "increasing" << fatal_error;
}


/***************************************************************************
  Description : Invokes the protected pure virtual function build_thresholds
                  to create the thresholds array after  checking  that the
		  Discretizor is in a safe state via. check_thresholds.
  Comments    : 
***************************************************************************/
void RealDiscretizor::create_thresholds(const InstanceBag& source)
{
   check_thresholds();
   build_thresholds(source); //Invoke protected pure-virtual (PPV) method
}


/***************************************************************************
  Description : Create a new thresholds array and copy the Real values
                  into it. This function serves as an easy interface to
		  create_thresholds. If you have some thresholds which
		  you would like to create, simply create an Array<Real>,
		  pass it to this baby and it will sort it and create
		  RealAttrValue_s for you.
  Comments    : Caller should sort the array first, otherwise OK() will fail.
***************************************************************************/
void RealDiscretizor::create_real_thresholds(Array<Real>& newVals)
{
   thresholds = new DynamicArray<RealAttrValue_>(newVals.size());
   for(int i = 0; i < newVals.size(); i++)
      rai->set_real_val(thresholds->index(i), newVals.index(i));
   DBG(OK());
}



/***************************************************************************
  Description : Creates the Nominal Attribute Info to be transferred to the
                  bag being discretized.
                If REMAP_PERIODS is yes, we map periods to @ to avoid
		  the period signalling end of line. 
  Comments    :
***************************************************************************/

NominalAttrInfo* RealDiscretizor::real_to_nominal_info() const
{
   DBG(OK());
   DblLinkList<MString>* attrVals = new DblLinkList<MString>;
   MString firstAttr;

   if (thresholds->size() == 0)
      firstAttr = "ignore"; // C4.5 has a bug if only one attribute value is
			    // given on the line.
   else
      firstAttr = "-Inf-" + 
	 rai->attrValue_to_string(thresholds->index(0));
   attrVals->append(firstAttr);

   for ( int binNum = 0; binNum < thresholds->size(); binNum++) {
      MString upperValue("Inf");
      if (binNum < thresholds->size()-1)
	 upperValue = rai->attrValue_to_string(thresholds->index(binNum + 1));
      MString newVal(rai->attrValue_to_string(thresholds->index(binNum)) +
		     "-" + upperValue);
      attrVals->append(newVal);
   }
   NominalAttrInfo* nominalAttrInfo = new NominalAttrInfo(descr, attrVals);
   return nominalAttrInfo;
}


/***************************************************************************
  Description : Given a RealAttrValue in Instance inst at index index, this
                  method sets the Attribute Value to the new discretized
		  NominalAttrValue.
		There may be instances in the test set for which the Real
                  Attribute values may be out of range so the Thresholds must
		  be <X0, X0-X1, X1-X2, ..., Xn-2-Xn-1, >Xn-1 to guarantee that
		  everything is discretized.
  Comments    :      
***************************************************************************/
NominalAttrValue_ RealDiscretizor::discretize_real_val(const InstanceRC& inst)
const
{
   DBGSLOW(OK());
   NominalAttrValue_ attrValue;
   const RealAttrValue_& rav = inst[attrNum];

   //If the NominalAttributeInfo has not been allocated, make it here.
   //This method is logically const, so we cast the constness away
   if(!nai)
      ((RealDiscretizor*) this)->nai = real_to_nominal_info();

   //Unknowns get mapped to UNKNOWN_NOMINAL_VALUE
   if(inst.attr_info(attrNum).is_unknown(rav))
      nai->set_unknown(attrValue);
   else
      if(thresholds->size() == 0 || rai->less_than_equal(rav,
							 thresholds->index(0)))
	 nai->set_nominal_val(attrValue, 0);
      else {
	 for(int i = 0; i < thresholds->size() &&
		      rai->greater_than(rav,thresholds->index(i)); i++)
	    ; //NULL Body
	 nai->set_nominal_val(attrValue, i); 
      }
   return attrValue; 
}


/***************************************************************************
  Description : Displays the nominalAttrInfo we created using the
                 Thresholds, displays the Thresholds we used to create the
		 nominalAttrInfo.
  Comments    :
***************************************************************************/
void  RealDiscretizor::display(MLCOStream& stream) const
{
   DBG(OK());
   stream << endl <<"Discretization of Attribute: \"" <<  descr << "\""
	  << endl;
   if(thresholds->size() != 0){
      stream << endl << "Bin0: (-INF, "
	     << rai->attrValue_to_string(thresholds->index(0)) << ")" <<  endl;
      for(int i = 0; i < thresholds->size() - 1 ; i++)
	 stream << "Bin" << i + 1 <<": ["
		<< rai->attrValue_to_string(thresholds->index(i)) << ","
		<< rai->attrValue_to_string(thresholds->index(i+1)) << ")"
		<< endl;
      stream << "Bin" << thresholds->size() << ": ["
	     << rai->attrValue_to_string(thresholds->index(i)) << ","
	     << "+INF)" << endl;
   } else {
      stream << "* Note: attribute \"" << descr << "\" had only one value\n"
	     << "  (only one bin was used). " << endl;
      stream << endl << "Bin0: (-INF, +INF)" << endl;
   }
}


/***************************************************************************
  Description : Returns the number of intervals chosen, aborts if 
                  create_thresholds has not been called.
  Comments    :
***************************************************************************/
int RealDiscretizor::num_intervals_chosen()
{
   if (!thresholds)
      err << "RealDiscretizor:num_intervals_chosen: create_thresholds"
	  << " must be called first." << fatal_error;
   return thresholds->size() + 1;
}


/***************************************************************************
  Description : Generates a discretized schema from an InstanceBag's old
                  schema and Array<RealDiscretizors*>*
  Comments    : The Array<RealDiscretizor*>* needs to be allocated with
                  a utility function before this call is made.
***************************************************************************/
SchemaRC gen_discretized_schema(const InstanceBag& bag,
				PtrArray<RealDiscretizor*>* const disc)
{
   SchemaRC schema = bag.get_schema();
   DblLinkList<AttrInfoPtr>* attrInfo = new DblLinkList<AttrInfoPtr>;

   for(int k = 0; k < schema.num_attr(); k++){
      const AttrInfo& ai = schema.attr_info(k);
      if ( ai.can_cast_to_nominal() )    //Nominal, just put in the list.
	 attrInfo->append(ai.clone());
      else if ( ai.can_cast_to_real())   //Discretize first, then put in list
	 attrInfo->append( (*disc)[k]->real_to_nominal_info());
      else 
	 err << "Unrecognized attribute type" << fatal_error;
   }
   AttrInfo* labelInfo =  bag.label_info().clone();
   SchemaRC newSchema(attrInfo, labelInfo);
   return newSchema;
}


/***************************************************************************
  Description : Discretizes a single instance using the discretizors
                  and the discretized schema.
  Comments    :
***************************************************************************/
void discretize_instance(const PtrArray<RealDiscretizor*>* const disc,
			 const SchemaRC& discretizedSchema,
			 InstanceRC& instance)
{
   for(int attrNum = 0; attrNum < instance.num_attr(); attrNum++) {
      //And each attribute of the instance
      if( instance.attr_info(attrNum).can_cast_to_real() ) {
	 // If we have a real attribute value we can discretize
	 // Check that there is a RealDiscretizor
	 ASSERT( NULL != disc->index(attrNum)); 
	 // And have the RealDiscretizor change the Instances'
	 // AttrValue_ from a RealAttrValue_ to a NominalAttrValue_
	 // with the correct index value of the schema so that it
	 // references the interval which the real value
	 // would lie within.
	 NominalAttrValue_ nav =
	    disc->index(attrNum)->discretize_real_val(instance);
	 instance[attrNum] = nav;
      }
   }
   //Remap schema of new instance
   instance.set_schema(discretizedSchema); 
}   




/***************************************************************************
  Description : Creates a new discretized InstanceBag* from the sourceBag
                  and discretized Schema using the RealDiscretizors.
  Comments    :
***************************************************************************/
InstanceBag* discretize_bag(const InstanceBag& sourceBag,
			    PtrArray<RealDiscretizor*>* const disc)
{
   const SchemaRC newSchema = gen_discretized_schema(sourceBag, disc);
   InstanceBag* newBag = sourceBag.create_my_type(newSchema);
   
   // The schema's and the Array of discretizors always
   // have to be the same size
   ASSERT(newSchema.num_attr() == disc->size());
   ASSERT(newSchema.num_attr() == sourceBag.get_schema().num_attr());
   const SchemaRC& schema = newBag->get_schema();
   
   for(Pix pix = sourceBag.first(); pix; sourceBag.next(pix)) {
      InstanceRC newInst = sourceBag.get_instance(pix);
      //discretize each instance
      discretize_instance(disc, schema, newInst);
      // Add to InstanceBag
      newBag->add_instance(newInst); 
   }
   return newBag;
}


/***************************************************************************
  Description : Determines the number of distinct values for a continuous
                  attrNum.
  Comments    :
***************************************************************************/
int num_distinct_vals(const InstanceBag& bag, int attrNum)
{
   if (!bag.get_schema().attr_info(attrNum).can_cast_to_real())
      err << "num_distinct_vals: requesting number of distinct continuous "
	  << "values from an attr_info which is NOT continuous."
	  << fatal_error;

   DynamicArray<Real> realVals(bag.num_instances());
   int c = realVals.low();
   for (Pix pix = bag.first(); pix; bag.next(pix)) {
      const InstanceRC& instance = bag.get_instance(pix);
      if (!bag.attr_info(attrNum).is_unknown(instance[attrNum]))
	 realVals[c++] =bag.attr_info(attrNum).get_real_val(instance[attrNum]);
   }
   realVals.truncate(c);
   realVals.sort();
   Real val = UNKNOWN_REAL_VAL;
   int numValues = 0;
   for (c = realVals.low(); c <= realVals.high(); c++)
      if (val != realVals[c]) {
	 numValues++;
	 val = realVals[c];
      }
   return numValues;
}

DEF_DISPLAY(RealDiscretizor);





