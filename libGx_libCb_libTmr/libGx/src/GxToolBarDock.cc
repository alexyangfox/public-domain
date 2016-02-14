#ifdef LIBGX_DEBUG_BUILD
#include <iostream>
#endif //LIBGX_DEBUG_BUILD

#include <string>

#include <libGx/GxToolBarManager.hh>
#include <libGx/GxToolBarDock.hh>

#include "GxDefines.hh"

using namespace std;

// ********************* start GxToolBarDock *****************************
GxToolBarDock::GxToolBarDock(GxRealOwner *pOwner, GxToolBarManager &rTManager, unsigned tDockID):
  GxOwnerWin(pOwner), GxToolBarCplxDockCore(this, rTManager), dockID(tDockID)

{
  rManager.AddToolBarDock(this);
}

GxToolBarDock::~GxToolBarDock(void)
{
  rManager.RemoveToolBarDock(this);
}

void GxToolBarDock::AddToolBar(GxToolBar *pToolBar)
{
  if(!pToolBar) return;

  ToolBarHolder &rHolder = AddBar(*pToolBar);

#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarDock::AddToolBar" << endl;
#endif //LIBGX_DEBUG_BUILD

  if( Created() )
    {
      //look at constructor, this is a safe cast
      ((GxRealOwner*)pWinAreaOwner)->PlaceChildren();
      rHolder.CreateAndDisplayButtonWindows();
      XClearWindow(dInfo.display, xWin);
      DrawDock();
    };
}

void GxToolBarDock::RemoveToolBar(GxToolBar *pToolBar)
{
  //we are not responsible for deleting the toolbar. (it probably will not be
  //deleted after it is removed)
  if( RemoveBar(pToolBar) )
    {
      if( Created() )
	{
	  ((GxRealOwner*)pWinAreaOwner)->PlaceChildren();
	  XClearWindow(dInfo.display, xWin);
	  DrawDock();
	};
    }
}

unsigned GxToolBarDock::GetDockID(void)
{
  return dockID;
}

void GxToolBarDock::SetVertical(bool newState)
{
  vertical = newState;
}

Window GxToolBarDock::GetDockBaseWindow(void) const
{
  return xWin;
}

UINT GxToolBarDock::GetDesiredWidth(void) const
{
  UINT desWidth, desHeight;
  GetDesiredDockSize(desWidth, desHeight);
  desWidth += 2*GX_BORDER_WD;
  desHeight += 2*GX_BORDER_WD;
  
  if(vertical)
    return desHeight;
  else
    return desWidth;
}

UINT GxToolBarDock::GetDesiredHeight(void) const
{
  /* we cannot do this as we might have a phantom toolbar
  if( tbList.empty() )
    return 2*GX_BORDER_WD+GX_TOOLBAR_GAP;
  */

  UINT desWidth, desHeight;
  GetDesiredDockSize(desWidth, desHeight);
  desWidth += 2*GX_BORDER_WD;
  desHeight += 2*GX_BORDER_WD;

  if(vertical)
    return desWidth;
  else
    return desHeight;
}

void GxToolBarDock::Place(int &lX, int &rX, int &tY, int &bY)
{
  GxOwnerWin::Place(lX, rX, tY, bY);
}

void GxToolBarDock::PlaceChildren(void)
{
  PlaceDockElements(GX_BORDER_WD, GX_BORDER_WD);
}

void GxToolBarDock::Create(void)
{
  GxOwnerWin::Create();

  XGrabButton(dInfo.display, 2, AnyModifier, xWin, false, (ButtonPressMask | ButtonReleaseMask | OwnerGrabButtonMask),
	      GrabModeAsync, GrabModeAsync, None, None);

  XGrabButton(dInfo.display, 3, AnyModifier, xWin, false, (ButtonPressMask | ButtonReleaseMask | OwnerGrabButtonMask),
	      GrabModeAsync, GrabModeAsync, None, None);
}

void GxToolBarDock::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      DrawDock();

  if(rEvent.type == ButtonPress)
    {
      if(rEvent.xbutton.button == 2)
	{
	  //cout << "got move button press" << endl;
	  UINT rowCol, num;
	  GxToolBar *pToolBar = SelectToolBar(GX_BORDER_WD, GX_BORDER_WD, rEvent.xbutton.x, rEvent.xbutton.y);
	  if(!pToolBar) return;
	  rManager.MoveToolBar(pToolBar, NULL, this);
	}else
	  if(rEvent.xbutton.button == 3) //hack. duplicate from GxToolBarFloatingDock
	    {
	      //cout << "got menu button press" << endl;
	      //we will map at the press coordinates.
	      int xStart = rEvent.xbutton.x;
	      int yStart = rEvent.xbutton.y;
	      int xRoot, yRoot;
	      Window childWin;
	      XTranslateCoordinates(dInfo.display, xWin, dInfo.rootWin, xStart, yStart,
				    &xRoot, &yRoot, &childWin);
	      GxToolBar *pTBar = SelectToolBar(GX_BORDER_WD, GX_BORDER_WD, xStart, yStart);
	      if(!pTBar) return;
	      DoToolBarMenu(xRoot,yRoot, pTBar);
	    };
    };
}

void GxToolBarDock::GetWindowData(XSetWindowAttributes &winAttrib, ULINT &valueMask)
{
  winAttrib.event_mask = ButtonPressMask | ButtonReleaseMask | ExposureMask
    | LeaveWindowMask | EnterWindowMask;

  valueMask |= CWEventMask;
}

void GxToolBarDock::ResizeAndPlaceDock(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarDock::ResizeAndPlaceDock" << endl;
#endif //LIBGX_DEBUG_BUILD
  //safe cast. look at constructor.
  ((GxRealOwner*)pWinAreaOwner)->PlaceChildren();
}

void GxToolBarDock::EraseDock(void)
{
  if( !Created() ) return;
  XClearArea(dInfo.display, xWin, GX_BORDER_WD, GX_BORDER_WD, width-2*GX_BORDER_WD, height-2*GX_BORDER_WD, false);
}

void GxToolBarDock::DrawDock(void)
{
  if( !Created() ) return;

  Draw3dBorder(0,0, width, height, true);
  GxToolBarCplxDockCore::DrawDock(dInfo, vData, xWin, GX_BORDER_WD, GX_BORDER_WD);
}

GxRealOwner& GxToolBarDock::GetToolBarWinOwner(void)
{
  return *this;
}
