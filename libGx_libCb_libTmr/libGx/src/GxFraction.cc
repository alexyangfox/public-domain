#include <libGx/GxFraction.hh>

//hack; should this be the max int value?
//this should not be used anywhere except internal to GxFraction
const UINT GxMaxFractionValue = 10000;

GxFraction::GxFraction(void) :
  fraction(0)
{}

GxFraction::GxFraction(UINT numerator, UINT denominator)
{
  if(denominator == 0)
    {
      fraction = GxMaxFractionValue;
      return;
    };
  //figure out a multiplier using the denominator. basicly:
  //mult = GxMaxFractionValue/denominator;
  //fraction = mult * numerator;
  //I multiply by 65 to try and retain accuracy.
  ULINT mult = (((ULINT)GxMaxFractionValue * (ULINT)65)/(ULINT)denominator);
  Set( (UINT)(((ULINT)numerator * mult)/65 ));
}

GxFraction::~GxFraction(void)
{}

int GxFraction::Convert(int arg) const
{
  //VERY IMPORTANT THAT WE WORK WITH LINT'S HERE
  LINT temp = arg*fraction;
  return (int)(temp/GxMaxFractionValue);
}

//hack? can this overflow
UINT GxFraction::Convert(UINT arg) const
{
  ULINT temp = arg*fraction;
  return (UINT)(temp/GxMaxFractionValue);
}

void GxFraction::Set(UINT fract)
{
  fraction = (fract > GxMaxFractionValue) ? GxMaxFractionValue : fract;
}

bool operator!=(const GxFraction &lhs, const GxFraction &rhs)
{
  if(lhs.fraction != rhs.fraction)
    return true;
  else
    return false;
}

bool operator==(const GxFraction &lhs, const GxFraction &rhs)
{
  if(lhs.fraction == rhs.fraction)
    return true;
  else
    return false;
}

std::ostream& operator<<(std::ostream& rOut, GxFraction& rFr)
{
  return rOut << rFr.fraction << "/" << GxMaxFractionValue;
}
