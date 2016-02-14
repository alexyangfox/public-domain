#include <libGx/GxDisplay.hh>

#include <libGx/GxNewList.hh>

#include "GxDefines.hh"

//******************** start GxNewListItem ***************************
GxNewListItem::GxNewListItem(void) :
  selected(false)
{}

GxNewListItem::~GxNewListItem(void)
{}

//******************** end GxNewListItem ***************************

GxNewListContents::GxNewListContents(void)
{}

GxNewListContents::~GxNewListContents(void)
{}

void GxNewListContents::DrawList(void)
{}

//***************** start GxNewList ****************************

GxNewList::GxNewList(GxRealOwner *pOwner) :
  GxAppScrolledWin(pOwner)
{}

GxNewList::~GxNewList(void)
{}

void GxNewList::SelectPress(int yPlace)
{}

void GxNewList::SelectMotion(int yPlace)
{}

void GxNewList::SelectRelease(int yPlace)
{}


// ****************** end GxNewList *******************************

// ********************* start ListWin ******************************
GxNewList::ListWin::ListWin(GxNewList *pOwner) :
  GxWin(pOwner), pushedHandlerID(NULL_EVENT_HANDLER_ID)
{}

GxNewList::ListWin::~ListWin(void)
{}

void GxNewList::ListWin::Clear(void)
{
  if( Created() )
    XClearArea(dInfo.display, xWin, GX_BORDER_WD,GX_BORDER_WD,
	       width-(GX_BORDER_WD*2), height-(GX_BORDER_WD*2), false);
}

void GxNewList::ListWin::HandleEvent(const XEvent &rEvent)
{
  switch(rEvent.type)
    {
    case Expose:
      if(rEvent.xexpose.count == 0)
	((GxNewList*)pWinAreaOwner)->DrawList();
      break;

    case ButtonPress:
      //grab events until we get the matching button release
      if(rEvent.xbutton.button == 1)
	{
	  //the server is grabed.
	  ((GxNewList*)pWinAreaOwner)->SelectPress(rEvent.xbutton.y);
	    dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxNewList::ListWin>
					       (this, &GxNewList::ListWin::HandleGrabbedEvents),
					       pushedHandlerID );
	};
      break;

    default:
      //do nothing
      break;
    };
}

void GxNewList::ListWin::HandleGrabbedEvents(const XEvent &rEvent)
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
      ((GxNewList*)pWinAreaOwner)->SelectMotion(finalMotion.xmotion.y);
    }else
    if(rEvent.type == ButtonRelease && rEvent.xbutton.button == 1 && rEvent.xbutton.window == xWin)
      {
	//if the button release was in the same item; return.
	((GxNewList*)pWinAreaOwner)->SelectRelease(rEvent.xbutton.y);
	dInfo.rGxDisplay.RemoveEventHandler(pushedHandlerID);
	pushedHandlerID = NULL_EVENT_HANDLER_ID;
      }else
      if(rEvent.type == Expose )
	{
	  dInfo.rGxDisplay.HandleSafeLoopEvent(rEvent);
	};
}

void GxNewList::ListWin::GetWindowData(XSetWindowAttributes &winAttributes,
				       ULINT &valueMask)
{
  winAttributes.background_pixel = dInfo.whitePix;
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask | Button1MotionMask;
  valueMask |= CWEventMask;
}

// ********************* end ListWin ******************************
