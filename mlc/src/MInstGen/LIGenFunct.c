// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Defines several functor classes that are used in
                   LabelledInstanceGenerator and its derived classes.
  Assumptions  : UniformAttrGenFunctor temporarily assumes that the
                   attribute is a nominal attribute. 
  Comments     :
  Complexity   : IndependentLabInstGenFunctor::labInstance() takes
                   O(Sum) time where Sum is the sum of the times 
		   IndependentLabInstGenerator::gen_attr_value() takes
		   for all different attributes. 
  Enhancements : Subclass LabInstGenFunct into several basic label
                   functions. 
  History      : Svetlozar Nestorov                                 2/25/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <basics.h>
#include <LIGenFunct.h>
#include <LabInstGen.h>

RCSID("MLC++, $RCSfile: LIGenFunct.c,v $ $Revision: 1.10 $")

/***************************************************************************
  Description : Returns a random value of the given attribute,
                  uniformly distributed. 
  Comments    : 
***************************************************************************/
AttrValue_ UniformAttrGenFunctor::operator()(const AttrInfo& ai)
{
   AttrValue_ result;
   if (ai.can_cast_to_nominal()) {
      const NominalAttrInfo& nai = ai.cast_to_nominal();
      nai.set_nominal_val(result,
		      FIRST_NOMINAL_VAL + randGen.integer(nai.num_values()));
   } else {
      const RealAttrInfo& rai = ai.cast_to_real();
      rai.set_real_val(result, randGen.real(0,1));
   }
   return result;
}

/***************************************************************************
  Description : Returns a random value of the given attribute,
                  skewed such that 0 gets probability p and the
		  others are uniform.
  Comments    : 
***************************************************************************/
AttrValue_ SkewedAttrGenFunctor::operator()(const AttrInfo& ai)
{
   if (!ai.can_cast_to_nominal())
      err << "SkewedAttrGenFunctor::operator(): only nominals are supported."
	  << fatal_error;

   const NominalAttrInfo& nai = ai.cast_to_nominal();
   ASSERT(nai.num_values() > 1);
   
   AttrValue_ result;
   Real p = randGen.real(0, 1);
   if (p < probZero) 
      nai.set_nominal_val(result, FIRST_NOMINAL_VAL);
   else 
      nai.set_nominal_val(result, FIRST_NOMINAL_VAL + 1 + 
			  randGen.integer(nai.num_values() - 1));
   return result;
}




/***************************************************************************
  Description : Constructor.
  Comments    :
***************************************************************************/
IndependentLabInstGenFunctor::IndependentLabInstGenFunctor
                                    (const SchemaRC& aSchema,
				     const IndependentLabInstGenerator& lig) 
: LabInstGenFunctor(),  labInstSchema(aSchema),  labInstGen(lig)
{}


/***************************************************************************
  Description : Constructs and returns a pointer to a new
                  LabelledInstanceAlloc.
		To generate a value for each attribute it uses the
		  function specified in the attrGenFunctors array of
		  independentGen.  
  Comments    : 
***************************************************************************/
InstanceRC IndependentLabInstGenFunctor::operator() ()
{
   InstanceRC inst(labInstSchema);

   for (int i = 0; i < labInstSchema.num_attr(); i++) {
      const AttrInfo& ai = labInstSchema.attr_info(i);
      AttrValue_ value = labInstGen.gen_attr_value(ai, i);
      inst[i] = value;
   }
   return inst;
}
