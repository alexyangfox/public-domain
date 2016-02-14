#ifndef GXTRUECOLORMAPINFO_INCLUDED
#define GXTRUECOLORMAPINFO_INCLUDED

//this contains the same sort of information which can be gleaned from a RGBcolormap
//on Pseudo-color and Direct color displays. this is the info contained within the
//XStandardColormap structure that allows arbritrary color calculations.
//all values default to 0.

//this should probably be renamed to something that involves RGBColorMap.

class GxTrueColorMapInfo
{
public:
  GxTrueColorMapInfo(void);
  GxTrueColorMapInfo(const GxTrueColorMapInfo &rhs);
  ~GxTrueColorMapInfo(void);

  const GxTrueColorMapInfo & operator=(const GxTrueColorMapInfo &rhs);

  unsigned long red_max;
  unsigned long red_mult;
  unsigned long green_max;
  unsigned long green_mult;
  unsigned long blue_max;
  unsigned long blue_mult;
  unsigned long base_pixel;

  //HACK. THIS SHOULD RETURN A PIXEL VALUE
  //this takes arguments that range from 0 to 1
  //hackish. this is one of only a few places we accept floating point values in libGx.
  //Should we consider annother API?
  unsigned long GetPixel(float redVal, float greenVal, float blueVal);
};

#endif //GXTRUECOLORMAPINFO_INCLUDED
