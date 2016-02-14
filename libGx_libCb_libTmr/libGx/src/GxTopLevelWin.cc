#include <vector>

#include <libGx/GxTopLevelWin.hh>

using namespace std;

GxTopLevelWin::GxTopLevelWin(GxOwner *pOwner) :
  GxFocusMaster(), GxCoreOwnerWin(pOwner)
{}

GxTopLevelWin::~GxTopLevelWin(void)
{}

void GxTopLevelWin::MoveFocusToChild(GxWinArea *pChild, Time eventTime)
{
  MoveFocusToObject(pChild,eventTime);
}

void GxTopLevelWin::Place(void)
{
  int lX = 0;
  int rX = (int)GetDesiredWidth();
  int tY = 0;
  int bY = (int)GetDesiredHeight();
  GxCoreOwnerWin::Place(lX,rX, tY, bY);
}

void GxTopLevelWin::Create(void)
{
  GxCoreOwnerWin::Create();

  //should not ever happen
  if(xWin == None) return; //hack set some error (?return false?)

  XTextProperty winName, iconName;
  XSizeHints *pSizeHints = XAllocSizeHints();
  XWMHints *pWMHints = XAllocWMHints();
  list<Atom> wmProtList;

  GetWMProperties(winName, iconName, *pSizeHints, *pWMHints, wmProtList);

  XSetWMName(dInfo.display, xWin, &winName);
  XSetWMIconName(dInfo.display, xWin, &iconName);
  XSetWMNormalHints(dInfo.display, xWin, pSizeHints);
  XSetWMHints(dInfo.display, xWin, pWMHints);

  if( !wmProtList.empty() )
    {
      vector<Atom> aVec( wmProtList.size() );
      unsigned cPlace = 0;
      while( !wmProtList.empty() )
	{
	  aVec[cPlace] = wmProtList.back();
	  cPlace++;
	  wmProtList.pop_back();
	};
      
      XSetWMProtocols(dInfo.display, xWin, &aVec[0], aVec.size() );
    };

  if(winName.value)
    XFree(winName.value);
  if(iconName.value)
    XFree(iconName.value);

  XFree(pSizeHints);
  XFree(pWMHints);

  //same for every application toplevel window
  XClassHint *pClassHint = XAllocClassHint();
  //hack: are the casts valid valid? what happens in XSetClassHint
  pClassHint->res_name = (char*)dInfo.rMainInterface.GetAppName();
  pClassHint->res_class = (char*)dInfo.rMainInterface.GetClassName();
  XSetClassHint(dInfo.display, xWin, pClassHint);
  XFree(pClassHint);
}

void GxTopLevelWin::SetTitle(const char *pTitle)
{
  if( !Created() ) return;

  XTextProperty winName;
  //hack: is the cast valid?
  XStringListToTextProperty( const_cast<char**>(&pTitle), 1, &winName);
  XSetWMName(dInfo.display, xWin, &winName);
  
  if(winName.value)
    XFree(winName.value);
}

void GxTopLevelWin::HandleEvent(const XEvent &rEvent)
{
  int finalWidth, finalHeight, finalX, finalY;

  Time eventTime;
  switch(rEvent.type)
    {
    case ClientMessage:
      if( rEvent.xclient.message_type != dInfo.wmProtocols ) break;
      if( rEvent.xclient.data.l[0] == dInfo.takeFocus )
	{
	  //eventTime = rEvent.xclient.data.l[1];
	  eventTime = CurrentTime; //MASSIVE HACK -> the above line is proper, but sometimes fails
#ifdef LIBGX_DEBUG_BUILD
	  std::cout << "format: " << std::dec << rEvent.xclient.format << std::endl;
	  std::cout << "focus client message in GxTopLevelWin::HandleEvent at time: "
		    << rEvent.xclient.data.l[1] << std::endl;
#endif //LIBGX_DEBUG_BUILD
	  RegainFocus(eventTime);
	  break;
	}else
	if( rEvent.xclient.data.l[0] == dInfo.deleteWindow )
	  {
	    DeleteWindow();
	  };

    case FocusIn:
      //some focus ins are irrelevant because they occur between my children
      //so should have better check than this
      //could we check for focus events between children so we can try
      //to keep GxFocusMaster pointing to the correct child if things don't
      //work as we expect?
      if(rEvent.xfocus.detail != NotifyInferior) break;
      
      //GrabModeSync for typeahead
      XGrabKey(dInfo.display, dInfo.travKeyCode, AnyModifier, xWin, false,
	       GrabModeAsync, GrabModeSync);
#ifdef LIBGX_DEBUG_BUILD
      std::cout << "got focus in and trying to grab traverse key for win: "
	   << xWin << std::endl;
#endif //LIBGX_DEBUG_BUILD
      //if(!haveFocus) //the ClientMessageAbove failed
      //RegainFocus(CurrentTime); //we cannot do this because we cannot use CurrentTime in XSetInputFocus
      break;

    case FocusOut:
      //some focus outs are irrelevant because they are between me and
      //one of my children or between two of my children
      if(rEvent.xfocus.detail == NotifyInferior)
	break;
      if(!haveFocus) break; //this event was irrelevant
      XUngrabKey(dInfo.display, dInfo.travKeyCode, AnyModifier, xWin);
      haveFocus = false;
#ifdef LIBGX_DEBUG_BUILD
      std::cout << "got focus out and ungrabed traverse key from win: "
	   << xWin << std::endl;
#endif //LIBGX_DEBUG_BUILD
      break;

    case KeyPress:
      //we have not seleced the keypress in our event mask, however we grab
      //the tab key to traverse our focus list so we do get keypress events
      if( rEvent.xkey.keycode == dInfo.travKeyCode)
	{
#ifdef LIBGX_DEBUG_BUILD
	  std::cout << "GxTopLevelWin::HandleEvent got traverse keypress" << std::endl;
#endif //LIBGX_DEBUG_BUILD
	  TransferFocus(rEvent.xkey.time);
	  XAllowEvents(dInfo.display, AsyncKeyboard, CurrentTime);
	};
      break;

    case ConfigureNotify:
      //the server already thinks our window is the new size, but
      //we have to update our client data. no need to call Object::Resize()
      //we can just reset the numbers; because we are actually already the
      //new size. We get this event when moved, etc. but we don't want to
      //PlaceChildren untill we have actually been resized

      finalWidth = rEvent.xconfigure.width;
      finalHeight = rEvent.xconfigure.height;
      finalX = rEvent.xconfigure.x;
      finalY = rEvent.xconfigure.y;
      //get only the final resize. resizing and repainting the entire widget tree
      //can be very expensive.
      while( true )
	{
	  XEvent event;
	  bool res = XCheckTypedWindowEvent(dInfo.display, xWin, ConfigureNotify, &event);
	  if(!res) break;

	  finalWidth = rEvent.xconfigure.width;
	  finalHeight = rEvent.xconfigure.height;
	  finalX = rEvent.xconfigure.x;
	  finalY = rEvent.xconfigure.y;
	};

      if(width != finalWidth || height != finalHeight)
	{
	  width = finalWidth;
	  height = finalHeight;
	  PlaceChildren();
	};

      if(x != rEvent.xconfigure.x)
	x = finalX;
      if(y != rEvent.xconfigure.y)
	y = finalY;
      break;

    default: //do nothing
      break;
    };
}

void GxTopLevelWin::DeleteWindow(void)
{}


Window GxTopLevelWin::GetParentWindow(void)
{
  return RootWindow(dInfo.display, dInfo.screenNum);
}

void GxTopLevelWin::GetWMProperties(XTextProperty &winName,
				    XTextProperty &iconName,
				    XSizeHints &rSizeHints,
				    XWMHints &rWMHints,
				    std::list<Atom> &rWMProtocolList)
{
  //hack: is the cast valid?
  char *pName = (char*)dInfo.rMainInterface.GetAppName();

  XStringListToTextProperty(&pName, 1, &winName);
  XStringListToTextProperty(&pName, 1, &iconName);

  rWMHints.flags = InputHint;
  rWMHints.input = true;

  rWMProtocolList.push_back( dInfo.takeFocus );
}

void GxTopLevelWin::GetWindowData(XSetWindowAttributes &winAttributes,
				ULINT &valueMask)
{
  winAttributes.cursor = dInfo.defaultCursor;
  winAttributes.event_mask = StructureNotifyMask | FocusChangeMask;
  valueMask |= CWEventMask | CWCursor;
}

