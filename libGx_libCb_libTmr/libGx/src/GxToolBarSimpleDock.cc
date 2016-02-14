#include <libGx/GxToolBarSimpleDock.hh>

#include <libGx/GxToolBarButton.hh>

#include "GxDefines.hh"

GxToolBarSimpleDock::GxToolBarSimpleDock(GxRealOwner *pOwner) :
  GxOwnerWin(pOwner), pToolBar(0)
{
  vertical = false;
}

GxToolBarSimpleDock::~GxToolBarSimpleDock(void)
{
  ClearButtonWinList();
}

void GxToolBarSimpleDock::SetToolBar(GxToolBar* pCBar)
{
  ClearButtonWinList();

  pToolBar = pCBar;
  pToolBar->AllocateButtonWindows(this, buttonWinList);
  if( Created() )
    CreateChildren();
}

UINT GxToolBarSimpleDock::GetDesiredWidth(void) const
{
  if(vertical)
    return GX_TOOLBAR_BUTTON_SIZE + 2*GX_BORDER_WD + 2;
  else
    {
      int tX = GX_BORDER_WD+1;
      std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
      std::list<GxWinArea*>::const_iterator cEnd = childList.end();
      while(cPlace != cEnd)
	{
	  tX += GX_TOOLBAR_BUTTON_SIZE + 1;
	  cPlace++;
	};

      return tX + GX_BORDER_WD;
    };
}

UINT GxToolBarSimpleDock::GetDesiredHeight(void) const
{
  if(vertical)
    {
      //sum the heights of my children, they should be GX_TOOLBAR_BUTTON_SIZE + 6 pixels square.
      //we start placing children from 3 pixels down. buttons have a one pix
      //gap between them
      int tY = GX_BORDER_WD+1;
      std::list<GxWinArea*>::const_iterator cPlace = childList.begin();
      std::list<GxWinArea*>::const_iterator cEnd = childList.end();
      while(cPlace != cEnd)
	{
	  tY += GX_TOOLBAR_BUTTON_SIZE + 1;
	  cPlace++;
	};
      //for the last gap we have already added one pixel width, now just add
      //two more to get the three pixel border
      return tY+GX_BORDER_WD;
    }else
      return GX_TOOLBAR_BUTTON_SIZE + 2*GX_BORDER_WD + 2;
}

void GxToolBarSimpleDock::SetVertical(bool nVert)
{
  if(nVert == vertical)
    return;

  vertical = nVert;
}

void GxToolBarSimpleDock::PlaceChildren(void)
{
  if(vertical)
    {
      int cY = GX_BORDER_WD+1;
      std::list<GxWinArea*>::iterator cPlace = childList.begin();
      std::list<GxWinArea*>::iterator cEnd = childList.end();
      while(cPlace != cEnd)
	{
	  (*cPlace)->Move(GX_BORDER_WD+1,cY);
	  (*cPlace)->Resize(GX_TOOLBAR_BUTTON_SIZE, GX_TOOLBAR_BUTTON_SIZE);
	  cY += GX_TOOLBAR_BUTTON_SIZE;
	  cY += 1;

	  cPlace++;
	};

      return;
    };

  int cX = GX_BORDER_WD+1;
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->Move(cX, GX_BORDER_WD+1);
      (*cPlace)->Resize(GX_TOOLBAR_BUTTON_SIZE, GX_TOOLBAR_BUTTON_SIZE);
      cX += GX_TOOLBAR_BUTTON_SIZE;
      cX += 1;

      cPlace++;
    };
}

void GxToolBarSimpleDock::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      {
	Draw3dBorder(0,0, width,height, true);
      };
}

void GxToolBarSimpleDock::GetWindowData(XSetWindowAttributes &winAttributes,
				ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}

void GxToolBarSimpleDock::ClearButtonWinList(void)
{
  while( !buttonWinList.empty() )
    {
      GxToolBarButtonWin *pBWin = buttonWinList.front();
      delete pBWin;
      pBWin = 0;
      buttonWinList.pop_front();
    };
}
