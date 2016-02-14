#ifndef GXFRACTION_INCLUDED
#define GXFRACTION_INCLUDED

#include <iostream>
#include <libGx/GxInc.hh>

//GxMaxFraction and GxMinFraction are defined below

//a class which exists so we don't have to use floating point in the library
class GxFraction
{
public:
  GxFraction(void);
  //must be able to take any values, no matter how bad (like 0 for denominator)
  //and keep fraction as a sane value
  GxFraction(UINT numerator, UINT denominator);
  ~GxFraction(void);

  //effectivly multiplies the argument by the "fraction" and returns the result
  int Convert(int arg) const;
  UINT Convert(UINT arg) const;

  friend bool operator!=(const GxFraction &lhs, const GxFraction &rhs);
  friend bool operator==(const GxFraction &lhs, const GxFraction &rhs);
  friend std::ostream& operator<<(std::ostream& rOut, GxFraction& rFr);

  void Set(UINT fract);
private:
  //valid range for this UINT 0-10000
  UINT fraction;
};

//?hack? should these be declared in GxInc.hh?
const GxFraction GX_MAX_FRACTION = GxFraction(1,0);
const GxFraction GX_MIN_FRACTION = GxFraction(0,1);

#endif //GXFRACTION_INCLUDED
