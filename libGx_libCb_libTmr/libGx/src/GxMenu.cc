#include <libGx/GxMenu.hh>

#include "GxDefines.hh"
#include <libGx/GxMenuBar.hh>
#include <libGx/GxMenuItems.hh>

GxMenu::GxMenu(GxMenuBar *pMenuBar, const char* pLabel) :
  GxCoreOwnerWin(pMenuBar), selected(false), active(true), stackRight(false), menuPane(this)
{
  SetGeomControl( GxBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT, GX_V_CENTERED));

  GxSetLabel(label, GX_SHORT_LABEL_LEN, pLabel, labelLen);
}

GxMenu::~GxMenu(void)
{}

void GxMenu::SetLabel(const char* pLabel)
{
  GxSetLabel(label, GX_SHORT_LABEL_LEN, pLabel, labelLen);
}

void GxMenu::StackRight(bool tStackRight)
{
  if(stackRight != tStackRight)
    {
      stackRight = tStackRight;
      if(stackRight)
	{
	  SetGeomControl( GxBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_RIGHT,
				  GX_V_CENTERED) );
	}else
	  SetGeomControl( GxBasic(GX_WD_INT, GX_HT_INT, GX_FLOW_LEFT,
				  GX_V_CENTERED) );
    };
}

void GxMenu::Active(bool nActive)
{
  if(active == nActive) return;

  active = nActive;
  if(selected) return; //don't do anything about this now. bad time for a change

  if( Created() )
    {
      XClearWindow(dInfo.display, xWin);
      DrawMenu();
    };
}

bool GxMenu::Active(void) const
{
  return active;
}

void GxMenu::GetWindowData(XSetWindowAttributes &winAttributes,
			   ULINT &valueMask)
{
  valueMask |= CWEventMask;
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask | EnterWindowMask | LeaveWindowMask;
}

UINT GxMenu::GetDesiredWidth(void) const
{
  return 2*GX_BORDER_WD + 2 + XTextWidth(dInfo.pMenuFont, label, labelLen);
}

UINT GxMenu::GetDesiredHeight(void) const
{
  return dInfo.pMenuFont->ascent + dInfo.pMenuFont->descent + 6;
}

void GxMenu::HandleEvent(const XEvent &rEvent)
{
  switch(rEvent.type)
    {
    case Expose:
      if(rEvent.xexpose.count == 0)
	DrawMenu();
      break;
    case ButtonPress:
      if(rEvent.xbutton.button == 1 && active)
	{
	  int xroot = rEvent.xbutton.x_root - rEvent.xbutton.x;
	  int yroot = rEvent.xbutton.y_root - rEvent.xbutton.y;
	  //ocasionally the window manager will mapraised the application
	  //main window over the raised menu. I have noted the same behavior
	  //in netscape (a motif application) but this like netscape allows
	  //the user to recover by just clicking on the same menu-button to
	  //re-raise it.
	  //Perhaps the menu-pane could detect when this happens and re-re-map
	  //itself, but for know this will do just fine since this is such a
	  //rare occurance
	  //this is probably due to a bug in the autoraise feature of the
	  //version of fvwm I am using (1.23b)
	  if(!selected)
	    {
	      //the first time this happens we must acivate the menu
	      //(which results in its' being mapped) before we call
	      //MenuButtonPress because it starts the menu event loop and
	      //does not return untill the menu is done
	      Select(xroot, yroot);
	      ((GxMenuBar*)pWinAreaOwner)->MenuButtonPress(this);
	    }else
	      Select(xroot, yroot);
	};
      break;
    case ButtonRelease:
      if(rEvent.xbutton.button == 1)
	{
	  if(active)
	    ((GxMenuBar*)pWinAreaOwner)->MenuButtonRelease();
	  else
	    ((GxMenuBar*)pWinAreaOwner)->EndMenuEventGrab(); //the same as releasing it outside the app.
	};
      break;
    case EnterNotify:
      //we care about this only if we are in the menu event loop
      if(!selected && active)
	{
	  if( ((GxMenuBar*)pWinAreaOwner)->SelectMenuBecauseEnter(this) )
	    {
	      Select( rEvent.xcrossing.x_root - rEvent.xcrossing.x,
			rEvent.xcrossing.y_root - rEvent.xcrossing.y);
	    };
	};
      break;
    case LeaveNotify:
      //menu's are DeSelectd by the menu bar
      break;
    default:
      break;
    };
}

void GxMenu::Select(int xRoot, int yRoot)
{
  selected = true;
  DrawMenu();
  activateCB();
  menuPane.Display(xRoot, yRoot+height);
}

void GxMenu::DeSelect(void)
{
  selected = false;
  XUnmapWindow(dInfo.display, menuPane.GetWindow());
  XClearWindow(dInfo.display, xWin);
  DrawMenu();
  // we cannot do this _now_, but must add a CB to the menuBar deactivateCB();
  ((GxMenuBar*)pWinAreaOwner)->AddPostEventCB( deactivateCB.CloneCurrentCallback() );
}

bool GxMenu::GetButtonHeldMode(void) const
{
  return ((GxMenuBar*)pWinAreaOwner)->GetMenuMode();
}

void GxMenu::SetButtonHeldMode(bool newMode)
{
  ((GxMenuBar*)pWinAreaOwner)->SetMenuMode(newMode);
}

void GxMenu::EndMenuEventGrab(void)
{
  ((GxMenuBar*)pWinAreaOwner)->EndMenuEventGrab();
}

GxRealOwner & GxMenu::GetRealOwnerObject(void)
{
  return menuPane;
}

void GxMenu::DrawMenu(void)
{
  /*
#ifdef LIBGX_DEBUG_BUILD
  std::cout << "GxMenu::DrawMenu" << std::endl;
#endif //LIBGX_DEBUG_BUILD
  */
  if(selected)
    Draw3dBorder(0,0, width,height, true);

  XSetFont(dInfo.display,vData.menuGC, dInfo.pMenuFont->fid);
  if(!active)
    XSetForeground(dInfo.display, vData.menuGC, dInfo.unActiveLabelTextPix);

  XDrawString(dInfo.display, xWin, vData.menuGC, (GX_BORDER_WD + 1),
	      (height - GX_BORDER_WD - 1 - dInfo.pMenuFont->descent),
	      label, labelLen);
  if(!active)
    XSetForeground(dInfo.display, vData.menuGC, dInfo.labelTextPix);
}

// ************ start GxMenuPane class methods ************

GxMenuPane::GxMenuPane(GxMenu *pOwner) :
  GxRootTransient(pOwner)
{}

GxMenuPane::~GxMenuPane(void)
{}

void GxMenuPane::PlaceChildren(void)
{
  UINT totalHeight = GX_BORDER_WD; //the 2 is to consider my border
  UINT maxMinWidth = 0;

  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
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

void GxMenuPane::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      Draw();
}

void GxMenuPane::GetWindowData(XSetWindowAttributes &winAttributes,
				   ULINT &valueMask)
{
  GxRootTransient::GetWindowData(winAttributes, valueMask);
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

void GxMenuPane::Draw(void)
{
  /*
#ifdef LIBGX_DEBUG_BUILD
  std::cout << "GxMenuPane::DrawMenu" << std::endl;
#endif //LIBGX_DEBUG_BUILD
  */
  Draw3dBorder(0,0, width,height, true);
}

