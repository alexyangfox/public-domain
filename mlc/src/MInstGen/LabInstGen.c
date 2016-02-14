// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : LabelledInstanceGenerator generates instances
                   according to the given specifications. The user
		   provides the function that generates labelled
		   instances (but with an unset label) and the
		   function that generates the label value and the
		   noisy label value.  
		 IndependentLabInstGenerator generates instances with
		   independent attributes (i.e. the value of each
		   attribute of the labelled instance does not depend
		   on the value of any other attribute). The default
		   attribute generators implement uniform distribution
		   for Nominal Attributes. The user can provide other
		   attribute generators.
  Assumptions  : 
  Comments     : 
  Complexity   : LabelledInstanceGenerator::get_lab_inst_no_label()
                   and LabelledInstanceGenerator::get_labelled_instance()
		   take O(labInstGenFunctor) time.
		 IndependentLabInstGenerator constructor takes O(number
		   of attributes in lii) time.
		 IndependentLabInstGenerator::gen_attr_value() takes
		   O(attrGenFunctors[i]) time where i is the index of
		   the attribute for which a value is generated.
  Enhancements : Implement uniform distribution for bounded reals and
                   Gaussian distribution for reals.
		 Support noisy attributes.
                 We may wish to reword some functions here now that
		    Instances may be labelled or unlabelled.
  History      : David Manley                                     2/94
                   Added full_space_generator()
                 Svetlozar Nestorov                             1/4/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <BagSet.h>
#include <LabInstGen.h>


RCSID("MLC++, $RCSfile: LabInstGen.c,v $ $Revision: 1.12 $")

/***************************************************************************
  Description : Generates and returns a label for the given instance.
                  The label is noisy with probability noiseRate percent.
  Comments    : It does NOT set the label in the instance.
                Private member.
***************************************************************************/
AttrValue_ LabelledInstanceGenerator::generate_label
                                       (const InstanceRC& inst)  
{
   AttrValue_ label, noisyLabel;
   labelGenFunctor(inst, label, noisyLabel);
   // No equality so we won't flip sign on 0 noiseRate.
   return ((randNumGen.real(0, 1) < noiseRate) ? noisyLabel : label);
}


/***************************************************************************
  Description : Constructor.
                Sets noiseRate to 0.
  Comments    : 
***************************************************************************/
LabelledInstanceGenerator::LabelledInstanceGenerator
                            (const SchemaRC& aSchema,
			     LabInstGenFunctor& instFunctor,
			     LabelGenFunctor& labFunctor)
: schema(aSchema), labInstGenFunctor(instFunctor), labelGenFunctor(labFunctor)
{
   set_label_noise(0);
}


/***************************************************************************
  Description : Sets the current level of label noise to be "rate".
  Comments    : The range of rate is [0,1].
***************************************************************************/
void  LabelledInstanceGenerator::set_label_noise(Real rate)
{
   if (rate<0 || rate>1)
      err << "LabelledInstanceGenerator::set_label_noise: "
	     "The rate "  << rate << " is out of range(0-1)"
	  << fatal_error;
   noiseRate = rate;
}


/***************************************************************************
  Description : Initializes the MRandom member of the object with the
                  given seed. 
  Comments    : Could be used to repeat a random sequence of labelled
                  instances.
***************************************************************************/
void LabelledInstanceGenerator::set_seed(int seed)
{
   randNumGen.init(seed);
}


/***************************************************************************
  Description : Creates and returns a pointer to LabelledInstanceAlloc
                  using the given distribution implemented by instanceGenFun
		  but does NOT set the label.
  Comments    : The caller gets ownership of the LabelledInstanceAlloc.
***************************************************************************/
InstanceRC LabelledInstanceGenerator::get_lab_inst_unset_label()
{
   return labInstGenFunctor();
}


/***************************************************************************
  Description : Creates and returns a pointer to LabelledInstanceAlloc.
                Sets the label using the label-generating function and
		  the noiseRate.  
  Comments    : The caller gets ownership of the LabelleInstanceAlloc.
***************************************************************************/
InstanceRC LabelledInstanceGenerator::get_labelled_inst() 
{
   InstanceRC labInst = get_lab_inst_unset_label();
   labInst.set_label(generate_label(labInst));
   return labInst;
}


/***************************************************************************
  Description : Constructor.
                Initializes the functions that generate the attribute values
		  to implement a uniform distribution.
		Sets the noiseRate to 0.
  Comments    : 
***************************************************************************/
IndependentLabInstGenerator::IndependentLabInstGenerator
                                        (const SchemaRC& schema,
					 LabelGenFunctor& labFunctor)
: LabelledInstanceGenerator(schema,
     *new IndependentLabInstGenFunctor(schema, *this), labFunctor),  
   attrGenFunctors(schema.num_attr())
{
   for (int i= attrGenFunctors.low(); i <= attrGenFunctors.high(); i++)
      attrGenFunctors[i] = new UniformAttrGenFunctor(randNumGen);   
} 



/***************************************************************************
  Description : Destructor.  
  Comments    : Deallocates the LabInstGenFunctor allocated in the
                  constructor. 
***************************************************************************/
IndependentLabInstGenerator::~IndependentLabInstGenerator()
{
   delete &labInstGenFunctor;
}


/***************************************************************************
  Description : Sets the attribute value-generating function for the
                  attrNum attribute to be "functor". 
  Comments    : Gets ownership of the AttrGenFunctor.
***************************************************************************/
void IndependentLabInstGenerator::set_attr_generator(int attrNum,
						     AttrGenFunctor*& functor)
{
   if (!functor)
      err << "IndependentLabInstGenerator::set_attr_generator: The "
             "AttrGenFunctor passed in is NULL" << fatal_error;
   delete attrGenFunctors[attrNum];
   attrGenFunctors[attrNum] = functor;
   functor = NULL;
}


/***************************************************************************
  Description : Generates a value for an attribute using the respective
                  function in attrGenFunctors array.
  Comments    : If the AttrInfo is not a member of the InstanceInfo stored
                  internally a fatal_error occurs.
	        This function is public because classes derived from
		  IndependentLabInstGenerator will use it to access
		  the private array of AttrGenFunctors - attrGenFunctors.
		Another reason, for making this function public, is the
		  fact that it is used by IndependentLabInstGeneratorFunctor.
***************************************************************************/
AttrValue_ IndependentLabInstGenerator::gen_attr_value
                                     (const AttrInfo& ai, int attrNum) const
{
   if (!schema.member(ai))
       err << "IndependentLabInstGenerator::gen_attr_value: The AttrInfo"
	      " passed is not in the InstanceInfo of the"
	      " IndependentLabInstGenerator" << fatal_error;
   return (*(attrGenFunctors[attrNum]))(ai);
}


/***************************************************************************
  Description : Adds numInstances of LabelledInstances to the given bag.
                The labelled instances are generated by a
		  LabelledInstanceGenerator which is passed as a parameter.
		Duplications are allowed iff the boolean allowDup is TRUE.
  Comments    : The bag gets ownership of the instances.
                If the noise rate of the LabelledInstanceGenerator
		  is greater than 0 and the allowDup is FALSE a fatal
		  error is generated because we consider these two
		  specifications conflicting for the following reason.
		  If an instance with a noisy label is generated then
		  we will never be able to generate the same instance
		  with the correct label. 
		If allowDup == TRUE the function takes O(numInstances).
		Otherwise the complexity depends on the labelled
		  instance generating functor (a private member of
		  LabelledInstanceGenerator--labInstGenFunctor).
		Potentially, the data_generator could take infinite
		  amount of time.
***************************************************************************/
void data_generator(InstanceBag& bag, int numInstances,
		    LabelledInstanceGenerator& lig, Bool allowDup)
{
   if (lig.get_label_noise() > 0 && !allowDup)
      err << "LabelledInstanceGenerator.c::data_generator: The label noise "
	     "rate " << lig.get_label_noise() << " of the "
	     "LabelledInstanceGenerator, passed as a parameter, is greater "
	     "than 0 and the duplication mode is turned off" << fatal_error;
   
   for (int i=0; i<numInstances; i++) {
      InstanceRC currentInst = lig.get_labelled_inst();
      while (!allowDup && bag.find_unlabelled(currentInst))
	 currentInst = lig.get_labelled_inst();
      bag.add_instance(currentInst);     
   }
}


/***************************************************************************
  Description : This does a basic "counter" increment, with each digit
                  in base "number of values" for that attributes.  It
                  returns TRUE if the increment succeeded, and FALSE
                  when the counter overflowed, signaling the end of
                  the generation process. 
  Comments    : Runs in O(number of attributes).  Worst case time is
                  when all attributes "carry" to the next position.     
***************************************************************************/
static Bool next_instance(InstanceRC& instance,
                          const SchemaRC& schema)
{
   // to simulate addition, we start with the last attribute
   // (least significant bits) and carry towards the first one.

   // we not only use needCarry for knowing when we need to loop
   // again, but also for when we have overflowed
   Bool needCarry = TRUE;
   for (int i = schema.num_attr()-1; i >= 0 && needCarry; i--) {
      const NominalAttrInfo& nai = schema.nominal_attr_info(i);
      AttrValue_ val = instance[i];
      int nextVal = nai.get_nominal_val(val) + 1;
      if (nextVal > UNKNOWN_NOMINAL_VAL + nai.num_values()) {
         nai.set_nominal_val(val, FIRST_NOMINAL_VAL);
         needCarry = TRUE;
      } else {
	 nai.set_nominal_val(val, nextVal);
	 needCarry = FALSE;
      }
      instance[i] = val;
   }

   // If we ran out of attributes for propigating carries,
   // this means that we overflowed, and thus return FALSE.
   if (i < 0 && needCarry)
      return FALSE;
   else
      return TRUE; 
}


/***************************************************************************
  Description : Generates a LabelledInstanceList with every possible
                  instance.
                Expects that fullSpaceLib is already allocated.
                Note that a pointer to fullSpaceLib is passed in
                  because we might want it to point to a subclass of
                  the bag.
  Comments    : Note that the labels are initially set to the
                  UNKNOWN_CATEGORY_VALUE. 
                Runs in O ((MAX(number of attribute values for an
                  attribute)) ^ number of attributes) time.
***************************************************************************/
void full_space_generator(InstanceBag *fullSpaceBag)
{
   ASSERT(fullSpaceBag -> no_instances() == TRUE);
   
   const SchemaRC schema = fullSpaceBag->get_schema();
   
   // loop to create count number of total instances.  Useful for
   // error checking.
   int totalNumInstances = 1;
   for (int i = 0; i < schema.num_attr(); i++) {
      totalNumInstances *= schema.attr_info(i).cast_to_nominal().num_values(); 
      if (totalNumInstances < 0)
	 err << "full_space_generator: more than INT_MAX (" << INT_MAX
	     << ") instances in full space.  You probably don't have "
	        "enough pixels for this" << fatal_error;
   }

   // set all of the values to FIRST_NOMINAL_VAL
   InstanceRC workLi(schema);
   for (i = 0; i < schema.num_attr(); i++) {
      const NominalAttrInfo& nai = schema.nominal_attr_info(i);
      AttrValue_ value;
      nai.set_nominal_val(value, FIRST_NOMINAL_VAL);
      workLi[i] = value;
   }

   // all labels are set to UNKNOWN_CATEGORY_VAL (the default value
   // for NominalAttrValue_)
   NominalAttrValue_ value;
   workLi.set_label(value);

   // Generate all instances and add them to the bag.
   int count = 0;
   do {
      count++;
      InstanceRC instance(workLi);
      fullSpaceBag -> add_instance(instance);
      ASSERT (count <= totalNumInstances);
   } while (next_instance(workLi, schema));
}


