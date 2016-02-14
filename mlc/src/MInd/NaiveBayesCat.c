// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  
  Description : This categorizer returns the category (label) that had the
                  greatest relative probability of being correct, assuming
		  independence of attributes. Relative probability of a label
		  is calculated by multiplying the relative probability for
		  each attribute.  The calculation of relative probabity for a
		  label on a single attribute depends on whether the attribute
		  is descrete or continuous.
                By Bayes Theorem, P(L=l | X1=x1, X2=x2, ... Xn=xn)
                                = P(X1=x1, X2=x2, ... Xn=xn | L=l)*P(L=l)/P(X)
                  where P(X) is P(X1=x1, ..., Xn=xn).
		  Since P(X) is constant independent of the classes, we
		  can ignore it.
		The Naive Bayesian approach asssumes complete independence
		  of the attributes GIVEN the label, thus
		  P(X1=x1, X2=x2, ... Xn=xn | L=l) = 
                     P(X1=x1|L=l)*P(X2=x2|L)*... P(Xn=xn|L)
		  and P(X1=x1|L=l) = P(X1=x1 ^ L=l)/P(L=l) where this
		  quantity is approximated form the data.
		When the computed probabilities for two labels have the same
                  value, we break the tie in favor of the most prevalent label.

		If the instance being categorized has the first attribute = 1,
		  and in the training set label A occured 20 times, 10 of 
		  which had value 1 for the first attribute, then the 
		  relative probability is 10/20 = 0.5.

		For continuous (real) attributes, the relative probability
		  is based on the Normal Distribution of the values of the
		  attribute on training instances with the label.  The actual
		  calculation is done with the Normal Density; constants,
		  which do not affect the relative probability between labels,
		  are ignored.  For example, say 3 training instances have
		  label 1 and these instances have the following values for a
		  continous attribute: 35, 50, 65.  The program would use the
		  mean and variance of this "sample" along with the attribute
		  value of the instance that is being categorized in the
		  Normal Density equation.  The evaluation of the Normal
		  Density equation, without constant factors, provides the
		  relative probability.

                 Unknown attributes are skipped over.
  Assumptions :  This method calculates the probability of a label as the
                   product of the probabilities of each attribute
		   This is assuming that the attributes are
		   independent, a condition not likely corresponding to
		   reality.  Thus the "Naive" of the title.
		 This method assumes that all continous attributes have a
		   Normal distribution for each label value.
  
  Comments :     For nominal attributes, if a label does not have
                   any occurences for a given attribute value
		   of the test instance, a probability of
		   noMatchesFactor * ( 1 / # instances in training set )
		   is used.

  		 For nominal attributes, if an attribute value does not
		   occur in the training set, the attribute is skipped
		   in the categorizer, since it does not serve to
		   differentiate the labels.

                 The code can handle dealing with unknowns as a special
		   value by doing the is_unknown only in the real attribute
		   case.

		 Helper class NBNorm is a simple structure to hold the
		   parameters needed to calculate the Normal Distribution
		   of each Attribute,Label pair.  The NBNorms are stored in
		   a Array2 table "continNorm" which is indexed by attribute
		   number and label value.
		   
		 For continuous attributes the variance must not equal 0 since
		   it is in the denominator.  If the variance is undefined for
		   a label value (e.g. if a label only has only one instance
		   in the training set), NaiveBayesInd will declare the
		   variance to be defaultVariance, a static variable.  In
		   cases where the variance is defined but equal to 0,
		   NaiveBayesInd will declare the variance to be epsilon,
		   a very small static variable.

		 For continous attributes, if a label does not occur in
		   the training set, a zero relative probability is
		   assigned.  If a label occurs in the training set but only
		   has unknown values for the attribute, noMatchesFactor is
		   used as in the nominal attribute case above.

  Complexity :   categorize() is O(ln) where l = the number of categories
                   and n = the number of attributes.
  
  Enhancements :
  History      : Robert Allen                                       12/03/94
                   Initial revision.
		   
***************************************************************************/

#include <basics.h>
#include <NaiveBayesCat.h>
#include <math.h>

RCSID("MLC++, $RCSfile: NaiveBayesCat.c,v $ $Revision: 1.20 $")


//  Static Values:
  
// Set factor to reduce probability when training set contains instances where
//  the categorized attribute value does not provide at least one match for all
//  labels, as described above. 

Real NaiveBayesCat::noMatchesFactor = 0.5;

/***************************************************************************
  Description : NBNorm is a helper class to hold the 3rd demension of
                  a Array2 containing the parameters of the Normal
		  Density for each (continuous attribute,label) combination.
		The "hasData" argument defaults to FALSE;
    Comments    :
***************************************************************************/
NBNorm::NBNorm(Real m, Real v, Bool hd)
   : mean(m), var(v), hasData(hd)
{}


/***************************************************************************
  Description : operator== is provided for use by NaiveBayesCat::operator==.  
  Comments    :
***************************************************************************/
Bool NBNorm::operator==(const NBNorm& nbn2) const
{
   return ( hasData == nbn2.hasData &&
	    mean == nbn2.mean &&
	    var == nbn2.var );
}

/***************************************************************************
  Description : operator!= is provided for use by NaiveBayesCat::operator==.  
  Comments    :
***************************************************************************/
Bool NBNorm::operator!=(const NBNorm& nbn2) const
{
   return !(*this == nbn2);
}

/***************************************************************************
  Description : Prints detail probability values with attribute names.
                  Used when logging maximum detail in categorize().
  Comments    : Private
***************************************************************************/
void NaiveBayesCat::log_prob(const Array<Real>& prob,
			     const SchemaRC& instSchema) const
{
   for (int i = prob.low(); i <= prob.high(); i++) 
      get_log_stream() << "Prob=" << prob[i] <<
      " for label " << i << " (" 
	 << instSchema.category_to_label_string(i) << ')' << endl;
   get_log_stream() << endl;
}
				   
/***************************************************************************
  Description : Check state of object after training.  Checks
                  1) bag counter is ok, and
                  2) the number of test cases is > 0, and
		  3) that there are no variances = 0.
  Comments    :
****************************************************************************/
void NaiveBayesCat::OK(int level) const
{
   nominCounts.OK(level);
   
   if (nominCounts.label_counts().size() < 2 )  // 1 comes free for unkn
      err << "NaiveBayesCat::OK: BagCounter has less than 2 labels"
          << fatal_error;
   
   if ( numTrainInst <= 0 )
      err << "NaiveBayesCat::OK: Number of training instances must be > 0. " 
	  << "Training instance currently: " << numTrainInst << fatal_error;

   if ( continNorm != NULL ) {
      int labelNumVals = nominCounts.label_counts().size() - 1;
      for (int attrVal = 0; attrVal < numAttributes; attrVal++) 
	 if ( nominCounts.value_counts()[attrVal] == NULL ) { // continuous
	    for (int labelVal = -1; labelVal < labelNumVals; labelVal++) {
	       NBNorm & nbn = (*continNorm)(attrVal,labelVal);
	       
	       if ( ! nbn.hasData )
		  err << "NaiveBayesCat::OK: Normal Distribution data "
		     "missing for (label, attribute) (" << labelVal << ", "
		     << attrVal << "). " << fatal_error;
	       
	       if ( ! nbn.var )
		  err << "NaiveBayesCat::OK: Varience must be > 0 for "
		     "continuous attributes.  Varience = for "
		     "(label, attribute) (" << labelVal << ", "
		     << attrVal << "). " << fatal_error;
	    }
	 }
   }
}


/***************************************************************************
  Description : Constructor for bags that have continuous and nominal
                  attributes.  Fills in the densityArray for the continuous
		  attributes.
  Comments    : 
***************************************************************************/
NaiveBayesCat::NaiveBayesCat(const MString& dscr, int numCategories,
			     const BagCounters& bc, 
			     const Array2<NBNorm> * normDens,
			     int numInst)
   :  Categorizer(numCategories, dscr),
      nominCounts(bc,ctorDummy),
      numTrainInst(numInst),
      numAttributes(nominCounts.value_counts().size())
{
   if ( normDens != NULL )
      continNorm = new Array2<NBNorm>(*normDens, ctorDummy);
   else
      continNorm = NULL;
}

/***************************************************************************
    Description : Copy constructor which takes an extra argument.
    Comments    : 
***************************************************************************/
NaiveBayesCat::NaiveBayesCat(const NaiveBayesCat& source,
			     CtorDummy /* dummyArg */)
   : Categorizer(source, ctorDummy),
     nominCounts(source.nominCounts, ctorDummy),
     numTrainInst(source.numTrainInst),
     numAttributes(source.numAttributes)
{
   if ( source.continNorm != NULL )
      continNorm = new Array2<NBNorm>(*(source.continNorm), ctorDummy);
   else
      continNorm = NULL;
}


/***************************************************************************
  Description : Destructor.
  Comments    : 
***************************************************************************/
NaiveBayesCat::~NaiveBayesCat()
{
   DBG(OK());
   delete continNorm;
}



/***************************************************************************
  Description : Returns a category given an instance by checking all 
                  attributes in schema and returning category with highest
		  relative probability.
                The relative probability is being estimated for each label.
                  The label with the highest values is the category returned.
		  The probability for a given label is
		    P(Nominal Attributes)*P(Continuous Attributes)
		  Since the probability is a product,  we can factor out any
		  constants that will be multiplied times every label, since
		  this will not change the ordering of labels.
		P(Continuous Attribute Value X) is caculated using the normal
		  density: 
                  Normal(X) = 1/(sqrt(2*pi)*std-dev)*exp((-1/2)*(X-mean)^2/var)
                  This calculation can be stripped of the constant
		  (sqrt(2*pi)) without changing the outcome.
		P(Nominal Attributes) is calculated as the percentage of a
		  label's training set that had the test instance's value
		  for a each attribute.
		The majority label is returned if all are equal.
		See this file's header for more information.
  Comments    :
***************************************************************************/

// Rescale the probabilities so that the highest is 1
// This is to avoid underflows 
static void rescale_prob(Array<Real>& prob)
{
   Real maxVal = prob.max();
   // Avoid division by zero.  Happens on shuttle-small.
   if (maxVal > 0) {
      for (int labelVal = prob.low(); labelVal <= prob.high(); labelVal++) {
	 prob[labelVal] = prob[labelVal] / maxVal;
	 ASSERT(prob[labelVal] >= 0);
      }
   }
}

// Initialize the probabilities to be the class probabilities P(L = l)
static void init_class_prob(const BagCounters& nominCounts, int numTrainInst,
			    Array<Real>& prob)
{
   for (Category labelVal = prob.low(); labelVal <= prob.high(); labelVal++)
      prob[labelVal] = Real(nominCounts.label_count(labelVal)) / numTrainInst;
}


AugCategory NaiveBayesCat::categorize(const InstanceRC& instance) const 
{
   int 			attrNum;
   Category             labelVal;
   const SchemaRC 	schema 	= instance.get_schema();
   const int 		labelNumVals 	= schema.num_label_values();
   // starts at -1 for the unknown category
   Array<Real> 		prob(UNKNOWN_CATEGORY_VAL, labelNumVals + 1);

   // Sanity check: the number of attributes and the number of labels of
   // the training set of the categorizer should correspond with the Schema
   // of the instance being categorized.

   DBGSLOW(OK());
   LOG(3, "Instance to categorize: ");
   IFLOG(3,instance.display_unlabelled(get_log_stream())); 
   LOG(3, endl);

   int trainLabelCount = nominCounts.label_counts().size() - 1;
   if ( labelNumVals != trainLabelCount || 
	numAttributes != schema.num_attr()) 
      err << "NaiveBayesCat::categorize: Schema of instance to be categorized"
	 " does not match Schema of training set." << fatal_error;

      
   init_class_prob(nominCounts, numTrainInst, prob);
   LOG(4, "Initial class probabilities" << endl);
   IFLOG(4, log_prob(prob, schema));


   // loop through each attribute in instance:
   for (attrNum=0; attrNum < numAttributes; attrNum++) {
     if (schema.attr_info(attrNum).is_unknown(instance[attrNum]))
	 LOG(4, "Skipping unknown value for attribute " << attrNum << endl);
      else {
	 // continuous attr
	 if ( nominCounts.value_counts()[attrNum] == NULL ) {
	    LOG(4, endl << "Continuous Attribute " << attrNum << endl);

	    ASSERT( continNorm != NULL );
	    if ( ! schema.attr_info(attrNum).can_cast_to_real() )
	       err << "NaiveBayesCat::categorize: Schema of instance to be "
		  "categorized does not match Schema of training set. "
		  "Attribute Number " << attrNum
		   << " is continuous in training "
		  "set and nominal in instance schema." << fatal_error;

	    const Real realVal =
	       schema.attr_info(attrNum).get_real_val(instance[attrNum]);

	    for (labelVal = prob.low(); labelVal <= prob.high(); labelVal++) {
	       NBNorm & nbn = (*continNorm)(attrNum,labelVal);
	       Real distToMean = realVal - nbn.mean;
	       Real stdDev = sqrt(nbn.var);
	       Real e2The = exp( -1 * distToMean * distToMean / (2*nbn.var) );
	       prob[labelVal] *= e2The / stdDev;
		  
	       LOG(5, " P(" << labelVal << "): times " << e2The / stdDev
		   << ", X = " << realVal << ",  Probability so far = " <<
		   prob[labelVal] << endl);
	    }
	 }
	 else { // nominal attribute
	    LOG(4, endl << "Nominal Attribute " << attrNum << endl);
	 
	    if ( ! schema.attr_info(attrNum).can_cast_to_nominal() )
	       err << "NaiveBayesCat::categorize: Schema of instance to be "
		  "categorized does not match Schema of training set. "
		  "Attribute Number " << attrNum << " is nominal in training "
		  "set and continuous in instance schema." << fatal_error;
	 
	    const NominalVal nomVal = 
	       schema.attr_info(attrNum).get_nominal_val(instance[attrNum]);
	    // Prob(Attr = a)
	    int totMatch = nominCounts.attr_count(attrNum, nomVal);
	    DBG(ASSERT(totMatch >= 0));

	    // if there are no occurences of instance's attribute val,
	    // skip this attribute
	    if ( totMatch < 0 )
	       LOG(4, "Value for attribute " << attrNum << " never appeared "
		   << " before." << endl);
	    else {
	       // loop through each label val, updating cumulative vector
	       // if no labels present, use noMatchesFactor to create
	       // a default probability > 0
	       for (labelVal = prob.low(); labelVal <= prob.high();labelVal++){
		  // this include unknowns in the label count: (langley didn't)
		  int labelCount = nominCounts.label_count(labelVal);
		  int labelMatch = nominCounts.val_count(labelVal,
							 attrNum, nomVal);
		  Real occurProb;
		  if ( labelMatch )
		     occurProb = Real(labelMatch) / labelCount;
		  else
		     occurProb = noMatchesFactor / numTrainInst;
		  prob[labelVal] *= occurProb;
		  LOG(4, "P(L=" << labelVal << " | Attr" << attrNum
		      << " = " << nomVal << ") = " << labelMatch 
		      << '/' << labelCount << " = " << occurProb
		      << ".  Cumulative prob = " << prob[labelVal] << endl);
	       }
	    }
	 }
	 // Since these are unscaled relative probabilities, we rescale them
	 //   to 0-1, so that we don't get underflows
	 rescale_prob(prob);
	 LOG(4, "Relative probabilities after scaling: " << endl);
	 IFLOG(4, log_prob(prob, schema));
      } // if unknown
   }


   // Find highest probability, and if equal, the one with more instances.
   Category mostLikely;
   int maxLabelCount = -1;
   Real maxProb = -1;

   for (labelVal = prob.low(); labelVal <= prob.high(); labelVal++) {
      ASSERT(prob[labelVal] >= 0); // >= 0 really
      Real diff = prob[labelVal] - maxProb;
      if (diff > REAL_EPSILON) {		// new best instance found
	 mostLikely = labelVal;
	 maxProb = prob[labelVal];
	 maxLabelCount = nominCounts.label_count(labelVal);
      }
      else if (( fabs( diff ) <= REAL_EPSILON ) && 	// equal instance
		(nominCounts.label_count(labelVal) > maxLabelCount ) ) {
	 mostLikely = labelVal;			// prefer higher # of occur
	 maxProb = prob[labelVal];
	 maxLabelCount = nominCounts.label_count(labelVal);
      }
   }

   // This assert can be removed when unknown label vals are supported
   //   by MLC++
   ASSERT( mostLikely != UNKNOWN_CATEGORY_VAL );
   
   MStringRC categoryString = schema.category_to_label_string(mostLikely);
   AugCategory augMostLikely(mostLikely, categoryString);

   LOG(3, "Best category: " << augMostLikely
       << " (Occurs " << maxLabelCount << " times in training set)"
       << endl << endl);
   
   return augMostLikely;
}

 
/***************************************************************************
  Description : Prints a readable representation of the Cat to the
                  given stream.
  Comments    : 
***************************************************************************/
void NaiveBayesCat::display_struct(MLCOStream& stream,
			    const DisplayPref& dp) const
{
   if ( stream.output_type() == XStream )
      err << "NaiveBayesCat::display_struct: Xstream is not a valid "
          << "stream for this display_struct"  << fatal_error;

   if ( dp.preference_type() != DisplayPref::ASCIIDisplay )
      err << "NaiveBayesCat::display_struct: Only ASCIIDisplay is "
          << "valid for this display_struct"  << fatal_error;
      

   stream << "Simple NaiveBayes Cat " << description()
	  << " categorizing using prevalence data in bag: "  << endl
          << nominCounts << endl;
   
   if ( continNorm != NULL ) {
      stream << "Categorizing uses Normal Density to estimate probability"
	 " of continuous attributes.  The mean, variance, and standard"
	 " deviation of each attribute,label combination is: " << endl;
      for (int i = 0; i < numAttributes; i++) {
	 if ( nominCounts.value_counts()[i] != NULL )    // nominal attribute
	    stream <<  "Attribute " << i << ":" << " Nominal Attribute."
		   << endl;
	 else {
	    stream << "Attribute " << i << ":" << endl;
	    for (int j = 0; j < num_categories(); j++) 
	       stream << "  Label " << j
		      << "\t\t" << (*continNorm)(i,j).mean
		      << "\t" << (*continNorm)(i,j).var << endl;
	 }
      }
   }
}


/***************************************************************************
  Description : Returns a pointer to a deep copy of this NaiveBayesCat.
  Comments    :
***************************************************************************/
Categorizer* NaiveBayesCat::copy() const
{
   return new NaiveBayesCat(*this, ctorDummy);
}


/***************************************************************************
  Description : Checks the argument categorizer to make sure its the same
                  type as this.  If so, use the specific operator ==.
  Comments    : Relies on unique categorizer #define in Categorizer.h
***************************************************************************/
Bool NaiveBayesCat::operator==(const Categorizer& cat) const
{
   if ( class_id() == cat.class_id() )
      return (*this) == (const NaiveBayesCat &) cat;
   return FALSE;
}


/***************************************************************************
  Description : Compares all elements of class for equality.
  Comments    :
***************************************************************************/
Bool NaiveBayesCat::operator==(const NaiveBayesCat &cat) const
{
  
   return ( ( (continNorm == NULL && cat.continNorm == NULL) 
	      || (*continNorm == *(cat.continNorm) ) ) &&
	    (nominCounts == cat.nominCounts) &&
	    (numTrainInst == cat.numTrainInst) &&
	    (num_categories() == cat.num_categories()) &&
	    (description()  == cat.description()) );
}






