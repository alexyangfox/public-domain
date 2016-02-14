#include <libGx/GxDisplay.hh>

#include <libGx/GxSlideGrip.hh>

GxSlideGrip::GxSlideGrip(GxRealOwner *pOwner, const CbOneBase<int> &rCB) :
  GxWin(pOwner), active(true)
{
  cb.Assign(rCB);
}

GxSlideGrip::~GxSlideGrip(void)
{}

bool GxSlideGrip::Active(void) const
{
  return active;
}

void GxSlideGrip::Active(bool newState)
{
  active = newState;
}

void GxSlideGrip::DrawSlideGrip(void)
{
  //hackish. what can we do with active?
  Draw3dBorder(0,0, width,height, true);
}

void GxSlideGrip::GetWindowData(XSetWindowAttributes &winAttributes,
			     ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask | Button1MotionMask |
    ButtonPressMask | ButtonReleaseMask;
  valueMask |= CWEventMask;
}

// ************************** start GxVSlideGrip **********************

GxVSlideGrip::GxVSlideGrip(GxRealOwner *pOwner, const CbOneBase<int> &rCB) :
  GxSlideGrip(pOwner, rCB), pushedHandlerID(NULL_EVENT_HANDLER_ID)
{}

GxVSlideGrip::~GxVSlideGrip(void)
{}

void GxVSlideGrip::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      DrawSlideGrip();

  if(!active) return;

  if(rEvent.type == ButtonPress)
    if(rEvent.xbutton.button == 1)
      {
	relY = rEvent.xbutton.y;
	//the server is grabed. now process motion events untill button release
	dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxVSlideGrip>
					   (this, &GxVSlideGrip::HandleGrabbedEvents),
					   pushedHandlerID );
      };
}

void GxVSlideGrip::HandleGrabbedEvents(const XEvent &rEvent)
{
   if(rEvent.type == MotionNotify && rEvent.xmotion.window == xWin )
    {
      int dist = rEvent.xmotion.y - relY;
      //be absolutly sure we are looking at the _final_ available xmotion event for this window
      while( true )
	{
	  XEvent event;
	  bool res = XCheckTypedWindowEvent(dInfo.display, xWin, MotionNotify, &event);
	  if(!res) break;

	  dist = event.xmotion.y - relY;
	};
      cb(dist);
    }else
    if(rEvent.type == ButtonRelease && rEvent.xbutton.button == 1)
      {
	dInfo.rGxDisplay.RemoveEventHandler(pushedHandlerID);
	pushedHandlerID = NULL_EVENT_HANDLER_ID;
      }else
      if(rEvent.type == Expose )
	{
	  dInfo.rGxDisplay.HandleSafeLoopEvent(rEvent);
	};
}

// ************************** start GxHSlideGrip **********************

GxHSlideGrip::GxHSlideGrip(GxRealOwner *pOwner, const CbOneBase<int> &rCB) :
  GxSlideGrip(pOwner, rCB)
{}

GxHSlideGrip::~GxHSlideGrip(void)
{}

void GxHSlideGrip::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      DrawSlideGrip();

  if(!active) return;

  if(rEvent.type == ButtonPress)
   if(rEvent.xbutton.button == 1)
      {
	relX = rEvent.xbutton.x;
	//the server is grabed. now process motion events untill button release
	dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxHSlideGrip>
					   (this, &GxHSlideGrip::HandleGrabbedEvents),
					   pushedHandlerID );
      };
}

void GxHSlideGrip::HandleGrabbedEvents(const XEvent &rEvent)
{
   if(rEvent.type == MotionNotify && rEvent.xmotion.window == xWin)
    {
      int dist = rEvent.xmotion.x - relX;

      //be absolutly sure we are looking at the _final_ available xmotion event for this window
      while( true )
	{
	  XEvent event;
	  bool res = XCheckTypedWindowEvent(dInfo.display, xWin, MotionNotify, &event);
	  if(!res) break;

	  dist = event.xmotion.x - relX;
	};

      cb(dist);
    }else
    if(rEvent.type == ButtonRelease && rEvent.xbutton.button == 1)
      {
	dInfo.rGxDisplay.RemoveEventHandler(pushedHandlerID);
	pushedHandlerID = NULL_EVENT_HANDLER_ID;
      }else
      if(rEvent.type == Expose )
	{
	  dInfo.rGxDisplay.HandleSafeLoopEvent(rEvent);
	};
}
