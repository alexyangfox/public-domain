#ifdef LIBGX_DEBUG_BUILD
#include <assert.h>
#endif //LIBGX_DEBUG_BUILD

#include <libGx/GxCoreOwnerWin.hh>

GxCoreOwnerWin::~GxCoreOwnerWin(void)
{}

GxCoreOwnerWin::GxCoreOwnerWin(GxOwner *pOwner) :
  GxCoreWin(pOwner),
  GxRealOwner()
{}

void GxCoreOwnerWin::Resize(UINT tWidth, UINT tHeight)
{
  if(!Created())
    {
      width = tWidth;
      height = tHeight;
      //GxWinArea::Resize(tWidth, tHeight);
    }else
      {
	if(tWidth == width && tHeight == height)
	  return;
	
	width = tWidth;
	height = tHeight;
	XResizeWindow(dInfo.display, xWin, width, height);
	//hack?
	this->PlaceChildren();
      };
}

UINT GxCoreOwnerWin::GetDesiredWidth(void) const
{
  if( childList.empty() ) return width;

  GxWinArea *pFirstArea = childList.front();
  return pFirstArea->GetDesiredWidth() + pFirstArea->RBorder() + pFirstArea->LBorder();
}

UINT GxCoreOwnerWin::GetDesiredHeight(void) const
{
  if( childList.empty() ) return height;

  GxWinArea *pFirstArea = childList.front();
  return pFirstArea->GetDesiredHeight() + pFirstArea->TBorder() + pFirstArea->BBorder();
}

void GxCoreOwnerWin::Place(int &lX, int &rX, int &tY, int &bY)
{
  GxWinArea::Place(lX, rX, tY, bY);
  PlaceChildren();
}

void GxCoreOwnerWin::PlaceChildren(void)
{
  int lX = 0;
  int rX = (int)width;
  int tY = 0;
  int bY = (int)height;

  std::list<GxWinArea*>::iterator cPlace = childList.begin();
  std::list<GxWinArea*>::iterator cEnd = childList.end();
  while(cPlace != cEnd)
    {
#ifdef LIBGX_DEBUG_BUILD
      assert( (*cPlace) );
#endif //LIBGX_DEBUG_BUILD
      //note each iteration through lX rX tY bY are posibly modified
      //by each child's GxGeomControl ?(if it has one)?
      (*cPlace)->Place(lX, rX, tY, bY);
      cPlace++;
    };
}

void GxCoreOwnerWin::Create(void)
{
  //the order is critical
  GxCoreWin::Create();
  CreateChildren();
}

void GxCoreOwnerWin::Display(void)
{
  if(Created())
    {
      XMapWindow(dInfo.display, xWin);
      DisplayChildren();
      //XMapSubwindows(dInfo.display, xWin);
    };
}

Window GxCoreOwnerWin::GetClosestXWin(void)
{
  return xWin;
}

GxMapHolder* GxCoreOwnerWin::GetClosestMapHolder(void)
{
  return pWinAreaOwner->GetClosestMapHolder();
}

void GxCoreOwnerWin::UnManageWindow(Window winID)
{
  pWinAreaOwner->UnManageWindow(winID);
}

void GxCoreOwnerWin::MoveFocusToChild(GxWinArea *pChild, Time eventTime)
{
  pWinAreaOwner->MoveFocusToChild(pChild, eventTime);
}

GxDisplayInfo& GxCoreOwnerWin::GetDisplayInfo(void)
{
  return dInfo;
}

GxVolatileData& GxCoreOwnerWin::GetVolatileData(void)
{
  return vData;
}
