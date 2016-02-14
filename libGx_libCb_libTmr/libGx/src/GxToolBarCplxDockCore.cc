#ifdef LIBGX_DEBUG_BUILD
#include <assert.h>
#include <iostream>
#endif //LIBGX_DEBUG_BUILD

#include <libGx/GxToolBarCplxDockCore.hh>

#include <libGx/GxToolBarManager.hh>

#include "GxDefines.hh"

using namespace std;

GxToolBarCplxDockCore::GxToolBarCplxDockCore(GxRealOwner *pOwner, GxToolBarManager &rTBManager) :
  rManager(rTBManager), pSelectedBar(0), vertical(false),
  toolbarMenu(pOwner), hideItem(&toolbarMenu)
{}

GxToolBarCplxDockCore::~GxToolBarCplxDockCore(void)
{
  list<ToolBarRow>::iterator cPlace = rowList.begin();
  while(cPlace != rowList.end() )
    {
      list<ToolBarHolder>::iterator dPlace = (*cPlace).barHolderList.begin();
      while( dPlace != (*cPlace).barHolderList.end() )
	{
	  (*dPlace).FreeButtonWindows(); //delete the button windows if they exits (fine to call for phantoms too)
	  dPlace++;
	};
      
      cPlace++;
    };
}

bool GxToolBarCplxDockCore::Vertical(void) const
{
  return vertical;
}

bool GxToolBarCplxDockCore::ToolBarUsed(GxToolBar *pToolBar) const
{
  list<ToolBarRow>::const_iterator cPlace = rowList.begin();
  while( cPlace != rowList.end() )
    {
      list<ToolBarHolder>::const_iterator dPlace = (*cPlace).barHolderList.begin();
      while(dPlace != (*cPlace).barHolderList.end() )
	{
	  if( (*dPlace).pToolBar == pToolBar ) return true;
	  dPlace++;
	};

      cPlace++;
    };

  return false;
}

bool GxToolBarCplxDockCore::WindowInDock(Window win) const
{
  if( win == GetDockBaseWindow() )  return true;

 list<ToolBarRow>::const_iterator cPlace = rowList.begin();
  while( cPlace != rowList.end() )
    {
      list<ToolBarHolder>::const_iterator dPlace = (*cPlace).barHolderList.begin();
      while(dPlace != (*cPlace).barHolderList.end() )
	{
	  list<GxToolBarButtonWin*>::const_iterator ePlace = (*dPlace).tbWinList.begin();
	  while(ePlace != (*dPlace).tbWinList.end())
	    {
	      if( (*ePlace)->GetWindow() == win ) return true;
	      ePlace++;
	    };
	  dPlace++;
	};
      

      cPlace++;
    };

  return false;
}

void GxToolBarCplxDockCore::GetPhantomPlace(int startX, int startY, int pointerX, int pointerY,
					    unsigned &rDockRow, unsigned &rRowPlace) const
{
  if( rowList.empty() )
    {
      rDockRow = 0;
      rRowPlace = 0;
      return;
    };

  //first get the row. if we select an 'even' row we know the rowPlace will be 0.
  int rowCoord = 0;
  if(vertical)
    {
      if(startX > pointerX )
	rowCoord = 0;
      else
	rowCoord = pointerX - startX;
    }else
    {
      if( startY > pointerY )
	rowCoord = 0;
      else
	rowCoord = pointerY - startY;
    };

  int baseNum = 2*(rowCoord/(GX_TOOLBAR_BUTTON_SIZE+GX_TOOLBAR_ROW_GAP)) +1; //*2 and +1 to keep it odd
  int rowSubPlace = rowCoord%(GX_TOOLBAR_BUTTON_SIZE+GX_TOOLBAR_ROW_GAP);
      
  int rowOffset = 0;
  if(rowSubPlace < (int)(GX_TOOLBAR_BUTTON_SIZE+GX_TOOLBAR_ROW_GAP)/4 ) // < 1/4 way across the row
    rowOffset = -1;
  else
    if(rowSubPlace > (int)(3*(GX_TOOLBAR_BUTTON_SIZE+GX_TOOLBAR_ROW_GAP))/4) // > 3/4 across the row
      rowOffset = 1;

  rDockRow = baseNum + rowOffset;

#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarCplxDockCore::GetPhantomPlace got row: " << rDockRow << endl;
#endif //LIBGX_DEBUG_BUILD

  if( !(rDockRow%2) ) //we are placing on an independant row.
    {
      rRowPlace = 0;
      return;
    };

  int rowPlaceCoord = 0;
  if(vertical)
    rowPlaceCoord = pointerY - startY;
  else
    rowPlaceCoord = pointerX - startX;

  bool rowFound = false;

  unsigned cRow = 1;
  list<ToolBarRow>::const_iterator cPlace = rowList.begin();
  while(cPlace != rowList.end() )
    {
      //get the referenced row
      if( cRow == rDockRow ) break;

      cRow += 2;
      cPlace++;
    };
  
  if( cPlace == rowList.end() )
    {
      rRowPlace = 0;
      return;
    };

  //cPlace must be derefereneable
  unsigned tbRowPlace = 0;
  unsigned tbStartPixPlace = 0;
  list<ToolBarHolder>::const_iterator dPlace = (*cPlace).barHolderList.begin();
  while( dPlace != (*cPlace).barHolderList.end() )
    {
      unsigned tbLength = 0;
      if( (*dPlace).pToolBar )
	tbLength = (*dPlace).pToolBar->BarLength();
      else
	tbLength = (*dPlace).phantomLength;

      if( rowPlaceCoord > tbStartPixPlace + GX_TOOLBAR_BUTTON_SIZE/4 &&
	  rowPlaceCoord < tbStartPixPlace + tbLength - GX_TOOLBAR_BUTTON_SIZE/4)
	{
	  rRowPlace = tbRowPlace;
	  return;
	};

      tbRowPlace += 2;
      *dPlace++;
    };

  rRowPlace = tbRowPlace;
  return;
}

void GxToolBarCplxDockCore::RemovePhantomBar(void)
{
  RemoveBar(0);
}

bool GxToolBarCplxDockCore::SetPhantomVisible(unsigned dockRow, unsigned rowPlace, unsigned phantomLength)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarCplxDockCore::SetPhantomVisible" << endl;
#endif //LIBGX_DEBUG_BUILD

  unsigned oldNumRows = rowList.size();

  RemovePhantomBar();
  AddBarLL(0, dockRow, rowPlace, phantomLength);

  unsigned newNumRows = rowList.size();

  ResizeAndPlaceDock();
  EraseDock();
  DrawDock();

  if(oldNumRows != newNumRows)
    return true;
  else
    return false;
}

void GxToolBarCplxDockCore::HidePhantom(void)
{
  RemovePhantomBar();
  ResizeAndPlaceDock();
  EraseDock();
}

bool GxToolBarCplxDockCore::RemoveBar(const GxToolBar *pToolBar)
{
  bool barFound = false;
  list<ToolBarRow>::iterator cPlace = rowList.begin();
  while(cPlace != rowList.end() )
    {
      list<ToolBarHolder>::iterator dPlace = (*cPlace).barHolderList.begin();
      while( dPlace != (*cPlace).barHolderList.end() )
	{
	  if( (*dPlace).pToolBar == pToolBar ) //this works if we want to remove a phantom (pToolBar is NULL) or if pToolBar is valid
	    {
	      //delete the button windows if they exits
	      (*dPlace).FreeButtonWindows();
	      dPlace = (*cPlace).barHolderList.erase(dPlace);
	      barFound = true;
	    }else
	    dPlace++;
	};
      
      if( (*cPlace).barHolderList.empty() )
	cPlace = rowList.erase(cPlace);
      else
	cPlace++;
    };

  if( barFound )
    NumberToolBars();

  return barFound;
}

GxToolBarCplxDockCore::ToolBarHolder& GxToolBarCplxDockCore::AddBar(GxToolBar &rToolBar)
{
  unsigned junk1 = 0, junk2 = 0, junk3 = 0;
  return AddBarLL(&rToolBar, junk1, junk2, junk3);
}

GxToolBarCplxDockCore::ToolBarHolder& GxToolBarCplxDockCore::AddBarLL(GxToolBar *pToolBar, unsigned phantomRow, unsigned phantomRowPlace, unsigned phantomLen)
{
  ToolBarHolder tbHolder;
  unsigned desRow = 0, desRowPlace = 0;

  if(pToolBar)
    {
      tbHolder.pToolBar = pToolBar;
      
      pToolBar->AllocateButtonWindows( &GetToolBarWinOwner(), tbHolder.tbWinList);
      pToolBar->GetDesDockPlace(desRow, desRowPlace);
    }else
    {
      desRow = phantomRow;
      desRowPlace = phantomRowPlace;

      tbHolder.phantomLength = phantomLen;
    };

  if( rowList.empty() || desRow == 0 )
    {
      ToolBarRow tbRow;
      tbRow.barHolderList.push_back(tbHolder);
      rowList.push_front(tbRow);
      
      ToolBarHolder &rHolder = rowList.front().barHolderList.back();
      
      NumberToolBars();
      return rHolder;
    };
 
  unsigned cRowNum = 1;
  list<ToolBarRow>::iterator cPlace = rowList.begin();

  while( cPlace != rowList.end() )
    {
      if(desRow == cRowNum)
	{
	  unsigned cRowPlace = 1;
	  //figure out where to place it on this row.
	  list<ToolBarHolder>::iterator dPlace = (*cPlace).barHolderList.begin();
	  while( dPlace != (*cPlace).barHolderList.end() )
	    {
	      if( desRowPlace == cRowPlace || desRowPlace == cRowPlace-1 )
		{
		  dPlace = (*cPlace).barHolderList.insert(dPlace, tbHolder);
		  NumberToolBars();
		  return *dPlace;
		};
	      cRowPlace += 2;
	      dPlace++;
	    }
	  //put it at the end on the current toolbar row.
	  (*cPlace).barHolderList.push_back(tbHolder);
	  NumberToolBars();
	  return (*cPlace).barHolderList.back();
	}else
	if( desRow == cRowNum -1) //on an even row before me! Its a new row.
	  {
	    ToolBarRow tbRow;
	    tbRow.barHolderList.push_back(tbHolder);
	    cPlace = rowList.insert(cPlace, tbRow);
	    NumberToolBars();
	    return (*cPlace).barHolderList.front();
	  };

      cRowNum += 2;
      cPlace++;
    }

  //must be on a new row at the end
  ToolBarRow tbRow;
  tbRow.barHolderList.push_back(tbHolder);
  rowList.push_back(tbRow);
  ToolBarHolder &rHolder = rowList.back().barHolderList.back();
  
  NumberToolBars();
  return rHolder;
}

void GxToolBarCplxDockCore::SetAllToolBarDisplayState(bool newState)
{
  list<ToolBarRow>::iterator cPlace = rowList.begin();
  while(cPlace != rowList.end() )
    {
      list<ToolBarHolder>::iterator dPlace = (*cPlace).barHolderList.begin();
      while( dPlace != (*cPlace).barHolderList.end() )
	{
	  ToolBarHolder& rHolder = *dPlace;
	  if( rHolder.pToolBar )
	    rHolder.pToolBar->displayChangeCB(newState);
	  dPlace++;
	};

      cPlace++;
    };
}

//remember the naming convention we stated in the header
void GxToolBarCplxDockCore::GetDesiredDockSize(UINT &rDesiredWidth, UINT &rDesiredHeight) const
{
  unsigned numRows = 0; //the *actual* row/column we are on
  unsigned longestRowWidth = 0;

  list<ToolBarRow>::const_iterator cPlace = rowList.begin();
  while(cPlace != rowList.end() )
    {
      unsigned currentRowWidth = 0; //does not take into account GX_TOOLBAR_GAPs until the end of the row
      
      unsigned numBarsOnRow = 0;
      list<ToolBarHolder>::const_iterator dPlace = (*cPlace).barHolderList.begin();
      while( dPlace != (*cPlace).barHolderList.end() )
	{

	  if( (*dPlace).pToolBar )
	    {
	      currentRowWidth += (*dPlace).pToolBar->BarLength();
	    }else
	    currentRowWidth += (*dPlace).phantomLength;
	  
	  numBarsOnRow++;
	  dPlace++;
	};

      if( numBarsOnRow > 1 )
	currentRowWidth += numBarsOnRow*GX_TOOLBAR_GAP;

      if(currentRowWidth > longestRowWidth)
	longestRowWidth = currentRowWidth;
      
      numRows++;
      cPlace++;
    };

  rDesiredWidth = longestRowWidth;
  if(numRows == 0)
    rDesiredHeight = 0;
  else
    rDesiredHeight = numRows*(GX_TOOLBAR_BUTTON_SIZE) + (numRows-1)*GX_TOOLBAR_ROW_GAP;
}


void GxToolBarCplxDockCore::NumberToolBars(void)
{
  /*
    valid row numbers start at 1 and increment by 2
    valid row positions start at 1 and increment by 2

    if a toolbar is added on an even row number, it is placed on a new row.
    if a toolbar is added on an even row position, it is placed between tool bars (or before tb 1)
  */

  unsigned cRowNum = 1;

  list<ToolBarRow>::iterator cPlace = rowList.begin();
  while( cPlace != rowList.end() )
    {
      ToolBarRow &rRow = *cPlace;

      bool phantomRow = true;

      unsigned cRowPlace = 1;
      list<ToolBarHolder>::iterator dPlace = rRow.barHolderList.begin();
      while( dPlace != rRow.barHolderList.end() )
	{
	  if( (*dPlace).pToolBar ) //don't number phantoms
	    {
	      (*dPlace).pToolBar->SetDesDockPlace(cRowNum, cRowPlace);
	      cRowPlace += 2;
	      phantomRow = false;
	    };

	  dPlace++;
	};
      
      if( !phantomRow ) //don't count rows with only a phantom bar
	cRowNum += 2;

      cPlace++;
    };
}

void GxToolBarCplxDockCore::PlaceDockElements(int startX, int startY)
{
  CbTwoFO<ToolBarDetails&, int> pdeFO( CbTwoMember<ToolBarDetails&, int, GxToolBarCplxDockCore>
				       (this, &GxToolBarCplxDockCore::PlaceDockElement) );
  ElementPosCalc(startX, startY, pdeFO, 0);
}

void GxToolBarCplxDockCore::PlaceDockElement(ToolBarDetails &rDetails, int junk)
{
  if( !rDetails.rHolder.pToolBar ) return; //a phantom bar... we can't place this.

  //a little ugly....
  rDetails.rHolder.pToolBar->PlaceButtons(rDetails.rHolder.tbWinList, rDetails.vertical, rDetails.cX, rDetails.cY);
}

void GxToolBarCplxDockCore::DrawDock(const GxDisplayInfo &rDInfo, GxVolatileData &rVData,
				     Window xWin, int startX, int startY)
{
  //we only draw the phantom
  XSetForeground(rDInfo.display, rVData.borderGC, rDInfo.blackPix);
  DrawInfo lDrawInfo(rDInfo, rVData, xWin);

  CbTwoFO<ToolBarDetails&, GxToolBarCplxDockCore::DrawInfo> ddeFO( CbTwoMember<ToolBarDetails&, GxToolBarCplxDockCore::DrawInfo, GxToolBarCplxDockCore>
								   (this, &GxToolBarCplxDockCore::DrawDockElement) );

  ElementPosCalc(startX, startY, ddeFO, lDrawInfo);
}

void GxToolBarCplxDockCore::DrawDockElement(ToolBarDetails& rDetails, GxToolBarCplxDockCore::DrawInfo drawInfo)
{

  if( rDetails.rHolder.pToolBar ) return; //we only draw phantoms

  unsigned cX = rDetails.cX;
  unsigned cY = rDetails.cY;

  unsigned toolBarLength = rDetails.rHolder.phantomLength;
  unsigned tbBorderWidth = 0;
  unsigned tbBorderHeight = 0;
  //set the physical sized
  if(rDetails.vertical)
    {
      tbBorderWidth = GX_TOOLBAR_BUTTON_SIZE;
      tbBorderHeight = toolBarLength;
    }else
      {
	tbBorderWidth = toolBarLength;
	tbBorderHeight = GX_TOOLBAR_BUTTON_SIZE;
      };

  //delete this
  cX -= 1;
  cY -= 1;
  tbBorderWidth += 1;
  tbBorderHeight += 1;
  
  //cout << "drawing phantom border" << endl;
  XDrawRectangle(drawInfo.rInfo.display, drawInfo.xWin, drawInfo.rVData.borderGC, cX, cY, tbBorderWidth, tbBorderHeight);
}

GxToolBar* GxToolBarCplxDockCore::SelectToolBar(int startX, int startY, int tXClick, int tYClick)
{
  pSelectedBar = 0;

  SelectInfo selInfo;
  selInfo.xClick = tXClick;
  selInfo.yClick = tYClick;

  CbTwoFO<ToolBarDetails&, GxToolBarCplxDockCore::SelectInfo> stbeFO( CbTwoMember<ToolBarDetails&, GxToolBarCplxDockCore::SelectInfo, GxToolBarCplxDockCore>
								      (this, &GxToolBarCplxDockCore::SelectToolBarElement) );
  ElementPosCalc(startX, startY, stbeFO, selInfo);

  GxToolBar *pLocalSelected = pSelectedBar;
  pSelectedBar = 0;
  return pLocalSelected;
}

void GxToolBarCplxDockCore::SelectToolBarElement(ToolBarDetails &rDetails, SelectInfo selInfo)
{
  if( pSelectedBar ) return; //no changes
  if( !rDetails.rHolder.pToolBar ) return; //a phantom bar. we can't select this.

  int cX = rDetails.cX;
  int cY = rDetails.cY;

  unsigned toolBarLength = rDetails.rHolder.pToolBar->BarLength();
  unsigned tbBorderWidth = 0;
  unsigned tbBorderHeight = 0;
  //set the physical sized
  if(rDetails.vertical)
    {
      tbBorderWidth = GX_TOOLBAR_BUTTON_SIZE;
      tbBorderHeight = toolBarLength;
    }else
      {
	tbBorderWidth = toolBarLength;
	tbBorderHeight = GX_TOOLBAR_BUTTON_SIZE;
      };

  if( selInfo.xClick > cX && selInfo.xClick < cX + tbBorderWidth && selInfo.yClick > cY && selInfo.yClick < cY + tbBorderHeight)
    pSelectedBar = rDetails.rHolder.pToolBar;
}


template<class T>
void GxToolBarCplxDockCore::ElementPosCalc(int startX, int startY, CbTwoFO<ToolBarDetails&, T> &rFO, T argTwo)
{
  //all calculations are done considering !vertical. only when we call the FO do we
  //compute the real x and y
  int cX = 0; 
  int cY = 0;
  
  //unsigned cRowPlace = 1;

  list<ToolBarRow>::iterator cPlace = rowList.begin();
  while( cPlace != rowList.end() )
    {
      list<ToolBarHolder>::iterator dPlace = (*cPlace).barHolderList.begin();
      while( dPlace != (*cPlace).barHolderList.end() )
	{
	  ToolBarDetails tbDetails(*dPlace);
	  if( vertical )
	    {
	      tbDetails.cX = startX + cY;
	      tbDetails.cY = startY + cX;
	    }else
	    {
	      tbDetails.cX = startX + cX;
	      tbDetails.cY = startY + cY;
	    };
	  tbDetails.vertical = vertical;

	  rFO(tbDetails, argTwo);

	  if( (*dPlace).pToolBar )
	    {
	      cX += (*dPlace).pToolBar->BarLength();
	    }else
	    cX += (*dPlace).phantomLength;

	  cX += GX_TOOLBAR_GAP;
	  dPlace++;
	};

      cX = 0;
      cY += GX_TOOLBAR_BUTTON_SIZE + GX_TOOLBAR_ROW_GAP;

      cPlace++;
    }
}


void GxToolBarCplxDockCore::DoToolBarMenu(int rootX, int rootY, GxToolBar *pToolBar)
{
  string labelStr( string("Show ") + pToolBar->Label() );
  hideItem.SetLabel( labelStr.c_str() );
  hideItem.Checked(true);
  hideItem.cb.Assign( CbOneMemberObj<bool, GxToolBarCplxDockCore, GxToolBar*>
		      (this, &GxToolBarCplxDockCore::HideToolBarMenuCB, pToolBar) );
  toolbarMenu.SetButtonHeldMode(true);
  toolbarMenu.Place();
  toolbarMenu.Create();
  toolbarMenu.Activate(rootX, rootY);
}

void GxToolBarCplxDockCore::HideToolBarMenuCB(bool arg, GxToolBar *pToolBar)
{
  //hackish in that the toolbarManager will have to re-figure out which dock to remove from
  rManager.HideToolBar(pToolBar);
}

// **************************
GxToolBarCplxDockCore::ToolBarHolder::ToolBarHolder(void) :
  pToolBar(0), phantomLength(0)
{}

GxToolBarCplxDockCore::ToolBarHolder::ToolBarHolder(const ToolBarHolder &rhs) :
  pToolBar( rhs.pToolBar ), phantomLength(rhs.phantomLength), tbWinList( rhs.tbWinList )
{}

GxToolBarCplxDockCore::ToolBarHolder::~ToolBarHolder(void)
{
  //we did not allocate the list contents, so we will not delete them
  //list<GxToolBarButtonWin*> tbWinList;
#ifdef LIBGX_DEBUG_BUILD
  if( !tbWinList.empty()  )
    cout << "GxToolBarCplxDockCore::ToolBarHolder::~ToolBarHolder warning: tbWinList is not empty"
	      << endl;
#endif //LIBGX_DEBUG_BUILD
}

const GxToolBarCplxDockCore::ToolBarHolder& GxToolBarCplxDockCore::ToolBarHolder::operator=(const ToolBarHolder &rhs)
{
  pToolBar = rhs.pToolBar;
  phantomLength = rhs.phantomLength;
  tbWinList = rhs.tbWinList;
  return *this;
}

void GxToolBarCplxDockCore::ToolBarHolder::CreateAndDisplayButtonWindows(void)
{
#ifdef LIBGX_DEBUG_BUILD
  cout << "GxToolBarCplxDockCore::ToolBarHolder::CreateAndDisplayButtonWindows" << endl;
#endif //LIBGX_DEBUG_BUILD

  list<GxToolBarButtonWin*>::iterator cPlace = tbWinList.begin();
  list<GxToolBarButtonWin*>::iterator cEnd = tbWinList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->Create();
      cPlace++;
    };

  cPlace = tbWinList.begin();
  //cEnd = tbWinList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->Display();
      cPlace++;
    };
}

void GxToolBarCplxDockCore::ToolBarHolder::FreeButtonWindows(void)
{
  while( !tbWinList.empty() )
    {
      GxToolBarButtonWin* pWin = tbWinList.back();
      tbWinList.pop_back();
      delete pWin;
      pWin = 0;
    };
}


// **************************

GxToolBarCplxDockCore::ToolBarRow::ToolBarRow(void)
{}

GxToolBarCplxDockCore::ToolBarRow::ToolBarRow(const ToolBarRow &rhs) :
  barHolderList(rhs.barHolderList)
{}

GxToolBarCplxDockCore::ToolBarRow::~ToolBarRow(void)
{}

GxToolBarCplxDockCore::ToolBarRow& GxToolBarCplxDockCore::ToolBarRow::operator=(const ToolBarRow &rhs)
{
  barHolderList = rhs.barHolderList;

  return *this;
}

// *****************************

GxToolBarCplxDockCore::ToolBarDetails::ToolBarDetails(ToolBarHolder &rTHolder) :
  rHolder(rTHolder)
{}

GxToolBarCplxDockCore::ToolBarDetails::ToolBarDetails(const ToolBarDetails &rhs) :
  rHolder(rhs.rHolder), cX(rhs.cX), cY(rhs.cY), vertical(rhs.vertical)
{}

GxToolBarCplxDockCore::ToolBarDetails::~ToolBarDetails(void)
{}

GxToolBarCplxDockCore::ToolBarDetails& GxToolBarCplxDockCore::ToolBarDetails::operator=(const GxToolBarCplxDockCore::ToolBarDetails &rhs)
{
  rHolder = rhs.rHolder;
  cX = rhs.cX;
  cY = rhs.cY;
  vertical = rhs.vertical;

  return *this;
}

// *****************************

GxToolBarCplxDockCore::DrawInfo::DrawInfo(const GxDisplayInfo &rTInfo, GxVolatileData &rTVData, Window tXWin) :
  rInfo(rTInfo), rVData(rTVData), xWin(tXWin)
{}
