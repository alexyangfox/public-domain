#include <libGx/GxSubMapHolder.hh>

#include <libGx/GxCoreWin.hh>

GxSubMapHolder::GxSubMapHolder(GxMapHolder *ptPrevHolder) :
  GxMapHolder(), pPrevHolder(ptPrevHolder)
{
  pPrevHolder->AddSubMapHolder(this);
}

GxSubMapHolder::~GxSubMapHolder(void)
{
  pPrevHolder->RemoveSubMapHolder(this);
  pPrevHolder = 0;
}

void GxSubMapHolder::SendXEventUp(const XEvent &rEvent)
{
  pPrevHolder->SendUnusedEvent(this, rEvent);  
}

void GxSubMapHolder::SendUnusedEvent(GxSubMapHolder *pFrom,
				     const XEvent &rEvent)
{
  GxCoreWin *pWin = winMap.GetWin(rEvent.xany.window);
  if(!pWin)
    {
      //try all of my GxSubMapHolder children _except_ pFrom untill the event is handled
      std::list<GxSubMapHolder*>::iterator cPlace = subMapList.begin();
      std::list<GxSubMapHolder*>::iterator cEnd = subMapList.end();
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

  pPrevHolder->SendUnusedEvent(this, rEvent);
}
