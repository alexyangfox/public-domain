// MLC++ - Machine Learning Library in -*- C++ -*-
// See Descrip.txt for terms and conditions relating to use and distribution.
/***************************************************************************
  Description  : StatData is a class for doing statistical
                  computations.
		 Insert puts an item into the StatData.
		 Operator [] returns a const item, but the order
                   may change if you call any function other than
                   operator[] and size().
		 mean() returns the mean of the values which have been
		   inserted into the StatData.  A trimming proportion
                   alpha may be given (0-.5).  A trimmed mean is a robust
                   statistics that ignores the [n alpha]100% highest
                   and the [n alpha]100% lowest values.
		 variance() returns the (unbiased) sample variance of the items
  		   currently stored in the StatData.  The trimmed variance
                   is computed according to Mathematical Statistics and
                   Data Analyais / John A. Rice, pp 332-333.  This is based 
                   on the Winsorized sample variance defined in 
                   Mathematical Statistics by Bickel and Doksum.
		 std_dev() returns the sqrt of the variance.
                 variance_of_mean() and std_dev_of_mean() return the
                   appropriate values for the mean, instead of for the
                   population. 
  Assumptions  : A fatal error is triggered if an attempt is made to insert
                   more than the maximum number of specified items.
                   This will be relaxed in the future.
  Comments     : The documentation for the Winsorized sample variance is
                   lacking in clarity.  Bickel & Doksum give an example
                   without specifying alpha.  Rice gives a very obscure
                   description, and we were able to derive his example
                   value by dividing by n-1 (instead of n as he says).
                   This agrees with trimming of zero, so we preferred it.
  Complexity   : Mean, variance, and std_dev are O(n) where n is the number
                   of elements in the data. 
  Enhancements : The trimmed mean should really trim the outer values by
                    a weighted factor.  This is suggested in "Robust Estimates 
                    of Location, Survey and Advances" by Andrews, Bickel,
		    Hampel, Huber, Rogers, Tukey, page 7.  [\alpha n]
                    points are removed at each end, and the largest and
		    smallest are given weight 1+[\alpha n] - \alpha n.  A
		    weighted mean is then taken.
                 Allow weighted instances.  This will help CValidator a lot.
                 Compute the variance in constant time.  This is possible
                   if we keep the sum of the squared values.  I'm not sure we
		   can do this for trimmed means easily.
                 The sample variance can computed as follows:
                    S^2 = 1/(n-1) \sum_{i=1}^n (X_i - \overline{X})^2
                    (see for example Mathematical Statistics and Data
                     Analysis by John A. Rice, page 171).
                 To avoid a second pass, we do the following:
                    S^2 = 1/(n-1) [\sum X_i^2 - 2\overline{X}\sum X_i + 
                          \sum \overline{X}^2]
                        = 1/(n-1) [\sum X_i^2 - 2\overline{X} (n\overline{X}) +
                          \sum \overline{X}^2]
                        = 1/(n-1) [\sum X_i^2 - n\overline{X}^2]
		 The main problem is that this doesn't work well for trimming.
  History      : Yeogirl Yun                                       12/12/94
                   Added percentile method.
                 James Dougherty and Ronny Kohavi                   5/23/94
                   Initial revision (.h,.c)
***************************************************************************/

#include <math.h>
#include <basics.h>
#include <StatData.h>

RCSID("MLC++, $RCSfile: StatData.c,v $ $Revision: 1.26 $")

/***************************************************************************
  Description : check that the trimming factor is between 0 and 0.5
  Comments    :
***************************************************************************/

static void check_trim(Real alpha)
{
   if (alpha < 0 || alpha > 0.5)
      err << "StatData::check_trim: trimming value " << alpha
          << " is not in the range [0,0.5]" << fatal_error;
}
   


/***************************************************************************
  Description : const subscripting operator, returns an item in the array. 
  Comments    :
***************************************************************************/
Real StatData::operator [](int index) const
{
   if (index > size())
      err << "Error: StatData::operator[]: index " << index
          << " is out of bound" << fatal_error;

   return items[index];
}

/***************************************************************************
  Description : Inserts an item into the Array.
  Comments    : If an item is inserted into the array and there is no
                  more space available, we fall back on the Array's
 		  error checking for operator [].
***************************************************************************/
void StatData::insert(Real item)
{
   items[size()] = item;
}


/***************************************************************************
  Description : Calculates the trimmed mean of the data.
  Comments    : trim defaults to 0.  Trim = 0.5 now gives the median.
***************************************************************************/
Real StatData::mean(Real trim) const
{
   check_trim(trim);
   if (size() < 1)
      err << "StatData::mean: no data" << fatal_error;

   Real result = 0;
   ((StatData*)this)->items.sort();
   int low = (int)(size()*trim); // starting at 0
   int high = size() - low;
   if (low == high) {
      low--; high++;
   }
   for(int k = low; k < high; k++) 
      result += items[k];
   ASSERT(2*low < size()); // Make sure we're not dividing by zero.
   return result / (size() - 2*low);
}


/***************************************************************************
  Description : Calculates the variance, standard-deviation of the 
                  data.
  Comments    : trim defaults to 0.
***************************************************************************/
Real StatData::variance(Real trim) const
{
   check_trim(trim);
   if (size() < 2)
      return UNDEFINED_VARIANCE;
      
   Real xBar = mean(trim);       // sorted now.
   int low = (int)(size()*trim); // starting at 0
   int high = size() - low;
   
   Real result = low * (squareReal(items[low] - xBar) +
                        squareReal(items[high-1] - xBar));
   
   for(int k = low; k < high; k++) 
      result += squareReal(items[k] - xBar);
   
   return result / (squareReal(1 - 2*trim)*(size() - 1));
}


Real StatData::std_dev(Real trim) const
{
   Real var = variance(trim);
   if(var == UNDEFINED_VARIANCE)
      return UNDEFINED_VARIANCE;
   return sqrt(var);
}

/***************************************************************************
  Description : Calculate the variance, std-dev of the mean.
  Comments    : trim defaults to 0.
  ***************************************************************************/

Real StatData::variance_of_mean(Real trim) const
{
   Real var = variance(trim);
   if(var == UNDEFINED_VARIANCE)
      return UNDEFINED_VARIANCE;
   return var / size();
}


Real StatData::std_dev_of_mean(Real trim) const
{
   Real var = variance_of_mean(trim);
   if(var == UNDEFINED_VARIANCE)
      return UNDEFINED_VARIANCE;
   return sqrt(var);
}



/*****************************************************************************
  Description : Returns the confidence interval for the given confidence
                  probability
  Comments    : 
*****************************************************************************/
void StatData::percentile(Real confIntProb, Real& low, Real& high) const
{
   if (confIntProb > 1 || confIntProb < 0)
      err << "StatData::percentile: confidence interval should be "
	     "between 0 and 1.  "
	     "The given value was : " << confIntProb << fatal_error;
   if (size() < 1)
      err << "StatData::percentile: no data" << fatal_error;
   
   // Note, casting constness because logically we're const, but
   //   physically, this is done best by sorting.
   ((StatData*)this)->items.sort();

   // For some reason, if size=10, passing in .8 gives 0 without REAL_EPSILON
   //   on our Sparc.  Reason is that (double)1-.8-.2 is not 0.
   int lowIndex = (int)(size()*(1-confIntProb)/2 + REAL_EPSILON);
   low = items[lowIndex]; // starting at 0
   high = items[size() - lowIndex - 1];
}



/***************************************************************************
  Description : Generate histogram for the data
                In mathematica do << Graphics`Graphics`
  Comments    : cell computation is  floor ((x - (mean+w/2))/w + 1)
                  		     floor ((x - mean)/w + .5)
***************************************************************************/

// cell returns a cell number centered around 0
static int cell(Real value, Real mean, Real columnWidth)
{
   return floor((value - mean) / columnWidth + 0.5);
}

void StatData::display_math_histogram(Real columnWidth,
				      int precision, MLCOStream& stream)
{
   Real meanVal = mean();
   Real minVal  = items.min(); // could get first, but this is safer
   Real maxVal  = items.max();

   int low  = cell(minVal, meanVal, columnWidth);
   int high = cell(maxVal, meanVal, columnWidth);
   Array<int> hist(low, high - low + 1, 0);

   for (int i = items.low(); i <= items.high(); i++)
      hist[cell(items[i], meanVal, columnWidth)]++;

   stream << "BarChart[{" << hist << "}, Ticks->{{";
   for (i = 0; i < hist.size(); i++) {
      stream << '{' << i + 1 << ", " // space for wrapping purposes.
      << Mround((hist.low() + i) * columnWidth + meanVal, precision)
      << '}';
      if (i < hist.size() - 1)
	 stream << ',';
   }

   stream << "}, Automatic}]" << endl;
}

/***************************************************************************
  Description : Display the data
  Comments    :
***************************************************************************/

void StatData::display(MLCOStream& stream) const
{
   items.display(stream);
}

DEF_DISPLAY(StatData);


/***************************************************************************
  Description : Operator==,!= of data elements (order is irrelevant).
                Since these are real numbers, we allow some tolerance in
  		  equality. 
  Comments    : 
***************************************************************************/

Bool operator==(const StatData& sd1, const StatData& sd2)
{
   (void)sd1.mean(); // sort arrays so they are equal in any order
   (void)sd2.mean(); 

   const Array<Real>& a1 = sd1.items;
   const Array<Real>& a2 = sd2.items;
   
   if (a1.size() != a2.size())
       err << "StatData::operator== Arrays of different size: " <<
       a1.size() << " vs. " << a2.size() << fatal_error;

   for (int i = 0; i < a1.size(); i++)
      if (fabs(a1.index(i) - a2.index(i)) > REAL_EPSILON)
         return FALSE;

   return TRUE;
}


Bool operator!=(const StatData& sd1, const StatData& sd2)
{
   return !(sd1 == sd2);
}

/***************************************************************************
  Description : Append function adds the items of the other StatData to
                  the items from this StatData.  Use this function to
		  combine statistical data from more than one source.
  Comments    : 
***************************************************************************/
void StatData::append(const StatData& other)
{
   items.append(other.items);
}

/***************************************************************************
  Description : Clear function removes all items from the StatData.
                  This avoids having to use StatData pointers just to
		  allow you to clear out a StatData.
  Comments    : 
***************************************************************************/
void StatData::clear()
{
   items.truncate(0);
}

StatData& StatData::operator=(const StatData& other)
{
   clear();
   append(other);
   return *this;
}


/***************************************************************************
  Description : Compute squared error around the true value.
  Comments    :
***************************************************************************/

Real StatData::squaredError(Real trueVal) const
{
   if (size() < 1)
      err << "StatData::squaredError: no data" << fatal_error;

   Real result = 0;
   for(int k = 0; k < size(); k++) 
      result += squareReal(items[k] - trueVal);
   
   return result;
}



