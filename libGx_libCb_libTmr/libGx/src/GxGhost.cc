#include <libGx/GxGhost.hh>

GxGhost::GxGhost(GxRealOwner *pOwner) :
  GxRealOwner(),
  GxWinArea(pOwner)
{}

GxGhost::~GxGhost(void)
{}

void GxGhost::Place(int &lX, int &rX, int &tY, int &bY)
{
  GxWinArea::Place(lX, rX, tY, bY);
  this->PlaceChildren();
}

void GxGhost::PlaceChildren(void)
{
  int lX = x;
  int rX = x + (int)width;
  int tY = y;
  int bY = y + (int)height;

  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      //note each iteration through lX rX tY bY are posibly modified
      //by each child's GxGeomControl ?(if it has one)?
      (*cPlace)->Place(lX, rX, tY, bY);
      cPlace++;
    };
}

void GxGhost::Create(void)
{
  CreateChildren();
}

void GxGhost::Display(void)
{
  DisplayChildren();
}

void GxGhost::Hide(void)
{
  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
      (*cPlace)->Hide();
      cPlace++;
    };
}

bool GxGhost::CoordInside(int xCoord, int yCoord)
{
  if( (xCoord > x) && (xCoord < (x + (int)width))  &&
      (yCoord > y) && (yCoord < (y + (int)height)) )
    return true;
  else
    return false;
}

int GxGhost::AdjustX(int xCoord)
{
  return (x + xCoord);
}

int GxGhost::AdjustY(int yCoord)
{
  return (y + yCoord);
}

UINT GxGhost::GetDesiredWidth(void) const
{
  if( childList.empty() ) return width;

  GxWinArea *pFirstArea = childList.front();
  return pFirstArea->GetDesiredWidth() + pFirstArea->RBorder() + pFirstArea->LBorder();
}

UINT GxGhost::GetDesiredHeight(void) const
{
  if( childList.empty() ) return height;

  GxWinArea *pFirstArea = childList.front();
  return pFirstArea->GetDesiredHeight() + pFirstArea->TBorder() + pFirstArea->BBorder();
}

Window GxGhost::GetClosestXWin(void)
{
  return pWinAreaOwner->GetClosestXWin();
}

GxMapHolder* GxGhost::GetClosestMapHolder(void)
{
  return pWinAreaOwner->GetClosestMapHolder();
}

void GxGhost::UnManageWindow(Window winID)
{
  pWinAreaOwner->UnManageWindow(winID);
}

void GxGhost::MoveFocusToChild(GxWinArea *pChild, Time eventTime)
{
  pWinAreaOwner->MoveFocusToChild(pChild, eventTime);
}

GxDisplayInfo& GxGhost::GetDisplayInfo(void)
{
  return pWinAreaOwner->GetDisplayInfo();
}

GxVolatileData& GxGhost::GetVolatileData(void)
{
  return pWinAreaOwner->GetVolatileData();
}
