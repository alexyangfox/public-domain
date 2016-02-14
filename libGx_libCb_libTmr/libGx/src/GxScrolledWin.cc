#include <libGx/GxScrolledWin.hh>

#include "GxDefines.hh"

GxBaseScrolledWin::GxBaseScrolledWin(GxRealOwner *pOwner) :
  GxGhost(pOwner), hScrollBar(this), vScrollBar(this)
{
  pClipWin = NULL;

  vSpacing = 0;
  hSpacing = 0;

  desW = 100;
  desH = 100;
}

GxBaseScrolledWin::~GxBaseScrolledWin(void)
{}

void GxBaseScrolledWin::SetVSpacing(UINT newSpacing)
{
  vSpacing = newSpacing;
  PlaceChildren();
}

void GxBaseScrolledWin::SetHSpacing(UINT newSpacing)
{
  hSpacing = newSpacing;
  PlaceChildren();
}

UINT GxBaseScrolledWin::GetDesiredWidth(void) const
{
  return desW;
}

UINT GxBaseScrolledWin::GetDesiredHeight(void) const
{
  return desH;
}

void GxBaseScrolledWin::SetDesiredWidth(UINT newW)
{
  if( newW >= 80 )
  desW = newW;
}

void GxBaseScrolledWin::SetDesiredHeight(UINT newH)
{
  if( newH >= 80 )
    desH = newH;
}

void GxBaseScrolledWin::PlaceChildren(void)
{
  vScrollBar.Height(height - GX_SLIDER_WIDTH);
  vScrollBar.Move(x+width - GX_SLIDER_WIDTH, y);
  vScrollBar.PlaceChildren();

  hScrollBar.Width(width - GX_SLIDER_WIDTH);
  hScrollBar.Move(x, y+ height - GX_SLIDER_WIDTH);
  hScrollBar.PlaceChildren();

  if(pClipWin)
    {
      int rX = x + (width - GX_SLIDER_WIDTH - hSpacing);
      int bY = y+(height - GX_SLIDER_WIDTH - vSpacing);
      pClipWin->Place(x, rX, y, bY);
    };
}


GxAutoScrolledWin::GxAutoScrolledWin(GxRealOwner *pOwner) :
  GxBaseScrolledWin(pOwner), clipWin(this)
{
  pClipWin = &clipWin;
}

GxAutoScrolledWin::~GxAutoScrolledWin(void)
{}

GxOwnerWin &GxAutoScrolledWin::GetClipWindow(void)
{
  return clipWin;
}


GxAppScrolledWin::GxAppScrolledWin(GxRealOwner *pOwner) :
  GxBaseScrolledWin(pOwner)
{}

GxAppScrolledWin::~GxAppScrolledWin(void)
{}

void GxAppScrolledWin::SetClipWindow(GxWin *pWindow)
{
  pClipWin = pWindow;
}
