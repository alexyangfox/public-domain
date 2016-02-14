#ifdef LIBGX_DEBUG_BUILD
#include <assert.h>
#endif //LIBGX_DEBUG_BUILD

#include <libGx/GxDisplay.hh>
#include <libGx/GxMenu.hh>

#include <libGx/GxMenuBar.hh>

#include "GxDefines.hh"

using namespace std;

GxMenuBar::GxMenuBar(GxRealOwner *pOwner) :
  GxCoreOwnerWin(pOwner), GxSubMapHolder( pOwner->GetClosestMapHolder() ),
  pActiveMenu(NULL), eventHandlerID(NULL_EVENT_HANDLER_ID)
{
  SetGeomControl( GxBasic(GX_WD_FILL, GX_HT_INT, GX_FLOW_LEFT, GX_FLOW_UP) );
}

GxMenuBar::~GxMenuBar(void)
{
  //hack?
  while( !postCBList.empty() )
    {
      CbVoidBase *pCB = postCBList.front();
      delete pCB;
      pCB = 0;
      postCBList.pop_front();
    };
}

UINT GxMenuBar::GetDesiredWidth(void) const
{
  //hack; shouldn't this return the width of all of my menu's?
  //do we even need to bother?
  return width;
}

UINT GxMenuBar::GetDesiredHeight(void) const
{
  return (dInfo.pMenuFont->ascent + dInfo.pMenuFont->descent +
	  2*GX_BORDER_WD + 6);  
}

void GxMenuBar::PlaceChildren(void)
{
  //std::cout << "GxMenuBar::PlaceChildren child list size: " << childList.size() << std::endl;
  int lX = GX_BORDER_WD;
  int rX = (int)(width - GX_BORDER_WD);
  int tY = 0;
  int bY = (int)height;

  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
#ifdef LIBGX_DEBUG_BUILD
      assert(this); //wtf did I need this for?
      assert( (*cPlace) );
#endif //LIBGX_DEBUG_BUILD

      //note each iteration through lX rX tY bY are posibly modified
      //by each child's GxGeomControl ?(if it has one)?
      (*cPlace)->Place(lX, rX, tY, bY);

      cPlace++;
    };
}

GxMapHolder* GxMenuBar::GetClosestMapHolder(void)
{
  return this;
}

void GxMenuBar::UnManageWindow(Window winID)
{
  UnManageWin(winID);
}

void GxMenuBar::GetWindowData(XSetWindowAttributes &winAttributes,
			      ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask;

  valueMask |= CWEventMask; //we still want GxWin's colors
}

void GxMenuBar::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type ==  Expose)
    if(rEvent.xexpose.count == 0)
      DrawBar();
  return;
}

void GxMenuBar::MenuButtonPress(GxMenu *pMenu)
{
  if( !InMenuLoop() ) //this is the first time
    {
      pActiveMenu = pMenu;
      //we will handle all events by the GrabbedDisplayHandleEvent() function untill the user is done with the menu
      XGrabPointer(dInfo.display, xWin, true, ButtonReleaseMask,
		   GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

      dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxMenuBar>(this, &GxMenuBar::GrabbedDisplayHandleEvent ),
					 eventHandlerID );
    }else
      {
	//a menu must be currently active
	if(pActiveMenu)
	  pActiveMenu->DeSelect();
	pActiveMenu = pMenu;
      };

  buttonHeld = true;
}

void GxMenuBar::MenuButtonRelease(void)
{
  if( !InMenuLoop() ) return;

  //pMenu should already be activated due to the crossing event;
  //so we just have to update button held
  buttonHeld = false;
}

bool GxMenuBar::SelectMenuBecauseEnter(GxMenu *pMenu)
{
  if( InMenuLoop() && buttonHeld)
    {
      if(pActiveMenu) //should be valid
	pActiveMenu->DeSelect();
      pActiveMenu = pMenu;
      return true;
    }else
      return false;
}

void GxMenuBar::EndMenuEventGrab(void)
{
  dInfo.rGxDisplay.RemoveEventHandler( eventHandlerID );
  eventHandlerID = NULL_EVENT_HANDLER_ID;

  //may push a callback to the post cb list
  if(pActiveMenu) //should be valid
    {
      pActiveMenu->DeSelect();
      pActiveMenu = NULL;
    };
}

bool GxMenuBar::GetMenuMode(void)
{
  return buttonHeld;
}

void GxMenuBar::SetMenuMode(bool newMode)
{
  buttonHeld = newMode;
}

bool GxMenuBar::InMenuLoop(void)
{
  return eventHandlerID != NULL_EVENT_HANDLER_ID;
}

void GxMenuBar::AddPostEventCB(CbVoidBase* pVoidBaseCB)
{
  postCBList.push_back(pVoidBaseCB);
}

void GxMenuBar::DrawBar(void)
{
  Draw3dBorder(0,0, width,height, true);
}

void GxMenuBar::GrabbedDisplayHandleEvent(const XEvent &rEvent)
{
  //cout << "GxMenuBar::GrabbedDisplayHandleEvent" << endl;

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
	  
	case ButtonRelease: //hack. I need to test if the relase is for the same button that started the loop
	  //this only happens if the button was released outside the
	  //application or in a window not in the menu heiarchy
	  EndMenuEventGrab();
	  //all is back to normal, this function will not be called until a child menu is activated again
	  break;
	  
	default:
	  break;
	};
    };

  //if we just delivered an event that caused teh event handler to be removed
  if( eventHandlerID == NULL_EVENT_HANDLER_ID )
    {
      XUngrabPointer(dInfo.display, CurrentTime);
      //we must be absolutly sure that the events have been processed before
      //the MenuItem is allowed to call it's function because the server is
      //still locked untill they are flushed
      //XSync(dInfo.display, false); //hack? commented out because I think! it is no longer relevant
      
      while( !postCBList.empty() )
	{
	  CbVoidBase *pCB = postCBList.front();
	  pCB->DoCallback();
	  delete pCB;
	  pCB = 0;
	  postCBList.pop_front();
	};
    }
}
