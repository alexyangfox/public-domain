#include <libGx/GxMapHolder.hh>

#include <libGx/GxCoreWin.hh>
#include <libGx/GxSubMapHolder.hh>

using namespace std;

GxMapHolder::GxMapHolder(void)
{}

GxMapHolder::~GxMapHolder(void)
{
  //NOTE: WE ARE NOT DELETING THE ACTUAL GxSubMapHolders
  //these will be taken care of by whatever class owns them

  //hack! we should notify them that we are being destroyed.
}

void GxMapHolder::AddSubMapHolder(GxSubMapHolder *pNewSMH)
{
#ifdef LIBGX_TEST_BUILD
  assert(pNewSMH);
#endif //LIBGX_TEST_BUILD
  if(!pNewSMH) return;

  subMapList.push_back(pNewSMH);
}

void GxMapHolder::RemoveSubMapHolder(GxSubMapHolder *pOldSMH)
{
  list<GxSubMapHolder*>::iterator cPlace = subMapList.begin();
  list<GxSubMapHolder*>::iterator cEnd = subMapList.end();
  while(cPlace != cEnd)
    {
      if( *cPlace == pOldSMH)
	{
	  cPlace = subMapList.erase(cPlace);
	  return;
	};
      cPlace++;
    };
}

void GxMapHolder::ManageNewWin(GxCoreWin *pWin, Window winID)
{
  winMap.ManageWin(pWin, winID);
}

void GxMapHolder::UnManageWin(Window winID)
{
  winMap.RemoveWin(winID);
}

bool GxMapHolder::SendXEventLocally(const XEvent &rEvent)
{
  GxCoreWin *pWin = winMap.GetWin(rEvent.xany.window);
  if(!pWin)
    {
      //try all of my GxSubMapHolder children untill the event is handled
      list<GxSubMapHolder*>::iterator cPlace = subMapList.begin();
      list<GxSubMapHolder*>::iterator cEnd = subMapList.end();
      while(cPlace != cEnd)
	{
	  if( (*cPlace)->SendXEventLocally(rEvent) )
	    return true;
	  cPlace++;
	};
    }else
      {
	pWin->HandleEvent(rEvent);
	return true;
      };

  //only if the event has not yet been handled do we return false
  return false;
}

void GxMapHolder::SendUnusedEvent(GxSubMapHolder *pFrom, const XEvent &rEvent)
{
  GxCoreWin *pWin = winMap.GetWin(rEvent.xany.window);
  if(!pWin)
    {
      //try all of my GxSubMapHolder children (except pFrom) untill the event is handled
      list<GxSubMapHolder*>::iterator cPlace = subMapList.begin();
      list<GxSubMapHolder*>::iterator cEnd = subMapList.end();
      while(cPlace != cEnd)
	{
	  if( *cPlace != pFrom )
	    if( (*cPlace)->SendXEventLocally(rEvent) )
	      return;
	  cPlace++;
	};
    }else
      {
	pWin->HandleEvent(rEvent);
	return;
      };

  //if were here we have not handled the event; should not happen ?much?
  //with current libGx design.
}
