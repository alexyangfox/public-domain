#ifndef GXVDIVIDER_INCLUDED
#define GXVDIVIDER_INCLUDED

#include <libGx/GxWin.hh>
#include <libGx/GxOwnerWin.hh>

//this has much in common with the GxHSlideGrip

class GxVDivider : public GxWin
{
public:
  GxVDivider(GxRealOwner *pOwner);
  virtual ~GxVDivider(void);

  virtual UINT GetDesiredWidth(void) const;

  void HandleEvent(const XEvent &rEvent);

  //called with the x offset of the grip
  CbOneFO<int> cb;

protected:
  void HandleGrabbedEvents(const XEvent &rEvent);

  void Draw(void);

  //overloaded for events _and_ for the proper cursor
  virtual void GetWindowData(XSetWindowAttributes &winAttrib, ULINT &valueMask);

  int relX;
  GxEventHandlerID pushedHandlerID;
};

#endif //GXVDIVIDER_INCLUDED
