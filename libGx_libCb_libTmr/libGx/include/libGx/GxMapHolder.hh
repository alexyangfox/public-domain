#ifndef GXMAPHOLDER_INCLUDED
#define GXMAPHOLDER_INCLUDED

#include <list>

#include <libGx/GxInc.hh>
#include <libGx/GxCoreWinMap.hh>

//hack-> included in GxMapHolder.cc
//#include <libGx/GxCoreWin.hh>
class GxCoreWin;

class GxSubMapHolder; //a class which inherits from me

class GxMapHolder
{
public:
  GxMapHolder(void);
  virtual ~GxMapHolder(void);

  //The order of adding GxSubMapHolders to my list is unimportant.
  void AddSubMapHolder(GxSubMapHolder *pNewSMH);
  void RemoveSubMapHolder(GxSubMapHolder *pOldSMH);

  //add and remove the window from my win-map
  void ManageNewWin(GxCoreWin *pWin, Window winID);
  void UnManageWin(Window winID);

  //handle the event within myself or within one of my SubWinMaps; return
  //TRUE if sucessful
  virtual bool SendXEventLocally(const XEvent &rEvent);
  //called by one of my GxSubMapHolder children; sees if the event belongs
  //to me if so handle it; if not try all of my (other) submapholder children.
  //we pass in pFrom to prevent from checking a GxMapHolder twice
  //(doing so would be very expensive and might cause a loop)
  virtual void SendUnusedEvent(GxSubMapHolder *pFrom, const XEvent &rEvent);

protected:
  //thie list of sub-maps that we should try to send events to if we fail
  //do deliver an event to on this level.
  std::list<GxSubMapHolder*> subMapList; //?slist?
  GxCoreWinMap winMap;
};

#endif //GXMAPHOLDER_INCLUDED
