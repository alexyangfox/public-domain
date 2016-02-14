#include <libGx/GxDisplay.hh>

#include <libGx/GxListTree.hh>

#include "GxDefines.hh"

GxListTree::GxListTree(GxRealOwner *pOwner) :
  GxAppScrolledWin(pOwner), listPane(this)
{
  SetClipWindow(&listPane);
}

GxListTree::~GxListTree(void)
{}

void GxListTree::DrawList(void) const
{
  if( !listPane.Created() ) return;
}

void GxListTree::SelectPress(int yClick)
{

}

void GxListTree::SelectRelease(int yClick)
{

}

void GxListTree::SelectMotion(int yMotion)
{

}

// ************************ 

GxListTree::ListPane::ListPane(GxListTree *pOwner) :
  GxWin(pOwner), pushedHandlerID(NULL_EVENT_HANDLER_ID)
{}

GxListTree::ListPane::~ListPane(void)
{}

void GxListTree::ListPane::Clear(void)
{
  if( !Created() ) return;

  XClearArea(dInfo.display, xWin, GX_BORDER_WD,GX_BORDER_WD,
	     width-(GX_BORDER_WD*2), height-(GX_BORDER_WD*2), false);
}

void GxListTree::ListPane::HandleEvent(const XEvent &rEvent)
{
  switch(rEvent.type)
    {
    case Expose:
      if(rEvent.xexpose.count == 0)
	((GxListTree*)pWinAreaOwner)->DrawList();
      break;

    case ButtonPress:
      //grab events until we get the matching button release
      if(rEvent.xbutton.button == 1)
	{
	  //the server is grabed.
	  ((GxListTree*)pWinAreaOwner)->SelectPress(rEvent.xbutton.y);
	    dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxListTree::ListPane>
					       (this, &GxListTree::ListPane::HandleGrabbedEvents),
					       pushedHandlerID );
	};
      break;

    default:
      //do nothing
      break;
    };
}

void GxListTree::ListPane::HandleGrabbedEvents(const XEvent &rEvent)
{
  if(rEvent.type == MotionNotify && rEvent.xmotion.window == xWin)
    {
      //get the _final_ xmotion event
      XEvent finalMotion = rEvent;
      while(true)
	{
	  XEvent newEvent;
	  if( XCheckTypedWindowEvent(dInfo.display, xWin, MotionNotify, &newEvent) )
	    finalMotion = newEvent;
	  else
	    break;
	};

      //select Items in range scrolling up & down as necessary
      //but only if we are that kind of list
      ((GxListTree*)pWinAreaOwner)->SelectMotion(finalMotion.xmotion.y);
    }else
    if(rEvent.type == ButtonRelease && rEvent.xbutton.button == 1 && rEvent.xbutton.window == xWin)
      {
	//if the button release was in the same item; return.
	((GxListTree*)pWinAreaOwner)->SelectRelease(rEvent.xbutton.y);
	dInfo.rGxDisplay.RemoveEventHandler(pushedHandlerID);
	pushedHandlerID = NULL_EVENT_HANDLER_ID;
      }else
      if(rEvent.type == Expose )
	{
	  dInfo.rGxDisplay.HandleSafeLoopEvent(rEvent);
	};
}

void GxListTree::ListPane::GetWindowData(XSetWindowAttributes &winAttributes,
					 ULINT &valueMask)
{
  winAttributes.background_pixel = dInfo.whitePix;
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask | Button1MotionMask;
  valueMask |= CWEventMask;
}
