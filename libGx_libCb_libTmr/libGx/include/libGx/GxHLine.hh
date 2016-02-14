#ifndef GXHLINE_INCLUDED
#define GXHLINE_INCLUDED

#include <libGx/GxWin.hh>

class GxHLine : public GxWin
{
public:
  GxHLine(GxRealOwner *pOwner);
  virtual ~GxHLine(void);

  void HandleEvent(const XEvent &rEvent);
protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
};

#endif //GXHLINE_INCLUDED
