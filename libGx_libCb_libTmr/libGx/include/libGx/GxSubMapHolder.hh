#ifndef GXSUBMAPHOLDER_INCLUDED
#define GXSUBMAPHOLDER_INCLUDED

#include <libGx/GxMapHolder.hh>

//hack-> included in GxSubMapHolder.cc
//#include <libGx/GxCoreWin.hh>

class GxSubMapHolder : public GxMapHolder
{
public:
  GxSubMapHolder(GxMapHolder *ptPrevHolder);
  virtual ~GxSubMapHolder(void);

  //this is sort of a hack
  virtual void SendXEventUp(const XEvent &rEvent);

  //virtually identical to the GxMapHolder function we are overloading; except
  //if we do not handle the event within myself or in one of my children;
  //pass it on to pPrevHolder via its HandleUnusedEvent
  virtual void SendUnusedEvent(GxSubMapHolder *pFrom, const XEvent &rEvent);

private:
  GxMapHolder *pPrevHolder;
};

#endif //GXSUBMAPHOLDER_INCLUDED
