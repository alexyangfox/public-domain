//#ifdef LIBGX_DEBUG_BUILD
#include <assert.h>
//#endif //LIBGX_DEBUG_BUILD
#include <string.h>

#include <libGx/GxWinArea.hh>

#include <libGx/GxOwner.hh>
#include <libGx/GxGeomControl.hh>

void GxSetLabel(char *pDest, unsigned destLen,  const char *pContents,
		unsigned &rNewLength)
{
  if(!pDest || !destLen)
    {
      rNewLength = 0; //should not happen.
      return;
    };

  std::string rContents;
  if(pContents)
    rContents = pContents;

  unsigned lengthToWrite = rContents.size();
  if(destLen < rContents.size()+1 )
    lengthToWrite = destLen-1; //we need space to null-terminate

  memcpy(pDest, rContents.c_str(), lengthToWrite);
  pDest[lengthToWrite] = '\0'; //will null terminate even if lengthToWrite is 0;
  rNewLength = lengthToWrite;
}

GxWinArea::~GxWinArea(void)
{
  //the owner might have been deleted
  if(pWinAreaOwner)
    {
      pWinAreaOwner->RemoveChild(this);
      pWinAreaOwner = 0;
    };

  delete pGControl;
  pGControl = 0;
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

UINT GxWinArea::Width(void) const
{
  return width;
}

UINT GxWinArea::Height(void) const
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
  pWinAreaOwner = 0;
}

bool GxWinArea::AcceptFocus(Time eventTime)
{
  return false;
}

UINT GxWinArea::LBorder(void) const
{
  if(!pGControl) return 0;

  return pGControl->LBorder();
}

UINT GxWinArea::RBorder(void) const
{
  if(!pGControl) return 0;

  return pGControl->RBorder();
}

UINT GxWinArea::TBorder(void) const
{
  if(!pGControl) return 0;

  return pGControl->TBorder();
}

UINT GxWinArea::BBorder(void) const
{
  if(!pGControl) return 0;

  return pGControl->BBorder();
}

void GxWinArea::SetGeomControl(const GxGeomControl& rNewControl)
{
  delete pGControl;
  pGControl = 0;

  pGControl = rNewControl.Clone();
}

GxWinArea::GxWinArea(GxOwner *pOwner) :
  pWinAreaOwner(pOwner), width(100), height(100), x(0), y(0), pGControl(0)
{
  assert(pWinAreaOwner); //this test should always be here. (even in release builds)
  //initialized x,y width & height to make sure the values are sane
  pWinAreaOwner->AddChild(this);
}
