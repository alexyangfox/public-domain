// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The WinnowInducer class induces a linear discriminator from
                   a labeled training set (supervised learning) using
                   the the WINNOW algorithm.  See Littlestone's "Learning
		   Quickly When Irrelevant Attributes Abound: A New Linear
		   Threshold Algorithm" in Machine Learning, 2:285-318,
		   1988 for more details on WINNOW.
  Assumptions  : No unknown values.  Only works with Reals.
  Comments     : 
  Complexity   : train() has unknown complexity if the data are linearly
                   separable.  In the worst case it takes time
		   O(maxEpochs*numInstances)
		 predict() takes time O(numAttribtues)
  Enhancements : 
  History      :
                 Ronny Kohavi                                        7/06/95
		   Normalization, updated copy-ctors, copy func
                 Mehran Sahami                                       1/12/95
                   Initial revision (.c,.h) based on the PtronInducer
***************************************************************************/

#include <basics.h>
#include <WinnowInducer.h>
#include <LinDiscr.h>
#include <CtrInstList.h>
#include <checkstream.h>
#include <MRandom.h>
#include <math.h>
#include <GetOption.h>


const MString MaxEpochsHelp = "This option specifies the maximum number of "
  "iterations to make over the training data during training.";

const MString AlphaHelp = "This option sets the alpha parameter for the "
  "winnow inducer.";

const MString BetaHelp = "This option sets the beta parameter for the "
  "winnow inducer.";

const MString RandomSeedHelp =
   "Seed for random generator.  Can duplicate the same result.";



/***************************************************************************
  Description : Set default values for MaxEpochs, Alpha, and Beta
  Comments    : 
***************************************************************************/
int WinnowInducer::defaultMaxEpochs = 10;
Real WinnowInducer::defaultAlpha = 1.2;
Real WinnowInducer::defaultBeta = 0.8; // should be about 1/alpha

/***************************************************************************
  Description : Constructors for Perceptron inducer.
  Comments    : 
***************************************************************************/

WinnowInducer::WinnowInducer(const MString& dscr)
 : Inducer(dscr)
{
   winnow=NULL;
   initialWeights=NULL;
   alpha=defaultAlpha;
   beta=defaultBeta;
   maxEpochs = defaultMaxEpochs;
}

WinnowInducer::WinnowInducer(const WinnowInducer& source, CtorDummy)
   : Inducer(source, ctorDummy)
{
   winnow = NULL;
   initialWeights = NULL;
   alpha=source.alpha;
   beta =source.beta;
   maxEpochs = source.maxEpochs;
}


/***************************************************************************
  Description : Deallocates winnow categorizer
  Comments    : 
***************************************************************************/
WinnowInducer::~WinnowInducer()
{
  DBG(OK());
  delete initialWeights;
  delete winnow;
}



/***************************************************************************
  Description : Trains the WINNOW linear discriminant.  Uses Littlestone's
                  WINNOW algorithm, cited above, as a multiplicative
		  weight updating mechanism.
  Comments    : Must be called after read_data().  We consider the
                  first category to be "negative", the second to
                  be "positive"
***************************************************************************/
void WinnowInducer::train()
{
   has_data(); // make sure we have a training set.

   TS->normalize_bag(InstanceBag::extreme);

   int numWts = TS->get_schema().num_attr()+1;

   delete winnow;

   winnow = 
      new LinearDiscriminant(TS->get_schema(),description(), 0.1, 1.0,
			     -2.0, -1.0);

   winnow->init_rand_num_gen(randomSeed++);

   Array<Real> weights(numWts);

   if (get_initialWeights()) {
      weights = *get_initialWeights();
   } else {
      winnow->init_weights(weights);
   }

   winnow->set_weights(weights);

   //will use this to store the x vector
   Array<Real> xVector(TS->get_schema().num_attr()+1);  
   const SchemaRC& schema = TS->get_schema();   

   Bool Mistakes = TRUE;
   
   //begin training
   for (int epoch = 0; Mistakes && epoch < get_maxEpochs(); epoch++)  {
     LOG(2,"WinnowInducer::train() epoch iteration " << epoch << endl);
     IFLOG(3,display_struct());

     Mistakes = FALSE;

     xVector.index(xVector.high()) = 1; //threshold never changes
     
     for (Pix instPix = TS->first(); instPix; TS->next(instPix)) {
       const InstanceRC& li = TS->get_instance(instPix);
       AugCategory predictedCat = winnow->categorize(li);
       const Category realCat =
	  schema.label_info().get_nominal_val(li.get_label());
       if (predictedCat != realCat) {
	  Mistakes = TRUE;
	  
          //construct the array of the instance attribute values
	  if (predictedCat.num() == FIRST_CATEGORY_VAL) {
	     for (int i = xVector.low(); i < xVector.high(); i++)
		xVector.index(i) =
		   pow(alpha,schema.attr_info(i).cast_to_real().
		       normalized_value(li[i]));
	  } else if (predictedCat.num() == (FIRST_CATEGORY_VAL+1)) {
	     for (int i = xVector.low(); i < xVector.high(); i++)
		xVector.index(i) =
		   pow(beta,schema.attr_info(i).cast_to_real().
		       normalized_value(li[i]));
	  } else {
	     ASSERT(FALSE);
	  }
	  winnow->multiply_weights(xVector);
       } 
     } //loop over each pattern
   } //loop for all epochs
}




/***************************************************************************
  Description : Return TRUE iff the class has a valid categorizer.
  Comments    : Default value of fatalOnFalse is TRUE
***************************************************************************/
Bool WinnowInducer::was_trained(Bool fatalOnFalse) const
{
   if (fatalOnFalse && (winnow == NULL))
      err << "WinnowInducer::was_trained: No categorizer. "
             " Call train() to create winnow" << fatal_error;
   return (winnow != NULL);
}


/***************************************************************************
  Description : Returns the categorizer that the inducer has generated.
  Comments    :
***************************************************************************/
const Categorizer& WinnowInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *winnow;
}


/***************************************************************************
  Description : Allows the user to set options for Winnow.
  Comments    :
***************************************************************************/
void WinnowInducer::set_user_options(const MString& prefix)
{
   set_maxEpochs(get_option_int(prefix + "MAX_EPOCHS", defaultMaxEpochs,
				 MaxEpochsHelp, TRUE));

   set_alpha(get_option_real(prefix + "ALPHA", defaultAlpha,
			      AlphaHelp, TRUE));

   set_beta(get_option_real(prefix + "BETA", defaultBeta,
			     BetaHelp, TRUE));

   randomSeed = get_option_int(prefix + "RANDOM_SEED",
			       LinearDiscriminant::defaultRandomSeed,
			       RandomSeedHelp,
			       TRUE);
   
}

/*****************************************************************************
  Description : Returns the copy of inducer.
  Comments    :
*****************************************************************************/
Inducer* WinnowInducer::copy() const
{
   return new WinnowInducer(*this, ctorDummy);
}
   

/***************************************************************************
  Description : Set the maximum number of training epochs and checks for
                validity.
  Comments    :
***************************************************************************/
void WinnowInducer::set_maxEpochs(int me)
{
   if (me <= 0) {
      err << "WinnowInducer::set_maxEpochs: Maximum number of training "
	 "epochs is being set to a non-positive value." << fatal_error;
   }
   maxEpochs = me;
}



/***************************************************************************
  Description : Set the alpha parameter and check for validity.
  Comments    :
***************************************************************************/
void WinnowInducer::set_alpha(Real alph)
{
   if (alph <= 1) {
      err << "WinnowInducer::set_alpha: Alpha is being set to a value "
	 "less than or equal to one." << fatal_error;
   }
   alpha = alph;
}



/***************************************************************************
  Description : Set the beta parameter and check for validity.
  Comments    :
***************************************************************************/
void WinnowInducer::set_beta(Real bet)
{
   if (bet >= 1 || bet <= 0) {
      err << "WinnowInducer::set_beta: Beta is being set to a value "
	 "outside the range (0, 1)." << fatal_error;
   }
   beta = bet;
}



/***************************************************************************
  Description : Set the initial weights and check for validity.
  Comments    :
***************************************************************************/
void WinnowInducer::set_initialWeights(const Array<Real>& iw)
{
   for(int i = 0; i < iw.size()-1; i++) {
      if (iw.index(i) < 0) {
	 err << "WinnowInducer::set_initialWeights: Initial weight vector "
	    "has a negative value at index " << i << "." << fatal_error;
      }
   }

   if (iw.index(iw.size()-1) >= 0) {
      err << "WinnowInducer::set_initialWeights: Initial weight vector "
	 "has a non-negative threshold value (negative theta value)." <<
	 fatal_error;
   }
   
   if (initialWeights) {
      delete initialWeights;
   }
   initialWeights = new Array<Real>(iw,ctorDummy);

}
