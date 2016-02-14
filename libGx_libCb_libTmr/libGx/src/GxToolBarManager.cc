#define LIBGX_DEBUG_BUILD

#ifdef LIBGX_DEBUG_BUILD
#include <assert.h>
#include <iostream>
#endif //LIBGX_DEBUG_BUILD

#include <libGx/GxToolBarManager.hh>

#include "GxDefines.hh"

using namespace std;

GxToolBarManager::GxToolBarManager(GxDisplay *pOwner) :
  pDisp(pOwner), pToolBarMoving(0), pushedHandlerID(NULL_EVENT_HANDLER_ID), pFDStart(NULL), pDStart(NULL),
  lastX(0), lastY(0), outlineDrawn(false), toolBarLength(1), toolBarVertical(false),
  pActiveDock(0), gWin(None)
{}

GxToolBarManager::~GxToolBarManager(void)
{
  while( !floatingDockList.empty() )
    {
      GxToolBarFloatingDock* pFDock = floatingDockList.front();
      delete pFDock;
      pFDock = 0;
      floatingDockList.pop_front();
    };

  //we do not own the docks in dockList
}

void GxToolBarManager::AddToolBarDock(GxToolBarDock *pDock)
{
  dockList.push_back(pDock);
}

void GxToolBarManager::RemoveToolBarDock(GxToolBarDock *pDock)
{
  list<GxToolBarDock*>::iterator cPlace = dockList.begin();
  list<GxToolBarDock*>::iterator cEnd = dockList.end();
  while(cPlace != cEnd)
    {
      if( (*cPlace) == pDock)
	{
	  cPlace = dockList.erase(cPlace);
	  return;
	};
      cPlace++;
    };
}

void GxToolBarManager::DeleteToolBarFloatingDock(GxToolBarFloatingDock *pDock)
{
  list<GxToolBarFloatingDock*>::iterator cPlace = floatingDockList.begin();
  while(cPlace != floatingDockList.end() )
    {
      if( *cPlace == pDock )
	cPlace = floatingDockList.erase(cPlace);
      else
	cPlace++;
    }

  delete pDock;
  pDock = 0;
}


void GxToolBarManager::DisplayToolBar(bool show, GxToolBar *pToolBar)
{
  if(!pToolBar) return;

  if(!show)
    {
      HideToolBar(pToolBar);
      return;
    };

  if( ToolBarShown(pToolBar) ) return;
  AddToolBarToNewFloatingDock(pToolBar);
  pToolBar->displayChangeCB(true);
}

void GxToolBarManager::DisplayToolBar(bool show, GxToolBar *pToolBar, GxToolBarDock *pDock)
{
  if(!pToolBar || !pDock) return;

  if(!show)
    {
      HideToolBar(pToolBar);
      return;
    };

  if( ToolBarShown(pToolBar) ) return;

  //hackish that we do not verify the dock is one of mine.
  pDock->AddToolBar(pToolBar);
  pToolBar->displayChangeCB(true);
}

bool GxToolBarManager::ToolBarShown(GxToolBar *pToolBar)
{
  list<GxToolBarDock*>::iterator dockPlace;
  bool presentDock = PresentDock(pToolBar, dockPlace);

  list<GxToolBarFloatingDock*>::iterator winPlace;
  bool presentWin = PresentFloatingDock(pToolBar, winPlace);

  return (presentDock || presentWin);
}

void GxToolBarManager::HideToolBar(GxToolBar *pToolBar)
{
  if(!pToolBar) return;

  list<GxToolBarDock*>::iterator dockPlace;
  bool presentDock = PresentDock(pToolBar, dockPlace);

  list<GxToolBarFloatingDock*>::iterator winPlace;
  bool presentWin = PresentFloatingDock(pToolBar, winPlace);

  if(presentDock)
    {
      //remove it from the dock
      GxToolBarDock *pDock = *dockPlace;
      //dock's don't go anywhere, so don't remove them
      //dockList.remove(dockPix);
#ifdef LIBGX_DEBUG_BUILD
      cout << "GxToolBarManager::HideToolBar strip state not implemented" << endl;
#endif //LIBGX_DEBUG_BUILD
      pDock->RemoveToolBar(pToolBar);
      pToolBar->displayChangeCB(false);
    };

  if(presentWin)
    {
      //we remove it from the floating dock, and delete the dock only if it is empty.
      //first we strip the dock state
      GxToolBarFloatingDock *pFloatingDock = *winPlace;
      int xPlace = 0, yPlace = 0;
      unsigned numButtonsRow = 1;
      pFloatingDock->GetGeom(xPlace, yPlace, numButtonsRow);
      //GX_TOOLBAR_LOCATION tbPlace = GX_TOOLBAR_ROOT_WIN;
      //pToolBar->SetPrevPosition(tbPlace, xPlace, yPlace, numButtonsRow);

      pFloatingDock->RemoveToolBar(pToolBar);
      pToolBar->displayChangeCB(false);
      if( !pFloatingDock->DockUsed() )
	{
	  winPlace = floatingDockList.erase(winPlace);
	  delete pFloatingDock;
	};
    };
}

void GxToolBarManager::MoveToolBar(GxToolBar *pToolBar, GxToolBarFloatingDock *pTFDStart,
				   GxToolBarDock *pTDStart)
{
  if(!pTFDStart && !pTDStart) return; //an error.

  Window grabWindow;
  if(pTFDStart)
    {
      pFDStart = pTFDStart;
      pDStart = 0;

      grabWindow = pFDStart->GetWindow();
      toolBarVertical = pFDStart->Vertical();
    }else
      {
	pDStart = pTDStart;
	pFDStart = 0;

	grabWindow = pDStart->GetWindow();
	toolBarVertical = pDStart->Vertical();
      };

  toolBarLength = pToolBar->BarLength();
  const GxDisplayInfo &dInfo = pDisp->GetDisplayInfo();
  //this active grab overrides the passive (and now active) grab of the button in the toolbar dock
  //hack. I would like to set owner_events to be false and use the subwindow attribute of the
  //event, but it does not seem to work.
  int stat =  XGrabPointer(dInfo.display, dInfo.rootWin, true,
			   ButtonPressMask | ButtonReleaseMask | Button2MotionMask |
			   EnterWindowMask | LeaveWindowMask,
			   GrabModeAsync, GrabModeAsync, None, dInfo.defaultCursor,
			   CurrentTime);
  
  if(stat != GrabSuccess) return;

  pToolBarMoving = pToolBar;
  outlineDrawn = false;

  //start a local loop/grab untill the button release.  we will try to
  //place the toolbar in the window we get in the Button Release Event
  pDisp->PushEventHandler( CbOneMember<const XEvent&, GxToolBarManager>
			   (this, &GxToolBarManager::HandleGrabbedEvents),
			   pushedHandlerID );
}

void GxToolBarManager::HandleGrabbedEvents(const XEvent &rEvent)
{
  XEvent localEvent = rEvent;
  XEvent junkEvent;

  if(localEvent.type == MotionNotify)
    {
      //this is more important than a motion event.
      if( XCheckTypedEvent(pDisp->XDisp(), ButtonRelease | EnterNotify | LeaveNotify | Expose, &junkEvent) )
	{
	  localEvent = junkEvent;
	}else
	{
	  //get the aboslute last Motion notify
	  while(true)
	    {
	      if( !XCheckTypedEvent(pDisp->XDisp(), PointerMotionMask, &junkEvent) ) break;
	      localEvent = junkEvent;//got a new motion event.
	    };
	};
    }

  switch(localEvent.type)
    {
    case MotionNotify:
      EraseToolBarOutline();
      if(pActiveDock)
	{
	  //cout << "motion notify had active dock" << endl;
	  int pointerX = localEvent.xmotion.x, pointerY = localEvent.xmotion.y;
	  Window dockBaseWin = pActiveDock->GetDockBaseWindow();
	  if(localEvent.xmotion.window != dockBaseWin )
	    {
	      Window junkWin;
	      XTranslateCoordinates(pDisp->XDisp(), localEvent.xmotion.window, dockBaseWin,
				    localEvent.xmotion.x, localEvent.xmotion.y, &pointerX, &pointerY, &junkWin);
	    };
	  unsigned newDockRow, newDockRowPlace;
	  pActiveDock->GetPhantomPlace(GX_BORDER_WD, GX_BORDER_WD, pointerX, pointerY,
				       newDockRow, newDockRowPlace);
#ifdef LIBGX_DEBUG_BUILD
	  cout << "got dock coords: " << phantomDockRow << "," << phantomRowPlace << endl;
#endif //LIBGX_DEBUG_BUILD

	  if(newDockRow != phantomDockRow || newDockRowPlace != phantomRowPlace )
	    {
	      phantomDockRow = newDockRow;
	      phantomRowPlace = newDockRowPlace;
	      bool warpPossible = pActiveDock->SetPhantomVisible(phantomDockRow, phantomRowPlace, toolBarLength);
	      //cout << "doing warp" << endl;
	      //This does not work in xnest?! ?Xnest bug?
	      //the dock may reflow by setting phantom visible if the docks origin moves
	      //wrt the pointer, then we would get an immediate motion event to a different
	      //location.  Our only choise is to move the pointer so it matintains its position
	      //wrt the modified dock origin
	      if( warpPossible )
		{
		  XWarpPointer(pDisp->XDisp(), None, dockBaseWin, 0,0,0,0, pointerX, pointerY);
		  XSync(pDisp->XDisp(), false); //hack. don't like. but does _greatly_ improve things.
		};
	    };
	};
      DrawToolBarOutline(localEvent.xmotion.x_root, localEvent.xmotion.y_root);
      break;
	  
    case LeaveNotify:
#ifdef LIBGX_DEBUG_BUILD
      cout << "got leave notify" << endl;
#endif //LIBGX_DEBUG_BUILD
      if( pActiveDock )
	if(pActiveDock->GetDockBaseWindow() == localEvent.xcrossing.window )
	  if(localEvent.xcrossing.detail != NotifyInferior)
	    {
	      LeaveActiveDock();
	      pActiveDock = 0;
	    };
      break;
      
    case EnterNotify:
#ifdef LIBGX_DEBUG_BUILD
      cout << "got enter notify" << endl;
#endif //LIBGX_DEBUG_BUILD
      PossibleEnterDock(localEvent.xcrossing.window);
      break;
      
    case ButtonRelease:
      if(localEvent.xbutton.button != 2) break;
      //cout << hex << lEvent.xbutton.window << " " << lEvent.xbutton.subwindow << dec << endl;
      
      //if we are here the correct button was released
      //no matter what else we stop grabbing events
      EraseToolBarOutline();
      if( pActiveDock )
	{
	  pActiveDock->HidePhantom();
	  pActiveDock = 0;
	};
      MoveToolBar(pToolBarMoving, localEvent.xbutton.window, localEvent.xbutton.x, localEvent.xbutton.y);
      pToolBarMoving = 0;
      XUngrabPointer(pDisp->XDisp(), CurrentTime);
      pDisp->RemoveEventHandler(pushedHandlerID);
      pushedHandlerID = NULL_EVENT_HANDLER_ID;
      break;

    case Expose:
      pDisp->HandleSafeLoopEvent(localEvent);
      break;

    default:
      break;
    };
}

void GxToolBarManager::SetGroupWindow(Window xID)
{
  gWin = xID;
}

Window GxToolBarManager::GetGroupWindow(void)
{
  return gWin;
}

void GxToolBarManager::RegisterToolBar(GxToolBar &rToolBar)
{
  list<GxToolBar*>::iterator cPlace = tbList.begin();
  while( cPlace != tbList.end() )
    {
      if( (*cPlace) == &rToolBar ) //already registered
	return;
      cPlace++;
    };
  
  tbList.push_back(&rToolBar);
}

void GxToolBarManager::UnRegisterToolBar(GxToolBar &rToolBar)
{
  list<GxToolBar*>::iterator cPlace = tbList.begin();
  while( cPlace != tbList.end() )
    {
      if( (*cPlace) == &rToolBar )
	{
	  tbList.erase(cPlace);
	  return;
	};

      cPlace++;
    };
}

void GxToolBarManager::OrganizeAndPlaceToolBars(void)
{
  list<GxToolBarDock*>::iterator cPlace = dockList.begin();
  while( cPlace != dockList.end() )
    {
      GxToolBarDock *pDock = *cPlace;
#ifdef LIBGX_DEBUG_BUILD
      assert(pDock);
#endif //LIBGX_DEBUG_BUILD

      unsigned cDockID =  pDock->GetDockID();
#ifdef LIBGX_DEBUG_BUILD
      assert(cDockID); //a slightly harsh test
#endif //LIBGX_DEBUG_BUILD
      if( !cDockID ) //an error
	{
	  cPlace++;
	  continue;
	};

      list<GxToolBar*> currentDockBars;

      list<GxToolBar*>::iterator dPlace = tbList.begin();
      while(dPlace != tbList.end() )
	{
	  GxToolBar *pBar = *dPlace;

	  unsigned desDock = pBar->GetDesDock();
	  if( desDock == cDockID )
	    currentDockBars.push_back(pBar);

	  dPlace++;
	};

      if( currentDockBars.empty() )
	{
	  cPlace++;
	  continue;
	};

      //sort the bars that are on the curent dock
      //this depends on !currentDockBars.empty()
      bool changes = true;
      while( changes )
	{
	  changes = false;
	  dPlace = currentDockBars.begin();
	  list<GxToolBar*>::iterator prevPlace = dPlace;
	  dPlace++;
	  while(dPlace != currentDockBars.end() && !changes )
	    {
	      GxToolBar& rCBar = *(*dPlace);
	      GxToolBar& rPrevBar = *(*prevPlace);

	      unsigned cBarRow = 0, cBarRowPlace = 0;
	      rCBar.GetDesDockPlace(cBarRow, cBarRowPlace);
	      unsigned prevBarRow = 0, prevBarRowPlace = 0;
	      rPrevBar.GetDesDockPlace(prevBarRow, prevBarRowPlace);
	      if( cBarRow < prevBarRow || 
		  ( (cBarRow == prevBarRow) && cBarRowPlace < prevBarRowPlace ) )
		{
		  //exchange the locations
		  dPlace = currentDockBars.erase(dPlace); //prevPlace is still valid
		  currentDockBars.insert(prevPlace, &rCBar);
		  changes = true;
		};
	      dPlace++;
	    };
	};

      //add the toolbars to the dock in their sorted order
      //should I give them correct places now?
      //?what are the correct places?
      dPlace = currentDockBars.begin();
      unsigned currentIntRow = 0;
      unsigned currentIntRowPlace = 0;
      (*dPlace)->GetDesDockPlace(currentIntRow, currentIntRowPlace);

      unsigned targetRow = 1;
      unsigned targetRowPlace = 1;
      (*dPlace)->SetDesDockPlace(targetRow, targetRowPlace);
      pDock->AddToolBar( *dPlace );
      dPlace++;
      while( dPlace != currentDockBars.end() )
	{
	  unsigned cBarRow = 0, cBarRowPlace = 0;
	  (*dPlace)->GetDesDockPlace(cBarRow, cBarRowPlace);
	  if( cBarRow > currentIntRow)
	    {
	      targetRow += 2;
	      targetRowPlace = 1;

	      currentIntRow = cBarRow;
	      currentIntRowPlace = cBarRowPlace;
	    }else
	    targetRowPlace += 2;

	  (*dPlace)->SetDesDockPlace(targetRow, targetRowPlace);
	  pDock->AddToolBar( *dPlace );
	  dPlace++;
	};

      cPlace++;
    };
}

void GxToolBarManager::MoveToolBar(GxToolBar *pToolBar, Window win, int xPix, int yPix)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarManager::MoveToolBar moving toolbar to window: " << win << endl;
#endif //LIBGX_DEBUG_BUILD

  if(!pToolBar) return; //an error
  if(!pFDStart && !pDStart) return; //we want to guarentee to the below that one or the other is valid
  //we can move from a dock to a different dock
  //from a dock to a different place within the same dock
  //from a dock to a unknown window (create a new floating dock)
  //from one floating dock to annother floating dock.

  //find out who the window win belogs to. search through our
  //list of docks for the correct one
  GxToolBarDock *pDestDock = 0;
  GxToolBarFloatingDock *pDestFloatingDock = 0;

  SelectDockByWindow(win, &pDestDock, &pDestFloatingDock);

#ifdef LIBGX_DEBUG_BUILD
  if(!pDestDock && !pDestFloatingDock)
    cout << "GxToolBarManager::MoveToolBar moving toolbar to unknown window" << endl;
#endif //LIBGX_DEBUG_BUILD

  // ********** first remove pToolBar from whatever dock it is in. *****************
  if(pFDStart) //we are moving from a floating dock.
    {
      //we do a slight performance optimization and bug fix here. if we are moving from a floating
      //dock to the same floating dock _and_ it has only one toolbar (the one we are moving), dont't
      //do anything at all.
      list<GxToolBarFloatingDock*>::iterator fdPlace;
      if( !PresentFloatingDock(pToolBar, fdPlace) ) return; //an error.
      GxToolBarFloatingDock* pFromDock = *fdPlace;
      if(!pFromDock) return; //an error.
      pFromDock->RemoveToolBar(pToolBar);
      if( !pFromDock->DockUsed() )
	{
	  if( pDestFloatingDock == pFromDock )
	    {
	      //add the toolbar back, and return;
	      pFromDock->AddToolBar(pToolBar);
	      return;
	    };
	  delete pFromDock;
	  pFromDock = 0;
	  floatingDockList.erase(fdPlace);
	};
    }else //pDStart must be valid
      {
	list<GxToolBarDock*>::iterator dockPlace;
	if( !PresentDock(pToolBar, dockPlace) ) return; //an error.
	pDStart->RemoveToolBar(pToolBar);
      };

  // ********** next add pToolBar to whatever dock it should be moved into *****************

  if(!pDestDock && !pDestFloatingDock)
    { //we are moving to a new floating dock (that we must create).
      AddToolBarToNewFloatingDock(pToolBar);
      return;
    };

  //if we are here we know that either pDestDock or pDestFloatingDock is valid (i.e. non-null)
  GxToolBarCplxDockCore *pFD = pDestDock;
  if(!pFD) pFD = pDestFloatingDock;

  unsigned destRow = 0, destRowPlace = 0;
  pFD->GetPhantomPlace(GX_BORDER_WD, GX_BORDER_WD, xPix, yPix, destRow, destRowPlace);
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarManager::MoveToolBar got final dest dock coords: " << destRow << "," << destRowPlace << endl;
#endif //LIBGX_DEBUG_BUILD

  pToolBar->SetDesDockPlace(destRow,destRowPlace);
  pFD->AddToolBar(pToolBar);
}

bool GxToolBarManager::PresentDock(GxToolBar *pToolBar, list<GxToolBarDock*>::iterator &objToRemove)
{
  list<GxToolBarDock*>::iterator cPlace = dockList.begin();
  list<GxToolBarDock*>::iterator cEnd = dockList.end();
  while(cPlace != cEnd)
    {
      if( (*cPlace)->ToolBarUsed(pToolBar) )
	{
	  objToRemove = cPlace;
	  return true;
	};
      cPlace++;
    };

  objToRemove = dockList.end();
  return false;
}

bool GxToolBarManager::PresentFloatingDock(GxToolBar *pToolBar, list<GxToolBarFloatingDock*>::iterator &objToRemove)
{
  list<GxToolBarFloatingDock*>::iterator cPlace = floatingDockList.begin();
  list<GxToolBarFloatingDock*>::iterator cEnd = floatingDockList.end();
  while(cPlace != cEnd)
    {
      if( (*cPlace)->ToolBarUsed(pToolBar) )
	{
	  objToRemove = cPlace;
	  return true;
	};
      cPlace++;
    };

  objToRemove = floatingDockList.end();
  return false;
}

bool GxToolBarManager::SelectDockByWindow(Window win, GxToolBarDock **pDestDock,
					  GxToolBarFloatingDock **pDestFloatingDock)
{
  *pDestDock = 0;
  *pDestFloatingDock = 0;

  list<GxToolBarDock*>::iterator dPlace = dockList.begin();
  list<GxToolBarDock*>::iterator dEnd = dockList.end();
  while( dPlace != dEnd )
    {
      GxToolBarDock *pDock = *dPlace;
      if( pDock->WindowInDock(win) )
	{
	  *pDestDock = pDock;
#ifdef LIBGX_DEBUG_BUILD
	  //cout << "found destination dock" << endl;
#endif //LIBGX_DEBUG_BUILD
	  return true;
	};
      dPlace++;
    };
  
  //we have returned already if we found a regular dock match
  list<GxToolBarFloatingDock*>::iterator cPlace = floatingDockList.begin();
  list<GxToolBarFloatingDock*>::iterator cEnd = floatingDockList.end();
  while( cPlace != cEnd )
    {
      GxToolBarFloatingDock *pFDock = *cPlace;
      if( pFDock->WindowInDock(win) )
	{
	  *pDestFloatingDock = pFDock;
#ifdef LIBGX_DEBUG_BUILD
	  //cout << "found destination floating dock" << endl;
#endif //LIBGX_DEBUG_BUILD
	  return true;
	};
      cPlace++;
    };

  return false;
}

void GxToolBarManager::AddToolBarToNewFloatingDock(GxToolBar *pToolBar)
{
  GxToolBarFloatingDock *pWin = new GxToolBarFloatingDock(pDisp, *this);
  floatingDockList.push_front(pWin);
  pWin->AddToolBar(pToolBar);
  pWin->Place();
  pWin->Create();
  pWin->Display();
}

void GxToolBarManager::PossibleEnterDock(Window win)
{
  GxToolBarDock *pDestDock = 0;
  GxToolBarFloatingDock *pDestFloatingDock = 0;
  if( !SelectDockByWindow(win, &pDestDock, &pDestFloatingDock) ) return;

#ifdef LIBGX_DEBUG_BUILD
  cout << "entered dock: " << win << endl;
#endif //LIBGX_DEBUG_BUILD

  if( pDestDock )
    if( pDestDock == pActiveDock) return;

  if(pDestFloatingDock)
    if( pDestFloatingDock == pActiveDock) return;

  bool outlineWasDrawn = false;
  if(outlineDrawn)
    {
      outlineWasDrawn = true;
      EraseToolBarOutline();
    };

  if(pDestDock)
    toolBarVertical = pDestDock->Vertical();
  else
    toolBarVertical = pDestFloatingDock->Vertical();
  
  if(outlineWasDrawn)
    DrawToolBarOutline(lastX, lastY);

  if( pActiveDock ) //this I think is a problem... how can we enter a dock without leaving another? ?via xwarp?
    LeaveActiveDock();

  //hackish. (the places are unsigned, so we set some invalid large number that forces a refresh on the next motion event)
  phantomDockRow  = -2;
  phantomRowPlace = -2;

  if(pDestDock)
    pActiveDock = pDestDock;
  else
    pActiveDock = pDestFloatingDock;
}

void GxToolBarManager::LeaveActiveDock(void)
{
#ifdef LIBGX_DEBUG_BUILD
  assert(pActiveDock);
  cout << "GxToolBarManager::LeaveActiveDock" << endl;
#endif //LIBGX_DEBUG_BUILD
  bool outlineWasDrawn = false;
  if(outlineDrawn)
    {
      outlineWasDrawn = true;
      EraseToolBarOutline();
    };

  pActiveDock->HidePhantom();
  pActiveDock = 0;

  if(outlineWasDrawn)
     DrawToolBarOutline(lastX, lastY); 
}

void GxToolBarManager::DrawToolBarOutline(int x, int y)
{
  //cout << "GxToolBarManager::DrawToolBarOutline" << endl;
  EraseToolBarOutline();

  DoOutlineDraw(x,y);

  lastX = x;
  lastY = y;
  outlineDrawn = true;
}

void GxToolBarManager::EraseToolBarOutline(void)
{
  if( !outlineDrawn ) return;

  DoOutlineDraw(lastX, lastY);
  outlineDrawn = false;
}

void GxToolBarManager::DoOutlineDraw(int x, int y) const
{
  //hack. this is broken if we have different gc's across the dipslay.
  //we probably must drag a window.
  const GxDisplayInfo & rDInfo = pDisp->GetDisplayInfo(); //hackish
  GxVolatileData &rVData = pDisp->GetVolatileData();
  XGCValues gcValues;

  unsigned tbWidth = 0;
  unsigned tbHeight = 0;
  if(toolBarVertical)
    {
      tbWidth = GX_TOOLBAR_BUTTON_SIZE;
      tbHeight = toolBarLength;
    }else
      {
	tbWidth = toolBarLength;
	tbHeight = GX_TOOLBAR_BUTTON_SIZE;
      };
  

  gcValues.function = GXxor;
  gcValues.subwindow_mode = IncludeInferiors;
  gcValues.foreground = rDInfo.whitePix;
  XChangeGC(rDInfo.display, rVData.borderGC, (GCFunction | GCSubwindowMode | GCForeground), &gcValues);

  XDrawRectangle(rDInfo.display, rDInfo.rootWin, rVData.borderGC, x-tbWidth/2,y-tbHeight/2, tbWidth,tbHeight);

  gcValues.function = GXcopy;
  gcValues.subwindow_mode = ClipByChildren;
  gcValues.foreground = rDInfo.whitePix;
  XChangeGC(rDInfo.display, rVData.borderGC, (GCFunction | GCSubwindowMode | GCForeground), &gcValues);
}
