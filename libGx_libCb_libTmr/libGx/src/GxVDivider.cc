#include <libGx/GxDisplay.hh>

#include <libGx/GxVDivider.hh>

GxVDivider::GxVDivider(GxRealOwner *pOwner) :
  GxWin(pOwner), pushedHandlerID(NULL_EVENT_HANDLER_ID)
{
  width = 4;
}

GxVDivider::~GxVDivider(void)
{}

UINT GxVDivider::GetDesiredWidth(void) const
{
  return 4;
}

void GxVDivider::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      Draw();

  if(rEvent.type == ButtonPress)
   if(rEvent.xbutton.button == 1)
      {
	relX = rEvent.xbutton.x;
	//the server is grabed. now process motion events untill button release
	dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxVDivider>
					   (this, &GxVDivider::HandleGrabbedEvents),
					   pushedHandlerID );
      };
}

void GxVDivider::HandleGrabbedEvents(const XEvent &rEvent)
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

void GxVDivider::Draw(void)
{
  if( !Created() ) return;

  XSetForeground(dInfo.display, vData.borderGC, dInfo.lightBorderPix);
  XDrawLine(dInfo.display, xWin, vData.borderGC, 1, 2, 1, height-2);

  XSetForeground(dInfo.display, vData.borderGC, dInfo.darkBorderPix);
  XDrawLine(dInfo.display, xWin, vData.borderGC, 2, 2, 2, height-2);
}

void GxVDivider::GetWindowData(XSetWindowAttributes &winAttrib, ULINT &valueMask)
{
  winAttrib.event_mask = ExposureMask | Button1MotionMask |
    ButtonPressMask | ButtonReleaseMask;

  winAttrib.cursor = dInfo.vDividerCursor;

  valueMask |= CWEventMask | CWCursor;
}
