#include <libGx/GxDisplay.hh>

#include <libGx/GxList.hh>

#include "GxDefines.hh"

GxList::GxList(GxRealOwner *pOwner) :
  GxAppScrolledWin(pOwner),
  listData(pOwner->GetVolatileData(), pOwner->GetDisplayInfo()),
  pSelectedItem(NULL),
  pFirstItem(NULL),
  pCurrentTop(NULL),
  listX(GX_BORDER_WD + 1), //one extra for drawing the selection box
  maxMinWidth(0),
  listHeight(0),
  cWin(this)
{
  hScrollBar.scrollCB.Assign( CbOneMember<GxFraction, GxList>
			      (this, &GxList::HScrollCallback) );
  vScrollBar.scrollCB.Assign( CbOneMember<GxFraction, GxList>
			     (this, &GxList::VScrollCallback) );

  hScrollBar.scrollLeftCB.Assign( CbVoidMember<GxList>(this, &GxList::ScrollLeftCB) );
  hScrollBar.scrollRightCB.Assign( CbVoidMember<GxList>(this, &GxList::ScrollRightCB) );

  vScrollBar.scrollUpCB.Assign( CbVoidMember<GxList>(this, &GxList::ScrollUpCB) );
  vScrollBar.scrollDownCB.Assign( CbVoidMember<GxList>(this, &GxList::ScrollDownCB) );
  
  vSpacing = 5;
  hSpacing = 5;

  SetClipWindow(&cWin);
}

GxList::~GxList(void)
{
  Clear();
}

void GxList::AddListItemStart(GxListItem *pNewItem)
{
  if(!pFirstItem)
    {
      pFirstItem = pNewItem;
    }else
      {
	pFirstItem->pPrevItem = pNewItem;
	pNewItem->pNextItem = pFirstItem;
	pFirstItem = pNewItem;
      };
}

void GxList::AddListItemEnd(GxListItem *pNewItem)
{
  if(pFirstItem)
    {
      GxListItem *pLastItem = pFirstItem;
      while(pLastItem->pNextItem)
	pLastItem = pLastItem->pNextItem;

      pLastItem->pNextItem = pNewItem;
      pNewItem->pPrevItem = pLastItem;
    }else
      pFirstItem = pNewItem;
}

void GxList::AddListItemAfter(GxListItem *pRefItem, GxListItem *pNewItem)
{
  pNewItem->pPrevItem = pRefItem;
  //could be null; but dosen't matter
  pNewItem->pNextItem = pRefItem->pNextItem;
  pRefItem->pNextItem = pNewItem;
  //this does matter if pNewItem->pNextItem is null
  if(pNewItem->pNextItem)
    pNewItem->pNextItem->pPrevItem = pNewItem;
}

void GxList::AddListItemBefore(GxListItem *pRefItem, GxListItem *pNewItem)
{
  //this must be done because pFirstItem _must_ be first in our list
  if(pRefItem == pFirstItem)
    pFirstItem = pNewItem;

  pNewItem->pNextItem = pRefItem;
  //could be null; but dosen't matter
  pNewItem->pPrevItem = pRefItem->pPrevItem;
  pRefItem->pPrevItem = pNewItem;
  //this does matter if pNewItem->pPrevItem is null
  if(pNewItem->pPrevItem)
    pNewItem->pPrevItem->pNextItem = pNewItem;
}

void GxList::SetTopItem(GxListItem *pNewTop)
{
  //Is doing this now a hack? Is doing it at all a hack?
  //should this be automated?
  if(!pNewTop)
    {
      pCurrentTop = pFirstItem;
      hScrollBar.SetScrollFraction(GX_MIN_FRACTION);
      vScrollBar.SetScrollFraction(GX_MIN_FRACTION);
    }else
      pCurrentTop = pNewTop;
}

void GxList::SetSelectedItem(GxListItem *pSelItem)
{
  pSelectedItem = pSelItem;
}

void GxList::Clear(void)
{
  while(pFirstItem)
    {
      GxListItem *pItemToDelete = pFirstItem;
      pFirstItem = pFirstItem->pNextItem;

      delete pItemToDelete;
    };

  pCurrentTop = NULL;
  pSelectedItem = NULL;

  maxMinWidth = 0;
  /*
  if( pClipWin->Created() && !inhibitRedraw)
    {
      cWin.Clear();
      SizeAll();
    };
  */
}

void GxList::RemoveListItem(GxListItem *pItem)
{
  if(!pItem) return;

  GxListItem *pCItem = pFirstItem;
  while(pCItem)
    {
      if(pCItem == pItem)
	break;

      pCItem = pCItem->pNextItem;
    };

  //this could happen if for some chance pItem is not in by list.
  if(!pCItem)
    return;

  //pCItem is the first on the list
  if(!pCItem->pPrevItem)
    pFirstItem = pCItem->pNextItem;
  else
    (pCItem->pPrevItem)->pNextItem = pCItem->pNextItem;

  //pCItem is not the last one on the list
  if(pCItem->pNextItem)
    (pCItem->pNextItem)->pPrevItem = pCItem->pPrevItem;

  if(pCItem == pSelectedItem)
    pSelectedItem = NULL;

  if(pCItem == pCurrentTop)
    {
      if(pCItem->pNextItem)
	pCurrentTop = pCItem->pNextItem;
      else
	pCurrentTop = pCItem->pPrevItem; //could be null, but who cares
    };
}

GxListItem * GxList::GetFirstListItem(void)
{
  return pFirstItem;
}

GxListData& GxList::GetListData(void)
{
  return listData;
}

void GxList::Create(void)
{
  GxAppScrolledWin::Create();
  //the above should create the xWin of the clip win; now set it in listData
  listData.win = pClipWin->GetWindow();
}

void GxList::PlaceChildren(void)
{
  GxAppScrolledWin::PlaceChildren();
  SizeAll();
}

void GxList::SelectPress(int yClick)
{
  pSelectedItem = LookupListItem(yClick);
}

void GxList::SelectRelease(int yClick)
{
  if(pSelectedItem)
    {
      GxListItem *pNewItem = LookupListItem(yClick);
      if(pSelectedItem == pNewItem)
	{
	  //if(true) //selectSingle)
	  cWin.Clear();
	  DrawList();
	  pNewItem->SelectCallback();
	  //pSelectedItem = NULL;
	};
    };
}

void GxList::SelectMotion(int yClick)
{
#ifdef DEBUG_BUILD
  cout << "GxList::SelectMotion" << endl;
#endif //DEBUG_BUILD
}

void GxList::ClearWindow(void)
{
  cWin.Clear();
}

void GxList::DrawList(void)
{
  GxListItem *pCurrentItem = pCurrentTop;

  UINT cW, cH;
  pClipWin->GetSize(cW, cH);

  UINT cY = GX_BORDER_WD+1;//to space beyond were the border is
  while(cY < cH && pCurrentItem)
    {
      pCurrentItem->Draw(listX, cY, listData);

      cY += pCurrentItem->GetHeight() + 1;
      pCurrentItem = pCurrentItem->pNextItem;
    };

  if(pSelectedItem)
    {
      //draw a border around the selected item
      GxListItem *pCurrentItem = pCurrentTop;
      UINT cY = GX_BORDER_WD;//to space to where the border is
      while(cY < cH && pCurrentItem)
	{
	  if(pSelectedItem == pCurrentItem)
	    {
	      XSetForeground(listData.dInfo.display, listData.vData.borderGC,
			     listData.dInfo.blackPix);
	      XDrawRectangle(listData.dInfo.display, pClipWin->GetWindow(),
			     listData.vData.borderGC, GX_BORDER_WD, cY,
			     cW - (2*GX_BORDER_WD) - 1,
			     pCurrentItem->GetHeight() );
	      break;
	    };

	  cY += pCurrentItem->GetHeight() + 1;
	  pCurrentItem = pCurrentItem->pNextItem;
	};
    };

  //*must* draw this last
  pClipWin->Draw3dBorder(0,0, cW,cH, false);
}

void GxList::HScrollCallback(GxFraction nFr)
{
  UINT clipWidth, clipHeight;
  pClipWin->GetSize(clipWidth, clipHeight);
  clipWidth -= (2*GX_BORDER_WD +2); //+2 for l and right border

  if(clipWidth >= maxMinWidth)
    {
      hScrollBar.SetTotalFraction(GX_MAX_FRACTION);
      hScrollBar.SetScrollFraction(GX_MIN_FRACTION);
      return;
    };

  UINT scrollW = maxMinWidth - clipWidth;
  UINT startX = nFr.Convert(scrollW);

  int adj = 0;

#ifdef DEBUG_BUILD
  //there is a bug, this demostrates the approximate correct result
  if(startX == scrollW)
    {
      cout << "startX == scrollW" << endl;
      adj = -10;
    };
#endif //DEBUG_BUILD

  listX = GX_BORDER_WD +1 - startX + adj;

  //we don't want to Clear the window because erasing it; then
  //redrawing it will cause flashing at its border
  //XClearWindow(listData.dInfo.display, pClipWin->GetWindow());
  //this uses XClearArea
  cWin.Clear();
  DrawList();
}

void GxList::ScrollLeftCB(void)
{
  UINT clipW, clipH;
  pClipWin->GetSize(clipW, clipH);
  
  if( maxMinWidth <= clipW )
    listX = GX_BORDER_WD +1;
  else
    {
      //hack. not sure of what increment to move 5% is a guestimate.
      int hMoveInc = (maxMinWidth*5)/100;
      if(hMoveInc <= 25) hMoveInc = 25;
      
      if(listX >= (int)(GX_BORDER_WD +1) || (listX < 0 && -listX < -hMoveInc) )
	{
	  listX = (int)(GX_BORDER_WD+1);
	}else
	   {
	     listX += hMoveInc;
	     if(listX >= (int)(GX_BORDER_WD +1) )
	       listX = (int)(GX_BORDER_WD+1);
	   };
    };

  LocateSliders();
  cWin.Clear();
  DrawList();
}

void GxList::ScrollRightCB(void)
{
  UINT clipW, clipH;
  pClipWin->GetSize(clipW, clipH);
  
  if( maxMinWidth <= clipW )
    listX = GX_BORDER_WD +1;
  else
    {
      //hack. recalculation of hMoveInc. see above
      int hMoveInc = (maxMinWidth*5)/100;
      if(hMoveInc <= 25) hMoveInc = 25;
      
      int scrollDist = (maxMinWidth -clipW); //positive
      int minListX = -scrollDist - (int)(GX_BORDER_WD+1); //remember listX and minListX are negative here.
      if( listX < minListX || (-minListX - -listX) < hMoveInc)
	listX = minListX;
      else
	listX -= hMoveInc;
    };

  LocateSliders();
  cWin.Clear();
  DrawList();
}

void GxList::VScrollCallback(GxFraction nFr)
{
  //hack; we should not have to recalulate this everytime
  UINT clipHeight = pClipWin->Height() - 2*GX_BORDER_WD;

  if(listHeight <= clipHeight)
    {
#ifdef DEBUG_BUILD
      cout << "in GxList::VScrollCallback and no need to scroll" << endl;
#endif //DEBUG_BUILD
      //hack? should we reset the scrollbar? -> probably yes but this
      //should not be the default mechanism for doing so. just a failsafe.
      vScrollBar.SetTotalFraction(GX_MAX_FRACTION);
      vScrollBar.SetScrollFraction(GX_MIN_FRACTION);
      return;
    };

  //the place where we should start drawing the list
  UINT scrollH = listHeight - clipHeight;
  UINT startY = nFr.Convert(scrollH);
  //if(startY == scrollH)
  //cout << "startY == scrollH" << endl;

  UINT cY = 1; //one pixel border
  GxListItem *pCurrentItem = pFirstItem;
  while(pCurrentItem)
    {
      if(cY >= startY)
	{
	  if(pCurrentTop == pCurrentItem) //no need to scroll
	    return;

	  pCurrentTop = pCurrentItem;
	  //we don't want to Clear the window because erasing it; then
	  //redrawing it will cause flashing at its borders
	  //XClearWindow(listData.dInfo.display, pClipWin->GetWindow());
	  //this uses XClearArea
	  cWin.Clear();
	  DrawList();
	  return;
	};

      cY += pCurrentItem->GetHeight() + 1; //one pixel border between items
      pCurrentItem = (pCurrentItem->pNextItem);
    };

  //hack; this should never happen. if it does do what?
}

void GxList::ScrollUpCB(void)
{
  if( pCurrentTop )
    {
      if( pCurrentTop->pPrevItem )
	{
	  pCurrentTop = pCurrentTop->pPrevItem;
	  LocateSliders();
	  cWin.Clear();
	  DrawList();
	};
    };
}

void GxList::ScrollDownCB(void)
{
  if( !pCurrentTop ) return;

  UINT clipHeight = pClipWin->Height() - 2*GX_BORDER_WD;
      
  GxListItem *pCItem = pCurrentTop;
  unsigned cY = 1 + pCItem->GetHeight() + 1;
  while(pCItem->pNextItem && cY <= clipHeight)
    {
      pCItem = pCItem->pNextItem;
      cY += pCItem->GetHeight() + 1; //one pixel border between items
    };

  if( cY > clipHeight )
    {
      pCurrentTop = pCurrentTop->pNextItem;
      LocateSliders();
      cWin.Clear();
      DrawList();
    };
}

void GxList::SizeAll(void)
{
  //figure out the total width/height of the stack of list Items
  maxMinWidth = 0;
  listHeight = 1; //one pixel border
  GxListItem *pCurrentItem = pFirstItem;
  while(pCurrentItem)
    {
      pCurrentItem->Size(listData);
      UINT cWidth = pCurrentItem->GetWidth();
      maxMinWidth = (cWidth > maxMinWidth) ? cWidth : maxMinWidth;
      listHeight += pCurrentItem->GetHeight() + 1; //one pixel between elements

      pCurrentItem = pCurrentItem->pNextItem;
    };
  listHeight += 1; //one pixel border

  UINT clipW, clipH;
  pClipWin->GetSize(clipW, clipH);
  //we must subtract (GX_BORDER_WD*2) pixels from both dimensions to take
  //into account the border around the clipwin, but we must be careful aboout
  //roll-over in our numbers. the +2 is for the 1 pixel item border
  if(clipW - (GX_BORDER_WD*2 + 2) < clipW)
    clipW -= (GX_BORDER_WD*2 + 2);
  if(clipH - (GX_BORDER_WD*2 + 2) < clipH)
    clipH -= (GX_BORDER_WD*2 + 2);

  //now resize all of the list items
  maxMinWidth = ((clipW-2) > maxMinWidth) ? (clipW-2) : maxMinWidth;
  pCurrentItem = pFirstItem;
  while(pCurrentItem)
    {
      pCurrentItem->SetWidth(maxMinWidth);
      pCurrentItem = pCurrentItem->pNextItem;
    };

#ifdef DEBUG_BUILD
  cout << "list Widths" << clipW << "," << maxMinWidth << endl;
  cout << "list Heights" << clipH << "," << listHeight << endl;
#endif //DEBUG_BUILD
  hScrollBar.SetTotalFraction(GxFraction(clipW, maxMinWidth));
  vScrollBar.SetTotalFraction(GxFraction(clipH, listHeight));
}

void GxList::LocateSliders(void)
{
  UINT clipW, clipH;
  pClipWin->GetSize(clipW, clipH);
  
  if( maxMinWidth <= clipW )
    hScrollBar.SetScrollFraction(GX_MIN_FRACTION);
  else
    {
      int scrollDist = (int)(maxMinWidth - clipW);
      if( listX >= 0 )
	hScrollBar.SetScrollFraction( GX_MIN_FRACTION );
      else
	if( -listX >= scrollDist ) //problemish
	  hScrollBar.SetScrollFraction(GX_MAX_FRACTION);
	else
	  hScrollBar.SetScrollFraction(GxFraction(-listX, maxMinWidth-clipW) );
    };


  if(listHeight <= clipH)
    vScrollBar.SetScrollFraction( GX_MIN_FRACTION );
  else
    {
      int listY = 1;
      GxListItem *pCItem = pFirstItem;
      while(pCItem && pCItem != pCurrentTop)
	{
	  listY += 1 + pCItem->GetHeight();
	  pCItem = pCItem->pNextItem;
	};
      
      if(listY >= (listHeight - clipH) ) //problemish
	vScrollBar.SetScrollFraction( GX_MAX_FRACTION );
      else
	vScrollBar.SetScrollFraction( GxFraction(listY, listHeight-clipH) );
    };
}

UINT GxList::GetNumItems(void)
{
  GxListItem *pCItem = pFirstItem;
  UINT numItems = 0;
  while(pCItem)
    {
      numItems++;
      pCItem = pCItem->pNextItem;
    };

  return numItems;
}

GxListItem* GxList::LookupListItem(int yClick)
{
  //starting from pCurrentTopNode, try and find the node yClick is in.
  //if we cannot find a node; return a NULL pointer

  int cY = GX_BORDER_WD + 1;
  if(yClick <= cY)
    return (GxListItem*)NULL;

  GxListItem *pCurrentItem = pCurrentTop;
  while(pCurrentItem)
    {
      cY += pCurrentItem->GetHeight() + 1;
      if(yClick < cY)
	return pCurrentItem;

      pCurrentItem = (pCurrentItem->pNextItem);
    };

  //only if we haven't returned by now
  return (GxListItem*)NULL;
}

/////////////////////////////////////////////////////////////////////////

GxList::ListPane::ListPane(GxList *pOwner) :
  GxWin(pOwner), pushedHandlerID(NULL_EVENT_HANDLER_ID)
{}

GxList::ListPane::~ListPane(void)
{}

void GxList::ListPane::Clear(void)
{
  if( !Created() ) return;

  XClearArea(dInfo.display, xWin, GX_BORDER_WD,GX_BORDER_WD,
	     width-(GX_BORDER_WD*2), height-(GX_BORDER_WD*2), false);
}

void GxList::ListPane::HandleEvent(const XEvent &rEvent)
{
  switch(rEvent.type)
    {
    case Expose:
      if(rEvent.xexpose.count == 0)
	((GxList*)pWinAreaOwner)->DrawList();
      break;

    case ButtonPress:
      //grab events until we get the matching button release
      if(rEvent.xbutton.button == 1)
	{
	  //the server is grabed.
	  ((GxList*)pWinAreaOwner)->SelectPress(rEvent.xbutton.y);
	    dInfo.rGxDisplay.PushEventHandler( CbOneMember<const XEvent&, GxList::ListPane>
					       (this, &GxList::ListPane::HandleGrabbedEvents),
					       pushedHandlerID );
	};
      break;

    default:
      //do nothing
      break;
    };
}

void GxList::ListPane::HandleGrabbedEvents(const XEvent &rEvent)
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
      ((GxList*)pWinAreaOwner)->SelectMotion(finalMotion.xmotion.y);
    }else
    if(rEvent.type == ButtonRelease && rEvent.xbutton.button == 1 && rEvent.xbutton.window == xWin)
      {
	//if the button release was in the same item; return.
	((GxList*)pWinAreaOwner)->SelectRelease(rEvent.xbutton.y);
	dInfo.rGxDisplay.RemoveEventHandler(pushedHandlerID);
	pushedHandlerID = NULL_EVENT_HANDLER_ID;
      }else
      if(rEvent.type == Expose )
	{
	  dInfo.rGxDisplay.HandleSafeLoopEvent(rEvent);
	};
}

void GxList::ListPane::GetWindowData(XSetWindowAttributes &winAttributes,
				     ULINT &valueMask)
{
  winAttributes.background_pixel = dInfo.whitePix;
  winAttributes.event_mask = ExposureMask | ButtonPressMask |
    ButtonReleaseMask | Button1MotionMask;
  valueMask |= CWEventMask;
}

///////////////////////////////////////////////////////////////////////////////

GxListItem::GxListItem(void) :
  pPrevItem(NULL), pNextItem(NULL)
{}

GxListItem::~GxListItem(void)
{}

void GxListItem::Size(GxListData &)
{}

void GxListItem::SelectCallback(void)
{}

void GxListItem::SetWidth(UINT newWidth)
{
  itemWidth = newWidth;
}

void GxListItem::Draw(int, int, const GxListData &)
{}

UINT GxListItem::GetWidth(void)
{
  return itemWidth;
}

UINT GxListItem::GetHeight(void)
{
  return itemHeight;
}

GxListItem * GxListItem::GetPrevItem(void)
{
  return pPrevItem;
}

GxListItem * GxListItem::GetNextItem(void)
{
  return pNextItem;
}

GxTextListItem::GxTextListItem(void) :
  GxListItem(), labelLen(0)
{
  label[0] = '\0';
}

GxTextListItem::~GxTextListItem(void)
{}

void GxTextListItem::SetLabel(const char *pLabel)
{
  GxSetLabel(label, GX_TEXT_LIST_ITEM_LABEL_LEN, pLabel, labelLen);
}

void GxTextListItem::Size(GxListData &rData)
{
  itemHeight = rData.dInfo.pDefaultFont->ascent +
    rData.dInfo.pDefaultFont->descent + 2;

  itemWidth = 2+XTextWidth(rData.dInfo.pDefaultFont, label, labelLen);
}

void GxTextListItem::SelectCallback(void)
{
  cb();
}

void GxTextListItem::Draw(int x, int y, const GxListData &rData)
{
  if(labelLen == 0)
    return;

  XSetFont(rData.dInfo.display, rData.vData.borderGC,
	   rData.dInfo.pDefaultFont->fid);
  XSetForeground(rData.dInfo.display, rData.vData.borderGC,
		 rData.dInfo.blackPix);
  XDrawString(rData.dInfo.display, rData.win, rData.vData.borderGC,
	      x+1, y + rData.dInfo.pDefaultFont->ascent + 1, label, labelLen);
}


GxListData::GxListData(GxVolatileData &tVData, GxDisplayInfo &tDInfo) :
  vData(tVData), dInfo(tDInfo), win(None)
{}

GxListData::~GxListData(void)
{}
