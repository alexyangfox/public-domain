#ifndef GXVLINE_INCLUDED
#define GXVLINE_INCLUDED

#include <libGx/GxWin.hh>

class GxVLine : public GxWin
{
public:
  GxVLine(GxRealOwner *pOwner);
  virtual ~GxVLine(void);

  void HandleEvent(const XEvent &rEvent);

protected:
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);

};

#endif //GXVLINE_INCLUDED
