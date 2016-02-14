#include <libGx/GxDisplay.hh>
#include <libGx/GxPopupMenu.hh>

#include "GxDefines.hh"

using namespace std;

GxPopupMenu::GxPopupMenu(GxRealOwner *pOwner) :
  GxRootTransient(pOwner),  GxSubMapHolder( pOwner->GetClosestMapHolder() ), 
  buttonHeldMode(false), eventHandlerID(NULL_EVENT_HANDLER_ID)

{}

GxPopupMenu::~GxPopupMenu(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxPopupMenu::~GxPopupMenu" << endl;
#endif //LIBGX_DEBUG_BUILD
}

void GxPopupMenu::Place(void)
{
  int lX = 0;
  int rX = (int)GetDesiredWidth();
  int tY = 0;
  int bY = (int)GetDesiredHeight();
  GxCoreOwnerWin::Place(lX,rX, tY, bY);
}

void GxPopupMenu::Activate(int xRoot, int yRoot)
{
  //Resize(100, 100);
  Display(xRoot, yRoot);
  StartMenuEventGrab();
}

void GxPopupMenu::DeActivate(void)
{
  XUnmapWindow(dInfo.display, xWin);
  Hide();
}

UINT GxPopupMenu::GetDesiredWidth(void) const
{
  return 100;
}

UINT GxPopupMenu::GetDesiredHeight(void) const
{
  return 100;
}

//hack. this should be combined with GxMenuPane
void GxPopupMenu::PlaceChildren(void)
{
  UINT totalHeight = GX_BORDER_WD; //the 2 is to consider my border
  UINT maxMinWidth = 0;

  list<GxWinArea*>::iterator cPlace = childList.begin();
  list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->Move(GX_BORDER_WD,totalHeight);
      totalHeight += (*cPlace)->Height();
      UINT cWidth = ((GxMenuItem*)(*cPlace))->GetMinimumWidth();
      if(cWidth > maxMinWidth)
	maxMinWidth = cWidth;

      cPlace++;
    };

  //the 4 is taking in consideration my border
  //the 2 is also when combined with totalHeight's inital value
  Resize(maxMinWidth + 2*GX_BORDER_WD, totalHeight + GX_BORDER_WD);

  //we now go back and set the GxMenuItems' width
  cPlace = childList.begin();
  while(cPlace != cEnd)
    {
      (*cPlace)->Width(maxMinWidth);
      cPlace++;
    };
}

bool GxPopupMenu::GetButtonHeldMode(void) const
{
  return buttonHeldMode;
}

void GxPopupMenu::SetButtonHeldMode(bool newMode)
{
  buttonHeldMode = newMode;
}

void GxPopupMenu::EndMenuEventGrab(void)
{
  dInfo.rGxDisplay.RemoveEventHandler( eventHandlerID );
  eventHandlerID = NULL_EVENT_HANDLER_ID;

  DeActivate();
  XUngrabPointer(dInfo.display, CurrentTime);

  XSync(dInfo.display, false);
}

GxRealOwner & GxPopupMenu::GetRealOwnerObject(void)
{
  return *this;
}

GxMapHolder* GxPopupMenu::GetClosestMapHolder(void)
{
  return this;
}

void GxPopupMenu::UnManageWindow(Window winID)
{
  UnManageWin(winID);
}

void GxPopupMenu::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      Draw();
}

void GxPopupMenu::GetWindowData(XSetWindowAttributes &winAttributes,
				ULINT &valueMask)
{
  GxRootTransient::GetWindowData(winAttributes, valueMask);
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

void GxPopupMenu::StartMenuEventGrab(void)
{
  XGrabPointer(dInfo.display, xWin, true, ButtonReleaseMask,
	       GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

  dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxPopupMenu>(this, &GxPopupMenu::HandleGrabbedEvents ),
				     eventHandlerID );
}

void GxPopupMenu::HandleGrabbedEvents(const XEvent &rEvent)
{
  if( !SendXEventLocally(rEvent) )
    {
      //pass Expose events to rest of application while we
      //are in the local event loop. Because we use save_under
      //this should rarely happen
      //hack; perhaps we sould send other kinds of events up too.
      switch(rEvent.type)
	{
	case Expose:
	  SendXEventUp(rEvent);
	  break;
	case ButtonRelease:
	  //this only happens if the button was released outside the
	  //application or in a window not in the menu heiarchy
	  //hack. check for correct release button
	  EndMenuEventGrab();
	  break;
	default:
	  break;
	};
    };

}

void GxPopupMenu::Draw(void)
{
  Draw3dBorder(0,0, width,height, true);
}
