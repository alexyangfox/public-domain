// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : AccData is a StatData which is specifically tuned to hold
                   results of accuracy estimation.  It therefore provides
		   convenience functions for handling accuracies.
		 AccData also allows storage of a real accuracy along with
		   the number of test instances used to derive that
		   accuracy.
  Assumptions  : 
  Comments     : If no values have been inserted into the AccData, but a
                   real accuracy is defined, then the estimated accuracy
		   and variance calls will all return values for the
		   real versions.  This is to allow use of testSet as an
		   accuracy estimation method.
  Complexity   : 
  Enhancements : Update AccData to take into account the
                   different sizes of folds by using weights in AccData.
  History      : Dan Sommerfield                                    4/12/95
                   Initial revision (.h,.c) based on previous AccData
		   bundled with AccEstimator.
***************************************************************************/

#include <basics.h>
#include <AccData.h>
#include <CatTestResult.h>

RCSID("MLC++, $RCSfile: AccData.c,v $ $Revision: 1.6 $")

int AccData::defaultPrecision = 2;

/***************************************************************************
  Description : Invariants
  Comments    :
***************************************************************************/
void AccData::OK() const
{
   // if the real accuracy is defined, make sure it is in range
   if(realAccuracy != -1) {
      ASSERT(realAccuracy >= 0.0);
      ASSERT(realAccuracy <= 1.0);
      ASSERT(numTestInstances >= 0);
   }
}

/***************************************************************************
  Description : Set real accuracy information
  Comments    : A test set size of 0 is permitted to indicate a real
                  accuracy which is not derived from testing.
***************************************************************************/
void AccData::set_real(Real realAcc, int n)
{
   if(realAcc < 0.0 || realAcc > 1.0)
      err << "AccData::set_real: Real accuracy " << realAcc <<
	 "is out of range [0.0,1.0]" << fatal_error;

   if(n < 0)
      err << "AccData::set_real: test set size (" << n << ") must "
	 "not be negative" << fatal_error;

   realAccuracy = realAcc;
   numTestInstances = n;
}

/***************************************************************************
  Description : Determine estimated accuracy
  Comments    : If no data in the AccData, then use the real value instead
***************************************************************************/
Real AccData::accuracy(Real trim) const
{
   if(size())
      return mean(trim);
   else
      return real_accuracy();
}

/***************************************************************************
  Description : Determine estimated std dev
  Comments    : If no data in the AccData, then use the theoretical value
***************************************************************************/
Real AccData::std_dev(Real trim) const
{
   if(size()) {
      check_std_dev();
      return std_dev_of_mean(trim);
   }
   else
      return theo_std_dev();
}

/***************************************************************************
  Description : Determine estimated confidence interval
  Comments    : Use theoretical value if no data
***************************************************************************/
void AccData::confidence(Real& low, Real& high) const
{
   if(size()) {
      check_std_dev();
      percentile(confidenceIntervalProbability, low, high);
   }
   else
      theo_confidence(low, high);
}


/***************************************************************************
  Description : Return real accuracy.  Abort if none defined
  Comments    : 
***************************************************************************/
Real AccData::real_accuracy() const
{
   check_real();
   return realAccuracy;
}

/***************************************************************************
  Description : Return theoretical std dev
  Comments    : 
***************************************************************************/
Real AccData::theo_std_dev() const
{
   check_theo_std_dev();
   return CatTestResult::theoretical_std_dev(
      realAccuracy, numTestInstances);
}

/***************************************************************************
  Description : Get theoretical confidence bounds
  Comments    : 
***************************************************************************/
void AccData::theo_confidence(Real& low, Real& high) const
{
   check_theo_std_dev();
   CatTestResult::confidence(low, high, realAccuracy, numTestInstances);
}

/***************************************************************************
  Description : Make sure that the real accuracy actually exists
  Comments    : 
***************************************************************************/
void AccData::check_real() const
{
   if(!has_real())
      err << "AccData::check_real: real accuracy is undefined" <<
	 fatal_error;
}

/***************************************************************************
  Description : Make sure that the estimated accuracy exists
  Comments    : 
***************************************************************************/
void AccData::check_estimated() const
{
   if(!has_estimated())
      err << "AccData::check_estimated: estimated accuracy is undefined"
	 " (no data has been inserted)" << fatal_error;
}

/***************************************************************************
  Description : Make sure that an estimated std dev exists
  Comments    : 
***************************************************************************/
void AccData::check_std_dev() const
{
   if(!has_std_dev())
      err << "AccData::check_std_dev: standard deviation is undefined"
	 " (not enough data to compute variance)" << fatal_error;
}


/***************************************************************************
  Description : Make sure that a theoretical std dev exists
  Comments    : 
***************************************************************************/
void AccData::check_theo_std_dev() const
{
   if(!has_theo_std_dev())
      err << "AccData::check_theo_std_dev: theoretical standard deviation "
	 " is undefined (not enough test instances)" << fatal_error;
}

/***************************************************************************
  Description : Append another AccData onto this one.  Real accuracy is
                  set if it is defined for either AccData.  If it is
		  defined for both and they differ, it is an error.
  Comments    : 
***************************************************************************/
void AccData::append(const AccData& other)
{
   StatData::append(other);
   if(has_real() && other.has_real()) {
      if(realAccuracy != other.realAccuracy)
	 err << "AccData::append: "
	    "Attempting to append non-matching AccData:  Real accuracy "
	    << realAccuracy << " != " << other.realAccuracy << fatal_error;
      if(numTestInstances != other.numTestInstances)
	 err << "AccData::append: "
	    "Attempting to append non-matching AccData:  number of "
	    "test instances " << numTestInstances << " != " <<
	    other.numTestInstances << fatal_error;
   }
   else if(other.has_real()) {
      realAccuracy = other.realAccuracy;
      numTestInstances = other.numTestInstances;
   }
   evalCost += other.evalCost;
}


/***************************************************************************
  Description : Show some basic statistics.
  Comments    : The output is tailored to show only what measures exist.
                  i.e., if there's no known variance, we don't display
		  it.
***************************************************************************/
void AccData::display(MLCOStream& stream, Real trim, 
			     int precision) const
{
   // if no real or estimated, indicate empty AccData
   if(!has_real() && !has_estimated())
      stream << "(empty)";

   // otherwise display estimated accuracy
   else {
      stream << MString(accuracy(trim) * 100, precision) << "%";
      if(!has_estimated())
	 stream << " (test set)";
      if(has_std_dev()) {
	 Real low, high;
	 confidence(low, high);
	 stream << " +- " << MString(std_dev(trim) * 100, precision) <<
	    "% (" <<
	    MString(low * 100, precision) << "% - " <<
	    MString(high * 100, precision) << "%)";
      }
      if(has_real()) {
	 stream << "." << endl <<
	    "Test Set: " << MString(real_accuracy() * 100, precision)
	    << "%";
	 if(has_theo_std_dev()) {
	    Real low, high;
	    theo_confidence(low, high);
	    stream << " +- " << MString(theo_std_dev() * 100, precision)
		   << "% [" << MString(low * 100, precision) << "% - " <<
	             MString(high * 100, precision) << "%]";
	 }
	 stream << ".  Bias: " << MString(bias(trim) * 100, precision)
	    << "%";
      }
   }
}

DEF_DISPLAY(AccData);

/***************************************************************************
  Description : Display statistics information in a format usable by
                  dot/dotty.  This includes abbreviating the information
		  and using '\\n' for newlines.
  Comments    : The output is tailored to show only what measures exist.
                  i.e., if there's no known variance, we don't display
		  it.
***************************************************************************/
void AccData::dot_display(MLCOStream& stream, Real trim, 
			  int precision) const
{
   // if no real or estimated, indicate empty AccData
   if(!has_real() && !has_estimated())
      stream << "(empty)";

   // otherwise display estimated accuracy
   else {
      stream << "est: ";
      stream << MString(accuracy(trim) * 100, precision) << "%";
      if(has_std_dev())
	 stream << " +- " << MString(std_dev(trim) * 100, precision) <<
	    "%";
      if(has_real()) {
	 stream << "\\n" << 
	    "real: " << MString(real_accuracy() * 100, precision)
	    << "%";
	 if(has_theo_std_dev())
	    stream << " +- " << MString(theo_std_dev() * 100, precision)
		   << "%";
      }
   }
}





