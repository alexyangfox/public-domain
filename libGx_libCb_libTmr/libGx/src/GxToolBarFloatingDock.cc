#include <libGx/GxToolBarFloatingDock.hh>

#include <libGx/GxToolBarManager.hh>

#include "GxDefines.hh"

using namespace std;

GxToolBarFloatingDock::GxToolBarFloatingDock(GxDisplay *pOwner, GxToolBarManager &rTManager) :
  GxTopLevelWin(pOwner), GxToolBarCplxDockCore(this, rTManager)
{}

GxToolBarFloatingDock::~GxToolBarFloatingDock(void)
{}

void GxToolBarFloatingDock::AddToolBar(GxToolBar *pNewToolBar)
{
  if(!pNewToolBar) return;

  ToolBarHolder &rHolder = AddBar( *pNewToolBar );

  if( Created() ) //hack. wrong order
    {
      Place();
      rHolder.CreateAndDisplayButtonWindows();
    };
}

void GxToolBarFloatingDock::RemoveToolBar(GxToolBar *pToolBar)
{
  if(!pToolBar) return;

  if( RemoveBar(pToolBar) )
    {
      Place();
      XClearWindow(dInfo.display, xWin);
      DrawDock();
    };
}

bool GxToolBarFloatingDock::DockUsed(void)
{
  return !rowList.empty(); /* hack? there better not be a single phantom in there */
}

void GxToolBarFloatingDock::GetGeom(int &rXRoot, int &rYRoot, UINT &rNumButtonsRow)
{
  rNumButtonsRow = (unsigned int)( (width-2*GX_TOOLBAR_GAP)/GX_TOOLBAR_BUTTON_SIZE );

  if( !Created() )
    {
      rXRoot = x;
      rYRoot = y;
      return;
    };

#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarFloatingDock::GetGeom" << endl;
#endif //LIBGX_DEBUG_BUILD
  Window unusedChild;
  XTranslateCoordinates(dInfo.display, xWin, dInfo.rootWin, 0,0, &rXRoot,
			&rYRoot, &unusedChild);
}

Window GxToolBarFloatingDock::GetDockBaseWindow(void) const
{
  return xWin;
}

void GxToolBarFloatingDock::Place(void)
{
  if( rowList.empty() ) return;

  UINT nWidth, nHeight;
  GetDesiredDockSize(nWidth, nHeight);
  nWidth += 2*GX_BORDER_WD;
  nHeight += 2*GX_BORDER_WD;

  if(vertical)
    {
      UINT temp = nWidth;
      nWidth = nHeight;
      nHeight = temp;
    };

  Resize(nWidth, nHeight);
  PlaceDockElements(GX_BORDER_WD, GX_BORDER_WD);
}

void GxToolBarFloatingDock::Create(void)
{
  GxTopLevelWin::Create();
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarFloatingDock::Create grabbing buttons" << endl;
#endif //LIBGX_DEBUG_BUILD

  XGrabButton(dInfo.display, 2, AnyModifier, xWin, false,
	      (ButtonPressMask | ButtonReleaseMask | OwnerGrabButtonMask),
	      GrabModeAsync, GrabModeAsync, None, None);

  XGrabButton(dInfo.display, 3, AnyModifier, xWin, false,
	      (ButtonPressMask | ButtonReleaseMask | OwnerGrabButtonMask),
	      GrabModeAsync, GrabModeAsync, None, None);
}

void GxToolBarFloatingDock::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	DrawDock();
	return;
      };

  if(rEvent.type == ClientMessage)
    {
      if( rEvent.xclient.message_type != dInfo.wmProtocols ) return;
      if( rEvent.xclient.data.l[0] == dInfo.deleteWindow )
	{
	  SetAllToolBarDisplayState(false);
	  rManager.DeleteToolBarFloatingDock(this); //effectivly a delete this
	  return; //don't touch anything local!
	};
    };
  
  if(rEvent.type == ButtonPress)
    if(rEvent.xbutton.button == 2)
      {
#ifdef LIBGX_DEBUG_BUILD
	cout << "GxToolBarFloatingDock::HandleEvent got button press. window: " << xWin << endl;
#endif //LIBGX_DEBUG_BUILD
	//this might result in this objects distruction so we must return
	//immediatly after we make the call.  This is simular to the delima
	//present in a button when it is used as a quit button
	GxToolBar* pToolBar = SelectToolBar(GX_BORDER_WD, GX_BORDER_WD, rEvent.xbutton.x, rEvent.xbutton.y);
	if(pToolBar)
	  rManager.MoveToolBar(pToolBar, this, NULL);
	return;
      }else
	if(rEvent.xbutton.button == 3) //hack. duplicate from GxToolBarDock
	    {
#ifdef LIBGX_DEBUG_BUILD
	      cout << "GxToolBarFloatingDock::HandleEvent got menu button press" << endl;
#endif //LIBGX_DEBUG_BUILD
	      //we will map at the press coordinates.
	      int xStart = rEvent.xbutton.x;
	      int yStart = rEvent.xbutton.y;
	      int xRoot, yRoot;
	      Window childWin;
	      XTranslateCoordinates(dInfo.display, xWin, dInfo.rootWin, xStart, yStart,
				    &xRoot, &yRoot, &childWin);
	      GxToolBar *pTBar = SelectToolBar(GX_BORDER_WD, GX_BORDER_WD, xStart, yStart);
	      if(!pTBar)
		{
#ifdef LIBGX_DEBUG_BUILD
		  cout << "GxToolBarFloatingDock::HandleEvent no menu selected" << endl;
#endif //LIBGX_DEBUG_BUILD
		  return;
		};
	      //this will enter its own event loop
	      DoToolBarMenu(xRoot,yRoot, pTBar);
	    };
}

void GxToolBarFloatingDock::DeleteWindow(void)
{

}

void GxToolBarFloatingDock::GetWMProperties(XTextProperty &winName,
					    XTextProperty &iconName,
					    XSizeHints &rSizeHints,
					    XWMHints &rWMHints,
					    std::list<Atom> &rWMProtocolList)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarFloatingDock::GetWMProperties" << endl;
#endif //LIBGX_DEBUG_BUILD
  //we don't need keyboard input for this window
  rWMHints.input = false;
  rWMHints.window_group = rManager.GetGroupWindow();
  rWMHints.flags = InputHint;
 
  if(rWMHints.window_group != None)
    {
#ifdef LIBGX_DEBUG_BUILD
      cout << "setting window group" << endl;
#endif //LIBGX_DEBUG_BUILD
      rWMHints.flags |= WindowGroupHint;
    };

  //we will have our own name.
  //hack: I'm assuming the cast is safe.
  char *pName = (char*)dInfo.rMainInterface.GetAppName();

  XStringListToTextProperty(&pName, 1, &winName);
  XStringListToTextProperty(&pName, 1, &iconName);

  //hack this is broken.
  GxToolBar* pToolBar = 0;

  if(pToolBar)
    {
      GX_TOOLBAR_LOCATION loc = GX_TOOLBAR_ROOT_WIN;
      int xP, yP;
      UINT unused;
      //pToolBar->GetPrevPosition(loc, xP, yP, unused);

      if(loc == GX_TOOLBAR_ROOT_WIN && !(xP == 0 && yP == 0) )
	{
	  rSizeHints.flags = PPosition | PSize;
#ifdef LIBGX_DEBUG_BUILD
	  cout << "allowing program program specified place" << endl;
#endif //LIBGX_DEBUG_BUILD
	  Move(xP,yP);
	}else
	  {
#ifdef LIBGX_DEBUG_BUILD
	    cout << "forced to autoplace" << endl;
#endif //LIBGX_DEBUG_BUILD
	    //Resize(250,250);
	    //rSizeHints.flags |= PPosition | PSize;
	    rSizeHints.flags = 0;
	    //hack; why dosen't this work?
	    //rSizeHints.flags = USPosition;// | USSize;
	  };
    }else //should not happen
      {
	rSizeHints.flags = USPosition | PSize;
      };

  rWMProtocolList.push_back( dInfo.deleteWindow );
}

void GxToolBarFloatingDock::GetWindowData(XSetWindowAttributes &winAttrib, ULINT &valueMask)
{
  GxTopLevelWin::GetWindowData(winAttrib, valueMask);
  //we need the button release mask is needed so that when the toolbar manager
  //does a pointer grab, the release event is reported with respect to the dock
  //the cursor is over, rather that the toolbar dock that started the move.
  winAttrib.event_mask |= ExposureMask | ButtonPressMask | ButtonReleaseMask | LeaveWindowMask | EnterWindowMask;
  valueMask |= CWEventMask;
}

void GxToolBarFloatingDock::ResizeAndPlaceDock(void)
{
  Place();
}

void GxToolBarFloatingDock::EraseDock(void)
{
  if( !Created() ) return;
  XClearArea(dInfo.display, xWin, GX_BORDER_WD, GX_BORDER_WD, width-2*GX_BORDER_WD, height-2*GX_BORDER_WD, false);
}

void GxToolBarFloatingDock::DrawDock(void)
{
  if( !Created() ) return;

  Draw3dBorder(0,0, width, height, true);
  GxToolBarCplxDockCore::DrawDock(dInfo, vData, xWin, GX_BORDER_WD, GX_BORDER_WD);
}

GxRealOwner& GxToolBarFloatingDock::GetToolBarWinOwner(void)
{
  return *this;
}
