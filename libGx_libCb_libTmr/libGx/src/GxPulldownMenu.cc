#include <string.h>

#include <libGx/GxDisplay.hh>

#include <libGx/GxPulldownMenu.hh>

#include "GxDefines.hh"

using namespace std;

// ***************************** GxPulldownItem *******************

GxPulldownItem::GxPulldownItem(const char *pLabel)
{
  unsigned junkLen;
  GxSetLabel(Label, GX_DEFAULT_LABEL_LEN, pLabel, junkLen);
}

GxPulldownItem::~GxPulldownItem(void)
{}

void GxPulldownItem::SetLabel(const char *pLabel)
{
  unsigned junkLen;
  GxSetLabel(Label, GX_DEFAULT_LABEL_LEN, pLabel, junkLen);
}

void GxPulldownItem::Draw(int x, int y, UINT width, UINT height,
			  GxListData &rLData, bool active) const
{
  if(!active)
    XSetForeground(rLData.dInfo.display, rLData.vData.textGC, rLData.dInfo.unActiveLabelTextPix);

  XDrawString(rLData.dInfo.display, rLData.win, rLData.vData.textGC,
	      x, y+rLData.dInfo.pDefaultFont->ascent, Label, strlen(Label) );

  if(!active)
    XSetForeground(rLData.dInfo.display, rLData.vData.textGC, rLData.dInfo.labelTextPix);
}

UINT GxPulldownItem::GetDesiredHeight(GxListData &rLData) const
{
  return rLData.dInfo.pDefaultFont->ascent + 
    rLData.dInfo.pDefaultFont->descent;
}

UINT GxPulldownItem::GetDesiredWidth(GxListData &rLData) const
{
  return (UINT)XTextWidth(rLData.dInfo.pDefaultFont, Label, strlen(Label) );
}

void GxPulldownItem::Select(void) const
{
  cb();
}


// *************************** GxPulldownPane ***********************

GxPulldownPane::GxPulldownPane(GxRealOwner *pOwner, GxPulldownMenu *pTMenu,
			       GxListData &rLData) :
  GxRootTransient(pOwner), pCurrentItem(0),
  winHolder(pWinAreaOwner->GetClosestMapHolder()), pMenu(pTMenu), listData(rLData),
  eventHandlerID(NULL_EVENT_HANDLER_ID), bPressHandlerID(NULL_EVENT_HANDLER_ID)
{}

GxPulldownPane::~GxPulldownPane(void)
{}

void GxPulldownPane::StartEventGrab(Window clipWin)
{
  XGrabPointer(dInfo.display, clipWin, true,
	       (ButtonPressMask | ButtonReleaseMask),
	       GrabModeAsync, GrabModeAsync, None, None, CurrentTime);

  dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxPulldownPane>
				     (this, &GxPulldownPane::GrabbedDisplayHandleEvent ),
				     eventHandlerID );
}

void GxPulldownPane::GrabbedDisplayHandleEvent(const XEvent &rEvent)
{
  if( !winHolder.SendXEventLocally(rEvent) )
    {
      //because we are a popup event we must eat certain events; like
      //ButtonPress and ButtonRelease; but some must make it to other
      //parts of the application; like expose events.
      switch(rEvent.type)
	{
	case Expose:
	  winHolder.SendXEventUp(rEvent);
	  break;
	  
	  //?hack?: should we really do this? ?we should not send these up?
	case FocusIn:
	case FocusOut:
	  winHolder.SendXEventUp(rEvent);
	  break;
	  
	case ButtonPress: //user clicked outside my window; exit menu
	case ButtonRelease:
	  EndEventGrab();
	  
	default:
	  break;//eat the event
	};
    };
  
}

void GxPulldownPane::EndEventGrab(void)
{
  XUnmapWindow(dInfo.display, xWin );
  XUngrabPointer(dInfo.display, CurrentTime);

  dInfo.rGxDisplay.RemoveEventHandler( eventHandlerID );
  eventHandlerID = NULL_EVENT_HANDLER_ID;
}


GxMapHolder* GxPulldownPane::GetClosestMapHolder(void)
{
  return &winHolder;
}

UINT GxPulldownPane::GetWidestItemWidth(void) const
{
  std::list<GxPulldownItem*>::const_iterator cPlace = itemList.begin();
  std::list<GxPulldownItem*>::const_iterator cEnd = itemList.end();
  UINT widestWidth = 0;

  while(cPlace != cEnd)
    {

      UINT tWidth = (*cPlace)->GetDesiredWidth(listData);
      widestWidth = (tWidth > widestWidth) ? tWidth : widestWidth;

      cPlace++;
    };

  return widestWidth;
}

void GxPulldownPane::GetDesiredSize(UINT &rDesiredWidth, UINT &rDesiredHeight) const
{
  std::list<GxPulldownItem*>::const_iterator cPlace = itemList.begin();
  std::list<GxPulldownItem*>::const_iterator cEnd = itemList.end();
  UINT widestWidth = 0;
  UINT totalHeight = 0;

  while(cPlace != cEnd)
    {

      UINT tWidth = (*cPlace)->GetDesiredWidth(listData);
      widestWidth = (tWidth > widestWidth) ? tWidth : widestWidth;

      totalHeight += (*cPlace)->GetDesiredHeight(listData);

      cPlace++;
    };

  rDesiredWidth = widestWidth + 2*GX_THIN_BORDER_WD;
  rDesiredHeight = totalHeight + 2*GX_THIN_BORDER_WD;
}

void GxPulldownPane::Create(void)
{
  //std::cout << "GxPulldownPane::Create" << std::endl;
  GxRootTransient::Create();
  listData.win = xWin;
}

void GxPulldownPane::HandleEvent(const XEvent &rEvent)
{
  switch(rEvent.type)
    {
    case Expose:
      DrawInterior();
      break;

    case ButtonPress:
      if(rEvent.xbutton.button != 1) break;
      pFItem = SelectItem(rEvent.xbutton.y);
      dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxPulldownPane>
					 (this, &GxPulldownPane::ButtonPressGrabbedHandleEvent ),
					 bPressHandlerID );
      break;

    default:
      break;
    };
}

void GxPulldownPane::ButtonPressGrabbedHandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == ButtonRelease && rEvent.xbutton.button == 1)
    {
      dInfo.rGxDisplay.RemoveEventHandler(bPressHandlerID);
      bPressHandlerID = NULL_EVENT_HANDLER_ID;
      const GxPulldownItem *pSItem = SelectItem(rEvent.xbutton.y);
      if(pFItem == pSItem)
	{
	  pCurrentItem = pFItem;
	  pMenu->DrawLabel(true);
	  EndEventGrab();
	  pFItem = 0;
	  pSItem->Select();
	};
    }else
    if( rEvent.type == Expose )
      dInfo.rGxDisplay.HandleSafeLoopEvent(rEvent);
}

const GxPulldownItem* GxPulldownPane::SelectItem(int yVal)
{
  UINT cY = GX_THIN_BORDER_WD;
  std::list<GxPulldownItem*>::const_iterator cPlace = itemList.begin();
  std::list<GxPulldownItem*>::const_iterator cEnd = itemList.end();
  while(cPlace != cEnd)
    {
      UINT newY = (*cPlace)->GetDesiredHeight(listData) + cY;
      if(yVal > cY && yVal < newY)
	return (*cPlace);
      
      cY = newY;
      cPlace++;
    };

  return (GxPulldownItem*)NULL;
}

void GxPulldownPane::GetWindowData(XSetWindowAttributes &winAttributes,
				   ULINT &valueMask)
{
  GxRootTransient::GetWindowData(winAttributes, valueMask);

  winAttributes.event_mask = ExposureMask | ButtonPressMask
    | ButtonReleaseMask;
  winAttributes.background_pixel = dInfo.whitePix;

  valueMask |= CWEventMask | CWBackPixel;
}

GxMapHolder * GxPulldownPane::GetMapHolder(void)
{
  return &winHolder;
}

void GxPulldownPane::DrawInterior(void)
{
  if( !Created() ) return;
  listData.win = xWin;
  int cY = GX_THIN_BORDER_WD;
  int cX = GX_THIN_BORDER_WD;

  std::list<GxPulldownItem*>::const_iterator cPlace = itemList.begin();
  std::list<GxPulldownItem*>::const_iterator cEnd = itemList.end();

  while(cPlace != cEnd)
    {
      (*cPlace)->Draw(cX, cY, 10,100, listData);
      cY += (*cPlace)->GetDesiredHeight(listData);
      cPlace++;
    };

  DrawThinBorder(0, 0, width, height);
}

// ************************** GxPulldownMenu *********************

GxPulldownMenu::GxPulldownMenu(GxRealOwner *pOwner) :
  GxOwnerWin(pOwner),
  listData( pWinAreaOwner->GetVolatileData(), pWinAreaOwner->GetDisplayInfo() ),
  AB(this), vLine(this), clipWindow(this), pdPane(&clipWindow, this, listData),
  desWidth(0), active(true)
{
#ifdef LIBGX_DEBUG_BUILD
  std::cout << "GxPulldownMenu::GxPulldownMenu" << std::endl;
#endif //LIBGX_DEBUG_BUILD
  label[0] = 0;
  AB.Resize(10,10);
  AB.SetDirection(GX_DOWN);
  AB.cb.Assign( CbVoidMember<GxPulldownMenu>(this, &GxPulldownMenu::DoPulldown) );
  AB.SetGeomControl( GxSBasic(GX_WD_FIXED, GX_HT_FIXED, GX_FLOW_RIGHT,
			      GX_V_CENTERED, 0,GX_BORDER_WD,GX_BORDER_WD,GX_BORDER_WD, true, false) );

  vLine.SetGeomControl( GxSBasic(GX_WD_FIXED, GX_HT_FILL, GX_FLOW_RIGHT,
				 GX_V_CENTERED, 0,0,GX_BORDER_WD,GX_BORDER_WD, true, false) );

  clipWindow.SetGeomControl( GxSBasic(GX_WD_FILL, GX_HT_FILL, GX_FLOW_LEFT,
				      GX_FLOW_UP, GX_BORDER_WD,0,GX_BORDER_WD,GX_BORDER_WD) );
}

GxPulldownMenu::~GxPulldownMenu(void)
{}

void GxPulldownMenu::SetActive(bool nActive)
{
  active = nActive;
  AB.SetActive(active);
  DrawLabel(); //does nothing now.
}

void GxPulldownMenu::Create(void)
{
  //std::cout << "GxPulldownMenu::Create" << std::endl;
  GxOwnerWin::Create();
  //std::cout << "child list size: " << childList.size() << std::endl;
}

void GxPulldownMenu::AddItem(GxPulldownItem *pNewItem)
{
  //add to the end of the list
  pdPane.itemList.push_back(pNewItem);
}

void GxPulldownMenu::RemoveItem(const GxPulldownItem *pItemToRemove)
{
  std::list<GxPulldownItem*>::iterator cPlace = pdPane.itemList.begin();
  std::list<GxPulldownItem*>::iterator cEnd = pdPane.itemList.end();
  while(cPlace != cEnd)
    {
      if( (*cPlace) == pItemToRemove )
	{
	  //hack. see if this is the current node
	  if( (*cPlace) == pdPane.pCurrentItem)
	    {
	      pdPane.pCurrentItem = NULL;
	      DrawLabel(true);
	    };

	  cPlace = pdPane.itemList.erase(cPlace);
	  return;
	};
      cPlace++;
    };
  //out of list without finding object; error;
}

void GxPulldownMenu::RemoveItem(UINT numToRemove)
{
  std::list<GxPulldownItem*>::iterator cPlace = pdPane.itemList.begin();
  std::list<GxPulldownItem*>::iterator cEnd = pdPane.itemList.end();
  unsigned cNum = 0;
  while(cPlace != cEnd)
    {
      if( cNum == numToRemove )
	{
	  //hack. see if this is the current node.
	  if( (*cPlace) == pdPane.pCurrentItem)
	    {
	      pdPane.pCurrentItem = NULL;
	      DrawLabel(true);
	    };

	  cPlace = pdPane.itemList.erase(cPlace);
	  return;
	};

      cNum++;
      cPlace++;
    };

  //error if we are here.
}

void GxPulldownMenu::SetNoneSelectedText(const char *pText)
{
  if(!pText)
    label[0] = '\0';
  else
    {
      strncpy(label, pText, GX_DEFAULT_LABEL_LEN-1);
      label[GX_DEFAULT_LABEL_LEN-1] = '\0';
    };

  if( Created() && !pdPane.pCurrentItem)
    DrawLabel(true);
}

void GxPulldownMenu::SetDesiredWidth(unsigned value)
{
  desWidth = value;
}

UINT GxPulldownMenu::GetDesiredWidth(void) const
{
  if(desWidth)
    return desWidth;
  else
    {
      unsigned calWidth = pdPane.GetWidestItemWidth() + vLine.Width() + vLine.RBorder() + vLine.LBorder()
	+ AB.Width() + AB.LBorder() + AB.RBorder() + 2*GX_BORDER_WD;
      
      return (30 > calWidth) ? 30 : calWidth; //30 is ugly.
    };
}

UINT GxPulldownMenu::GetDesiredHeight(void) const
{
  return dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent + 2*GX_BORDER_WD;
}

void GxPulldownMenu::DrawLabel(bool clear)
{
  if(clear && Created() )
    XClearWindow(dInfo.display, clipWindow.GetWindow());

  if(pdPane.pCurrentItem)
    {
      int selItemWidth = pdPane.pCurrentItem->GetDesiredWidth(listData);
      int selItemHeight = pdPane.pCurrentItem->GetDesiredHeight(listData);
      int xSpacing = ( ((int)clipWindow.Width()) - selItemWidth )/2;
      int ySpacing = ( ((int)clipWindow.Height()) - selItemHeight )/2;
      listData.win = clipWindow.GetWindow();
      pdPane.pCurrentItem->Draw(xSpacing,ySpacing, pdPane.Width(), pdPane.Height(), listData, active);
    }else
      if(label[0] != '\0')
	{
	  //std::cout << "drawing text" << std::endl;
	  int numChars = strlen(label);
	  int desWidth = XTextWidth(dInfo.pDefaultFont, label, numChars);
	  int desHeight = dInfo.pDefaultFont->ascent + dInfo.pDefaultFont->descent;
	  int hSpace = (int)clipWindow.Width();
	  int vSpace = (int)clipWindow.Height();

	  if(!active)
	    XSetForeground(dInfo.display, vData.textGC, dInfo.unActiveLabelTextPix);


	  XDrawString(dInfo.display, clipWindow.GetWindow(), vData.textGC,
		      (hSpace-desWidth)/2, (vSpace-desHeight)/2+dInfo.pDefaultFont->ascent, label, numChars);

	  if(!active)
	    XSetForeground(dInfo.display, vData.textGC, dInfo.labelTextPix);
	};
}

void GxPulldownMenu::SetCurrentItem(UINT numToSetCurrent)
{
  if( numToSetCurrent == 0 ) //we could process the loop and fall down to the 'error' but this would be tacky.
    {
      ClearCurrentItem();
      return;
    };

  std::list<GxPulldownItem*>::iterator cPlace = pdPane.itemList.begin();
  std::list<GxPulldownItem*>::iterator cEnd = pdPane.itemList.end();
  unsigned cNum = 1;
  while(cPlace != cEnd)
    {
      if( cNum == numToSetCurrent )
	{
	  if( (*cPlace) != pdPane.pCurrentItem)
	    {
	      pdPane.pCurrentItem = *cPlace;
	      DrawLabel(true);
	    };
	  return;
	};

      cNum++;
      cPlace++;
    };

  //error if we are here. we 'handle' this by setting the current item to 0.
  ClearCurrentItem();
}

UINT GxPulldownMenu::GetCurrentItem(void) const
{
  if( !pdPane.pCurrentItem ) return 0;

  list<GxPulldownItem*>::const_iterator cPlace = pdPane.itemList.begin();
  list<GxPulldownItem*>::const_iterator cEnd = pdPane.itemList.end();
  unsigned cNum = 1;
  while(cPlace != cEnd)
    {
      if( *cPlace == pdPane.pCurrentItem )
	return cNum;

      cNum++;
      cPlace++;
    };

  return 0; //error
}

void GxPulldownMenu::ClearCurrentItem(void)
{
  pdPane.pCurrentItem = NULL;
  DrawLabel(true);
}

void GxPulldownMenu::ClearItems(void)
{
  pdPane.itemList.clear();

  pdPane.pCurrentItem = NULL;
  DrawLabel(true);
}

void GxPulldownMenu::DeleteItems(void)
{
  while( !pdPane.itemList.empty() )
    {
      GxPulldownItem *pCItem = pdPane.itemList.front();
      delete pCItem;
      pCItem = 0;
      pdPane.itemList.pop_front();
    };

  pdPane.pCurrentItem = NULL;
  DrawLabel(true);
}

void GxPulldownMenu::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0 && Created() )
      Draw3dBorder(0,0, width,height, true);
}

void GxPulldownMenu::GetWindowData(XSetWindowAttributes &winAttributes,
				   ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

void GxPulldownMenu::DoPulldown(void)
{
#ifdef LIBGX_DEBUG_BUILD
  std::cout << "entering GxPulldownMenu::DoPulldown" << std::endl;
#endif //LIBGX_DEBUG_BUILD
  if(!active) return;

  UINT desiredWidth = 0;
  UINT desiredHeight = 0;
  pdPane.GetDesiredSize(desiredWidth, desiredHeight);
  pdPane.Resize(desiredWidth, desiredHeight);

  int xRoot, yRoot;
  Window junkWin;
  XTranslateCoordinates(dInfo.display, xWin, dInfo.rootWin, 0, height,
			&xRoot, &yRoot, &junkWin);

  pdPane.Display(xRoot, yRoot);
  pdPane.StartEventGrab( clipWindow.GetWindow() );

#ifdef LIBGX_DEBUG_BUILD
  std::cout << "leaving GxPulldownMenu::DoPulldown" << std::endl;
#endif //LIBGX_DEBUG_BUILD
}

// ********************************* start ClipWindow **************************

GxPulldownMenu::ClipWindow::ClipWindow(GxPulldownMenu *pTOwner) :
  GxOwnerWin(pTOwner)
{}

GxPulldownMenu::ClipWindow::~ClipWindow(void)
{}

void GxPulldownMenu::ClipWindow::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0 )
      ((GxPulldownMenu*)pWinAreaOwner)->DrawLabel();
}

void GxPulldownMenu::ClipWindow::GetWindowData(XSetWindowAttributes
					      &winAttributes,
					      ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}
