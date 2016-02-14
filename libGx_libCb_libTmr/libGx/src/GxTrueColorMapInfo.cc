#include <libGx/GxTrueColorMapInfo.hh>

GxTrueColorMapInfo::GxTrueColorMapInfo(void) :
  red_max(0),
  red_mult(0),
  green_max(0),
  green_mult(0),
  blue_max(0),
  blue_mult(0),
  base_pixel(0)
{}

GxTrueColorMapInfo::GxTrueColorMapInfo(const GxTrueColorMapInfo &rhs) :
  red_max(rhs.red_max),
  red_mult(rhs.red_mult),
  green_max(rhs.green_max),
  green_mult(rhs.green_mult),
  blue_max(rhs.blue_max),
  blue_mult(rhs.blue_mult),
  base_pixel(rhs.base_pixel)
{}

GxTrueColorMapInfo::~GxTrueColorMapInfo(void)
{}

const GxTrueColorMapInfo & GxTrueColorMapInfo::operator=(const GxTrueColorMapInfo &rhs)
{
  red_max = rhs.red_max;
  red_mult = rhs.red_mult;
  green_max = rhs.green_max;
  green_mult = rhs.green_mult;
  blue_max = rhs.blue_max;
  blue_mult = rhs.blue_mult;
  base_pixel = rhs.base_pixel;

  return *this;
}

unsigned long GxTrueColorMapInfo::GetPixel(float redVal, float greenVal, float blueVal)
{
  //Pixel pixVal; //hack. this should be used
  unsigned long pixVal = (redVal*((float)red_max))*red_mult +
    (greenVal*((float)green_max))*green_mult +
    (blueVal*((float)blue_max))*blue_mult +
    base_pixel;

  return pixVal;
}
