// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : The PerceptronInducer class induces a perceptron from
                   a labeled training set (supervised learning) using
                   the perceptron error correction rule.  See Equation 5.19
		   in "Introduction to the Theory of Neural Computation"
		   by Hertz, Krogh and Palmer.
  Assumptions  : No unknown values.  Only works with Reals.
  Comments     : 
  Complexity   : train() has unknown complexity if the data are linearly
                   separable.  In the worst case it takes time
		   O(maxEpochs*numInstances*numAttributes)
  Enhancements : 
  History      :
                 Ronny Kohavi                                        7/06/95
		   Normalization, updated copy-ctors, copy func
                 Mehran Sahami                                       1/12/95
		   Changed code to make functions consistent with
		   other changes made in MLC++ base classes
                 George John                                         6/14/94
		   Code review mods
                 George John                                         3/15/94
                   Initial revision (.c,.h)
***************************************************************************/

#include <basics.h>
#include <PtronInducer.h>
#include <LinDiscr.h>
#include <CtrInstList.h>
#include <checkstream.h>
#include <MRandom.h>
#include <GetOption.h>

const MString MaxEpochsHelp = "This option specifies the maximum number of "
  "iterations to make over the training data during training.";

const MString LearningRateHelp = "This option specifies the learning rate "
  "by which updates to the weight vector are multiplied.";

const MString RandomSeedHelp =
   "Seed for random generator.  Can duplicate the same result.";


/***************************************************************************
  Description : Set default values for MaxEpochs and LearningRate
  Comments    : 
***************************************************************************/
int PerceptronInducer::defaultMaxEpochs = 10;
Real PerceptronInducer::defaultLearningRate = 1.0;
unsigned int PerceptronInducer::defaultRandomSeed = 7258787;

/***************************************************************************
  Description : Constructor for Perceptron inducer.
  Comments    : 
***************************************************************************/
PerceptronInducer::PerceptronInducer(const MString& dscr)
   : Inducer(dscr)
{
   perceptron=NULL;
   initialWeights=NULL;
   learningRate = defaultLearningRate;
   maxEpochs = defaultMaxEpochs;
   randomSeed = defaultRandomSeed;
}

/*****************************************************************************
  Description : Copy constructor.
  Comments    :
*****************************************************************************/
PerceptronInducer::PerceptronInducer(const PerceptronInducer& source,
				     CtorDummy)
   : Inducer(source, ctorDummy)
{
   perceptron = NULL;
   initialWeights = NULL;
   learningRate = source.learningRate;
   maxEpochs = source.maxEpochs;
   randomSeed = source.randomSeed;
   DBG(OK());
}   



/***************************************************************************
  Description : Deallocates perceptron categorizer
  Comments    : 
***************************************************************************/
PerceptronInducer::~PerceptronInducer()
{
  DBG(OK());
  delete initialWeights;
  delete perceptron;
}



/***************************************************************************
  Description : Trains the Perceptron.  Uses Eq 5.19 in Hertz et al.
                  (pg 97) They're fond of greek letters so  let me translate.
		  Let wi=the ith weight, xi = the ith input value, o=the 
		  output of the perceptron on x, t=the label of x in the
		  training set, lrate = some constant.  The rule is:
		  wi = wi + lrate*(t - o)*xi.  This is described
                  in the code below using inline comments.  Note that if
                  the data is linearly separable then this procedure is
                  guaranteed to find a 0-error perceptronCategorizer.  
                  Otherwise the algorithm oscillates until it gets 
                  stopped by the maxEpochs parameter.
  Comments    : Must be called after read_data().  We consider the
                  first category to be "negative", the second to
                  be "positive" in order to use the standard definition
                  of the perceptron training algorithm
***************************************************************************/
void PerceptronInducer::train()
{
   has_data(); // make sure we have a training set.

   TS->normalize_bag(InstanceBag::extreme);
   int numWts = TS->get_schema().num_attr()+1;

   delete perceptron;

   perceptron = 
      new LinearDiscriminant(TS->get_schema(),description(), -1, 1, -1, 1);

   perceptron->init_rand_num_gen(randomSeed++);
   
   Array<Real> weights(numWts);

   if (get_initialWeights()) {
      weights = *get_initialWeights();
   } else {
      perceptron->init_weights(weights);
   }
   
   perceptron->set_weights(weights);

   //will use this to store the x vector
   Array<Real> xVector(TS->get_schema().num_attr()+1);  
   const SchemaRC& schema = TS->get_schema();   

   Bool Mistakes = TRUE;

   //begin training
   for (int epoch = 0; Mistakes && epoch < get_maxEpochs(); epoch++)  {
     LOG(2,"PerceptronInducer::train() epoch iteration " << epoch << endl);
     IFLOG(3,display_struct());

     Mistakes = FALSE;
     
     xVector[xVector.high()] = get_learningRate(); //include the threshold

     for (Pix instPix = TS->first(); instPix; TS->next(instPix)) {
       const InstanceRC& li = TS->get_instance(instPix);
       const AugCategory predictedCat = perceptron->categorize(li);
       const Category realCat =
	  schema.label_info().get_nominal_val(li.get_label());
       if (predictedCat != realCat) {
	 //construct the array of the instance attribute values
	 for (int i = xVector.low(); i < xVector.high(); i++)
	   xVector[i] = (get_learningRate() * 
			 schema.attr_info(i).cast_to_real().
			 normalized_value(li[i]));

	 Mistakes = TRUE;
	 if (predictedCat.num() == (FIRST_CATEGORY_VAL)) {
	    perceptron->add_to_weights(xVector);
	 } else if (predictedCat.num() == (FIRST_CATEGORY_VAL + 1)) {
	    perceptron->subtract_from_weights(xVector);
	 } else {
	    ASSERT(FALSE);
	 }
       } 
     } //loop over each pattern
//     if (!Mistakes)
   } //loop for all epochs
}



/***************************************************************************
  Description : Return TRUE iff the class has a valid categorizer.
  Comments    : Default value of fatalOnFalse is TRUE
***************************************************************************/
Bool PerceptronInducer::was_trained(Bool fatalOnFalse) const
{
   if (fatalOnFalse && perceptron == NULL)
      err << "PerceptronInducer::was_trained: No categorizer. "
             " Call train() to create perceptron" << fatal_error;
   return perceptron != NULL;
}


/***************************************************************************
  Description : Returns the categorizer that the inducer has generated.
  Comments    :
***************************************************************************/
const Categorizer& PerceptronInducer::get_categorizer() const
{
   was_trained(TRUE);
   return *perceptron;
}



/***************************************************************************
  Description : Allows the user the set options for Perceptron.
  Comments    :
***************************************************************************/
void PerceptronInducer::set_user_options(const MString& prefix)
{
   set_maxEpochs(get_option_int(prefix + "MAX_EPOCHS", defaultMaxEpochs,
				MaxEpochsHelp, TRUE));
   
   set_learningRate(get_option_real(prefix + "LEARNING_RATE",
				    defaultLearningRate, LearningRateHelp,
				    TRUE));

   randomSeed = get_option_int(prefix + "RANDOM_SEED",
			       defaultRandomSeed,
			       RandomSeedHelp,
			       TRUE);
}

/*****************************************************************************
  Description : Returns the copy of inducer.
  Comments    :
*****************************************************************************/
Inducer* PerceptronInducer::copy() const
{
   return new PerceptronInducer(*this, ctorDummy);
}
   
/***************************************************************************
  Description : Sets the learning rate and checks for validity.
  Comments    :
***************************************************************************/
void PerceptronInducer::set_learningRate(Real lr)
{
   if (lr <= 0) {
      err << "PerceptronInducer::set_learningRate: Learning rate is "
	 "being set to a non-positive value." << fatal_error;
   }
   learningRate = lr;
}



/***************************************************************************
  Description : Sets the maximum number of trainging epochs and checks
                for validity.
  Comments    :
***************************************************************************/
void PerceptronInducer::set_maxEpochs(int me)
{
   if (me <= 0) {
      err << "PerceptronInducer::set_maxEpochs: Maximum number of training "
	 "epochs is being set to a non-positive value." << fatal_error;
   }
   maxEpochs = me;
}




