#include <libGx/GxDisplay.hh>

#include <libGx/GxFocusButtonBase.hh>

GxFocusButtonBase::GxFocusButtonBase(GxRealOwner *pOwner) :
  GxWin(pOwner), haveFocus(false), active(true), pressed(false),
  pushedHandlerID(NULL_EVENT_HANDLER_ID)
{}

GxFocusButtonBase::~GxFocusButtonBase(void)
{}

void GxFocusButtonBase::SetActive(bool nActive)
{
  if(nActive != active)
    {
      active = nActive;
      if( Created() )
	{
	  XClearWindow(dInfo.display, xWin);
	  DrawButton();
	};
    };
}

bool GxFocusButtonBase::AcceptFocus(Time eventTime)
{
#ifdef LIBGX_DEBUG_BUILD
  std::cout << "GxFocusButtonBase::AcceptFocus" << std::endl;
#endif //LIBGX_DEBUG_BUILD

  if(!Created())
    return false;
  else
    {
      //hack according to icccm; CurrentTime is invalid, however it is
      //just fine according to XSetInputFocus manpage
      XSetInputFocus(dInfo.display, xWin, RevertToParent, eventTime);
      return true;
    };
}

void GxFocusButtonBase::GetWindowData(XSetWindowAttributes &winAttributes,
				 ULINT &valueMask)
{
  winAttributes.event_mask = EnterWindowMask | LeaveWindowMask |
    ButtonPressMask | ButtonReleaseMask | ExposureMask | KeyPressMask |
    FocusChangeMask;

  valueMask |= CWEventMask;
}

void GxFocusButtonBase::HandleEvent(const XEvent &rEvent)
{
  switch(rEvent.type)
    {
    case ButtonPress:
      //check if it was the correct button
      if(rEvent.xbutton.button != 1)
	return; //the incorrect button so just return
      else
	break; //it was the correct button and we need to continue below

    case ButtonRelease:
      return;

    case Expose:
      if(rEvent.xexpose.count == 0)
	DrawButton();
      return;

    case FocusIn:
      if(!haveFocus)
	{
	  haveFocus = true;
	  DrawButton();
	};
      return;
    case FocusOut:
      if(haveFocus)
	{
	  XClearWindow(dInfo.display, xWin);
	  haveFocus = false;
	  DrawButton();
	};
      return;

    case KeyPress:
      //hack: I am assuming the cast is safe.
      if( (XLookupKeysym( (XKeyEvent*)(&rEvent.xkey), 0) == XK_Return) && haveFocus)
	DoAction();
      return;      
      //we don't care about these unless we are in our event loop
    case LeaveNotify:
    case EnterNotify:
    default:
      return;
    };

  if(!active) return;

  //if we are here; the event must have been a ButtonPress on the correct
  //button and now we will start our own private event loop untill the pointer
  //button is released. If the pointer moves out of the window after it
  //was pressed we redraw the button raised. If it is released with the
  //pointer out of the window, we do not call the button callback. If we
  //reenter the window with the button still pressed, we redraw the button
  //down. This heavily relies on the implicit grab done by the X-server but
  //relying on this behavior is normal.

  pressed = true;
  XClearWindow(dInfo.display, xWin);
  DrawButton();

  dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxFocusButtonBase>
				     (this, &GxFocusButtonBase::HandleGrabbedEvents),
				      pushedHandlerID );
}

void GxFocusButtonBase::HandleGrabbedEvents(const XEvent &rEvent)
{
  switch(rEvent.type)
    {
    case Expose:
      dInfo.rGxDisplay.HandleSafeLoopEvent(rEvent);
      break;
    case LeaveNotify:
      if(rEvent.xcrossing.window != xWin ) break;
      pressed = false;
      XClearWindow(dInfo.display, xWin);
      DrawButton();
      break;
    case EnterNotify:
      if(rEvent.xcrossing.window != xWin ) break;
      pressed = true;
      XClearWindow(dInfo.display, xWin);
      DrawButton();
      break;
    case ButtonRelease:
      if(rEvent.xbutton.button != 1) break;
      //if we are here the correct button was released
      //no matter what else we remove this event handler
      dInfo.rGxDisplay.RemoveEventHandler(pushedHandlerID);
      pushedHandlerID = NULL_EVENT_HANDLER_ID;
      if(pressed)
	{
	  pressed = false;
	  DoAction();
	  return;
	};
    default: 
      break;
    };
}
