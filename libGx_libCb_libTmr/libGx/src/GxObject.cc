#include <libGx/GxWinArea.hh>

#include <libGx/GxOwner.hh>
#include <libGx/GxGeomControl.hh>

GxWinArea::~GxWinArea(void)
{
  //the owner might have been deleted
  if(pOwner)
    pOwner->RemoveChild(this);

  if(pGControl)
    delete pGControl;
}

void GxWinArea::Resize(UINT tWidth, UINT tHeight)
{
  width = tWidth;
  height = tHeight;
}

void GxWinArea::GetSize(UINT &rWidth, UINT &rHeight)
{
  rWidth = width;
  rHeight = height;
}

void GxWinArea::Width(UINT nWidth)
{
  Resize(nWidth, height);
}

void GxWinArea::Height(UINT nHeight)
{
  Resize(width, nHeight);
}

UINT GxWinArea::Width(void)
{
  return width;
}

UINT GxWinArea::Height(void)
{
  return height;
}

void GxWinArea::Move(int newX, int newY)
{
  x = newX;
  y = newY;
}

void GxWinArea::GetPosition(int &rX, int &rY)
{
  rX = x;
  rY = y;
}

void GxWinArea::X(int nX)
{
  Move(nX, y);
}

void GxWinArea::Y(int nY)
{
  Move(x, nY);
}

int GxWinArea::X(void)
{
  return x;
}

int GxWinArea::Y(void)
{
  return y;
}

UINT GxWinArea::GetDesiredWidth(void) const
{
  return width;
}

UINT GxWinArea::GetDesiredHeight(void) const
{
  return height;
}

void GxWinArea::Place(int &lX, int &rX, int &tY, int &bY)
{
  if(pGControl)
    pGControl->PlaceOwner(this, lX,rX, tY,bY);
  else
    {
      //these if's should be asserts?
      if( (rX - lX) < (int)(LBorder() + RBorder()) )
	Width(1);
      else
	Width( (rX - lX) - LBorder() - RBorder() );

      if( (bY - tY) < (int)(TBorder() + BBorder()) )
	Width(1);
      else
	Height( (bY - tY) - TBorder() - BBorder() );

      Move(lX + LBorder(), tY + TBorder());
    };
}

void GxWinArea::Create(void)
{
  
}

void GxWinArea::Display(void)
{

}

void GxWinArea::OwnerDeleted(void)
{
  pOwner = NULL;
}

bool GxWinArea::AcceptFocus(Time eventTime)
{
  return false;
}

UINT GxWinArea::LBorder(void) const
{
  if(!pGControl)
    return 0;

  return pGControl->LBorder();
}

UINT GxWinArea::RBorder(void) const
{
  if(!pGControl)
    return 0;

  return pGControl->RBorder();
}

UINT GxWinArea::TBorder(void) const
{
  if(!pGControl)
    return 0;

  return pGControl->TBorder();
}

UINT GxWinArea::BBorder(void) const
{
  if(!pGControl)
    return 0;

  return pGControl->BBorder();
}

void GxWinArea::SetGeomControl(const GxGeomControl& rNewControl)
{
  if(pGControl)
    delete pGControl;

  pGControl = rNewControl.Clone();
}

GxWinArea::GxWinArea(GxOwner *pTOwner) :
  pOwner(pTOwner), width(100), height(100), x(0), y(0), pGControl(NULL)
{
  //initialized x,y width & height to make sure the values are sane
  pOwner->AddChild(this);
}
