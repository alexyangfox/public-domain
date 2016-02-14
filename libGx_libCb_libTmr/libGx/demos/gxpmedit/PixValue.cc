#include "PixValue.hh"

PixValue::PixValue(void) :
  xcolor(0), masked(false), selected(false)
{}

PixValue::PixValue(unsigned long initValue) :
  xcolor(initValue), masked(false), selected(false)
{}

PixValue::PixValue(const PixValue &rhs) :
  xcolor(rhs.xcolor), masked(rhs.masked), selected(rhs.selected)
{}

PixValue::~PixValue(void)
{}

const PixValue& PixValue::operator=(const PixValue &rhs)
{
  xcolor = rhs.xcolor;
  masked = rhs.masked;
  selected = rhs.selected;

  return *this;
}
