#ifndef GXTOOLTIP_INCLUDED
#define GXTOOLTIP_INCLUDED

#include <libGx/GxRootTransient.hh>

class GxToolTip : public GxRootTransient
{
public:
  GxToolTip(GxRealOwner *pOwner);
  GxToolTip(GxRealOwner *pOwner, const char *pText);
  virtual ~GxToolTip(void);

  virtual void Place(void);

  void HandleEvent(const XEvent &rEvent);

protected:
  //this sets critical information: if subclases over-load this should
  //always call this via GxMenuPane::GetWindowData in the overloaded function
  virtual void GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask);
  char textBuffer[GX_DEFAULT_LABEL_LEN];
};

#endif //GXTOOLTIP_INCLUDED
