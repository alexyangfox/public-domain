#ifdef LIBGX_DEBUG_BUILD
#include <assert.h>
#endif //LIBGX_DEBUG_BUILD

#include <libGx/GxFrame.hh>

#include "GxDefines.hh"

GxFrame::GxFrame(GxRealOwner *pOwner) :
  GxOwnerWin(pOwner)
{}

GxFrame::~GxFrame(void)
{}

UINT GxFrame::GetDesiredWidth(void) const
{
  if( childList.empty() ) return width;

  GxWinArea *pFirstArea = childList.front();
  return pFirstArea->GetDesiredWidth() + pFirstArea->RBorder() + pFirstArea->LBorder() + 2*GX_BORDER_WD;
}

UINT GxFrame::GetDesiredHeight(void) const
{
  if( childList.empty() ) return height;

  GxWinArea *pFirstArea = childList.front();
  return pFirstArea->GetDesiredHeight() + pFirstArea->TBorder() + pFirstArea->BBorder() + 2*GX_BORDER_WD;
}

void GxFrame::PlaceChildren(void)
{
  int lX = GX_BORDER_WD;
  int rX = (int)width - GX_BORDER_WD;
  int tY = GX_BORDER_WD;
  int bY = (int)height - GX_BORDER_WD;

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

void GxFrame::HandleEvent(const XEvent &rEvent)
{
  if(rEvent.type == Expose)
    if(rEvent.xexpose.count == 0)
      Draw3dBorder(0,0, width,height, true);
}

void GxFrame::GetWindowData(XSetWindowAttributes &winAttributes,
			    ULINT &valueMask)
{
  winAttributes.event_mask = ExposureMask;
  valueMask |= CWEventMask;
}
