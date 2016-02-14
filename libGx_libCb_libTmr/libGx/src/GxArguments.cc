#include <libGx/GxArguments.hh>

GxArguments::GxArguments(void) :
  dispSync(false), mainWinWidth(0), mainWinHeight(0)
{}

GxArguments::GxArguments(const GxArguments &rhs) :
  displayName(rhs.displayName), dispSync(rhs.dispSync),
  mainWinWidth(rhs.mainWinWidth), mainWinHeight(rhs.mainWinHeight)
{}

GxArguments::~GxArguments(void)
{}

GxArguments& GxArguments::operator=(const GxArguments &rhs)
{
  displayName = rhs.displayName;
  dispSync = rhs.dispSync;
  mainWinWidth = rhs.mainWinWidth;
  mainWinHeight = rhs.mainWinHeight;

  return *this;
}
