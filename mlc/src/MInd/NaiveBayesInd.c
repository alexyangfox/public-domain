// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.

/*****************************************************************************
  Description  : NaiveBayesInd implements a simple occurance prevalence-
                   based categorizer, namely NaiveBayesCategorizer.  For
		   discrete attributes, the categorizer requires prevalences
		   for each attribute value, label combination.  For continuous
		   attributes, Normal Density coefficients for each attribute,
		   label pair are needed.  The data on discrete
		   attributes are tabulated in a BagCounter; continuous
		   Normal Densities are calculated with the StatData class
		   and stored as pairs of means and variances.

  Assumptions  : 
  Comments     : 
  Complexity   : 
  Enhancements :
  History      : Yeogirl Yun                                   7/4/95
                   Added copy constructor and copy() method.
                 Robert Allen
                   Initial Revision(.c .h)                     11/27/94
*****************************************************************************/
#include <basics.h>
#include <MLCStream.h>
#include <NaiveBayesInd.h>
#include <CtrBag.h>
#include <Array2.h>
#include <StatData.h>
#include <math.h>


RCSID("MLC++, $RCSfile: NaiveBayesInd.c,v $ $Revision: 1.6 $")

// Value to be used for variance of continuous attributes when the
// actual variance is 0.  This is public, so the user can modify it directly.
// Need this to keep from deviding by 0.

Real NaiveBayesInd::epsilon = 0.01;

// Value to be used for variance of continuous attributes when the
// actual variance is undefined, such as when there is only one instance.

Real NaiveBayesInd::defaultVariance = 1.0;

/*****************************************************************************
  Description  : Initializes data members.
  Comments     : Private.
*****************************************************************************/
void NaiveBayesInd::clear()
{
   categorizer = NULL;
   normDens = NULL;
   cAttrCount = 0;
}


/*****************************************************************************
  Description  : Re-initializes data members.
  Comments     : Private.
*****************************************************************************/
void NaiveBayesInd::cleanup()
{
   delete categorizer;
   delete normDens;
   clear();
}
     

/*****************************************************************************
  Description  : Constructor with description MString.
  Comments     :  
*****************************************************************************/
NaiveBayesInd::NaiveBayesInd(const MString& description)
   : CtrInducer(description)
{
   categorizer = NULL;
   normDens = NULL;
}


/*****************************************************************************
  Description : Copy constructor.
  Comments    :
*****************************************************************************/
NaiveBayesInd::NaiveBayesInd(const NaiveBayesInd& source, CtorDummy)
   : CtrInducer(source, ctorDummy)
{
   categorizer = NULL;
   normDens = NULL;   
}   


/*****************************************************************************
  Description  : Deallocates categorizer.
  Comments     :
*****************************************************************************/
NaiveBayesInd::~NaiveBayesInd()
{
   DBG(OK());
   delete categorizer;
   delete normDens;
}


/*****************************************************************************
  Description  : Return TRUE iff the class has a valid categorizer.
  Comments     :
*****************************************************************************/
Bool NaiveBayesInd::was_trained(Bool fatal_on_false) const
{
   if( fatal_on_false && categorizer == NULL )
      err << "NaiveBayesInd::was_trained: No categorizer, "
	 "Call train() to create categorizer" << fatal_error;
   
   return ( categorizer != NULL );
}


/*****************************************************************************
  Description  : Returns the categorizer that the inducer has generated.
  Comments     :
*****************************************************************************/
const Categorizer& NaiveBayesInd::get_categorizer() const
{
   was_trained(TRUE);
   return *categorizer;
}


/*****************************************************************************
  Description  : Trains Naive Bayes Categorizer.  Descrete attributes can
                   be handled by simply passing occurance counts in the
		   bagCounters bag.  Continuous attributes are fed into
		   StatDatas to get mean, variance, and standard deviation.
  Comments     : It is possible that some label is not in the training set, or
		   it has unknown for a continous attribute.  In this case,
		   statData::size() = 0, and the loop below will not write
		   any data into the NBNorm structure.  Since the NBNorm
		   structure is initialized to HasData = FALSE, doing nothing
		   will correctly indicate that there is no data.
*****************************************************************************/
void NaiveBayesInd::train()
{
   has_data();
   DBG(OK());

   int numCategories = TS_with_counters().num_categories();
   const SchemaRC schema = TS->get_schema();
   int numAttributes = schema.num_attr();

   cleanup();   // reinitializes data

   // start labels at -1 for unknown
   normDens = new Array2<NBNorm>(0, -1, numAttributes, numCategories + 1);

   // loop through each attribute, and process all instances for each
   // continuous one
   for (int attrNum = 0; attrNum < numAttributes; attrNum++) {
      const AttrInfo& attrinfo = schema.attr_info(attrNum);
      if (attrinfo.can_cast_to_real()) {
	 // this is a continuous attribute
	 cAttrCount++;
	 
	 // read each occurance in the bag and feed the stats for attribute
	 Array<StatData> continStats(-1, numCategories + 1);
	 for (Pix pix = TS->first(); pix; TS->next(pix)) {
	    const InstanceRC& inst = instance_bag().get_instance(pix);
	    int labelVal = schema.label_info().cast_to_nominal().
     	                   get_nominal_val(inst.get_label());
	    DBG(ASSERT(labelVal < numCategories));

            // Ignore unknowns.
	    if ( !attrinfo.is_unknown(inst[attrNum])) {
	       Real value = attrinfo.get_real_val(inst[attrNum]);
	       continStats[labelVal].insert( value );
	    }
	 }

	 Real mean, var;
	 // extract Normal Density parameters into normDens table
	 for (int label = -1; label < numCategories; label++) {
	    if (continStats[label].size() == 0 ) {
	       mean = 0;
	       var = NaiveBayesInd::defaultVariance;
	    }
	    else {
	       mean = continStats[label].mean();
	       if (continStats[label].size() == 1 )
		  var = NaiveBayesInd::defaultVariance;
	       
	       else if ( !(var = continStats[label].variance()) )   // var == 0
		  var = NaiveBayesInd::epsilon;
	    }
	    (*normDens)(attrNum, label).mean    = mean;
	    (*normDens)(attrNum, label).var     = var;
	    (*normDens)(attrNum, label).hasData = TRUE;
	       
	    LOG(3, " Continuous Attribute # " << attrNum <<
		", Label " << label << ": Mean = " << mean <<
		", Variation = " << var << endl );
	 }
      } // end of handling this continous attribute
   }    // end of loop through all attributes

   if (!cAttrCount) {  // no continous attributes found
      delete normDens;
      normDens = NULL;
   }


   categorizer = new NaiveBayesCat( description(),
				    TS_with_counters().num_categories(),
				    TS_with_counters().counters(),
				    normDens,
				    TS_with_counters().num_instances());
   categorizer->set_log_level(get_log_level());
}


/*****************************************************************************
  Description  : Cast to IncrInducer test.  This ain't one.
  Comments     : Inline function in .h file.  Returns FALSE.
*****************************************************************************/

 
/***************************************************************************
  Description : Prints a readable representation of the Cat to the
                  given stream.
  Comments    : 
***************************************************************************/
void NaiveBayesInd::display(MLCOStream& stream,
				       const DisplayPref& dp) const
{
   if (stream.output_type() == XStream)
      err << "NaiveBayesInd::display_struct: Xstream is not a valid "
          << "stream for this display_struct"  << fatal_error;

   if (dp.preference_type() != DisplayPref::ASCIIDisplay)
      err << "NaiveBayesInd::display_struct: Only ASCIIDisplay is "
          << "valid for this display_struct"  << fatal_error;
      

   stream << "NaiveBayes Inducer " << description() << endl;
   if ( was_trained(FALSE) ) {
      stream << "Current Categorizer "  << endl;
      get_categorizer().display_struct(stream, dp);
   }
}


/*****************************************************************************
  Description : Returns the pointer to the copy of *this.
  Comments    :
*****************************************************************************/
Inducer* NaiveBayesInd::copy() const
{
   Inducer *ind = new NaiveBayesInd(*this, ctorDummy);
   return ind;
}
   
   


DEF_DISPLAY(NaiveBayesInd);



