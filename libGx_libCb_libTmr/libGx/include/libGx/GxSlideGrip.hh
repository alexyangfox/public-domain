#ifndef GXSLIDEGRIP_INCLUDED
#define GXSLIDEGRIP_INCLUDED

#include <libCb/CbCallback.hh>

#include <libGx/GxWin.hh>

//we are active by default
class GxSlideGrip : public GxWin
{
public:
  virtual ~GxSlideGrip(void);

  bool Active(void) const;
  void Active(bool newState);
protected:
  GxSlideGrip(GxRealOwner *pOwner, const CbOneBase<int> &rCB);
  void GetWindowData(XSetWindowAttributes &winAttributes, ULINT &valueMask);

  void DrawSlideGrip(void);
  bool active; //if not active do not call cb on events.
  CbOneFO<int> cb;
};

class GxVSlideGrip : public GxSlideGrip
{
public:
  GxVSlideGrip(GxRealOwner *pOwner, const CbOneBase<int> &rCB);
  virtual ~GxVSlideGrip(void);

  void HandleEvent(const XEvent &rEvent);
private:
  void HandleGrabbedEvents(const XEvent &rEvent);

  int relY;
  GxEventHandlerID pushedHandlerID;
};

class GxHSlideGrip : public GxSlideGrip
{
public:
  GxHSlideGrip(GxRealOwner *pOwner, const CbOneBase<int> &rCB);
  virtual ~GxHSlideGrip(void);

  void HandleEvent(const XEvent &rEvent);
private:
  void HandleGrabbedEvents(const XEvent &rEvent);

  int relX;
  GxEventHandlerID pushedHandlerID;
};

#endif //GXSLIDEGRIP_INCLUDED
