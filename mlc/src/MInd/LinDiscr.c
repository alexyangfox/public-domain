// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : Linear Discriminant categorizer returns category1 if
                   weights . inputs >= threshold, category0 otherwise.
		   The "." here indicates the vector dot product.  For
		   convenience, we consider the threshold as an extra
		   weight whose input is always 1, and thus return
		   category1 when weights.inputs >= 0.
		   For example, if the weights are <2,-1> and the threshold
		   is 3, the weight Array stored by the LinearDiscriminant
		   is <2,-1,-3>.  If the input vector is <4,5> then the
		   augmented input vector is <4,5,1>, and
		   weights . inputs = 2*4 + -1*5 + -3*1 = 3.
		   Since 3 >= 0 the categorizer would return category1.
  Assumptions  : Number of labels must be 2 (only binary classification).
                 Attributes must be Real.
		 Doesn't handle missing values
  Comments     : 
  Complexity   : All operations are O(n) where n is the number of
                   attributes.
  Enhancements : In OK(), check for floating point exceptions in weight values
  History      : Mehran Sahami                                1/12/94
                   Modifications to update code to work with changes
		   made in other MLC++ base classes.  Added
		   multiply_weights method
                 George John                                  3/21/94
		   First Working Version
                 George John & Karl Pfleger                  12/15/93
                   Initial Version
***************************************************************************/

#include <basics.h>
#include <LinDiscr.h>
#include <MRandom.h>
#include <DisplayPref.h>
#include <InstList.h>


unsigned int LinearDiscriminant::defaultRandomSeed = 7258789;

/***************************************************************************
  Description : Performs various internal consistency checks.
  Comments    : 
***************************************************************************/
void LinearDiscriminant::OK(int /*level*/) const
{
  ASSERT((cat0 != NULL) && (cat1 != NULL));
  if (*cat0 == *cat1)
    err << "LinearDiscriminant::OK: cat0 "
	<< *cat0 << " and cat1 " << *cat1 << " are the same"
	<< fatal_error;

  ASSERT(weights.size() >= 1);
  ASSERT(weights.low() == 0);
}


/***************************************************************************
  Description : Initializes the Linear Discriminant.  We just use the 
                  LabelledInstanceInfo to see how many attributes there
                  are and to get the category names.
                The schema must have the normalization factors.
  Comments    : Ideas on creating AugCategories borrowed from AttrCat.c
***************************************************************************/
LinearDiscriminant::LinearDiscriminant(const SchemaRC& aSchema,
				       const MString& descr,
				       Real randL, Real randH,
				       Real threshL, Real threshH)
: Categorizer(2, descr), 
  schema(aSchema),
  weights(aSchema.num_attr() + 1)
{
  if (schema.num_label_values() != 2)
         err << "LinearDiscriminant::LinearDiscriminant: Number of "
             << "categories " << schema.num_label_values() << " != 2"
             << fatal_error;

  cat0 = new AugCategory(FIRST_CATEGORY_VAL,
			 schema.nominal_label_info().
			 get_value(FIRST_CATEGORY_VAL));
  cat1 = new AugCategory(FIRST_CATEGORY_VAL+1,
			 schema.nominal_label_info().
			 get_value(FIRST_CATEGORY_VAL+1));


  set_randRange(randL, randH);
  set_thresholdRange(threshL, threshH);
  init_rand_num_gen(defaultRandomSeed);
  
  for(int i = 0; i < schema.num_attr() + 1; i++) {
     weights.index(i) = 0.0;
  }

  for ( int z = 0; z < schema.num_attr(); z++ )
     if(!schema.attr_info(z).can_cast_to_real())
	err << "LinearDiscriminant::LinearDiscriminant: Attribute "
	    << z << " is not continuous." << fatal_error;


}


/***************************************************************************
  Description : Copies the structure
  Comments    : 
***************************************************************************/
LinearDiscriminant::LinearDiscriminant(const LinearDiscriminant& source, 
					     const CtorDummy dummyArg)
: Categorizer(source,dummyArg), 
  schema(source.schema),
  cat0(new AugCategory(*source.cat0)), 
  cat1(new AugCategory(*source.cat1)), weights(source.weights,ctorDummy)
{
  set_randRange(source.randLo, source.randHi);
  set_thresholdRange(source.threshLo, source.threshHi);
  init_rand_num_gen(defaultRandomSeed);
  DBG(OK());
}


/***************************************************************************
  Description : Deletes memory used for weight vector, categories.
  Comments    : 
***************************************************************************/
LinearDiscriminant::~LinearDiscriminant()
{
  delete cat0;
  delete cat1;
}


/***************************************************************************
  Description : Multiplies weight vector by instance vector (dot product).
                  Returns cat1 if product >= 0, cat0 otherwise.  For example,
                  if instances have 2 attributes then this computes
                  w1*x1 + w2*x2 + w3 and checks to see if this is >=0
                  or < 0.
  Comments    : 
***************************************************************************/
AugCategory LinearDiscriminant::categorize(const InstanceRC& inst) const
{
  DBG(OK());
  
  //check that inst has right number of attributes
  DBGSLOW(if(schema.num_attr() != weights.size()-1)
         err << "LinearDiscriminant::categorize: Instance has "
             << schema.num_attr() << " attributes,"
             << " expecting " << weights.size() - 1 << fatal_error;
      );

  // start with the threshold weight
  Real sum = weights.index(weights.size()-1);
  
  for (int i=0;i<weights.size()-1; i++) 
    sum += weights.index(i) * schema.attr_info(i).cast_to_real().
            normalized_value(inst[i]);
  if (sum>=0)
    return *cat1;
  else
    return *cat0;
}


/***************************************************************************
Description : Prints a readable representation of the categorizer to the
                given stream.
Comments    : Only implemented for ASCIIDisplay for now.
***************************************************************************/
void LinearDiscriminant::display_struct(MLCOStream& stream,
					const DisplayPref& dp) const
{
  DBG(OK());
  if (dp.preference_type() == DisplayPref::ASCIIDisplay) {
    stream << "Linear Discriminant Categorizer " << description()
           << " Weights: " << weights << endl;
  }
  else err << "LinearDiscriminant::display_struct: only implemented for"
	 << "ASCIIDisplay" << fatal_error;
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of this Categorizer
  Comments    :
***************************************************************************/
Categorizer* LinearDiscriminant::copy() const
{ 
  OK();
  return new LinearDiscriminant(*this, ctorDummy);
}
  


/***************************************************************************
  Description : Check that the array has low=0 and same size as
                  weights
  Comments    : 
***************************************************************************/
#ifndef FAST
static void array_ok(const Array<Real>& a, const Array<Real>& wts,
		     const char* whereInCode)
{
  if (a.low() != 0)
    err << whereInCode << "Array has low = " << a.low() << " was "
	<< "expecting low = 0" << fatal_error;
  if (a.size() != wts.size())
    err << whereInCode << "Array has size = " << a.low() << " was "
	<< "expecting size = " << wts.size() << fatal_error;
}
#endif


/***************************************************************************
  Description : Returns TRUE if (*this == cat)
  Comments    :
***************************************************************************/

Bool LinearDiscriminant::operator==(const Categorizer &cat) const
{
   if (class_id() == cat.class_id())
      return (*this) == (const LinearDiscriminant &) cat;
   return FALSE;
}


Bool LinearDiscriminant::operator==(const LinearDiscriminant &cat) const
{
   return ((cat0->num() == cat.cat0->num()) &&
	   (cat1->num() == cat.cat1->num()) &&
           (cat0->description() == cat.cat0->description()) &&
	   (cat1->description() == cat.cat1->description()) &&
           (num_categories() == cat.num_categories()) &&
           (description()  == cat.description()) &&
	   (randLo == cat.randLo) && (randHi == cat.randHi) &&
	   (threshLo == cat.threshLo) && (threshHi == cat.threshHi) &&
	   (weights == cat.weights));
}

/***************************************************************************
  Description : Initializes the array newWeights to have random values as
                given by the ranges randLo to randHi and threshLo to threshHi
  Comments    : 
***************************************************************************/
void LinearDiscriminant::init_weights(Array<Real>& newWeights)
{
  DBG(array_ok(newWeights, weights, "LinearDiscriminant::init__weights: "));
  for(int i = 0; i < newWeights.size() - 1; i++) {
     newWeights.index(i) = rand_num_gen().real(randLo, randHi);
  }
  newWeights.index(newWeights.size()-1) = rand_num_gen().real(threshLo,
							      threshHi);
  DBG(OK());
}


/***************************************************************************
  Description : Set weights to new specified values
  Comments    : 
***************************************************************************/
void LinearDiscriminant::set_weights(const Array<Real>& newWeights)
{
  DBG(array_ok(newWeights, weights, "LinearDiscriminant::set_weights: "));
  weights = newWeights;
  DBG(OK());
}


/***************************************************************************
  Description : Adds the specified values to the weights
  Comments    : 
***************************************************************************/
void LinearDiscriminant::add_to_weights(const Array<Real>& delta)
{
  DBGSLOW(array_ok(delta, weights, "LinearDiscriminant::add_to_weights: "));
  weights += delta;
  DBGSLOW(OK());
}


/***************************************************************************
  Description : Subtracts the specified values to the weights
  Comments    : 
***************************************************************************/
void LinearDiscriminant::subtract_from_weights(const Array<Real>& delta)
{
  DBGSLOW(array_ok(delta, weights,
		   "LinearDiscriminant::subtract_from_weights: "));
  weights -= delta;
  DBGSLOW(OK());
}


/***************************************************************************
  Description : Multiplies the weights by the specified values
  Comments    :
***************************************************************************/
void LinearDiscriminant::multiply_weights(const Array<Real>& delta)
{
  DBGSLOW(array_ok(delta, weights, "LinearDiscriminant::multiply_weights: "));
  for (int i=0; i<delta.size(); i++)
    weights.index(i) *= delta[i];
  DBGSLOW(OK());
}


